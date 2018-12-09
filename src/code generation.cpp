//
//  code generation.cpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "code generation.hpp"

#include "log output.hpp"
#include "builtin code.hpp"
#include "generate ids.hpp"
#include "generate decl.hpp"

std::string stela::generateCpp(const Symbols &syms, LogSink &sink) {
  Log log{sink, LogCat::generate};
  log.status() << "Generating code" << endlog;
  gen::String type{syms.decls.size() * 100};
  gen::String func{syms.decls.size() * 100};
  gen::String code{syms.decls.size() * 1000};
  gen::TypeInst inst;
  gen::Ctx ctx {type, func, code, inst, log};
  generateIDs(syms.decls);
  generateDecl(ctx, syms.decls);
  std::string bin;
  bin.reserve(syms.decls.size() * 1000 + 8000);
  appendBuiltinCode(bin);
  appendTypes(bin, type);
  appendFuncs(bin, func);
  appendCode(bin, code);
  bin.append("int main() {}");
  return bin;
}
