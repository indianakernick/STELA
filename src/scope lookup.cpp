//
//  scope lookup.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "scope lookup.hpp"

#include <cassert>
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
    log.error(loc) << "Calling \"" << key.name
      << "\" but it is not a function. " << symbol->loc << fatal;
  } else {
    if (convParams(func->params, key.params)) {
      return func;
    } else {
      log.error(loc) << "No matching call to function \"" << key.name
        << "\" at " << func->loc << fatal;
    }
  }
}

sym::Func *selectOverload(
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

template <typename Symbol>
Symbol *referTo(Symbol *const symbol) {
  symbol->referenced = true;
  return symbol;
}

class TypeVisitor final : public ast::Visitor {
public:
  explicit TypeVisitor(sym::Scope *const scope, Log &log)
    : lkp{scope, log}, log{log} {}

  void visit(ast::ArrayType &) override {
    assert(false);
  }
  
  void visit(ast::MapType &) override {
    assert(false);
  }
  
  void visit(ast::FuncType &) override {
    assert(false);
  }
  
  void visit(ast::NamedType &named) override {
    named.definition = lkp.lookupIdent(sym::Name(named.name), named.loc);
    type = named.definition;
  }
  
  void visit(ast::NestedType &nested) override {
    lkp.member(sym::Name(nested.name));
    nested.parent->accept(*this);
    nested.definition = lkp.lookupMember(nested.loc);
    type = nested.definition;
  }
  
  sym::Symbol *type;
  
  void enter() {
    lkp.enterSubExpr();
  }
  void leave(const Loc loc) {
    const sym::ExprType etype = lkp.leaveSubExpr();
    if (!etype.typeExpr) {
      log.error(loc) << "Expected type" << fatal;
    }
  }

private:
  ExprLookup lkp;
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

}

sym::Symbol *stela::lookupType(sym::Scope *scope, Log &log, const ast::TypePtr &type) {
  scope = findNearest<sym::NSScope>(scope);
  TypeVisitor visitor{scope, log};
  visitor.enter();
  type->accept(visitor);
  visitor.leave(type->loc);
  return referTo(visitor.type);
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

sym::Func *ExprLookup::lookupFun(
  sym::Scope *scope,
  const sym::FunKey &key,
  const Loc loc
) {
  const std::vector<sym::Symbol *> symbols = findMany(scope, key.name);
  if (symbols.empty()) {
    if (sym::Scope *parent = parentScope(scope)) {
      return lookupFun(parent, key, loc);
    } else {
      log.error(loc) << "Use of undefined symbol \"" << key.name << '"' << fatal;
    }
  } else if (symbols.size() == 1) {
    return referTo(callFunc(log, symbols.front(), key, loc));
  } else {
    return referTo(selectOverload(log, symbols, key, loc));
  }
}

sym::Func *ExprLookup::lookupFun(
  sym::StructScope *scope,
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
  std::vector<sym::Symbol *> candidates;
  std::vector<bool> noAccess;
  for (const sym::StructTableRow &row : scope->table) {
    if (row.name == key.name && row.scope == key.scope) {
      candidates.push_back(row.val.get());
      noAccess.push_back(
        key.access == sym::MemAccess::public_ &&
        row.access == sym::MemAccess::private_
      );
    }
  }
  if (candidates.empty()) {
    log.error(loc) << "No member function \"" << key.name << "\" found in struct" << fatal;
  } else if (candidates.size() == 1) {
    sym::Func *func = callFunc(log, candidates.front(), {key.name, key.params}, loc);
    if (noAccess.front()) {
      log.error(loc) << "Cannot access private member function \"" << key.name << "\" of struct" << fatal;
    }
    return referTo(func);
  } else {
    sym::Func *func = selectOverload(log, candidates, {key.name, key.params}, loc);
    sym::Symbol *const symbol = func;
    for (size_t i = 0; i != candidates.size(); ++i) {
      if (symbol == candidates[i]) {
        if (noAccess[i]) {
          log.error(loc) << "Cannot access private member function \"" << key.name << "\" of struct" << fatal;
        }
        break;
      }
    }
    return referTo(func);
  }
}

sym::Func *ExprLookup::lookupFunc(const sym::FuncParams &params, const Loc loc) {
  if (memFunExpr(Expr::Type::expr)) {
    auto *const strut = dynamic_cast<sym::StructType *>(popType());
    if (strut == nullptr) {
      log.error(loc) << "Can only call member functions on struct objects" << fatal;
    }
    return popCallPushRet(lookupFun(strut->scope, memFunKey(strut, params, etype), loc));
  }
  if (memFunExpr(Expr::Type::static_type)) {
    auto *const strut = dynamic_cast<sym::StructType *>(popType());
    if (strut == nullptr) {
      log.error(loc) << "Can only call static member functions on struct types" << fatal;
    }
    return popCallPushRet(lookupFun(strut->scope, memFunKey(strut, params), loc));
  }
  if (freeFun()) {
    return popCallPushRet(lookupFun(scope, {popName(), params}, loc));
  }
  if (call(Expr::Type::expr)) {
    log.error(loc) << "Overloaded function call operator has not been implemented" << fatal;
  }
  log.error(loc) << "Function call operator applied to invalid subject" << fatal;
}

sym::Func *ExprLookup::lookupFunc(
  const sym::Name &name,
  const sym::FuncParams &params,
  const Loc loc
) {
  sym::Func *const func = lookupFun(scope, {name, params}, loc);
  pushExpr(func->ret);
  return func;
}

void ExprLookup::member(const sym::Name &name) {
  exprs.push_back({Expr::Type::member, name});
}

sym::Symbol *ExprLookup::lookupMem(
  sym::StructScope *scope,
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
    return referTo(row.val.get());
  }
  log.error(loc) << "No member \"" << key.name << "\" found in struct" << fatal;
}

sym::Object *ExprLookup::lookupMem(
  sym::EnumScope *scope,
  const sym::Name &name,
  const Loc loc
) {
  for (const sym::EnumTableRow &row : scope->table) {
    if (row.key == name) {
      auto *object = dynamic_cast<sym::Object *>(row.val.get());
      assert(object);
      return referTo(object);
    }
  }
  log.error(loc) << "No case \"" << name << "\" found in enum" << fatal;
}

sym::Symbol *ExprLookup::lookupMember(const Loc loc) {
  if (memVarExpr(Expr::Type::expr)) {
    auto *strut = dynamic_cast<sym::StructType *>(popType());
    if (strut == nullptr) {
      log.error(loc) << "Can only use . operator on struct objects" << fatal;
    }
    const sym::MemKey key = {
      popName(),
      accessLevel(scope, strut),
      sym::MemScope::instance
    };
    sym::Symbol *const member = lookupMem(strut->scope, key, loc);
    auto *object = dynamic_cast<sym::Object *>(member);
    if (object == nullptr) {
      log.error(loc) << "Member \"" << key.name << "\" is not a variable" << fatal;
    }
    pushExpr(memberType(etype, object->etype));
    return object;
  }
  if (memVarExpr(Expr::Type::static_type)) {
    sym::Symbol *const type = popType();
    if (auto *strut = dynamic_cast<sym::StructType *>(type)) {
      const sym::MemKey key = {
        popName(),
        accessLevel(scope, strut),
        sym::MemScope::static_
      };
      sym::Symbol *const member = lookupMem(strut->scope, key, loc);
      if (auto *func = dynamic_cast<sym::Func *>(member)) {
        log.error(loc) << "Reference to function \"" << key.name << "\" must be called" << fatal;
      }
      if (auto *object = dynamic_cast<sym::Object *>(member)) {
        return pushObj(object);
      } else {
        return pushStatic(member);
      }
    }
    if (auto *enm = dynamic_cast<sym::EnumType *>(type)) {
      return pushObj(lookupMem(enm->scope, popName(), loc));
    }
    log.error(loc) << "Can only access static members of struct and enum types" << fatal;
  }
  return nullptr;
}

sym::Symbol *ExprLookup::lookupIdent(
  sym::Scope *scope,
  const sym::Name &name,
  const Loc loc
) {
  sym::Symbol *const symbol = find(scope, name);
  if (symbol) {
    return referTo(symbol);
  }
  if (sym::Scope *parent = parentScope(scope)) {
    return lookupIdent(parent, name, loc);
  } else {
    log.error(loc) << "Use of undefined symbol \"" << name << '"' << fatal;
  }
}

sym::Symbol *ExprLookup::lookupIdent(const sym::Name &name, const Loc loc) {
  sym::Symbol *symbol = lookupIdent(scope, name, loc);
  if (auto *func = dynamic_cast<sym::Func *>(symbol)) {
    exprs.push_back({Expr::Type::ident, name});
    return nullptr;
  }
  if (auto *object = dynamic_cast<sym::Object *>(symbol)) {
    return pushObj(object);
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

void ExprLookup::setExpr(const sym::ExprType type) {
  assert(exprs.back().type != Expr::Type::expr);
  pushExpr(type);
}

void ExprLookup::enterSubExpr() {
  exprs.push_back({Expr::Type::subexpr});
}

sym::ExprType ExprLookup::leaveSubExpr() {
  assert(exprs.size() >= 2);
  const auto top = exprs.rbegin();
  assert(top[0].type == Expr::Type::expr || top[0].type == Expr::Type::static_type);
  assert(top[1].type == Expr::Type::subexpr);
  exprs.pop_back();
  exprs.pop_back();
  return etype;
}

ExprLookup::Expr::Expr(const Type type)
  : type{type}, name{} {
  assert(
    type == Type::call ||
    type == Type::expr ||
    type == Type::static_type ||
    type == Type::subexpr
  );
}

ExprLookup::Expr::Expr(const Type type, const sym::Name &name)
  : type{type}, name{name} {
  assert(type == Type::member || type == Type::ident);
}

void ExprLookup::pushExpr(const sym::ExprType type) {
  exprs.push_back({Expr::Type::expr});
  etype = type;
}

sym::Object *ExprLookup::pushObj(sym::Object *const obj) {
  pushExpr(obj->etype);
  return obj;
}

sym::Symbol *ExprLookup::pushStatic(sym::Symbol *const type) {
  exprs.push_back({Expr::Type::static_type});
  etype.type = type;
  etype.typeExpr = true;
  return type;
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

bool ExprLookup::call(const Expr::Type type) const {
  const auto top = exprs.rbegin();
  return exprs.size() >= 2
    && top[0].type == type
    && top[1].type == Expr::Type::call
  ;
}

bool ExprLookup::freeFun() const {
  return call(Expr::Type::ident);
}

sym::Name ExprLookup::popName() {
  const Expr &expr = exprs.back();
  assert(expr.type == Expr::Type::member || expr.type == Expr::Type::ident);
  sym::Name name = std::move(expr.name);
  exprs.pop_back();
  return name;
}

sym::Symbol *ExprLookup::popType() {
  assert(!exprs.empty());
  const Expr::Type type = exprs.back().type;
  assert(type == Expr::Type::expr || type == Expr::Type::static_type);
  exprs.pop_back();
  return etype.type;
}

sym::Func *ExprLookup::popCallPushRet(sym::Func *const func) {
  assert(!exprs.empty());
  assert(exprs.back().type == Expr::Type::call);
  exprs.pop_back();
  pushExpr(func->ret);
  return func;
}

sym::MemFunKey ExprLookup::memFunKey(sym::StructType *strut, const sym::FuncParams &params) {
  return {
    popName(),
    params,
    accessLevel(scope, strut),
    sym::MemScope::static_
  };
}

sym::MemFunKey ExprLookup::memFunKey(
  sym::StructType *strut,
  const sym::FuncParams &params,
  const sym::ExprType etype
) {
  sym::MemFunKey key = memFunKey(strut, params);
  key.scope = sym::MemScope::instance;
  key.params.insert(key.params.begin(), etype);
  return key;
}
