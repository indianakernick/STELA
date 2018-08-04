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

using namespace stela;

namespace {

// an expression followed by a ; is a statement
ast::StatPtr parseExprStatement(ParseTokens &tok) {
  if (ast::StatPtr node = parseExpr(tok)) {
    Context ctx = tok.context("after expression");
    tok.expectOp(";");
    return node;
  } else {
    return nullptr;
  }
}

ast::StatPtr parseIf(ParseTokens &tok) {
  if (!tok.checkKeyword("if")) {
    return nullptr;
  }
  Context ctx = tok.context("in if statement");
  auto ifNode = std::make_unique<ast::If>();
  ifNode->loc = tok.lastLoc();
  tok.expectOp("(");
  ifNode->cond = tok.expectNode(parseExpr, "condition expression");
  tok.expectOp(")");
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
  auto switchNode = std::make_unique<ast::Switch>();
  switchNode->loc = tok.lastLoc();
  tok.expectOp("(");
  switchNode->expr = tok.expectNode(parseExpr, "expression");
  tok.expectOp(")");
  tok.expectOp("{");
  
  while (!tok.checkOp("}")) {
    if (ast::StatPtr stat = parseStat(tok)) {
      switchNode->body.nodes.emplace_back(std::move(stat));
    } else if (tok.checkKeyword("case")) {
      auto scase = std::make_unique<ast::SwitchCase>();
      scase->loc = tok.lastLoc();
      scase->expr = tok.expectNode(parseExpr, "expression");
      tok.expectOp(":");
      switchNode->body.nodes.emplace_back(std::move(scase));
    } else if (tok.checkKeyword("default")) {
      auto def = std::make_unique<ast::SwitchDefault>();
      def->loc = tok.lastLoc();
      tok.expectOp(":");
      switchNode->body.nodes.emplace_back(std::move(def));
    } else {
      tok.log().error(tok.loc()) << "Expected case label or statement but found "
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
    auto type = std::make_unique<Type>();
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

ast::StatPtr parseFallthrough(ParseTokens &tok) {
  return parseKeywordStatement<ast::Fallthrough>(tok, "fallthrough");
}

ast::StatPtr parseReturn(ParseTokens &tok) {
  if (tok.checkKeyword("return")) {
    Context ctx = tok.context("in return statement");
    auto ret = std::make_unique<ast::Return>();
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
  auto whileNode = std::make_unique<ast::While>();
  whileNode->loc = tok.lastLoc();
  tok.expectOp("(");
  whileNode->cond = tok.expectNode(parseExpr, "condition expression");
  tok.expectOp(")");
  whileNode->body = tok.expectNode(parseStat, "statement or block");
  return whileNode;
}

ast::StatPtr parseRepeatWhile(ParseTokens &tok) {
  if (!tok.checkKeyword("repeat")) {
    return nullptr;
  }
  Context ctx = tok.context("in repeat statement");
  auto repeat = std::make_unique<ast::RepeatWhile>();
  repeat->loc = tok.lastLoc();
  repeat->body = tok.expectNode(parseStat, "statement or block");
  tok.expectKeyword("while");
  tok.expectOp("(");
  repeat->cond = tok.expectNode(parseExpr, "condition expression");
  tok.expectOp(")");
  tok.expectOp(";");
  return repeat;
}

ast::StatPtr parseForInit(ParseTokens &tok) {
  if (ast::StatPtr node = parseVar(tok)) return node;
  if (ast::StatPtr node = parseLet(tok)) return node;
  if (ast::StatPtr node = parseExpr(tok)) {
    tok.expectOp(";");
    return node;
  }
  tok.expectOp(";");
  return nullptr;
}

ast::StatPtr parseFor(ParseTokens &tok) {
  if (!tok.checkKeyword("for")) {
    return nullptr;
  }
  Context ctx = tok.context("in for statement");
  auto forNode = std::make_unique<ast::For>();
  forNode->loc = tok.lastLoc();
  tok.expectOp("(");
  forNode->init = parseForInit(tok); // init statement is optional
  forNode->cond = tok.expectNode(parseExpr, "condition expression");
  tok.expectOp(";");
  forNode->incr = parseExpr(tok); // incr expression is optional
  tok.expectOp(")");
  forNode->body = tok.expectNode(parseStat, "statement or block");
  return forNode;
}

ast::StatPtr parseBlock(ParseTokens &tok) {
  if (tok.checkOp("{")) {
    Context ctx = tok.context("in block");
    auto block = std::make_unique<ast::Block>();
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
  if (ast::StatPtr node = parseFallthrough(tok)) return node;
  if (ast::StatPtr node = parseReturn(tok)) return node;
  if (ast::StatPtr node = parseWhile(tok)) return node;
  if (ast::StatPtr node = parseRepeatWhile(tok)) return node;
  if (ast::StatPtr node = parseFor(tok)) return node;
  if (ast::StatPtr node = parseBlock(tok)) return node;
  if (ast::StatPtr node = parseDecl(tok)) return node;
  if (ast::StatPtr node = parseExprStatement(tok)) return node;
  if (tok.checkOp(";")) {
    auto empty = std::make_unique<ast::EmptyStatement>();
    empty->loc = tok.lastLoc();
    return empty;
  }
  return nullptr;
}

