//
//  lexical analysis.hpp
//  STELA
//
//  Created by Indi Kernick on 17/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_lexical_analysis_hpp
#define stela_lexical_analysis_hpp

#include "error.hpp"
#include "token.hpp"

namespace stela {

Tokens lex(std::string_view, Logger &);

}

#endif
