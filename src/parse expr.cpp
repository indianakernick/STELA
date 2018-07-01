//
//  parse expr.cpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "parse expr.hpp"

#include "parse litr.hpp"

stela::ast::ExprPtr stela::parseExpr(ParseTokens &tok) {
  if (tok.check(Token::Type::identifier, "expr")) {
    return std::make_unique<ast::BinaryExpr>();
  }
  if (ast::ExprPtr node = parseLitr(tok)) return node;
  return nullptr;
}
