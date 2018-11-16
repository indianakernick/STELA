//
//  retain ptr.hpp
//  STELA
//
//  Created by Indi Kernick on 16/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_retain_ptr_hpp
#define stela_retain_ptr_hpp

#include <cstdint>
#include <utility>
#include <cassert>

/* LCOV_EXCL_START */

namespace stela {

template <typename T>
struct retain_traits;

template <typename T>
class ref_count {
  unsigned count = 1;

  friend retain_traits<T>;
};

template <typename T>
struct retain_traits {
  using pointer = T *;

  static void increment(ref_count<T> *const ptr) noexcept {
    static_assert(std::is_base_of_v<ref_count<T>, T>);
    assert(ptr->count != ~unsigned{});
    ++ptr->count;
  }
  static void decrement(ref_count<T> *const ptr) noexcept {
    assert(ptr->count != 0);
    --ptr->count;
    if (ptr->count == 0) {
      delete ptr;
    }
  }
  static unsigned use_count(ref_count<T> *const ptr) noexcept {
    return ptr->count;
  }
};

struct retain_t {};
constexpr retain_t retain {};

template <typename T, typename R = retain_traits<T>>
class retain_ptr {
public:
  using element_type = T;
  using traits_type = R;
  using pointer = typename traits_type::pointer;

  constexpr retain_ptr() noexcept
    : ptr{nullptr} {}
  constexpr retain_ptr(std::nullptr_t) noexcept
    : ptr{nullptr} {}
  explicit retain_ptr(const pointer ptr) noexcept
    : ptr{ptr} {}
  retain_ptr(retain_t, const pointer ptr) noexcept(noexcept(incr()))
    : ptr{ptr} {
    incr(ptr);
  }
  ~retain_ptr() noexcept(noexcept(decr())) {
    decr();
  }
  
  retain_ptr &operator=(std::nullptr_t) noexcept(noexcept(reset())) {
    reset();
    return *this;
  }
  retain_ptr(retain_ptr<T, R> &&other) noexcept
    : ptr{other.detach()} {}
  retain_ptr &operator=(retain_ptr<T, R> &&other) noexcept(noexcept(reset())) {
    reset(other.detach());
    return *this;
  }
  retain_ptr(const retain_ptr<T, R> &other) noexcept(noexcept(incr()))
    : ptr{other.get()} {
    incr();
  }
  retain_ptr &operator=(const retain_ptr<T, R> &other) noexcept(noexcept(reset()) && noexcept(incr())) {
    reset(other.get());
    incr();
  }
  
  void reset(const pointer newPtr = nullptr) noexcept(noexcept(decr())) {
    decr();
    ptr = newPtr;
  }
  void reset(retain_t, const pointer newPtr) noexcept(noexcept(decr()) && noexcept(incr())) {
    decr();
    ptr = newPtr;
    incr();
  }
  void swap(retain_ptr<T, R> &other) noexcept {
    std::swap(ptr, other.ptr);
  }
  [[nodiscard]] pointer detach() noexcept {
    return std::exchange(ptr, nullptr);
  }
  
  pointer get() const noexcept {
    return ptr;
  }
  element_type &operator*() const noexcept {
    assert(ptr);
    return *ptr;
  }
  pointer operator->() const noexcept {
    assert(ptr);
    return ptr;
  }
  element_type &operator[](const ptrdiff_t i) const noexcept {
    assert(ptr);
    return ptr[i];
  }
  explicit operator bool() const noexcept {
    return ptr != nullptr;
  }
  
  unsigned use_count() const noexcept(noexcept(traits_type::use_count(nullptr))) {
    if (ptr) {
      return traits_type::use_count(ptr);
    } else {
      return 0;
    }
  }
  bool unique() const noexcept(noexcept(use_count())) {
    return use_count() == 1;
  }
  
  bool operator==(const retain_ptr<T, R> &rhs) const noexcept {
    return ptr == rhs.ptr;
  }
  bool operator!=(const retain_ptr<T, R> &rhs) const noexcept {
    return ptr != rhs.ptr;
  }
  bool operator<(const retain_ptr<T, R> &rhs) const noexcept {
    return ptr < rhs.ptr;
  }
  bool operator>(const retain_ptr<T, R> &rhs) const noexcept {
    return ptr > rhs.ptr;
  }
  bool operator<=(const retain_ptr<T, R> &rhs) const noexcept {
    return ptr <= rhs.ptr;
  }
  bool operator>=(const retain_ptr<T, R> &rhs) const noexcept {
    return ptr >= rhs.ptr;
  }
  
  friend bool operator==(const retain_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr == nullptr;
  }
  friend bool operator!=(const retain_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr != nullptr;
  }
  friend bool operator<(const retain_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr < nullptr;
  }
  friend bool operator>(const retain_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr > nullptr;
  }
  friend bool operator<=(const retain_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr <= nullptr;
  }
  friend bool operator>=(const retain_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr >= nullptr;
  }
  
  friend bool operator==(std::nullptr_t, const retain_ptr<T> &rhs) noexcept {
    return nullptr == rhs.ptr;
  }
  friend bool operator!=(std::nullptr_t, const retain_ptr<T> &rhs) noexcept {
    return nullptr != rhs.ptr;
  }
  friend bool operator<(std::nullptr_t, const retain_ptr<T> &rhs) noexcept {
    return nullptr < rhs.ptr;
  }
  friend bool operator>(std::nullptr_t, const retain_ptr<T> &rhs) noexcept {
    return nullptr > rhs.ptr;
  }
  friend bool operator<=(std::nullptr_t, const retain_ptr<T> &rhs) noexcept {
    return nullptr <= rhs.ptr;
  }
  friend bool operator>=(std::nullptr_t, const retain_ptr<T> &rhs) noexcept {
    return nullptr >= rhs.ptr;
  }

private:
  pointer ptr;
  
  void incr() const noexcept(noexcept(traits_type::increment(nullptr))) {
    if (ptr) {
      traits_type::increment(ptr);
    }
  }
  void decr() const noexcept(noexcept(traits_type::decrement(nullptr))) {
    if (ptr) {
      traits_type::decrement(ptr);
    }
  }
};

template <typename T>
void swap(retain_ptr<T> &a, retain_ptr<T> &b) {
  a.swap(b);
}

template <typename T, typename... Args>
retain_ptr<T> make_retain(Args &&... args) noexcept {
  return retain_ptr<T>{new T (std::forward<Args>(args)...)};
}

}

template <typename T>
struct std::hash<retain_ptr<T>> {
  size_t operator()(const retain_ptr<T> &ptr) const noexcept {
    return reinterpret_cast<size_t>(ptr.get());
  }
};

/* LCOV_EXCL_END */

#endif
