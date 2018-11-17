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

stela::retain_ptr<ast::BuiltinType> pushType(
  sym::Table &table,
  const ast::BuiltinType::Enum e,
  const ast::Name name
) {
  auto type = stela::make_retain<ast::BuiltinType>();
  type->value = e;
  auto alias = make_retain<ast::TypeAlias>();
  alias->name = name;
  alias->strong = false;
  alias->type = type;
  auto symbol = std::make_unique<sym::TypeAlias>();
  symbol->node = std::move(alias);
  table.insert({sym::Name(name), std::move(symbol)});
  return type;
}

void insertTypes(sym::Table &table, sym::Builtins &t) {
  #define INSERT(TYPE, NAME)                                                    \
    t.TYPE = pushType(table, ast::BuiltinType::TYPE, NAME);
  
  INSERT(Bool, "bool");
  INSERT(Byte, "byte");
  INSERT(Char, "char");
  INSERT(Real, "real");
  INSERT(Sint, "sint");
  INSERT(Uint, "uint");

  #undef INSERT
  
  // A string literal has the type [char]
  auto charName = make_retain<ast::NamedType>();
  charName->name = "char";
  auto string = make_retain<ast::ArrayType>();
  string->elem = std::move(charName);
  t.string = std::move(string);
}

using TypeEnum = ast::BuiltinType::Enum;

bool isBoolType(const TypeEnum type) {
  return type == TypeEnum::Bool;
}

bool isBitwiseType(const TypeEnum type) {
  return type == TypeEnum::Byte || type == TypeEnum::Uint;
}

bool isArithType(const TypeEnum type) {
  return TypeEnum::Char <= type && type <= TypeEnum::Uint;
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

bool stela::validIncr(const ast::BuiltinTypePtr &type) {
  return isArithType(type->value);
}

bool stela::validOp(const ast::UnOp op, const ast::BuiltinTypePtr &type) {
  switch (op) {
    case ast::UnOp::neg:
      return isArithType(type->value);
    case ast::UnOp::bool_not:
      return isBoolType(type->value);
    case ast::UnOp::bit_not:
      return isBitwiseType(type->value);
  }
}

ast::BuiltinTypePtr checkType(bool(*category)(TypeEnum), const ast::BuiltinTypePtr &type) {
  return category(type->value) ? type : nullptr;
}

ast::BuiltinTypePtr stela::validOp(
  const sym::Builtins &btn,
  const ast::BinOp op,
  const ast::BuiltinTypePtr &left,
  const ast::BuiltinTypePtr &right
) {
  if (left != right) {
    return nullptr;
  }
  if (isBoolOp(op)) {
    return checkType(isBoolType, left);
  } else if (isBitwiseOp(op)) {
    return checkType(isBitwiseType, left);
  } else if (isEqualOp(op)) {
    return btn.Bool;
  } else if (isOrderOp(op)) {
    return isArithType(left->value) ? btn.Bool : nullptr;
  } else if (isArithOp(op)) {
    return checkType(isArithType, left);
  } else {
    assert(false);
    return nullptr;
  }
}

bool stela::validOp(
  const ast::AssignOp op,
  const ast::BuiltinTypePtr &left,
  const ast::BuiltinTypePtr &right
) {
  if (left != right) {
    return false;
  }
  if (isArithOp(op)) {
    return isArithType(left->value);
  } else if (isBitwiseOp(op)) {
    return isBitwiseType(left->value);
  } else {
    assert(false);
    return false;
  }
}

sym::Module stela::makeBuiltinModule(sym::Builtins &btn) {
  auto scope = std::make_unique<sym::Scope>(nullptr, sym::Scope::Type::ns);
  sym::Module module;
  insertTypes(scope->table, btn);
  btn.scope = scope.get();
  module.scopes.push_back(std::move(scope));
  return module;
}
