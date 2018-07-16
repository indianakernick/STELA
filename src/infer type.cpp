//
//  infer type.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "infer type.hpp"

#include "operator name.hpp"

using namespace stela;
using namespace stela::ast;

namespace {

class InferVisitor final : public ast::Visitor {
public:
  explicit InferVisitor(stela::SymbolMan &man)
    : man{man}, log{man.logger()} {}

  void visit(Assignment &as) override {
    as.left->accept(*this);
    const sym::ExprType left = etype;
    as.right->accept(*this);
    const sym::ExprType right = etype;
    sym::Func *func = man.lookup(opName(as.oper), {left, right}, as.loc);
    etype = func->ret;
  }
  void visit(BinaryExpr &bin) override {
    bin.left->accept(*this);
    const sym::ExprType left = etype;
    bin.right->accept(*this);
    const sym::ExprType right = etype;
    sym::Func *func = man.lookup(opName(bin.oper), {left, right}, bin.loc);
    etype = func->ret;
  }
  void visit(UnaryExpr &un) override {
    un.expr->accept(*this);
    sym::Func *func = man.lookup(opName(un.oper), {etype}, un.loc);
    etype = func->ret;
  }
  void visit(FuncCall &call) override {
    if (auto *id = dynamic_cast<ast::Identifier *>(call.func.get())) {
      sym::FuncParams params;
      for (const ast::ExprPtr &expr : call.args) {
        expr->accept(*this);
        params.push_back(etype);
      }
      sym::Func *func = man.lookup(id->name, params, call.loc);
      etype = func->ret;
    } else {
      assert(false);
    }
  }
  void visit(MemberIdent &mem) override {
    mem.object->accept(*this);
    // lookup members
  }
  void visit(InitCall &init) override {
    etype.type = man.type(init.type);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(Identifier &) override {
    assert(false);
  }
  
  sym::ExprType etype;

private:
  SymbolMan &man;
  Log &log;
};

}

sym::ExprType stela::inferType(SymbolMan &man, ast::Expression *expr) {
  InferVisitor visitor{man};
  expr->accept(visitor);
  return std::move(visitor.etype);
}
