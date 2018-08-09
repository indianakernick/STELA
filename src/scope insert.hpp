//
//  scope insert.hpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_scope_insert_hpp
#define stela_scope_insert_hpp

#include "ast.hpp"
#include <cassert>
#include "symbols.hpp"
#include "log output.hpp"
#include "scope manager.hpp"

namespace stela {

class SymbolInserter {
public:
  virtual ~SymbolInserter() = default;
  
  virtual void insert(const sym::Name &, sym::SymbolPtr) = 0;
  virtual sym::Func *insert(const ast::Func &) = 0;
  // @TODO might be able to get symbol from AST node
  virtual void enterFuncScope(sym::Func *, const ast::Func &) = 0;
};

class UnorderedInserter : public SymbolInserter {
public:
  UnorderedInserter(sym::UnorderedScope *, Log &);
  
  void insert(const sym::Name &, sym::SymbolPtr) override;
  sym::Func *insert(const ast::Func &) override;
  void enterFuncScope(sym::Func *, const ast::Func &) override;

private:
  sym::UnorderedScope *scope;
  Log &log;
};

class NSInserter final : public UnorderedInserter {
public:
  NSInserter(sym::NSScope *, Log &);
};

class BlockInserter final : public UnorderedInserter {
public:
  BlockInserter(sym::BlockScope *, Log &);
};

class FuncInserter final : public UnorderedInserter {
public:
  FuncInserter(sym::FuncScope *, Log &);
};

class StructInserter final : public SymbolInserter {
public:
  StructInserter(sym::StructType *, Log &);

  void insert(const sym::Name &, sym::SymbolPtr) override;
  sym::Func *insert(const ast::Func &) override;
  void enterFuncScope(sym::Func *, const ast::Func &) override;

  void accessScope(const ast::Member &);

private:
  sym::StructType *strut;
  Log &log;
  sym::MemAccess access;
  sym::MemScope scope;
  ast::MemMut mut;
};

class EnumInserter final : public SymbolInserter {
public:
  EnumInserter(sym::EnumType *, Log &);
  
  void insert(const sym::Name &, sym::SymbolPtr) override;
  sym::Func *insert(const ast::Func &) override;
  void enterFuncScope(sym::Func *, const ast::Func &) override;
  
  void insert(const ast::EnumCase &);

private:
  sym::EnumType *enm;
  Log &log;
};

class InserterManager {
public:
  InserterManager(sym::NSScope *, Log &);
  
  template <typename Symbol>
  Symbol *insert(const sym::Name &name) {
    static_assert(!std::is_same_v<Symbol, sym::Func>);
    assert(ins);
    auto symbol = std::make_unique<Symbol>();
    Symbol *const ret = symbol.get();
    ins->insert(name, std::move(symbol));
    return ret;
  }
  template <typename Symbol, typename AST_Node>
  Symbol *insert(const AST_Node &node) {
    Symbol *const symbol = insert<Symbol>(sym::Name(node.name));
    symbol->loc = node.loc;
    return symbol;
  }
  void insert(const sym::Name &name, sym::SymbolPtr symbol) {
    assert(ins);
    ins->insert(name, std::move(symbol));
  }
  sym::Func *insert(const ast::Func &func) {
    assert(ins);
    return ins->insert(func);
  }
  void enterFuncScope(sym::Func *const funcSym, const ast::Func &func) {
    assert(ins);
    ins->enterFuncScope(funcSym, func);
  }
  
  SymbolInserter *set(SymbolInserter *);
  SymbolInserter *setDef();
  void restore(SymbolInserter *);

private:
  NSInserter defIns;
  SymbolInserter *ins;
};

}

#endif
