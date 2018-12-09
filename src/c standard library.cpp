//
//  c standard library.cpp
//  STELA
//
//  Created by Indi Kernick on 4/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "c standard library.hpp"

#include "log output.hpp"
#include "scope manager.hpp"

using namespace stela;

namespace {

ScopeMan createModule(Symbols &syms, const ast::Name name) {
  ScopeMan man{syms.scopes, syms.global};
  man.enterScope(name);
  syms.global = man.cur();
  return man;
}

void insertUnary(const ast::TypePtr &type, sym::Table &table, const sym::Name &name) {
  auto symbol = std::make_unique<sym::Func>();
  symbol->params.push_back(sym::null_type);
  symbol->params.push_back(sym::makeLetVal(type));
  symbol->ret = sym::makeVarVal(type);
  table.insert({name, std::move(symbol)});
}

void insertBinary(const ast::TypePtr &type, sym::Table &table, const sym::Name &name) {
  auto symbol = std::make_unique<sym::Func>();
  symbol->params.push_back(sym::null_type);
  symbol->params.push_back(sym::makeLetVal(type));
  symbol->params.push_back(sym::makeLetVal(type));
  symbol->ret = sym::makeVarVal(type);
  table.insert({name, std::move(symbol)});
}

}

void stela::includeCmath(Symbols &syms, LogSink &sink) {
  Log log{sink, LogCat::semantic};
  log.status() << "Including cmath" << endlog;
  log.module("cmath");
  ScopeMan man = createModule(syms, "$cmath");
  sym::Table &table = man.cur()->table;
  
  insertUnary(syms.builtins.Real, table, "fabs");
  insertBinary(syms.builtins.Real, table, "fmod");
  
  insertUnary(syms.builtins.Real, table, "exp");
  insertUnary(syms.builtins.Real, table, "exp2");
  insertUnary(syms.builtins.Real, table, "expm1");
  insertUnary(syms.builtins.Real, table, "log");
  insertUnary(syms.builtins.Real, table, "log10");
  insertUnary(syms.builtins.Real, table, "log2");
  insertUnary(syms.builtins.Real, table, "log1p");
  
  insertBinary(syms.builtins.Real, table, "pow");
  insertUnary(syms.builtins.Real, table, "sqrt");
  insertUnary(syms.builtins.Real, table, "cbrt");
  insertBinary(syms.builtins.Real, table, "hypot");
  
  insertUnary(syms.builtins.Real, table, "sin");
  insertUnary(syms.builtins.Real, table, "cos");
  insertUnary(syms.builtins.Real, table, "tan");
  insertUnary(syms.builtins.Real, table, "asin");
  insertUnary(syms.builtins.Real, table, "acos");
  insertUnary(syms.builtins.Real, table, "atan");
  insertBinary(syms.builtins.Real, table, "atan2");
  
  insertUnary(syms.builtins.Real, table, "ceil");
  insertUnary(syms.builtins.Real, table, "floor");
  insertUnary(syms.builtins.Real, table, "trunc");
  insertUnary(syms.builtins.Real, table, "round");
}
