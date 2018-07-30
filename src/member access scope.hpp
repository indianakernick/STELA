//
//  member access scope.hpp
//  STELA
//
//  Created by Indi Kernick on 30/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_member_access_scope_hpp
#define stela_member_access_scope_hpp

#include "ast.hpp"
#include "symbols.hpp"
#include "log output.hpp"

namespace stela {

sym::MemAccess memAccess(const ast::Member &, Log &);
sym::MemScope memScope(const ast::Member &, Log &);

}

#endif
