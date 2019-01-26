//
//  c standard library.hpp
//  STELA
//
//  Created by Indi Kernick on 4/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_c_standard_library_hpp
#define stela_c_standard_library_hpp

#include "log.hpp"
#include "symbols.hpp"

namespace stela {

AST makeCmath(sym::Builtins &, LogSink &);

}

#endif
