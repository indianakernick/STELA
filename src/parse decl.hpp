//
//  parse decl.hpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_parse_decl_hpp
#define stela_parse_decl_hpp

#include "ast.hpp"
#include "parse tokens.hpp"

namespace stela {

ast::DeclPtr parseVar(ParseTokens &, bool = false);
ast::DeclPtr parseLet(ParseTokens &, bool = false);
ast::DeclPtr parseDecl(ParseTokens &, bool = false);

}

#endif
