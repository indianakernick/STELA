//
//  function ir.cpp
//  STELA
//
//  Created by Indi Kernick on 15/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "function builder.hpp"

using namespace stela;

FunctionBuilder::FunctionBuilder(llvm::Function *func)
  : ir{func->getContext()}, func{func} {
  ir.SetInsertPoint(makeBlock("entry"));
  curr = makeBlock();
  ir.CreateBr(curr);
  ir.SetInsertPoint(curr);
}

void FunctionBuilder::setCurr(llvm::BasicBlock *block) {
  curr = block;
  ir.SetInsertPoint(block);
}

llvm::BasicBlock *FunctionBuilder::makeBlock(const llvm::Twine &name) {
  return llvm::BasicBlock::Create(func->getContext(), name, func);
}

std::vector<llvm::BasicBlock *> FunctionBuilder::makeBlocks(size_t count) {
  std::vector<llvm::BasicBlock *> blocks;
  blocks.reserve(count);
  while (count--) {
    blocks.push_back(makeBlock());
  }
  return blocks;
}

llvm::BasicBlock *FunctionBuilder::nextEmpty() {
  llvm::BasicBlock *next;
  if (curr->empty()) {
    next = curr;
  } else {
    next = makeBlock();
    ir.CreateBr(next);
  }
  return next;
}

llvm::Value *FunctionBuilder::alloc(llvm::Type *type) {
  llvm::BasicBlock *entry = &func->getEntryBlock();
  ir.SetInsertPoint(entry, std::prev(entry->end()));
  llvm::Value *addr = ir.CreateAlloca(type);
  ir.SetInsertPoint(curr);
  return addr;
}

llvm::Value *FunctionBuilder::allocStore(llvm::Type *type, llvm::Value *value) {
  llvm::Value *addr = alloc(type);
  ir.CreateStore(value, addr);
  return addr;
}

void FunctionBuilder::terminate(llvm::BasicBlock *dest) {
  if (curr->empty() || !curr->back().isTerminator()) {
    ir.CreateBr(dest);
  }
}

llvm::BasicBlock *FunctionBuilder::terminateLazy(llvm::BasicBlock *dest) {
  if (curr->empty() || !curr->back().isTerminator()) {
    if (!dest) {
      dest = makeBlock();
    }
    ir.CreateBr(dest);
  }
  return dest;
}

llvm::MutableArrayRef<llvm::Argument> FunctionBuilder::args() const {
  return {func->arg_begin(), func->arg_end()};
}
