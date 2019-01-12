//
//  builtin code.cpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "builtin code.hpp"

constexpr const char *builtinCode = R"delimiter(

// BEGIN BUILTIN LIBRARY

#include <new>
#include <cmath>
#include <cstdio>
#include <memory>
#include <climits>
#include <cstdint>
#include <utility>
#include <cassert>
#include <cstddef>
#include <algorithm>

#define LIKELY(EXP) __builtin_expect(!!(EXP), 1)
#define UNLIKELY(EXP) __builtin_expect(!!(EXP), 0)

namespace {

using t_void = void;
using t_bool = bool;
using t_byte = int8_t;
using t_char = char;
using t_real = float;
using t_sint = int32_t;
using t_uint = uint32_t;

/*[[noreturn]] void panic(const char *const reason) noexcept {
  std::puts(reason);
  std::exit(EXIT_FAILURE);
}

void *allocate(const size_t bytes) noexcept {
  void *const ptr = operator new(bytes, std::nothrow_t{});
  if (UNLIKELY(ptr == nullptr)) {
    panic("Out of memory");
  } else {
    return ptr;
  }
}

void deallocate(void *const ptr) noexcept {
  operator delete(ptr);
}

struct ref_count {
  t_uint count;
};

template <typename T>
class retain_ptr {
public:
  using pointer = T *;

  retain_ptr() noexcept
    : ptr{nullptr} {}
  
  explicit retain_ptr(T *const ptr) noexcept
    : ptr{ptr} {}
  
  ~retain_ptr() noexcept {
    decr();
  }
  
  retain_ptr(const retain_ptr<T> &other) noexcept
    : ptr{other.get()} {
    incr();
  }
  retain_ptr &operator=(const retain_ptr<T> &other) noexcept {
    reset(other.get());
    incr();
    return *this;
  }
  
  void reset(T *const newPtr = nullptr) noexcept {
    decr();
    ptr = newPtr;
  }
  
  pointer get() const noexcept {
    return ptr;
  }
  pointer operator->() const noexcept {
    assert(ptr);
    return ptr;
  }
  explicit operator bool() const noexcept {
    return ptr != nullptr;
  }

private:
  pointer ptr;
  
  void incr() const noexcept {
    ref_count *const refPtr = ptr;
    assert(refPtr->count != ~t_uint{});
    ++refPtr->count;
  }
  
  void decr() const noexcept {
    ref_count *const refPtr = ptr;
    assert(refPtr->count != 0);
    if (--refPtr->count == 0) {
      deallocate(ptr);
    }
  }
};*/

/*#define BITS(T) static_cast<T>(sizeof(T) * CHAR_BIT)

unsigned long long ceilToPowerOf2(const unsigned long long num) {
  assert(num != 0);
  return (1ull << (BITS(unsigned long long) - static_cast<unsigned long long>(__builtin_clzll(num - 1ull)))) - (num == 1ull);
}
unsigned long ceilToPowerOf2(const unsigned long num) {
  assert(num != 0);
  return (1ul << (BITS(unsigned long) - static_cast<unsigned long>(__builtin_clzl(num - 1ul)))) - (num == 1ul);
}
unsigned ceilToPowerOf2(const unsigned num) {
  assert(num != 0);
  return (1u << (BITS(unsigned) - static_cast<unsigned>(__builtin_clz(num - 1u)))) - (num == 1u);
}

#undef BITS*/

template <typename T>
struct Array {
  T *data;
  t_uint cap;
  t_uint len;
  
  Array(T *const data, const t_uint cap, const t_uint len)
    : data{data}, cap{cap}, len{len} {}
  Array &operator=(const Array<T> &other) noexcept {
    if (cap < other.len) {
      std::destroy_n(data, len);
      deallocate(data);
      cap = ceilToPowerOf2(other.len);
      data = static_cast<T *>(allocate(cap));
      std::uninitialized_copy_n(other.data, other.len, data);
    } else if (len < other.len) {
      std::copy_n(other.data, len, data);
      std::uninitialized_copy_n(other.data + len, other.len - len, data + len);
    } else {
      std::copy_n(other.data, other.len, data);
      std::destroy_n(data + other.len, other.len - len);
    }
    len = other.len;
    return *this;
  }
  Array(const Array<T> &other) noexcept {
    len = other.len;
    cap = ceilToPowerOf2(len);
    data = static_cast<T *>(allocate(sizeof(T) * cap));
    std::uninitialized_copy_n(other.data, other.len, data);
  }
  ~Array() noexcept {
    std::destroy_n(data, len);
    deallocate(data);
  }
};

/*template <typename T>
Array<T> make_array(const t_uint cap, const t_uint len) noexcept {
  return {static_cast<T *>(allocate(cap * sizeof(T))), cap, len};
}

template <typename T>
Array<T> make_array_len(const t_uint len) noexcept {
  return make_array<T>(ceilToPowerOf2(len), len);
}

template <typename T>
Array<T> make_null_array() noexcept {
  return make_array<T>(0, 0);
}

template <typename T, typename... Args>
Array<T> array_literal(Args &&... args) noexcept {
  Array<T> array = make_array_len<T>(sizeof...(Args));
  const T temp[] = {std::forward<Args>(args)...};
  std::uninitialized_copy_n(&temp[0], sizeof...(Args), array.data);
  return array;
}

Array<t_char> make_null_string() noexcept {
  return make_array<t_char>(0, 0);
}

template <size_t Size>
Array<t_char> string_literal(const t_char (&string)[Size]) noexcept {
  Array<t_char> array = make_array_len<t_char>(Size - 1);
  std::uninitialized_copy_n(&string[0], Size - 1, array.data);
  return array;
}*/

/*template <typename T>
t_uint capacity(const Array<T> &array) noexcept {
  return array.cap;
}

template <typename T>
t_uint size(const Array<T> &array) noexcept {
  return array.len;
}*/

/*template <typename T>
void reallocate(Array<T> &array, const t_uint cap) noexcept {
  T *newData = static_cast<T *>(allocate(sizeof(T) * cap));
  std::uninitialized_move_n(array.data, array.len, newData);
  std::destroy_n(array.data, array.len);
  deallocate(array.data);
  array.data = newData;
  array.cap = cap;
}*/

template <typename T>
void push_back(Array<T> &array, const T value) noexcept {
  if (array.len == array.cap) {
    reallocate(array, array.cap * 2);
  }
  new (array.data + array.len) T(value);
  ++array.len;
}

template <typename T>
void append(Array<T> &array, const Array<T> &other) noexcept {
  if (array.len + other.len > array.cap) {
    reallocate(array, ceilToPowerOf2(array.len + other.len));
  }
  std::uninitialized_copy_n(other.data, other.len, array.data + array.len);
  array.len += other.len;
}

/*template <typename T>
void pop_back(Array<T> &array) noexcept {
  if (UNLIKELY(array.len == 0)) {
    panic("pop_back from empty array");
  } else {
    --array.len;
    std::destroy_at(array.data + array.len);
  }
}*/

template <typename T>
void resize(Array<T> &array, const t_uint size) noexcept {
  assert(array);
  if (size <= array.len) {
    std::destroy_n(array.data + size, array.len - size);
  } else if (size <= array.cap) {
    std::uninitialized_value_construct_n(array.data + array.len, size - array.len);
  } else {
    reallocate(array, ceilToPowerOf2(size));
    std::uninitialized_value_construct_n(array.data + array.len, size - array.len);
  }
  array.len = size;
}

template <typename T>
void reserve(Array<T> &array, const t_uint cap) noexcept {
  if (cap > array.cap) {
    reallocate(array, cap);
  }
}

/*template <typename T>
T &index(Array<T> &array, const t_uint index) noexcept {
  if (UNLIKELY(index >= array.len)) {
    panic("Array index out of bounds");
  } else {
    return array.data[index];
  }
}

template <typename T>
T &index(Array<T> &array, const t_sint index) noexcept {
  if (UNLIKELY(index < t_sint{0} || index >= array.len)) {
    panic("Array index out of bounds");
  } else {
    return array.data[index];
  }
}*/

struct ClosureData : ref_count {
  virtual ~ClosureData() = default;
};

using ClosureDataPtr = retain_ptr<ClosureData>;

struct FuncClosureData : ClosureData {};

}

// END BUILTIN LIBRARY

)delimiter";
