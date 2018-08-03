//
//  scope insert.cpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "scope insert.hpp"

#include "scope find.hpp"
#include "scope lookup.hpp"
#include "member access scope.hpp"
#include "compare params args.hpp"

using namespace stela;

namespace {

std::unique_ptr<sym::Func> makeFunc(const ast::Func &func) {
  auto funcSym = std::make_unique<sym::Func>();
  funcSym->loc = func.loc;
  return funcSym;
}

sym::ExprType inferRetType(sym::Scope *scope, Log &log, const ast::Func &func) {
  if (func.ret) {
    return {type(scope, log, func.ret), sym::ValueCat::rvalue};
  } else {
    // @TODO infer return type
    log.warn(func.loc) << "Return type is Void by default" << endlog;
    return {lookup(scope, log, "Void", func.loc), sym::ValueCat::rvalue};
  }
}

sym::ExprType selfType(sym::StructType *const structType) {
  return {structType, sym::ValueCat::lvalue_var};
}

auto makeSelf(sym::Func &funcSym, sym::StructType *const structType) {
  auto self = std::make_unique<sym::Object>();
  self->loc = funcSym.loc;
  self->etype = selfType(structType);
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

sym::Func *UnorderedInserter::insert(const ast::Func &func) {
  std::unique_ptr<sym::Func> funcSym = makeFunc(func);
  funcSym->params = funcParams(scope, log, func.params);
  funcSym->ret = inferRetType(scope, log, func);
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

NSInserter::NSInserter(sym::NSScope *scope, Log &log)
  : UnorderedInserter{scope, log} {}

FuncInserter::FuncInserter(sym::FuncScope *scope, Log &log)
  : UnorderedInserter{scope, log} {}

BlockInserter::BlockInserter(sym::BlockScope *scope, Log &log)
  : UnorderedInserter{scope, log} {}

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

sym::Func *StructInserter::insert(const ast::Func &func) {
  std::unique_ptr<sym::Func> funcSym = makeFunc(func);
  funcSym->params = funcParams(strut->scope, log, func.params);
  funcSym->params.insert(funcSym->params.begin(), selfType(strut));
  funcSym->ret = inferRetType(strut->scope, log, func);
  sym::StructTable &table = strut->scope->table;
  for (const sym::StructTableRow &row : table) {
    if (row.name != func.name) {
      continue;
    }
    sym::Func *const dupFunc = dynamic_cast<sym::Func *>(row.val.get());
    if (dupFunc) {
      if (row.scope != scope) {
        log.error(func.loc) << "Cannot overload static and non-static member functions \""
          << row.name << '"' << fatal;
      }
      if (sameParams(dupFunc->params, funcSym->params)) {
        log.error(func.loc) << "Redefinition of function \"" << func.name
          << "\" previously declared at " << row.val->loc << fatal;
      }
    } else {
      log.error(func.loc) << "Redefinition of function \"" << func.name
        << "\" previously declared (as a different kind of symbol) at "
        << row.val->loc << fatal;
    }
  }
  sym::Func *const ret = funcSym.get();
  table.push_back({sym::Name(func.name), access, scope, std::move(funcSym)});
  return ret;
}

void StructInserter::enterFuncScope(sym::Func *funcSym, const ast::Func &func) {
  funcSym->scope->table.insert({sym::Name("self"), makeSelf(*funcSym, strut)});
  for (size_t i = 0; i != funcSym->params.size(); ++i) {
    funcSym->scope->table.insert({
      sym::Name(func.params[i].name),
      makeParam(funcSym->params[i], func.params[i])
    });
  }
}

void StructInserter::accessScope(const ast::Member &member) {
  access = memAccess(member, log);
  scope = memScope(member, log);
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

sym::Func *EnumInserter::insert(const ast::Func &) {
  assert(false);
}

void EnumInserter::enterFuncScope(sym::Func *, const ast::Func &) {
  assert(false);
}

void EnumInserter::insert(const ast::EnumCase &cs) {
  auto caseSym = std::make_unique<sym::Object>();
  caseSym->loc = cs.loc;
  caseSym->etype.type = enm;
  caseSym->etype.cat = sym::ValueCat::lvalue_let;
  insert(sym::Name(cs.name), std::move(caseSym));
}

InserterManager::InserterManager(sym::NSScope *scope, Log &log)
  : defIns{scope, log}, ins{&defIns} {}

SymbolInserter *InserterManager::set(SymbolInserter *const newIns) {
  assert(newIns);
  return std::exchange(ins, newIns);
}

SymbolInserter *InserterManager::setDef() {
  return set(&defIns);
}

void InserterManager::restore(SymbolInserter *const oldIns) {
  assert(oldIns);
  ins = oldIns;
}
