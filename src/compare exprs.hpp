//
//  compare exprs.hpp
//  STELA
//
//  Created by Indi Kernick on 8/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_compare_exprs_hpp
#define stela_compare_exprs_hpp

#include "generate expr.hpp" // @TODO move gen::Expr
#include <llvm/IR/IRBuilder.h>

namespace stela {

namespace gen {

class FuncInst;

}

class CompareExpr {
public:
  CompareExpr(gen::FuncInst &, llvm::IRBuilder<> &);

  llvm::Value *equal(ast::Type *, gen::Expr, gen::Expr);
  llvm::Value *notEqual(ast::Type *, gen::Expr, gen::Expr);
  llvm::Value *less(ast::Type *, gen::Expr, gen::Expr);
  llvm::Value *greater(ast::Type *, gen::Expr, gen::Expr);
  llvm::Value *lessEqual(ast::Type *, gen::Expr, gen::Expr);
  llvm::Value *greaterEqual(ast::Type *, gen::Expr, gen::Expr);

private:
  gen::FuncInst &inst;
  llvm::IRBuilder<> &ir;
  
  llvm::Value *getBtnValue(gen::Expr);
};

}

#endif
