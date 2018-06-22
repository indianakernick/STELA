//
//  syntax analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "syntax analysis.hpp"

using namespace stela::ast;

stela::SyntaxError::SyntaxError(const Loc loc, const char *msg)
  : Error{"Syntax Error", loc, msg} {}


stela::AST stela::createAST(const Tokens &) {
  
  return {};
}
