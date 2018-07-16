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

sym::SymbolPtr makeBuiltinType(const sym::BuiltinType::Enum e) {
  auto type = std::make_unique<sym::BuiltinType>();
  type->loc = {};
  type->value = e;
  return type;
}

void insertTypes(sym::Table &table) {
  #define INSERT(TYPE)                                                          \
    table.insert({#TYPE, makeBuiltinType(sym::BuiltinType::TYPE)})
  
  INSERT(Void);
  INSERT(Int);
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
  
  #undef INSERT
}

#define INSERT_INT(OP) \
  INSERT(OP, Int); \
  INSERT(OP, Char); \
  INSERT(OP, Int8); \
  INSERT(OP, Int16); \
  INSERT(OP, Int32); \
  INSERT(OP, Int64); \
  INSERT(OP, UInt8); \
  INSERT(OP, UInt16); \
  INSERT(OP, UInt32); \
  INSERT(OP, UInt64)

#define INSERT_NUM(OP) \
  INSERT_INT(OP); \
  INSERT(OP, Float); \
  INSERT(OP, Double) \

#define INSERT_ALL(OP) \
  INSERT_NUM(OP); \
  INSERT(OP, Bool); \
  INSERT(OP, String);

sym::Symbol *lookupType(sym::Table &table, const std::string_view type) {
  const auto [begin, end] = table.equal_range(type);
  assert(std::next(begin) == end);
  assert(dynamic_cast<sym::BuiltinType *>(begin->second.get()));
  return begin->second.get();
}

sym::SymbolPtr makeBinOp(sym::Symbol *type, sym::Symbol *ret) {
  auto func = std::make_unique<sym::Func>();
  func->loc = {};
  func->ret = ret;
  func->params.push_back(type);
  func->params.push_back(type);
  return func;
}

sym::SymbolPtr makeAssignOp(sym::Symbol *type) {
  return makeBinOp(type, type);
}

void insertAssign(sym::Table &table) {
  #define INSERT(OP, TYPE) \
    table.insert({opName(ast::AssignOp::OP), makeAssignOp(lookupType(table, #TYPE))})
  
  INSERT_ALL(assign);
  INSERT_NUM(add);
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

void insertBin(sym::Table &table) {
  #define INSERT(OP, TYPE) \
    table.insert({opName(ast::BinOp::OP), makeAssignOp(lookupType(table, #TYPE))})
  
  INSERT(bool_or, Bool);
  INSERT(bool_and, Bool);
  INSERT_INT(bit_or);
  INSERT_INT(bit_xor);
  INSERT_INT(bit_and);
  INSERT_INT(bit_shl);
  INSERT_INT(bit_shr);
  INSERT_NUM(add);
  INSERT_NUM(sub);
  INSERT_NUM(mul);
  INSERT_NUM(div);
  INSERT_NUM(mod);
  INSERT_NUM(pow);
  
  sym::Symbol *const boolType = lookupType(table, "Bool");
  
  #undef INSERT
  #define INSERT(OP, TYPE) \
    table.insert({opName(ast::BinOp::OP), makeBinOp(lookupType(table, #TYPE), boolType)})
  
  INSERT_ALL(eq);
  INSERT_ALL(ne);
  INSERT_NUM(lt);
  INSERT_NUM(le);
  INSERT_NUM(gt);
  INSERT_NUM(ge);
  
  #undef INSERT
}

sym::SymbolPtr makeUnOp(sym::Symbol *type) {
  auto func = std::make_unique<sym::Func>();
  func->loc = {};
  func->ret = type;
  func->params.push_back(type);
  return func;
}

void insertUn(sym::Table &table) {
  #define INSERT(OP, TYPE) \
    table.insert({opName(ast::UnOp::OP), makeUnOp(lookupType(table, #TYPE))})
  
  INSERT_NUM(neg);
  INSERT(bool_not, Bool);
  INSERT_INT(bit_not);
  INSERT_NUM(pre_incr);
  INSERT_NUM(pre_decr);
  INSERT_NUM(post_incr);
  INSERT_NUM(post_decr);
  
  #undef INSERT
}

}

stela::sym::ScopePtr stela::createBuiltinScope() {
  sym::ScopePtr scope = std::make_unique<sym::Scope>();
  scope->parent = nullptr;
  insertTypes(scope->table);
  insertAssign(scope->table);
  insertBin(scope->table);
  insertUn(scope->table);
  return scope;
}
