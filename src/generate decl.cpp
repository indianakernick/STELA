//
//  generate decl.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate decl.hpp"

#include "symbols.hpp"
#include "unreachable.hpp"
#include "generate expr.hpp"
#include "generate type.hpp"
#include "operator name.hpp"
#include "generate zero expr.hpp"

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
    ctx.code += "{\n";
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
    ctx.code += "}\n";
  }
  void visit(ast::If &fi) override {
    ctx.code += "if (";
    ctx.code += generateExpr(ctx, fi.cond.get());
    ctx.code += ") ";
    fi.body->accept(*this);
    if (fi.elseBody) {
      ctx.code += " else ";
      fi.elseBody->accept(*this);
    }
  }
  
  ast::Statement *nearestFlow = nullptr;

  void visitFlow(ast::Statement &flow, const ast::StatPtr &body) {
    ast::Statement *oldFlow = std::exchange(nearestFlow, &flow);
    body->accept(*this);
    nearestFlow = oldFlow;
  }

  void visit(ast::Switch &swich) override {
    ctx.code += "{\n";
    ctx.code += "const auto v_swh_";
    ctx.code += swich.id;
    ctx.code += " = ";
    ctx.code += generateExpr(ctx, swich.expr.get());
    ctx.code += ";\n";
    
    const ast::SwitchCase *defaultCase = nullptr;
    for (const ast::SwitchCase &cse : swich.cases) {
      if (!cse.expr) {
        defaultCase = &cse;
        continue;
      }
      ctx.code += "if (v_swh_";
      ctx.code += swich.id;
      ctx.code += " == (";
      ctx.code += generateExpr(ctx, cse.expr.get());
      ctx.code += ")) goto CASE_LABEL_";
      ctx.code += cse.id;
      ctx.code += ";\n";
    }
    if (defaultCase) {
      ctx.code += "goto CASE_LABEL_";
      ctx.code += defaultCase->id;
      ctx.code += ";\n";
    }
    
    for (ast::SwitchCase &cse : swich.cases) {
      ctx.code += "CASE_LABEL_";
      ctx.code += cse.id;
      ctx.code += ": ;\n";
      visitFlow(cse, cse.body);
      ctx.code += "goto BREAK_LABEL_";
      ctx.code += swich.id;
      ctx.code += ";\n";
      ctx.code += "CONTINUE_LABEL_";
      ctx.code += cse.id;
      ctx.code += ": ;\n";
    }
    
    ctx.code += "}\n";
    ctx.code += "BREAK_LABEL_";
    ctx.code += swich.id;
    ctx.code += ": ;\n";
  }
  void visit(ast::Break &) override {
    assert(nearestFlow);
    ctx.code += "goto BREAK_LABEL_";
    if (auto *cse = dynamic_cast<ast::SwitchCase *>(nearestFlow)) {
      ctx.code += cse->parent->id;
    } else if (auto *wile = dynamic_cast<ast::While *>(nearestFlow)) {
      ctx.code += wile->id;
    } else if (auto *four = dynamic_cast<ast::For *>(nearestFlow)) {
      ctx.code += four->id;
    } else {
      UNREACHABLE();
    }
    ctx.code += ";\n";
  }
  void visit(ast::Continue &) override {
    assert(nearestFlow);
    ctx.code += "goto CONTINUE_LABEL_";
    if (auto *cse = dynamic_cast<ast::SwitchCase *>(nearestFlow)) {
      ctx.code += cse->id;
    } else if (auto *wile = dynamic_cast<ast::While *>(nearestFlow)) {
      ctx.code += wile->id;
    } else if (auto *four = dynamic_cast<ast::For *>(nearestFlow)) {
      ctx.code += four->id;
    } else {
      UNREACHABLE();
    }
    ctx.code += ";\n";
  }
  void visit(ast::Return &ret) override {
    ctx.code += "return";
    if (ret.expr) {
      ctx.code += ' ';
      ctx.code += generateExpr(ctx, ret.expr.get());
    }
    ctx.code += ";\n";
  }
  void visit(ast::While &wile) override {
    ctx.code += "while (";
    ctx.code += generateExpr(ctx, wile.cond.get());
    ctx.code += ") {\n";
    visitFlow(wile, wile.body);
    ctx.code += "CONTINUE_LABEL_";
    ctx.code += wile.id;
    ctx.code += ": ;\n";
    ctx.code += "}\n";
    ctx.code += "BREAK_LABEL_";
    ctx.code += wile.id;
    ctx.code += ": ;\n";
  }
  void visit(ast::For &four) override {
    ctx.code += "{\n";
    if (four.init) {
      four.init->accept(*this);
    }
    ctx.code += "while (";
    ctx.code += generateExpr(ctx, four.cond.get());
    ctx.code += ") {\n";
    visitFlow(four, four.body);
    ctx.code += "CONTINUE_LABEL_";
    ctx.code += four.id;
    ctx.code += ": ;\n";
    if (four.incr) {
      four.incr->accept(*this);
    }
    ctx.code += "}\n";
    ctx.code += "BREAK_LABEL_";
    ctx.code += four.id;
    ctx.code += ": ;\n";
    ctx.code += "}\n";
  }
  
  void visit(ast::Func &func) override {
    sym::Func *symbol = func.symbol;
    ctx.code += exprType(symbol->ret);
    ctx.code += "f_";
    ctx.code += func.id;
    ctx.code += '(';
    ctx.code += exprType(symbol->params.front());
    ctx.code += "p_0";
    for (size_t p = 1; p != symbol->params.size(); ++p) {
      ctx.code += ", ";
      ctx.code += exprType(symbol->params[p]);
      ctx.code += "p_";
      ctx.code += p;
    }
    ctx.code += ") noexcept ";
    func.body.accept(*this);
  }
  void appendVar(const sym::ExprType &etype, const uint32_t id, ast::Expression *expr) {
    ctx.code += exprType(etype);
    ctx.code += "v_";
    ctx.code += id;
    ctx.code += " = ";
    if (expr) {
      ctx.code += generateExpr(ctx, expr);
    } else {
      ctx.code += generateZeroExpr(ctx, etype.type.get());
    }
    ctx.code += ";\n";
  }
  void visit(ast::Var &var) override {
    appendVar(var.symbol->etype, var.id, var.expr.get());
  }
  void visit(ast::Let &let) override {
    appendVar(let.symbol->etype, let.id, let.expr.get());
  }
  
  void visit(ast::CompAssign &assign) override {
    ctx.code += generateExpr(ctx, assign.left.get());
    ctx.code += ' ';
    ctx.code += opName(assign.oper);
    ctx.code += ' ';
    ctx.code += generateExpr(ctx, assign.right.get());
    ctx.code += ";\n";
  }
  void visit(ast::IncrDecr &incr) override {
    if (incr.incr) {
      ctx.code += "++";
    } else {
      ctx.code += "--";
    }
    ctx.code += '(';
    ctx.code += generateExpr(ctx, incr.expr.get());
    ctx.code += ");\n";
  }
  void visit(ast::Assign &assign) override {
    ctx.code += generateExpr(ctx, assign.left.get());
    ctx.code += " = ";
    ctx.code += generateExpr(ctx, assign.right.get());
    ctx.code += ";\n";
  }
  void visit(ast::DeclAssign &decl) override {
    appendVar(decl.symbol->etype, decl.id, decl.expr.get());
  }
  void visit(ast::CallAssign &call) override {
    ctx.code += generateExpr(ctx, &call.call);
    ctx.code += ";\n";
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
