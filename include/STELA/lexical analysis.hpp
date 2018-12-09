//
//  lexical analysis.hpp
//  STELA
//
//  Created by Indi Kernick on 17/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_lexical_analysis_hpp
#define stela_lexical_analysis_hpp

#include "log.hpp"
#include "token.hpp"

namespace stela {

/// Split a source file into tokens. Multiple std::string_views refer directly
/// to the source string in later phases so the source should remain available
/// until compilation is complete.
Tokens tokenize(std::string_view, LogSink &);

}

#endif
