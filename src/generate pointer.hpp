//
//  generate pointer.hpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_pointer_hpp
#define stela_generate_pointer_hpp

#include "function builder.hpp"
#include "func instantiations.hpp"

namespace stela {

// @TODO we really should be generating functions here instead of doing this

llvm::Value *ptrDefCtor(FuncInst &, FuncBuilder &, llvm::Type *);
void ptrDtor(
  FuncInst &,
  FuncBuilder &,
  llvm::Value *,
  llvm::Function *,
  llvm::BasicBlock *
);
void ptrCopCtor(FuncBuilder &, llvm::Value *, llvm::Value *);
void ptrMovCtor(FuncBuilder &, llvm::Value *, llvm::Value *);
void ptrCopAsgn(
  FuncInst &,
  FuncBuilder &,
  llvm::Value *,
  llvm::Value *,
  llvm::Function *,
  llvm::BasicBlock *
);
void ptrMovAsgn(
  FuncInst &,
  FuncBuilder &,
  llvm::Value *,
  llvm::Value *,
  llvm::Function *,
  llvm::BasicBlock *
);

}

#endif
