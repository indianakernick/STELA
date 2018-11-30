//
//  generate decl.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate decl.hpp"

#include "symbols.hpp"
#include "generate type.hpp"

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
      if (etype.mut == sym::ValueMut::let) {
        name += "const ";
      }
      name += generateType(ctx, etype.type);
      name += ' ';
      if (etype.ref == sym::ValueRef::ref) {
        name += '&';
      }
      return name;
    }
  }
  
  void visit(ast::Func &func) override {
    sym::Func *symbol = func.symbol;
    ctx.fun += exprType(symbol->ret);
    ctx.fun += "f_";
    ctx.fun += func.name;
    ctx.fun += '(';
    ctx.fun += exprType(symbol->params.front());
    ctx.fun += "p_0";
    for (size_t p = 1; p != symbol->params.size(); ++p) {
      ctx.fun += ", ";
      ctx.fun += exprType(symbol->params[p]);
      ctx.fun += "p_";
      ctx.fun += std::to_string(p);
    }
    ctx.fun += ") noexcept {\n";
    func.body.accept(*this);
    ctx.fun += "}\n";
  }
  void appendVar(const ast::TypePtr &type, const ast::Name name) {
    if (type) {
      ctx.fun += generateType(ctx, type);
    } else {
      ctx.fun += "auto ";
    }
    ctx.fun += "v_";
    ctx.fun += name;
    ctx.fun += " = ";
  }
  void visit(ast::Var &var) override {
    appendVar(var.type, var.name);
  }
  void visit(ast::Let &let) override {
    ctx.fun += "const ";
    appendVar(let.type, let.name);
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
