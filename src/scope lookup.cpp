//
//  scope lookup.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "scope lookup.hpp"

#include <cassert>
#include "compare types.hpp"
#include "scope traverse.hpp"
#include "compare params args.hpp"
#include <Simpleton/Utils/algorithm.hpp>

using namespace stela;

namespace {

template <typename Symbol>
Symbol *referTo(Symbol *const symbol) {
  symbol->referenced = true;
  return symbol;
}

sym::Symbol *find(sym::Scope *scope, const ast::Name name) {
  const auto iter = scope->table.find(sym::Name{name});
  if (iter == scope->table.end()) {
    return nullptr;
  } else {
    return iter->second.get();
  }
}

ast::TypeAlias *lookupTypeImpl(Log &log, sym::Scope *scope, ast::NamedType &type) {
  if (sym::Symbol *symbol = find(scope, type.name)) {
    if (auto *alias = dynamic_cast<sym::TypeAlias *>(symbol)) {
      type.definition = alias->node.get();
      alias->referenced = true;
      return type.definition;
    }
    log.error(type.loc) << "The name \"" << type.name << "\" does not refer to a type" << fatal;
  }
  if (sym::Scope *parent = scope->parent) {
    return lookupTypeImpl(log, parent, type);
  } else {
    log.error(type.loc) << "Expected type name but found \"" << type.name << "\"" << fatal;
  }
}

void validateStruct(sym::Ctx ctx, const retain_ptr<ast::StructType> &strut) {
  std::vector<std::pair<std::string_view, Loc>> names;
  names.reserve(strut->fields.size());
  for (const ast::Field &field : strut->fields) {
    validateType(ctx, field.type);
    names.push_back({field.name, field.loc});
  }
  Utils::sort(names, [] (auto a, auto b) {
    return a.first < b.first;
  });
  const auto dup = Utils::adjacent_find(names, [] (auto a, auto b) {
    return a.first == b.first;
  });
  if (dup != names.cend()) {
    ctx.log.error(dup->second) << "Duplicate field \"" << dup->first << "\" in struct" << fatal;
  }
}

sym::Scope *getModuleScope(sym::Ctx ctx, const ast::Name module, const Loc loc) {
  if (module.empty()) {
    return ctx.man.cur();
  }
  auto iter = ctx.mods.find(sym::Name{module});
  if (iter == ctx.mods.end()) {
    ctx.log.error(loc) << "Module \"" << module << "\" is not imported by this module" << fatal;
  }
  return iter->second.scopes[0].get();
}

}

ast::TypeAlias *stela::lookupTypeName(sym::Ctx ctx, ast::NamedType &type) {
  if (type.definition) {
    return type.definition;
  } else {
    sym::Scope *scope = getModuleScope(ctx, type.module, type.loc);
    return lookupTypeImpl(ctx.log, scope, type);
  }
}

ast::TypePtr stela::lookupStrongType(sym::Ctx ctx, const ast::TypePtr &type) {
  if (auto named = dynamic_pointer_cast<ast::NamedType>(type)) {
    ast::TypeAlias *alias = lookupTypeName(ctx, *named);
    if (alias->strong) {
      return type;
    } else {
      return lookupStrongType(ctx, alias->type);
    }
  } else {
    return type;
  }
}

ast::TypePtr stela::lookupConcreteType(sym::Ctx ctx, const ast::TypePtr &type) {
  if (auto named = dynamic_pointer_cast<ast::NamedType>(type)) {
    ast::TypeAlias *alias = lookupTypeName(ctx, *named);
    return lookupConcreteType(ctx, alias->type);
  } else {
    return type;
  }
}

ast::TypePtr stela::validateType(sym::Ctx ctx, const ast::TypePtr &type) {
  ast::TypePtr concrete = lookupConcreteType(ctx, type);
  if (auto strut = dynamic_pointer_cast<ast::StructType>(type)) {
    validateStruct(ctx, strut);
  }
  return concrete;
}

ExprLookup::ExprLookup(sym::Ctx ctx)
  : ctx{ctx}, etype{sym::null_type} {}

void ExprLookup::call() {
  exprs.push_back(Expr{Expr::Type::call});
}

sym::Func *ExprLookup::lookupFun(
  sym::Scope *scope,
  const FunKey &key,
  const Loc loc
) {
  const auto [begin, end] = scope->table.equal_range(key.name);
  if (begin == end) {
    if (sym::Scope *parent = scope->parent) {
      return lookupFun(parent, key, loc);
    } else {
      ctx.log.error(loc) << "Use of undefined symbol \"" << key.name << '"' << fatal;
    }
  } else {
    for (auto s = begin; s != end; ++s) {
      sym::Symbol *const symbol = s->second.get();
      auto *func = dynamic_cast<sym::Func *>(symbol);
      if (func == nullptr) {
        ctx.log.error(loc) << "Calling \"" << key.name
          << "\" but it is not a function. " << symbol->loc << fatal;
      }
      if (compatParams(ctx, func->params, key.params)) {
        return referTo(func);
      }
    }
    ctx.log.error(loc) << "No matching call to function \"" << key.name << '"' << fatal;
  }
}

ExprLookup::FunKey ExprLookup::funKey(const sym::ExprType etype, const sym::FuncParams &params) {
  FunKey key;
  key.params.reserve(1 + params.size());
  key.params.push_back(etype);
  key.params.insert(key.params.end(), params.cbegin(), params.cend());
  key.name = popName();
  return key;
}

ast::Func *ExprLookup::lookupFunc(const sym::FuncParams &params, const Loc loc) {
  if (memFunExpr(Expr::Type::expr)) {
    return popCallPushRet(lookupFun(ctx.man.cur(), funKey(popExpr(), params), loc));
  }
  if (call(Expr::Type::free_fun)) {
    sym::Scope *const scope = exprs.back().scope;
    return popCallPushRet(lookupFun(scope, funKey(sym::null_type, params), loc));
  }
  if (call(Expr::Type::expr)) {
    ctx.log.error(loc) << "Calling an object. Might be a function pointer. Who knows!" << fatal;
  }
  ctx.log.error(loc) << "Function call operator applied to invalid subject" << fatal;
}

void ExprLookup::member(const sym::Name &name) {
  exprs.push_back({Expr::Type::member, name});
}

ast::Field *ExprLookup::lookupMember(const Loc loc) {
  if (memVarExpr(Expr::Type::expr)) {
    ast::TypePtr type = lookupConcreteType(ctx, popExpr().type);
    const sym::Name name = popName();
    if (type == sym::void_type.type) {
      ctx.log.error(loc) << "Cannot access field \"" << name << "\" of void expression" << fatal;
    }
    if (auto strut = dynamic_pointer_cast<ast::StructType>(std::move(type))) {
      for (ast::Field &field : strut->fields) {
        if (field.name == name) {
          pushExpr(memberType(etype, field.type));
          return &field;
        }
      }
      ctx.log.error(loc) << "No field \"" << name << "\" found in struct" << fatal;
    } else {
      ctx.log.error(loc) << "Cannot access field \"" << name << "\" of non-struct expression" << fatal;
    }
  }
  return nullptr;
}

namespace {

struct SymbolScope {
  sym::Symbol *symbol;
  sym::Scope *scope;
};

// find an identifier by traversing up the scopes
SymbolScope findIdent(sym::Scope *scope, const sym::Name &name) {
  if (sym::Symbol *symbol = find(scope, name)) {
    return {symbol, scope};
  }
  if (sym::Scope *parent = scope->parent) {
    return findIdent(parent, name);
  } else {
    return {nullptr, nullptr};
  }
}

// determine whether a variable in the given scope is reachable from the
// current scope
bool reachableObject(sym::Scope *current, sym::Scope *scope) {
  if (current == nullptr) {
    return false;
  }
  if (current == scope) {
    return true;
  }
  if (current->type == sym::Scope::Type::func) {
    return reachableObject(findNearest(sym::Scope::Type::ns, current), scope);
  }
  return reachableObject(current->parent, scope);
}

}

ast::Statement *ExprLookup::lookupIdent(const sym::Name &module, const sym::Name &name, const Loc loc) {
  sym::Scope *currentScope = getModuleScope(ctx, module, loc);
  const auto [symbol, scope] = findIdent(currentScope, name);
  if (symbol == nullptr) {
    ctx.log.error(loc) << "Use of undefined symbol \"" << name << '"' << fatal;
  }
  if (auto *object = dynamic_cast<sym::Object *>(symbol)) {
    if (!reachableObject(currentScope, scope)) {
      ctx.log.error(loc) << "Variable \"" << name << "\" at " << object->loc << " is unreachable" << fatal;
    }
    return pushObj(referTo(object));
  }
  if (exprs.back().type == Expr::Type::call) {
    exprs.push_back({Expr::Type::free_fun, name, scope});
    return nullptr;
  }
  if (auto *func = dynamic_cast<sym::Func *>(symbol)) {
    return pushFunPtr(scope, name, loc);
  }
  ctx.log.error(loc) << "Expected variable or function name but found \"" << name << "\"" << fatal;
}

void ExprLookup::setExpr(const sym::ExprType type) {
  assert(exprs.back().type != Expr::Type::expr);
  pushExpr(type);
}

void ExprLookup::enterSubExpr() {
  exprs.push_back(Expr{Expr::Type::subexpr});
}

sym::ExprType ExprLookup::leaveSubExpr() {
  assert(exprs.size() >= 2);
  const auto top = exprs.rbegin();
  assert(top[0].type == Expr::Type::expr);
  assert(top[1].type == Expr::Type::subexpr);
  exprs.pop_back();
  exprs.pop_back();
  return etype;
}

void ExprLookup::expected(const ast::TypePtr &type) {
  expType = type;
}

ExprLookup::Expr::Expr(const Type type)
  : type{type}, name{}, scope{nullptr} {
  assert(
    type == Type::call ||
    type == Type::expr ||
    type == Type::subexpr
  );
}

ExprLookup::Expr::Expr(const Type type, const sym::Name &name)
  : type{type}, name{name}, scope{nullptr} {
  assert(type == Type::member);
}

ExprLookup::Expr::Expr(const Type type, const sym::Name &name, sym::Scope *scope)
  : type{type}, name{name}, scope{scope} {
  assert(type == Type::free_fun);
}

void ExprLookup::pushExpr(const sym::ExprType &type) {
  exprs.push_back(Expr{Expr::Type::expr});
  etype = type;
}

ast::Statement *ExprLookup::pushObj(sym::Object *const obj) {
  pushExpr(obj->etype);
  return obj->node.get();
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

sym::ExprType ExprLookup::popExpr() {
  assert(!exprs.empty());
  assert(exprs.back().type == Expr::Type::expr);
  exprs.pop_back();
  return etype;
}

ast::Func *ExprLookup::popCallPushRet(sym::Func *const func) {
  assert(!exprs.empty());
  assert(exprs.back().type == Expr::Type::call);
  exprs.pop_back();
  pushExpr(func->ret.type ? func->ret : sym::void_type);
  return func->node.get();
}

namespace {

stela::retain_ptr<ast::FuncType> getFuncType(Log &log, sym::Func *funcSym, Loc loc) {
  if (funcSym->params[0].type) {
    log.error(loc) << "Cannot take address of member function" << fatal;
  }
  auto funcType = make_retain<ast::FuncType>();
  funcType->params.reserve(funcSym->params.size() - 1);
  for (auto p = funcSym->params.cbegin() + 1; p != funcSym->params.cend(); ++p) {
    ast::ParamType param;
    param.type = p->type;
    param.ref = p->ref == sym::ValueRef::ref
              ? ast::ParamRef::inout
              : ast::ParamRef::value;
    funcType->params.push_back(std::move(param));
  }
  if (funcSym->ret.type == sym::void_type.type) {
    funcType->ret = nullptr;
  } else {
    funcType->ret = funcSym->ret.type;
  }
  return funcType;
}

}

ast::Func *ExprLookup::pushFunPtr(sym::Scope *scope, const sym::Name &name, const Loc loc) {
  const auto [begin, end] = scope->table.equal_range(name);
  assert(begin != end);
  if (std::next(begin) == end) {
    auto *funcSym = dynamic_cast<sym::Func *>(begin->second.get());
    assert(funcSym);
    auto funcType = getFuncType(ctx.log, funcSym, loc);
    if (expType && !compareTypes(ctx, expType, funcType)) {
      ctx.log.error(loc) << "Function \"" << name << "\" does not match signature" << fatal;
    }
    pushExpr(sym::makeLetVal(std::move(funcType)));
    return funcSym->node.get();
  } else {
    if (!expType) {
      ctx.log.error(loc) << "Ambiguous reference to overloaded function \"" << name << '"' << fatal;
    }
    for (auto f = begin; f != end; ++f) {
      auto *funcSym = dynamic_cast<sym::Func *>(f->second.get());
      assert(funcSym);
      auto funcType = getFuncType(ctx.log, funcSym, loc);
      if (compareTypes(ctx, expType, funcType)) {
        pushExpr(sym::makeLetVal(std::move(funcType)));
        return funcSym->node.get();
      }
    }
    ctx.log.error(loc) << "No overload of function \"" << name << "\" matches signature" << fatal;
  }
}
