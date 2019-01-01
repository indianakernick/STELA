//
//  expr lookup.hpp
//  STELA
//
//  Created by Indi Kernick on 26/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_expr_lookup_hpp
#define stela_expr_lookup_hpp

#include "ast.hpp"
#include "context.hpp"
#include "expr stack.hpp"

namespace stela {

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
  
  explicit ExprLookup(sym::Ctx);
  
  void call();
  ast::Declaration *lookupFunc(const sym::FuncParams &, Loc);
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
  void lookupMember(ast::MemberIdent &);
    // memVarExpr(expr)
    //   pop expr
    //   pop member
    //   member variable lookup
    //   push expr
    //   return object
    // else
    //   return null
  
  void lookupIdent(ast::Identifier &);
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
  ast::TypePtr topType() const;
  
  void expected(const ast::TypePtr &);
  
private:
  struct FunKey {
    sym::Name name;
    sym::FuncParams args;
  };
  
  sym::Ctx ctx;
  ast::TypePtr expType;
  ExprStack stack;
  
  ast::Func *popCallPushRet(sym::Func *);
  
  sym::Symbol *lookupIdent(sym::Scope *, const sym::Name &);
  sym::Func *lookupFun(sym::Scope *, const FunKey &, Loc);
  sym::BtnFunc *lookupBtnFun(sym::Scope *, const sym::Name &);
  
  ast::Func *pushFunPtr(sym::Scope *, const sym::Name &, Loc);
};

}

#endif
