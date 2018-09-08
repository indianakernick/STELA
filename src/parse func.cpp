//
//  parse func.cpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "parse func.hpp"

#include "parse type.hpp"
#include "parse stat.hpp"

using namespace stela;

namespace {

ast::FuncParam parseParam(ParseTokens &tok) {
  ast::FuncParam param;
  param.loc = tok.loc();
  param.name = tok.expectID();
  tok.expectOp(":");
  param.ref = parseRef(tok);
  param.type = tok.expectNode(parseType, "type");
  return param;
}

std::experimental::optional<ast::FuncParam> parseReceiver(ParseTokens &tok) {
  if (!tok.checkOp("(")) {
    return std::experimental::nullopt;
  }
  ast::FuncParam param = parseParam(tok);
  tok.expectOp(")");
  return param;
}

}

ast::FuncParams stela::parseFuncParams(ParseTokens &tok) {
  Context ctx = tok.context("in parameter list");
  tok.expectOp("(");
  if (tok.checkOp(")")) {
    return {};
  }
  ast::FuncParams params;
  do {
    params.push_back(parseParam(tok));
  } while (tok.expectEitherOp(")", ",") == ",");
  return params;
}

ast::TypePtr stela::parseFuncRet(ParseTokens &tok) {
  if (tok.checkOp("->")) {
    Context ctx = tok.context("after ->");
    return tok.expectNode(parseType, "type");
  } else {
    return nullptr;
  }
}

ast::Block stela::parseFuncBody(ParseTokens &tok) {
  ast::Block body;
  body.loc = tok.loc();
  Context ctx = tok.context("in body");
  tok.expectOp("{");
  while (ast::StatPtr node = parseStat(tok)) {
    body.nodes.emplace_back(std::move(node));
  }
  tok.expectOp("}");
  return body;
}

ast::DeclPtr stela::parseFunc(ParseTokens &tok) {
  if (!tok.checkKeyword("func")) {
    return nullptr;
  }
  
  Context ctx = tok.context("in function declaration");
  auto funcNode = std::make_unique<ast::Func>();
  funcNode->loc = tok.lastLoc();
  funcNode->receiver = parseReceiver(tok);
  funcNode->name = tok.expectID();
  ctx.ident(funcNode->name);
  funcNode->params = parseFuncParams(tok);
  funcNode->ret = parseFuncRet(tok);
  funcNode->body = parseFuncBody(tok);
  
  return funcNode;
}
