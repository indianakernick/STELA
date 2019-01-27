//
//  binding.cpp
//  STELA
//
//  Created by Indi Kernick on 26/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "binding.hpp"

#include <llvm/ExecutionEngine/ExecutionEngine.h>

uint64_t stela::getFunctionAddress(llvm::ExecutionEngine *engine, const std::string &name) {
  return engine->getFunctionAddress(name);
}
