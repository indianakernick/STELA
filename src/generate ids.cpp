//
//  generate ids.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate ids.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  void visit(ast::BinaryExpr &expr) override {
    expr.left->accept(*this);
    expr.right->accept(*this);
  }
  void visit(ast::UnaryExpr &expr) override {
    expr.expr->accept(*this);
  }
  void visit(ast::FuncCall &call) override {
    call.func->accept(*this);
    for (const ast::ExprPtr &arg : call.args) {
      arg->accept(*this);
    }
  }
  void visit(ast::MemberIdent &mem) override {
    mem.object->accept(*this);
  }
  void visit(ast::Subscript &sub) override {
    sub.object->accept(*this);
    sub.index->accept(*this);
  }
  void visit(ast::Ternary &tern) override {
    tern.cond->accept(*this);
    tern.tru->accept(*this);
    tern.fals->accept(*this);
  }
  void visit(ast::Make &make) override {
    make.expr->accept(*this);
  }
  
  void visit(ast::Block &block) override {
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
  }
  void visit(ast::If &fi) override {
    fi.cond->accept(*this);
    fi.body->accept(*this);
    if (fi.elseBody) {
      fi.elseBody->accept(*this);
    }
  }
  void visit(ast::Switch &swich) override {
    for (ast::SwitchCase &cse : swich.cases) {
      if (cse.expr) {
        cse.expr->accept(*this);
      }
      cse.body->accept(*this);
    }
  }
  void visit(ast::Return &ret) override {
    if (ret.expr) {
      ret.expr->accept(*this);
    }
  }
  void visit(ast::While &wile) override {
    wile.cond->accept(*this);
    wile.body->accept(*this);
  }
  void visit(ast::For &four) override {
    if (four.init) {
      four.init->accept(*this);
    }
    four.cond->accept(*this);
    if (four.incr) {
      four.incr->accept(*this);
    }
    four.body->accept(*this);
  }
  
  void visit(ast::Func &func) override {
    visit(func.body);
  }
  void visit(ast::Var &var) override {
    if (var.expr) {
      var.expr->accept(*this);
    }
  }
  void visit(ast::Let &let) override {
    let.expr->accept(*this);
  }
  
  void visit(ast::CompAssign &assign) override {
    assign.left->accept(*this);
    assign.right->accept(*this);
  }
  void visit(ast::IncrDecr &incr) override {
    incr.expr->accept(*this);
  }
  void visit(ast::DeclAssign &assign) override {
    assign.expr->accept(*this);
  }
  void visit(ast::CallAssign &assign) override {
    visit(assign.call);
  }
  
  void visit(ast::ArrayLiteral &array) override {
    for (const ast::ExprPtr &expr : array.exprs) {
      expr->accept(*this);
    }
  }
  void visit(ast::InitList &list) override {
    for (const ast::ExprPtr &expr : list.exprs) {
      expr->accept(*this);
    }
  }
  void visit(ast::Lambda &lam) override {
    lam.id = lamID++;
    visit(lam.body);
  }
  
private:
  uint32_t lamID = 0;
};

}

void stela::generateIDs(const ast::Decls &decls) {
  Visitor visitor;
  for (const ast::DeclPtr &decl : decls) {
    decl->accept(visitor);
  }
}
