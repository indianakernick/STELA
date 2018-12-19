//
//  type builder.cpp
//  STELA
//
//  Created by Indi Kernick on 17/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "type builder.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>

using namespace stela;

TypeBuilder::TypeBuilder(llvm::LLVMContext &ctx)
  : ctx{ctx} {}

llvm::IntegerType *TypeBuilder::len() const {
  return llvm::IntegerType::getInt32Ty(ctx);
}

llvm::IntegerType *TypeBuilder::ref() const {
  return llvm::IntegerType::getInt64Ty(ctx);
}

llvm::PointerType *TypeBuilder::voidPtr() const {
  return llvm::IntegerType::getInt8PtrTy(ctx);
}

llvm::FunctionType *TypeBuilder::dtor() const {
  return llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {voidPtr()}, false);
}

llvm::StructType *TypeBuilder::cloData() const {
  return llvm::StructType::get(ctx, {
    ref(), // reference count
    dtor() // virtual destructor
  });
}

llvm::StructType *TypeBuilder::arrayOf(llvm::Type *elem) const {
  return llvm::StructType::get(ctx, {
    ref(),               // reference count
    len(),               // capacity
    len(),               // length
    elem->getPointerTo() // data
  }, true); // @TODO do we need to declare the struct packed
}

llvm::ConstantPointerNull *TypeBuilder::nullPtr(llvm::PointerType *ptr) const {
  return llvm::ConstantPointerNull::get(ptr);
}

llvm::ConstantPointerNull *TypeBuilder::nullPtrTo(llvm::Type *type) const {
  return nullPtr(type->getPointerTo());
}

llvm::ArrayType *TypeBuilder::wrap(llvm::Type *type) const {
  return llvm::ArrayType::get(type, 1);
}

llvm::ArrayType *TypeBuilder::wrapPtrTo(llvm::Type *type) const {
  return wrap(type->getPointerTo());
}

llvm::ArrayType *TypeBuilder::wrapPtrToArrayOf(llvm::Type *elem) const {
  return wrapPtrTo(arrayOf(elem));
}
