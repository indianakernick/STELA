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
#include "gen types.hpp"
#include "categories.hpp"
#include <llvm/IR/Type.h>
#include "unreachable.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  explicit Visitor(llvm::LLVMContext &ctx)
    : ctx{ctx} {}
  
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
    llvmType = ptrToArrayTy(generateType(ctx, type.elem.get()));
  }
  void visit(ast::FuncType &type) override {
    llvmType = llvm::StructType::get(ctx, {
      generateSig(ctx, getSignature(type)),
      ptrToCloDataTy(ctx)
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

ast::ParamType convert(const sym::ExprType &param) {
  return {static_cast<ast::ParamRef>(param.ref), param.type};
}

bool isBool(ast::Type *type) {
  if (auto *btn = concreteType<ast::BtnType>(type)) {
    if (btn->value == ast::BtnTypeEnum::Bool) {
      return true;
    }
  }
  return false;
}

}

llvm::FunctionType *stela::generateSig(llvm::LLVMContext &ctx, const Signature &sig) {
  std::vector<llvm::Type *> params;
  params.reserve(1 + sig.params.size());
  if (sig.receiver.type) {
    params.push_back(convertParam(ctx, sig.receiver));
  } else {
    params.push_back(voidPtrTy(ctx));
  }
  for (const ast::ParamType &param : sig.params) {
    params.push_back(convertParam(ctx, param));
  }
  llvm::Type *ret = generateType(ctx, sig.ret.get());
  if (classifyType(sig.ret.get()) == TypeCat::trivially_copyable) {
    return llvm::FunctionType::get(ret, params, false);
  } else {
    params.push_back(ret->getPointerTo());
    return llvm::FunctionType::get(voidTy(ctx), params, false);
  }
}

Signature stela::getSignature(const ast::Func &func) {
  Signature sig;
  sig.receiver = convert(func.symbol->params[0]);
  sig.params.reserve(func.params.size());
  for (auto p = func.symbol->params.cbegin() + 1; p != func.symbol->params.cend(); ++p) {
    sig.params.push_back(convert(*p));
  }
  sig.ret = func.symbol->ret.type;
  return sig;
}

Signature stela::getSignature(const ast::Lambda &lam) {
  Signature sig;
  sig.receiver = {};
  sig.params.reserve(lam.params.size());
  for (const sym::ExprType &param : lam.symbol->params) {
    sig.params.push_back(convert(param));
  }
  sig.ret = lam.symbol->ret.type;
  return sig;
}

Signature stela::getSignature(const ast::FuncType &type) {
  Signature sig;
  sig.receiver = {};
  sig.params.reserve(type.params.size());
  sig.params = type.params;
  sig.ret = type.ret;
  return sig;
}

namespace {

void assignParamAttrs(
  llvm::Function *func,
  const ast::ParamType &param,
  const unsigned idx,
  const unsigned retParam
) {
  llvm::FunctionType *fnType = func->getFunctionType();
  llvm::Type *paramType = fnType->getParamType(idx);
  if (paramType->isPointerTy()) {
    func->addParamAttr(idx, llvm::Attribute::NonNull);
    func->addParamAttr(idx, llvm::Attribute::NoCapture);
    if (idx != retParam && param.ref != ast::ParamRef::ref) {
      func->addParamAttr(idx, llvm::Attribute::NoAlias);
    }
  } else if (idx != retParam && isBool(param.type.get())) {
    func->addParamAttr(idx, llvm::Attribute::ZExt);
  }
}

}

void stela::assignAttrs(llvm::Function *func, const Signature &sig) {
  llvm::FunctionType *type = func->getFunctionType();
  const unsigned numParams = type->getNumParams();
  const unsigned retParam = numParams > sig.params.size() + 1 ? numParams - 1 : 0;
  assignParamAttrs(func, sig.receiver, 0, retParam);
  for (unsigned p = 1; p != numParams; ++p) {
    assignParamAttrs(func, sig.params[p - 1], p, retParam);
  }
  if (retParam) {
    func->addParamAttr(retParam, llvm::Attribute::NoAlias);
    func->addParamAttr(retParam, llvm::Attribute::WriteOnly);
  } else if (isBool(sig.ret.get())) {
    func->addAttribute(0, llvm::Attribute::ZExt);
  }
  func->addFnAttr(llvm::Attribute::NoUnwind);
}

llvm::Type *stela::convertParam(llvm::LLVMContext &ctx, const ast::ParamType &param) {
  TypeCat typeCat = classifyType(param.type.get());
  llvm::Type *paramType = generateType(ctx, param.type.get());
  if (param.ref == ast::ParamRef::ref || typeCat != TypeCat::trivially_copyable) {
    paramType = paramType->getPointerTo();
  }
  return paramType;
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
  std::vector<llvm::Type *> types;
  types.reserve(2 + numCaptures);
  types.push_back(refTy(ctx));
  types.push_back(dtorTy(ctx));
  for (const sym::ClosureCap &cap : symbol->captures) {
    types.push_back(generateType(ctx, cap.type.get()));
  }
  return llvm::StructType::get(ctx, types);
}
