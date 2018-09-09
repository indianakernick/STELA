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

// When are sized integers useful?
// Compatability with C++
// Writing fast code
// ...

// char
// bool
// real
// int
// byte
// uint

struct Builtins {
  ast::BuiltinType *Bool;
  ast::BuiltinType *Char;
  ast::BuiltinType *Float;
  ast::BuiltinType *Double;
  ast::BuiltinType *Int8;
  ast::BuiltinType *Int16;
  ast::BuiltinType *Int32;
  ast::BuiltinType *Int64;
  ast::BuiltinType *UInt8;
  ast::BuiltinType *UInt16;
  ast::BuiltinType *UInt32;
  ast::BuiltinType *UInt64;
  ast::BuiltinType *String;
};

bool validIncr(ast::BuiltinType *);
bool validOp(ast::UnOp, ast::BuiltinType *);
ast::BuiltinType *validOp(const Builtins &, ast::BinOp, ast::BuiltinType *, ast::BuiltinType *);
bool validOp(ast::AssignOp, ast::BuiltinType *, ast::BuiltinType *);

Builtins makeBuiltins(sym::Scopes &, ast::Decls &);

}

#endif
