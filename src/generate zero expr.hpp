//
//  generate zero expr.hpp
//  STELA
//
//  Created by Indi Kernick on 1/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_zero_expr_hpp
#define stela_generate_zero_expr_hpp

#include "ast.hpp"
#include "gen context.hpp"
#include <llvm/IR/IRBuilder.h>

namespace stela {

llvm::Value *generateZeroExpr(gen::Ctx, llvm::IRBuilder<> &, ast::Type *);

}

#endif
