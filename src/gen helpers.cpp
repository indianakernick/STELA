//
//  gen helpers.cpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "gen helpers.hpp"

#include "gen types.hpp"

using namespace stela;

llvm::Function *stela::makeInternalFunc(
  llvm::Module *module,
  llvm::FunctionType *type,
  const llvm::Twine &name
) {
  llvm::Function *func = llvm::Function::Create(
    type,
    llvm::Function::ExternalLinkage, // @TODO temporary. So functions aren't removed
    name,
    module
  );
  func->addFnAttr(llvm::Attribute::NoUnwind);
  func->addFnAttr(llvm::Attribute::AlwaysInline);
  return func;
}

llvm::Function *stela::declareCFunc(
  llvm::Module *module,
  llvm::FunctionType *type,
  const llvm::Twine &name
) {
  llvm::Function *func = llvm::Function::Create(
    type,
    llvm::Function::ExternalLinkage,
    name,
    module
  );
  func->addFnAttr(llvm::Attribute::NoUnwind);
  return func;
}

llvm::FunctionType *stela::unaryCtorFor(llvm::Type *type) {
  return llvm::FunctionType::get(
    voidTy(type->getContext()),
    {type->getPointerTo()},
    false
  );
}

llvm::FunctionType *stela::binaryCtorFor(llvm::Type *type) {
  return llvm::FunctionType::get(
    voidTy(type->getContext()),
    {type->getPointerTo(), type->getPointerTo()},
    false
  );
}

llvm::FunctionType *stela::compareFor(llvm::Type *type) {
  return llvm::FunctionType::get(
    llvm::Type::getInt1Ty(type->getContext()),
    {type->getPointerTo(), type->getPointerTo()},
    false
  );
}

void stela::assignUnaryCtorAttrs(llvm::Function *func) {
  func->addFnAttr(llvm::Attribute::NoRecurse);
  func->addParamAttr(0, llvm::Attribute::NonNull);
}

void stela::assignBinaryAliasCtorAttrs(llvm::Function *func) {
  assignUnaryCtorAttrs(func);
  func->addParamAttr(1, llvm::Attribute::NonNull);
}

void stela::assignBinaryCtorAttrs(llvm::Function *func) {
  assignBinaryAliasCtorAttrs(func);
  func->addParamAttr(0, llvm::Attribute::NoAlias);
  func->addParamAttr(1, llvm::Attribute::NoAlias);
}

void stela::assignCompareAttrs(llvm::Function *func) {
  assignBinaryAliasCtorAttrs(func);
  func->addAttribute(0, llvm::Attribute::ZExt);
  func->addParamAttr(0, llvm::Attribute::ReadOnly);
  func->addParamAttr(1, llvm::Attribute::ReadOnly);
  func->addFnAttr(llvm::Attribute::ReadOnly);
}

llvm::Constant *stela::constantFor(llvm::Type *type, const uint64_t value) {
  return llvm::ConstantInt::get(type, value);
}

llvm::Constant *stela::constantFor(llvm::Value *val, const uint64_t value) {
  return constantFor(val->getType(), value);
}

llvm::Constant *stela::constantForPtr(llvm::Value *ptrToVal, const uint64_t value) {
  return constantFor(ptrToVal->getType()->getPointerElementType(), value);
}

llvm::Value *stela::loadStructElem(llvm::IRBuilder<> &ir, llvm::Value *srtPtr, unsigned idx) {
  return ir.CreateLoad(ir.CreateStructGEP(srtPtr, idx));
}

llvm::Value *stela::arrayIndex(llvm::IRBuilder<> &ir, llvm::Value *ptr, llvm::Value *idx) {
  llvm::Type *sizeTy = getType<size_t>(ir.getContext());
  llvm::Value *wideIdx = ir.CreateIntCast(idx, sizeTy, false);
  llvm::Type *elemTy = ptr->getType()->getPointerElementType();
  return ir.CreateInBoundsGEP(elemTy, ptr, wideIdx);
}

void stela::setNull(llvm::IRBuilder<> &ir, llvm::Value *ptrToPtr) {
  llvm::Type *dstType = ptrToPtr->getType()->getPointerElementType();
  llvm::PointerType *dstPtrType = llvm::dyn_cast<llvm::PointerType>(dstType);
  llvm::Value *null = llvm::ConstantPointerNull::get(dstPtrType);
  ir.CreateStore(null, ptrToPtr);
}

void stela::likely(llvm::BranchInst *branch) {
  llvm::LLVMContext &ctx = branch->getContext();
  llvm::IntegerType *i32 = llvm::IntegerType::getInt32Ty(ctx);
  llvm::Metadata *troo = llvm::ConstantAsMetadata::get(constantFor(i32, 2048));
  llvm::Metadata *fols = llvm::ConstantAsMetadata::get(constantFor(i32, 1));
  llvm::MDTuple *tuple = llvm::MDNode::get(ctx, {
    llvm::MDString::get(ctx, "branch_weights"), troo, fols
  });
  branch->setMetadata("prof", tuple);
}

void stela::callPanic(llvm::IRBuilder<> &ir, llvm::Function *panic, std::string_view message) {
  ir.CreateCall(panic, ir.CreateGlobalStringPtr({message.data(), message.size()}));
  ir.CreateUnreachable();
}

llvm::Value *stela::callAlloc(llvm::IRBuilder<> &ir, llvm::Function *alloc, llvm::Type *type, llvm::Value *count) {
  llvm::LLVMContext &ctx = alloc->getContext();
  llvm::Type *sizeTy = getType<size_t>(ctx);
  llvm::Constant *size64 = llvm::ConstantExpr::getSizeOf(type);
  llvm::Constant *size = llvm::ConstantExpr::getIntegerCast(size64, sizeTy, false);
  llvm::Value *numElems = ir.CreateIntCast(count, sizeTy, false);
  llvm::Value *bytes = ir.CreateMul(size, numElems);
  llvm::Value *memPtr = ir.CreateCall(alloc, {bytes});
  return ir.CreatePointerCast(memPtr, type->getPointerTo());
}

llvm::Value *stela::callAlloc(llvm::IRBuilder<> &ir, llvm::Function *alloc, llvm::Type *type) {
  llvm::Type *sizeTy = getType<size_t>(alloc->getContext());
  return callAlloc(ir, alloc, type, constantFor(sizeTy, 1));
}

void stela::callFree(llvm::IRBuilder<> &ir, llvm::Function *free, llvm::Value *ptr) {
  ir.CreateCall(free, ir.CreatePointerCast(ptr, voidPtrTy(ptr->getContext())));
}

gen::Expr stela::lvalue(llvm::Value *obj) {
  return {obj, ValueCat::lvalue};
}

void stela::returnBool(llvm::IRBuilder<> &ir, const bool ret) {
  ir.CreateRet(ir.getInt1(ret));
}

llvm::PointerType *stela::refPtrTy(llvm::LLVMContext &ctx) {
  return refTy(ctx)->getPointerTo();
}

llvm::PointerType *stela::refPtrPtrTy(llvm::LLVMContext &ctx) {
  return refPtrTy(ctx)->getPointerTo();
}

llvm::FunctionType *stela::refPtrDtorTy(llvm::LLVMContext &ctx) {
  return llvm::FunctionType::get(
    voidTy(ctx),
    {refPtrTy(ctx)},
    false
  );
}

llvm::PointerType *stela::refPtrDtorPtrTy(llvm::LLVMContext &ctx) {
  return refPtrDtorTy(ctx)->getPointerTo();
}

llvm::Value *stela::refPtrPtrCast(llvm::IRBuilder<> &ir, llvm::Value *ptrPtr) {
  return ir.CreatePointerCast(ptrPtr, refPtrPtrTy(ir.getContext()));
}

void stela::initRefCount(llvm::IRBuilder<> &ir, llvm::Value *ptr) {
  llvm::Value *refPtr = ir.CreatePointerCast(ptr, refPtrTy(ir.getContext()));
  ir.CreateStore(constantForPtr(refPtr, 1), refPtr);
}
