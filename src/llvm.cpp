//
//  llvm.cpp
//  STELA
//
//  Created by Indi Kernick on 10/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "llvm.hpp"

#include <memory>
#include <cassert>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/TargetSelect.h>

namespace {

std::unique_ptr<llvm::LLVMContext> globalContext;

}

void stela::initLLVM() {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  
  assert(!globalContext);
  globalContext = std::make_unique<llvm::LLVMContext>();
}

void stela::quitLLVM() {
  assert(globalContext);
  globalContext.reset();
}

llvm::LLVMContext &stela::getLLVM() {
  assert(globalContext);
  return *globalContext;
}
