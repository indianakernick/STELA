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
#if 1

#include <memory>

namespace stela {

namespace ast {

struct Node;

}

using ref_count = std::enable_shared_from_this<ast::Node>;

struct retain_t {};
constexpr retain_t retain {};

template <typename T>
class retain_ptr : public std::shared_ptr<T> {
public:
  constexpr retain_ptr() noexcept
    : std::shared_ptr<T>{} {}
  constexpr retain_ptr(std::nullptr_t) noexcept
    : std::shared_ptr<T>{nullptr} {}
  
  template <typename U>
  retain_ptr(retain_t, U *const ptr)
    : std::shared_ptr<T>{std::dynamic_pointer_cast<T>(ptr->shared_from_this())} {
    assert(*this);
  }
  
  retain_ptr &operator=(std::nullptr_t) noexcept {
    std::shared_ptr<T>::operator=(nullptr);
    return *this;
  }
  
  // Move constructors
  template <typename U>
  retain_ptr(retain_ptr<U> &&other) noexcept
    : std::shared_ptr<T>{std::move(other)} {}
  retain_ptr(retain_ptr<T> &&other)
    : std::shared_ptr<T>{std::move(other)} {}
  template <typename U>
  retain_ptr(std::shared_ptr<U> &&other) noexcept
    : std::shared_ptr<T>{std::move(other)} {}
  retain_ptr(std::shared_ptr<T> &&other)
    : std::shared_ptr<T>{std::move(other)} {}
  
  // Move assignment
  template <typename U>
  retain_ptr &operator=(retain_ptr<U> &&other) noexcept {
    std::shared_ptr<T>::operator=(std::move(other));
    return *this;
  }
  retain_ptr &operator=(retain_ptr<T> &&other) noexcept {
    std::shared_ptr<T>::operator=(std::move(other));
    return *this;
  }
  
  // Copy constructors
  template <typename U>
  retain_ptr(const retain_ptr<U> &other)
    : std::shared_ptr<T>{other} {}
  retain_ptr(const retain_ptr<T> &other)
    : std::shared_ptr<T>{other} {}
  template <typename U>
  retain_ptr(const std::shared_ptr<U> &other)
    : std::shared_ptr<T>{other} {}
  retain_ptr(const std::shared_ptr<T> &other)
    : std::shared_ptr<T>{other} {}
  
  // Copy assignment
  template <typename U>
  retain_ptr &operator=(const retain_ptr<U> &other) noexcept {
    std::shared_ptr<T>::operator=(other);
    return *this;
  }
  retain_ptr &operator=(const retain_ptr<T> &other) noexcept {
    std::shared_ptr<T>::operator=(other);
    return *this;
  }
};

template <typename T, typename... Args>
auto make_retain(Args &&... args) noexcept {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename Derived, typename Base>
retain_ptr<Derived> dynamic_pointer_cast(const retain_ptr<Base> &base) {
  return std::dynamic_pointer_cast<Derived>(base);
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
    : ptr{ptr} {
    incr();
  }
  
  ~retain_ptr() noexcept {
    decr();
  }
  retain_ptr &operator=(std::nullptr_t) noexcept {
    reset();
    return *this;
  }
  
  // Move constructors
  template <typename U>
  retain_ptr(retain_ptr<U> &&other) noexcept
    : ptr{other.detach()} {}
  retain_ptr(retain_ptr<T> &&other) noexcept
    : ptr{other.detach()} {}
  
  // Move assignment
  template <typename U>
  retain_ptr &operator=(retain_ptr<U> &&other) noexcept {
    reset(other.detach());
    return *this;
  }
  retain_ptr &operator=(retain_ptr<T> &&other) noexcept {
    reset(other.detach());
    return *this;
  }
  
  // Copy constructors
  template <typename U>
  retain_ptr(const retain_ptr<U> &other) noexcept
    : ptr{other.get()} {
    incr();
  }
  retain_ptr(const retain_ptr<T> &other) noexcept
    : ptr{other.get()} {
    incr();
  }
  
  // Copy assignment
  template <typename U>
  retain_ptr &operator=(const retain_ptr<U> &other) noexcept {
    reset(other.get());
    incr();
    return *this;
  }
  retain_ptr &operator=(const retain_ptr<T> &other) noexcept {
    reset(other.get());
    incr();
    return *this;
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
  
  uint32_t use_count() const noexcept {
    if (ptr) {
      ref_count *const refPtr = ptr;
      return refPtr->count;
    } else {
      return 0;
    }
  }
  bool unique() const noexcept {
    return use_count() == 1;
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
      ref_count *const refPtr = ptr;
      assert(refPtr->count != ~uint32_t{});
      ++refPtr->count;
    }
  }
  
  void decr() const noexcept {
    if (ptr) {
      ref_count *const refPtr = ptr;
      assert(refPtr->count != 0);
      if (--refPtr->count == 0) {
        delete ptr;
      }
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

template <typename Derived, typename Base>
retain_ptr<Derived> dynamic_pointer_cast(const retain_ptr<Base> &base) {
  return retain_ptr<Derived>(retain, dynamic_cast<Derived *>(base.get()));
}

template <typename Derived, typename Base>
retain_ptr<Derived> dynamic_pointer_cast(retain_ptr<Base> &&base) {
  return retain_ptr<Derived>(dynamic_cast<Derived *>(base.detach()));
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
