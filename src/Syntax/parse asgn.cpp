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

ast::AsgnPtr makeAssign(ParseTokens &tok, const ast::AssignOp op, ast::ExprPtr dst) {
  auto assign = make_retain<ast::CompAssign>();
  assign->loc = tok.lastLoc();
  assign->dst = std::move(dst);
  assign->oper = op;
  assign->src = tok.expectNode(parseExpr, "expression");
  return assign;
}

}

ast::AsgnPtr stela::parseAsgn(ParseTokens &tok) {
  ast::ExprPtr dst = parseNested(tok);
  if (!dst) {
    return nullptr;
  }
  if (auto *ident = dynamic_cast<ast::Identifier *>(dst.get())) {
    if (tok.checkOp(":=")) {
      auto decl = make_retain<ast::DeclAssign>();
      decl->loc = tok.lastLoc();
      decl->name = ident->name;
      decl->expr = tok.expectNode(parseExpr, "expression");
      return decl;
    }
  }
  
  if (const bool incr = tok.checkOp("++"); incr || tok.checkOp("--")) {
    auto incrDecr = make_retain<ast::IncrDecr>();
    incrDecr->loc = tok.lastLoc();
    incrDecr->incr = incr;
    incrDecr->expr = std::move(dst);
    return incrDecr;
  }
  
  if (tok.checkOp("=")) {
    auto assign = make_retain<ast::Assign>();
    assign->loc = tok.lastLoc();
    assign->dst = std::move(dst);
    assign->src = tok.expectNode(parseExpr, "expression");
    return assign;
  }
  
  #define CHECK(TOKEN, ENUM)                                                    \
    if (tok.checkOp(#TOKEN)) {                                                  \
      return makeAssign(tok, ast::AssignOp::ENUM, std::move(dst));              \
    }
  
  CHECK(+=, add)
  CHECK(-=, sub)
  CHECK(*=, mul)
  CHECK(/=, div)
  CHECK(%=, mod)
  CHECK(<<=, bit_shl)
  CHECK(>>=, bit_shr)
  CHECK(&=, bit_and)
  CHECK(^=, bit_xor)
  CHECK(|=, bit_or)
  
  #undef CHECK
  
  if (auto *call = dynamic_cast<ast::FuncCall *>(dst.get())) {
    auto assign = make_retain<ast::CallAssign>();
    assign->loc = call->loc;
    assign->call = std::move(*call);
    return assign;
  }
  
  tok.log().error(dst->loc) << "Expression used outside of assignment or function call" << fatal;
}
