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
gen::String generateLambda(gen::Ctx, const ast::Lambda &);
gen::String generateMakeLam(gen::Ctx, const ast::Lambda &);

}

#endif
