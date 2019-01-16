//
//  parse stat.cpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "parse stat.hpp"

#include "parse expr.hpp"
#include "parse decl.hpp"
#include "parse asgn.hpp"

using namespace stela;

namespace {

ast::StatPtr parseIf(ParseTokens &tok) {
  if (!tok.checkKeyword("if")) {
    return nullptr;
  }
  Context ctx = tok.context("in if statement");
  auto ifNode = make_retain<ast::If>();
  ifNode->loc = tok.lastLoc();
  ifNode->cond = tok.expectNode(parseExpr, "condition expression");
  ifNode->body = tok.expectNode(parseStat, "statement or block");
  if (tok.checkKeyword("else")) {
    ifNode->elseBody = tok.expectNode(parseStat, "statement or block");
  }
  return ifNode;
}

ast::StatPtr parseSwitch(ParseTokens &tok) {
  if (!tok.checkKeyword("switch")) {
    return nullptr;
  }
  Context ctx = tok.context("in switch statement");
  auto switchNode = make_retain<ast::Switch>();
  switchNode->loc = tok.lastLoc();
  switchNode->expr = tok.expectNode(parseExpr, "expression");
  tok.expectOp("{");
  
  while (!tok.checkOp("}")) {
    if (tok.checkKeyword("case")) {
      ast::SwitchCase scase;
      scase.loc = tok.lastLoc();
      scase.expr = tok.expectNode(parseExpr, "expression");
      scase.body = tok.expectNode(parseStat, "statement or block");
      switchNode->cases.push_back(std::move(scase));
    } else if (tok.checkKeyword("default")) {
      ast::SwitchCase def;
      def.loc = tok.lastLoc();
      def.body = tok.expectNode(parseStat, "statement or block");
      def.expr = nullptr;
      switchNode->cases.push_back(std::move(def));
    } else {
      tok.log().error(tok.loc()) << "Expected case or default but found "
        << tok.front() << tok.contextStack() << fatal;
    }
  }
  
  return switchNode;
}

template <typename Type>
ast::StatPtr parseKeywordStatement(ParseTokens &tok, const std::string_view name) {
  if (tok.checkKeyword(name)) {
    Context ctx = tok.context("after keyword");
    ctx.ident(name);
    auto type = make_retain<Type>();
    type->loc = tok.lastLoc();
    tok.expectOp(";");
    return type;
  } else {
    return nullptr;
  }
}

ast::StatPtr parseBreak(ParseTokens &tok) {
  return parseKeywordStatement<ast::Break>(tok, "break");
}

ast::StatPtr parseContinue(ParseTokens &tok) {
  return parseKeywordStatement<ast::Continue>(tok, "continue");
}

ast::StatPtr parseReturn(ParseTokens &tok) {
  if (tok.checkKeyword("return")) {
    Context ctx = tok.context("in return statement");
    auto ret = make_retain<ast::Return>();
    ret->loc = tok.lastLoc();
    if (tok.checkOp(";")) {
      return ret;
    }
    ret->expr = tok.expectNode(parseExpr, "expression or ;");
    tok.expectOp(";");
    return ret;
  } else {
    return nullptr;
  }
}

ast::StatPtr parseWhile(ParseTokens &tok) {
  if (!tok.checkKeyword("while")) {
    return nullptr;
  }
  Context ctx = tok.context("in while statement");
  auto whileNode = make_retain<ast::While>();
  whileNode->loc = tok.lastLoc();
  whileNode->cond = tok.expectNode(parseExpr, "condition expression");
  whileNode->body = tok.expectNode(parseStat, "statement or block");
  return whileNode;
}

ast::AsgnPtr parseOptAsgnSemi(ParseTokens &tok) {
  ast::AsgnPtr node = parseAsgn(tok);
  tok.expectOp(";");
  return node;
}

ast::AsgnPtr parseAsgnSemi(ParseTokens &tok) {
  if (ast::AsgnPtr node = parseAsgn(tok)) {
    tok.expectOp(";");
    return node;
  }
  return nullptr;
}

ast::StatPtr parseFor(ParseTokens &tok) {
  if (!tok.checkKeyword("for")) {
    return nullptr;
  }
  Context ctx = tok.context("in for statement");
  auto forNode = make_retain<ast::For>();
  forNode->loc = tok.lastLoc();
  tok.expectOp("(");
  forNode->init = parseOptAsgnSemi(tok); // init assignment is optional
  forNode->cond = tok.expectNode(parseExpr, "condition expression");
  tok.expectOp(";");
  forNode->incr = parseAsgn(tok); // incr assignment is optional
  tok.expectOp(")");
  forNode->body = tok.expectNode(parseStat, "statement or block");
  return forNode;
}

ast::StatPtr parseBlock(ParseTokens &tok) {
  if (tok.checkOp("{")) {
    Context ctx = tok.context("in block");
    auto block = make_retain<ast::Block>();
    block->loc = tok.lastLoc();
    while (ast::StatPtr node = parseStat(tok)) {
      block->nodes.emplace_back(std::move(node));
    }
    tok.expectOp("}");
    return block;
  } else {
    return nullptr;
  }
}

}

ast::StatPtr stela::parseStat(ParseTokens &tok) {
  if (ast::StatPtr node = parseIf(tok)) return node;
  if (ast::StatPtr node = parseSwitch(tok)) return node;
  if (ast::StatPtr node = parseBreak(tok)) return node;
  if (ast::StatPtr node = parseContinue(tok)) return node;
  if (ast::StatPtr node = parseReturn(tok)) return node;
  if (ast::StatPtr node = parseWhile(tok)) return node;
  if (ast::StatPtr node = parseFor(tok)) return node;
  if (ast::StatPtr node = parseBlock(tok)) return node;
  if (ast::StatPtr node = parseDecl(tok)) return node;
  if (ast::StatPtr node = parseAsgnSemi(tok)) return node;
  return nullptr;
}

