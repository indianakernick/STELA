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
  syms.modules.insert({"$builtin", makeBuiltinModule(syms.builtins)});
  return syms;
}

void stela::compileModule(Symbols &syms, AST &ast, LogBuf &buf) {
  Log log{buf, LogCat::semantic};
  sym::Module module;
  module.decls = std::move(ast.global);
  ScopeMan man{module.scopes, syms.builtins.scope};
  man.enterScope(sym::Scope::Type::ns);
  traverse(man, module, syms, log);
  syms.modules.emplace(ast.name, std::move(module));
}

void stela::compileModules(Symbols &syms, const ModuleOrder &order, ASTs &asts, LogBuf &buf) {
  syms.modules.reserve(1 + asts.size());
  for (const size_t index : order) {
    compileModule(syms, asts[index], buf);
  }
}
