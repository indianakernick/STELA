//
//  semantic analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "semantic analysis.hpp"

#include "scan decl.hpp"
#include "log output.hpp"
#include "syntax analysis.hpp"

using namespace stela;

namespace {

sym::SymbolPtr makeBuiltinType(const sym::BuiltinType::Enum e) {
  auto type = std::make_unique<sym::BuiltinType>();
  type->loc = {};
  type->value = e;
  return type;
}

sym::Scope createGlobalScope() {
  sym::Scope global;
  global.parent = size_t(-1);
  
  #define INSERT(TYPE)                                                          \
    global.table.insert({#TYPE, makeBuiltinType(sym::BuiltinType::TYPE)})
  
  INSERT(Void);
  INSERT(Int);
  INSERT(Char);
  INSERT(Bool);
  INSERT(Float);
  INSERT(Double);
  INSERT(String);
  INSERT(Int8);
  INSERT(Int16);
  INSERT(Int32);
  INSERT(Int64);
  INSERT(UInt8);
  INSERT(UInt16);
  INSERT(UInt32);
  INSERT(UInt64);
  
  #undef INSERT
  
  return global;
}

}

stela::Symbols stela::createSym(const AST &ast, LogBuf &buf) {
  Log log{buf, LogCat::semantic};
  Symbols syms;
  syms.scopes.push_back(createGlobalScope());
  scanDecl(syms.scopes[0], ast, log);
  return syms;
}

stela::SymbolsAST stela::createSym(const std::string_view source, LogBuf &buf) {
  SymbolsAST sa;
  sa.ast = createAST(source, buf);
  sa.sym = createSym(sa.ast, buf);
  return sa;
}
