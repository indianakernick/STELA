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
#include "categories.hpp"
#include "gen context.hpp"
#include "function builder.hpp"

namespace stela {

namespace gen {

struct Expr {
  llvm::Value *obj;
  ValueCat cat;
};

}

gen::Expr generateValueExpr(gen::Ctx, FuncBuilder &, ast::Expression *);
gen::Expr generateExpr(gen::Ctx, FuncBuilder &, ast::Expression *, llvm::Value *);

}

#endif
