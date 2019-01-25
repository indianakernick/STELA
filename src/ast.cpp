//
//  ast.cpp
//  STELA
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "ast.hpp"

#include "unreachable.hpp"

using namespace stela;

ast::Node::~Node() = default;
ast::Type::~Type() = default;
ast::Expression::~Expression() = default;
ast::Statement::~Statement() = default;
ast::Declaration::~Declaration() = default;
ast::Assignment::~Assignment() = default;
ast::Literal::~Literal() = default;

ast::Visitor::~Visitor() = default;

/* LCOV_EXCL_START */

void ast::FuncParam::accept(Visitor &) {
  UNREACHABLE();
}
void ast::SwitchCase::accept(Visitor &) {
  UNREACHABLE();
}

#define ACCEPT(TYPE)                                                            \
  void ast::TYPE::accept(Visitor &visitor) {                                    \
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
ACCEPT(Terminate)
ACCEPT(Break)
ACCEPT(Continue)
ACCEPT(Return)
ACCEPT(While)
ACCEPT(For)

ACCEPT(Func)
ACCEPT(ExtFunc)
ACCEPT(BtnFunc)
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

/* LCOV_EXCL_END */

#undef ACCEPT
