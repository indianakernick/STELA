//
//  parse decl.cpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "parse decl.hpp"

#include "parse func.hpp"
#include "parse expr.hpp"
#include "parse type.hpp"
#include "parse decl.hpp"

using namespace stela;

ast::DeclPtr stela::parseVar(ParseTokens &tok) {
  if (!tok.checkKeyword("var")) {
    return nullptr;
  }
  Context ctx = tok.context("in var declaration");
  auto var = std::make_unique<ast::Var>();
  var->loc = tok.lastLoc();
  var->name = tok.expectID();
  if (tok.checkOp(":")) {
    var->type = tok.expectNode(parseType, "type after :");
  }
  if (tok.checkOp("=")) {
    var->expr = tok.expectNode(parseExpr, "expression after =");
  }
  tok.expectOp(";");
  return var;
}

ast::DeclPtr stela::parseLet(ParseTokens &tok) {
  if (!tok.checkKeyword("let")) {
    return nullptr;
  }
  Context ctx = tok.context("in let declaration");
  auto let = std::make_unique<ast::Let>();
  let->loc = tok.lastLoc();
  let->name = tok.expectID();
  if (tok.checkOp(":")) {
    let->type = tok.expectNode(parseType, "type after :");
  }
  tok.expectOp("=");
  let->expr = tok.expectNode(parseExpr, "expression after =");
  tok.expectOp(";");
  return let;
}

namespace {

ast::DeclPtr parseTypealias(ParseTokens &tok) {
  if (!tok.checkKeyword("type")) {
    return nullptr;
  }
  Context ctx = tok.context("in type alias");
  auto aliasNode = std::make_unique<ast::TypeAlias>();
  aliasNode->loc = tok.lastLoc();
  aliasNode->name = tok.expectID();
  aliasNode->strong = !tok.checkOp("=");
  aliasNode->type = tok.expectNode(parseType, "type");
  tok.expectOp(";");
  return aliasNode;
}

}

ast::DeclPtr stela::parseDecl(ParseTokens &tok) {
  if (ast::DeclPtr node = parseFunc(tok)) return node;
  if (ast::DeclPtr node = parseVar(tok)) return node;
  if (ast::DeclPtr node = parseLet(tok)) return node;
  if (ast::DeclPtr node = parseTypealias(tok)) return node;
  return nullptr;
}
