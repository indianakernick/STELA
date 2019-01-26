//
//  optimize module.cpp
//  STELA
//
//  Created by Indi Kernick on 12/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "optimize module.hpp"

#include <llvm/IR/Verifier.h>
#include "Utils/unreachable.hpp"
#include <llvm/Transforms/IPO.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/Utils/CtorUtils.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

namespace {

void addOptPasses(
  llvm::legacy::PassManagerBase &passes,
  llvm::legacy::FunctionPassManager &fnPasses,
  llvm::TargetMachine *machine,
  const stela::OptFlags opt
) {
  llvm::PassManagerBuilder builder;
  builder.OptLevel = 3;
  builder.SizeLevel = 0;
  if (opt.inliner) {
    builder.Inliner = llvm::createFunctionInliningPass(3, 0, false);
  }
  builder.LoopVectorize = opt.vectorize;
  builder.SLPVectorize = opt.vectorize;
  builder.NewGVN = true;
  builder.PrepareForLTO = true;
  machine->adjustPassManager(builder);
  
  builder.populateFunctionPassManager(fnPasses);
  builder.populateModulePassManager(passes);
}

void addLinkPasses(llvm::legacy::PassManagerBase &passes, stela::OptFlags opt) {
  llvm::PassManagerBuilder builder;
  builder.PrepareForLTO = true;
  if (opt.inliner) {
    builder.Inliner = llvm::createFunctionInliningPass(3, 0, false);
  }
  builder.populateLTOPassManager(passes);
}

bool shouldRemoveCtor(llvm::Function *ctor) {
  if (ctor->size() > 1) {
    return false;
  }
  llvm::BasicBlock *entry = &ctor->getEntryBlock();
  if (entry->size() > 1) {
    return false;
  }
  return entry->front().isTerminator();
}

}

namespace llvm {

// this is a bit sneaky!
std::unique_ptr<Module> getLazyIRFileModule(StringRef, SMDiagnostic &, LLVMContext &, bool) {
  UNREACHABLE(); return nullptr;
}

}

void stela::optimizeModule(
  llvm::TargetMachine *machine,
  llvm::Module *module,
  const OptFlags opt
) {
  module->setTargetTriple(machine->getTargetTriple().str());
  module->setDataLayout(machine->createDataLayout());
  
  /*
  
  // need to link /usr/local/opt/llvm/lib/libLLVMPasses.a
  // crashes in ModuleAnalysisManager destructor
  
  llvm::ModuleAnalysisManager MAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::LoopAnalysisManager LAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::PassBuilder PB;
  
  PB.registerModuleAnalyses(MAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
  
  auto MPM = PB.buildModuleOptimizationPipeline(llvm::PassBuilder::O3);
  MPM.run(*module, MAM);
  
  */

  llvm::legacy::PassManager passes;
  passes.add(new llvm::TargetLibraryInfoWrapperPass(machine->getTargetTriple()));
  passes.add(llvm::createTargetTransformInfoWrapperPass(machine->getTargetIRAnalysis()));

  llvm::legacy::FunctionPassManager fnPasses(module);
  fnPasses.add(llvm::createTargetTransformInfoWrapperPass(machine->getTargetIRAnalysis()));

  addOptPasses(passes, fnPasses, machine, opt);
  addLinkPasses(passes, opt);

  fnPasses.doInitialization();
  for (llvm::Function &func : *module) {
    fnPasses.run(func);
  }
  fnPasses.doFinalization();

  passes.add(llvm::createVerifierPass());
  passes.run(*module);
  
  //llvm::optimizeGlobalCtorsList(*module, &shouldRemoveCtor);
}
