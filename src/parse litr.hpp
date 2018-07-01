//
//  parse litr.hpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef parse_litr_hpp
#define parse_litr_hpp

#include "ast.hpp"
#include "parse tokens.hpp"

namespace stela {

ast::LitrPtr parseLitr(ParseTokens &);

}

#endif
