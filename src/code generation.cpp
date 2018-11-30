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
#include "generate decl.hpp"

std::string stela::generateCpp(const Symbols &syms, LogBuf &buf) {
  Log log{buf, LogCat::generate};
  log.status() << "Generating code" << endlog;
  std::string bin;
  bin.reserve(syms.decls.size() * 1000 + 8000);
  appendBuiltinCode(bin);
  generateDecl(bin, log, syms.decls);
  bin.append("int main() {}");
  return bin;
}
