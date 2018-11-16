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

struct no_ref_incr_t {};
constexpr no_ref_incr_t no_ref_incr {};

template <typename T>
class intrusive_ptr {
public:
  constexpr intrusive_ptr() noexcept
    : ptr{nullptr} {}
  constexpr intrusive_ptr(std::nullptr_t) noexcept
    : ptr{nullptr} {}
  template <typename U>
  explicit intrusive_ptr(U *const ptr) noexcept
    : ptr{ptr} {
    incr();
  }
  template <typename U>
  explicit intrusive_ptr(no_ref_incr_t, U *const ptr) noexcept
    : ptr{ptr} {}
  ~intrusive_ptr() noexcept {
    decr();
  }
  
  template <typename U>
  intrusive_ptr(intrusive_ptr<U> &&other) noexcept
    : ptr{other.detach()} {}
  template <typename U>
  intrusive_ptr &operator=(intrusive_ptr<U> &&other) noexcept {
    reset(other.detach());
    return *this;
  }
  template <typename U>
  intrusive_ptr(const intrusive_ptr<U> &other) noexcept
    : ptr{other.get()} {
    incr();
  }
  template <typename U>
  intrusive_ptr &operator=(const intrusive_ptr<U> &other) noexcept {
    reset(other.get());
    incr();
  }
  
  template <typename U = T>
  void reset(U *const newPtr = nullptr) noexcept {
    decr();
    ptr = newPtr;
  }
  void swap(intrusive_ptr<T> &other) noexcept {
    std::swap(ptr, other.ptr);
  }
  void detach() noexcept {
    return std::exchange(ptr, nullptr);
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
  T &operator[](const ptrdiff_t i) const noexcept {
    assert(ptr);
    return ptr[i];
  }
  explicit operator bool() const noexcept {
    return ptr != nullptr;
  }
  
  template <typename U>
  bool operator==(const intrusive_ptr<U> &rhs) const noexcept {
    return ptr == rhs.ptr;
  }
  template <typename U>
  bool operator!=(const intrusive_ptr<U> &rhs) const noexcept {
    return ptr != rhs.ptr;
  }
  template <typename U>
  bool operator<(const intrusive_ptr<U> &rhs) const noexcept {
    return ptr < rhs.ptr;
  }
  template <typename U>
  bool operator>(const intrusive_ptr<U> &rhs) const noexcept {
    return ptr > rhs.ptr;
  }
  template <typename U>
  bool operator<=(const intrusive_ptr<U> &rhs) const noexcept {
    return ptr <= rhs.ptr;
  }
  template <typename U>
  bool operator>=(const intrusive_ptr<U> &rhs) const noexcept {
    return ptr >= rhs.ptr;
  }
  
  friend bool operator==(const intrusive_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr == nullptr;
  }
  friend bool operator!=(const intrusive_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr != nullptr;
  }
  friend bool operator<(const intrusive_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr < nullptr;
  }
  friend bool operator>(const intrusive_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr > nullptr;
  }
  friend bool operator<=(const intrusive_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr <= nullptr;
  }
  friend bool operator>=(const intrusive_ptr<T> &lhs, std::nullptr_t) noexcept {
    return lhs.ptr >= nullptr;
  }
  
  friend bool operator==(std::nullptr_t, const intrusive_ptr<T> &rhs) noexcept {
    return nullptr == rhs.ptr;
  }
  friend bool operator!=(std::nullptr_t, const intrusive_ptr<T> &rhs) noexcept {
    return nullptr != rhs.ptr;
  }
  friend bool operator<(std::nullptr_t, const intrusive_ptr<T> &rhs) noexcept {
    return nullptr < rhs.ptr;
  }
  friend bool operator>(std::nullptr_t, const intrusive_ptr<T> &rhs) noexcept {
    return nullptr > rhs.ptr;
  }
  friend bool operator<=(std::nullptr_t, const intrusive_ptr<T> &rhs) noexcept {
    return nullptr <= rhs.ptr;
  }
  friend bool operator>=(std::nullptr_t, const intrusive_ptr<T> &rhs) noexcept {
    return nullptr >= rhs.ptr;
  }

private:
  T *ptr;
  
  void incr() const noexcept {
    if (ptr) {
      intrusive_incr(ptr);
    }
  }
  void decr() const noexcept {
    if (ptr) {
      intrusive_decr(ptr);
    }
  }
};

template <typename T>
void swap(intrusive_ptr<T> &a, intrusive_ptr<T> &b) {
  a.swap(b);
}

template <typename T, typename... Args>
intrusive_ptr<T> make_intrusive(Args &&... args) noexcept {
  return intrusive_ptr<T>{new T (std::forward<Args>(args)...)};
}

}

template <typename T>
struct std::hash<intrusive_ptr<T>> {
  size_t operator()(const intrusive_ptr<T> &ptr) const noexcept {
    return reinterpret_cast<size_t>(ptr.get());
  }
};

namespace stela {

template <typename T>
class ref_count;

template <typename T>
void intrusive_incr(ref_count<T> *const ptr) noexcept {
  static_assert(std::is_base_of_v<ref_count<T>, T>);
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
