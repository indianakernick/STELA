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
  Visitor(gen::Ctx ctx, FuncBuilder &builder)
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
    llvm::Type *strut = generateType(ctx, &type);
    llvm::Value *addr = builder.alloc(strut);
    for (unsigned f = 0; f != type.fields.size(); ++f) {
      type.fields[f].type->accept(*this);
      llvm::Value *fieldAddr = builder.ir.CreateStructGEP(addr, f);
      builder.ir.CreateStore(value, fieldAddr);
    }
    value = builder.ir.CreateLoad(addr);
  }
  
  llvm::Value *value = nullptr;

private:
  gen::Ctx ctx;
  FuncBuilder &builder;
};

}

llvm::Value *stela::generateZeroExpr(gen::Ctx ctx, FuncBuilder &builder, ast::Type *type) {
  Visitor visitor{ctx, builder};
  type->accept(visitor);
  return visitor.value;
}
