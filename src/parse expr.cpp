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
  /// Left to right
  LTR,
  /// Right to left
  RTL
};

struct PrecAssoc {
  int prec;
  Assoc assoc;
};

struct Operator {
  std::string_view view;
  PrecAssoc pa;
};

enum class OtherOp {
  beg_ = ast::next_op_enum_beg,

  member = beg_,
  subscript,
  fun_call,
  l_paren,
  
  end_
};
constexpr int operator+(const OtherOp op) {
  return static_cast<int>(op);
}

// https://en.cppreference.com/w/cpp/language/operator_precedence

constexpr Operator ops[] = {
  {"=",   {0, Assoc::RTL}},
  {"+=",  {0, Assoc::RTL}},
  {"-=",  {0, Assoc::RTL}},
  {"*=",  {0, Assoc::RTL}},
  {"/=",  {0, Assoc::RTL}},
  {"%=",  {0, Assoc::RTL}},
  {"**=", {0, Assoc::RTL}},
  {"|=",  {0, Assoc::RTL}},
  {"^=",  {0, Assoc::RTL}},
  {"&=",  {0, Assoc::RTL}},
  {"<<=", {0, Assoc::RTL}},
  {">>=", {0, Assoc::RTL}},

  {"||", {1,  Assoc::LTR}},
  {"&&", {2,  Assoc::LTR}},
  {"|",  {3,  Assoc::LTR}},
  {"^",  {4,  Assoc::LTR}},
  {"&",  {5,  Assoc::LTR}},
  {"==", {6,  Assoc::LTR}},
  {"!=", {6,  Assoc::LTR}},
  {"<",  {7,  Assoc::LTR}},
  {"<=", {7,  Assoc::LTR}},
  {">",  {7,  Assoc::LTR}},
  {">=", {7,  Assoc::LTR}},
  {"<<", {8,  Assoc::LTR}},
  {">>", {8,  Assoc::LTR}},
  {"+",  {9,  Assoc::LTR}},
  {"-",  {9,  Assoc::LTR}},
  {"*",  {10, Assoc::LTR}},
  {"/",  {10, Assoc::LTR}},
  {"%",  {10, Assoc::LTR}},
  {"**", {11, Assoc::RTL}},

  {"-",  {12, Assoc::RTL}}, // negate
  {"!",  {12, Assoc::RTL}},
  {"~",  {12, Assoc::RTL}},
  {"++", {12, Assoc::RTL}}, // pre increment
  {"--", {12, Assoc::RTL}}, // pre decrement
  {"++", {13, Assoc::LTR}}, // post increment
  {"--", {13, Assoc::LTR}}, // post decrement
  
  {".", {13, Assoc::LTR}}, // member
  {"[", {13, Assoc::LTR}}, // subscript
  {"(", {13, Assoc::LTR}}, // function call
  {"(", {13, Assoc::LTR}}  // left parentheses
};

constexpr int npos = -1;

/// Get the index of an operator in the table. Return npos if not found
int lookup(const int beg, const int end, const std::string_view view) {
  for (int o = beg; o != end; ++o) {
    if (ops[o].view == view) {
      return o;
    }
  }
  return npos;
}

template <typename Enum>
bool inRange(const int oper) {
  return +Enum::beg_ <= oper && oper < +Enum::end_;
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
using OpStack = std::vector<int>;

bool popBinOp(ExprPtrs &exprs, const int oper) {
  if (inRange<ast::BinOp>(oper)) {
    auto binExpr = std::make_unique<ast::BinaryExpr>();
    binExpr->right = pop(exprs);
    binExpr->left = pop(exprs);
    binExpr->oper = static_cast<ast::BinOp>(oper);
    exprs.push_back(std::move(binExpr));
    return true;
  }
  return false;
}

bool popAssignOp(ExprPtrs &exprs, const int oper) {
  if (inRange<ast::AssignOp>(oper)) {
    auto assign = std::make_unique<ast::Assignment>();
    assign->right = pop(exprs);
    assign->left = pop(exprs);
    assign->oper = static_cast<ast::AssignOp>(oper);
    exprs.push_back(std::move(assign));
    return true;
  }
  return false;
}

bool popUnaryOp(ExprPtrs &exprs, const int oper) {
  if (inRange<ast::UnOp>(oper)) {
    auto unary = std::make_unique<ast::UnaryExpr>();
    unary->expr = pop(exprs);
    unary->oper = static_cast<ast::UnOp>(oper);
    exprs.push_back(std::move(unary));
    return true;
  }
  return false;
}

bool popOtherOp(ExprPtrs &exprs, const int oper) {
  if (oper == +OtherOp::member) {
    auto member = std::make_unique<ast::MemberIdent>();
    member->member = pop(exprs);
    member->object = pop(exprs);
    exprs.push_back(std::move(member));
    return true;
  }
  return false;
}

/// Pop an operator from the top of the stack to the expression stack
void popOper(ExprPtrs &exprs, OpStack &opStack) {
  const int oper = pop(opStack);
  if (popBinOp(exprs, oper)) {}
  else if (popAssignOp(exprs, oper)) {}
  else if (popUnaryOp(exprs, oper)) {}
  else if (popOtherOp(exprs, oper)) {}
  else {
    assert(false);
  }
}

/// Determine whether the operator on the top of the stack should be popped by
/// compare precedence and associativity with the given operator
bool shouldPop(const PrecAssoc curr, const int topOper) {
  const PrecAssoc top = ops[topOper].pa;
  return curr.prec < top.prec || (
    curr.prec == top.prec &&
    curr.assoc == Assoc::LTR
  );
}

/// Return true if the top operator is not OtherOp::l_paren
bool topNotLParen(const OpStack &stack) {
  return !stack.empty() && stack.back() != +OtherOp::l_paren;
}

/// Make room on the operator stack for a given operator. Checks precedence and
/// associativity
void makeRoomForOp(ExprPtrs &exprs, OpStack &opStack, const PrecAssoc curr) {
  while (topNotLParen(opStack) && shouldPop(curr, opStack.back())) {
    popOper(exprs, opStack);
  }
}

/// Given the previous token, is the current operator a prefix unary operator?
bool isPreUnary(const Token *const prev) {
  return prev == nullptr || (
    prev->type == Token::Type::oper &&
    prev->view != ")"
  );
}

/// Given the previous token, is the current operator a postfix unary operator?
bool isPostUnary(const Token *const prev) {
  return prev != nullptr && (
    prev->type == Token::Type::identifier ||
    (prev->type == Token::Type::oper && prev->view == ")")
  );
}

/// Push an operator to the operator stack. Return true if pushed
bool pushOp(ExprPtrs &exprs, OpStack &opStack, const int oper) {
  if (oper != npos) {
    makeRoomForOp(exprs, opStack, ops[oper].pa);
    opStack.push_back(oper);
    return true;
  }
  return false;
}

/// Push a prefix unary operator. Return true if pushed
bool pushPreUnOp(ExprPtrs &exprs, OpStack &opStack, const std::string_view token) {
  return pushOp(exprs, opStack, lookup(
    +ast::UnOp::beg_, +ast::UnOp::post_incr, token
  ));
}

/// Push a prefix unary operator. Return true if pushed
bool pushPostUnOp(ExprPtrs &exprs, OpStack &opStack, const std::string_view token) {
  return pushOp(exprs, opStack, lookup(
    +ast::UnOp::post_incr, +ast::UnOp::end_, token
  ));
}

/// Push a binary operator. Return true if pushed
bool pushBinOp(ExprPtrs &exprs, OpStack &opStack, const std::string_view token) {
  return pushOp(exprs, opStack, lookup(
    +ast::AssignOp::beg_, +ast::BinOp::end_, token
  ));
}

/// Pop a left bracket from the top of the operator stack. Return true if popped
bool popLeftBracket(OpStack &opStack) {
  if (!opStack.empty() && opStack.back() == +OtherOp::l_paren) {
    opStack.pop_back();
    return true;
  }
  return false;
}

/// Pop from the operator stack until a left bracket is reached or the stack
/// is emptied. Return true if a left bracket was reached
bool popUntilLeftBracket(ExprPtrs &exprs, OpStack &opStack) {
  while (topNotLParen(opStack)) {
    popOper(exprs, opStack);
  }
  return popLeftBracket(opStack);
}

/// Clear all operators from the stack
void clearOps(ExprPtrs &exprs, OpStack &opStack) {
  while (!opStack.empty()) {
    // if top op is ( then there are mismatched brackets
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
    /*
    
    WRITE AN ERROR MESSAGE!!!
    
    */
    assert(false);
  }
}

bool consumeOper(ExprPtrs &exprs, OpStack &opStack, ParseTokens &tok) {
  const std::string_view token = tok.front().view;
  const Token *prevToken = tok.size() == 1 ? nullptr : &tok.front() - 1;
  if (token == ")" && popUntilLeftBracket(exprs, opStack)) {}
  else if (isPreUnary(prevToken) && pushPreUnOp(exprs, opStack, token)) {}
  else if (isPostUnary(prevToken) && pushPostUnOp(exprs, opStack, token)) {}
  else if (pushBinOp(exprs, opStack, token)) {}
  else return false;
  tok.consume();
  return true;
}

}

ast::ExprPtr stela::parseExpr(ParseTokens &tok) {
  ExprPtrs exprs;
  OpStack opStack;
  
  const Token *const startPos = tok.pos();
  
  while (true) {
    if (ast::ExprPtr expr = parseUnaryExpr(tok)) {
      exprs.push_back(std::move(expr));
    } else if (tok.checkOp("(")) {
      opStack.push_back(+OtherOp::l_paren);
    } else if (tok.peekOpType()) {
      if (!consumeOper(exprs, opStack, tok)) {
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
  
  ast::ExprPtr expr = finalExpr(exprs, opStack);
  if (expr == nullptr) {
    tok.pos(startPos);
  }
  return expr;
}

