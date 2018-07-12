//
//  symbols.hpp
//  STELA
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_symbols_hpp
#define stela_symbols_hpp

#include <vector>
#include <string>
#include "location.hpp"
#include <unordered_map>

namespace stela::sym {

class Visitor;

struct Symbol {
  virtual ~Symbol() = default;

  Loc loc;
  // for detecting things like: unused variable, unused function, etc
  bool referenced = false;
};
using SymbolPtr = std::unique_ptr<Symbol>;

struct BuiltinType final : Symbol {
  enum Enum {
    Void,
    Int,
    Char,
    Bool,
    Float,
    Double,
    String,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64
  } value;
};

struct Var final : Symbol {
  std::string type;
};

struct Let final : Symbol {
  std::string type;
};

using FuncParams = std::vector<std::string>;
struct Func final : Symbol {
  std::string ret;
  FuncParams params;
};

using Name = std::string_view;
using Table = std::unordered_multimap<Name, SymbolPtr>;

struct Scope {
  Name name;
  Table table;
  size_t parent;
  
  static constexpr size_t no_parent = size_t(-1);
};
using Scopes = std::vector<Scope>;

}

namespace stela {

struct Symbols {
  sym::Scopes scopes;
};

}

#endif
