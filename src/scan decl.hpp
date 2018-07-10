//
//  scan decl.hpp
//  STELA
//
//  Created by Indi Kernick on 10/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_scan_decl_hpp
#define stela_scan_decl_hpp

#include "ast.hpp"
#include "symbols.hpp"
#include "log output.hpp"

namespace stela {

void scanDecl(sym::Scope &, const AST &, Log &);

}

#endif
