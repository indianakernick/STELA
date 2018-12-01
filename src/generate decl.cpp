//
//  generate decl.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate decl.hpp"

#include "symbols.hpp"
#include "generate expr.hpp"
#include "generate type.hpp"
#include "operator name.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  explicit Visitor(gen::Ctx ctx)
    : ctx{ctx} {}
  
  gen::String exprType(const sym::ExprType &etype) {
    if (etype.type == nullptr) {
      return gen::String{"void *"};
    } else {
      gen::String name;
      name += generateType(ctx, etype.type.get());
      name += ' ';
      if (etype.ref == sym::ValueRef::ref) {
        name += '&';
      }
      return name;
    }
  }
  
  void visit(ast::Block &block) override {
    ctx.fun += "{\n";
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
    ctx.fun += "}\n";
  }
  void visit(ast::If &fi) override {
    ctx.fun += "if (";
    ctx.fun += generateExpr(ctx, fi.cond.get());
    ctx.fun += ") ";
    fi.body->accept(*this);
    if (fi.elseBody) {
      ctx.fun += " else ";
      fi.elseBody->accept(*this);
    }
  }
  void visit(ast::Return &ret) override {
    ctx.fun += "return";
    if (ret.expr) {
      ctx.fun += ' ';
      ctx.fun += generateExpr(ctx, ret.expr.get());
    }
    ctx.fun += ";\n";
  }
  void visit(ast::While &wile) override {
    ctx.fun += "while (";
    ctx.fun += generateExpr(ctx, wile.cond.get());
    ctx.fun += ") ";
    wile.body->accept(*this);
  }
  void visit(ast::For &four) override {
    ctx.fun += "{\n";
    if (four.init) {
      four.init->accept(*this);
    }
    ctx.fun += "while (";
    ctx.fun += generateExpr(ctx, four.cond.get());
    ctx.fun += ") {\n";
    four.body->accept(*this);
    if (four.incr) {
      four.incr->accept(*this);
    }
    ctx.fun += "}\n";
    ctx.fun += "}\n";
  }
  
  void visit(ast::Func &func) override {
    sym::Func *symbol = func.symbol;
    ctx.fun += exprType(symbol->ret);
    ctx.fun += "f_";
    ctx.fun += func.id;
    ctx.fun += '(';
    ctx.fun += exprType(symbol->params.front());
    ctx.fun += "p_0";
    for (size_t p = 1; p != symbol->params.size(); ++p) {
      ctx.fun += ", ";
      ctx.fun += exprType(symbol->params[p]);
      ctx.fun += "p_";
      ctx.fun += p;
    }
    ctx.fun += ") noexcept ";
    func.body.accept(*this);
  }
  void appendVar(const sym::ExprType &etype, const uint32_t id, ast::Expression *expr) {
    ctx.fun += exprType(etype);
    ctx.fun += "v_";
    ctx.fun += id;
    ctx.fun += " = ";
    if (expr) {
      ctx.fun += generateExpr(ctx, expr);
    } else {
      ctx.fun += "{}";
    }
    ctx.fun += ";\n";
  }
  void visit(ast::Var &var) override {
    appendVar(var.symbol->etype, var.id, var.expr.get());
  }
  void visit(ast::Let &let) override {
    appendVar(let.symbol->etype, let.id, let.expr.get());
  }
  
  void visit(ast::CompAssign &assign) override {
    ctx.fun += generateExpr(ctx, assign.left.get());
    ctx.fun += ' ';
    ctx.fun += opName(assign.oper);
    ctx.fun += ' ';
    ctx.fun += generateExpr(ctx, assign.right.get());
    ctx.fun += ";\n";
  }
  void visit(ast::IncrDecr &incr) override {
    if (incr.incr) {
      ctx.fun += "++";
    } else {
      ctx.fun += "--";
    }
    ctx.fun += '(';
    ctx.fun += generateExpr(ctx, incr.expr.get());
    ctx.fun += ");\n";
  }
  void visit(ast::Assign &assign) override {
    ctx.fun += generateExpr(ctx, assign.left.get());
    ctx.fun += " = ";
    ctx.fun += generateExpr(ctx, assign.right.get());
    ctx.fun += ";\n";
  }
  void visit(ast::DeclAssign &decl) override {
    appendVar(decl.symbol->etype, decl.id, decl.expr.get());
  }
  void visit(ast::CallAssign &call) override {
    ctx.fun += generateExpr(ctx, &call.call);
    ctx.fun += ";\n";
  }
  
private:
  gen::Ctx ctx;
};

}

void stela::generateDecl(gen::Ctx ctx, const ast::Decls &decls) {
  Visitor visitor{ctx};
  for (const ast::DeclPtr &decl : decls) {
    decl->accept(visitor);
  }
}
