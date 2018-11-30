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
#include <Simpleton/Utils/algorithm.hpp>

using namespace stela;

namespace {

auto makeParam(const sym::ExprType &etype, ast::FuncParam &param) {
  auto paramSym = std::make_unique<sym::Object>();
  param.symbol = paramSym.get();
  paramSym->loc = param.loc;
  paramSym->etype = etype;
  paramSym->node = {retain, &param};
  return paramSym;
}

sym::ValueRef convertRef(const ast::ParamRef ref) {
  if (ref == ast::ParamRef::inout) {
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

}

sym::ExprType stela::convert(sym::Ctx ctx, const ast::TypePtr &type, const ast::ParamRef ref) {
  validateType(ctx, type);
  return {
    type,
    sym::ValueMut::var,
    convertRef(ref)
  };
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

sym::Func *stela::insert(sym::Ctx ctx, ast::Func &func) {
  if (ctx.man.cur()->type != sym::ScopeType::ns) {
    ctx.log.error(func.loc) << "Functions must appear at global scope" << fatal;
  }
  auto funcSym = std::make_unique<sym::Func>();
  func.symbol = funcSym.get();
  funcSym->loc = func.loc;
  if (func.receiver) {
    if (auto strut = lookupConcrete<ast::StructType>(ctx, func.receiver->type)) {
      for (const ast::Field &field : strut->fields) {
        if (field.name == func.name) {
          ctx.log.error(func.loc) << "Colliding function and field \"" << func.name << "\"" << fatal;
        }
      }
    }
  }
  funcSym->params = convertParams(ctx, func.receiver, func.params);
  funcSym->ret = convert(ctx, func.ret, ast::ParamRef::value);
  funcSym->node = {retain, &func};
  const auto [beg, end] = ctx.man.cur()->table.equal_range(sym::Name{func.name});
  for (auto s = beg; s != end; ++s) {
    sym::Symbol *const symbol = s->second.get();
    sym::Func *const dupFunc = dynamic_cast<sym::Func *>(symbol);
    if (dupFunc) {
      if (sameParams(ctx, dupFunc->params, funcSym->params)) {
        ctx.log.error(funcSym->loc) << "Redefinition of function \"" << func.name
          << "\" previously declared at " << moduleName(ctx.man.cur()) << ':'
          << symbol->loc << fatal;
      }
    } else {
      ctx.log.error(funcSym->loc) << "Redefinition of function \"" << func.name
        << "\" previously declared (as a different kind of symbol) at "
        << moduleName(ctx.man.cur()) << ':' << symbol->loc << fatal;
    }
  }
  checkIdentShadow(ctx, sym::Name{func.name}, func.loc);
  sym::Func *const ret = funcSym.get();
  ctx.man.cur()->table.insert({sym::Name{func.name}, std::move(funcSym)});
  return ret;
}

void stela::enterFuncScope(sym::Func *funcSym, ast::Func &func) {
  for (size_t i = 0; i != func.params.size(); ++i) {
    funcSym->scope->table.insert({
      sym::Name{func.params[i].name},
      makeParam(funcSym->params[i + 1], func.params[i])
    });
  }
  if (func.receiver) {
    ast::FuncParam &param = *func.receiver;
    funcSym->scope->table.insert({
      sym::Name{param.name},
      makeParam(funcSym->params[0], param)
    });
  }
}

sym::Lambda *stela::insert(sym::Ctx ctx, ast::Lambda &lam) {
  auto lamSym = std::make_unique<sym::Lambda>();
  lam.symbol = lamSym.get();
  lamSym->loc = lam.loc;
  lamSym->referenced = true;
  lamSym->params = convertParams(ctx, lam.params);
  lamSym->ret = convert(ctx, lam.ret, ast::ParamRef::value);
  lamSym->node = {retain, &lam};
  sym::Lambda *const ret = lamSym.get();
  ctx.man.cur()->table.insert({"$lambda", std::move(lamSym)});
  return ret;
}

void stela::enterLambdaScope(sym::Lambda *lamSym, ast::Lambda &lam) {
  for (size_t i = 0; i != lam.params.size(); ++i) {
    lamSym->scope->table.insert({
      sym::Name{lam.params[i].name},
      makeParam(lamSym->params[i], lam.params[i])
    });
  }
}
