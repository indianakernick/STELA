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

namespace {

ast::DeclPtr parseVar(ParseTokens &tok) {
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

ast::DeclPtr parseLet(ParseTokens &tok) {
  if (!tok.checkKeyword("let")) {
    return nullptr;
  }
  Context ctx = tok.context("in var declaration");
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

ast::DeclPtr parseTypealias(ParseTokens &tok) {
  if (!tok.checkKeyword("typealias")) {
    return nullptr;
  }
  Context ctx = tok.context("in type alias");
  auto aliasNode = std::make_unique<ast::TypeAlias>();
  aliasNode->loc = tok.lastLoc();
  aliasNode->name = tok.expectID();
  tok.expectOp("=");
  aliasNode->type = tok.expectNode(parseType, "type");
  tok.expectOp(";");
  return aliasNode;
}

ast::DeclPtr parseInit(ParseTokens &tok) {
  if (!tok.checkKeyword("init")) {
    return nullptr;
  }
  Context ctx = tok.context("in init function");
  auto init = std::make_unique<ast::Init>();
  init->loc = tok.lastLoc();
  init->params = parseFuncParams(tok);
  init->body = parseFuncBody(tok);
  return init;
}

ast::MemAccess parseMemAccess(ParseTokens &tok) {
  if (tok.checkKeyword("private")) {
    return ast::MemAccess::private_;
  } else {
    tok.checkKeyword("public");
    return ast::MemAccess::public_;
  }
}

ast::MemScope parseMemScope(ParseTokens &tok) {
  if (tok.checkKeyword("static")) {
    return ast::MemScope::static_;
  } else {
    return ast::MemScope::member;
  }
}

ast::Member parseStructMember(ParseTokens &tok) {
  ast::Member member;
  member.access = parseMemAccess(tok);
  member.scope = parseMemScope(tok);
  member.node = parseDecl(tok);
  return member;
}

ast::DeclPtr parseStruct(ParseTokens &tok) {
  if (!tok.checkKeyword("struct")) {
    return nullptr;
  }
  Context ctx = tok.context("in struct");
  auto structNode = std::make_unique<ast::Struct>();
  structNode->loc = tok.lastLoc();
  structNode->name = tok.expectID();
  ctx.desc(structNode->name);
  tok.expectOp("{");
  while(!tok.checkOp("}")) {
    structNode->body.push_back(parseStructMember(tok));
  }
  return structNode;
}

ast::EnumCase parseEnumCase(ParseTokens &tok) {
  ast::EnumCase ecase;
  ecase.name = tok.expectID();
  if (tok.checkOp("=")) {
    ecase.value = tok.expectNode(parseExpr, "expression or ,");
  }
  return ecase;
}

ast::DeclPtr parseEnum(ParseTokens &tok) {
  if (!tok.checkKeyword("enum")) {
    return nullptr;
  }
  Context ctx = tok.context("in enum");
  auto enumNode = std::make_unique<ast::Enum>();
  enumNode->loc = tok.lastLoc();
  enumNode->name = tok.expectID();
  ctx.ident(enumNode->name);
  tok.expectOp("{");
  
  if (!tok.checkOp("}")) {
    do {
      enumNode->cases.push_back(parseEnumCase(tok));
    } while (tok.expectEitherOp("}", ",") == ",");
  }
  
  return enumNode;
}

}

ast::DeclPtr stela::parseDecl(ParseTokens &tok) {
  if (ast::DeclPtr node = parseFunc(tok)) return node;
  if (ast::DeclPtr node = parseVar(tok)) return node;
  if (ast::DeclPtr node = parseLet(tok)) return node;
  if (ast::DeclPtr node = parseInit(tok)) return node;
  if (ast::DeclPtr node = parseTypealias(tok)) return node;
  if (ast::DeclPtr node = parseStruct(tok)) return node;
  if (ast::DeclPtr node = parseEnum(tok)) return node;
  return nullptr;
}
