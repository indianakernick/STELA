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

std::string stela::generateCpp(const Symbols &syms, LogBuf &buf) {
  Log log{buf, LogCat::generate};
  log.status() << "Generating code" << endlog;
  std::string out;
  out.reserve(syms.decls.size() * 1000);
  appendBuiltinCode(out);
  out.append("int main() {}");
  return out;
}
