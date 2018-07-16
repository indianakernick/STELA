//
//  infer type.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "infer type.hpp"

#include "clone type.hpp"

using namespace stela;
using namespace stela::ast;

namespace {

class InferVisitor final : public ast::Visitor {
public:
  explicit InferVisitor(stela::SymbolMan &man)
    : man{man}, log{man.logger()} {}

  void visit(Assignment &as) override {
    as.left->accept(*this);
    if (!type.mut) {
      log.ferror(as.loc) << "Left side of assignment must be mutable" << endlog;
    }
    sym::Symbol *left = type.type;
    as.right->accept(*this);
    if (left != type.type) {
      log.ferror(as.loc) << "Both operands of assignment must have the same type" << endlog;
    }
    type.type = left;
    type.mut = true;
  }
  void visit(BinaryExpr &bin) override {
    bin.left->accept(*this);
    sym::Symbol *left = type.type;
    bin.right->accept(*this);
    if (left != type.type) {
      log.ferror(bin.loc) << "Both operands of binary operator must have the same type" << endlog;
    }
  }
  void visit(UnaryExpr &) override {}
  void visit(FuncCall &call) override {
    if (auto *id = dynamic_cast<ast::Identifier *>(call.func.get())) {
      sym::FuncParams params;
      for (const ast::ExprPtr &expr : call.args) {
        expr->accept(*this);
        params.push_back(type.type);
      }
      sym::Func *func = man.lookup(id->name, params, call.loc);
      type.type = func->ret;
      type.mut = false;
    } else {
      assert(false);
    }
  }
  void visit(MemberIdent &mem) override {
    mem.object->accept(*this);
    // lookup members
  }
  void visit(InitCall &init) override {
    type.type = man.type(init.type);
    type.mut = false;
  }
  void visit(Identifier &) override {
    assert(false);
  }
  
  ExprType type;

private:
  SymbolMan &man;
  Log &log;
};

}

stela::ExprType stela::inferType(SymbolMan &man, ast::Expression *expr) {
  InferVisitor visitor{man};
  expr->accept(visitor);
  return std::move(visitor.type);
}
