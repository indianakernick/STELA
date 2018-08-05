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
  auto *func = dynamic_cast<sym::Func *>(symbol);
  if (func == nullptr) {
    // @TODO check if this is a function object
    log.error(loc) << "Calling \"" << key.name
      << "\" but it is not a function. " << symbol->loc << fatal;
  } else {
    if (convParams(func->params, key.params)) {
      func->referenced = true;
      return func;
    } else {
      log.error(loc) << "No matching call to function \"" << key.name
        << "\" at " << func->loc << fatal;
    }
  }
}

sym::Func *findFunc(
  Log &log,
  const std::vector<sym::Symbol *> &symbols,
  const sym::FunKey &key,
  const Loc loc
) {
  std::vector<sym::Func *> candidates;
  for (sym::Symbol *symbol : symbols) {
    auto *func = dynamic_cast<sym::Func *>(symbol);
    // if there is more than one symbol with the same name then those symbols
    // must be functions
    assert(func);
    if (compatParams(func->params, key.params)) {
      candidates.push_back(func);
    }
  }
  if (candidates.size() == 1) {
    return candidates[0];
  }
  if (candidates.size() == 0) {
    log.error(loc) << "No matching call to overloaded function \"" << key.name << '"' << fatal;
  }
  log.error(loc) << "Ambiguous call to function \"" << key.name << '"' << fatal;
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
    sym::Symbol *symbol = lookupAny(scope, log, sym::Name(namedType.name), namedType.loc);
    if (auto *builtin = dynamic_cast<sym::BuiltinType *>(symbol)) {
      type = symbol;
      namedType.definition = type;
    } else if (auto *strut = dynamic_cast<sym::StructType *>(symbol)) {
      type = symbol;
      namedType.definition = type;
    } else if (auto *num = dynamic_cast<sym::EnumType *>(symbol)) {
      type = symbol;
      namedType.definition = type;
    } else if (auto *alias = dynamic_cast<sym::TypeAlias *>(symbol)) {
      type = alias->type;
      namedType.definition = type;
    } else {
      assert(false);
    }
  }
  
  sym::Symbol *type;

private:
  sym::Scope *scope;
  Log &log;
};

template <typename ScopeType>
ScopeType *findNearest(sym::Scope *const scope) {
  if (scope == nullptr) {
    return nullptr;
  } else if (auto *dynamic = dynamic_cast<ScopeType *>(scope)) {
    return dynamic;
  } else {
    return findNearest<ScopeType>(scope->parent);
  }
}

/// When a symbol is not found in a scope. This visitor determines the parent
/// scope that should be searched.
class ParentScopeVisitor final : public sym::ScopeVisitor {
public:
  ParentScopeVisitor() = default;

  void visit(sym::NSScope &ns) override {
    parent = ns.parent;
  }
  void visit(sym::BlockScope &block) override {
    parent = block.parent;
  }
  void visit(sym::FuncScope &func) override {
    parent = findNearest<sym::NSScope>(func.parent);
  }
  void visit(sym::StructScope &strut) override {
    parent = strut.parent;
  }
  void visit(sym::EnumScope &enm) override {
    parent = enm.parent;
  }
  
  sym::Scope *parent;
};

sym::Scope *parentScope(sym::Scope *const scope) {
  ParentScopeVisitor visitor;
  scope->accept(visitor);
  return visitor.parent;
}

sym::MemAccess accessLevel(sym::Scope *const scope, sym::StructType *const strut) {
  assert(strut);
  assert(strut->scope);
  if (scope == strut->scope) {
    return sym::MemAccess::private_;
  }
  if (findNearest<sym::StructScope>(scope) == strut->scope) {
    return sym::MemAccess::private_;
  }
  return sym::MemAccess::public_;
}

sym::Symbol *lookup(
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
  if (sym::Scope *parent = parentScope(scope)) {
    return lookup(parent, log, name, loc);
  } else {
    log.error(loc) << "Use of undefined symbol \"" << name << '"' << fatal;
  }
}

sym::Func *lookup(
  sym::Scope *scope,
  Log &log,
  const sym::FunKey &key,
  const Loc loc
) {
  const std::vector<sym::Symbol *> symbols = findMany(scope, key.name);
  if (symbols.empty()) {
    if (sym::Scope *parent = parentScope(scope)) {
      return lookup(parent, log, key, loc);
    } else {
      log.error(loc) << "Use of undefined symbol \"" << key.name << '"' << fatal;
    }
  } else if (symbols.size() == 1) {
    return callFunc(log, symbols.front(), key, loc);
  } else {
    return findFunc(log, symbols, key, loc);
  }
}

sym::Symbol *lookup(
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
      log.error(loc) << "Cannot access private member \"" << key.name << "\" of struct" << fatal;
    }
    row.val->referenced = true;
    return row.val.get();
  }
  log.error(loc) << "No member \"" << key.name << "\" found in struct" << fatal;
}

sym::Object *lookup(
  sym::EnumScope *scope,
  Log &log,
  const sym::Name &name,
  const Loc loc
) {
  for (const sym::EnumTableRow &row : scope->table) {
    if (row.key == name) {
      row.val->referenced = true;
      auto *object = dynamic_cast<sym::Object *>(row.val.get());
      // an enum is just a bunch of cases
      // a case is an object
      // an enum is just a bunch of objects
      assert(object);
      return object;
    }
  }
  log.error(loc) << "No case \"" << name << "\" found in enum" << fatal;
}

sym::Func *lookup(
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
    log.error(loc) << "No member function \"" << key.name << "\" found in struct" << fatal;
  } else if (symbols.size() == 1) {
    sym::Func *func = callFunc(log, symbols.front(), {key.name, key.params}, loc);
    if (noAccess.front()) {
      log.error(loc) << "Cannot access private member function \"" << key.name << "\" of struct" << fatal;
    }
    return func;
  } else {
    sym::Func *func = findFunc(log, symbols, {key.name, key.params}, loc);
    sym::Symbol *symbol = func;
    for (size_t i = 0; i != symbols.size(); ++i) {
      if (symbol == symbols[i]) {
        if (noAccess[i]) {
          log.error(loc) << "Cannot access private member function \"" << key.name << "\" of struct" << fatal;
        }
        break;
      }
    }
    return func;
  }
}

}

sym::Symbol *stela::lookupAny(sym::Scope *scope, Log &log, const sym::Name &name, const Loc loc) {
  return lookup(scope, log, name, loc);
}

sym::Symbol *stela::lookupType(sym::Scope *scope, Log &log, const ast::TypePtr &type) {
  TypeVisitor visitor{scope, log};
  type->accept(visitor);
  visitor.type->referenced = true;
  return visitor.type;
}

sym::ValueMut refToMut(const ast::ParamRef ref) {
  if (ref == ast::ParamRef::inout) {
    return sym::ValueMut::var;
  } else {
    return sym::ValueMut::let;
  }
}

sym::ValueRef refToRef(const ast::ParamRef ref) {
  if (ref == ast::ParamRef::inout) {
    return sym::ValueRef::ref;
  } else {
    return sym::ValueRef::val;
  }
}

sym::FuncParams stela::lookupParams(sym::Scope *scope, Log &log, const ast::FuncParams &params) {
  sym::FuncParams symParams;
  for (const ast::FuncParam &param : params) {
    symParams.push_back({
      lookupType(scope, log, param.type),
      refToMut(param.ref),
      refToRef(param.ref)
    });
  }
  return symParams;
}

TypeLookup::TypeLookup(ScopeMan &man, Log &log)
  : man{man}, log{log} {}

sym::Symbol *TypeLookup::lookupBuiltinType(const sym::Name &name, const Loc loc) {
  return lookup(man.builtin(), log, name, loc);
}

sym::Symbol *TypeLookup::lookupType(const ast::TypePtr &astType) {
  return stela::lookupType(man.cur(), log, astType);
}

sym::FuncParams TypeLookup::lookupParams(const ast::FuncParams &params) {
  return stela::lookupParams(man.cur(), log, params);
}

ExprLookup::ExprLookup(sym::Scope *const scope, Log &log)
  : scope{scope}, log{log}, etype{sym::null_type} {}

void ExprLookup::call() {
  exprs.push_back({Expr::Type::call});
}

sym::Func *ExprLookup::lookupFunc(const sym::FuncParams &params, const Loc loc) {
  if (memFunExpr(Expr::Type::expr)) {
    exprs.pop_back(); // pop expr
    auto *strut = dynamic_cast<sym::StructType *>(etype.type);
    if (strut == nullptr) {
      log.error(loc) << "Can only call member functions on struct objects" << fatal;
    }
    sym::MemFunKey key = {
      exprs.back().name,
      params,
      accessLevel(scope, strut),
      sym::MemScope::instance
    };
    key.params.insert(key.params.begin(), etype);
    exprs.pop_back(); // pop member
    exprs.pop_back(); // pop call
    sym::Func *const func = lookup(strut->scope, log, key, loc);
    pushExpr(func->ret);
    return func;
  }
  if (memFunExpr(Expr::Type::static_type)) {
    exprs.pop_back(); // pop static_type
    auto *strut = dynamic_cast<sym::StructType *>(etype.type);
    if (strut == nullptr) {
      log.error(loc) << "Can only call static member functions on struct types" << fatal;
    }
    const sym::MemFunKey key = {
      exprs.back().name,
      params,
      accessLevel(scope, strut),
      sym::MemScope::static_
    };
    exprs.pop_back(); // pop member
    exprs.pop_back(); // pop call
    sym::Func *const func = lookup(strut->scope, log, key, loc);
    pushExpr(func->ret);
    return func;
  }
  if (freeFun()) {
    const sym::FunKey key = {
      exprs.back().name,
      params
    };
    exprs.pop_back(); // pop ident
    exprs.pop_back(); // pop call
    sym::Func *const func = lookup(scope, log, key, loc);
    pushExpr(func->ret);
    return func;
  }
  log.error(loc) << "Overloaded function call operator has not been implemented" << fatal;
}

sym::Func *ExprLookup::lookupFunc(
  const sym::Name &name,
  const sym::FuncParams &params,
  const Loc loc
) {
  sym::Func *const func = lookup(scope, log, {name, params}, loc);
  pushExpr(func->ret);
  return func;
}

void ExprLookup::member(const sym::Name &name) {
  exprs.push_back({Expr::Type::member, name});
}

sym::Symbol *ExprLookup::lookupMember(const Loc loc) {
  if (memVarExpr(Expr::Type::expr)) {
    exprs.pop_back(); // pop expr
    auto *strut = dynamic_cast<sym::StructType *>(etype.type);
    if (strut == nullptr) {
      log.error(loc) << "Can only use . operator on struct objects" << fatal;
    }
    const sym::MemKey key = {
      exprs.back().name,
      accessLevel(scope, strut),
      sym::MemScope::instance
    };
    exprs.pop_back(); // pop member
    sym::Symbol *const member = lookup(strut->scope, log, key, loc);
    auto *object = dynamic_cast<sym::Object *>(member);
    if (object == nullptr) {
      log.error(loc) << "Member \"" << key.name << "\" is not a variable" << fatal;
    }
    pushExpr(memberType(etype, object->etype));
    return object;
  }
  if (memVarExpr(Expr::Type::static_type)) {
    exprs.pop_back(); // pop static_type
    if (auto *strut = dynamic_cast<sym::StructType *>(etype.type)) {
      const sym::MemKey key = {
        exprs.back().name,
        accessLevel(scope, strut),
        sym::MemScope::static_
      };
      exprs.pop_back(); // pop member
      sym::Symbol *const member = lookup(strut->scope, log, key, loc);
      auto *object = dynamic_cast<sym::Object *>(member);
      if (object == nullptr) {
        log.error(loc) << "Static member \"" << key.name << "\" is not a variable" << fatal;
      }
      pushExpr(object->etype);
      return object;
    }
    if (auto *enm = dynamic_cast<sym::EnumType *>(etype.type)) {
      sym::Object *const cse = lookup(enm->scope, log, exprs.back().name, loc);
      exprs.pop_back(); // pop member
      pushExpr(cse->etype);
      return cse;
    }
    log.error(loc) << "Can only access static members of struct and enum types" << fatal;
  }
  return nullptr;
}

sym::Symbol *ExprLookup::lookupIdent(const sym::Name &name, const Loc loc) {
  sym::Symbol *symbol = lookup(scope, log, name, loc);
  if (auto *func = dynamic_cast<sym::Func *>(symbol)) {
    exprs.push_back({Expr::Type::ident, name});
    return nullptr;
  }
  if (auto *object = dynamic_cast<sym::Object *>(symbol)) {
    pushExpr(object->etype);
    return object;
  }
  if (auto *alias = dynamic_cast<sym::TypeAlias *>(symbol)) {
    symbol = alias->type;
  }
  // symbol must be a StructType or EnumType
  pushStatic(symbol);
  return symbol;
}

sym::Symbol *ExprLookup::lookupSelf(const Loc loc) {
  return lookupIdent("self", loc);
}

void ExprLookup::setExprType(const sym::ExprType type) {
  if (exprs.empty() || exprs.back().type != Expr::Type::expr) {
    pushExpr(type);
  }
}

sym::ExprType ExprLookup::getExprType() {
  if (!exprs.empty() && exprs.back().type == Expr::Type::expr && currentEtype) {
    currentEtype = false;
    if (exprs.size() == 1) {
      exprs.clear();
    }
    return etype;
  } else {
    return sym::null_type;
  }
}

ExprLookup::Expr::Expr(const Type type)
  : type{type}, name{} {
  assert(type == Type::call || type == Type::expr || type == Type::static_type);
}

ExprLookup::Expr::Expr(const Type type, const sym::Name &name)
  : type{type}, name{name} {
  assert(type == Type::member || type == Type::ident);
}

void ExprLookup::pushExpr(const sym::ExprType type) {
  exprs.push_back({Expr::Type::expr});
  etype = type;
  currentEtype = true;
}

void ExprLookup::pushStatic(sym::Symbol *const type) {
  exprs.push_back({Expr::Type::static_type});
  etype.type = type;
  currentEtype = false;
}

bool ExprLookup::memVarExpr(const Expr::Type type) const {
  const auto top = exprs.rbegin();
  return exprs.size() >= 2
    && top[0].type == type
    && top[1].type == Expr::Type::member
    && (exprs.size() == 2 || top[2].type != Expr::Type::call)
  ;
}

bool ExprLookup::memFunExpr(const Expr::Type type) const {
  const auto top = exprs.rbegin();
  return exprs.size() >= 3
    && top[0].type == type
    && top[1].type == Expr::Type::member
    && top[2].type == Expr::Type::call
  ;
}

bool ExprLookup::freeFun() const {
  const auto top = exprs.rbegin();
  return exprs.size() >= 2
    && top[0].type == Expr::Type::ident
    && top[1].type == Expr::Type::call
  ;
}
