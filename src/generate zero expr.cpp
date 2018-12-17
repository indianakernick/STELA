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
    llvm::Type *elem = generateType(ctx, type.elem.get());
    llvm::StructType *arrayType = getArrayOf(ctx, elem);
    value = builder.ir.Insert(llvm::CallInst::CreateMalloc(
      builder.ir.GetInsertBlock(),
      llvm::IntegerType::getInt64Ty(ctx.llvm),
      arrayType,
      llvm::ConstantExpr::getSizeOf(arrayType),
      nullptr, nullptr
    ));
    llvm::Type *i64 = llvm::IntegerType::getInt64Ty(ctx.llvm);
    llvm::Type *i32 = llvm::IntegerType::getInt32Ty(ctx.llvm);
    llvm::Value *ref = builder.ir.CreateStructGEP(value, 0);
    builder.ir.CreateStore(llvm::ConstantInt::get(i64, 1), ref);
    llvm::Value *cap = builder.ir.CreateStructGEP(value, 1);
    builder.ir.CreateStore(llvm::ConstantInt::get(i32, 0), cap);
    llvm::Value *len = builder.ir.CreateStructGEP(value, 2);
    builder.ir.CreateStore(llvm::ConstantInt::get(i32, 0), len);
    llvm::Value *dat = builder.ir.CreateStructGEP(value, 3);
    builder.ir.CreateStore(llvm::ConstantPointerNull::get(elem->getPointerTo()), dat);
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
