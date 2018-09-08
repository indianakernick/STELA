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

// char    
// bool
// float
// int

struct Builtins {
  ast::BuiltinType *Char;
  ast::BuiltinType *Bool;
  ast::BuiltinType *Float;
  ast::BuiltinType *Double;
  ast::BuiltinType *String;
  ast::BuiltinType *Int8;
  ast::BuiltinType *Int16;
  ast::BuiltinType *Int32;
  ast::BuiltinType *Int64;
  ast::BuiltinType *UInt8;
  ast::BuiltinType *UInt16;
  ast::BuiltinType *UInt32;
  ast::BuiltinType *UInt64;
  
  bool isInteger(ast::BuiltinType *) const;
  bool isNumber(ast::BuiltinType *) const;
  
  bool validIncr(bool, ast::BuiltinType *) const;
  bool validOp(ast::UnOp, ast::BuiltinType *) const;
  bool validOp(ast::BinOp, ast::BuiltinType *, ast::BuiltinType *) const;
  bool validOp(ast::AssignOp, ast::BuiltinType *, ast::BuiltinType *) const;
};

Builtins makeBuiltins(sym::Scopes &, ast::Decls &);

}

#endif
