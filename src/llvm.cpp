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
#include <llvm/IR/LegacyPassManager.h>

namespace {

std::unique_ptr<llvm::LLVMContext> globalContext;

void initializeOptimizers() {
  llvm::PassRegistry &registry = *llvm::PassRegistry::getPassRegistry();
  llvm::initializeCore(registry);
  llvm::initializeScalarOpts(registry);
  llvm::initializeVectorization(registry);
  llvm::initializeIPO(registry);
  llvm::initializeAnalysis(registry);
  llvm::initializeTransformUtils(registry);
  llvm::initializeInstCombine(registry);
  llvm::initializeAggressiveInstCombine(registry);
  llvm::initializeInstrumentation(registry);
  llvm::initializeTarget(registry);
  llvm::initializeExpandMemCmpPassPass(registry);
  llvm::initializeScalarizeMaskedMemIntrinPass(registry);
  llvm::initializeCodeGenPreparePass(registry);
  llvm::initializeAtomicExpandPass(registry);
  llvm::initializeRewriteSymbolsLegacyPassPass(registry);
  llvm::initializeWinEHPreparePass(registry);
  llvm::initializeDwarfEHPreparePass(registry);
  llvm::initializeSafeStackLegacyPassPass(registry);
  llvm::initializeSjLjEHPreparePass(registry);
  llvm::initializePreISelIntrinsicLoweringLegacyPassPass(registry);
  llvm::initializeGlobalMergePass(registry);
  llvm::initializeIndirectBrExpandPassPass(registry);
  llvm::initializeInterleavedAccessPass(registry);
  llvm::initializeEntryExitInstrumenterPass(registry);
  llvm::initializePostInlineEntryExitInstrumenterPass(registry);
  llvm::initializeUnreachableBlockElimLegacyPassPass(registry);
  llvm::initializeExpandReductionsPass(registry);
  llvm::initializeWasmEHPreparePass(registry);
  llvm::initializeWriteBitcodePassPass(registry);
}

}

void stela::initLLVM() {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  
  assert(!globalContext);
  globalContext = std::make_unique<llvm::LLVMContext>();
  
  initializeOptimizers();
}

void stela::quitLLVM() {
  assert(globalContext);
  globalContext.reset();
}

llvm::LLVMContext &stela::getLLVM() {
  assert(globalContext);
  return *globalContext;
}
