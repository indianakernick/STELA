//
//  generate func.hpp
//  STELA
//
//  Created by Indi Kernick on 2/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_func_hpp
#define stela_generate_func_hpp

#include "ast.hpp"
#include "symbols.hpp"
#include "gen context.hpp"

namespace stela {

gen::String generateNullFunc(gen::Ctx, const ast::FuncType &);
gen::String generateMakeFunc(gen::Ctx, ast::FuncType &);
gen::String boolConv(gen::Ctx, const sym::ExprType &, ast::Name);
gen::String callClosure(gen::Ctx, const sym::ExprType &, ast::Name);

}

#endif
