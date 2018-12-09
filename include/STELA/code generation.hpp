//
//  code generation.hpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_code_generation_hpp
#define stela_code_generation_hpp

#include "log.hpp"
#include "symbols.hpp"

namespace stela {

std::string generateCpp(const Symbols &, LogSink &);

}

#endif
