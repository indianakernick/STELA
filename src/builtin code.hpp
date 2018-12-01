//
//  builtin code.hpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_builtin_code_hpp
#define stela_builtin_code_hpp

#include "gen string.hpp"

namespace stela {

void appendBuiltinCode(std::string &);
void appendTypes(std::string &, const gen::String &);
void appendFuncs(std::string &, const gen::String &);
void appendCode(std::string &, const gen::String &);

}

#endif
