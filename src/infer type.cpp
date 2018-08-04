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
#include "number literal.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(ScopeMan &man, Log &log)
    : lkp{man.cur(), log}, tlk{man, log}, log{log} {}

  void visit(ast::Assignment &as) override {
    as.left->accept(*this);
    const sym::ExprType left = etype;
    as.right->accept(*this);
    const sym::ExprType right = etype;
    as.definition = lkp.lookupFunc(sym::Name(opName(as.oper)), {left, right}, as.loc);
    etype = lkp.getExprType();
  }
  void visit(ast::BinaryExpr &bin) override {
    bin.left->accept(*this);
    const sym::ExprType left = etype;
    bin.right->accept(*this);
    const sym::ExprType right = etype;
    bin.definition = lkp.lookupFunc(sym::Name(opName(bin.oper)), {left, right}, bin.loc);
    etype = lkp.getExprType();
  }
  void visit(ast::UnaryExpr &un) override {
    un.expr->accept(*this);
    un.definition = lkp.lookupFunc(sym::Name(opName(un.oper)), {etype}, un.loc);
    etype = lkp.getExprType();
  }
  sym::FuncParams argTypes(const ast::FuncArgs &args) {
    sym::FuncParams params;
    for (const ast::ExprPtr &expr : args) {
      expr->accept(*this);
      params.push_back(etype);
    }
    return params;
  }
  void visit(ast::FuncCall &call) override {
    lkp.call();
    call.func->accept(*this);
    lkp.setExprType(etype);
    call.definition = lkp.lookupFunc(argTypes(call.args), call.loc);
    etype = call.definition->ret;
  }
  void visit(ast::MemberIdent &mem) override {
    lkp.member(sym::Name(mem.member));
    mem.object->accept(*this);
    lkp.setExprType(etype);
    mem.definition = lkp.lookupMember(mem.loc);
    etype = lkp.getExprType();
  }
  void visit(ast::InitCall &init) override {
    etype.type = tlk.lookupType(init.type);
    etype.cat = sym::ValueCat::rvalue;
    init.definition = lkp.lookupFunc(argTypes(init.args), init.loc);
  }
  void visit(ast::Identifier &id) override {
    id.definition = lkp.lookupIdent(sym::Name(id.name), id.loc);
    etype = lkp.getExprType();
  }
  void visit(ast::Self &self) override {
    self.definition = lkp.lookupSelf(self.loc);
    etype = lkp.getExprType();
  }
  void visit(ast::Ternary &tern) override {
    tern.cond->accept(*this);
    const sym::ExprType cond = etype;
    if (cond.type != tlk.lookupBuiltinType("Bool", tern.cond->loc)) {
      log.error(tern.loc) << "Condition of ternary conditional must be a Bool" << fatal;
    }
    tern.tru->accept(*this);
    const sym::ExprType tru = etype;
    tern.fals->accept(*this);
    const sym::ExprType fals = etype;
    if (tru.type != fals.type) {
      log.error(tern.loc) << "True and false branch of ternary condition must have same type" << fatal;
    }
    etype.type = tru.type;
    etype.cat = mostRestrictive(tru.cat, fals.cat);
  }
  
  void visit(ast::StringLiteral &s) override {
    etype.type = tlk.lookupBuiltinType("String", s.loc);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(ast::CharLiteral &c) override {
    etype.type = tlk.lookupBuiltinType("Char", c.loc);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(ast::NumberLiteral &n) override {
    const NumberVariant num = parseNumberLiteral(n.value, log);
    if (num.type == NumberVariant::Float) {
      etype.type = tlk.lookupBuiltinType("Double", n.loc);
    } else if (num.type == NumberVariant::Int) {
      etype.type = tlk.lookupBuiltinType("Int64", n.loc);
    } else if (num.type == NumberVariant::UInt) {
      etype.type = tlk.lookupBuiltinType("UInt64", n.loc);
    }
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(ast::BoolLiteral &b) override {
    etype.type = tlk.lookupBuiltinType("Bool", b.loc);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(ast::ArrayLiteral &) override {}
  void visit(ast::MapLiteral &) override {}
  void visit(ast::Lambda &) override {}
  
  sym::ExprType etype = sym::null_type;

private:
  ExprLookup lkp;
  TypeLookup tlk;
  Log &log;
};

}

sym::ExprType stela::getExprType(ScopeMan &man, Log &log, ast::Expression *expr) {
  Visitor visitor{man, log};
  expr->accept(visitor);
  return visitor.etype;
}