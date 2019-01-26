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

// https://en.cppreference.com/w/cpp/language/operator_precedence

template <typename ParseFunc>
ast::ExprPtr expectExpr(ParseTokens &tok, ParseFunc &&parse) {
  return tok.expectNode(parse, "expression");
}

ast::ExprPtr parseIdent(ParseTokens &tok) {
  if (!tok.peekIdentType()) {
    return nullptr;
  }
  auto ident = make_retain<ast::Identifier>();
  ident->loc = tok.loc();
  ident->name = tok.expectID();
  return ident;
}

ast::ExprPtr parseTerm(ParseTokens &tok) {
  // 1
  if (tok.checkOp("(")) {
    ast::ExprPtr node = tok.expectNode(parseExpr, "expression after (");
    tok.expectOp(")");
    return node;
  }
  if (ast::ExprPtr node = parseIdent(tok)) return node;
  if (ast::ExprPtr node = parseLitr(tok)) return node;
  return nullptr;
}

}

ast::ExprPtr stela::parseNested(ParseTokens &tok) {
  // 2
  ast::ExprPtr lhs = parseTerm(tok);
  if (!lhs) {
    return nullptr;
  }
  while (true) {
    if (tok.checkOp("(")) {
      auto call = make_retain<ast::FuncCall>();
      call->loc = tok.loc();
      call->func = std::move(lhs);
      Context ctx = tok.context("in function argument list");
      call->args = parseExprList(tok, ")");
      lhs = std::move(call);
    } else if (tok.checkOp("[")) {
      auto sub = make_retain<ast::Subscript>();
      sub->loc = tok.lastLoc();
      sub->object = std::move(lhs);
      sub->index = expectExpr(tok, parseExpr);
      tok.expectOp("]");
      lhs = std::move(sub);
    } else if (tok.checkOp(".")) {
      auto mem = make_retain<ast::MemberIdent>();
      mem->loc = tok.lastLoc();
      mem->object = std::move(lhs);
      mem->member = tok.expectID();
      lhs = std::move(mem);
    } else {
      break;
    }
  }
  return lhs;
}

namespace {

ast::ExprPtr makeUnary(const ast::UnOp oper, ast::ExprPtr expr) {
  auto unary = make_retain<ast::UnaryExpr>();
  unary->loc = expr->loc;
  unary->oper = oper;
  unary->expr = std::move(expr);
  return unary;
}

ast::ExprPtr parseUnary(ParseTokens &);

ast::ExprPtr makeUnary(const ast::UnOp oper, ParseTokens &tok) {
  return makeUnary(oper, expectExpr(tok, parseUnary));
}

ast::ExprPtr parseUnary(ParseTokens &tok) {
  // 3
  if (tok.checkOp("-")) {
    return makeUnary(ast::UnOp::neg, tok);
  } else if (tok.checkOp("!")) {
    return makeUnary(ast::UnOp::bool_not, tok);
  } else if (tok.checkOp("~")) {
    return makeUnary(ast::UnOp::bit_not, tok);
  } else if (tok.checkKeyword("make")) {
    auto make = make_retain<ast::Make>();
    make->loc = tok.lastLoc();
    Context ctx = tok.context("in make expression");
    make->type = tok.expectNode(parseType, "type");
    make->expr = expectExpr(tok, parseUnary);
    return make;
  } else {
    return parseNested(tok);
  }
}

ast::ExprPtr makeBinary(const ast::BinOp oper, ast::ExprPtr lhs, ast::ExprPtr rhs) {
  auto bin = make_retain<ast::BinaryExpr>();
  bin->loc = lhs->loc;
  bin->lhs = std::move(lhs);
  bin->oper = oper;
  bin->rhs = std::move(rhs);
  return bin;
}

ast::ExprPtr parseMul(ParseTokens &tok) {
  // 5
  ast::ExprPtr lhs = parseUnary(tok);
  if (!lhs) {
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
    lhs = makeBinary(op, std::move(lhs), expectExpr(tok, parseUnary));
  }
  return lhs;
}

ast::ExprPtr parseAdd(ParseTokens &tok) {
  // 6
  ast::ExprPtr lhs = parseMul(tok);
  if (!lhs) {
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
    lhs = makeBinary(op, std::move(lhs), expectExpr(tok, parseMul));
  }
  return lhs;
}

ast::ExprPtr parseShift(ParseTokens &tok) {
  // 7
  ast::ExprPtr lhs = parseAdd(tok);
  if (!lhs) {
    return nullptr;
  }
  while (true) {
    ast::BinOp op;
    if (tok.checkOp("<<")) {
      op = ast::BinOp::bit_shl;
    } else if (tok.checkOp(">>")) {
      op = ast::BinOp::bit_shr;
    } else {
      break;
    }
    lhs = makeBinary(op, std::move(lhs), expectExpr(tok, parseAdd));
  }
  return lhs;
}

/// Parse a left associative binary operator
ast::ExprPtr parseBin(
  ParseTokens &tok,
  const ast::BinOp op,
  const std::string_view token,
  ast::ExprPtr (*parseLower)(ParseTokens &)
) {
  ast::ExprPtr lhs = parseLower(tok);
  if (!lhs) {
    return nullptr;
  }
  while (tok.checkOp(token)) {
    lhs = makeBinary(op, std::move(lhs), expectExpr(tok, parseLower));
  }
  return lhs;
}

ast::ExprPtr parseBitAnd(ParseTokens &tok) {
  // 11
  return parseBin(tok, ast::BinOp::bit_and, "&", parseShift);
}

ast::ExprPtr parseBitXor(ParseTokens &tok) {
  // 12
  return parseBin(tok, ast::BinOp::bit_xor, "^", parseBitAnd);
}

ast::ExprPtr parseBitOr(ParseTokens &tok) {
  // 13
  return parseBin(tok, ast::BinOp::bit_or, "|", parseBitXor);
}

ast::ExprPtr parseRel(ParseTokens &tok) {
  // 9
  ast::ExprPtr lhs = parseBitOr(tok);
  if (!lhs) {
    return nullptr;
  }
  while (true) {
    ast::BinOp op;
    if (tok.checkOp("<")) {
      op = ast::BinOp::lt;
    } else if (tok.checkOp("<=")) {
      op = ast::BinOp::le;
    } else if (tok.checkOp(">")) {
      op = ast::BinOp::gt;
    } else if (tok.checkOp(">=")) {
      op = ast::BinOp::ge;
    } else {
      break;
    }
    lhs = makeBinary(op, std::move(lhs), expectExpr(tok, parseBitOr));
  }
  return lhs;
}

ast::ExprPtr parseEq(ParseTokens &tok) {
  // 10
  ast::ExprPtr lhs = parseRel(tok);
  if (!lhs) {
    return nullptr;
  }
  while (true) {
    ast::BinOp op;
    if (tok.checkOp("==")) {
      op = ast::BinOp::eq;
    } else if (tok.checkOp("!=")) {
      op = ast::BinOp::ne;
    } else {
      break;
    }
    lhs = makeBinary(op, std::move(lhs), expectExpr(tok, parseRel));
  }
  return lhs;
}

ast::ExprPtr parseBoolAnd(ParseTokens &tok) {
  // 14
  return parseBin(tok, ast::BinOp::bool_and, "&&", parseEq);
}

ast::ExprPtr parseBoolOr(ParseTokens &tok) {
  // 15
  return parseBin(tok, ast::BinOp::bool_or, "||", parseBoolAnd);
}

ast::ExprPtr parseTern(ParseTokens &tok) {
  // 16
  ast::ExprPtr lhs = parseBoolOr(tok);
  if (!lhs) {
    return nullptr;
  }
  if (tok.checkOp("?")) {
    auto tern = make_retain<ast::Ternary>();
    tern->loc = lhs->loc;
    tern->cond = std::move(lhs);
    tern->troo = expectExpr(tok, parseTern);
    tok.expectOp(":");
    tern->fols = expectExpr(tok, parseTern);
    return tern;
  }
  return lhs;
}

}

ast::ExprPtr stela::parseExpr(ParseTokens &tok) {
  return parseTern(tok);
}

std::vector<ast::ExprPtr> stela::parseExprList(ParseTokens &tok, const ast::Name end) {
  if (tok.checkOp(end)) {
    return {};
  }
  std::vector<ast::ExprPtr> exprs;
  do {
    exprs.push_back(expectExpr(tok, parseExpr));
  } while (tok.expectEitherOp(",", end) == ",");
  return exprs;
}
