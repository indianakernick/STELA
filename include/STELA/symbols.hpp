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

struct Symbol {
  virtual ~Symbol() = default;

  Loc loc;
  // for detecting things like: unused variable, unused function, etc
  bool referenced = false;
};
using SymbolPtr = std::unique_ptr<Symbol>;

class ScopeVisitor;

struct Scope {
  explicit Scope(Scope *const parent)
    : parent{parent} {}
  virtual ~Scope() = default;

  virtual void accept(ScopeVisitor &) = 0;

  Scope *const parent;
};
using ScopePtr = std::unique_ptr<Scope>;
using Scopes = std::vector<ScopePtr>;

using Name = std::string;
using UnorderedTable = std::unordered_multimap<Name, SymbolPtr>;

struct UnorderedScope : Scope {
  explicit UnorderedScope(Scope *const parent)
    : Scope{parent} {}

  UnorderedTable table;
};

struct NSScope final : UnorderedScope {
  explicit NSScope(Scope *const parent)
    : UnorderedScope{parent} {}

  void accept(ScopeVisitor &) override;
};

struct BlockScope final : UnorderedScope {
  explicit BlockScope(Scope *const parent)
    : UnorderedScope{parent} {}
  
  void accept(ScopeVisitor &) override;
};

struct FuncScope final : UnorderedScope {
  explicit FuncScope(Scope *const parent)
    : UnorderedScope{parent} {}
  
  void accept(ScopeVisitor &) override;
};

enum class MemAccess {
  public_,
  private_
};

enum class MemScope {
  instance,
  static_
};

struct StructTableRow {
  Name name;
  MemAccess access;
  MemScope scope;
  SymbolPtr val;
};
using StructTable = std::vector<StructTableRow>;

struct StructScope final : Scope {
  explicit StructScope(Scope *const parent)
    : Scope{parent} {}
  
  void accept(ScopeVisitor &) override;
  
  StructTable table;
};

struct EnumTableRow {
  Name key;
  SymbolPtr val;
};
using EnumTable = std::vector<EnumTableRow>;

struct EnumScope final : Scope {
  explicit EnumScope(Scope *const parent)
    : Scope{parent} {}

  void accept(ScopeVisitor &) override;

  EnumTable table;
};

class ScopeVisitor {
public:
  virtual ~ScopeVisitor() = default;
  
  virtual void visit(NSScope &) = 0;
  virtual void visit(BlockScope &) = 0;
  virtual void visit(FuncScope &) = 0;
  virtual void visit(StructScope &) = 0;
  virtual void visit(EnumScope &) = 0;
};

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
  StructScope *scope;
};

struct EnumType final : Symbol {
  EnumScope *scope;
};

struct TypeAlias final : Symbol {
  Symbol *type;
};

enum class ValueMut {
  let,
  var
};

enum class ValueRef {
  val,
  ref
};

struct ExprType {
  Symbol *type = nullptr;
  ValueMut mut;
  ValueRef ref;
};

constexpr ValueMut common(const ValueMut a, const ValueMut b) {
  return a == ValueMut::let ? a : b;
}

constexpr ValueRef common(const ValueRef a, const ValueRef b) {
  return a == ValueRef::val ? a : b;
}

constexpr ExprType memberType(const ExprType obj, const ExprType mem) {
  return {mem.type, common(obj.mut, mem.mut), obj.ref};
}

constexpr bool callMut(const ValueMut param, const ValueMut arg) {
  return static_cast<int>(param) <= static_cast<int>(arg);
}

constexpr bool callMutRef(const ExprType param, const ExprType arg) {
  return param.ref == sym::ValueRef::val || callMut(param.mut, arg.mut);
}

constexpr ExprType null_type {};

struct Object final : Symbol {
  ExprType etype;
};

using FuncParams = std::vector<ExprType>;
struct Func final : Symbol {
  ExprType ret;
  FuncParams params;
  FuncScope *scope;
};
using FuncPtr = std::unique_ptr<Func>;

struct FunKey {
  Name name;
  FuncParams params;
};

struct MemKey {
  Name name;
  MemAccess access;
  MemScope scope;
};

struct MemFunKey {
  Name name;
  FuncParams params;
  MemAccess access;
  MemScope scope;
};

}

namespace stela {

struct Symbols {
  sym::Scopes scopes;
};

}

#endif
