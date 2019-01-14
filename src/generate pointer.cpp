//
//  generate pointer.cpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "gen types.hpp"
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

}

template <>
llvm::Function *stela::genFn<FGI::ptr_inc>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx), {refPtrTy(ctx)}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "ptr_inc");
  FuncBuilder builder{func};
  
  /*
  if ptr != null
    ptr.ref++
  */
  
  llvm::BasicBlock *incBlock = builder.makeBlock();
  llvm::BasicBlock *doneBlock = builder.makeBlock();
  llvm::Value *ptrNotNull = builder.ir.CreateIsNotNull(func->arg_begin());
  builder.ir.CreateCondBr(ptrNotNull, incBlock, doneBlock);
  
  builder.setCurr(incBlock);
  refChange(builder.ir, func->arg_begin(), RefChg::inc);
  builder.ir.CreateBr(doneBlock);
  
  builder.setCurr(doneBlock);
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<FGI::ptr_dec>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx),
    {ptrToDtorTy(ctx), refPtrTy(ctx)},
    false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "ptr_dec");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  if ptr != null
    ptr.ref--
    if ptr.ref == 0
      dtor
      free ptr
  */
  
  llvm::BasicBlock *decBlock = builder.makeBlock();
  llvm::BasicBlock *destroyBlock = builder.makeBlock();
  llvm::BasicBlock *doneBlock = builder.makeBlock();
  llvm::Value *dtor = func->arg_begin();
  llvm::Value *ptr = func->arg_begin() + 1;
  llvm::Value *ptrNotNull = builder.ir.CreateIsNotNull(ptr);
  builder.ir.CreateCondBr(ptrNotNull, decBlock, doneBlock);
  
  builder.setCurr(decBlock);
  llvm::Value *subed = refChange(builder.ir, ptr, RefChg::dec);
  llvm::Value *refIsZero = builder.ir.CreateICmpEQ(subed, constantFor(subed, 0));
  builder.ir.CreateCondBr(refIsZero, destroyBlock, doneBlock);
  
  builder.setCurr(destroyBlock);
  llvm::Value *voidPtr = builder.ir.CreatePointerCast(ptr, voidPtrTy(ctx));
  builder.ir.CreateCall(dtor, {voidPtr});
  callFree(builder.ir, data.inst.get<FGI::free>(), ptr);
  builder.ir.CreateBr(doneBlock);
  
  builder.setCurr(doneBlock);
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<FGI::ptr_dtor>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx),
    {ptrToDtorTy(ctx), refPtrPtrTy(ctx)},
    false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "ptr_dtor");
  func->addParamAttr(0, llvm::Attribute::ReadOnly);
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  dec ptr
  */
  
  llvm::Value *dtor = func->arg_begin();
  llvm::Value *ptr = builder.ir.CreateLoad(func->arg_begin() + 1);
  llvm::Function *dec = data.inst.get<FGI::ptr_dec>();
  builder.ir.CreateCall(dec, {dtor, ptr});
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<FGI::ptr_cop_ctor>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx),
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
  inc other
  obj = other
  */
  
  llvm::Value *objPtr = func->arg_begin();
  llvm::Value *other = builder.ir.CreateLoad(func->arg_begin() + 1);
  llvm::Function *inc = data.inst.get<FGI::ptr_inc>();
  builder.ir.CreateCall(inc, {other});
  builder.ir.CreateStore(other, objPtr);
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<FGI::ptr_cop_asgn>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx),
    {ptrToDtorTy(ctx), refPtrPtrTy(ctx), refPtrPtrTy(ctx)},
    false
  );
  llvm::Function *func = makeInternalFunc(data.mod, sig, "ptr_cop_asgn");
  func->addParamAttr(0, llvm::Attribute::NonNull);
  func->addParamAttr(0, llvm::Attribute::ReadOnly);
  func->addParamAttr(1, llvm::Attribute::NonNull);
  func->addParamAttr(2, llvm::Attribute::NonNull);
  FuncBuilder builder{func};
  
  /*
  inc right
  dec left
  left = right
  */
  
  llvm::Value *dtor = func->arg_begin();
  llvm::Value *leftPtr = func->arg_begin() + 1;
  llvm::Value *left = builder.ir.CreateLoad(leftPtr);
  llvm::Value *right = builder.ir.CreateLoad(func->arg_begin() + 2);
  llvm::Function *inc = data.inst.get<FGI::ptr_inc>();
  builder.ir.CreateCall(inc, {right});
  llvm::Function *dec = data.inst.get<FGI::ptr_dec>();
  builder.ir.CreateCall(dec, {dtor, left});
  builder.ir.CreateStore(right, leftPtr);
  builder.ir.CreateRetVoid();
  
  return func;
}

template <>
llvm::Function *stela::genFn<FGI::ptr_mov_ctor>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::FunctionType *sig = llvm::FunctionType::get(
    voidTy(ctx),
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
    voidTy(ctx),
    {ptrToDtorTy(ctx), refPtrPtrTy(ctx), refPtrPtrTy(ctx)},
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
  dec left
  left = right
  right = null
  */
  
  llvm::Value *dtor = func->arg_begin();
  llvm::Value *leftPtr = func->arg_begin() + 1;
  llvm::Value *rightPtr = func->arg_begin() + 2;
  llvm::Function *dec = data.inst.get<FGI::ptr_dec>();
  builder.ir.CreateCall(dec, {dtor, builder.ir.CreateLoad(leftPtr)});
  builder.ir.CreateStore(builder.ir.CreateLoad(rightPtr), leftPtr);
  setNull(builder.ir, rightPtr);
  
  builder.ir.CreateRetVoid();
  return func;
}
