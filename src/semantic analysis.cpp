//
//  semantic analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "semantic analysis.hpp"

#include "log output.hpp"
#include "syntax analysis.hpp"

stela::Symbols stela::annotateAST(AST &, LogBuf &buf) {
  Log log{buf, LogCat::semantic};
  
  return {};
}

stela::SymbolsAST stela::annotateAST(const std::string_view source, LogBuf &buf) {
  SymbolsAST sa;
  sa.ast = createAST(source, buf);
  sa.sym = annotateAST(sa.ast, buf);
  return sa;
}
