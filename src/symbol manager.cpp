//
//  symbol manager.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "symbol manager.hpp"

using namespace stela;

namespace {

/*class TypeNameVisitor final : public ast::Visitor {
public:
  explicit TypeNameVisitor(SymbolMan &man)
    : man{man} {}

  void visit(ast::ArrayType &array) override {
    std::string newName;
    newName += '[';
    assert(array.elem);
    array.elem->accept(*this);
    newName += std::move(name);
    newName += ']';
    name = std::move(newName);
  }
  
  void visit(ast::MapType &map) override {
    std::string newName;
    newName += "[{";
    assert(map.key);
    map.key->accept(*this);
    newName += std::move(name);
    newName += ':';
    assert(map.val);
    map.val->accept(*this);
    newName += std::move(name);
    newName += "}]";
    name = std::move(newName);
  }
  
  void visit(ast::FuncType &func) override {
    std::string newName;
    newName += '(';
    if (!func.params.empty()) {
      for (auto p = func.params.begin(); p != func.params.end(); ++p) {
        if (p->ref == ast::ParamRef::inout) {
          newName += "inout ";
        }
        assert(p->type);
        p->type->accept(*this);
        newName += std::move(name);
        if (p != func.params.end() - 1) {
          newName += ',';
        }
      }
    }
    newName += "->";
    assert(func.ret);
    func.ret->accept(*this);
    newName += std::move(name);
    name = std::move(newName);
  }
  
  void visit(ast::NamedType &namedType) override {
    sym::Symbol *const sym = man.lookup(namedType.name, namedType.loc);
    if (auto *alias = dynamic_cast<sym::TypeAlias *>(sym); alias) {
      name = alias->type;
    } else {
      name = std::string(namedType.name);
    }
  }
  
  std::string name;

private:
  SymbolMan &man;
};*/

class TypeVisitor final : public ast::Visitor {
public:
  explicit TypeVisitor(SymbolMan &man)
    : man{man} {}

  void visit(ast::ArrayType &) override {
    assert(false);
  }
  
  void visit(ast::MapType &) override {
    assert(false);
  }
  
  void visit(ast::FuncType &) override {
    assert(false);
  }
  
  void visit(ast::NamedType &namedType) override {
    sym::Symbol *symbol = man.lookup(namedType.name, namedType.loc);
    if (auto *builtin = dynamic_cast<sym::BuiltinType *>(symbol)) {
      type = symbol;
    } else if (auto *strut = dynamic_cast<sym::StructType *>(symbol)) {
      type = symbol;
    } else if (auto *num = dynamic_cast<sym::EnumType *>(symbol)) {
      type = symbol;
    } else if (auto *alias = dynamic_cast<sym::TypeAlias *>(symbol)) {
      type = alias->type;
    } else {
      assert(false);
    }
  }
  
  sym::Symbol *type;

private:
  SymbolMan &man;
};

}

SymbolMan::SymbolMan(sym::Scopes &scopes, Log &log)
  : scopes{scopes}, scope{scopes.back().get()}, log{log} {}

Log &SymbolMan::logger() {
  return log;
}

sym::Symbol *SymbolMan::type(const ast::TypePtr &type) {
  TypeVisitor visitor{*this};
  type->accept(visitor);
  return visitor.type;
}

sym::FuncParams SymbolMan::funcParams(const ast::FuncParams &params) {
  sym::FuncParams symParams;
  for (const ast::FuncParam &param : params) {
    symParams.push_back(type(param.type));
  }
  return symParams;
}

sym::Scope *SymbolMan::enterScope() {
  sym::ScopePtr newScope = std::make_unique<sym::Scope>();
  newScope->parent = scope;
  scope = newScope.get();
  scopes.push_back(std::move(newScope));
  return scope;
}

void SymbolMan::leaveScope() {
  scope = scope->parent;
}

sym::Symbol *SymbolMan::lookup(const sym::Name name, const Loc loc) {
  const auto iter = scope->table.find(name);
  if (iter == scope->table.end()) {
    if (scope->parent) {
      sym::Scope *const current = std::exchange(scope, scope->parent);
      sym::Symbol *const symbol = lookup(name, loc);
      scope = current;
      return symbol;
    } else {
      log.ferror(loc) << "Use of undefined symbol \"" << name << '"' << endlog;
    }
  }
  iter->second->referenced = true;
  return iter->second.get();
}

sym::Func *SymbolMan::lookup(
  const sym::Name name,
  const sym::FuncParams &params,
  const Loc loc
) {
  const auto [begin, end] = scope->table.equal_range(name);
  if (begin == end) {
    if (scope->parent) {
      sym::Scope *const current = std::exchange(scope, scope->parent);
      sym::Func *const func = lookup(name, params, loc);
      scope = current;
      return func;
    } else {
      log.ferror(loc) << "Use of undefined symbol \"" << name << '"' << endlog;
    }
  } else if (std::next(begin) == end) {
    sym::Func *func = dynamic_cast<sym::Func *>(begin->second.get());
    if (func == nullptr) {
      // @TODO check if this is a function object
      log.ferror(loc) << "Calling \"" << name
        << "\" but it is not a function. " << begin->second->loc << endlog;
    } else {
      if (func->params == params) {
        return func;
      } else {
        // @TODO lookup type conversions
        log.ferror(loc) << "No matching call to function \"" << name
          << "\" at " << func->loc << endlog;
      }
    }
  } else {
    for (auto i = begin; i != end; ++i) {
      sym::Func *func = dynamic_cast<sym::Func *>(i->second.get());
      // if there is more than one symbol with the same name then those symbols
      // must be functions
      assert(func);
      if (func->params == params) {
        return func;
      }
    }
    log.ferror(loc) << "Ambiguous call to function \"" << name << '"' << endlog;
  }
  return nullptr;
}

void SymbolMan::insert(const sym::Name name, sym::SymbolPtr symbol) {
  const auto iter = scope->table.find(name);
  if (iter == scope->table.end()) {
    scope->table.insert({name, std::move(symbol)});
  } else {
    log.ferror(symbol->loc) << "Redefinition of symbol \"" << name << "\" "
      << iter->second->loc << endlog;
  }
}

void SymbolMan::insert(const sym::Name name, sym::FuncPtr func) {
  const auto [begin, end] = scope->table.equal_range(name);
  for (auto i = begin; i != end; ++i) {
    sym::Func *otherFunc = dynamic_cast<sym::Func *>(i->second.get());
    if (otherFunc == nullptr) {
      log.ferror(func->loc) << "Redefinition of function \"" << name
        << "\" as a different kind of symbol. " << i->second->loc << endlog;
    } else {
      if (otherFunc->params == func->params) {
        log.ferror(func->loc) << "Redefinition of function \"" << name << "\". "
          << i->second->loc << endlog;
      }
    }
  }
  scope->table.insert({name, std::move(func)});
}
