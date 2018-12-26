//
//  function ir.cpp
//  STELA
//
//  Created by Indi Kernick on 15/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "function builder.hpp"

#include "generate func.hpp"

using namespace stela;

FuncBuilder::FuncBuilder(llvm::Function *func)
  : ir{func->getContext()}, func{func} {
  ir.SetInsertPoint(makeBlock("entry"));
  curr = makeBlock();
  ir.CreateBr(curr);
  ir.SetInsertPoint(curr);
}

void FuncBuilder::setCurr(llvm::BasicBlock *block) {
  curr = block;
  ir.SetInsertPoint(block);
}

void FuncBuilder::link(llvm::BasicBlock *from, llvm::BasicBlock *to) {
  ir.SetInsertPoint(from);
  ir.CreateBr(to);
  ir.SetInsertPoint(curr);
}

llvm::BasicBlock *FuncBuilder::makeBlock(const llvm::Twine &name) {
  return llvm::BasicBlock::Create(func->getContext(), name, func);
}

std::vector<llvm::BasicBlock *> FuncBuilder::makeBlocks(size_t count) {
  std::vector<llvm::BasicBlock *> blocks;
  blocks.reserve(count);
  while (count--) {
    blocks.push_back(makeBlock());
  }
  return blocks;
}

llvm::BasicBlock *FuncBuilder::nextEmpty() {
  llvm::BasicBlock *next;
  if (curr->empty()) {
    next = curr;
  } else {
    next = makeBlock();
    ir.CreateBr(next);
  }
  return next;
}

llvm::Value *FuncBuilder::alloc(llvm::Type *type) {
  llvm::BasicBlock *entry = &func->getEntryBlock();
  ir.SetInsertPoint(entry, std::prev(entry->end()));
  llvm::Value *addr = ir.CreateAlloca(type);
  ir.SetInsertPoint(curr);
  return addr;
}

llvm::Value *FuncBuilder::allocStore(llvm::Value *value) {
  llvm::Value *addr = alloc(value->getType());
  ir.CreateStore(value, addr);
  return addr;
}

void FuncBuilder::terminate(llvm::BasicBlock *dest) {
  if (curr->empty() || !curr->back().isTerminator()) {
    ir.CreateBr(dest);
  }
}

llvm::BasicBlock *FuncBuilder::terminateLazy(llvm::BasicBlock *dest) {
  if (curr->empty() || !curr->back().isTerminator()) {
    if (!dest) {
      dest = makeBlock();
    }
    ir.CreateBr(dest);
  }
  return dest;
}

llvm::MutableArrayRef<llvm::Argument> FuncBuilder::args() const {
  return {func->arg_begin(), func->arg_end()};
}

llvm::Value *FuncBuilder::callAlloc(llvm::Function *alloc, llvm::Type *type, const uint64_t elems) {
  llvm::LLVMContext &ctx = alloc->getContext();
  llvm::Type *sizeTy = getType<size_t>(ctx);
  llvm::Constant *size64 = llvm::ConstantExpr::getSizeOf(type);
  llvm::Constant *size = llvm::ConstantExpr::getIntegerCast(size64, sizeTy, false);
  llvm::Constant *numElems = llvm::ConstantInt::get(sizeTy, elems, false);
  llvm::Constant *bytes = llvm::ConstantExpr::getMul(size, numElems);
  llvm::Value *memPtr = ir.CreateCall(alloc, {bytes});
  return ir.CreatePointerCast(memPtr, type->getPointerTo());
}

void FuncBuilder::callFree(llvm::Function *free, llvm::Value *ptr) {
  llvm::Type *i8ptr = llvm::Type::getInt8PtrTy(ptr->getContext());
  ir.CreateCall(free, ir.CreatePointerCast(ptr, i8ptr));
}
