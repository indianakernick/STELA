//
//  intrusive ptr.hpp
//  STELA
//
//  Created by Indi Kernick on 16/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_intrusive_ptr_hpp
#define stela_intrusive_ptr_hpp

#include <cstdint>
#include <utility>
#include <cassert>

namespace stela {

template <typename T>
class intrusive_ptr {
public:
  using element_type = T;

  constexpr intrusive_ptr() noexcept
    : ptr{nullptr} {}
  constexpr intrusive_ptr(std::nullptr_t) noexcept
    : ptr{nullptr} {}
  explicit intrusive_ptr(T *ptr) noexcept
    : ptr{ptr} {
    if (ptr) {
      intrusive_incr(ptr);
    }
  }
  ~intrusive_ptr() noexcept {
    if (ptr) {
      intrusive_decr(ptr);
    }
  }
  
  intrusive_ptr(intrusive_ptr<T> &&other) noexcept
    : ptr{std::exchange(other.ptr, nullptr)} {}
  intrusive_ptr &operator=(intrusive_ptr<T> &&other) noexcept {
    reset(std::exchange(other.ptr, nullptr));
    return *this;
  }
  intrusive_ptr(const intrusive_ptr<T> &other) noexcept
    : ptr{other.ptr} {
    if (ptr) {
      intrusive_incr(ptr);
    }
  }
  intrusive_ptr &operator=(const intrusive_ptr<T> &other) noexcept {
    reset(other.ptr);
    if (ptr) {
      intrusive_incr(ptr);
    }
  }
  
  void reset(T *const newPtr = nullptr) noexcept {
    if (ptr) {
      intrusive_decr(ptr);
    }
    ptr = newPtr;
  }
  void swap(intrusive_ptr<T> &other) noexcept {
    std::swap(ptr, other.ptr);
  }
  
  T *get() const noexcept {
    return ptr;
  }
  T &operator*() const noexcept {
    assert(ptr);
    return *ptr;
  }
  T *operator->() const noexcept {
    assert(ptr);
    return ptr;
  }
  explicit operator bool() const noexcept {
    return ptr;
  }

private:
  T *ptr;
};

template <typename T, typename... Args>
intrusive_ptr<T> make_intrusive(Args &&... args) noexcept {
  return intrusive_ptr<T>{new T (std::forward<Args>(args)...)};
}

template <typename T>
class ref_count;

template <typename T>
void intrusive_incr(ref_count<T> *const ptr) noexcept {
  assert(ptr->count != ~uint8_t{});
  ++ptr->count;
}

template <typename T>
void intrusive_decr(ref_count<T> *const ptr) noexcept {
  assert(ptr->count != 0);
  --ptr->count;
  if (ptr->count == 0) {
    delete ptr;
  }
}

template <typename T>
class ref_count {
  uint8_t count = 0;

  friend void intrusive_incr<T>(ref_count<T> *) noexcept;
  friend void intrusive_decr<T>(ref_count<T> *) noexcept;
};

}

#endif
