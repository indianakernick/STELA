//
//  operator name.cpp
//  STELA
//
//  Created by Indi Kernick on 16/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "operator name.hpp"

std::string_view stela::opName(const ast::AssignOp op) {
  switch (op) {
    case ast::AssignOp::assign:
      return "=";
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
    case ast::AssignOp::pow:
      return "**=";
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
  }
}

std::string_view stela::opName(const ast::BinOp op) {
  switch (op) {
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
    case ast::BinOp::pow:
      return "**";
  }
}

std::string_view stela::opName(const ast::UnOp op) {
  switch (op) {
    case ast::UnOp::neg:
      return "-a";
    case ast::UnOp::bool_not:
      return "!a";
    case ast::UnOp::bit_not:
      return "~a";
    case ast::UnOp::pre_incr:
      return "++a";
    case ast::UnOp::pre_decr:
      return "--a";
    case ast::UnOp::post_incr:
      return "a++";
    case ast::UnOp::post_decr:
      return "a--";
  }
}
