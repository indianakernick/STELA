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
#include "function builder.hpp"

namespace stela {

enum class ArithNumber {
  signed_int,
  unsigned_int,
  floating_point
};
ArithNumber classifyArith(ast::Expression *);

llvm::Value *generateAddrExpr(gen::Ctx, FunctionBuilder &, ast::Expression *);
llvm::Value *generateValueExpr(gen::Ctx, FunctionBuilder &, ast::Expression *);
void generateDiscardExpr(gen::Ctx, FunctionBuilder &, ast::Expression *);

}

#endif
