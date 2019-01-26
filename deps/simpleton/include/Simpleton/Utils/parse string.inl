//
//  parse string.inl
//  Simpleton Engine
//
//  Created by Indi Kernick on 30/9/17.
//  Copyright © 2017 Indi Kernick. All rights reserved.
//

#include <cstring>
#include <cassert>
#include <functional>

namespace Utils {
  auto equal_to(const char ch) {
    return [ch] (const char c) {
      return c == ch;
    };
  }
}

inline Utils::ParseString::ParseString(const std::string_view view)
  : beg{view.data()}, end{view.data() + view.size()} {
  assert(view.data());
}

inline Utils::ParseString::ParseString(const char *const data, const size_t size)
  : beg{data}, end{data + size} {
  assert(data);
}

inline Utils::LineCol<> Utils::ParseString::lineCol() const {
  return pos;
}

inline std::string_view Utils::ParseString::view() const {
  assert(beg <= end);
  return {beg, static_cast<size_t>(end - beg)};
}

inline bool Utils::ParseString::empty() const {
  return beg == end;
}

inline Utils::ParseString &Utils::ParseString::advance(const size_t numChars) {
  if (beg + numChars > end) {
    throw std::out_of_range("Advanced parse string too many characters");
  }
  advanceNoCheck(numChars);
  return *this;
}

inline Utils::ParseString &Utils::ParseString::advance() {
  if (beg + 1 > end) {
    throw std::out_of_range("Advanced parse string too many characters");
  }
  advanceNoCheck();
  return *this;
}

template <typename Pred>
Utils::ParseString &Utils::ParseString::skip(Pred &&pred) {
  while (beg < end && pred(*beg)) {
    advanceNoCheck();
  }
  return *this;
}

inline Utils::ParseString &Utils::ParseString::skipWhitespace() {
  return skip(isspace);
}

inline Utils::ParseString &Utils::ParseString::skipUntil(const char c) {
  return skipUntil(equal_to(c));
}

template <typename Pred>
Utils::ParseString &Utils::ParseString::skipUntil(Pred &&pred) {
  return skip(std::not_fn(pred));
}

inline bool Utils::ParseString::check(const char c) {
  if (empty() || *beg != c) {
    return false;
  } else {
    advanceNoCheck();
    return true;
  }
}

inline bool Utils::ParseString::check(const char *data, const size_t size) {
  if (size == 0) {
    return true;
  }
  if (end < beg + size) {
    return false;
  }
  if (std::memcmp(beg, data, size) == 0) {
    advanceNoCheck(size);
    return true;
  } else {
    return false;
  }
}

inline bool Utils::ParseString::check(const std::string_view view) {
  return check(view.data(), view.size());
}

template <size_t SIZE>
bool Utils::ParseString::check(const char (&string)[SIZE]) {
  return check(string, SIZE - 1);
}

template <typename Pred>
bool Utils::ParseString::check(Pred &&pred) {
  if (empty() || !pred(*beg)) {
    return false;
  } else {
    advanceNoCheck();
    return true;
  }
}

inline size_t Utils::ParseString::tryParseEnum(
  const std::string_view *const names,
  const size_t numNames
) {
  assert(names);
  const std::string_view *const namesEnd = names + numNames;
  for (const std::string_view *n = names; n != namesEnd; ++n) {
    if (check(*n)) {
      return static_cast<size_t>(n - names);
    }
  }
  return numNames;
}

template <typename Pred>
size_t Utils::ParseString::tryParseEnum(
  const std::string_view *const names,
  const size_t numNames,
  Pred &&pred
) {
  assert(names);
  const std::string_view *const namesEnd = names + numNames;
  for (const std::string_view *n = names; n != namesEnd; ++n) {
    if (end < beg + n->size()) {
      continue;
    }
    if (n->size() == 0 && (beg == end || pred(*beg))) {
      return static_cast<size_t>(n - names);
    }
    if (beg + n->size() == end || pred(beg[n->size()])) {
      if (std::memcmp(beg, n->data(), n->size()) == 0) {
        advanceNoCheck(n->size());
        return static_cast<size_t>(n - names);
      }
    }
  }
  return numNames;
}

inline std::string_view Utils::ParseString::beginViewing() const {
  return {beg, 0};
}

inline void Utils::ParseString::endViewing(std::string_view &view) const {
  view = {view.data(), static_cast<size_t>(beg - view.data())};
}

inline void Utils::ParseString::advanceNoCheck(const size_t numChars) {
  pos.putString(beg, numChars);
  beg += numChars;
}

inline void Utils::ParseString::advanceNoCheck() {
  pos.putChar(*beg);
  ++beg;
}
