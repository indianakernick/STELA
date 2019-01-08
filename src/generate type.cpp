//
//  generate type.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate type.hpp"

#include "llvm.hpp"
#include "symbols.hpp"
#include "categories.hpp"
#include <llvm/IR/Type.h>
#include "unreachable.hpp"
#include "type builder.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  explicit Visitor(llvm::LLVMContext &ctx)
    : ctx{ctx}, builder{ctx} {}
  
  void visit(ast::BtnType &type) override {
    switch (type.value) {
      case ast::BtnTypeEnum::Void:
        llvmType = llvm::Type::getVoidTy(ctx); return;
      case ast::BtnTypeEnum::Bool:
        llvmType = llvm::Type::getInt1Ty(ctx); return;
      case ast::BtnTypeEnum::Byte:
      case ast::BtnTypeEnum::Char:
        llvmType = llvm::Type::getInt8Ty(ctx); return;
      case ast::BtnTypeEnum::Real:
        llvmType = llvm::Type::getFloatTy(ctx); return;
      case ast::BtnTypeEnum::Sint:
      case ast::BtnTypeEnum::Uint:
        llvmType = llvm::Type::getInt32Ty(ctx); return;
    }
    UNREACHABLE();
  }
  void visit(ast::ArrayType &type) override {
    llvm::Type *elem = generateType(ctx, type.elem.get());
    llvmType = builder.ptrToArrayOf(elem);
  }
  void visit(ast::FuncType &type) override {
    llvmType = llvm::StructType::get(ctx, {
      generateLambSig(ctx, type),
      builder.cloData()->getPointerTo()
    });
  }
  void visit(ast::NamedType &type) override {
    type.definition->type->accept(*this);
  }
  void visit(ast::StructType &type) override {
    std::vector<llvm::Type *> elems;
    elems.reserve(type.fields.size());
    for (const ast::Field &field : type.fields) {
      field.type->accept(*this);
      elems.push_back(llvmType);
    }
    llvmType = llvm::StructType::get(ctx, elems);
  }
  
  llvm::Type *llvmType = nullptr;
  
private:
  llvm::LLVMContext &ctx;
  TypeBuilder builder;
};

}

llvm::Type *stela::generateType(llvm::LLVMContext &ctx, ast::Type *type) {
  assert(type);
  if (!type->llvm) {
    Visitor visitor{ctx};
    type->accept(visitor);
    type->llvm = visitor.llvmType;
  }
  return type->llvm;
}

namespace {

ast::ParamType convert(const ast::FuncParam &param) {
  return {param.ref, param.type};
}

llvm::Type *convertParam(llvm::LLVMContext &ctx, const ast::ParamType &param) {
  TypeCat typeCat = classifyType(param.type.get());
  llvm::Type *paramType = generateType(ctx, param.type.get());
  if (param.ref == ast::ParamRef::ref || typeCat != TypeCat::trivially_copyable) {
    paramType = paramType->getPointerTo();
  }
  return paramType;
}

using LLVMTypes = std::vector<llvm::Type *>;

llvm::Type *pushRet(ast::Type *retType, llvm::Type *ret, LLVMTypes &params) {
  if (classifyType(retType) != TypeCat::trivially_copyable) {
    params.push_back(ret->getPointerTo());
    ret = llvm::Type::getVoidTy(ret->getContext());
  }
  return ret;
}

}

llvm::FunctionType *stela::generateFuncSig(llvm::LLVMContext &ctx, const ast::Func &func) {
  llvm::Type *ret = generateType(ctx, func.ret.get());
  LLVMTypes params;
  params.reserve(1 + func.params.size() + 1);
  if (func.receiver) {
    params.push_back(convertParam(ctx, convert(*func.receiver)));
  } else {
    TypeBuilder builder{ctx};
    params.push_back(builder.voidPtr());
  }
  for (const ast::FuncParam &param : func.params) {
    params.push_back(convertParam(ctx, convert(param)));
  }
  ret = pushRet(func.ret.get(), ret, params);
  return llvm::FunctionType::get(ret, params, false);
}

llvm::FunctionType *stela::generateLambSig(llvm::LLVMContext &ctx, const ast::FuncType &type) {
  llvm::Type *ret = ret = generateType(ctx, type.ret.get());
  LLVMTypes params;
  params.reserve(1 + type.params.size() + 1);
  TypeBuilder builder{ctx};
  params.push_back(builder.voidPtr());
  for (const ast::ParamType &param : type.params) {
    params.push_back(convertParam(ctx, param));
  }
  ret = pushRet(type.ret.get(), ret, params);
  return llvm::FunctionType::get(ret, params, false);
}

void stela::assignAttributes(llvm::Function *func, const sym::FuncParams &params) {
  llvm::FunctionType *type = func->getFunctionType();
  const unsigned numParams = type->getNumParams();
  const unsigned retParam = numParams > params.size() ? numParams - 1 : 0;
  for (unsigned p = 0; p != numParams; ++p) {
    llvm::Type *paramType = type->getParamType(p);
    if (paramType->isPointerTy()) {
      func->addParamAttr(p, llvm::Attribute::NonNull);
      func->addParamAttr(p, llvm::Attribute::NoCapture);
      if (p != retParam && params[p].ref != sym::ValueRef::ref) {
        func->addParamAttr(p, llvm::Attribute::NoAlias);
      }
    }
  }
  if (retParam) {
    func->addParamAttr(retParam, llvm::Attribute::NoAlias);
    func->addParamAttr(retParam, llvm::Attribute::WriteOnly);
  }
  func->addFnAttr(llvm::Attribute::NoUnwind);
}

std::string stela::generateFuncName(gen::Ctx ctx, const ast::FuncType &type) {
  /*gen::String name{16 + 16 * type.params.size()};
  const gen::String ret = generateType(ctx, type.ret.get());
  name += ret.size();
  name += "_";
  name += ret;
  for (const ast::ParamType &param : type.params) {
    name += "_";
    const gen::String paramType = generateType(ctx, param.type.get());
    const std::string_view ref = param.ref == ast::ParamRef::ref ? "_ref" : "";
    name += paramType.size() + ref.size();
    name += "_";
    name += paramType;
    name += ref;
  }
  return name;*/
  return {};
}

ast::Type *stela::concreteType(ast::Type *type) {
  if (auto *named = dynamic_cast<ast::NamedType *>(type)) {
    return concreteType(named->definition->type.get());
  }
  return type;
}

llvm::StructType *stela::generateLambdaCapture(llvm::LLVMContext &ctx, const ast::Lambda &lambda) {
  sym::Lambda *symbol = lambda.symbol;
  const size_t numCaptures = symbol->captures.size();
  TypeBuilder builder{ctx};
  std::vector<llvm::Type *> types;
  types.reserve(2 + numCaptures);
  types.push_back(builder.ref());
  types.push_back(builder.dtor());
  for (const sym::ClosureCap &cap : symbol->captures) {
    types.push_back(generateType(ctx, cap.type.get()));
  }
  return llvm::StructType::get(ctx, types);
}
