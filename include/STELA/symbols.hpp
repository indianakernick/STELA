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

namespace stela::ast {

struct Type;
struct TypeAlias;
struct Statement;
struct Func;

}

namespace stela::sym {

struct Symbol {
  virtual ~Symbol() = default;

  Loc loc;
  // for detecting things like: unused variable, unused function, etc
  bool referenced = false;
};
using SymbolPtr = std::unique_ptr<Symbol>;

using Name = std::string;
using Table = std::unordered_multimap<Name, SymbolPtr>;

struct Scope {
  enum class Type {
    // global namespace scope
    ns,
    block,
    func,
    // break and continue are valid within flow scopes (while, for, switch)
    flow
  };

  Scope(Scope *const parent, const Type type)
    : parent{parent}, type{type} {}

  Scope *const parent;
  const Type type;
  Table table;
};
using ScopePtr = std::unique_ptr<Scope>;
using Scopes = std::vector<ScopePtr>;

struct TypeAlias final : Symbol {
  ast::TypeAlias *node;
};

enum class ValueMut : uint8_t {
  let,
  var
};

enum class ValueRef : uint8_t  {
  val,
  ref
};

struct ExprType {
  ast::Type *type = nullptr;
  ValueMut mut;
  ValueRef ref;
};

constexpr ValueMut common(const ValueMut a, const ValueMut b) {
  return a == ValueMut::let ? a : b;
}

constexpr ValueRef common(const ValueRef a, const ValueRef b) {
  return a == ValueRef::val ? a : b;
}

constexpr ExprType memberType(const ExprType obj, ast::Type *mem) {
  return {mem, obj.mut, obj.ref};
}

constexpr bool callMut(const ValueMut param, const ValueMut arg) {
  return static_cast<uint8_t>(param) <= static_cast<uint8_t>(arg);
}

constexpr bool callMutRef(const ExprType param, const ExprType arg) {
  return param.ref == sym::ValueRef::val || callMut(param.mut, arg.mut);
}

constexpr ExprType null_type {};

struct Object final : Symbol {
  ExprType etype;
  ast::Statement *node;
};

// the first parameter is the receiver
// if params.front().type == nullptr then there is no receiver
using FuncParams = std::vector<ExprType>;
struct Func final : Symbol {
  ExprType ret;
  FuncParams params;
  Scope *scope;
  ast::Func *node;
};
using FuncPtr = std::unique_ptr<Func>;

struct FunKey {
  Name name;
  FuncParams params;
};

}

namespace stela {

struct Symbols {
  sym::Scopes scopes;
};

}

#endif
