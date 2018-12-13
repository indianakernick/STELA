//
//  optimize module.cpp
//  STELA
//
//  Created by Indi Kernick on 12/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "optimize module.hpp"

#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/Transforms/Scalar/SimpleLoopUnswitch.h>

void stela::optimizeModule(llvm::ExecutionEngine *, llvm::Module *module) {
  llvm::FunctionPassManager FPM{false};
  llvm::FunctionAnalysisManager FAM{false};

  for (llvm::Function &func : module->functions()) {
    FPM.run(func, FAM);
  }
}
