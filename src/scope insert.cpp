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
#include "member access scope.hpp"
#include "compare params args.hpp"

using namespace stela;

namespace {

sym::FuncPtr makeFunc(const Loc loc) {
  auto funcSym = std::make_unique<sym::Func>();
  funcSym->loc = loc;
  return funcSym;
}

sym::ExprType inferRetType(
  sym::Scope *scope,
  Log &log,
  const ast::Func &func,
  const BuiltinTypes &bnt
) {
  if (func.ret) {
    return {lookupType(scope, log, func.ret), sym::ValueMut::let, sym::ValueRef::val};
  } else {
    // @TODO infer return type
    log.warn(func.loc) << "Return type is Void by default" << endlog;
    return {bnt.Void, sym::ValueMut::let, sym::ValueRef::val};
  }
}

sym::ExprType selfType(sym::StructType *const structType, const ast::MemMut mut) {
  return {
    structType,
    mut == ast::MemMut::mutating ? sym::ValueMut::var : sym::ValueMut::let,
    sym::ValueRef::ref
  };
}

auto makeSelf(sym::Func &funcSym, sym::StructType *const structType, const ast::MemMut mut) {
  auto self = std::make_unique<sym::Object>();
  self->loc = funcSym.loc;
  self->etype = selfType(structType, mut);
  return self;
}

auto makeParam(const sym::ExprType &etype, const ast::FuncParam &param) {
  auto paramSym = std::make_unique<sym::Object>();
  paramSym->loc = param.loc;
  paramSym->etype = etype;
  return paramSym;
}

}

UnorderedInserter::UnorderedInserter(sym::UnorderedScope *scope, Log &log)
  : scope{scope}, log{log} {
  assert(scope);
}

void UnorderedInserter::insert(const sym::Name &name, sym::SymbolPtr symbol) {
  const auto iter = scope->table.find(name);
  if (iter != scope->table.end()) {
    log.error(symbol->loc) << "Redefinition of symbol \"" << name
      << "\" previously declared at " << iter->second->loc << fatal;
  } else {
    scope->table.insert({name, std::move(symbol)});
  }
}

sym::Func *UnorderedInserter::insert(const ast::Func &func, const BuiltinTypes &bnt) {
  sym::FuncPtr funcSym = makeFunc(func.loc);
  funcSym->params = lookupParams(scope, log, func.params);
  funcSym->ret = inferRetType(scope, log, func, bnt);
  const auto [beg, end] = scope->table.equal_range(sym::Name(func.name));
  for (auto s = beg; s != end; ++s) {
    sym::Symbol *const symbol = s->second.get();
    sym::Func *const dupFunc = dynamic_cast<sym::Func *>(symbol);
    if (dupFunc) {
      if (sameParams(dupFunc->params, funcSym->params)) {
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
  scope->table.insert({sym::Name(func.name), std::move(funcSym)});
  return ret;
}

void UnorderedInserter::enterFuncScope(sym::Func *funcSym, const ast::Func &func) {
  for (size_t i = 0; i != funcSym->params.size(); ++i) {
    funcSym->scope->table.insert({
      sym::Name(func.params[i].name),
      makeParam(funcSym->params[i], func.params[i])
    });
  }
}

StructInserter::StructInserter(sym::StructType *strut, Log &log)
  : strut{strut}, log{log}, access{}, scope{} {
  assert(strut);
}

void StructInserter::insert(const sym::Name &name, sym::SymbolPtr symbol) {
  for (const sym::StructTableRow &row : strut->scope->table) {
    if (row.name == name) {
      log.error(symbol->loc) << "Redefinition of member \"" << name
        << "\" previously declared at " << row.val->loc << fatal;
    }
  }
  strut->scope->table.push_back({name, access, scope, std::move(symbol)});
}

sym::Func *StructInserter::insert(const ast::Func &func, const BuiltinTypes &bnt) {
  sym::FuncPtr funcSym = makeFunc(func.loc);
  funcSym->params = lookupParams(strut->scope, log, func.params);
  if (scope == sym::MemScope::instance) {
    funcSym->params.insert(funcSym->params.begin(), selfType(strut, mut));
  }
  funcSym->ret = inferRetType(strut->scope, log, func, bnt);
  return insertFunc(funcSym, sym::Name(func.name));
}

sym::Func *StructInserter::insert(const ast::Init &init) {
  sym::FuncPtr funcSym = makeFunc(init.loc);
  funcSym->params = lookupParams(strut->scope, log, init.params);
  funcSym->params.insert(funcSym->params.begin(), selfType(strut, ast::MemMut::mutating));
  funcSym->ret = {strut, sym::ValueMut::let, sym::ValueRef::val};
  return insertFunc(funcSym, "init");
}

void StructInserter::enterFuncScope(sym::Func *funcSym, const ast::Func &func) {
  const bool hasSelf = (scope == sym::MemScope::instance);
  if (hasSelf) {
    funcSym->scope->table.insert({sym::Name("self"), makeSelf(*funcSym, strut, mut)});
  }
  for (size_t i = 0; i != func.params.size(); ++i) {
    funcSym->scope->table.insert({
      sym::Name(func.params[i].name),
      makeParam(funcSym->params[i + hasSelf], func.params[i])
    });
  }
}

void StructInserter::enterFuncScope(sym::Func *funcSym, const ast::Init &init) {
  funcSym->scope->table.insert({sym::Name("self"), makeSelf(*funcSym, strut, ast::MemMut::mutating)});
  for (size_t i = 0; i != init.params.size(); ++i) {
    funcSym->scope->table.insert({
      sym::Name(init.params[i].name),
      makeParam(funcSym->params[i + 1], init.params[i])
    });
  }
}

void StructInserter::accessScope(const ast::Member &member) {
  access = memAccess(member, log);
  scope = memScope(member, log);
  mut = member.mut;
}

sym::Func *StructInserter::insertFunc(
  sym::FuncPtr &funcSym,
  const sym::Name &name
) {
  sym::StructTable &table = strut->scope->table;
  for (const sym::StructTableRow &row : table) {
    if (row.name != name) {
      continue;
    }
    sym::Func *const dupFunc = dynamic_cast<sym::Func *>(row.val.get());
    if (dupFunc) {
      if (row.scope != scope) {
        log.error(funcSym->loc) << "Cannot overload static and instance member functions \""
          << row.name << '"' << fatal;
      }
      if (sameParams(dupFunc->params, funcSym->params)) {
        log.error(funcSym->loc) << "Redefinition of member function \"" << name
          << "\" previously declared at " << row.val->loc << fatal;
      }
    } else {
      log.error(funcSym->loc) << "Redefinition of member function \"" << name
        << "\" previously declared (as a different kind of symbol) at "
        << row.val->loc << fatal;
    }
  }
  sym::Func *const ret = funcSym.get();
  table.push_back({name, access, scope, std::move(funcSym)});
  return ret;
}

EnumInserter::EnumInserter(sym::EnumType *enm, Log &log)
  : enm{enm}, log{log} {
  assert(enm);
}

void EnumInserter::insert(const sym::Name &name, sym::SymbolPtr symbol) {
  sym::EnumTable &table = enm->scope->table;
  for (auto r = table.begin(); r != table.end(); ++r) {
    if (r->key == name) {
      log.error(symbol->loc) << "Redefinition of enum case \"" << name
        << "\" previously declared at " << r->val->loc << fatal;
    }
  }
  table.push_back({name, std::move(symbol)});
}

/* LCOV_EXCL_START */
sym::Func *EnumInserter::insert(const ast::Func &, const BuiltinTypes &) {
  assert(false);
}
void EnumInserter::enterFuncScope(sym::Func *, const ast::Func &) {
  assert(false);
}
/* LCOV_EXCL_END */

void EnumInserter::insert(const ast::EnumCase &cs) {
  auto caseSym = std::make_unique<sym::Object>();
  caseSym->loc = cs.loc;
  caseSym->etype.type = enm;
  caseSym->etype.mut = sym::ValueMut::let;
  caseSym->etype.ref = sym::ValueRef::val;
  insert(sym::Name(cs.name), std::move(caseSym));
}

InserterManager::InserterManager(sym::NSScope *scope, Log &log, const BuiltinTypes &types)
  : bnt{types} {
  push<UnorderedInserter>(scope, log);
}

void InserterManager::pop() {
  assert(!stack.empty());
  stack.pop_back();
}

SymbolInserter *InserterManager::get() const {
  assert(!stack.empty());
  return stack.back().get();
}
