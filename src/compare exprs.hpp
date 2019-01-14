//
//  compare exprs.hpp
//  STELA
//
//  Created by Indi Kernick on 8/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_compare_exprs_hpp
#define stela_compare_exprs_hpp

#include "categories.hpp"
#include <llvm/IR/IRBuilder.h>

namespace stela {

class FuncInst;

class CompareExpr {
public:
  CompareExpr(FuncInst &, llvm::IRBuilder<> &);

  llvm::Value *eq(ast::Type *, gen::Expr, gen::Expr);
  llvm::Value *ne(ast::Type *, gen::Expr, gen::Expr);
  llvm::Value *lt(ast::Type *, gen::Expr, gen::Expr);
  llvm::Value *gt(ast::Type *, gen::Expr, gen::Expr);
  llvm::Value *le(ast::Type *, gen::Expr, gen::Expr);
  llvm::Value *ge(ast::Type *, gen::Expr, gen::Expr);

private:
  FuncInst &inst;
  llvm::IRBuilder<> &ir;
  
  llvm::Value *getBtnValue(gen::Expr);
};

}

#endif
