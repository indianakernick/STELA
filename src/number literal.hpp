//
//  number literal.hpp
//  STELA
//
//  Created by Indi Kernick on 26/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_number_literal_hpp
#define stela_number_literal_hpp

#include <string_view>
#include "log output.hpp"

namespace stela {

/// If a valid number literal can be found at the start of the string, the
/// size (in characters) of the literal is returned, otherwise 0 is returned
size_t validNumberLiteral(std::string_view, Log &);

// Xcode doesn't have std::variant yet
// maybe I should use CLion for this project
struct FakeVariant {
  union {
    double f;
    int64_t i;
    uint64_t u;
  } value;
  enum {
    Invalid,
    Float,
    Int,
    UInt
  } type;
  
  FakeVariant()
    : type{Invalid} {}
  explicit FakeVariant(const double f) {
    value.f = f;
    type = Float;
  }
  explicit FakeVariant(const int64_t i) {
    value.i = i;
    type = Int;
  }
  explicit FakeVariant(const uint64_t u) {
    value.u = u;
    type = UInt;
  }
};

/// Parses the number literal at the start of the string. Returns a FakeVariant
/// that holds the number literal in a type that most accuratly represents it
/// or FakeVariant::Invalid if the number cannot be parsed
FakeVariant parseNumberLiteral(std::string_view, Log &);

}

#endif
