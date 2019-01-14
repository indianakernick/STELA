//
//  generate closure.cpp
//  STELA
//
//  Created by Indi Kernick on 13/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "gen types.hpp"
#include "gen helpers.hpp"
#include "generate type.hpp"
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

namespace {

constexpr unsigned clo_idx_fun = 0;
constexpr unsigned clo_idx_dat = 1;

// constexpr unsigned clodat_idx_ref = 0;
constexpr unsigned clodat_idx_dtor = 1;

void stubAttributes(llvm::Function *func) {
  const size_t args = func->arg_size();
  for (unsigned a = 0; a != args; ++a) {
    if (func->hasParamAttribute(a, llvm::Attribute::ReadOnly)) {
      func->removeParamAttr(a, llvm::Attribute::ReadOnly);
      func->addParamAttr(a, llvm::Attribute::ReadNone);
    }
    if (func->hasParamAttribute(a, llvm::Attribute::WriteOnly)) {
      func->removeParamAttr(a, llvm::Attribute::WriteOnly);
      func->addParamAttr(a, llvm::Attribute::ReadNone);
    }
  }
  func->addFnAttr(llvm::Attribute::NoReturn);
}

}

template <>
llvm::Function *stela::genFn<PFGI::clo_stub>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  const Signature sig = getSignature(*clo);
  llvm::FunctionType *fnType = generateSig(ctx, sig);
  llvm::Function *func = makeInternalFunc(data.mod, fnType, "clo_stub");
  assignAttrs(func, sig);
  stubAttributes(func);
  FuncBuilder builder{func};
  llvm::Function *panic = data.inst.get<FGI::panic>();
  callPanic(builder.ir, panic, "Calling null function pointer");
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_fun_ctor>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::Type *funTy = type->getStructElementType(clo_idx_fun);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo(), funTy}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_fun_ctor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  clo.fun = fun
  clo.dat = null
  */
  
  llvm::Value *cloPtr = func->arg_begin();
  llvm::Value *fun = func->arg_begin() + 1;
  llvm::Value *cloFunPtr = builder.ir.CreateStructGEP(cloPtr, clo_idx_fun);
  builder.ir.CreateStore(fun, cloFunPtr);
  llvm::Value *cloDatPtr = builder.ir.CreateStructGEP(cloPtr, clo_idx_dat);
  setNull(builder.ir, cloDatPtr);
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_lam_ctor>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::Type *funTy = type->getStructElementType(clo_idx_fun);
  llvm::Type *datTy = type->getStructElementType(clo_idx_dat);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo(), funTy, datTy}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_lam_ctor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  func->addParamAttr(2, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  clo.fun = fun
  clo.dat = dat
  */
  
  llvm::Value *cloPtr = func->arg_begin();
  llvm::Value *fun = func->arg_begin() + 1;
  llvm::Value *dat = func->arg_begin() + 2;
  llvm::Value *cloFunPtr = builder.ir.CreateStructGEP(cloPtr, clo_idx_fun);
  builder.ir.CreateStore(fun, cloFunPtr);
  llvm::Value *cloDatPtr = builder.ir.CreateStructGEP(cloPtr, clo_idx_dat);
  builder.ir.CreateStore(dat, cloDatPtr);
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_dtor>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_dtor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  ptr_dtor(clo.dat.dtor, bitcast clo)
  */
  
  llvm::Value *cloPtr = func->arg_begin();
  llvm::Value *cloDatPtr = builder.ir.CreateStructGEP(cloPtr, clo_idx_dat);
  llvm::Value *cloDat = builder.ir.CreateLoad(cloDatPtr);
  llvm::Value *cloDatDtor = loadStructElem(builder.ir, cloDat, clodat_idx_dtor);
  llvm::Value *refPtr = refPtrPtrCast(builder.ir, cloDatPtr);
  llvm::Function *ptrDtor = data.inst.get<FGI::ptr_dtor>();
  builder.ir.CreateCall(ptrDtor, {cloDatDtor, refPtr});
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_def_ctor>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_def_ctor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  clo.fun = stub
  clo.dat = null
  */
  
  llvm::Value *cloPtr = func->arg_begin();
  llvm::Value *cloFunPtr = builder.ir.CreateStructGEP(cloPtr, clo_idx_fun);
  llvm::Function *stub = data.inst.get<PFGI::clo_stub>(clo);
  builder.ir.CreateStore(stub, cloFunPtr);
  llvm::Value *cloDatPtr = builder.ir.CreateStructGEP(cloPtr, clo_idx_dat);
  setNull(builder.ir, cloDatPtr);
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_eq>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::FunctionType *sig = compareFor(type);
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_eq");
  assignCompareAttrs(func);
  FuncBuilder builder{func};
  
  /*
  return left.fun == right.fun
  */
  
  llvm::Value *leftPtr = func->arg_begin();
  llvm::Value *rightPtr = func->arg_begin() + 1;
  llvm::Value *leftFun = loadStructElem(builder.ir, leftPtr, clo_idx_fun);
  llvm::Value *rightFun = loadStructElem(builder.ir, rightPtr, clo_idx_fun);
  llvm::Value *equal = builder.ir.CreateICmpEQ(leftFun, rightFun);
  builder.ir.CreateRet(equal);
  
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::clo_lt>(InstData data, ast::FuncType *clo) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, clo);
  llvm::FunctionType *sig = compareFor(type);
  llvm::Function *func = makeInternalFunc(data.mod, sig, "clo_lt");
  assignCompareAttrs(func);
  FuncBuilder builder{func};
  
  /*
  return left.fun < right.fun
  */
  
  llvm::Value *leftPtr = func->arg_begin();
  llvm::Value *rightPtr = func->arg_begin() + 1;
  llvm::Value *leftFun = loadStructElem(builder.ir, leftPtr, clo_idx_fun);
  llvm::Value *rightFun = loadStructElem(builder.ir, rightPtr, clo_idx_fun);
  llvm::Value *equal = builder.ir.CreateICmpULT(leftFun, rightFun);
  builder.ir.CreateRet(equal);
  
  return func;
}
