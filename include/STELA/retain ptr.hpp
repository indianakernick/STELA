//
//  retain ptr.hpp
//  STELA
//
//  Created by Indi Kernick on 16/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_retain_ptr_hpp
#define stela_retain_ptr_hpp

/* LCOV_EXCL_START */

// switch out stela::retain_ptr for std::shared_ptr
#if 0

#include <memory>

namespace stela {

struct ref_count {};

template <typename T>
using retain_ptr = std::shared_ptr<T>;

template <typename T, typename... Args>
auto make_retain(Args &&... args) noexcept {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

}

#else

#include <cstdint>
#include <utility>
#include <cassert>

namespace stela {

template <typename T>
class retain_ptr;

struct ref_count {
  template <typename T>
  friend class retain_ptr;
  
protected:
  ref_count() = default;

private:
  uint32_t count = 1;
};

struct retain_t {};
constexpr retain_t retain {};

template <typename T>
class retain_ptr {
public:
  using element_type = T;
  using pointer = T *;

  constexpr retain_ptr() noexcept
    : ptr{nullptr} {}
  constexpr retain_ptr(std::nullptr_t) noexcept
    : ptr{nullptr} {}
  
  template <typename U>
  explicit retain_ptr(U *const ptr) noexcept
    : ptr{ptr} {}
  template <typename U>
  retain_ptr(retain_t, U *const ptr) noexcept
    : ptr{ptr} {}
  
  ~retain_ptr() noexcept {
    decr();
  }
  retain_ptr &operator=(std::nullptr_t) noexcept {
    reset();
    return *this;
  }
  
  template <typename U>
  retain_ptr(retain_ptr<U> &&other) noexcept
    : ptr{other.detach()} {}
  template <typename U>
  retain_ptr &operator=(retain_ptr<U> &&other) noexcept {
    reset(other.detach());
    return *this;
  }
  
  template <typename U>
  retain_ptr(const retain_ptr<U> &other) noexcept
    : ptr{other.get()} {
    incr();
  }
  template <typename U>
  retain_ptr &operator=(const retain_ptr<U> &other) noexcept {
    reset(other.get());
    incr();
  }
  
  template <typename U = T>
  void reset(U *const newPtr = nullptr) noexcept {
    decr();
    ptr = newPtr;
  }
  template <typename U = T>
  void reset(retain_t, U *const newPtr) noexcept {
    decr();
    ptr = newPtr;
    incr();
  }
  
  void swap(retain_ptr<T> &other) noexcept {
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
  explicit operator bool() const noexcept {
    return ptr != nullptr;
  }
  
  bool operator==(const retain_ptr<T> &rhs) const noexcept {
    return ptr == rhs.ptr;
  }
  bool operator!=(const retain_ptr<T> &rhs) const noexcept {
    return ptr != rhs.ptr;
  }
  bool operator<(const retain_ptr<T> &rhs) const noexcept {
    return ptr < rhs.ptr;
  }
  bool operator>(const retain_ptr<T> &rhs) const noexcept {
    return ptr > rhs.ptr;
  }
  bool operator<=(const retain_ptr<T> &rhs) const noexcept {
    return ptr <= rhs.ptr;
  }
  bool operator>=(const retain_ptr<T> &rhs) const noexcept {
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
  
  void incr() const noexcept {
    if (ptr) {
      incr(ptr);
    }
  }
  static void incr(ref_count *const ptr) noexcept {
    assert(ptr->count != ~uint32_t{});
    ++ptr->count;
  }
  void decr() const noexcept {
    if (ptr) {
      decr(ptr);
    }
  }
  static void decr(ref_count *const ptr) noexcept {
    assert(ptr->count != 0);
    if (--ptr->count == 0) {
      delete static_cast<T *>(ptr);
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
struct std::hash<stela::retain_ptr<T>> {
  size_t operator()(const stela::retain_ptr<T> &ptr) const noexcept {
    return reinterpret_cast<size_t>(ptr.get());
  }
};

#endif

/* LCOV_EXCL_END */

#endif
