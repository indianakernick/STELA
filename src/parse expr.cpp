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
    args.push_back(tok.expectNode(parseExpr, "expression"));
  } while (tok.expectEitherOp(",", ")") == ",");
  return args;
}

ast::ExprPtr parseIdent(ParseTokens &tok) {
  if (tok.peekIdentType()) {
    auto ident = std::make_unique<ast::Identifier>();
    ident->name = tok.expectID();
    return ident;
  } else {
    return nullptr;
  }
}

ast::ExprPtr parseTerm(ParseTokens &tok) {
  // 1
  if (tok.checkOp("(")) {
    ast::ExprPtr node = parseExpr(tok);
    tok.expectOp(")");
    return node;
  }
  if (ast::ExprPtr node = parseIdent(tok)) return node;
  if (ast::ExprPtr node = parseLitr(tok)) return node;
  return nullptr;
}

ast::ExprPtr makeUnary(const ast::UnOp oper, ast::ExprPtr expr) {
  auto unary = std::make_unique<ast::UnaryExpr>();
  unary->oper = oper;
  unary->expr = std::move(expr);
  return unary;
}

ast::ExprPtr parseMisc(ParseTokens &tok) {
  // 2
  ast::ExprPtr left;
  if (!(left = parseTerm(tok))) {
    return nullptr;
  }
  if (tok.checkOp("++")) {
    return makeUnary(ast::UnOp::post_incr, std::move(left));
  } else if (tok.checkOp("--")) {
    return makeUnary(ast::UnOp::pre_incr, std::move(left));
  } else if (tok.peekOp("(")) {
    auto call = std::make_unique<ast::FuncCall>();
    call->func = std::move(left);
    call->args = parseFuncArgs(tok);
    return call;
  } else if (tok.checkOp("[")) {
    auto sub = std::make_unique<ast::Subscript>();
    sub->object = std::move(left);
    sub->index = tok.expectNode(parseExpr, "expression");
    tok.expectOp("]");
    return sub;
  } else if (tok.checkOp(".")) {
    auto mem = std::make_unique<ast::MemberIdent>();
    mem->object = std::move(left);
    mem->member = tok.expectNode(parseIdent, "identifier");
    return mem;
  } else {
    return left;
  }
}

ast::ExprPtr parseUnary(ParseTokens &);

ast::ExprPtr makeUnary(const ast::UnOp oper, ParseTokens &tok) {
  return makeUnary(oper, tok.expectNode(parseUnary, "expression"));
}

ast::ExprPtr parseUnary(ParseTokens &tok) {
  // 3
  if (tok.checkOp("++")) {
    return makeUnary(ast::UnOp::pre_incr, tok);
  } else if (tok.checkOp("--")) {
    return makeUnary(ast::UnOp::pre_decr, tok);
  } else if (tok.checkOp("-")) {
    return makeUnary(ast::UnOp::neg, tok);
  } else if (tok.checkOp("!")) {
    return makeUnary(ast::UnOp::bool_not, tok);
  } else if (tok.checkOp("~")) {
    return makeUnary(ast::UnOp::bit_not, tok);
  } else if (tok.checkKeyword("make")) {
    Context ctx = tok.context("object initialization");
    auto init = std::make_unique<ast::InitCall>();
    init->type = parseType(tok);
    init->args = parseFuncArgs(tok);
    return init;
  } else {
    return parseMisc(tok);
  }
}

// https://en.cppreference.com/w/cpp/language/operator_precedence

ast::ExprPtr parsePow(ParseTokens &tok) {
  // 4
  ast::ExprPtr left;
  if (!(left = parseUnary(tok))) {
    return nullptr;
  }
  if (!tok.checkOp("**")) {
    return left;
  }
  ast::ExprPtr right = tok.expectNode(parsePow, "expression");
  auto expr = std::make_unique<ast::BinaryExpr>();
  expr->left = std::move(left);
  expr->oper = ast::BinOp::pow;
  expr->right = std::move(right);
  return expr;
}

ast::ExprPtr parseMul(ParseTokens &tok) {
  // 5
  ast::ExprPtr left;
  if (!(left = parsePow(tok))) {
    return nullptr;
  }
  while (true) {
    ast::BinOp op;
    if (tok.checkOp("*")) {
      op = ast::BinOp::mul;
    } else if (tok.checkOp("/")) {
      op = ast::BinOp::div;
    } else if (tok.checkOp("%")) {
      op = ast::BinOp::mod;
    } else {
      break;
    }
    auto expr = std::make_unique<ast::BinaryExpr>();
    expr->left = std::move(left);
    expr->oper = op;
    expr->right = tok.expectNode(parsePow, "expression");
    left = std::move(expr);
  }
  return left;
}

ast::ExprPtr parseAdd(ParseTokens &tok) {
  // 6
  ast::ExprPtr left;
  if (!(left = parseMul(tok))) {
    return nullptr;
  }
  while (true) {
    ast::BinOp op;
    if (tok.checkOp("+")) {
      op = ast::BinOp::add;
    } else if (tok.checkOp("-")) {
      op = ast::BinOp::sub;
    } else {
      break;
    }
    auto expr = std::make_unique<ast::BinaryExpr>();
    expr->left = std::move(left);
    expr->oper = op;
    expr->right = tok.expectNode(parseMul, "expression");
    left = std::move(expr);
  }
  return left;
}

}

ast::ExprPtr stela::parseExpr(ParseTokens &tok) {
  return parseAdd(tok);
}

