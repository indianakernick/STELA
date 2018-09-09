//
//  infer type.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "infer type.hpp"

#include "scope lookup.hpp"
#include "operator name.hpp"
#include "compare types.hpp"
#include "number literal.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(ScopeMan &man, Log &log, const Builtins &types)
    : lkp{man.cur(), log}, tlk{man.cur(), log}, man{man}, log{log}, bnt{types} {}

  void visit(ast::BinaryExpr &bin) override {
    const sym::ExprType left = visitValueExpr(bin.left.get());
    const sym::ExprType right = visitValueExpr(bin.right.get());
    if (auto *builtinLeft = tlk.lookupBuiltinType(left.type)) {
      if (auto *builtinRight = tlk.lookupBuiltinType(right.type)) {
        if (auto *retType = validOp(bnt, bin.oper, builtinLeft, builtinRight)) {
          sym::ExprType retExpr;
          retExpr.type = retType;
          retExpr.mut = sym::ValueMut::let;
          retExpr.ref = sym::ValueRef::val;
          lkp.setExpr(retExpr);
          return;
        }
      }
    }
    log.error(bin.loc) << "Invalid operands to binary expression " << opName(bin.oper) << fatal;
  }
  void visit(ast::UnaryExpr &un) override {
    const sym::ExprType etype = visitValueExpr(un.expr.get());
    if (auto *builtin = tlk.lookupBuiltinType(etype.type)) {
      if (validOp(un.oper, builtin)) {
        sym::ExprType retExpr;
        retExpr.type = etype.type;
        retExpr.mut = sym::ValueMut::let;
        retExpr.ref = sym::ValueRef::val;
        lkp.setExpr(retExpr);
        return;
      }
    }
    log.error(un.loc) << "Invalid operand to unary expression " << opName(un.oper) << fatal;
  }
  sym::FuncParams argTypes(const ast::FuncArgs &args) {
    sym::FuncParams params;
    for (const ast::ExprPtr &expr : args) {
      params.push_back(getExprType(man, log, bnt, expr.get()));
    }
    return params;
  }
  void visit(ast::FuncCall &call) override {
    lkp.call();
    call.func->accept(*this);
    call.definition = lkp.lookupFunc(argTypes(call.args), call.loc);
  }
  void visit(ast::MemberIdent &mem) override {
    lkp.member(sym::Name(mem.member));
    mem.object->accept(*this);
    mem.definition = lkp.lookupMember(mem.loc);
  }
  void visit(ast::Identifier &id) override {
    id.definition = lkp.lookupIdent(sym::Name(id.name), id.loc);
  }
  void visit(ast::Ternary &tern) override {
    const sym::ExprType cond = visitValueExpr(tern.cond.get());
    if (!compareTypes(tlk, cond.type, bnt.Bool)) {
      log.error(tern.loc) << "Condition expression must be of type Bool" << fatal;
    }
    const sym::ExprType tru = visitValueExpr(tern.tru.get());
    const sym::ExprType fals = visitValueExpr(tern.fals.get());
    if (!compareTypes(tlk, tru.type, fals.type)) {
      log.error(tern.loc) << "True and false branch of ternary condition must have same type" << fatal;
    }
    sym::ExprType etype;
    etype.type = tru.type;
    etype.mut = sym::common(tru.mut, fals.mut);
    etype.ref = sym::common(tru.ref, fals.ref);
    lkp.setExpr(etype);
  }
  
  void visit(ast::StringLiteral &) override {
    sym::ExprType etype;
    etype.type = bnt.String;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::CharLiteral &) override {
    sym::ExprType etype;
    etype.type = bnt.Char;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::NumberLiteral &n) override {
    const NumberVariant num = parseNumberLiteral(n.value, log);
    sym::ExprType etype;
    if (num.type == NumberVariant::Float) {
      etype.type = bnt.Double;
    } else if (num.type == NumberVariant::Int) {
      etype.type = bnt.Int64;
    } else if (num.type == NumberVariant::UInt) {
      etype.type = bnt.UInt64;
    }
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::BoolLiteral &) override {
    sym::ExprType etype;
    etype.type = bnt.Bool;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }

  sym::ExprType visitValueExpr(ast::Node *const node) {
    lkp.enterSubExpr();
    node->accept(*this);
    return lkp.leaveSubExpr();
  }

private:
  ExprLookup lkp;
  NameLookup tlk;
  ScopeMan &man;
  Log &log;
  const Builtins &bnt;
};

}

sym::ExprType stela::getExprType(
  ScopeMan &man,
  Log &log,
  const Builtins &bnt,
  ast::Expression *expr
) {
  return Visitor{man, log, bnt}.visitValueExpr(expr);
}
