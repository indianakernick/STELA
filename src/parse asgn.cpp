//
//  parse asgn.cpp
//  STELA
//
//  Created by Indi Kernick on 5/9/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "parse asgn.hpp"

#include "parse expr.hpp"

using namespace stela;

namespace {

ast::AsgnPtr makeAssign(ParseTokens &tok, const ast::AssignOp op, ast::ExprPtr left) {
  auto assign = std::make_unique<ast::CompAssign>();
  assign->loc = tok.lastLoc();
  assign->left = std::move(left);
  assign->oper = op;
  assign->right = tok.expectNode(parseExpr, "expression");
  return assign;
}

}

ast::AsgnPtr stela::parseAsgn(ParseTokens &tok) {
  ast::ExprPtr left = parseNested(tok);
  if (!left) {
    return nullptr;
  }
  if (auto *ident = dynamic_cast<ast::Identifier *>(left.get())) {
    if (tok.checkOp(":=")) {
      auto decl = std::make_unique<ast::DeclAssign>();
      decl->loc = tok.lastLoc();
      decl->name = ident->name;
      decl->expr = tok.expectNode(parseExpr, "expression");
      return decl;
    }
  }
  
  if (const bool incr = tok.checkOp("++"); incr || tok.checkOp("--")) {
    auto incrDecr = std::make_unique<ast::IncrDecr>();
    incrDecr->loc = tok.lastLoc();
    incrDecr->incr = incr;
    incrDecr->expr = std::move(left);
    return incrDecr;
  }
  
  if (tok.checkOp("=")) {
    auto assign = std::make_unique<ast::Assign>();
    assign->loc = tok.lastLoc();
    assign->left = std::move(left);
    assign->right = tok.expectNode(parseExpr, "expression");
    return assign;
  }
  
  #define CHECK(TOKEN, ENUM)                                                    \
    if (tok.checkOp(#TOKEN)) {                                                  \
      return makeAssign(tok, ast::AssignOp::ENUM, std::move(left));             \
    }
  
  CHECK(+=, add)
  CHECK(-=, sub)
  CHECK(*=, mul)
  CHECK(/=, div)
  CHECK(%=, mod)
  CHECK(**=, pow)
  CHECK(<<=, bit_shl)
  CHECK(>>=, bit_shr)
  CHECK(&=, bit_and)
  CHECK(^=, bit_xor)
  CHECK(|=, bit_or)
  
  #undef CHECK
  
  if (auto *call = dynamic_cast<ast::FuncCall *>(left.get())) {
    auto assign = std::make_unique<ast::CallAssign>();
    assign->call = std::move(*call);
    return assign;
  }
  
  return nullptr;
}
