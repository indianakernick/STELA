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
    return makeUnary(ast::UnOp::post_decr, std::move(left));
  } else if (tok.peekOp("(")) {
    auto call = std::make_unique<ast::FuncCall>();
    call->func = std::move(left);
    call->args = parseFuncArgs(tok);
    return call;
  } else if (tok.checkOp("[")) {
    auto sub = std::make_unique<ast::Subscript>();
    sub->object = std::move(left);
    sub->index = expectExpr(tok, parseExpr);
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
  return makeUnary(oper, expectExpr(tok, parseUnary));
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

ast::ExprPtr makeBinary(const ast::BinOp oper, ast::ExprPtr left, ast::ExprPtr right) {
  auto bin = std::make_unique<ast::BinaryExpr>();
  bin->left = std::move(left);
  bin->oper = oper;
  bin->right = std::move(right);
  return bin;
}

ast::ExprPtr parsePow(ParseTokens &tok) {
  // 4
  ast::ExprPtr left;
  if (!(left = parseUnary(tok))) {
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
    left = makeBinary(op, std::move(left), expectExpr(tok, parsePow));
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
    left = makeBinary(op, std::move(left), expectExpr(tok, parseMul));
  }
  return left;
}

ast::ExprPtr parseShift(ParseTokens &tok) {
  // 7
  ast::ExprPtr left;
  if (!(left = parseAdd(tok))) {
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
  ast::ExprPtr left;
  if (!(left = parseShift(tok))) {
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
  ast::ExprPtr left;
  if (!(left = parseRel(tok))) {
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
  ast::ExprPtr left;
  if (!(left = parseLower(tok))) {
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

ast::ExprPtr makeAssign(const ast::AssignOp op, ast::ExprPtr left, ast::ExprPtr right) {
  auto assign = std::make_unique<ast::Assignment>();
  assign->left = std::move(left);
  assign->oper = op;
  assign->right = std::move(right);
  return assign;
}

ast::ExprPtr parseAssign(ParseTokens &);

ast::ExprPtr makeAssign(const ast::AssignOp op, ast::ExprPtr left, ParseTokens &tok) {
  return makeAssign(op, std::move(left), expectExpr(tok, parseAssign));
}

ast::ExprPtr parseAssign(ParseTokens &tok) {
  // 16
  ast::ExprPtr left;
  if (!(left = parseBoolOr(tok))) {
    return nullptr;
  }
  if (tok.checkOp("?")) {
    auto tern = std::make_unique<ast::Ternary>();
    tern->cond = std::move(left);
    tern->tru = expectExpr(tok, parseAssign);
    tok.expectOp(":");
    tern->fals = expectExpr(tok, parseAssign);
    return tern;
  }
  
  #define CHECK(TOKEN, ENUM)                                                    \
    if (tok.checkOp(#TOKEN)) {                                                  \
      return makeAssign(ast::AssignOp::ENUM, std::move(left), tok);             \
    }
  
  CHECK(=, assign)
  CHECK(+=, add)
  CHECK(-=, sub)
  CHECK(*=, mul)
  CHECK(/=, div)
  CHECK(%=, mod)
  CHECK(**=, pow)
  CHECK(<<=, bit_shl)
  CHECK(>>=, bit_shr)
  CHECK(&=, bit_and)
  CHECK(^=, bit_xor)
  CHECK(|=, bit_or)
  
  #undef CHECK
  
  return left;
}

}

ast::ExprPtr stela::parseExpr(ParseTokens &tok) {
  return parseAssign(tok);
}

