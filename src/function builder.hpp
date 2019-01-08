//
//  function builder.hpp
//  STELA
//
//  Created by Indi Kernick on 15/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_function_builder_hpp
#define stela_function_builder_hpp

#include <llvm/IR/IRBuilder.h>

namespace stela {

class FuncBuilder {
public:
  explicit FuncBuilder(llvm::Function *);
  
  /// Set the current block
  void setCurr(llvm::BasicBlock *);
  /// Append a branch instruction to the first block that points to the
  /// second block
  void link(llvm::BasicBlock *, llvm::BasicBlock *);
  
  /// Create a new block attached to this function
  llvm::BasicBlock *makeBlock(const llvm::Twine & = "");
  /// Create a range of blocks attached to this function
  std::vector<llvm::BasicBlock *> makeBlocks(size_t);
  /// If the current block is empty, return the current block, otherwise return
  /// a new block
  llvm::BasicBlock *nextEmpty();
  
  /// Insert an alloca instruction in the entry block
  llvm::Value *alloc(llvm::Type *);
  /// Insert an alloca instruction in the entry block and store a value to it
  llvm::Value *allocStore(llvm::Value *);
 
  /// If the current block has not already been terminated,
  /// terminate by branching to the given block
  void terminate(llvm::BasicBlock *);
  /// Similar to terminate but lazily create the destination block and return it
  llvm::BasicBlock *terminateLazy(llvm::BasicBlock *);
  
  /// Get the function arguments
  llvm::MutableArrayRef<llvm::Argument> args() const;
  
  /// Allocate space for an object
  llvm::Value *callAlloc(llvm::Function *, llvm::Type *);
  /// Allocate space for a runtime number of objects
  llvm::Value *callAlloc(llvm::Function *, llvm::Type *, llvm::Value *);
  /// Make a call to free
  void callFree(llvm::Function *, llvm::Value *);
  
  llvm::IRBuilder<> ir;
  
private:
  llvm::Function *func;
  llvm::BasicBlock *curr;
};

}

#endif
