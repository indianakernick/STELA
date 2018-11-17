//
//  symbols.hpp
//  STELA
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_symbols_hpp
#define stela_symbols_hpp

#include <string>
#include "ast.hpp"
#include <unordered_map>

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
  retain_ptr<ast::TypeAlias> node;
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
  ast::TypePtr type;
  ValueMut mut;
  ValueRef ref;
};

inline ValueMut common(const ValueMut a, const ValueMut b) {
  return a == ValueMut::let ? a : b;
}

inline ValueRef common(const ValueRef a, const ValueRef b) {
  return a == ValueRef::val ? a : b;
}

inline ExprType memberType(const ExprType &obj, const ast::TypePtr &mem) {
  return {mem, obj.mut, obj.ref};
}

inline bool callMut(const ValueMut param, const ValueMut arg) {
  return static_cast<uint8_t>(param) <= static_cast<uint8_t>(arg);
}

inline bool callMutRef(const ExprType &param, const ExprType &arg) {
  return param.ref == sym::ValueRef::val || callMut(param.mut, arg.mut);
}

inline const ExprType null_type {};

struct Object final : Symbol {
  ExprType etype;
  ast::StatPtr node;
};

// the first parameter is the receiver
// if params.front().type == nullptr then there is no receiver
using FuncParams = std::vector<ExprType>;
struct Func final : Symbol {
  ExprType ret;
  FuncParams params;
  Scope *scope;
  retain_ptr<ast::Func> node;
};
using FuncPtr = std::unique_ptr<Func>;

struct Module {
  Scopes scopes;
  ast::Decls decls;
};

using Modules = std::unordered_map<Name, Module>;

struct Builtins {
  // Builtin types
  ast::BuiltinTypePtr Bool;
  ast::BuiltinTypePtr Byte;
  ast::BuiltinTypePtr Char;
  ast::BuiltinTypePtr Real;
  ast::BuiltinTypePtr Sint;
  ast::BuiltinTypePtr Uint;
  
  // Alias of [char] used as the type of string literals
  ast::TypePtr string;
  
  // Global scope of the builtin module
  sym::Scope *scope;
};

}

namespace stela {

struct Symbols {
  sym::Modules modules;
  sym::Builtins builtins;
};

}

#endif
