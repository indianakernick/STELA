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

stela::retain_ptr<ast::BtnType> pushType(
  sym::Table &table,
  const ast::BtnType::Enum e,
  const ast::Name name
) {
  auto type = stela::make_retain<ast::BtnType>(e);
  auto alias = make_retain<ast::TypeAlias>();
  alias->name = name;
  alias->strong = false;
  alias->type = type;
  auto symbol = std::make_unique<sym::TypeAlias>();
  symbol->node = std::move(alias);
  table.insert({sym::Name{name}, std::move(symbol)});
  return type;
}

void insertTypes(sym::Table &table, sym::Builtins &t) {
  t.Bool = pushType(table, ast::BtnType::Bool, "bool");
  t.Byte = pushType(table, ast::BtnType::Byte, "byte");
  t.Char = pushType(table, ast::BtnType::Char, "char");
  t.Real = pushType(table, ast::BtnType::Real, "real");
  t.Sint = pushType(table, ast::BtnType::Sint, "sint");
  t.Uint = pushType(table, ast::BtnType::Uint, "uint");
  
  // A string literal has the type [char]
  auto charName = make_retain<ast::NamedType>();
  charName->name = "char";
  auto string = make_retain<ast::ArrayType>();
  string->elem = std::move(charName);
  t.string = std::move(string);
}

using TypeEnum = ast::BtnType::Enum;

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

bool stela::validIncr(const ast::BtnTypePtr &type) {
  return isArithType(type->value);
}

bool stela::validOp(const ast::UnOp op, const ast::BtnTypePtr &type) {
  switch (op) {
    case ast::UnOp::neg:
      return isArithType(type->value);
    case ast::UnOp::bool_not:
      return isBoolType(type->value);
    case ast::UnOp::bit_not:
      return isBitwiseType(type->value);
    /* LCOV_EXCL_START */
    default:
      assert(false);
      return false;
    /* LCOV_EXCL_END */
  }
}

ast::BtnTypePtr checkType(bool(*category)(TypeEnum), const ast::BtnTypePtr &type) {
  return category(type->value) ? type : nullptr;
}

ast::BtnTypePtr stela::validOp(
  const sym::Builtins &btn,
  const ast::BinOp op,
  const ast::BtnTypePtr &left,
  const ast::BtnTypePtr &right
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
    /* LCOV_EXCL_START */
    assert(false);
    return nullptr;
    /* LCOV_EXCL_END */
  }
}

bool stela::validOp(
  const ast::AssignOp op,
  const ast::BtnTypePtr &left,
  const ast::BtnTypePtr &right
) {
  if (left != right) {
    return false;
  }
  if (isArithOp(op)) {
    return isArithType(left->value);
  } else if (isBitwiseOp(op)) {
    return isBitwiseType(left->value);
  } else {
    /* LCOV_EXCL_START */
    assert(false);
    return false;
    /* LCOV_EXCL_END */
  }
}

bool stela::validSubscript(const ast::BtnTypePtr &index) {
  return index->value == TypeEnum::Sint || index->value == TypeEnum::Uint;
}

sym::ScopePtr stela::makeBuiltinModule(sym::Builtins &btn) {
  auto scope = std::make_unique<sym::Scope>(nullptr, sym::ScopeType::ns);
  insertTypes(scope->table, btn);
  return scope;
}
