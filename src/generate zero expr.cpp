//
//  generate zero expr.cpp
//  STELA
//
//  Created by Indi Kernick on 1/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate zero expr.hpp"

#include "unreachable.hpp"
#include "generate type.hpp"
#include "generate func.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, llvm::IRBuilder<> &builder)
    : ctx{ctx}, builder{builder} {}

  void visit(ast::BtnType &type) override {
    llvm::Type *llvmType = generateType(ctx, &type);
    switch (type.value) {
      case ast::BtnTypeEnum::Bool:
      case ast::BtnTypeEnum::Byte:
      case ast::BtnTypeEnum::Uint:
        value = llvm::ConstantInt::get(llvmType, 0, false); return;
      case ast::BtnTypeEnum::Char:
      case ast::BtnTypeEnum::Sint:
        value = llvm::ConstantInt::get(llvmType, 0, true); return;
      case ast::BtnTypeEnum::Real:
        value = llvm::ConstantFP::get(llvmType, 0.0); return;
      case ast::BtnTypeEnum::Void:
      default:
        UNREACHABLE();
    }
  }
  void visit(ast::ArrayType &type) override {
    //str = "make_null_array<";
    //str += generateType(ctx, type.elem.get());
    //str += ">()";
  }
  void visit(ast::FuncType &type) override {
    //str += generateMakeFunc(ctx, type);
    //str += "(&";
    //str += generateNullFunc(ctx, type);
    //str += ")";
  }
  void visit(ast::NamedType &type) override {
    type.definition->type->accept(*this);
  }
  void visit(ast::StructType &type) override {
    /*gen::String strut = generateType(ctx, &type);
    strut += "{";
    if (type.fields.empty()) {
      strut += "}";
      return;
    }
    type.fields[0].type->accept(*this);
    strut += str;
    for (auto f = type.fields.cbegin() + 1; f != type.fields.cend(); ++f) {
      strut += ", ";
      f->type->accept(*this);
      strut += str;
    }
    strut += "}";
    str = std::move(strut);*/
  }
  
  llvm::Value *value;

private:
  gen::Ctx ctx;
  llvm::IRBuilder<> &builder;
};

}

llvm::Value *generateZeroExpr(gen::Ctx ctx, llvm::IRBuilder<> &builder, ast::Type *type) {
  Visitor visitor{ctx, builder};
  type->accept(visitor);
  return visitor.value;
}
