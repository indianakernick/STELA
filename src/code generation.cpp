//
//  code generation.cpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "code generation.hpp"

#include "log output.hpp"

std::string stela::generateCpp(const Symbols &syms, LogBuf &buf) {
  Log log{buf, LogCat::generate};
  log.status() << "Generating code" << endlog;
  return "int main() {}\n";
}
