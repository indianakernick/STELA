//
//  scope insert.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "scope insert.hpp"

#include <cassert>
#include "scope lookup.hpp"
#include "compare types.hpp"
#include "compare params args.hpp"

using namespace stela;

namespace {

auto makeParam(const sym::ExprType &etype, ast::FuncParam &param, sym::Scope *scope) {
  auto paramSym = std::make_unique<sym::Object>();
  param.symbol = paramSym.get();
  paramSym->scope = scope;
  paramSym->loc = param.loc;
  paramSym->etype = etype;
  paramSym->node = {retain, &param};
  return paramSym;
}

sym::ValueRef convertRef(const ast::ParamRef ref) {
  if (ref == ast::ParamRef::ref) {
    return sym::ValueRef::ref;
  } else {
    return sym::ValueRef::val;
  }
}

sym::ExprType convert(sym::Ctx ctx, const ast::FuncParam &param) {
  return convert(ctx, param.type, param.ref);
}

void convertParams(sym::FuncParams &symParams, sym::Ctx ctx, const ast::FuncParams &params) {
  for (const ast::FuncParam &param : params) {
    symParams.push_back(convert(ctx, param));
  }
}

sym::FuncParams convertParams(sym::Ctx ctx, const ast::FuncParams &params) {
  sym::FuncParams symParams;
  convertParams(symParams, ctx, params);
  return symParams;
}

sym::FuncParams convertParams(
  sym::Ctx ctx,
  const ast::Receiver &receiver,
  const ast::FuncParams &params
) {
  sym::FuncParams symParams;
  if (receiver) {
    symParams.push_back(convert(ctx, *receiver));
  } else {
    symParams.push_back(sym::null_type);
  }
  convertParams(symParams, ctx, params);
  return symParams;
}

sym::FuncParams convertParams(
  sym::Ctx ctx,
  const ast::ParamType &receiver,
  const ast::ParamTypes &params
) {
  sym::FuncParams symParams;
  if (receiver.type) {
    symParams.push_back(convert(ctx, receiver.type, receiver.ref));
  } else {
    symParams.push_back(sym::null_type);
  }
  for (const ast::ParamType &param : params) {
    symParams.push_back(convert(ctx, param.type, param.ref));
  }
  return symParams;
}

}

sym::ExprType stela::convert(sym::Ctx ctx, const ast::TypePtr &type, const ast::ParamRef ref) {
  assert(type);
  validateType(ctx, type);
  return {type, sym::ValueMut::var, convertRef(ref)};
}

sym::ExprType stela::convertNullable(sym::Ctx ctx, const ast::TypePtr &type, const ast::ParamRef ref) {
  if (type == nullptr) {
    return {nullptr, sym::ValueMut::let, convertRef(ref)};
  } else {
    validateType(ctx, type);
    return {type, sym::ValueMut::let, convertRef(ref)};
  }
}

void stela::insert(sym::Ctx ctx, const sym::Name &name, sym::SymbolPtr symbol) {
  const auto iter = ctx.man.cur()->table.find(name);
  if (iter != ctx.man.cur()->table.end()) {
    ctx.log.error(symbol->loc) << "Redefinition of symbol \"" << name
      << "\" previously declared at " << moduleName(ctx.man.cur()) << ':'
      << iter->second->loc << fatal;
  } else {
    checkIdentShadow(ctx, name, symbol->loc);
    ctx.man.cur()->table.insert({name, std::move(symbol)});
  }
}

namespace {

void checkMethodCollide(sym::Ctx ctx, ast::Name name, const ast::TypePtr &receiver, Loc loc) {
  if (auto strut = lookupConcrete<ast::StructType>(ctx, receiver)) {
    for (const ast::Field &field : strut->fields) {
      if (field.name == name) {
        ctx.log.error(loc) << "Colliding function and field \"" << name << "\"" << fatal;
      }
    }
  } else if (auto user = lookupConcrete<ast::UserType>(ctx, receiver)) {
    for (const ast::UserField &field : user->fields) {
      if (field.name == name) {
        ctx.log.error(loc) << "Colliding function and field \"" << name << "\"" << fatal;
      }
    }
  }
}

void checkFuncCollide(sym::Ctx ctx, ast::Name name, sym::Func *funcSym) {
  const auto [beg, end] = ctx.man.cur()->table.equal_range(sym::Name{name});
  for (auto s = beg; s != end; ++s) {
    sym::Symbol *const symbol = s->second.get();
    sym::Func *const dupFunc = dynamic_cast<sym::Func *>(symbol);
    if (dupFunc) {
      if (sameParams(ctx, dupFunc->params, funcSym->params)) {
        ctx.log.error(funcSym->loc) << "Redefinition of function \"" << name
          << "\" previously declared at " << moduleName(ctx.man.cur()) << ':'
          << symbol->loc << fatal;
      }
    } else {
      ctx.log.error(funcSym->loc) << "Redefinition of function \"" << name
        << "\" previously declared (as a different kind of symbol) at "
        << moduleName(ctx.man.cur()) << ':' << symbol->loc << fatal;
    }
  }
}

template <typename FuncNode>
sym::Func *insertFunc(
  sym::Ctx ctx,
  std::unique_ptr<sym::Func> funcSym,
  FuncNode &func
) {
  funcSym->params = convertParams(ctx, func.receiver, func.params);
  funcSym->ret = convertNullable(ctx, func.ret, ast::ParamRef::val);
  funcSym->node = {retain, &func};
  funcSym->loc = func.loc;
  func.symbol = funcSym.get();
  checkFuncCollide(ctx, func.name, funcSym.get());
  checkIdentShadow(ctx, sym::Name{func.name}, func.loc);
  sym::Func *const ret = funcSym.get();
  ctx.man.cur()->table.insert({sym::Name{func.name}, std::move(funcSym)});
  return ret;
}

}

sym::Func *stela::insert(sym::Ctx ctx, ast::Func &func) {
  if (ctx.man.cur()->type != sym::ScopeType::ns) {
    ctx.log.error(func.loc) << "Functions must appear at global scope" << fatal;
  }
  auto funcSym = std::make_unique<sym::Func>();
  funcSym->referenced = func.external;
  if (func.receiver) {
    checkMethodCollide(ctx, func.name, func.receiver->type, func.loc);
  }
  return insertFunc(ctx, std::move(funcSym), func);
}

void stela::enterFuncScope(sym::Func *funcSym, ast::Func &func) {
  sym::Scope *scope = funcSym->scope;
  for (size_t i = 0; i != func.params.size(); ++i) {
    scope->table.insert({
      sym::Name{func.params[i].name},
      makeParam(funcSym->params[i + 1], func.params[i], scope)
    });
  }
  if (func.receiver) {
    ast::FuncParam &param = *func.receiver;
    scope->table.insert({
      sym::Name{param.name},
      makeParam(funcSym->params[0], param, scope)
    });
  }
}

void stela::leaveFuncScope(sym::Ctx ctx, sym::Func *funcSym, ast::Func &func) {
  if (!funcSym->ret.type) {
    funcSym->ret.type = ctx.btn.Void;
  }
  func.ret = funcSym->ret.type;
}

sym::Lambda *stela::insert(sym::Ctx ctx, ast::Lambda &lam) {
  auto lamSym = std::make_unique<sym::Lambda>();
  lam.symbol = lamSym.get();
  lamSym->loc = lam.loc;
  lamSym->referenced = true;
  lamSym->params = convertParams(ctx, lam.params);
  lamSym->ret = convertNullable(ctx, lam.ret, ast::ParamRef::val);
  lamSym->node = {retain, &lam};
  sym::Lambda *const ret = lamSym.get();
  ctx.man.cur()->table.insert({"$lambda", std::move(lamSym)});
  return ret;
}

void stela::enterLambdaScope(sym::Lambda *lamSym, ast::Lambda &lam) {
  sym::Scope *scope = lamSym->scope;
  for (size_t i = 0; i != lam.params.size(); ++i) {
    scope->table.insert({
      sym::Name{lam.params[i].name},
      makeParam(lamSym->params[i], lam.params[i], scope)
    });
  }
}

void stela::leaveLambdaScope(sym::Ctx ctx, sym::Lambda *lamSym, ast::Lambda &lam) {
  if (!lamSym->ret.type) {
    lamSym->ret.type = ctx.btn.Void;
  }
  lam.ret = lamSym->ret.type;
}

void stela::insert(sym::Ctx ctx, ast::ExtFunc &func) {
  if (ctx.man.cur()->type != sym::ScopeType::ns) {
    ctx.log.error(func.loc) << "External functions must appear at global scope" << fatal;
  }
  auto funcSym = std::make_unique<sym::Func>();
  funcSym->referenced = false;
  if (func.receiver.type) {
    checkMethodCollide(ctx, func.name, func.receiver.type, func.loc);
  }
  insertFunc(ctx, std::move(funcSym), func);
}
