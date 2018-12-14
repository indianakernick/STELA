//
//  optimize module.cpp
//  STELA
//
//  Created by Indi Kernick on 12/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "optimize module.hpp"

#include "unreachable.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

namespace {

void addOptPasses(
  llvm::legacy::PassManagerBase &passes,
  llvm::legacy::FunctionPassManager &fnPasses,
  llvm::TargetMachine *machine
) {
  llvm::PassManagerBuilder builder;
  builder.OptLevel = 3;
  builder.SizeLevel = 0;
  builder.Inliner = llvm::createFunctionInliningPass(3, 0, false);
  builder.LoopVectorize = true;
  builder.SLPVectorize = true;
  machine->adjustPassManager(builder);
  
  builder.populateFunctionPassManager(fnPasses);
  builder.populateModulePassManager(passes);
}

void addLinkPasses(llvm::legacy::PassManagerBase &passes) {
  llvm::PassManagerBuilder builder;
  builder.VerifyInput = true;
  builder.Inliner = llvm::createFunctionInliningPass(3, 0, false);
  builder.populateLTOPassManager(passes);
}

}

namespace llvm {

// this is a bit sneaky!
std::unique_ptr<Module> getLazyIRFileModule(StringRef, SMDiagnostic &, LLVMContext &, bool) {
  UNREACHABLE(); return nullptr;
}

}

void stela::optimizeModule(llvm::TargetMachine *machine, llvm::Module *module) {
  // @TODO do we need to do this
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

  module->setTargetTriple(machine->getTargetTriple().str());
  module->setDataLayout(machine->createDataLayout());

  llvm::legacy::PassManager passes;
  passes.add(new llvm::TargetLibraryInfoWrapperPass(machine->getTargetTriple()));
  passes.add(llvm::createTargetTransformInfoWrapperPass(machine->getTargetIRAnalysis()));

  llvm::legacy::FunctionPassManager fnPasses(module);
  fnPasses.add(llvm::createTargetTransformInfoWrapperPass(machine->getTargetIRAnalysis()));

  addOptPasses(passes, fnPasses, machine);
  addLinkPasses(passes);

  fnPasses.doInitialization();
  for (llvm::Function &func : *module) {
    fnPasses.run(func);
  }
  fnPasses.doFinalization();

  passes.add(llvm::createVerifierPass());
  passes.run(*module);
}
