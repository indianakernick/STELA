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

ast::FuncArgs parseFuncArgs(ParseTokens &tok) {
  Context ctx = tok.context("function argument list");
  tok.expectOp("(");
  if (tok.checkOp(")")) {
    return {};
  }
  ast::FuncArgs args;
  do {
    args.push_back(expectExpr(tok, parseExpr));
  } while (tok.expectEitherOp(",", ")") == ",");
  return args;
}

ast::ExprPtr parseIdent(ParseTokens &tok) {
  if (!tok.peekIdentType()) {
    return nullptr;
  }
  auto ident = std::make_unique<ast::Identifier>();
  ident->loc = tok.loc();
  const ast::Name name = tok.expectID();
  if (tok.checkOp("::")) {
    ident->module = name;
    Context ctx = tok.context("after scope operator");
    ident->name = tok.expectID();
  } else {
    ident->name = name;
  }
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
  ast::ExprPtr left = parseTerm(tok);
  if (!left) {
    return nullptr;
  }
  while (true) {
    if (tok.peekOp("(")) {
      auto call = std::make_unique<ast::FuncCall>();
      call->loc = tok.loc();
      call->func = std::move(left);
      call->args = parseFuncArgs(tok);
      left = std::move(call);
    } else if (tok.checkOp("[")) {
      auto sub = std::make_unique<ast::Subscript>();
      sub->loc = tok.lastLoc();
      sub->object = std::move(left);
      sub->index = expectExpr(tok, parseExpr);
      tok.expectOp("]");
      left = std::move(sub);
    } else if (tok.checkOp(".")) {
      auto mem = std::make_unique<ast::MemberIdent>();
      mem->loc = tok.lastLoc();
      mem->object = std::move(left);
      mem->member = tok.expectID();
      left = std::move(mem);
    } else {
      break;
    }
  }
  return left;
}

namespace {

ast::ExprPtr makeUnary(const ast::UnOp oper, ast::ExprPtr expr) {
  auto unary = std::make_unique<ast::UnaryExpr>();
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
  } else {
    return parseNested(tok);
  }
}

ast::ExprPtr makeBinary(const ast::BinOp oper, ast::ExprPtr left, ast::ExprPtr right) {
  auto bin = std::make_unique<ast::BinaryExpr>();
  bin->loc = left->loc;
  bin->left = std::move(left);
  bin->oper = oper;
  bin->right = std::move(right);
  return bin;
}

ast::ExprPtr parsePow(ParseTokens &tok) {
  // 4
  ast::ExprPtr left = parseUnary(tok);
  if (!left) {
    return nullptr;
  }
  if (!tok.checkOp("**")) {
    return left;
  }
  return makeBinary(
    ast::BinOp::pow, std::move(left), expectExpr(tok, parsePow)
  );
}

ast::ExprPtr parseMul(ParseTokens &tok) {
  // 5
  ast::ExprPtr left = parsePow(tok);
  if (!left) {
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
    left = makeBinary(op, std::move(left), expectExpr(tok, parsePow));
  }
  return left;
}

ast::ExprPtr parseAdd(ParseTokens &tok) {
  // 6
  ast::ExprPtr left = parseMul(tok);
  if (!left) {
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
    left = makeBinary(op, std::move(left), expectExpr(tok, parseMul));
  }
  return left;
}

ast::ExprPtr parseShift(ParseTokens &tok) {
  // 7
  ast::ExprPtr left = parseAdd(tok);
  if (!left) {
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
    left = makeBinary(op, std::move(left), expectExpr(tok, parseAdd));
  }
  return left;
}

ast::ExprPtr parseRel(ParseTokens &tok) {
  // 9
  ast::ExprPtr left = parseShift(tok);
  if (!left) {
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
    left = makeBinary(op, std::move(left), expectExpr(tok, parseShift));
  }
  return left;
}

ast::ExprPtr parseEq(ParseTokens &tok) {
  // 10
  ast::ExprPtr left = parseRel(tok);
  if (!left) {
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
    left = makeBinary(op, std::move(left), expectExpr(tok, parseRel));
  }
  return left;
}

/// Parse a left associative binary operator
ast::ExprPtr parseBin(
  ParseTokens &tok,
  const ast::BinOp op,
  const std::string_view token,
  ast::ExprPtr (*parseLower)(ParseTokens &)
) {
  ast::ExprPtr left = parseLower(tok);
  if (!left) {
    return nullptr;
  }
  while (tok.checkOp(token)) {
    left = makeBinary(op, std::move(left), expectExpr(tok, parseLower));
  }
  return left;
}

ast::ExprPtr parseBitAnd(ParseTokens &tok) {
  // 11
  return parseBin(tok, ast::BinOp::bit_and, "&", parseEq);
}

ast::ExprPtr parseBitXor(ParseTokens &tok) {
  // 12
  return parseBin(tok, ast::BinOp::bit_xor, "^", parseBitAnd);
}

ast::ExprPtr parseBitOr(ParseTokens &tok) {
  // 13
  return parseBin(tok, ast::BinOp::bit_or, "|", parseBitXor);
}

ast::ExprPtr parseBoolAnd(ParseTokens &tok) {
  // 14
  return parseBin(tok, ast::BinOp::bool_and, "&&", parseBitOr);
}

ast::ExprPtr parseBoolOr(ParseTokens &tok) {
  // 15
  return parseBin(tok, ast::BinOp::bool_or, "||", parseBoolAnd);
}

ast::ExprPtr parseTern(ParseTokens &tok) {
  // 16
  ast::ExprPtr left = parseBoolOr(tok);
  if (!left) {
    return nullptr;
  }
  if (tok.checkOp("?")) {
    auto tern = std::make_unique<ast::Ternary>();
    tern->loc = left->loc;
    tern->cond = std::move(left);
    tern->tru = expectExpr(tok, parseTern);
    tok.expectOp(":");
    tern->fals = expectExpr(tok, parseTern);
    return tern;
  }
  return left;
}

}

ast::ExprPtr stela::parseExpr(ParseTokens &tok) {
  return parseTern(tok);
}
