//
//  reference count.cpp
//  STELA
//
//  Created by Indi Kernick on 17/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "reference count.hpp"

using namespace stela;

ReferenceCount::ReferenceCount(llvm::IRBuilder<> &ir)
  : ir{ir} {}

namespace {

llvm::Constant *constantFor(llvm::Value *val, const uint64_t value) {
  return llvm::ConstantInt::get(val->getType(), value);
}

}

void ReferenceCount::incr(llvm::Value *objPtr) {
  llvm::Value *refPtr = ir.CreateStructGEP(objPtr, 0);
  llvm::Value *value = ir.CreateLoad(refPtr);
  llvm::Constant *one = constantFor(value, 1);
  llvm::Value *added = ir.CreateAdd(value, one);
  ir.CreateStore(added, refPtr);
}

llvm::Value *ReferenceCount::decr(llvm::Value *objPtr) {
  llvm::Value *refPtr = ir.CreateStructGEP(objPtr, 0);
  llvm::Value *value = ir.CreateLoad(refPtr);
  llvm::Constant *one = constantFor(value, 1);
  llvm::Value *subed = ir.CreateSub(value, one);
  ir.CreateStore(subed, refPtr);
  return ir.CreateICmpEQ(subed, constantFor(value, 0));
}
