//
//  check missing return.hpp
//  STELA
//
//  Created by Indi Kernick on 12/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_check_missing_return_hpp
#define stela_check_missing_return_hpp

#include "ast.hpp"
#include "context.hpp"

namespace stela {

void checkMissingRet(sym::Ctx, ast::Block &, const ast::TypePtr &, Loc);

}

#endif
