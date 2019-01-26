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

ast::DeclPtr stela::parseVar(ParseTokens &tok, const bool external) {
  if (!tok.checkKeyword("var")) {
    return nullptr;
  }
  Context ctx = tok.context("in var declaration");
  auto var = make_retain<ast::Var>();
  var->external = external;
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

ast::DeclPtr stela::parseLet(ParseTokens &tok, const bool external) {
  if (!tok.checkKeyword("let")) {
    return nullptr;
  }
  Context ctx = tok.context("in let declaration");
  auto let = make_retain<ast::Let>();
  let->external = external;
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

ast::DeclPtr parseTypealias(ParseTokens &tok, const bool external) {
  if (!tok.checkKeyword("type")) {
    return nullptr;
  }
  if (external) {
    tok.log().error(tok.lastLoc()) << "extern cannot be applied to type alias" << fatal;
  }
  Context ctx = tok.context("in type alias");
  auto aliasNode = make_retain<ast::TypeAlias>();
  aliasNode->loc = tok.lastLoc();
  aliasNode->name = tok.expectID();
  aliasNode->strong = !tok.checkOp("=");
  aliasNode->type = tok.expectNode(parseType, "type");
  tok.expectOp(";");
  return aliasNode;
}

}

ast::DeclPtr stela::parseDecl(ParseTokens &tok, const bool external) {
  if (ast::DeclPtr node = parseFunc(tok, external)) return node;
  if (ast::DeclPtr node = parseVar(tok, external)) return node;
  if (ast::DeclPtr node = parseLet(tok, external)) return node;
  if (ast::DeclPtr node = parseTypealias(tok, external)) return node;
  return nullptr;
}
