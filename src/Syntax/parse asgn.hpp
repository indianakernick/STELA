//
//  parse asgn.hpp
//  STELA
//
//  Created by Indi Kernick on 5/9/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_parse_asgn_hpp
#define stela_parse_asgn_hpp

#include "ast.hpp"
#include "parse tokens.hpp"

namespace stela {

ast::AsgnPtr parseAsgn(ParseTokens &);

}

#endif
