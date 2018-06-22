//
//  syntax analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "syntax analysis.hpp"

using namespace stela::ast;

stela::SyntaxError::SyntaxError(
  const Line line, const Col col, const char *msg
) : Error{"Syntax Error", line, col, msg} {}


stela::AST stela::createAST(const Tokens &) {
  
  return {};
}
