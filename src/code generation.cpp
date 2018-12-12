//
//  code generation.cpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "code generation.hpp"

#include "llvm.hpp"
#include "log output.hpp"
#include "generate ids.hpp"
#include "generate decl.hpp"
#include <llvm/IR/Verifier.h>
#include "optimize module.hpp"
#include <llvm/ExecutionEngine/ExecutionEngine.h>

llvm::ExecutionEngine *stela::generate(const Symbols &syms, LogSink &sink) {
  Log log{sink, LogCat::generate};
  log.status() << "Generating code" << endlog;
  
  gen::String type{syms.decls.size() * 100};
  gen::String func{syms.decls.size() * 100};
  gen::String code{syms.decls.size() * 1000};
  gen::TypeInst inst;
  gen::Ctx ctx {type, func, code, inst, log};
  generateIDs(syms.decls);
  auto module = std::make_unique<llvm::Module>("module", getLLVM());
  generateDecl(ctx, module.get(), syms.decls);
  
  std::string str;
  llvm::raw_string_ostream strStream(str);
  strStream << *module;
  strStream.flush();
  log.info() << '\n' << str << endlog;
  
  str.clear();
  if (llvm::verifyModule(*module, &strStream)) {
    strStream.flush();
    log.error() << str << fatal;
  }
  
  llvm::Module *modulePtr = module.get();
  auto engine = llvm::EngineBuilder(std::move(module))
                .setErrorStr(&str)
                .setOptLevel(llvm::CodeGenOpt::Aggressive)
                .setEngineKind(llvm::EngineKind::JIT)
                .create();
  if (engine == nullptr) {
    log.error() << str << fatal;
  }
  
  optimizeModule(engine, modulePtr);
  
  if (llvm::TargetMachine *machine = engine->getTargetMachine()) {
    log.info() << "Generating code for " << machine->getTargetTriple().str() << endlog;
  }
  engine->finalizeObject();
  
  return engine;
}
