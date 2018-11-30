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

std::string stela::generateCpp(const Symbols &syms, LogBuf &buf) {
  Log log{buf, LogCat::generate};
  log.status() << "Generating code" << endlog;
  gen::String typ{syms.decls.size() * 100};
  gen::String fun{syms.decls.size() * 1000};
  gen::TypeInst inst;
  gen::Ctx ctx {typ, fun, inst, log};
  generateIDs(syms.decls);
  generateDecl(ctx, syms.decls);
  std::string bin;
  bin.reserve(syms.decls.size() * 1000 + 8000);
  appendBuiltinCode(bin);
  appendBeginTypes(bin);
  bin.append(typ.str());
  appendEndTypes(bin);
  appendBeginCode(bin);
  bin.append(fun.str());
  appendEndCode(bin);
  bin.append("int main() {}");
  return bin;
}
