//
//  builtin symbols.hpp
//  STELA
//
//  Created by Indi Kernick on 16/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_builtin_symbols_hpp
#define stela_builtin_symbols_hpp

#include "symbols.hpp"

namespace stela {

struct BuiltinTypes {
  sym::Symbol *Void = nullptr;
  sym::Symbol *Char = nullptr;
  sym::Symbol *Bool = nullptr;
  sym::Symbol *Float = nullptr;
  sym::Symbol *Double = nullptr;
  sym::Symbol *String = nullptr;
  sym::Symbol *Int8 = nullptr;
  sym::Symbol *Int16 = nullptr;
  sym::Symbol *Int32 = nullptr;
  sym::Symbol *Int64 = nullptr;
  sym::Symbol *UInt8 = nullptr;
  sym::Symbol *UInt16 = nullptr;
  sym::Symbol *UInt32 = nullptr;
  sym::Symbol *UInt64 = nullptr;
};

sym::ScopePtr createBuiltinScope(BuiltinTypes &);

}

#endif
