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

struct BuiltinTypes {
  ast::BuiltinType *Char;
  ast::BuiltinType *Bool;
  ast::BuiltinType *Float;
  ast::BuiltinType *Double;
  ast::BuiltinType *String;
  ast::BuiltinType *Int;
  ast::BuiltinType *Int8;
  ast::BuiltinType *Int16;
  ast::BuiltinType *Int32;
  ast::BuiltinType *Int64;
  ast::BuiltinType *UInt8;
  ast::BuiltinType *UInt16;
  ast::BuiltinType *UInt32;
  ast::BuiltinType *UInt64;
};

BuiltinTypes pushBuiltins(sym::Scopes &, ast::Decls &);

}

#endif
