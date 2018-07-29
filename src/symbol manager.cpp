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
    sym::Symbol *symbol = man.lookup(sym::Name(namedType.name), namedType.loc);
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

namespace {

sym::ValueCat refToCat(const ast::ParamRef ref) {
  if (ref == ast::ParamRef::inout) {
    return sym::ValueCat::lvalue_var;
  } else {
    return sym::ValueCat::rvalue;
  }
}

}

sym::FuncParams SymbolMan::funcParams(const ast::FuncParams &params) {
  sym::FuncParams symParams;
  for (const ast::FuncParam &param : params) {
    symParams.push_back({type(param.type), refToCat(param.ref)});
  }
  return symParams;
}

sym::Scope *SymbolMan::current() const {
  return scope;
}

sym::Scope *SymbolMan::current(sym::Scope *const curr) {
  return std::exchange(scope, curr);
}

sym::Scope *SymbolMan::enterScope(const sym::ScopeType type) {
  sym::ScopePtr newScope = std::make_unique<sym::Scope>();
  newScope->type = type;
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
      assert(false);
      return nullptr;
    }
  } else if (std::next(begin) == end) {
    return callFunc(begin->second.get(), name, params, loc);
  } else {
    return findFunc({begin, end}, name, params, loc);
  }
}

sym::Symbol *SymbolMan::memberLookup(
  sym::StructType *strut,
  const sym::Name name,
  const Loc loc
) {
  strut->referenced = true;
  const sym::Table &table = strut->scope->table;
  const auto iter = table.find(name);
  if (iter == table.end()) {
    log.ferror(loc) << "Member \"" << name << "\" not found in struct" << endlog;
  }
  iter->second->referenced = true;
  return iter->second.get();
}

sym::Func *SymbolMan::memberLookup(
  sym::StructType *strut,
  const sym::Name name,
  const sym::FuncParams &params,
  const Loc loc
) {
  strut->referenced = true;
  const sym::Table &table = strut->scope->table;
  const auto [begin, end] = table.equal_range(name);
  if (begin == end) {
    log.ferror(loc) << "Member function \"" << name << "\" not found in struct" << endlog;
    assert(false);
    return nullptr;
  } else if (std::next(begin) == end) {
    return callFunc(begin->second.get(), name, params, loc);
  } else {
    return findFunc({begin, end}, name, params, loc);
  }
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
      if (sameParams(otherFunc->params, func->params)) {
        log.ferror(func->loc) << "Redefinition of function \"" << name << "\". "
          << i->second->loc << endlog;
      }
    }
  }
  scope->table.insert({name, std::move(func)});
}

/// Argument types are convertible to parameter types
bool SymbolMan::convParams(
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  if (params.size() != args.size()) {
    return false;
  }
  for (size_t i = 0; i != params.size(); ++i) {
    if (convertibleTo(args[i].cat, params[i].cat)) {
      if (params[i].cat == sym::ValueCat::rvalue) {
        // @TODO lookup type conversions
     // if (args[i].type is not convertible to params[i].type) {
        if (params[i].type != args[i].type) {
          return false;
        }
      } else if (params[i].type != args[i].type) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

/// Argument types are compatible with parameter types. (Checks ValueCat)
bool SymbolMan::compatParams(
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  if (params.size() != args.size()) {
    return false;
  }
  for (size_t i = 0; i != params.size(); ++i) {
    if (!convertibleTo(args[i].cat, params[i].cat) || params[i].type != args[i].type) {
      return false;
    }
  }
  return true;
}

/// Argument types are the same as parameter types. ValueCat may be different
bool SymbolMan::sameParams(
  const sym::FuncParams &params,
  const sym::FuncParams &args
) {
  if (params.size() != args.size()) {
    return false;
  }
  for (size_t i = 0; i != params.size(); ++i) {
    if (params[i].type != args[i].type) {
      return false;
    }
  }
  return true;
}

sym::Func *SymbolMan::callFunc(
  sym::Symbol *const symbol,
  const sym::Name name,
  const sym::FuncParams &params,
  const Loc loc
) {
  sym::Func *func = dynamic_cast<sym::Func *>(symbol);
  if (func == nullptr) {
    // @TODO check if this is a function object
    log.ferror(loc) << "Calling \"" << name
      << "\" but it is not a function. " << symbol->loc << endlog;
  } else {
    if (convParams(func->params, params)) {
      func->referenced = true;
      return func;
    } else {
      log.ferror(loc) << "No matching call to function \"" << name
        << "\" at " << func->loc << endlog;
    }
  }
  assert(false);
  return nullptr;
}

sym::Func *SymbolMan::findFunc(
  const IterPair pair,
  const sym::Name name,
  const sym::FuncParams &params,
  const Loc loc
) {
  for (auto i = pair.first; i != pair.second; ++i) {
    sym::Func *func = dynamic_cast<sym::Func *>(i->second.get());
    // if there is more than one symbol with the same name then those symbols
    // must be functions
    assert(func);
    if (compatParams(func->params, params)) {
      func->referenced = true;
      return func;
    }
  }
  log.ferror(loc) << "Ambiguous call to function \"" << name << '"' << endlog;
  assert(false);
  return nullptr;
}
