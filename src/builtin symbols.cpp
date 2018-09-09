//
//  builtin symbols.cpp
//  STELA
//
//  Created by Indi Kernick on 16/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "builtin symbols.hpp"

#include "operator name.hpp"

using namespace stela;

namespace {

std::unique_ptr<ast::BuiltinType> makeType(const ast::BuiltinType::Enum e) {
  auto type = std::make_unique<ast::BuiltinType>();
  type->value = e;
  return type;
}

std::unique_ptr<sym::TypeAlias> makeSymbol(ast::TypeAlias *node) {
  auto symbol = std::make_unique<sym::TypeAlias>();
  symbol->node = node;
  return symbol;
}

ast::BuiltinType *pushType(
  sym::Table &table,
  ast::Decls &decls,
  const ast::BuiltinType::Enum e,
  const ast::Name name
) {
  auto alias = std::make_unique<ast::TypeAlias>();
  alias->name = name;
  alias->strong = false;
  std::unique_ptr<ast::BuiltinType> type = makeType(e);
  ast::BuiltinType *ptr = type.get();
  alias->type = std::move(type);
  table.insert({sym::Name(name), makeSymbol(alias.get())});
  decls.push_back(std::move(alias));
  return ptr;
}

void insertTypes(sym::Table &table, ast::Decls &decls, Builtins &t) {
  #define INSERT(TYPE)                                                          \
    t.TYPE = pushType(table, decls, ast::BuiltinType::TYPE, #TYPE);
  
  INSERT(Char);
  INSERT(Bool);
  INSERT(Float);
  INSERT(Double);
  INSERT(String);
  INSERT(Int8);
  INSERT(Int16);
  INSERT(Int32);
  INSERT(Int64);
  INSERT(UInt8);
  INSERT(UInt16);
  INSERT(UInt32);
  INSERT(UInt64);
  
  auto alias = std::make_unique<ast::TypeAlias>();
  alias->name = "Int";
  alias->strong = false;
  auto intName = std::make_unique<ast::NamedType>();
  intName->name = "Int64";
  alias->type = std::move(intName);
  table.insert({sym::Name("Int"), makeSymbol(alias.get())});
  decls.push_back(std::move(alias));
  
  #undef INSERT
}

using TypeEnum = ast::BuiltinType::Enum;

bool isBoolType(const TypeEnum type) {
  return type == TypeEnum::Bool;
}

bool isBitwiseType(const TypeEnum type) {
  return TypeEnum::Int8 <= type && type <= TypeEnum::UInt64;
}

bool isArithType(const TypeEnum type) {
  return TypeEnum::Char <= type && type <= TypeEnum::UInt64;
}

template <typename Enum>
auto operator+(const Enum e) -> std::underlying_type_t<Enum> {
  return static_cast<std::underlying_type_t<Enum>>(e);
}

template <typename Enum>
bool incRange(const Enum e, const Enum min, const Enum max) {
  assert(min <= max);
  return +min <= e && e <= +max;
}

bool isBoolOp(const ast::BinOp op) {
  return op == ast::BinOp::bool_or || op == ast::BinOp::bool_and;
}

bool isBitwiseOp(const ast::BinOp op) {
  return +ast::BinOp::bit_or <= +op && +op <= +ast::BinOp::bit_shr;
}

bool isEqualOp(const ast::BinOp op) {
  return op == ast::BinOp::eq || op == ast::BinOp::ne;
}

bool isOrderOp(const ast::BinOp op) {
  return +ast::BinOp::lt <= +op && +op <= +ast::BinOp::ge;
}

bool isArithOp(const ast::BinOp op) {
  return +ast::BinOp::add <= +op && +op <= +ast::BinOp::pow;
}

bool isBitwiseOp(const ast::AssignOp op) {
  return +ast::AssignOp::bit_or <= +op && +op <= +ast::AssignOp::bit_shr;
}

bool isArithOp(const ast::AssignOp op) {
  return +ast::AssignOp::add <= +op && +op <= +ast::AssignOp::pow;
}

}

bool stela::validIncr(ast::BuiltinType *type) {
  return isArithType(type->value);
}

bool stela::validOp(const ast::UnOp op, ast::BuiltinType *type) {
  switch (op) {
    case ast::UnOp::neg:
      return isArithType(type->value);
    case ast::UnOp::bool_not:
      return isBoolType(type->value);
    case ast::UnOp::bit_not:
      return isBitwiseType(type->value);
  }
}

ast::BuiltinType *checkType(bool(*category)(TypeEnum), ast::BuiltinType *type) {
  return category(type->value) ? type : nullptr;
}

ast::BuiltinType *stela::validOp(
  const Builtins &bnt,
  const ast::BinOp op,
  ast::BuiltinType *left,
  ast::BuiltinType *right
) {
  if (left != right) {
    return nullptr;
  }
  if (isBoolOp(op)) {
    return checkType(isBoolType, left);
  } else if (isBitwiseOp(op)) {
    return checkType(isBitwiseType, left);
  } else if (isEqualOp(op)) {
    return bnt.Bool;
  } else if (isOrderOp(op)) {
    return isArithType(left->value) ? bnt.Bool : nullptr;
  } else if (isArithOp(op)) {
    // @TODO maybe remove this
    if (op == ast::BinOp::add && left->value == TypeEnum::String) {
      return left;
    }
    return checkType(isArithType, left);
  } else {
    assert(false);
    return nullptr;
  }
}

bool stela::validOp(const ast::AssignOp op, ast::BuiltinType *left, ast::BuiltinType *right) {
  if (left != right) {
    return false;
  }
  if (isArithOp(op)) {
    // @TODO maybe remove this
    if (op == ast::AssignOp::add && left->value == TypeEnum::String) {
      return true;
    }
    return isArithType(left->value);
  } else if (isBitwiseOp(op)) {
    return isBitwiseType(left->value);
  } else {
    assert(false);
    return false;
  }
}

Builtins stela::makeBuiltins(sym::Scopes &scopes, ast::Decls &decls) {
  auto scope = std::make_unique<sym::Scope>(nullptr, sym::Scope::Type::ns);
  Builtins types;
  insertTypes(scope->table, decls, types);
  scopes.push_back(std::move(scope));
  return types;
}
