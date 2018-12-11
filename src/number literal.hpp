//
//  number literal.hpp
//  STELA
//
//  Created by Indi Kernick on 26/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_number_literal_hpp
#define stela_number_literal_hpp

#include "number.hpp"
#include <string_view>
#include "log output.hpp"

namespace stela {

/// If a valid number literal can be found at the start of the string, the
/// size (in characters) of the literal is returned, otherwise 0 is returned
size_t validNumberLiteral(std::string_view, Loc, Log &);

/// Parses the number literal at the start of the string. Returns a NumberVariant
/// that holds the number literal in a type that most accuratly represents it
/// or NumberVariant::Invalid if the number cannot be parsed
NumberVariant parseNumberLiteral(std::string_view, Log &);

}

#endif
