//
//  iterator range.hpp
//  STELA
//
//  Created by Indi Kernick on 7/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_iterator_range_hpp
#define stela_iterator_range_hpp

#include <iterator>

namespace stela::detail {

template <typename Iter>
class IteratorWrapper {
public:
  constexpr explicit IteratorWrapper(const Iter iter)
    : iter{iter} {}
  
  constexpr IteratorWrapper &operator++() noexcept {
    ++iter;
    return *this;
  }
  constexpr Iter operator*() const noexcept {
    return iter;
  }
  constexpr bool operator!=(const IteratorWrapper &other) const noexcept {
    return iter != other.iter;
  }

private:
  Iter iter;
};

template <typename Iter>
class Range {
public:
  constexpr Range(const Iter begin, const Iter end)
    : begin_{begin}, end_{end} {}

  constexpr Iter begin() const noexcept {
    return begin_;
  }
  constexpr Iter end() const noexcept {
    return end_;
  }

private:
  const Iter begin_;
  const Iter end_;
};

template <typename Iter>
auto make_iter_range(const Iter begin, const Iter end) {
  return Range{IteratorWrapper{begin}, IteratorWrapper{end}};
}

}

namespace stela {

template <typename Container>
constexpr auto iter_range(Container &c) noexcept {
  return detail::make_iter_range(std::begin(c), std::end(c));
}

template <typename Container>
constexpr auto citer_range(Container &c) noexcept {
  return detail::make_iter_range(std::cbegin(c), std::cend(c));
}

}

#endif
