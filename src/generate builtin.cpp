//
//  generate builtin.cpp
//  STELA
//
//  Created by Indi Kernick on 12/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "gen types.hpp"
#include "gen helpers.hpp"
#include "generate type.hpp"
#include "lifetime exprs.hpp"
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

template <>
llvm::Function *stela::genFn<PFGI::destroy_n>(InstData data, ast::Type *obj) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, obj);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo(), lenTy(ctx)}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "destroy_n");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  if (classifyType(obj) != TypeCat::nontrivial) {
    builder.ir.CreateRetVoid();
    return func;
  }
  
  /*
  while len != 0
    destroy dat
    dat++
    len--
  */
  
  llvm::BasicBlock *headBlock = builder.makeBlock();
  llvm::BasicBlock *bodyBlock = builder.makeBlock();
  llvm::BasicBlock *doneBlock = builder.makeBlock();
  llvm::Value *datPtr = builder.allocStore(func->arg_begin());
  llvm::Value *lenPtr = builder.allocStore(func->arg_begin() + 1);
  
  builder.branch(headBlock);
  llvm::Value *len = builder.ir.CreateLoad(lenPtr);
  llvm::Value *lenNotZero = builder.ir.CreateICmpNE(len, constantFor(len, 0));
  builder.ir.CreateCondBr(lenNotZero, bodyBlock, doneBlock);
  
  builder.setCurr(bodyBlock);
  llvm::Value *dat = builder.ir.CreateLoad(datPtr);
  LifetimeExpr lifetime{data.inst, builder.ir};
  lifetime.destroy(obj, dat);
  llvm::Value *incDat = builder.ir.CreateConstInBoundsGEP1_64(dat, 1);
  builder.ir.CreateStore(incDat, datPtr);
  llvm::Value *decLen = builder.ir.CreateNSWSub(len, constantFor(len, 1));
  builder.ir.CreateStore(decLen, lenPtr);
  builder.ir.CreateBr(headBlock);
  
  builder.setCurr(doneBlock);
  builder.ir.CreateRetVoid();
  return func;
}

namespace {

enum class CopyType {
  copy,
  move
};

llvm::Function *copyImpl(InstData data, ast::Type *obj, CopyType copyType) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, obj);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo(), lenTy(ctx), type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "move_n");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(0, llvm::Attribute::NoAlias);
  func->addParamAttr(2, llvm::Attribute::NonNull);
  func->addParamAttr(2, llvm::Attribute::NoAlias);
  func->addParamAttr(2, llvm::Attribute::WriteOnly);
  FuncBuilder builder{func};
  
  // @TODO find out why the optimizer won't emit memcpy
  // the optimizer produces a vectorized loop but not a call to memcpy
  if (classifyType(obj) != TypeCat::nontrivial) {
    llvm::Value *src = func->arg_begin();
    llvm::Value *len = func->arg_begin() + 1;
    llvm::Value *dst = func->arg_begin() + 2;
    llvm::DataLayout layout{""};
    const unsigned align = layout.getPrefTypeAlignment(type);
    builder.ir.CreateMemCpy(dst, align, src, align, len);
    builder.ir.CreateRetVoid();
    return func;
  }
  
  /*
  while len != 0
    move_construct dst src
    destroy src
    src++
    dst++
    len--
  */
  
  llvm::BasicBlock *headBlock = builder.makeBlock();
  llvm::BasicBlock *bodyBlock = builder.makeBlock();
  llvm::BasicBlock *doneBlock = builder.makeBlock();
  llvm::Value *srcPtr = builder.allocStore(func->arg_begin());
  llvm::Value *lenPtr = builder.allocStore(func->arg_begin() + 1);
  llvm::Value *dstPtr = builder.allocStore(func->arg_begin() + 2);
  
  builder.branch(headBlock);
  llvm::Value *len = builder.ir.CreateLoad(lenPtr);
  llvm::Value *lenNotZero = builder.ir.CreateICmpNE(len, constantFor(len, 0));
  builder.ir.CreateCondBr(lenNotZero, bodyBlock, doneBlock);
  
  builder.setCurr(bodyBlock);
  llvm::Value *src = builder.ir.CreateLoad(srcPtr);
  llvm::Value *dst = builder.ir.CreateLoad(dstPtr);
  LifetimeExpr lifetime{data.inst, builder.ir};
  if (copyType == CopyType::copy) {
    lifetime.copyConstruct(obj, dst, src);
  } else {
    lifetime.moveConstruct(obj, dst, src);
    lifetime.destroy(obj, src);
  }
  llvm::Value *incSrc = builder.ir.CreateConstGEP1_64(src, 1);
  builder.ir.CreateStore(incSrc, srcPtr);
  llvm::Value *incDst = builder.ir.CreateConstGEP1_64(dst, 1);
  builder.ir.CreateStore(incDst, dstPtr);
  llvm::Value *decLen = builder.ir.CreateNSWSub(len, constantFor(len, 1));
  builder.ir.CreateStore(decLen, lenPtr);
  builder.ir.CreateBr(headBlock);
  
  builder.setCurr(doneBlock);
  builder.ir.CreateRetVoid();
  return func;
}

}

template <>
llvm::Function *stela::genFn<PFGI::move_n>(InstData data, ast::Type *obj) {
  return copyImpl(data, obj, CopyType::move);
}

template <>
llvm::Function *stela::genFn<PFGI::copy_n>(InstData data, ast::Type *obj) {
  return copyImpl(data, obj, CopyType::copy);
}

template <>
llvm::Function *stela::genFn<PFGI::reallocate>(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type, lenTy(ctx)}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "reallocate");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  newDat = malloc cap
  move_n array.dat, array.len, newDat
  free array.dat
  array.dat = newDat
  array.cap = cap
  */
  
  llvm::Value *array = func->arg_begin();
  llvm::Value *cap = func->arg_begin() + 1;
  llvm::Type *storageTy = type->getPointerElementType();
  llvm::Type *elemPtrTy = storageTy->getStructElementType(array_idx_dat);
  llvm::Type *elemTy = elemPtrTy->getPointerElementType();
  llvm::Value *newDat = callAlloc(
    builder.ir, data.inst.get<FGI::alloc>(), elemTy, cap
  );
  llvm::Function *move_n = data.inst.get<PFGI::move_n>(arr->elem.get());
  llvm::Value *datPtr = builder.ir.CreateStructGEP(array, array_idx_dat);
  llvm::Value *dat = builder.ir.CreateLoad(datPtr);
  llvm::Value *len = loadStructElem(builder.ir, array, array_idx_len);
  builder.ir.CreateCall(move_n, {dat, len, newDat});
  callFree(builder.ir, data.inst.get<FGI::free>(), dat);
  builder.ir.CreateStore(newDat, datPtr);
  llvm::Value *capPtr = builder.ir.CreateStructGEP(array, array_idx_cap);
  builder.ir.CreateStore(cap, capPtr);
  
  builder.ir.CreateRetVoid();
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::btn_capacity>(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    lenTy(ctx), {type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "btn_capacity");
  assignUnaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  return array.cap
  */
  
  llvm::Value *array = builder.ir.CreateLoad(func->arg_begin());
  builder.ir.CreateRet(loadStructElem(builder.ir, array, array_idx_cap));
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::btn_size>(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    lenTy(ctx), {type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "btn_size");
  assignUnaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  return array.len
  */
  
  llvm::Value *array = builder.ir.CreateLoad(func->arg_begin());
  builder.ir.CreateRet(loadStructElem(builder.ir, array, array_idx_len));
  return func;
}

template <>
llvm::Function *stela::genFn<PFGI::btn_pop_back>(InstData data, ast::ArrayType *arr) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = generateType(ctx, arr);
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {type->getPointerTo()}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "btn_pop_back");
  assignUnaryCtorAttrs(func);
  FuncBuilder builder{func};
  
  /*
  if array.len != 0
    array.len--
    destroy array.dat[array.len]
    return
  else
    panic
  */
  
  llvm::BasicBlock *popBlock = builder.makeBlock();
  llvm::BasicBlock *panicBlock = builder.makeBlock();
  llvm::Value *array = builder.ir.CreateLoad(func->arg_begin());
  llvm::Value *lenPtr = builder.ir.CreateStructGEP(array, array_idx_len);
  llvm::Value *len = builder.ir.CreateLoad(lenPtr);
  llvm::Value *notEmpty = builder.ir.CreateICmpNE(len, constantFor(len, 0));
  likely(builder.ir.CreateCondBr(notEmpty, popBlock, panicBlock));
  
  builder.setCurr(popBlock);
  llvm::Value *lenMinus1 = builder.ir.CreateNSWSub(len, constantFor(len, 1));
  builder.ir.CreateStore(lenMinus1, lenPtr);
  llvm::Value *dat = loadStructElem(builder.ir, array, array_idx_dat);
  LifetimeExpr lifetime{data.inst, builder.ir};
  lifetime.destroy(arr->elem.get(), arrayIndex(builder.ir, dat, lenMinus1));
  builder.ir.CreateRetVoid();
  
  builder.setCurr(panicBlock);
  callPanic(builder.ir, data.inst.get<FGI::panic>(), "pop_back from empty array");
  
  return func;
}
