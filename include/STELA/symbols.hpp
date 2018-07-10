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
#include "ast.hpp"
#include <unordered_map>

namespace stela::sym {

struct Symbol {
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
  ast::TypePtr type;
};

struct Let final : Symbol {
  ast::TypePtr type;
};

using FuncParams = std::vector<ast::TypePtr>;
struct Func final : Symbol {
  ast::TypePtr ret;
  FuncParams params;
};

using Name = std::string_view;
using Table = std::unordered_map<Name, SymbolPtr>;

struct Scope {
  Name name;
  Table table;
  size_t parent;
};
using Scopes = std::vector<Scope>;
using ScopeStack = std::vector<size_t>;

}

namespace stela {

struct Symbols {
  sym::Scopes scopes;
};

}

#endif
