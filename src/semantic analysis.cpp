//
//  semantic analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 9/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "semantic analysis.hpp"

#include "log output.hpp"
#include "clone ast.hpp"
#include "syntax analysis.hpp"

using namespace stela;

namespace {

void insert(sym::Table &table, const sym::Name name, sym::SymbolPtr sym, Log &log) {
  if (!table.insert({name, std::move(sym)}).second) {
    log.ferror() << "Redefinition of " << name << endlog;
  }
}

sym::Symbol *lookup(sym::Table &table, const sym::Name name, Log &log) {
  const auto iter = table.find(name);
  if (iter == table.end()) {
    log.ferror() << "Use of undefined symbol " << name << endlog;
    return nullptr;
  }
  sym::Symbol *const sym = iter->second.get();
  sym->referenced = true;
  return sym;
}

sym::SymbolPtr makeBuiltinType(const sym::BuiltinType::Enum e) {
  auto type = std::make_unique<sym::BuiltinType>();
  type->value = e;
  return type;
}

sym::Scope createGlobalScope(Log &log) {
  sym::Scope global;
  global.parent = 0;
  
  #define INSERT(TYPE)                                                          \
    insert(global.table, #TYPE, makeBuiltinType(sym::BuiltinType::TYPE), log)
  
  INSERT(Void);
  INSERT(Int);
  INSERT(Char);
  INSERT(Bool);
  INSERT(Float);
  INSERT(Double);
  INSERT(String);
  INSERT(Int8);
  INSERT(Int16);
  INSERT(Int32);
  INSERT(Int64);
  INSERT(UInt8);
  INSERT(UInt16);
  INSERT(UInt32);
  INSERT(UInt64);
  
  #undef INSERT
  
  return global;
}

class DeclVisitor final : public ast::Visitor {
public:
  DeclVisitor(sym::Scope &scope, Log &log)
    : scope{scope}, log{log} {}

  void visit(ast::Func &func) override {
    auto funcSym = std::make_unique<sym::Func>();
    if (func.ret) {
      funcSym->ret = clone(func.ret);
    }
    for (const ast::FuncParam &param : func.params) {
      funcSym->params.push_back(clone(param.type));
    }
    insert(scope.table, func.name, std::move(funcSym), log);
  }
  
  void visit(ast::Var &) override {}
  void visit(ast::Let &) override{}
  void visit(ast::TypeAlias &) override{}
  void visit(ast::Init &) override{}
  void visit(ast::Struct &) override{}
  void visit(ast::Enum &) override{}

private:
  sym::Scope &scope;
  Log &log;
};

}

stela::Symbols stela::createSym(const AST &ast, LogBuf &buf) {
  Log log{buf, LogCat::semantic};
  Symbols syms;
  syms.scopes.push_back(createGlobalScope(log));
  DeclVisitor declVisitor(syms.scopes[0], log);
  for (const ast::DeclPtr &decl : ast.global) {
    decl->accept(declVisitor);
  }
  return syms;
}

stela::SymbolsAST stela::createSym(const std::string_view source, LogBuf &buf) {
  SymbolsAST sa;
  sa.ast = createAST(source, buf);
  sa.sym = createSym(sa.ast, buf);
  return sa;
}
