//
//  ast.cpp
//  STELA
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "ast.hpp"

#include <cassert>

/* LCOV_EXCL_START */

stela::ast::Node::~Node() = default;
stela::ast::Type::~Type() = default;
stela::ast::Expression::~Expression() = default;
stela::ast::Statement::~Statement() = default;
stela::ast::Declaration::~Declaration() = default;
stela::ast::Assignment::~Assignment() = default;
stela::ast::Literal::~Literal() = default;

stela::ast::Visitor::~Visitor() = default;

// visitor disabled for FuncParam
void stela::ast::FuncParam::accept(Visitor &) {
  assert(false);
}

#define ACCEPT(TYPE)                                                            \
  void stela::ast::TYPE::accept(Visitor &visitor) {                             \
    visitor.visit(*this);                                                       \
  }

ACCEPT(BtnType)
ACCEPT(ArrayType)
ACCEPT(FuncType)
ACCEPT(NamedType)
ACCEPT(StructType)

ACCEPT(BinaryExpr)
ACCEPT(UnaryExpr)
ACCEPT(FuncCall)
ACCEPT(MemberIdent)
ACCEPT(Subscript)
ACCEPT(Identifier)
ACCEPT(Ternary)
ACCEPT(Make)

ACCEPT(Block)
ACCEPT(If)
ACCEPT(Switch)
ACCEPT(Break)
ACCEPT(Continue)
ACCEPT(Return)
ACCEPT(While)
ACCEPT(For)

ACCEPT(Func)
ACCEPT(Var)
ACCEPT(Let)
ACCEPT(TypeAlias)

ACCEPT(CompAssign)
ACCEPT(IncrDecr)
ACCEPT(Assign)
ACCEPT(DeclAssign)
ACCEPT(CallAssign)

ACCEPT(StringLiteral)
ACCEPT(CharLiteral)
ACCEPT(NumberLiteral)
ACCEPT(BoolLiteral)
ACCEPT(ArrayLiteral)
ACCEPT(InitList)
ACCEPT(Lambda)

#undef ACCEPT

/* LCOV_EXCL_END */
