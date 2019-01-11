//
//  generate pointer.cpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "generate pointer.hpp"

#include "gen helpers.hpp"
#include "type builder.hpp"
#include "function builder.hpp"

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
  FuncBuilder &funcBdr,
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
  
  llvm::BasicBlock *del = funcBdr.makeBlock();
  llvm::Value *subed = refChange(funcBdr.ir, ptr, RefChg::dec);
  llvm::Value *isZero = funcBdr.ir.CreateICmpEQ(subed, constantFor(subed, 0));
  funcBdr.ir.CreateCondBr(isZero, del, done);
  funcBdr.setCurr(del);
  funcBdr.ir.CreateCall(dtor, {ptr});
  callFree(funcBdr.ir, inst.free(), ptr);
  funcBdr.branch(done);
}

}

llvm::Function *stela::genPtrDtor(InstData data) {
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
  FuncBuilder funcBdr{func};
  
  /*
  if ptr == null
    return
  else
    reset ptr
    return
  */
  
  llvm::BasicBlock *decr = funcBdr.makeBlock();
  llvm::BasicBlock *done = funcBdr.makeBlock();
  llvm::Value *dtor = func->arg_begin();
  llvm::Value *ptr = funcBdr.ir.CreateLoad(func->arg_begin() + 1);
  funcBdr.ir.CreateCondBr(funcBdr.ir.CreateIsNull(ptr), done, decr);
  
  funcBdr.setCurr(decr);
  resetPtr(data.inst, funcBdr, ptr, dtor, done);
  funcBdr.ir.CreateRetVoid();
  
  return func;
}

llvm::Function *stela::genPtrCopCtor(InstData data) {
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
  FuncBuilder funcBdr{func};
  
  /*
  other.ref++
  obj = other
  */
  
  llvm::Value *other = funcBdr.ir.CreateLoad(func->arg_begin() + 1);
  refChange(funcBdr.ir, other, RefChg::inc);
  funcBdr.ir.CreateStore(other, func->arg_begin());
  
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genPtrCopAsgn(InstData data) {
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
  FuncBuilder funcBdr{func};
  
  /*
  right.ref++
  reset left
  left = right
  */
  
  llvm::Value *dtor = func->arg_begin();
  llvm::Value *leftPtr = func->arg_begin() + 1;
  llvm::Value *rightPtr = func->arg_begin() + 2;
  llvm::BasicBlock *asgn = funcBdr.makeBlock();
  llvm::Value *right = funcBdr.ir.CreateLoad(rightPtr);
  refChange(funcBdr.ir, right, RefChg::inc);
  llvm::Value *left = funcBdr.ir.CreateLoad(leftPtr);
  resetPtr(data.inst, funcBdr, left, dtor, asgn);
  funcBdr.ir.CreateStore(right, leftPtr);
  
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genPtrMovCtor(InstData data) {
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
  FuncBuilder funcBdr{func};
  
  /*
  obj = other
  other = null
  */
  
  llvm::Value *otherPtr = func->arg_begin() + 1;
  funcBdr.ir.CreateStore(funcBdr.ir.CreateLoad(otherPtr), func->arg_begin());
  setNull(funcBdr.ir, otherPtr);
  
  funcBdr.ir.CreateRetVoid();
  return func;
}

llvm::Function *stela::genPtrMovAsgn(InstData data) {
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
  FuncBuilder funcBdr{func};
  
  /*
  reset left
  left = right
  right = null
  */
  
  llvm::Value *dtor = func->arg_begin();
  llvm::Value *leftPtr = func->arg_begin() + 1;
  llvm::Value *rightPtr = func->arg_begin() + 2;
  llvm::BasicBlock *asgn = funcBdr.makeBlock();
  llvm::Value *left = funcBdr.ir.CreateLoad(leftPtr);
  resetPtr(data.inst, funcBdr, left, dtor, asgn);
  funcBdr.ir.CreateStore(funcBdr.ir.CreateLoad(rightPtr), leftPtr);
  setNull(funcBdr.ir, rightPtr);
  
  funcBdr.ir.CreateRetVoid();
  return func;
}
