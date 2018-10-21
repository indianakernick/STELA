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
#include "compare params args.hpp"

using namespace stela;

namespace {

sym::FuncPtr makeFunc(const Loc loc) {
  auto funcSym = std::make_unique<sym::Func>();
  funcSym->loc = loc;
  return funcSym;
}

sym::ExprType retType(const ast::Func &func) {
  return {func.ret.get(), sym::ValueMut::let, sym::ValueRef::val};
}

auto makeParam(const sym::ExprType &etype, const ast::FuncParam &param) {
  auto paramSym = std::make_unique<sym::Object>();
  paramSym->loc = param.loc;
  paramSym->etype = etype;
  paramSym->node = const_cast<ast::FuncParam *>(&param);
  return paramSym;
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

sym::ExprType convert(const NameLookup &tlk, const ast::FuncParam &param) {
  tlk.validateType(param.type.get());
  return {
    param.type.get(),
    refToMut(param.ref),
    refToRef(param.ref)
  };
}

sym::FuncParams convertParams(
  const NameLookup &tlk,
  const ast::Receiver &receiver,
  const ast::FuncParams &params
) {
  sym::FuncParams symParams;
  if (receiver) {
    symParams.push_back(convert(tlk, *receiver));
  } else {
    symParams.push_back(sym::null_type);
  }
  for (const ast::FuncParam &param : params) {
    symParams.push_back(convert(tlk, param));
  }
  return symParams;
}

}

void InserterManager::insert(const sym::Name &name, sym::SymbolPtr symbol) {
  const auto iter = man.cur()->table.find(name);
  if (iter != man.cur()->table.end()) {
    log.error(symbol->loc) << "Redefinition of symbol \"" << name
      << "\" previously declared at " << iter->second->loc << fatal;
  } else {
    man.cur()->table.insert({name, std::move(symbol)});
  }
}

sym::Func *InserterManager::insert(const ast::Func &func) {
  sym::FuncPtr funcSym = makeFunc(func.loc);
  if (func.receiver) {
    if (auto *strut = tlk.lookupConcrete<ast::StructType>(func.receiver->type.get())) {
      for (const ast::Field &field : strut->fields) {
        if (field.name == func.name) {
          log.error(func.loc) << "Colliding function and field \"" << func.name << "\"" << fatal;
        }
      }
    }
  }
  funcSym->params = convertParams(tlk, func.receiver, func.params);
  funcSym->ret = retType(func);
  funcSym->node = const_cast<ast::Func *>(&func);
  const auto [beg, end] = man.cur()->table.equal_range(sym::Name(func.name));
  for (auto s = beg; s != end; ++s) {
    sym::Symbol *const symbol = s->second.get();
    sym::Func *const dupFunc = dynamic_cast<sym::Func *>(symbol);
    if (dupFunc) {
      if (sameParams(tlk, dupFunc->params, funcSym->params)) {
        log.error(funcSym->loc) << "Redefinition of function \"" << func.name
          << "\" previously declared at " << symbol->loc << fatal;
      }
    } else {
      log.error(funcSym->loc) << "Redefinition of function \"" << func.name
        << "\" previously declared (as a different kind of symbol) at "
        << symbol->loc << fatal;
    }
  }
  sym::Func *const ret = funcSym.get();
  man.cur()->table.insert({sym::Name(func.name), std::move(funcSym)});
  return ret;
}

void InserterManager::enterFuncScope(sym::Func *funcSym, const ast::Func &func) {
  for (size_t i = 0; i != func.params.size(); ++i) {
    funcSym->scope->table.insert({
      sym::Name(func.params[i].name),
      makeParam(funcSym->params[i + 1], func.params[i])
    });
  }
  if (func.receiver) {
    const ast::FuncParam &param = *func.receiver;
    funcSym->scope->table.insert({
      sym::Name(param.name),
      makeParam(funcSym->params[0], param)
    });
  }
}

InserterManager::InserterManager(ScopeMan &man, Log &log)
  : log{log}, man{man}, tlk{man, log} {}
