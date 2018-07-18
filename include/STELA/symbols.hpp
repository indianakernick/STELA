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

enum class ValueCat {
  // order from most restrictive to least restrictive
  rvalue,
  lvalue_let,
  lvalue_var
};

constexpr ValueCat mostRestrictive(const ValueCat a, const ValueCat b) {
  const int min = std::min(static_cast<int>(a), static_cast<int>(b));
  return static_cast<ValueCat>(min);
}

constexpr bool convertibleTo(const ValueCat from, const ValueCat to) {
  return static_cast<int>(from) >= static_cast<int>(to);
}

struct ExprType {
  Symbol *type;
  ValueCat cat;
};

struct Object final : Symbol {
  ExprType etype;
};

enum class MemAccess {
  public_,
  private_
};

enum class MemScope {
  member,
  static_
};

using FuncParams = std::vector<ExprType>;
struct Func final : Symbol {
  ExprType ret;
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
