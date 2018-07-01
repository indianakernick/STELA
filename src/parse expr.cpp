//
//  parse expr.cpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "parse expr.hpp"

#include "parse litr.hpp"
#include "parse type.hpp"

using namespace stela;

namespace {

ast::FuncArgs parseFuncArgs(ParseTokens &tok) {
  Context ctx = tok.context("function argument list");
  tok.expectOp("(");
  if (tok.checkOp(")")) {
    return {};
  }
  ast::FuncArgs args;
  do {
    args.push_back(ast::FuncArg{tok.expectNode(parseExpr, "expression")});
  } while (tok.expectEitherOp(",", ")") == ",");
  return args;
}

ast::ExprPtr parseInitCall(ParseTokens &tok) {
  if (!tok.checkKeyword("make")) {
    return nullptr;
  }
  Context ctx = tok.context("object initialization");
  auto init = std::make_unique<ast::InitCall>();
  init->type = parseType(tok);
  init->args = parseFuncArgs(tok);
  return init;
}

ast::ExprPtr parseIdent(ParseTokens &tok) {
  if (tok.front().type == Token::Type::identifier) {
    auto ident = std::make_unique<ast::Identifier>();
    ident->name = tok.expectID();
    return ident;
  } else {
    return nullptr;
  }
}

}

ast::ExprPtr stela::parseExpr(ParseTokens &tok) {
  if (ast::ExprPtr node = parseInitCall(tok)) return node;
  if (ast::ExprPtr node = parseLitr(tok)) return node;
  if (ast::ExprPtr node = parseIdent(tok)) return node;
  return nullptr;
}
