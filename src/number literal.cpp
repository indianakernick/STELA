//
//  number literal.cpp
//  STELA
//
//  Created by Indi Kernick on 26/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "number literal.hpp"

namespace {

template <typename Number>
size_t valid(const std::string_view str, char *const end, const Number num) {
  if (errno != ERANGE && !(str.data() == end && num == Number{})) {
    if (end - str.data() <= static_cast<ptrdiff_t>(str.size())) {
      return end - str.data();
    }
  }
  return 0;
}

}

size_t stela::validNumberLiteral(const std::string_view str, Log &) {
  // @TODO take a more manual approach to perhaps get better errors when we
  // encounter an invalid literal

  // check double
  // then check int64
  // then check uint64
  
  char *end;
  errno = 0;
  const auto d = std::strtod(str.data(), &end);
  if (const size_t size = valid(str, end, d)) {
    return size;
  }
  errno = 0;
  const auto ll = std::strtoll(str.data(), &end, 0);
  if (const size_t size = valid(str, end, ll)) {
    return size;
  }
  errno = 0;
  const auto ull = std::strtoull(str.data(), &end, 0);
  return valid(str, end, ull);
}

stela::NumberVariant stela::parseNumberLiteral(const std::string_view str, Log &) {
  char *end;
  errno = 0;
  const auto ll = std::strtoll(str.data(), &end, 0);
  if (valid(str, end, ll) == str.size()) {
    return NumberVariant{static_cast<int64_t>(ll)};
  }
  errno = 0;
  const auto ull = std::strtoull(str.data(), &end, 0);
  if (valid(str, end, ull) == str.size()) {
    return NumberVariant{static_cast<uint64_t>(ull)};
  }
  errno = 0;
  const auto d = std::strtod(str.data(), &end);
  if (valid(str, end, d) == str.size()) {
    return NumberVariant{d};
  }
  return NumberVariant{};
}
