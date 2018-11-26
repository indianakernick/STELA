//
//  expr lookup.cpp
//  STELA
//
//  Created by Indi Kernick on 26/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "expr lookup.hpp"

#include <cassert>
#include "scope lookup.hpp"
#include "scope insert.hpp"
#include "compare types.hpp"
#include "scope traverse.hpp"
#include "compare params args.hpp"

using namespace stela;

ExprLookup::ExprLookup(sym::Ctx ctx)
  : ctx{ctx} {}

void ExprLookup::call() {
  stack.pushCall();
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
      if (compatParams(ctx, func->params, key.args)) {
        func->referenced = true;
        return func;
      }
    }
    ctx.log.error(loc) << "No matching call to function \"" << key.name << '"' << fatal;
  }
}

namespace {

sym::FuncParams pushReceiver(const sym::ExprType &receiver, const sym::FuncParams &args) {
  sym::FuncParams recArgs;
  recArgs.reserve(1 + args.size());
  recArgs.push_back(receiver);
  recArgs.insert(recArgs.end(), args.cbegin(), args.cend());
  return recArgs;
}

}

ast::Func *ExprLookup::lookupFunc(const sym::FuncParams &args, const Loc loc) {
  if (stack.memFunExpr(ExprKind::expr)) {
    sym::ExprType receiver = stack.popExpr();
    sym::Name name = stack.popMember();
    const FunKey key {name, pushReceiver(receiver, args)};
    return popCallPushRet(lookupFun(ctx.man.cur(), key, loc));
  }
  if (stack.call(ExprKind::free_fun)) {
    sym::Name name = stack.popFunc();
    const FunKey key {name, pushReceiver(sym::null_type, args)};
    return popCallPushRet(lookupFun(ctx.man.cur(), key, loc));
  }
  if (stack.call(ExprKind::expr)) {
    sym::ExprType expr = stack.popExpr();
    stack.popCall();
    auto func = dynamic_pointer_cast<ast::FuncType>(std::move(expr.type));
    if (!func) {
      ctx.log.error(loc) << "Calling an expression but it is not a function object" << fatal;
    }
    sym::FuncParams params;
    for (const ast::ParamType &param : func->params) {
      params.push_back(convert(ctx, param.type, param.ref));
    }
    if (!compatParams(ctx, params, args)) {
      ctx.log.error(loc) << "No matching call to function object" << fatal;
    }
    stack.pushExpr(func->ret ? convert(ctx, func->ret, ast::ParamRef::value) : sym::void_type);
    return nullptr;
  }
  /* LCOV_EXCL_START */
  assert(false);
  return nullptr;
  /* LCOV_EXCL_END */
}

void ExprLookup::member(const sym::Name &name) {
  stack.pushMember(name);
}

ast::Field *ExprLookup::lookupMember(const Loc loc) {
  if (stack.memVarExpr(ExprKind::expr)) {
    ast::TypePtr type = lookupConcreteType(ctx, stack.popExpr().type);
    const sym::Name name = stack.popMember();
    if (type == sym::void_type.type) {
      ctx.log.error(loc) << "Cannot access field \"" << name << "\" of void expression" << fatal;
    }
    if (auto strut = dynamic_pointer_cast<ast::StructType>(std::move(type))) {
      for (ast::Field &field : strut->fields) {
        if (field.name == name) {
          stack.pushMemberExpr(field.type);
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

}

ast::Statement *ExprLookup::lookupIdent(const sym::Name &name, const Loc loc) {
  sym::Scope *currentScope = ctx.man.cur();
  const auto [symbol, scope] = findIdent(currentScope, name);
  if (symbol == nullptr) {
    ctx.log.error(loc) << "Use of undefined symbol \"" << name << '"' << fatal;
  }
  if (auto *object = dynamic_cast<sym::Object *>(symbol)) {
    // don't capture globals
    if (scope->type != sym::ScopeType::ns) {
      // make sure we're inside a closure
      if (sym::Scope *closureScope = findNearest(sym::ScopeType::closure, currentScope)) {
        assert(closureScope->symbol);
        auto lamSym = dynamic_cast<sym::Lambda *>(closureScope->symbol);
        assert(lamSym);
        lamSym->captures.push_back(object);
      }
    }
    object->referenced = true;
    stack.pushExpr(object->etype);
    return object->node.get();
  }
  if (stack.top() == ExprKind::call) {
    stack.pushFunc(name);
    return nullptr;
  }
  if (auto *func = dynamic_cast<sym::Func *>(symbol)) {
    return pushFunPtr(scope, name, loc);
  }
  ctx.log.error(loc) << "Expected variable or function name but found \"" << name << "\"" << fatal;
}

void ExprLookup::setExpr(sym::ExprType type) {
  stack.setExpr(std::move(type));
}

void ExprLookup::enterSubExpr() {
  stack.enterSubExpr();
}

sym::ExprType ExprLookup::leaveSubExpr() {
  return stack.leaveSubExpr();
}

void ExprLookup::expected(const ast::TypePtr &type) {
  expType = type;
}

ast::Func *ExprLookup::popCallPushRet(sym::Func *const func) {
  stack.popCall();
  stack.pushExpr(func->ret.type ? func->ret : sym::void_type);
  return func->node.get();
}

ast::Func *ExprLookup::pushFunPtr(sym::Scope *scope, const sym::Name &name, const Loc loc) {
  const auto [begin, end] = scope->table.equal_range(name);
  assert(begin != end);
  if (std::next(begin) == end) {
    auto *funcSym = dynamic_cast<sym::Func *>(begin->second.get());
    assert(funcSym);
    auto funcType = getFuncType(ctx.log, *funcSym->node, loc);
    if (expType && !compareTypes(ctx, expType, funcType)) {
      ctx.log.error(loc) << "Function \"" << name << "\" does not match signature" << fatal;
    }
    stack.pushExpr(sym::makeLetVal(std::move(funcType)));
    return funcSym->node.get();
  } else {
    if (!expType) {
      ctx.log.error(loc) << "Ambiguous reference to overloaded function \"" << name << '"' << fatal;
    }
    for (auto f = begin; f != end; ++f) {
      auto *funcSym = dynamic_cast<sym::Func *>(f->second.get());
      assert(funcSym);
      auto funcType = getFuncType(ctx.log, *funcSym->node, loc);
      if (compareTypes(ctx, expType, funcType)) {
        stack.pushExpr(sym::makeLetVal(std::move(funcType)));
        return funcSym->node.get();
      }
    }
    ctx.log.error(loc) << "No overload of function \"" << name << "\" matches signature" << fatal;
  }
}
