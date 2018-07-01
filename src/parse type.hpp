//
//  parse type.hpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_parse_type_hpp
#define stela_parse_type_hpp

#include "ast.hpp"
#include "parse tokens.hpp"

namespace stela {

ast::ParamRef parseRef(ParseTokens &);
ast::TypePtr parseType(ParseTokens &);

}

#endif
