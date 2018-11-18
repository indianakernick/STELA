//
//  parse litr.cpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "parse litr.hpp"

#include "parse expr.hpp"
#include "parse stat.hpp"
#include "parse func.hpp"

using namespace stela;

namespace {

template <typename Literal, bool Trim = false>
ast::LitrPtr parseLiteralToken(ParseTokens &tok, const Token::Type type) {
  if (tok.peekType(type)) {
    auto literal = make_retain<Literal>();
    literal->loc = tok.loc();
    literal->value = tok.expect(type);
    if constexpr (Trim) {
      literal->value.remove_prefix(1);
      literal->value.remove_suffix(1);
    }
    return literal;
  } else {
    return nullptr;
  }
}

ast::LitrPtr parseString(ParseTokens &tok) {
  return parseLiteralToken<ast::StringLiteral, true>(tok, Token::Type::string);
}

ast::LitrPtr parseChar(ParseTokens &tok) {
  return parseLiteralToken<ast::CharLiteral, true>(tok, Token::Type::character);
}

ast::LitrPtr parseNumber(ParseTokens &tok) {
  return parseLiteralToken<ast::NumberLiteral>(tok, Token::Type::number);
}

ast::LitrPtr parseBool(ParseTokens &tok) {
  bool value;
  if (tok.checkKeyword("true")) {
    value = true;
  } else if (tok.checkKeyword("false")) {
    value = false;
  } else {
    return nullptr;
  }
  auto literal = make_retain<ast::BoolLiteral>();
  literal->loc = tok.lastLoc();
  literal->value = value;
  return literal;
}

ast::LitrPtr parseArray(ParseTokens &tok) {
  if (!tok.checkOp("[")) {
    return nullptr;
  }
  Context ctx = tok.context("in array literal");
  auto array = make_retain<ast::ArrayLiteral>();
  array->loc = tok.lastLoc();
  array->exprs = parseExprList(tok, "]");
  return array;
}

ast::LitrPtr parseInitList(ParseTokens &tok) {
  if (!tok.checkOp("{")) {
    return nullptr;
  }
  Context ctx = tok.context("in initializer list");
  auto list = make_retain<ast::InitList>();
  list->loc = tok.lastLoc();
  list->exprs = parseExprList(tok, "}");
  return list;
}

ast::LitrPtr parseLambda(ParseTokens &tok) {
  if (!tok.checkKeyword("func")) {
    return nullptr;
  }
  Context ctx = tok.context("in lambda expression");
  auto lambda = make_retain<ast::Lambda>();
  lambda->loc = tok.lastLoc();
  lambda->params = parseFuncParams(tok);
  lambda->ret = parseFuncRet(tok);
  lambda->body = parseFuncBody(tok);
  return lambda;
}

}

ast::LitrPtr stela::parseLitr(ParseTokens &tok) {
  if (ast::LitrPtr node = parseString(tok)) return node;
  if (ast::LitrPtr node = parseChar(tok)) return node;
  if (ast::LitrPtr node = parseNumber(tok)) return node;
  if (ast::LitrPtr node = parseBool(tok)) return node;
  if (ast::LitrPtr node = parseArray(tok)) return node;
  if (ast::LitrPtr node = parseInitList(tok)) return node;
  if (ast::LitrPtr node = parseLambda(tok)) return node;
  return nullptr;
}
