//
//  categories.cpp
//  STELA
//
//  Created by Indi Kernick on 28/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "categories.hpp"

#include "ast.hpp"
#include "generate type.hpp"
#include "Utils/unreachable.hpp"

using namespace stela;

namespace {

class TypeVisitor final : public ast::Visitor {
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
    // @TODO some structs can be trivially copyable
    // if the struct can fit in a register and
    // all of its fields are trivially copyable then
    //   the struct is trivially copyable
    // otherwise
    //   the struct is nontrivial
    cat = TypeCat::nontrivial;
  }
  // @TODO check for user types

  TypeCat cat;
};

class ValueVisitor final : public ast::Visitor {
public:
  void visit(ast::BinaryExpr &) override {
    cat = ValueCat::prvalue;
  }
  void visit(ast::UnaryExpr &) override {
    cat = ValueCat::prvalue;
  }
  void visit(ast::FuncCall &) override {
    cat = ValueCat::prvalue;
  }
  void visit(ast::MemberIdent &mem) override {
    mem.object->accept(*this);
    if (cat == ValueCat::prvalue) {
      cat = ValueCat::xvalue;
    }
  }
  void visit(ast::Subscript &sub) override {
    sub.object->accept(*this);
    if (cat == ValueCat::prvalue) {
      cat = ValueCat::xvalue;
    }
  }
  void visit(ast::Identifier &ident) override {
    if (dynamic_cast<ast::Func *>(ident.definition)) {
      cat = ValueCat::prvalue;
    } else {
      cat = ValueCat::lvalue;
    }
  }
  void visit(ast::Ternary &tern) override {
    tern.troo->accept(*this);
    const ValueCat trooCat = cat;
    tern.fols->accept(*this);
    const ValueCat folsCat = cat;
    if (trooCat == ValueCat::lvalue && folsCat == ValueCat::lvalue) {
      cat = ValueCat::lvalue;
    } else {
      cat = ValueCat::prvalue;
    }
  }
  void visit(ast::Make &make) override {
    if (make.cast) {
      cat = ValueCat::prvalue;
    } else if (isBoolCast(make.type.get(), make.expr->exprType.get())) {
      cat = ValueCat::prvalue;
    } else {
      make.expr->accept(*this);
    }
  }
  
  void visit(ast::StringLiteral &) override {
    cat = ValueCat::prvalue;
  }
  void visit(ast::CharLiteral &) override {
    cat = ValueCat::prvalue;
  }
  void visit(ast::NumberLiteral &) override {
    cat = ValueCat::prvalue;
  }
  void visit(ast::BoolLiteral &) override {
    cat = ValueCat::prvalue;
  }
  void visit(ast::ArrayLiteral &) override {
    cat = ValueCat::prvalue;
  }
  void visit(ast::InitList &) override {
    cat = ValueCat::prvalue;
  }
  void visit(ast::Lambda &) override {
    cat = ValueCat::prvalue;
  }
  
  ValueCat cat;
};

class LValueVisitor final : public ast::Visitor {
public:
  void visit(ast::MemberIdent &mem) override {
    mem.object->accept(*this);
  }
  void visit(ast::Identifier &ident) override {
    if (dynamic_cast<ast::Func *>(ident.definition) == nullptr) {
      root = &ident;
    }
  }
  
  ast::Identifier *root = nullptr;
};

}

TypeCat stela::classifyType(ast::Type *type) {
  assert(type);
  TypeVisitor visitor;
  type->accept(visitor);
  return visitor.cat;
}

ValueCat stela::classifyValue(ast::Expression *expr) {
  assert(expr);
  ValueVisitor visitor;
  expr->accept(visitor);
  return visitor.cat;
}

ast::Identifier *stela::rootLvalue(ast::Expression *expr) {
  LValueVisitor visitor;
  expr->accept(visitor);
  return visitor.root;
}

ArithCat stela::classifyArith(ast::BtnType *type) {
  switch (type->value) {
    case ast::BtnTypeEnum::Char:
    case ast::BtnTypeEnum::Sint:
      return ArithCat::signed_int;
    case ast::BtnTypeEnum::Bool:
    case ast::BtnTypeEnum::Byte:
    case ast::BtnTypeEnum::Uint:
      return ArithCat::unsigned_int;
    case ast::BtnTypeEnum::Real:
      return ArithCat::floating_point;
    case ast::BtnTypeEnum::Void:
    case ast::BtnTypeEnum::Opaq: ;
  }
  UNREACHABLE();
}

ArithCat stela::classifyArith(ast::Type *type) {
  return classifyArith(assertConcreteType<ast::BtnType>(type));
}

ArithCat stela::classifyArith(ast::Expression *expr) {
  return classifyArith(expr->exprType.get());
}
