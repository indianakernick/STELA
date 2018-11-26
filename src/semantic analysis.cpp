//
//  semantic analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "semantic analysis.hpp"

#include "traverse.hpp"
#include "log output.hpp"
#include "scope manager.hpp"
#include "builtin symbols.hpp"
#include "syntax analysis.hpp"

using namespace stela;

Symbols stela::initModules(LogBuf &) {
  Symbols syms;
  syms.scopes.push_back(makeBuiltinModule(syms.builtins));
  syms.global = syms.scopes.back().get();
  return syms;
}

void stela::compileModule(Symbols &syms, AST &ast, LogBuf &buf) {
  Log log{buf, LogCat::semantic};
  log.status() << "Analysing module \"" << ast.name << "\"" << endlog;
  log.module(ast.name);
  ScopeMan man{syms.scopes, syms.global};
  man.enterScope(ast.name);
  syms.global = man.cur();
  traverse({syms.builtins, man, log}, ast.global);
  std::move(ast.global.begin(), ast.global.end(), std::back_inserter(syms.decls));
  ast.global.clear();
}

void stela::compileModules(Symbols &syms, const ModuleOrder &order, ASTs &asts, LogBuf &buf) {
  for (const size_t index : order) {
    compileModule(syms, asts[index], buf);
  }
}
