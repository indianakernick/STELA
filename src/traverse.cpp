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
      funcSym->ret = man.typeName(func.ret);
    } else {
      // @TODO infer return type
      log.ferror(func.loc) << "Return type inference has not been implemented" << endlog;
    }
    funcSym->params = man.funcParams(func.params);
    man.insert(func.name, std::move(funcSym), func.loc);
    
    man.enterScope();
    // process parameters and body
    man.leaveScope();
  }
  
  void visit(ast::Var &var) override {
    auto varSym = std::make_unique<sym::Var>();
    varSym->loc = var.loc;
    if (var.type) {
      varSym->type = man.typeName(var.type);
    } else {
      // @TODO infer variable type
      log.ferror(var.loc) << "Type inference has not been implemented" << endlog;
    }
    man.insert(var.name, std::move(varSym), var.loc);
  }
  void visit(ast::Let &let) override {
    auto letSym = std::make_unique<sym::Let>();
    letSym->loc = let.loc;
    if (let.type) {
      letSym->type = man.typeName(let.type);
    } else {
      // @TODO infer variable type
      log.ferror(let.loc) << "Type inference has not been implemented" << endlog;
    }
    man.insert(let.name, std::move(letSym), let.loc);
  }
  void visit(ast::TypeAlias &alias) override {
    auto aliasSym = std::make_unique<sym::TypeAlias>();
    aliasSym->loc = alias.loc;
    aliasSym->type = man.typeName(alias.type);
    man.insert(alias.name, std::move(aliasSym), alias.loc);
  }
  /* LCOV_EXCL_START */
  void visit(ast::Init &) override {}
  void visit(ast::Struct &) override {}
  void visit(ast::Enum &) override {}
  /* LCOV_EXCL_END */

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
