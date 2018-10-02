//
//  builtin symbols.hpp
//  STELA
//
//  Created by Indi Kernick on 16/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_builtin_symbols_hpp
#define stela_builtin_symbols_hpp

#include "ast.hpp"
#include "symbols.hpp"

namespace stela {

// bool
// byte
// char
// real
// sint
// uint

struct Builtins {
  ast::BuiltinType *Bool;
  ast::BuiltinType *Byte;
  ast::BuiltinType *Char;
  ast::BuiltinType *Real;
  ast::BuiltinType *Sint;
  ast::BuiltinType *Uint;
  
  // Alias of char[]
  std::unique_ptr<ast::ArrayType> string;
};

bool validIncr(ast::BuiltinType *);
bool validOp(ast::UnOp, ast::BuiltinType *);
ast::BuiltinType *validOp(const Builtins &, ast::BinOp, ast::BuiltinType *, ast::BuiltinType *);
bool validOp(ast::AssignOp, ast::BuiltinType *, ast::BuiltinType *);

Builtins makeBuiltins(sym::Scopes &, ast::Decls &);

}

#endif
