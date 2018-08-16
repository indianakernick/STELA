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
#include "builtin symbols.hpp"

namespace stela {

class SymbolInserter {
public:
  virtual ~SymbolInserter() = default;
  
  virtual void insert(const sym::Name &, sym::SymbolPtr) = 0;
  virtual sym::Func *insert(const ast::Func &, const BuiltinTypes &) = 0;
  // @TODO might be able to get symbol from AST node
  virtual void enterFuncScope(sym::Func *, const ast::Func &) = 0;
};

class UnorderedInserter : public SymbolInserter {
public:
  UnorderedInserter(sym::UnorderedScope *, Log &);
  
  void insert(const sym::Name &, sym::SymbolPtr) override;
  sym::Func *insert(const ast::Func &, const BuiltinTypes &) override;
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
  sym::Func *insert(const ast::Func &, const BuiltinTypes &) override;
  void enterFuncScope(sym::Func *, const ast::Func &) override;
  
  sym::Func *insert(const ast::Init &);
  void enterFuncScope(sym::Func *, const ast::Init &);

  void accessScope(const ast::Member &);

private:
  sym::StructType *strut;
  Log &log;
  sym::MemAccess access;
  sym::MemScope scope;
  ast::MemMut mut;
  
  sym::Func *insertFunc(
    sym::FuncPtr &,
    const sym::Name &
  );
};

class EnumInserter final : public SymbolInserter {
public:
  EnumInserter(sym::EnumType *, Log &);
  
  void insert(const sym::Name &, sym::SymbolPtr) override;
  sym::Func *insert(const ast::Func &, const BuiltinTypes &) override;
  void enterFuncScope(sym::Func *, const ast::Func &) override;
  
  void insert(const ast::EnumCase &);

private:
  sym::EnumType *enm;
  Log &log;
};

class InserterManager {
public:
  InserterManager(sym::NSScope *, Log &, const BuiltinTypes &);
  
  template <typename Symbol, typename AST_Node>
  Symbol *insert(const AST_Node &node) {
    auto symbol = std::make_unique<Symbol>();
    Symbol *const ret = symbol.get();
    symbol->loc = node.loc;
    ins->insert(sym::Name(node.name), std::move(symbol));
    return ret;
  }
  void insert(const sym::Name &name, sym::SymbolPtr symbol) {
    assert(ins);
    ins->insert(name, std::move(symbol));
  }
  sym::Func *insert(const ast::Func &func) {
    assert(ins);
    return ins->insert(func, bnt);
  }
  void enterFuncScope(sym::Func *const funcSym, const ast::Func &func) {
    assert(ins);
    ins->enterFuncScope(funcSym, func);
  }
  
  SymbolInserter *set(SymbolInserter *);
  void restore(SymbolInserter *);
  SymbolInserter *get() const;

private:
  NSInserter defIns;
  SymbolInserter *ins;
  const BuiltinTypes &bnt;
};

}

#endif
