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
  return paramSym;
}

}

void InserterManager::insert(const sym::Name &name, sym::SymbolPtr symbol) {
  const auto iter = scope()->table.find(name);
  if (iter != scope()->table.end()) {
    log.error(symbol->loc) << "Redefinition of symbol \"" << name
      << "\" previously declared at " << iter->second->loc << fatal;
  } else {
    scope()->table.insert({name, std::move(symbol)});
  }
}

sym::Func *InserterManager::insert(const ast::Func &func) {
  sym::FuncPtr funcSym = makeFunc(func.loc);
  funcSym->params = convertParams(func.receiver, func.params);
  funcSym->ret = retType(func);
  const auto [beg, end] = scope()->table.equal_range(sym::Name(func.name));
  for (auto s = beg; s != end; ++s) {
    sym::Symbol *const symbol = s->second.get();
    sym::Func *const dupFunc = dynamic_cast<sym::Func *>(symbol);
    if (dupFunc) {
      if (sameParams(lkp, dupFunc->params, funcSym->params)) {
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
  scope()->table.insert({sym::Name(func.name), std::move(funcSym)});
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

InserterManager::InserterManager(sym::Scope *scope, Log &log)
  : log{log}, lkp{scope, log} {
  push(scope);
}

void InserterManager::push(sym::Scope *const scope) {
  assert(scope);
  stack.push_back(scope);
}

void InserterManager::pop() {
  assert(!stack.empty());
  stack.pop_back();
}

sym::Scope *InserterManager::scope() const {
  assert(!stack.empty());
  return stack.back();
}
