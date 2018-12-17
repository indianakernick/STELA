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
#include <llvm/IR/Type.h>
#include "unreachable.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  explicit Visitor(gen::Ctx ctx)
    : ctx{ctx} {}
  
  void visit(ast::BtnType &type) override {
    switch (type.value) {
      case ast::BtnTypeEnum::Void:
        llvmType = llvm::Type::getVoidTy(ctx.llvm); return;
      case ast::BtnTypeEnum::Bool:
        llvmType = llvm::Type::getInt1Ty(ctx.llvm); return;
      case ast::BtnTypeEnum::Byte:
      case ast::BtnTypeEnum::Char:
        llvmType = llvm::Type::getInt8Ty(ctx.llvm); return;
      case ast::BtnTypeEnum::Real:
        llvmType = llvm::Type::getFloatTy(ctx.llvm); return;
      case ast::BtnTypeEnum::Sint:
      case ast::BtnTypeEnum::Uint:
        llvmType = llvm::Type::getInt32Ty(ctx.llvm); return;
    }
    UNREACHABLE();
  }
  void visit(ast::ArrayType &type) override {
    // @TODO placeholder
    llvmType = generateType(ctx, type.elem.get())->getPointerTo();
    /*type.elem->accept(*this);
    gen::String elem = std::move(name);
    name = "t_arr_";
    name += elem.size();
    name += '_';
    name += elem;
    if (ctx.inst.arrayNotInst(name)) {
      ctx.type += "typedef Array<";
      ctx.type += elem;
      ctx.type += "> ";
      ctx.type += name;
      ctx.type += ";\n";
    }*/
  }
  void visit(ast::FuncType &type) override {
    llvmType = llvm::StructType::get(ctx.llvm, {
      generateLambSig(ctx, type),
      getCloDataPtr(ctx)
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
    llvmType = llvm::StructType::get(ctx.llvm, elems);
  }
  
  llvm::Type *llvmType = nullptr;
  
private:
  gen::Ctx ctx;
};

}

llvm::Type *stela::generateType(gen::Ctx ctx, ast::Type *type) {
  assert(type);
  Visitor visitor{ctx};
  type->accept(visitor);
  return std::move(visitor.llvmType);
}

namespace {

ast::ParamType convert(const ast::FuncParam &param) {
  return {param.ref, param.type};
}

llvm::Type *convertParam(gen::Ctx ctx, const ast::ParamType &param) {
  llvm::Type *paramType = generateType(ctx, param.type.get());
  if (param.ref == ast::ParamRef::ref || paramType->isStructTy()) {
    paramType = paramType->getPointerTo();
  }
  return paramType;
}

using LLVMTypes = std::vector<llvm::Type *>;

llvm::Type *pushRet(gen::Ctx ctx, llvm::Type *ret, LLVMTypes &params) {
  if (ret->isStructTy()) {
    params.push_back(ret->getPointerTo());
    ret = llvm::Type::getVoidTy(ctx.llvm);
  }
  return ret;
}

}

llvm::FunctionType *stela::generateFuncSig(gen::Ctx ctx, const ast::Func &func) {
  llvm::Type *ret = generateType(ctx, func.ret.get());
  LLVMTypes params;
  params.reserve(1 + func.params.size() + 1);
  if (func.receiver) {
    params.push_back(convertParam(ctx, convert(*func.receiver)));
  } else {
    params.push_back(getVoidPtr(ctx));
  }
  for (const ast::FuncParam &param : func.params) {
    params.push_back(convertParam(ctx, convert(param)));
  }
  ret = pushRet(ctx, ret, params);
  return llvm::FunctionType::get(ret, params, false);
}

llvm::FunctionType *stela::generateLambSig(gen::Ctx ctx, const ast::FuncType &type) {
  llvm::Type *ret = ret = generateType(ctx, type.ret.get());
  LLVMTypes params;
  params.reserve(1 + type.params.size() + 1);
  params.push_back(getVoidPtr(ctx));
  for (const ast::ParamType &param : type.params) {
    params.push_back(convertParam(ctx, param));
  }
  ret = pushRet(ctx, ret, params);
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
      if (p == retParam) continue;
      if (params[p].ref == sym::ValueRef::ref) continue;
      if (paramType->getPointerElementType()->isStructTy()) {
        func->addParamAttr(p, llvm::Attribute::ReadOnly);
        // @TODO can we be sure that pointers to byval structs won't be aliased?
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

llvm::PointerType *stela::getVoidPtr(gen::Ctx ctx) {
  return llvm::IntegerType::getInt8PtrTy(ctx.llvm);
}

llvm::PointerType *stela::getCloDataPtr(gen::Ctx ctx) {
  llvm::FunctionType *virtualDtor = llvm::FunctionType::get(getVoidPtr(ctx), false);
  llvm::IntegerType *refCount = llvm::IntegerType::getInt32Ty(ctx.llvm);
  llvm::StructType *cloData = llvm::StructType::get(ctx.llvm, {virtualDtor, refCount});
  return llvm::PointerType::get(cloData, 0);
}

ast::Type *stela::concreteType(ast::Type *type) {
  if (auto *named = dynamic_cast<ast::NamedType *>(type)) {
    return concreteType(named->definition->type.get());
  }
  return type;
}

llvm::StructType *stela::generateLambdaCapture(gen::Ctx ctx, const ast::Lambda &lambda) {
  sym::Lambda *symbol = lambda.symbol;
  const size_t numCaptures = symbol->captures.size();
  std::vector<llvm::Type *> types;
  types.reserve(2 + numCaptures);
  types.push_back(llvm::FunctionType::get(getVoidPtr(ctx), false));
  types.push_back(llvm::IntegerType::getInt32Ty(ctx.llvm));
  for (const sym::ClosureCap &cap : symbol->captures) {
    types.push_back(generateType(ctx, cap.type.get()));
  }
  return llvm::StructType::get(ctx.llvm, types);
}
