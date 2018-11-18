//
//  ast.cpp
//  STELA
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "ast.hpp"

#define ACCEPT(TYPE)                                                            \
  void stela::ast::TYPE::accept(Visitor &visitor) {                             \
    visitor.visit(*this);                                                       \
  }

/* LCOV_EXCL_START */

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
ACCEPT(Lambda)

/* LCOV_EXCL_END */

#undef ACCEPT
