//
//  categories.cpp
//  STELA
//
//  Created by Indi Kernick on 28/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "categories.hpp"

#include "ast.hpp"
#include "unreachable.hpp"
#include "generate type.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  void visit(ast::BtnType &) override {
    cat = TypeCat::trivially_copyable;
  }
  void visit(ast::ArrayType &) override {
    cat = TypeCat::trivially_relocatable;
  }
  void visit(ast::FuncType &) override {
    cat = TypeCat::trivially_relocatable;
  }
  void visit(ast::NamedType &name) override {
    name.definition->type->accept(*this);
  }
  void visit(ast::StructType &) override {
    cat = TypeCat::nontrivial;
  }
  // @TODO check for user types

  TypeCat cat;
};

}

TypeCat stela::classifyType(ast::Type *type) {
  assert(type);
  Visitor visitor;
  type->accept(visitor);
  return visitor.cat;
}

ArithCat stela::classifyArith(ast::Type *type) {
  ast::BtnType *btn = assertConcreteType<ast::BtnType>(type);
  switch (btn->value) {
    case ast::BtnTypeEnum::Char:
    case ast::BtnTypeEnum::Sint:
      return ArithCat::signed_int;
    case ast::BtnTypeEnum::Bool:
    case ast::BtnTypeEnum::Byte:
    case ast::BtnTypeEnum::Uint:
      return ArithCat::unsigned_int;
    case ast::BtnTypeEnum::Real:
      return ArithCat::floating_point;
    case ast::BtnTypeEnum::Void: ;
  }
  UNREACHABLE();
}

ArithCat stela::classifyArith(ast::Expression *expr) {
  return classifyArith(expr->exprType.get());
}
