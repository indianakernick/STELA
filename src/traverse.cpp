//
//  traverse.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "traverse.hpp"

#include "symbol manager.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(sym::Scopes &scopes, Log &log)
    : man{scopes, log}, log{log} {}

  void visit(ast::Func &func) override {
    auto funcSym = std::make_unique<sym::Func>();
    funcSym->loc = func.loc;
    if (func.ret) {
      funcSym->ret = man.type(func.ret);
    } else {
      // @TODO infer return type
      log.ferror(func.loc) << "Return type inference has not been implemented" << endlog;
    }
    funcSym->params = man.funcParams(func.params);
    man.insert(func.name, std::move(funcSym));
    
    man.enterScope();
    for (const ast::FuncParam &param : func.params) {
      auto paramSym = std::make_unique<sym::Object>();
      paramSym->loc = param.loc;
      paramSym->type = man.type(param.type);
      paramSym->mut = (param.ref == ast::ParamRef::inout);
      man.insert(param.name, std::move(paramSym));
    }
    for (const ast::StatPtr &stat : func.body.nodes) {
      stat->accept(*this);
    }
    man.leaveScope();
  }
  void visit(ast::Var &var) override {
    auto varSym = std::make_unique<sym::Object>();
    varSym->loc = var.loc;
    varSym->mut = true;
    if (var.type) {
      varSym->type = man.type(var.type);
    } else {
      // @TODO infer variable type
      log.ferror(var.loc) << "Type inference has not been implemented" << endlog;
    }
    man.insert(var.name, std::move(varSym));
  }
  void visit(ast::Let &let) override {
    auto letSym = std::make_unique<sym::Object>();
    letSym->loc = let.loc;
    letSym->mut = false;
    if (let.type) {
      letSym->type = man.type(let.type);
    } else {
      // @TODO infer variable type
      log.ferror(let.loc) << "Type inference has not been implemented" << endlog;
    }
    man.insert(let.name, std::move(letSym));
  }
  void visit(ast::TypeAlias &alias) override {
    auto aliasSym = std::make_unique<sym::TypeAlias>();
    aliasSym->loc = alias.loc;
    aliasSym->type = man.type(alias.type);
    man.insert(alias.name, std::move(aliasSym));
  }
  
  void visit(ast::Init &) override {}
  void visit(ast::Struct &) override {}
  void visit(ast::Enum &e) override {}

private:
  SymbolMan man;
  Log &log;
};

}

void stela::traverse(sym::Scopes &scopes, const AST &ast, Log &log) {
  Visitor visitor{scopes, log};
  for (const ast::DeclPtr &decl : ast.global) {
    decl->accept(visitor);
  }
}
