//
//  semantic analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "semantic analysis.hpp"

#include "traverse.hpp"
#include "check scopes.hpp"
#include "scope manager.hpp"
#include "Log/log output.hpp"
#include "builtin symbols.hpp"
#include "syntax analysis.hpp"

using namespace stela;

Symbols stela::initModules(LogSink &) {
  Symbols syms;
  syms.scopes.push_back(makeBuiltinModule(syms.builtins));
  syms.global = syms.scopes.back().get();
  return syms;
}

namespace {

void compileModuleImpl(Symbols &syms, AST &ast, Log &log) {
  log.module({});
  log.status() << "Analysing module \"" << ast.name << "\"" << endlog;
  log.module(ast.name);
  ScopeMan man{syms.scopes, syms.global};
  man.enterScope(ast.name);
  syms.global = man.cur();
  traverse({syms.builtins, man, log}, ast.global);
  std::move(ast.global.begin(), ast.global.end(), std::back_inserter(syms.decls));
  ast.global.clear();
}

}

void stela::compileModule(Symbols &syms, AST &ast, LogSink &sink) {
  Log log{sink, LogCat::semantic};
  compileModuleImpl(syms, ast, log);
  checkScopes(log, syms);
}

void stela::compileModule(Symbols &syms, std::string_view source, LogSink &sink) {
  AST ast = createAST(source, sink);
  compileModule(syms, ast, sink);
}

void stela::compileModules(Symbols &syms, const ModuleOrder &order, ASTs &asts, LogSink &sink) {
  Log log{sink, LogCat::semantic};
  for (const size_t index : order) {
    compileModuleImpl(syms, asts[index], log);
  }
  checkScopes(log, syms);
}

void stela::compileModules(Symbols &syms, ASTs &asts, LogSink &sink) {
  compileModules(syms, findModuleOrder(asts, sink), asts, sink);
}
