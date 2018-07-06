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

struct BinOp {
  std::string_view view;
  int prec;
  Assoc assoc;
  ast::BinOp op;
};

struct AssignOp {
  std::string_view view;
  ast::AssignOp op;
};

struct UnOp {
  std::string_view view;
  int prec;
  Assoc assoc;
  ast::UnOp op;
};

struct PrecAssoc {
  int prec;
  Assoc assoc;
};

// https://en.cppreference.com/w/cpp/language/operator_precedence

constexpr AssignOp assign_ops[] = {
  {"=",   ast::AssignOp::assign},
  {"+=",  ast::AssignOp::add},
  {"-=",  ast::AssignOp::sub},
  {"*=",  ast::AssignOp::mul},
  {"/=",  ast::AssignOp::div},
  {"%=",  ast::AssignOp::mod},
  {"&=",  ast::AssignOp::bit_and},
  {"|=",  ast::AssignOp::bit_or},
  {"^=",  ast::AssignOp::bit_xor},
  {"<<=", ast::AssignOp::bit_shl},
  {">>=", ast::AssignOp::bit_shr}
};

constexpr PrecAssoc assign = {0, Assoc::RTL};

constexpr BinOp bin_ops[] = {
  {"||", 1,  Assoc::LTR, ast::BinOp::bool_or},
  {"&&", 2,  Assoc::LTR, ast::BinOp::bool_and},
  {"|",  3,  Assoc::LTR, ast::BinOp::bit_or},
  {"^",  4,  Assoc::LTR, ast::BinOp::bit_xor},
  {"&",  5,  Assoc::LTR, ast::BinOp::bit_and},
  {"==", 6,  Assoc::LTR, ast::BinOp::eq},
  {"!=", 6,  Assoc::LTR, ast::BinOp::ne},
  {"<",  7,  Assoc::LTR, ast::BinOp::lt},
  {"<=", 7,  Assoc::LTR, ast::BinOp::le},
  {">",  7,  Assoc::LTR, ast::BinOp::gt},
  {">=", 7,  Assoc::LTR, ast::BinOp::ge},
  {"<<", 8,  Assoc::LTR, ast::BinOp::bit_shl},
  {">>", 8,  Assoc::LTR, ast::BinOp::bit_shr},
  {"+",  9,  Assoc::LTR, ast::BinOp::add},
  {"-",  9,  Assoc::LTR, ast::BinOp::sub},
  {"*",  10, Assoc::LTR, ast::BinOp::mul},
  {"/",  10, Assoc::LTR, ast::BinOp::div},
  {"%",  10, Assoc::LTR, ast::BinOp::mod},
  {"**", 11, Assoc::RTL, ast::BinOp::pow}
};

constexpr UnOp un_ops[] = {
  {"!",  12, Assoc::RTL, ast::UnOp::bool_not},
  {"~",  12, Assoc::RTL, ast::UnOp::bit_not},
  {"-",  12, Assoc::RTL, ast::UnOp::neg},
  {"++", 12, Assoc::RTL, ast::UnOp::pre_incr},
  {"--", 12, Assoc::RTL, ast::UnOp::pre_decr},
  {"++", 13, Assoc::LTR, ast::UnOp::post_incr},
  {"--", 13, Assoc::LTR, ast::UnOp::post_decr}
};

constexpr PrecAssoc member = {13, Assoc::LTR};
constexpr PrecAssoc subscript = {13, Assoc::LTR};
constexpr PrecAssoc funCall = {13, Assoc::LTR};

constexpr size_t npos = size_t(-1);

/// Get the index of an operator in the table. Return npos if the operator was
/// not found
template <auto &Table>
size_t lookup(const std::string_view view) {
  for (size_t i = 0; i != sizeof(Table) / sizeof(Table[0]); ++i) {
    if (Table[i].view == view) {
      return i;
    }
  }
  return npos;
}

bool getBinOp(PrecAssoc &pa, const std::string_view view) {
  size_t index = lookup<bin_ops>(view);
  if (index != npos) {
    pa = {bin_ops[index].prec, bin_ops[index].assoc};
    return true;
  }
  
  index = lookup<assign_ops>(view);
  if (index != npos) {
    pa = assign;
    return true;
  }
  return false;
}

PrecAssoc getBinOp(const std::string_view view) {
  PrecAssoc pa;
  const bool got [[maybe_unused]] = getBinOp(pa, view);
  assert(got);
  return pa;
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
  const std::string_view oper = pop(opStack);
  
  if (const size_t index = lookup<bin_ops>(oper); index != npos) {
    auto binExpr = std::make_unique<ast::BinaryExpr>();
    binExpr->right = pop(exprs);
    binExpr->left = pop(exprs);
    binExpr->oper = bin_ops[index].op;
    exprs.push_back(std::move(binExpr));
  } else if (const size_t index = lookup<assign_ops>(oper); index != npos) {
    auto assign = std::make_unique<ast::Assignment>();
    assign->right = pop(exprs);
    assign->left = pop(exprs);
    assign->oper = assign_ops[index].op;
    exprs.push_back(std::move(assign));
  } else {
    assert(false);
  }
}

/// Determine whether the operator on the top of the stack should be popped by
/// compare precedence and associativity with the given operator
bool shouldPop(const PrecAssoc curr, const std::string_view topOper) {
  const PrecAssoc top = getBinOp(topOper);
  return curr.prec < top.prec || (
    curr.prec == top.prec &&
    curr.assoc == Assoc::LTR
  );
}

/// Return true if the top element is not equal to the given element
template <typename Stack, typename Element>
bool topNeq(const Stack &stack, const Element &elem) {
  return !stack.empty() && stack.back() != elem;
}

/// Make room on the operator stack for a given operator. Checks precedence and
/// associativity
void makeRoomForOp(ExprPtrs &exprs, OpStack &opStack, const PrecAssoc curr) {
  while (topNeq(opStack, "(") && shouldPop(curr, opStack.back())) {
    popOper(exprs, opStack);
  }
}

/// Push a binary operator. Return true if pushed
bool pushBinaryOp(ExprPtrs &exprs, OpStack &opStack, const std::string_view token) {
  PrecAssoc pa;
  if (getBinOp(pa, token)) {
    makeRoomForOp(exprs, opStack, pa);
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

}

ast::ExprPtr stela::parseExpr(ParseTokens &tok) {
  ExprPtrs exprs;
  OpStack opStack;
  
  const Token *const startPos = tok.pos();
  
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
  
  ast::ExprPtr expr = finalExpr(exprs, opStack);
  if (expr == nullptr) {
    tok.pos(startPos);
  }
  return expr;
}

