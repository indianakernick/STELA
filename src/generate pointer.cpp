//
//  generate pointer.cpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "gen helpers.hpp"
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

namespace {

enum class RefChg {
  inc,
  dec
};

llvm::Value *refChange(llvm::IRBuilder<> &ir, llvm::Value *ptr, const RefChg chg) {
  llvm::Value *ref = ir.CreateLoad(ptr);
  llvm::Constant *one = constantFor(ref, 1);
  llvm::Value *changed = chg == RefChg::inc ? ir.CreateNSWAdd(ref, one)
                                            : ir.CreateNSWSub(ref, one);
  ir.CreateStore(changed, ptr);
  return changed;
}

void resetPtr(
  FuncInst &inst,
  FuncBuilder &builder,
  llvm::Value *ptr,
  llvm::Value *dtor,
  llvm::BasicBlock *done
) {
  /*
  ptr.ref--
  if ptr.ref == 0
    dtor
    free ptr
    done
  else
    done
  */
  
  llvm::BasicBlock *del = builder.makeBlock();
  llvm::Value *subed = refChange(builder.ir, ptr, RefChg::dec);
  llvm::Value *isZero = builder.ir.CreateICmpEQ(subed, constantFor(subed, 0));
  builder.ir.CreateCondBr(isZero, del, done);
  builder.setCurr(del);
  builder.ir.CreateCall(dtor, {ptr});
  callFree(builder.ir, inst.get<FGI::free>(), ptr);
  builder.branch(done);
}

}

template <>
llvm::Function *stela::genFn<FGI::ptr_dtor>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::FunctionType *sig = llvm::FunctionType::get(
    llvm::Type::getVoidTy(ctx),
    {refPtrDtorPtrTy(ctx), refPtrPtrTy(ctx)},
    false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "ptr_dtor");
  func->addParamAttr(0, llvm::Attribute::ReadOnly);
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  if ptr == null
    return
  else
    reset ptr
    return
  */
  
  llvm::BasicBlock *decr = builder.makeBlock();
  llvm::BasicBlock *done = builder.makeBlock();
  llvm::Value *dtor = func->arg_begin();
  llvm::Value *ptr = builder.ir.CreateLoad(func->arg_begin() + 1);
  builder.ir.CreateCondBr(builder.ir.CreateIsNull(ptr), done, decr);
  
  builder.setCurr(decr);
  resetPtr(data.inst, builder, ptr, dtor, done);
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<FGI::ptr_cop_ctor>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::FunctionType *sig = llvm::FunctionType::get(
    llvm::Type::getVoidTy(ctx),
    {refPtrPtrTy(ctx), refPtrPtrTy(ctx)},
    false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "ptr_cop_ctor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(0, llvm::Attribute::NoAlias);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NoAlias);
  FuncBuilder builder{func};
  
  /*
  other.ref++
  obj = other
  */
  
  llvm::Value *other = builder.ir.CreateLoad(func->arg_begin() + 1);
  refChange(builder.ir, other, RefChg::inc);
  builder.ir.CreateStore(other, func->arg_begin());
  
  builder.ir.CreateRetVoid();
  return func;
}

template <>
llvm::Function *stela::genFn<FGI::ptr_cop_asgn>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::FunctionType *sig = llvm::FunctionType::get(
    llvm::Type::getVoidTy(ctx),
    {refPtrDtorPtrTy(ctx), refPtrPtrTy(ctx), refPtrPtrTy(ctx)},
    false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "ptr_cop_asgn");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(0, llvm::Attribute::ReadOnly);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  func->addParamAttr(2, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  right.ref++
  reset left
  left = right
  */
  
  llvm::Value *dtor = func->arg_begin();
  llvm::Value *leftPtr = func->arg_begin() + 1;
  llvm::Value *rightPtr = func->arg_begin() + 2;
  llvm::BasicBlock *asgn = builder.makeBlock();
  llvm::Value *right = builder.ir.CreateLoad(rightPtr);
  refChange(builder.ir, right, RefChg::inc);
  llvm::Value *left = builder.ir.CreateLoad(leftPtr);
  resetPtr(data.inst, builder, left, dtor, asgn);
  builder.ir.CreateStore(right, leftPtr);
  
  builder.ir.CreateRetVoid();
  return func;
}

template <>
llvm::Function *stela::genFn<FGI::ptr_mov_ctor>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::FunctionType *sig = llvm::FunctionType::get(
    llvm::Type::getVoidTy(ctx),
    {refPtrPtrTy(ctx), refPtrPtrTy(ctx)},
    false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "ptr_mov_ctor");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(0, llvm::Attribute::NoAlias);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NoAlias);
  FuncBuilder builder{func};
  
  /*
  obj = other
  other = null
  */
  
  llvm::Value *otherPtr = func->arg_begin() + 1;
  builder.ir.CreateStore(builder.ir.CreateLoad(otherPtr), func->arg_begin());
  setNull(builder.ir, otherPtr);
  
  builder.ir.CreateRetVoid();
  return func;
}

template <>
llvm::Function *stela::genFn<FGI::ptr_mov_asgn>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::FunctionType *sig = llvm::FunctionType::get(
    llvm::Type::getVoidTy(ctx),
    {refPtrDtorPtrTy(ctx), refPtrPtrTy(ctx), refPtrPtrTy(ctx)},
    false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "ptr_mov_asgn");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(0, llvm::Attribute::ReadOnly);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NoAlias);
  func->addParamAttr(2, llvm::Attribute::NonNull);
  func->addParamAttr(2, llvm::Attribute::NoAlias);
  FuncBuilder builder{func};
  
  /*
  reset left
  left = right
  right = null
  */
  
  llvm::Value *dtor = func->arg_begin();
  llvm::Value *leftPtr = func->arg_begin() + 1;
  llvm::Value *rightPtr = func->arg_begin() + 2;
  llvm::BasicBlock *asgn = builder.makeBlock();
  llvm::Value *left = builder.ir.CreateLoad(leftPtr);
  resetPtr(data.inst, builder, left, dtor, asgn);
  builder.ir.CreateStore(builder.ir.CreateLoad(rightPtr), leftPtr);
  setNull(builder.ir, rightPtr);
  
  builder.ir.CreateRetVoid();
  return func;
}
