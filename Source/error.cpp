//
//  error.cpp
//  STELA
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "error.hpp"

#include <cstdio>
#include <type_traits>

stela::Error::Error(
  const char *type,
  const Loc loc,
  const char *msg
) : type{type}, msg{msg}, loc{loc} {}

const char *stela::Error::what() const noexcept {
  static_assert(std::is_same_v<Line, unsigned>);
  static_assert(std::is_same_v<Col, unsigned>);
  static char string[1024];
  std::snprintf(string, 1024, "%u:%u  %s - %s", loc.l, loc.c, type, msg);
  return string;
}
