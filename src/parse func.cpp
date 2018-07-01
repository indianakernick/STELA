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

namespace stela {

ast::FuncParams parseFuncParams(ParseTokens &tok) {
  Context ctx = tok.context("in parameter list");
  tok.expectOp("(");
  if (tok.checkOp(")")) {
    return {};
  }
  ast::FuncParams params;
  do {
    ast::FuncParam &param = params.emplace_back();
    param.name = tok.expectID();
    tok.expectOp(":");
    param.ref = parseRef(tok);
    param.type = tok.expectNode(parseType, "type");
  } while (tok.expectEitherOp(")", ",") == ",");
  return params;
}

ast::TypePtr parseFuncRet(ParseTokens &tok) {
  if (tok.checkOp("->")) {
    tok.context("after ->");
    return tok.expectNode(parseType, "type");
  } else {
    return nullptr;
  }
}

ast::Block parseFuncBody(ParseTokens &tok) {
  ast::Block body;
  Context ctx = tok.context("in body");
  tok.expectOp("{");
  while (ast::StatPtr node = parseStat(tok)) {
    body.nodes.emplace_back(std::move(node));
  }
  tok.expectOp("}");
  return body;
}

ast::DeclPtr parseFunc(ParseTokens &tok) {
  if (!tok.checkKeyword("func")) {
    return nullptr;
  }
  
  Context ctx = tok.context("in function declaration");
  auto funcNode = std::make_unique<ast::Func>();
  funcNode->name = tok.expectID();
  ctx.ident(funcNode->name);
  funcNode->params = parseFuncParams(tok);
  funcNode->ret = parseFuncRet(tok);
  funcNode->body = parseFuncBody(tok);
  
  return funcNode;
}

}
