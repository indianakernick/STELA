//
//  code generation.cpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "code generation.hpp"

#include "llvm.hpp"
#include "generate decl.hpp"
#include "Log/log output.hpp"
#include <llvm/IR/Verifier.h>
#include "optimize module.hpp"
#include <llvm/ExecutionEngine/MCJIT.h>

std::unique_ptr<llvm::Module> stela::generateIR(const Symbols &syms, LogSink &sink) {
  Log log{sink, LogCat::generate};
  log.status() << "Generating code" << endlog;
  
  auto module = std::make_unique<llvm::Module>("", getLLVM());
  // @TODO how do we get the native target machine object?
  // module->setTargetTriple(machine->getTargetTriple().str());
  // module->setDataLayout(machine->createDataLayout());
  FuncInst inst{module.get()};
  gen::Ctx ctx {module->getContext(), module.get(), inst, log};
  generateDecl(ctx, module.get(), syms.decls);
  
  std::string str;
  llvm::raw_string_ostream strStream(str);
  if (llvm::verifyModule(*module, &strStream)) {
    strStream.flush();
    log.error() << str << fatal;
  }
  
  return module;
}

namespace {

llvm::CodeGenOpt::Level codeGenOpt(const bool optimize) {
  return optimize ? llvm::CodeGenOpt::Aggressive : llvm::CodeGenOpt::None;
}

}

llvm::ExecutionEngine *stela::generateCode(
  std::unique_ptr<llvm::Module> module,
  LogSink &sink,
  const OptFlags opt
) {
  Log log{sink, LogCat::generate};
  
  std::string str;
  llvm::Module *modulePtr = module.get();
  auto engine = llvm::EngineBuilder(std::move(module))
                .setErrorStr(&str)
                .setOptLevel(codeGenOpt(opt.optimizeASM))
                .setEngineKind(llvm::EngineKind::JIT)
                .create();
  if (engine == nullptr) {
    log.error() << str << fatal;
  }
  
  if (opt.optimizeIR) {
    optimizeModule(engine->getTargetMachine(), modulePtr, opt);
  }
  engine->finalizeObject();
  
  // @TODO don't forget to call destructors
  engine->runStaticConstructorsDestructors(false);
  
  return engine;
}

llvm::ExecutionEngine *stela::generateCode(
  const Symbols &syms,
  LogSink &sink,
  const OptFlags flags
) {
  return generateCode(generateIR(syms, sink), sink, flags);
}
