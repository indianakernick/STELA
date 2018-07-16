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
#include "builtin symbols.hpp"
#include "syntax analysis.hpp"

stela::Symbols stela::createSym(const AST &ast, LogBuf &buf) {
  Log log{buf, LogCat::semantic};
  Symbols syms;
  syms.scopes.push_back(createBuiltinScope());
  log.verbose() << "Initialized " << syms.scopes[0]->table.size() << " builtins" << endlog;
  auto global = std::make_unique<sym::Scope>();
  global->parent = syms.scopes.back().get();
  syms.scopes.push_back(std::move(global));
  traverse(syms.scopes, ast, log);
  log.verbose() << "Created symbol table with " << syms.scopes[1]->table.size() << " global symbols" << endlog;
  return syms;
}

stela::SymbolsAST stela::createSym(const std::string_view source, LogBuf &buf) {
  SymbolsAST sa;
  sa.ast = createAST(source, buf);
  sa.sym = createSym(sa.ast, buf);
  return sa;
}
