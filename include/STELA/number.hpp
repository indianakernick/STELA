//
//  number.hpp
//  STELA
//
//  Created by Indi Kernick on 11/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_number_hpp
#define stela_number_hpp

#include <variant>
#include <cstdint>

namespace stela {

using Void = void;
using Opaq = void *;
using Bool = bool;
using Byte = uint8_t;
using Char = int8_t;
using Real = float;
using Sint = int32_t;
using Uint = uint32_t;

using NumberVariant = std::variant<std::monostate, Byte, Char, Real, Sint, Uint>;

}

#endif
