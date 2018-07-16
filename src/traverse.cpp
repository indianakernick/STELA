//
//  traverse.cpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "traverse.hpp"

#include "infer type.hpp"
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
      funcSym->ret = {man.type(func.ret), sym::ValueCat::rvalue};
    } else {
      // @TODO infer return type
      log.warn(func.loc) << "Return type inference has not been implemented. "
        << "Return type is Void by default" << endlog;
      funcSym->ret = {man.lookup("Void", func.loc), sym::ValueCat::rvalue};
    }
    funcSym->params = man.funcParams(func.params);
    man.enterScope();
    for (size_t p = 0; p != func.params.size(); ++p) {
      auto paramSym = std::make_unique<sym::Object>();
      paramSym->loc = func.params[p].loc;
      paramSym->etype = funcSym->params[p];
      man.insert(func.params[p].name, std::move(paramSym));
    }
    for (const ast::StatPtr &stat : func.body.nodes) {
      stat->accept(*this);
    }
    man.leaveScope();
    man.insert(func.name, std::move(funcSym));
  }
  void visit(ast::Var &var) override {
    auto varSym = std::make_unique<sym::Object>();
    varSym->loc = var.loc;
    varSym->etype.cat = sym::ValueCat::lvalue_var;
    if (var.type) {
      varSym->etype.type = man.type(var.type);
    } else {
      varSym->etype.type = inferType(man, var.expr.get()).type;
    }
    man.insert(var.name, std::move(varSym));
  }
  void visit(ast::Let &let) override {
    auto letSym = std::make_unique<sym::Object>();
    letSym->loc = let.loc;
    letSym->etype.cat = sym::ValueCat::lvalue_let;
    if (let.type) {
      letSym->etype.type = man.type(let.type);
    } else {
      letSym->etype.type = inferType(man, let.expr.get()).type;
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
  void visit(ast::Struct &strut) override {
    auto structSym = std::make_unique<sym::StructType>();
    structSym->loc = strut.loc;
    structSym->scope = man.enterScope();
    for (const ast::Member &mem : strut.body) {
      // @TODO member, static, public, private
      mem.node->accept(*this);
    }
    man.leaveScope();
    man.insert(strut.name, std::move(structSym));
  }
  void visit(ast::Enum &num) override {
    auto enumSym = std::make_unique<sym::EnumType>();
    enumSym->loc = num.loc;
    enumSym->scope = man.enterScope();
    for (const ast::EnumCase &cs : num.cases) {
      auto caseSym = std::make_unique<sym::Object>();
      caseSym->loc = cs.loc;
      caseSym->etype.type = enumSym.get();
      caseSym->etype.cat = sym::ValueCat::lvalue_let;
      man.insert(cs.name, std::move(caseSym));
    }
    man.leaveScope();
    man.insert(num.name, std::move(enumSym));
  }

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
