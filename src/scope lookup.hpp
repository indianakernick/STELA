//
//  scope lookup.hpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_scope_lookup_hpp
#define stela_scope_lookup_hpp

#include "ast.hpp"
#include "symbols.hpp"
#include "log output.hpp"
#include "scope manager.hpp"

namespace stela {

sym::Symbol *lookupAny(sym::Scope *, Log &, const sym::Name &, Loc);
sym::Symbol *lookupType(sym::Scope *, Log &, const ast::TypePtr &);
sym::FuncParams lookupParams(sym::Scope *, Log &, const ast::FuncParams &);

class TypeLookup {
public:
  TypeLookup(ScopeMan &, Log &);

  sym::Symbol *lookupBuiltinType(const sym::Name &, Loc);
  sym::Symbol *lookupType(const ast::TypePtr &);
  sym::FuncParams lookupParams(const ast::FuncParams &);

private:
  ScopeMan &man;
  Log &log;
};

class ExprLookup {
public:
  
  /*
  
  object.member(args)
  
  call() {call}
  member("member") {call, m"member"}
  lookupIndent("object") {call, m"member", etype}
  lookupMember("member") {call, m"member", etype}
  lookupFunc({args})     {expr}
   
--------------------------------------------------------------------------------
  
  doStuff(args)
  
  call() {call}
  lookupIdent("doStuff") {call, i"doStuff"}
  getExprType() return null
  lookupFunc({args})     {expr}
  getExprType() return func->ret
  
--------------------------------------------------------------------------------
  
  object.subobject.member(args)
  
  call() {call}
  member("member") {call, member"member"}
  member("subobject") {call, member"member", member"subobject"}
  lookupIdent("object") {call, member"member", member"subobject", expr}
  lookupMember() {call, member"member", expr} return subobject currentType = false
  lookupMember() {call, member"member", expr} return null
  lookupFunc({args}) {expr} need subobject here
    pop call member expr
    push expr

--------------------------------------------------------------------------------

  object.member
  
  member("member") {m"member"}
  lookupIdent("object") {m"member", expr}
  getExprType() currentType = false;
  lookupMember() {expr} currentType = true;
  getExprType() returnedType = true;
  */
  
  ExprLookup(sym::Scope *, Log &);
  
  void call();
  sym::Func *lookupFunc(const sym::FuncParams &, Loc);
    // memFunExpr(expr)
    //   pop expr
    //   pop member
    //   pop call
    //   member function lookup
    //   push expr
    // memFunExpr(static_type)
    //   pop stuff
    //   static member function lookup
    //   push expr
    // freeFun()
    //   pop stuff
    //   free function lookup
    //   push expr
  sym::Func *lookupFunc(const sym::Name &, const sym::FuncParams &, Loc);
    // effectively this:
    //   call()
    //   lookupIdent(name)
    //   return lookupFunc(params, loc);
    // but just do free function lookup
  
  void member(const sym::Name &);
  sym::Symbol *lookupMember(Loc);
    // memVarExpr(expr)
    //   pop expr
    //   pop member
    //   member variable lookup
    //   push expr
    //   return object
    // memVarExpr(static_type)
    //   pop static_type
    //   pop member
    //   static member variable lookup
    //   push expr
    //   return object
    // else
    //   return null
  
  sym::Symbol *lookupIdent(const sym::Name &, Loc);
    // standard lookup
    // if name is function
    //   exprs.push_back({Expr::Type::ident, name});
    //   return null
    // if name is object
    //   exprs.push_back({Expr::Type::expr});
    //   set etype
    //   return object
    // if name is struct or enum
    //   exprs.push_back({Expr::Type::static_type});
    //   set etype to struct or enum
    //   currentEtype = false;
  sym::Symbol *lookupSelf(Loc);
  
  void setExprType(sym::ExprType);
  sym::ExprType getExprType();
  
private:
  struct Expr {
    enum class Type {
      call,
      member,
      ident,
      expr,
      static_type
    } type;
    sym::Name name;
    
    Expr(Type);
    Expr(Type, const sym::Name &);
  };
  sym::Scope *const scope;
  Log &log;
  std::vector<Expr> exprs;
  sym::ExprType etype;
  bool currentEtype = false;
  
  void pushExpr(sym::ExprType);
  void pushStatic(sym::Symbol *);
  bool memVarExpr(Expr::Type) const;
    // return top == type && below top == member && below below top != call
  bool memFunExpr(Expr::Type) const;
    // return top == type && below top == member && below below top == call
  bool freeFun() const;
    // return top == ident && below top == call
};

}

#endif