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
    args.push_back(ast::FuncArg{tok.expectNode(parseExpr, "expression")});
  } while (tok.expectEitherOp(",", ")") == ",");
  return args;
}

ast::ExprPtr parseInitCall(ParseTokens &tok) {
  if (!tok.checkKeyword("make")) {
    return nullptr;
  }
  Context ctx = tok.context("object initialization");
  auto init = std::make_unique<ast::InitCall>();
  init->type = parseType(tok);
  init->args = parseFuncArgs(tok);
  return init;
}

ast::ExprPtr parseIdent(ParseTokens &tok) {
  if (tok.front().type == Token::Type::identifier) {
    auto ident = std::make_unique<ast::Identifier>();
    ident->name = tok.expectID();
    return ident;
  } else {
    return nullptr;
  }
}

ast::ExprPtr parseUnaryExpr(ParseTokens &tok) {
  if (ast::ExprPtr node = parseInitCall(tok)) return node;
  if (ast::ExprPtr node = parseIdent(tok)) return node;
  if (ast::ExprPtr node = parseLitr(tok)) return node;
  return nullptr;
}

enum class Assoc {
  LEFT_TO_RIGHT,
  RIGHT_TO_LEFT
};

struct Operator {
  std::string_view view;
  int prec;
  Assoc assoc;
  ast::BinOp op;
};

constexpr Operator op_table[] = {
  {"+", 1, Assoc::LEFT_TO_RIGHT, ast::BinOp::add},
  {"-", 1, Assoc::LEFT_TO_RIGHT, ast::BinOp::sub},
  {"*", 2, Assoc::LEFT_TO_RIGHT, ast::BinOp::mul},
  {"/", 2, Assoc::LEFT_TO_RIGHT, ast::BinOp::div},
  {"%", 2, Assoc::LEFT_TO_RIGHT, ast::BinOp::mod},
};

constexpr size_t null_op = std::numeric_limits<size_t>::max();

size_t lookupOp(const std::string_view view) {
  for (size_t i = 0; i != sizeof(op_table) / sizeof(op_table[0]); ++i) {
    if (op_table[i].view == view) {
      return i;
    }
  }
  return null_op;
}

void pushPop(std::vector<ast::ExprPtr> &exprs, std::vector<std::string_view> &opStack) {
  assert(!opStack.empty());
  assert(exprs.size() >= 2);
  auto binExpr = std::make_unique<ast::BinaryExpr>();
  binExpr->right = std::move(exprs.back());
  exprs.pop_back();
  binExpr->left = std::move(exprs.back());
  exprs.pop_back();
  const size_t index = lookupOp(opStack.back());
  opStack.pop_back();
  assert(index != null_op);
  binExpr->oper = op_table[index].op;
  exprs.push_back(std::move(binExpr));
}


}

ast::ExprPtr stela::parseExpr(ParseTokens &tok) {
  std::vector<ast::ExprPtr> exprs;
  std::vector<std::string_view> opStack;
  
  while (true) {
    if (ast::ExprPtr expr = parseUnaryExpr(tok)) {
      exprs.push_back(std::move(expr));
    } else if (tok.checkOp("(")) {
      opStack.push_back("(");
    } else if (tok.front().type == Token::Type::oper) {
      const size_t index = lookupOp(tok.front().view);
      if (index != null_op) {
        const Operator &currOp = op_table[index];
        while (!opStack.empty() && opStack.back() != "(") {
          const size_t topIndex = lookupOp(opStack.back());
          if (op_table[topIndex].prec >= currOp.prec) {
            pushPop(exprs, opStack);
          } else {
            break;
          }
        }
        opStack.push_back(tok.expectOp());
      } else if (tok.front().view == ")") {
        if (opStack.empty()) {
          break;
        }
        tok.expectOp();
        while (!opStack.empty() && opStack.back() != "(") {
          pushPop(exprs, opStack);
        }
        opStack.pop_back(); // pop (
      } else {
        break;
      }
    } else {
      break;
    }
  }
  if (exprs.empty()) {
    return nullptr;
  }
  while (!opStack.empty()) {
    pushPop(exprs, opStack);
  }
  assert(exprs.size() == 1);
  return std::move(exprs.back());
}
