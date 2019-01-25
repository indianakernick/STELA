//
//  operator name.cpp
//  STELA
//
//  Created by Indi Kernick on 16/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "operator name.hpp"

#include "unreachable.hpp"

std::string_view stela::opName(const ast::AssignOp op) {
  switch (op) {
    /* LCOV_EXCL_START */
    case ast::AssignOp::add:
      return "+=";
    case ast::AssignOp::sub:
      return "-=";
    case ast::AssignOp::mul:
      return "*=";
    case ast::AssignOp::div:
      return "/=";
    case ast::AssignOp::mod:
      return "%=";
    case ast::AssignOp::bit_or:
      return "|=";
    case ast::AssignOp::bit_xor:
      return "^=";
    case ast::AssignOp::bit_and:
      return "&=";
    case ast::AssignOp::bit_shl:
      return "<<=";
    case ast::AssignOp::bit_shr:
      return ">>=";
    /* LCOV_EXCL_END */
  }
  UNREACHABLE();
}

std::string_view stela::opName(const ast::BinOp op) {
  switch (op) {
    /* LCOV_EXCL_START */
    case ast::BinOp::bool_or:
      return "||";
    case ast::BinOp::bool_and:
      return "&&";
    case ast::BinOp::bit_or:
      return "|";
    case ast::BinOp::bit_xor:
      return "^";
    case ast::BinOp::bit_and:
      return "&";
    case ast::BinOp::eq:
      return "==";
    case ast::BinOp::ne:
      return "!=";
    case ast::BinOp::lt:
      return "<";
    case ast::BinOp::le:
      return "<=";
    case ast::BinOp::gt:
      return ">";
    case ast::BinOp::ge:
      return ">=";
    case ast::BinOp::bit_shl:
      return "<<";
    case ast::BinOp::bit_shr:
      return ">>";
    case ast::BinOp::add:
      return "+";
    case ast::BinOp::sub:
      return "-";
    case ast::BinOp::mul:
      return "*";
    case ast::BinOp::div:
      return "/";
    case ast::BinOp::mod:
      return "%";
    /* LCOV_EXCL_END */
  }
  UNREACHABLE();
}

std::string_view stela::opName(const ast::UnOp op) {
  switch (op) {
    /* LCOV_EXCL_START */
    case ast::UnOp::neg:
      return "-";
    case ast::UnOp::bool_not:
      return "!";
    case ast::UnOp::bit_not:
      return "~";
    /* LCOV_EXCL_END */
  }
  UNREACHABLE();
}

std::string_view stela::typeName(const ast::BtnTypeEnum type) {
  switch (type) {
    /* LCOV_EXCL_START */
    case ast::BtnTypeEnum::Void:
      return "void";
    case ast::BtnTypeEnum::Opaq:
      return "opaq";
    case ast::BtnTypeEnum::Bool:
      return "bool";
    case ast::BtnTypeEnum::Byte:
      return "byte";
    case ast::BtnTypeEnum::Char:
      return "char";
    case ast::BtnTypeEnum::Real:
      return "real";
    case ast::BtnTypeEnum::Sint:
      return "sint";
    case ast::BtnTypeEnum::Uint:
      return "uint";
    /* LCOV_EXCL_END */
  }
  UNREACHABLE();
}
