//
//  scan decl.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "scan decl.hpp"

#include "lookup.hpp"
#include "clone ast.hpp"

using namespace stela;

namespace {

class DeclVisitor final : public ast::Visitor {
public:
  DeclVisitor(sym::Scope &scope, Log &log)
    : scope{scope}, log{log} {}

  void visit(ast::Func &func) override {
    auto funcSym = std::make_unique<sym::Func>();
    funcSym->loc = func.loc;
    if (func.ret) {
      funcSym->ret = clone(func.ret);
    } else {
      // @TODO infer return type
      log.ferror(func.loc) << "Return type inference has not been implemented" << endlog;
      return;
    }
    for (const ast::FuncParam &param : func.params) {
      funcSym->params.push_back(clone(param.type));
    }
    sym::Func *dupFunc = lookup(scope.table, func.name, funcSym->params);
    if (!dupFunc) {
      scope.table.insert({func.name, std::move(funcSym)});
    } else {
      log.ferror(func.loc) << "Redefinition of function " << func.name
        << " previously declared at " << dupFunc->loc << endlog;
    }
  }
  
  void visit(ast::Var &) override {}
  void visit(ast::Let &) override {}
  void visit(ast::TypeAlias &) override {}
  void visit(ast::Init &) override {}
  void visit(ast::Struct &) override {}
  void visit(ast::Enum &) override {}

private:
  sym::Scope &scope;
  Log &log;
};

}

void stela::scanDecl(sym::Scope &scope, const AST &ast, Log &log) {
  DeclVisitor visitor{scope, log};
  for (const ast::DeclPtr &decl : ast.global) {
    decl->accept(visitor);
  }
}
