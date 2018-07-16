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
#include <memory>
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
using Name = std::string_view;
using Table = std::unordered_multimap<Name, SymbolPtr>;

struct Scope {
  Table table;
  Scope *parent = nullptr;
};

using ScopePtr = std::unique_ptr<Scope>;
using Scopes = std::vector<ScopePtr>;

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

struct StructType final : Symbol {
  Scope *scope;
};

struct EnumType final : Symbol {
  Scope *scope;
};

struct TypeAlias final : Symbol {
  Symbol *type;
};

struct Object final : Symbol {
  Symbol *type;
  bool mut;
};

using FuncParams = std::vector<Symbol *>;
struct Func final : Symbol {
  Symbol *ret;
  FuncParams params;
  Scope *scope;
};
using FuncPtr = std::unique_ptr<Func>;

}

namespace stela {

struct Symbols {
  sym::Scopes scopes;
};

}

#endif
