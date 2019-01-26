//
//  number literal.cpp
//  STELA
//
//  Created by Indi Kernick on 26/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "number literal.hpp"

#include <limits>
#include <cassert>
#include "Utils/unreachable.hpp"

using namespace stela;

namespace {

// OH FOR FUCK SAKE! I'VE BEEN WAITING FOR STD::FROM_CHARS SINCE 2016!
// @TODO refactor this when we get std::from_chars

struct from_chars_result {
  const char *ptr;
  bool err;
};

template <typename Number>
from_chars_result from_chars(const char *first, const char *last, Number &number, int base = 0) {
  char *end;
  errno = 0;
  assert(first <= last);
  std::string nullTerm(first, static_cast<size_t>(std::min(32l, last - first)));
  if constexpr (std::is_floating_point_v<Number>) {
    const long double ld = std::strtold(nullTerm.c_str(), &end);
    if (errno != 0 || (end == nullTerm.c_str() && ld == 0.0l)) {
      return {first, true};
    }
    using Limits = std::numeric_limits<Number>;
    if (ld < static_cast<long double>(Limits::lowest()) || ld > static_cast<long double>(Limits::max())) {
      return {first, true};
    }
    number = static_cast<Number>(ld);
    return {first + (end - nullTerm.c_str()), false};
  } else if constexpr (std::is_signed_v<Number>) {
    const long long ll = std::strtoll(nullTerm.c_str(), &end, base);
    if (errno != 0 || (end == nullTerm.c_str() && ll == 0)) {
      return {first, true};
    }
    using Limits = std::numeric_limits<Number>;
    if (ll < static_cast<long long>(Limits::lowest()) || ll > static_cast<long long>(Limits::max())) {
      return {first, true};
    }
    number = static_cast<Number>(ll);
    return {first + (end - nullTerm.c_str()), false};
  } else if constexpr (std::is_unsigned_v<Number>) {
    const unsigned long long ull = std::strtoull(nullTerm.c_str(), &end, base);
    if (errno != 0 || (end == nullTerm.c_str() && ull == 0)) {
      return {first, true};
    }
    using Limits = std::numeric_limits<Number>;
    if (ull > static_cast<unsigned long long>(Limits::max())) {
      return {first, true};
    }
    number = static_cast<Number>(ull);
    return {first + (end - nullTerm.c_str()), false};
  }
}

template <typename Dst, typename Src>
bool inRange(const Src num) {
  using Limits = std::numeric_limits<Dst>;
  return static_cast<Src>(Limits::lowest()) <= num &&
         num <= static_cast<Src>(Limits::max()) &&
         num == static_cast<Src>(static_cast<Dst>(num));
}

bool eqEither(const char value, const char a, const char b) {
  return value == a || value == b;
}

template <typename Number>
bool hasValidSuffix(const char suffix, const Number num, const Loc loc, Log &log) {
  if (eqEither(suffix, 'b', 'B')) {
    if (inRange<Byte>(num)) {
      return true;
    } else {
      log.error(loc) << "Number literal cannot be represented as byte" << fatal;
    }
  }
  
  if (eqEither(suffix, 'c', 'C')) {
    if (inRange<Char>(num)) {
      return true;
    } else {
      log.error(loc) << "Number literal cannot be represented as char" << fatal;
    }
  }
  
  if (eqEither(suffix, 'r', 'R')) {
    if (inRange<Real>(num)) {
      return true;
    } else {
      // @TODO this is not reachable
      log.error(loc) << "Number literal cannot be represented as real" << fatal;
    }
  }
  
  if (eqEither(suffix, 's', 'S')) {
    if (inRange<Sint>(num)) {
      return true;
    } else {
      log.error(loc) << "Number literal cannot be represented as sint" << fatal;
    }
  }
  
  if (eqEither(suffix, 'u', 'U')) {
    if (inRange<Uint>(num)) {
      return true;
    } else {
      log.error(loc) << "Number literal cannot be represented as uint" << fatal;
    }
  }
  
  return false;
}

}

size_t stela::validNumberLiteral(const std::string_view str, const Loc loc, Log &log) {
  assert(!str.empty());
  
  // real, uint, sint
  from_chars_result res;
  
  Real real;
  res = from_chars(str.cbegin(), str.cend(), real);
  if (!res.err) {
    assert(str.cbegin() <= res.ptr);
    const size_t size = static_cast<size_t>(res.ptr - str.cbegin());
    if (hasValidSuffix(*res.ptr, real, loc, log)) {
      return size + 1;
    }
    return size;
  }
  
  Uint uint;
  res = from_chars(str.cbegin(), str.cend(), uint);
  if (!res.err) {
    assert(str.cbegin() <= res.ptr);
    const size_t size = static_cast<size_t>(res.ptr - str.cbegin());
    if (hasValidSuffix(*res.ptr, uint, loc, log)) {
      return size + 1;
    }
    return size;
  }
  
  Sint sint;
  res = from_chars(str.cbegin(), str.cend(), sint);
  if (!res.err) {
    assert(str.cbegin() <= res.ptr);
    const size_t size = static_cast<size_t>(res.ptr - str.cbegin());
    if (hasValidSuffix(*res.ptr, sint, loc, log)) {
      return size + 1;
    }
    return size;
  }
  
  return 0;
}

stela::NumberVariant stela::parseNumberLiteral(const std::string_view str, Log &) {
  assert(!str.empty());
  
  if (eqEither(str.back(), 'b', 'B')) {
    Byte byte;
    from_chars(str.cbegin(), str.cend(), byte);
    return byte;
  }
  
  if (eqEither(str.back(), 'c', 'C')) {
    Char ch;
    from_chars(str.cbegin(), str.cend(), ch);
    return ch;
  }
  
  if (eqEither(str.back(), 'r', 'R')) {
    Real real;
    from_chars(str.cbegin(), str.cend(), real);
    return real;
  }
  
  if (eqEither(str.back(), 's', 'S')) {
    Sint sint;
    from_chars(str.cbegin(), str.cend(), sint);
    return sint;
  }
  
  if (eqEither(str.back(), 'u', 'U')) {
    Uint uint;
    from_chars(str.cbegin(), str.cend(), uint);
    return uint;
  }
  
  // sint, uint, real
  from_chars_result res;
  
  Sint sint;
  res = from_chars(str.cbegin(), str.cend(), sint);
  if (res.ptr == str.cend()) {
    return sint;
  }
  
  Uint uint;
  res = from_chars(str.cbegin(), str.cend(), uint);
  if (res.ptr == str.cend()) {
    return uint;
  }
  
  Real real;
  res = from_chars(str.cbegin(), str.cend(), real);
  if (res.ptr == str.cend()) {
    return real;
  }
  
  UNREACHABLE();
}

namespace {

using Iter = std::string_view::const_iterator;

bool isOctDigit(const char c) {
  return std::isdigit(c) && c != '8' && c != '9';
}

bool isHexDigit(const char c) {
  return std::isxdigit(c);
}

char parseOctChar(Iter &ch, Iter end, Loc loc, Log &log) {
  Iter octEnd = ch;
  while (octEnd != end && isOctDigit(*octEnd)) {
    ++octEnd;
  }
  long long charCode;
  const auto [ptr, err] = from_chars(ch, octEnd, charCode, 8);
  if (err || !inRange<Char>(charCode)) {
    log.error(loc) << "Octal sequence out of range" << fatal;
  }
  ch = std::prev(octEnd);
  return static_cast<char>(charCode);
}

char parseHexChar(Iter &ch, Iter end, Loc loc, Log &log) {
  Iter hexEnd = ch;
  while (hexEnd != end && isHexDigit(*hexEnd)) {
    ++hexEnd;
  }
  long long charCode;
  const auto [ptr, err] = from_chars(ch, hexEnd, charCode, 16);
  if (err || !inRange<Char>(charCode)) {
    log.error(loc) << "Hex sequence out of range" << fatal;
  }
  ch = std::prev(hexEnd);
  return static_cast<char>(charCode);
}

}

std::string stela::parseStringLiteral(const std::string_view lit, Loc loc, Log &log) {
  std::string str;
  bool prevWasBack = false;
  for (auto ch = lit.cbegin(); ch != lit.cend(); ++ch) {
    const char c = *ch;
    if (prevWasBack) {
      if (c == '\'' || c == '\"' || c == '?' || c == '\\') {
        str += c;
      } else if (c == 'a') {
        str += '\a';
      } else if (c == 'b') {
        str += '\b';
      } else if (c == 'f') {
        str += '\f';
      } else if (c == 'n') {
        str += '\n';
      } else if (c == 'r') {
        str += '\r';
      } else if (c == 't') {
        str += '\t';
      } else if (c == 'v') {
        str += '\v';
      } else if (isOctDigit(c)) {
        str += parseOctChar(ch, lit.cend(), loc, log);
      } else if (c == 'x') {
        ++ch;
        str += parseHexChar(ch, lit.cend(), loc, log);
      }
      prevWasBack = false;
    } else {
      if (c == '\\') {
        prevWasBack = true;
      } else {
        str += c;
      }
    }
  }
  // lexer shouldn't allow this
  assert(!prevWasBack);
  return str;
}
