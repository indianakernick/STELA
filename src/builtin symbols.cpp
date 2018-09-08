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

void insertTypes(sym::Table &table, ast::Decls &decls, BuiltinTypes &t) {
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

/*

THIS IS JUST FOR TYPE CHECKING

Operators are not function calls.
Operators are put on the symbol table to check types.
The operators defined in this file can only be applied to primitive types

*/

#define INSERT_INT(OP)                                                          \
  INSERT(OP, Char);                                                             \
  INSERT(OP, Int);                                                              \
  INSERT(OP, Int8);                                                             \
  INSERT(OP, Int16);                                                            \
  INSERT(OP, Int32);                                                            \
  INSERT(OP, Int64);                                                            \
  INSERT(OP, UInt8);                                                            \
  INSERT(OP, UInt16);                                                           \
  INSERT(OP, UInt32);                                                           \
  INSERT(OP, UInt64)

#define INSERT_NUM(OP)                                                          \
  INSERT_INT(OP);                                                               \
  INSERT(OP, Float);                                                            \
  INSERT(OP, Double)

#define INSERT_ALL(OP)                                                          \
  INSERT_NUM(OP);                                                               \
  INSERT(OP, Bool);                                                             \
  INSERT(OP, String);

sym::SymbolPtr makeBinOp(ast::Type *type, ast::Type *ret) {
  auto func = std::make_unique<sym::Func>();
  func->ret = {ret, sym::ValueMut::let, sym::ValueRef::val};
  func->params.push_back({type, sym::ValueMut::let, sym::ValueRef::val});
  func->params.push_back({type, sym::ValueMut::let, sym::ValueRef::val});
  return func;
}

sym::SymbolPtr makeAssignOp(ast::Type *type) {
  auto func = std::make_unique<sym::Func>();
  func->ret = {type, sym::ValueMut::var, sym::ValueRef::ref};
  func->params.push_back({type, sym::ValueMut::var, sym::ValueRef::ref});
  func->params.push_back({type, sym::ValueMut::let, sym::ValueRef::val});
  return func;
}

void insertAssign(sym::Table &table, const BuiltinTypes &t) {
  #define INSERT(OP, TYPE)                                                      \
    table.insert({sym::Name(opName(ast::AssignOp::OP)), makeAssignOp(t.TYPE)})
  
  INSERT_NUM(add);
  INSERT(add, String);
  INSERT_NUM(sub);
  INSERT_NUM(mul);
  INSERT_NUM(div);
  INSERT_NUM(mod);
  INSERT_NUM(pow);
  INSERT_INT(bit_or);
  INSERT_INT(bit_xor);
  INSERT_INT(bit_and);
  INSERT_INT(bit_shl);
  INSERT_INT(bit_shr);
  
  #undef INSERT
}

void insertBin(sym::Table &table, const BuiltinTypes &t) {
  #define INSERT(OP, TYPE)                                                      \
    table.insert({sym::Name(opName(ast::BinOp::OP)), makeBinOp(t.TYPE, t.TYPE)})
  
  INSERT(bool_or, Bool);
  INSERT(bool_and, Bool);
  INSERT_INT(bit_or);
  INSERT_INT(bit_xor);
  INSERT_INT(bit_and);
  INSERT_INT(bit_shl);
  INSERT_INT(bit_shr);
  INSERT_NUM(add);
  INSERT(add, String);
  INSERT_NUM(sub);
  INSERT_NUM(mul);
  INSERT_NUM(div);
  INSERT_NUM(mod);
  INSERT_NUM(pow);
  
  #undef INSERT
  #define INSERT(OP, TYPE)                                                      \
    table.insert({sym::Name(opName(ast::BinOp::OP)), makeBinOp(t.TYPE, t.Bool)})
  
  INSERT_ALL(eq);
  INSERT_ALL(ne);
  INSERT_NUM(lt);
  INSERT_NUM(le);
  INSERT_NUM(gt);
  INSERT_NUM(ge);
  
  #undef INSERT
}

sym::SymbolPtr makeUnOp(ast::Type *type) {
  auto func = std::make_unique<sym::Func>();
  func->ret = {type, sym::ValueMut::let, sym::ValueRef::val};
  func->params.push_back({type, sym::ValueMut::let, sym::ValueRef::val});
  return func;
}

void insertUn(sym::Table &table, const BuiltinTypes &t) {
  #define INSERT(OP, TYPE)                                                      \
    table.insert({sym::Name(opName(ast::UnOp::OP)), makeUnOp(t.TYPE)})
  
  INSERT_NUM(neg);
  INSERT(bool_not, Bool);
  INSERT_INT(bit_not);
  
  #undef INSERT
}

sym::SymbolPtr makeIncrDecr(ast::Type *type) {
  auto func = std::make_unique<sym::Func>();
  func->ret = {type, sym::ValueMut::let, sym::ValueRef::val};
  func->params.push_back({type, sym::ValueMut::var, sym::ValueRef::ref});
  return func;
}

void insertIncrDecr(sym::Table &table, const BuiltinTypes &t) {
  #define INSERT(OP, TYPE)                                                      \
    table.insert({sym::Name(#OP), makeIncrDecr(t.TYPE)})
  
  INSERT_NUM(a--);
  INSERT_NUM(a++);
  
  #undef INSERT
}

}

BuiltinTypes stela::pushBuiltins(sym::Scopes &scopes, ast::Decls &decls) {
  auto scope = std::make_unique<sym::Scope>(nullptr, sym::Scope::Type::ns);
  BuiltinTypes types;
  insertTypes(scope->table, decls, types);
  insertAssign(scope->table, types);
  insertBin(scope->table, types);
  insertUn(scope->table, types);
  insertIncrDecr(scope->table, types);
  scopes.push_back(std::move(scope));
  return types;
}
