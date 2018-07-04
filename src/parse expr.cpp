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
  if (tok.peekIdentType()) {
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

/// Get the index of an operator on the operator table. Return null_op if the
/// operator was not found
size_t lookupOp(const std::string_view view) {
  for (size_t i = 0; i != sizeof(op_table) / sizeof(op_table[0]); ++i) {
    if (op_table[i].view == view) {
      return i;
    }
  }
  return null_op;
}

/// Get an operator from the operator table. Assert that the given token
/// is valid
const Operator &getOp(const std::string_view view) {
  const size_t index = lookupOp(view);
  assert(index != null_op);
  return op_table[index];
}

/// Pop the top element from the stack and return it
template <typename Element>
Element pop(std::vector<Element> &elems) {
  assert(!elems.empty());
  Element top = std::move(elems.back());
  elems.pop_back();
  return top;
}

using ExprPtrs = std::vector<ast::ExprPtr>;
using OpStack = std::vector<std::string_view>;

/// Pop an operator from the top of the stack to the expression stack. The top
/// two expressions become operands
void popOper(ExprPtrs &exprs, OpStack &opStack) {
  auto binExpr = std::make_unique<ast::BinaryExpr>();
  binExpr->right = pop(exprs);
  binExpr->left = pop(exprs);
  binExpr->oper = getOp(pop(opStack)).op;
  exprs.push_back(std::move(binExpr));
}

/// Determine whether the operator on the top of the stack should be popped by
/// compare precedence and associativity with the given operator
bool shouldPop(const Operator &curr, const std::string_view topOper) {
  const Operator &top = getOp(topOper);
  return curr.prec < top.prec || (
    curr.prec == top.prec &&
    curr.assoc == Assoc::LEFT_TO_RIGHT
  );
}

/// Return true if the top element is not equal to the given element
template <typename Stack, typename Element>
bool topNeq(const Stack &stack, const Element &elem) {
  return !stack.empty() && stack.back() != elem;
}

/// Make room on the operator stack for a given operator. Checks precedence and
/// associativity
void makeRoomForOp(ExprPtrs &exprs, OpStack &opStack, const Operator &curr) {
  while (topNeq(opStack, "(") && shouldPop(curr, opStack.back())) {
    popOper(exprs, opStack);
  }
}

/// Push a binary operator. Return true if pushed
bool pushBinaryOp(ExprPtrs &exprs, OpStack &opStack, const std::string_view token) {
  const size_t index = lookupOp(token);
  if (index != null_op) {
    makeRoomForOp(exprs, opStack, op_table[index]);
    opStack.push_back(token);
    return true;
  }
  return false;
}

/// Pop a left bracket from the top of the operator stack. Return true if popped
bool popLeftBracket(OpStack &opStack) {
  if (!opStack.empty() && opStack.back() == "(") {
    opStack.pop_back();
    return true;
  }
  return false;
}

/// Pop from the operator stack until a left bracket is reached or the stack
/// is emptied. Return true if a left bracket was reached
bool popUntilLeftBracket(ExprPtrs &exprs, OpStack &opStack) {
  while (topNeq(opStack, "(")) {
    popOper(exprs, opStack);
  }
  return popLeftBracket(opStack);
}

/// Clear all operators from the stack
void clearOps(ExprPtrs &exprs, OpStack &opStack) {
  while (!opStack.empty()) {
    popOper(exprs, opStack);
  }
}

/// Use the last of the operators to create the final expression node
ast::ExprPtr finalExpr(ExprPtrs &exprs, OpStack &opStack) {
  if (exprs.empty()) {
    return nullptr;
  }
  clearOps(exprs, opStack);
  if (exprs.size() == 1) {
    return std::move(exprs.back());
  } else {
    // um...
    assert(false);
  }
}

}

ast::ExprPtr stela::parseExpr(ParseTokens &tok) {
  ExprPtrs exprs;
  OpStack opStack;
  
  while (true) {
    if (ast::ExprPtr expr = parseUnaryExpr(tok)) {
      exprs.push_back(std::move(expr));
    } else if (tok.checkOp("(")) {
      opStack.push_back("(");
    } else if (tok.peekOpType()) {
      if (tok.front().view == ")" && popUntilLeftBracket(exprs, opStack)) {
        tok.expectOp();
      } else if (pushBinaryOp(exprs, opStack, tok.front().view)) {
        tok.expectOp();
      } else {
        // extraneous ) or
        // invalid binary operator
        break;
      }
    } else {
      // not a valid unary expression
      // not an operator
      break;
    }
  }
  
  return finalExpr(exprs, opStack);
}

