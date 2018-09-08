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

}

bool Builtins::isInteger(ast::BuiltinType *type) const {
  return type == Char ||
         type == Int8 ||
         type == Int16 ||
         type == Int32 ||
         type == Int64 ||
         type == UInt8 ||
         type == UInt16 ||
         type == UInt32 ||
         type == UInt64;
}

bool Builtins::isNumber(ast::BuiltinType *type) const {
  return isInteger(type) ||
         type == Float ||
         type == Double;
}

bool Builtins::validIncr(bool incr [[maybe_unused]], ast::BuiltinType *type) const {
  return isNumber(type);
}

bool Builtins::validOp(const ast::UnOp op, ast::BuiltinType *type) const {
  switch (op) {
    case ast::UnOp::neg:
      return isNumber(type);
    case ast::UnOp::bool_not:
      return type == Bool;
    case ast::UnOp::bit_not:
      return isInteger(type);
  }
}

bool Builtins::validOp(const ast::BinOp op, ast::BuiltinType *left, ast::BuiltinType *right) const {
  if (left != right) {
    return false;
  }
  #define VALID(OP, COND)                                                       \
    case ast::BinOp::OP: return COND
  switch (op) {
    VALID(bool_or, left == Bool);
    VALID(bool_and, left == Bool);
    VALID(bit_or, isInteger(left));
    VALID(bit_xor, isInteger(left));
    VALID(bit_and, isInteger(left));
    VALID(bit_shl, isInteger(left));
    VALID(bit_shr, isInteger(left));
    VALID(add, isNumber(left) || left == String);
    VALID(sub, isNumber(left));
    VALID(mul, isNumber(left));
    VALID(div, isNumber(left));
    VALID(mod, isNumber(left));
    VALID(pow, isNumber(left));
    VALID(eq, true);
    VALID(ne, true);
    VALID(lt, isNumber(left));
    VALID(le, isNumber(left));
    VALID(gt, isNumber(left));
    VALID(ge, isNumber(left));
  }
  #undef VALID
}

bool Builtins::validOp(const ast::AssignOp op, ast::BuiltinType *left, ast::BuiltinType *right) const {
  if (left != right) {
    return false;
  }
  #define VALID(OP, COND)                                                       \
    case ast::AssignOp::OP: return COND
  switch (op) {
    VALID(add, isNumber(left) || left == String);
    VALID(sub, isNumber(left));
    VALID(mul, isNumber(left));
    VALID(div, isNumber(left));
    VALID(mod, isNumber(left));
    VALID(pow, isNumber(left));
    VALID(bit_or, isInteger(left));
    VALID(bit_and, isInteger(left));
    VALID(bit_xor, isInteger(left));
    VALID(bit_shl, isInteger(left));
    VALID(bit_shr, isInteger(left));
  }
}

Builtins stela::makeBuiltins(sym::Scopes &scopes, ast::Decls &decls) {
  auto scope = std::make_unique<sym::Scope>(nullptr, sym::Scope::Type::ns);
  Builtins types;
  insertTypes(scope->table, decls, types);
  scopes.push_back(std::move(scope));
  return types;
}
