//
//  scope lookup.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "scope lookup.hpp"

#include "scope find.hpp"
#include "compare params args.hpp"

using namespace stela;

namespace {

sym::Func *callFunc(
  Log &log,
  sym::Symbol *const symbol,
  const sym::FunKey &key,
  const Loc loc
) {
  sym::Func *func = dynamic_cast<sym::Func *>(symbol);
  if (func == nullptr) {
    // @TODO check if this is a function object
    log.ferror(loc) << "Calling \"" << key.name
      << "\" but it is not a function. " << symbol->loc << endlog;
  } else {
    if (convParams(func->params, key.params)) {
      func->referenced = true;
      return func;
    } else {
      log.ferror(loc) << "No matching call to function \"" << key.name
        << "\" at " << func->loc << endlog;
    }
  }
  assert(false);
  return nullptr;
}

sym::Func *findFunc(
  Log &log,
  const std::vector<sym::Symbol *> &symbols,
  const sym::FunKey &key,
  const Loc loc
) {
  for (sym::Symbol *symbol : symbols) {
    sym::Func *func = dynamic_cast<sym::Func *>(symbol);
    // if there is more than one symbol with the same name then those symbols
    // must be functions
    assert(func);
    if (compatParams(func->params, key.params)) {
      func->referenced = true;
      return func;
    }
  }
  log.ferror(loc) << "Ambiguous call to function \"" << key.name << '"' << endlog;
  assert(false);
  return nullptr;
}

class TypeVisitor final : public ast::Visitor {
public:
  explicit TypeVisitor(sym::Scope *scope, Log &log)
    : scope{scope}, log{log} {}

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
    sym::Symbol *symbol = lookup(scope, log, sym::Name(namedType.name), namedType.loc);
    if (auto *builtin = dynamic_cast<sym::BuiltinType *>(symbol)) {
      type = symbol;
      namedType.definition = symbol;
    } else if (auto *strut = dynamic_cast<sym::StructType *>(symbol)) {
      type = symbol;
      namedType.definition = symbol;
    } else if (auto *num = dynamic_cast<sym::EnumType *>(symbol)) {
      type = symbol;
      namedType.definition = symbol;
    } else if (auto *alias = dynamic_cast<sym::TypeAlias *>(symbol)) {
      type = alias->type;
      namedType.definition = alias->type;
    } else {
      assert(false);
    }
  }
  
  sym::Symbol *type;

private:
  sym::Scope *scope;
  Log &log;
};

sym::ValueCat refToCat(const ast::ParamRef ref) {
  if (ref == ast::ParamRef::inout) {
    return sym::ValueCat::lvalue_var;
  } else {
    return sym::ValueCat::lvalue_let;
  }
}

}

sym::Symbol *stela::lookup(
  sym::Scope *scope,
  Log &log,
  const sym::Name name,
  const Loc loc
) {
  sym::Symbol *const symbol = find(scope, name);
  if (symbol) {
    symbol->referenced = true;
    return symbol;
  }
  if (scope->parent) {
    return lookup(scope->parent, log, name, loc);
  } else {
    log.ferror(loc) << "Use of undefined symbol \"" << name << '"' << endlog;
    assert(false);
    return nullptr;
  }
}

sym::Func *stela::lookup(
  sym::Scope *scope,
  Log &log,
  const sym::FunKey &key,
  const Loc loc
) {
  const std::vector<sym::Symbol *> symbols = findMany(scope, key.name);
  if (symbols.empty()) {
    if (scope->parent) {
      sym::Scope *const current = std::exchange(scope, scope->parent);
      sym::Func *const func = lookup(scope, log, key, loc);
      scope = current;
      return func;
    } else {
      log.ferror(loc) << "Use of undefined symbol \"" << key.name << '"' << endlog;
      assert(false);
      return nullptr;
    }
  } else if (symbols.size() == 1) {
    return callFunc(log, symbols.front(), key, loc);
  } else {
    return findFunc(log, symbols, key, loc);
  }
}

sym::Symbol *stela::lookup(
  sym::StructScope *scope,
  Log &log,
  const sym::MemKey &key,
  const Loc loc
) {
  for (const sym::StructTableRow &row : scope->table) {
    if (row.name != key.name) {
      continue;
    }
    assert(row.scope == key.scope);
    if (key.access == sym::MemAccess::public_ && row.access == sym::MemAccess::private_) {
      log.ferror(loc) << "Cannot access private member \"" << key.name << "\" of struct" << endlog;
    }
    row.val->referenced = true;
    return row.val.get();
  }
  log.ferror(loc) << "No member \"" << key.name << "\" found in struct" << endlog;
  assert(false);
  return nullptr;
}

sym::Func *stela::lookup(
  sym::StructScope *scope,
  Log &log,
  const sym::MemFunKey &key,
  const Loc loc
) {
  /*
  
  need to perform function lookup as if all functions are accessible
  
  struct Test {
    private func doThing(n: Double);
    public func doThing(n: Float);
  }
  
  func main() {
    Test test;
    test.doThing(0); // 0 is an Int
                     // Should get "ambiguous call to function"
  }
  
  */
  std::vector<sym::Symbol *> symbols;
  std::vector<bool> noAccess;
  for (const sym::StructTableRow &row : scope->table) {
    if (row.name == key.name && row.scope == key.scope) {
      symbols.push_back(row.val.get());
      noAccess.push_back(
        key.access == sym::MemAccess::public_ &&
        row.access == sym::MemAccess::private_
      );
    }
  }
  if (symbols.empty()) {
    log.ferror(loc) << "No member function \"" << key.name << "\" found in struct" << endlog;
    assert(false);
    return nullptr;
  } else if (symbols.size() == 1) {
    sym::Func *func = callFunc(log, symbols.front(), {key.name, key.params}, loc);
    if (noAccess.front()) {
      log.ferror(loc) << "Cannot access private member function \"" << key.name << "\" of struct" << endlog;
    }
    return func;
  } else {
    sym::Func *func = findFunc(log, symbols, {key.name, key.params}, loc);
    sym::Symbol *symbol = func;
    for (size_t i = 0; i != symbols.size(); ++i) {
      if (symbol == symbols[i]) {
        if (noAccess[i]) {
          log.ferror(loc) << "Cannot access private member function \"" << key.name << "\" of struct" << endlog;
        }
        break;
      }
    }
    return func;
  }
}

sym::Symbol *stela::type(sym::Scope *scope, Log &log, const ast::TypePtr &type) {
  TypeVisitor visitor{scope, log};
  type->accept(visitor);
  visitor.type->referenced = true;
  return visitor.type;
}

sym::FuncParams stela::funcParams(sym::Scope *scope, Log &log, const ast::FuncParams &params) {
  sym::FuncParams symParams;
  for (const ast::FuncParam &param : params) {
    symParams.push_back({type(scope, log, param.type), refToCat(param.ref)});
  }
  return symParams;
}
