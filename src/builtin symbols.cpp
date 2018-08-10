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
  type->value = e;
  return type;
}

void insertTypes(sym::UnorderedTable &table, BuiltinTypes &t) {
  #define INSERT(TYPE)                                                          \
    t.TYPE = table.insert({#TYPE, makeBuiltinType(sym::BuiltinType::TYPE)})->second.get()
  
  INSERT(Void);
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
  
  auto intType = std::make_unique<sym::TypeAlias>();
  intType->type = t.Int64;
  table.insert({"Int", std::move(intType)});
  
  #undef INSERT
}

#define INSERT_INT(OP)                                                          \
  INSERT(OP, Char);                                                             \
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

sym::SymbolPtr makeBinOp(sym::Symbol *type, sym::Symbol *ret) {
  auto func = std::make_unique<sym::Func>();
  func->ret = {ret, sym::ValueMut::let, sym::ValueRef::val};
  func->params.push_back({type, sym::ValueMut::let, sym::ValueRef::val});
  func->params.push_back({type, sym::ValueMut::let, sym::ValueRef::val});
  return func;
}

sym::SymbolPtr makeAssignOp(sym::Symbol *type) {
  auto func = std::make_unique<sym::Func>();
  func->ret = {type, sym::ValueMut::var, sym::ValueRef::ref};
  func->params.push_back({type, sym::ValueMut::var, sym::ValueRef::ref});
  func->params.push_back({type, sym::ValueMut::let, sym::ValueRef::val});
  return func;
}

void insertAssign(sym::UnorderedTable &table, const BuiltinTypes &t) {
  #define INSERT(OP, TYPE)                                                      \
    table.insert({sym::Name(opName(ast::AssignOp::OP)), makeAssignOp(t.TYPE)})
  
  INSERT_ALL(assign);
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

void insertBin(sym::UnorderedTable &table, const BuiltinTypes &t) {
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

sym::SymbolPtr makeUnOp(sym::Symbol *type) {
  auto func = std::make_unique<sym::Func>();
  func->ret = {type, sym::ValueMut::let, sym::ValueRef::val};
  func->params.push_back({type, sym::ValueMut::let, sym::ValueRef::val});
  return func;
}

sym::SymbolPtr makePreOp(sym::Symbol *type) {
  auto func = std::make_unique<sym::Func>();
  func->ret = {type, sym::ValueMut::var, sym::ValueRef::ref};
  func->params.push_back({type, sym::ValueMut::var, sym::ValueRef::ref});
  return func;
}

sym::SymbolPtr makePostOp(sym::Symbol *type) {
  auto func = std::make_unique<sym::Func>();
  func->ret = {type, sym::ValueMut::let, sym::ValueRef::val};
  func->params.push_back({type, sym::ValueMut::var, sym::ValueRef::ref});
  return func;
}

void insertUn(sym::UnorderedTable &table, const BuiltinTypes &t) {
  #define INSERT(OP, TYPE)                                                      \
    table.insert({sym::Name(opName(ast::UnOp::OP)), MAKE(t.TYPE)})
  #define MAKE makeUnOp
  
  INSERT_NUM(neg);
  INSERT(bool_not, Bool);
  INSERT_INT(bit_not);
  
  #undef MAKE
  #define MAKE makePreOp
  
  INSERT_NUM(pre_incr);
  INSERT_NUM(pre_decr);
  
  #undef MAKE
  #define MAKE makePostOp
  
  INSERT_NUM(post_incr);
  INSERT_NUM(post_decr);
  
  #undef MAKE
  #undef INSERT
}

}

stela::sym::ScopePtr stela::createBuiltinScope(BuiltinTypes &types) {
  auto scope = std::make_unique<sym::NSScope>(nullptr);
  insertTypes(scope->table, types);
  insertAssign(scope->table, types);
  insertBin(scope->table, types);
  insertUn(scope->table, types);
  return scope;
}
