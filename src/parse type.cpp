//
//  parse type.cpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "parse type.hpp"

using namespace stela;

namespace {

ast::TypePtr parseFuncType(ParseTokens &tok) {
  if (!tok.checkOp("(")) {
    return nullptr;
  }
  Context ctx = tok.context("in function type");
  auto type = std::make_unique<ast::FuncType>();
  if (!tok.checkOp(")")) {
    do {
      ast::ParamType &param = type->params.emplace_back();
      param.ref = parseRef(tok);
      param.type = parseType(tok);
    } while (tok.expectEitherOp(")", ",") == ",");
  }
  ctx.desc("after function parameter types");
  tok.expectOp("->");
  type->ret = parseType(tok);
  return type;
}

ast::TypePtr parseArrayType(ParseTokens &tok) {
  if (!tok.checkOp("[")) {
    return nullptr;
  }
  Context ctx = tok.context("in array type");
  auto arrayType = std::make_unique<ast::ArrayType>();
  arrayType->elem = tok.expectNode(parseType, "element type");
  tok.expectOp("]");
  return arrayType;
}

ast::TypePtr parseMapType(ParseTokens &tok) {
  if (!tok.checkOp("[{")) {
    return nullptr;
  }
  Context ctx = tok.context("in map type");
  auto mapType = std::make_unique<ast::MapType>();
  mapType->key = tok.expectNode(parseType, "key type");
  tok.expectOp(":");
  mapType->val = tok.expectNode(parseType, "value type");
  tok.expectOp("}]");
  return mapType;
}

ast::TypePtr parseBuiltinType(ParseTokens &tok) {
  #define CHECK(NAME)                                                           \
    if (tok.checkKeyword(#NAME)) {                                              \
      auto type = std::make_unique<ast::BuiltinType>();                         \
      type->value = ast::BuiltinType::NAME;                                     \
      return type;                                                              \
    }

  CHECK(Void)
  CHECK(Int)
  CHECK(Char)
  CHECK(Bool)
  CHECK(Float)
  CHECK(Double)
  CHECK(String)

  /*
  CHECK(Int8)
  CHECK(Int16)
  CHECK(Int32)
  CHECK(Int64)
  CHECK(UInt8)
  CHECK(UInt16)
  CHECK(UInt32)
  CHECK(UInt64)
  */

  return nullptr;
  
  #undef CHECK
}

ast::TypePtr parseNamedType(ParseTokens &tok) {
  if (!tok.peekIdentType()) {
    return nullptr;
  }
  auto namedType = std::make_unique<ast::NamedType>();
  namedType->name = tok.expectID();
  return namedType;
}

}

ast::ParamRef stela::parseRef(ParseTokens &tok) {
  if (tok.checkKeyword("inout")) {
    return ast::ParamRef::inout;
  } else {
    return ast::ParamRef::value;
  }
}

ast::TypePtr stela::parseType(ParseTokens &tok) {
  if (ast::TypePtr type = parseFuncType(tok)) return type;
  if (ast::TypePtr type = parseArrayType(tok)) return type;
  if (ast::TypePtr type = parseMapType(tok)) return type;
  if (ast::TypePtr type = parseBuiltinType(tok)) return type;
  if (ast::TypePtr type = parseNamedType(tok)) return type;
  return nullptr;
}
