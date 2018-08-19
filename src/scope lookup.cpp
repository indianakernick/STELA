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

template <typename Derived, typename Base>
Derived *assertDownCast(Base *const base) {
  assert(base);
  Derived *const derived = dynamic_cast<Derived *>(base);
  assert(derived);
  return derived;
}

sym::Func *selectOverload(
  Log &log,
  const std::vector<sym::Symbol *> &symbols,
  const sym::FunKey &key,
  const Loc loc
) {
  assert(!symbols.empty());
  for (sym::Symbol *symbol : symbols) {
    auto *func = dynamic_cast<sym::Func *>(symbol);
    if (func == nullptr) {
      log.error(loc) << "Calling \"" << key.name
        << "\" but it is not a function. " << symbols.front()->loc << fatal;
    }
    if (compatParams(func->params, key.params)) {
      return func;
    }
  }
  log.error(loc) << "No matching call to function \"" << key.name << '"' << fatal;
}

template <typename Symbol>
Symbol *referTo(Symbol *const symbol) {
  symbol->referenced = true;
  return symbol;
}

std::string_view scopeName(const sym::MemScope scope) {
  return scope == sym::MemScope::instance ? "instance" : "static";
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
    if (etype.cat != sym::ExprCat::type) {
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

template <typename... ScopeType>
sym::Scope *findNearestNot(sym::Scope *const scope) {
  if (scope == nullptr) {
    return nullptr;
  } else if ((dynamic_cast<ScopeType *>(scope) || ...)) {
    return findNearestNot<ScopeType...>(scope->parent);
  } else {
    return scope;
  }
}

sym::Scope *parentScope(sym::Scope *const scope) {
  sym::FuncScope *func = dynamic_cast<sym::FuncScope *>(scope);
  if (func) {
    return findNearestNot<sym::FuncScope, sym::BlockScope>(func->parent);
  } else {
    return scope->parent;
  }
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
  } else {
    return referTo(selectOverload(log, symbols, key, loc));
  }
}

sym::Func *ExprLookup::lookupFun(
  sym::StructScope *scope,
  const sym::MemFunKey &key,
  const Loc loc
) {
  for (const sym::StructTableRow &row : scope->table) {
    if (row.name != key.name || row.scope != key.scope) {
      continue;
    }
    auto *func = dynamic_cast<sym::Func *>(row.val.get());
    if (func == nullptr) {
      log.error(loc) << "Calling \"" << key.name
        << "\" but it is not a function. " << row.val->loc << fatal;
    }
    if (!compatParams(func->params, key.params)) {
      continue;
    }
    if (key.access == sym::MemAccess::public_ && row.access == sym::MemAccess::private_) {
      log.error(loc) << "Cannot access private member function \""
        << key.name << "\" of struct" << fatal;
    }
    return referTo(func);
  }
  log.error(loc) << "No matching " << scopeName(key.scope) << " member function \""
    << key.name << "\" found in struct" << fatal;
}

sym::Func *ExprLookup::lookupFunc(const sym::FuncParams &params, const Loc loc) {
  if (memFunExpr(Expr::Type::expr)) {
    auto *const strut = dynamic_cast<sym::StructType *>(popType());
    if (strut == nullptr) {
      log.error(loc) << "Can only call member functions on struct objects" << fatal;
    }
    return popCallPushRet(lookupFun(strut->scope, iMemFunKey(strut, params), loc));
  }
  if (memFunExpr(Expr::Type::type)) {
    auto *const strut = dynamic_cast<sym::StructType *>(popType());
    if (strut == nullptr) {
      log.error(loc) << "Can only call static member functions on struct types" << fatal;
    }
    return popCallPushRet(lookupFun(strut->scope, sMemFunKey(strut, params), loc));
  }
  if (call(Expr::Type::free_fun)) {
    return popCallPushRet(lookupFun(scope, {popName(), params}, loc));
  }
  if (call(Expr::Type::type)) {
    auto *const strut = dynamic_cast<sym::StructType *>(popType());
    if (strut == nullptr) {
      log.error(loc) << "Can only call constructor of struct" << fatal;
    }
    sym::MemFunKey key = {
      "init", params, accessLevel(scope, strut), sym::MemScope::instance
    };
    key.params.insert(key.params.begin(), {strut, sym::ValueMut::var, sym::ValueRef::ref});
    return popCallPushRet(lookupFun(strut->scope, key, loc));
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
    if (row.name != key.name || row.scope != key.scope) {
      continue;
    }
    if (key.access == sym::MemAccess::public_ && row.access == sym::MemAccess::private_) {
      log.error(loc) << "Cannot access private member \"" << key.name << "\" of struct" << fatal;
    }
    return referTo(row.val.get());
  }
  log.error(loc) << "No " << scopeName(key.scope) << " member \""
    << key.name << "\" found in struct" << fatal;
}

sym::Object *ExprLookup::lookupMem(
  sym::EnumScope *scope,
  const sym::Name &name,
  const Loc loc
) {
  for (const sym::EnumTableRow &row : scope->table) {
    if (row.key == name) {
      return referTo(assertDownCast<sym::Object>(row.val.get()));
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
    const sym::MemKey key = iMemKey(strut);
    sym::Symbol *const member = lookupMem(strut->scope, key, loc);
    if (auto *func = dynamic_cast<sym::Func *>(member)) {
      log.error(loc) << "Reference to " << scopeName(key.scope) << " member function \""
        << key.name << "\" must be called" << fatal;
    }
    sym::Object *const object = assertDownCast<sym::Object>(member);
    pushExpr(memberType(etype, object->etype));
    return object;
  }
  if (memVarExpr(Expr::Type::type)) {
    sym::Symbol *const type = popType();
    if (auto *strut = dynamic_cast<sym::StructType *>(type)) {
      const sym::MemKey key = sMemKey(strut);
      sym::Symbol *const member = lookupMem(strut->scope, key, loc);
      if (auto *func = dynamic_cast<sym::Func *>(member)) {
        log.error(loc) << "Reference to " << scopeName(key.scope) << " member function \""
          << key.name << "\" must be called" << fatal;
      } else if (auto *object = dynamic_cast<sym::Object *>(member)) {
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
  if (memFunExpr(Expr::Type::type)) {
    sym::Symbol *const type = popType();
    if (auto *strut = dynamic_cast<sym::StructType *>(type)) {
      const sym::MemKey key = sMemKey(strut);
      sym::Symbol *const member = lookupMem(strut->scope, key, loc);
      if (auto *innerStrut = dynamic_cast<sym::StructType *>(member)) {
        return pushStatic(innerStrut);
      }
      this->member(key.name);
    }
    pushStatic(type);
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
    return symbol;
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
    if (exprs.back().type != Expr::Type::call) {
      log.error(loc) << "Reference to function \"" << name << "\" must be called" << fatal;
    }
    exprs.push_back({Expr::Type::free_fun, name});
    return nullptr;
  }
  if (auto *object = dynamic_cast<sym::Object *>(symbol)) {
    return pushObj(referTo(object));
  }
  if (auto *alias = dynamic_cast<sym::TypeAlias *>(symbol)) {
    symbol = referTo(alias)->type;
  }
  // symbol must be a StructType or EnumType
  pushStatic(referTo(symbol));
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
  assert(top[0].type == Expr::Type::expr || top[0].type == Expr::Type::type);
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
    type == Type::type ||
    type == Type::subexpr
  );
}

ExprLookup::Expr::Expr(const Type type, const sym::Name &name)
  : type{type}, name{name} {
  assert(type == Type::member || type == Type::free_fun);
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
  exprs.push_back({Expr::Type::type});
  etype.type = type;
  etype.cat = sym::ExprCat::type;
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

sym::Name ExprLookup::popName() {
  const Expr &expr = exprs.back();
  assert(expr.type == Expr::Type::member || expr.type == Expr::Type::free_fun);
  sym::Name name = std::move(expr.name);
  exprs.pop_back();
  return name;
}

sym::Symbol *ExprLookup::popType() {
  assert(!exprs.empty());
  const Expr::Type type = exprs.back().type;
  assert(type == Expr::Type::expr || type == Expr::Type::type);
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

sym::MemFunKey ExprLookup::sMemFunKey(sym::StructType *strut, const sym::FuncParams &params) {
  return {
    popName(),
    params,
    accessLevel(scope, strut),
    sym::MemScope::static_
  };
}

sym::MemFunKey ExprLookup::iMemFunKey(sym::StructType *strut, const sym::FuncParams &params) {
  sym::MemFunKey key = sMemFunKey(strut, params);
  key.scope = sym::MemScope::instance;
  key.params.insert(key.params.begin(), etype);
  return key;
}

sym::MemKey ExprLookup::sMemKey(sym::StructType *const strut) {
  return {
    popName(),
    accessLevel(scope, strut),
    sym::MemScope::static_
  };
}

sym::MemKey ExprLookup::iMemKey(sym::StructType *const strut) {
  sym::MemKey key = sMemKey(strut);
  key.scope = sym::MemScope::instance;
  return key;
}
