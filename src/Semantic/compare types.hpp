//
//  compare types.hpp
//  STELA
//
//  Created by Indi Kernick on 5/9/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_compare_types_hpp
#define stela_compare_types_hpp

#include "ast.hpp"
#include "context.hpp"

namespace stela {

bool compareTypes(sym::Ctx, const ast::TypePtr &, const ast::TypePtr &);
void validateType(sym::Ctx, const ast::TypePtr &);

}

#endif
