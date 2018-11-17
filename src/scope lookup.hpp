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

class NameLookup {
public:
  NameLookup(sym::Modules &, ScopeMan &, Log &);
  
  ast::TypeAlias *lookupType(ast::NamedType &) const;
  
  ast::TypePtr lookupConcreteType(const ast::TypePtr &) const;
  ast::TypePtr validateType(const ast::TypePtr &) const;
  
  template <typename Type>
  retain_ptr<Type> lookupConcrete(const ast::TypePtr &type) const {
    return dynamic_pointer_cast<Type>(lookupConcreteType(type));
  }

private:
  sym::Modules &modules;
  ScopeMan &man;
  Log &log;
  
  ast::TypeAlias *lookupType(sym::Scope *, ast::NamedType &) const;
  void validateStruct(const retain_ptr<ast::StructType> &) const;
};

class ExprLookup {
public:
  
  /*
  
  object.member(args)
  
  call()                 {call}
  member("member")       {call, m"member"}
  lookupIdent("object")  {call, m"member", etype}
  lookupMember("member") {call, m"member", etype}
  lookupFunc({args})     {expr}
   
--------------------------------------------------------------------------------
  
  doStuff(args)
  
  call()                 {call}
  lookupIdent("doStuff") {call, i"doStuff"}
  getExprType()          return null
  lookupFunc({args})     {expr}
  getExprType()          return func->ret
  
--------------------------------------------------------------------------------
  
  object.subobject.member(args)
  
  call()                {call}
  member("member")      {call, member"member"}
  member("subobject")   {call, member"member", member"subobject"}
  lookupIdent("object") {call, member"member", member"subobject", expr}
  lookupMember()        {call, member"member", expr} return subobject currentType = false
  lookupMember()        {call, member"member", expr} return null
  lookupFunc({args})    {expr} need subobject here
    pop call member expr
    push expr

--------------------------------------------------------------------------------

  object.member
  
  member("member")      {m"member"}
  lookupIdent("object") {m"member", expr}
  getExprType()         currentType = false;
  lookupMember()        {expr} currentType = true;
  getExprType()         returnedType = true;
  */
  
  ExprLookup(sym::Modules &, ScopeMan &, Log &);
  
  void call();
  ast::Func *lookupFunc(const sym::FuncParams &, Loc);
    // memFunExpr(expr)
    //   pop expr
    //   pop member
    //   pop call
    //   member function lookup
    //   push expr
    // freeFun()
    //   pop stuff
    //   free function lookup
    //   push expr
  
  void member(const sym::Name &);
  ast::Field *lookupMember(Loc);
    // memVarExpr(expr)
    //   pop expr
    //   pop member
    //   member variable lookup
    //   push expr
    //   return object
    // else
    //   return null
  
  ast::Statement *lookupIdent(const sym::Name &, const sym::Name &, Loc);
    // standard lookup
    // if name is function
    //   exprs.push_back({Expr::Type::free_fun, name});
    //   return null
    // if name is object
    //   exprs.push_back({Expr::Type::expr});
    //   set etype
    //   return object
  
  void setExpr(sym::ExprType);
  void enterSubExpr();
  sym::ExprType leaveSubExpr();
  
private:
  struct Expr {
    enum class Type {
      call,
      member,
      free_fun,
      expr,
      subexpr
    };
    
    Type type;
    sym::Name name;
    sym::Scope *scope;
    
    Expr(Type);
    Expr(Type, const sym::Name &);
    Expr(Type, const sym::Name &, sym::Scope *);
  };
  
  struct FunKey {
    sym::Name name;
    sym::FuncParams params;
  };
  
  sym::Modules &modules;
  ScopeMan &man;
  Log &log;
  std::vector<Expr> exprs;
  sym::ExprType etype;
  
  void pushExpr(const sym::ExprType &);
  ast::Statement *pushObj(sym::Object *);
  bool memVarExpr(Expr::Type) const;
    // return top == type && below top == member && below below top != call
  bool memFunExpr(Expr::Type) const;
    // return top == type && below top == member && below below top == call
  bool call(Expr::Type) const;
    // return top == type && below top == call
  
  sym::Name popName();
  sym::ExprType popExpr();
  ast::Func *popCallPushRet(sym::Func *);
  
  sym::Symbol *lookupIdent(sym::Scope *, const sym::Name &, Loc);
  sym::Func *lookupFun(sym::Scope *, const FunKey &, Loc);
  FunKey funKey(sym::ExprType, const sym::FuncParams &);
};

}

#endif
