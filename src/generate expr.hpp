//
//  generate expr.hpp
//  STELA
//
//  Created by Indi Kernick on 1/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_expr_hpp
#define stela_generate_expr_hpp

#include "ast.hpp"
#include "gen context.hpp"
#include <llvm/IR/IRBuilder.h>

namespace stela {

enum class ArithNumber {
  signed_int,
  unsigned_int,
  floating_point
};
ArithNumber classifyArith(ast::Expression *);

llvm::Value *generateAddrExpr(gen::Ctx, llvm::IRBuilder<> &, ast::Expression *);
llvm::Value *generateValueExpr(gen::Ctx, llvm::IRBuilder<> &, ast::Expression *);

}

#endif
