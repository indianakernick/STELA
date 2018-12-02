//
//  builtin code.cpp
//  STELA
//
//  Created by Indi Kernick on 29/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "builtin code.hpp"

constexpr std::string_view builtinCode = R"delimiter(

// BEGIN BUILTIN LIBRARY

#include <new>
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

[[noreturn]] void panic(const char *const reason) noexcept {
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
};

#define BITS(T) static_cast<T>(sizeof(T) * CHAR_BIT)

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

#undef BITS

template <typename T>
struct Array : ref_count {
  t_uint cap;
  t_uint len;
  t_byte dat[];
  
  T *data() noexcept {
    return reinterpret_cast<T *>(&dat);
  }
  const T *data() const noexcept {
    return reinterpret_cast<const T *>(&dat);
  }
  
  ~Array() {
    std::destroy_n(data(), len);
  }
};

template <typename T>
using ArrayPtr = retain_ptr<Array<T>>;

template <typename T>
ArrayPtr<T> make_array(const t_uint cap, const t_uint len) noexcept {
  auto *ptr = static_cast<Array<T> *>(allocate(sizeof(Array<T>) + cap * sizeof(T)));
  ptr->count = 1;
  ptr->cap = cap;
  ptr->len = len;
  return ArrayPtr<T>{ptr};
}

template <typename T>
ArrayPtr<T> make_array_len(const t_uint len) noexcept {
  return make_array<T>(ceilToPowerOf2(len), len);
}

template <typename T>
ArrayPtr<T> make_null_array() noexcept {
  return make_array<T>(0, 0);
}

template <typename T, typename... Args>
ArrayPtr<T> array_literal(Args &&... args) noexcept {
  ArrayPtr<T> array = make_array_len<T>(sizeof...(Args));
  const T temp[] = {std::forward<Args>(args)...};
  std::uninitialized_copy_n(&temp[0], sizeof...(Args), array->data());
  return array;
}

ArrayPtr<t_char> make_null_string() noexcept {
  return make_array<t_char>(0, 0);
}

template <size_t Size>
ArrayPtr<t_char> string_literal(const t_char (&string)[Size]) noexcept {
  ArrayPtr<t_char> array = make_array_len<t_char>(Size - 1);
  std::uninitialized_copy_n(&string[0], Size - 1, array->data());
  return array;
}

template <typename T>
ArrayPtr<T> duplicate(ArrayPtr<T> array) noexcept {
  assert(array);
  ArrayPtr<T> newArray = make_array_len<T>(array->len);
  std::uninitialized_copy_n(array->data(), array->len, newArray->data());
  return newArray;
}

template <typename T>
t_uint capacity(ArrayPtr<T> array) noexcept {
  assert(array);
  return array->cap;
}

template <typename T>
t_uint size(ArrayPtr<T> array) noexcept {
  assert(array);
  return array->len;
}

template <typename T>
void push_back(ArrayPtr<T> &array, const T value) noexcept {
  assert(array);
  if (array->len == array->cap) {
    auto newArray = make_array<T>(ceilToPowerOf2(array->cap * 2), array->len + 1);
    std::uninitialized_copy_n(array->data(), array->len, newArray->data());
    newArray->data()[array->len] = value;
    array = newArray;
  } else {
    array->data()[array->len] = value;
    ++array->len;
  }
}

template <typename T>
void append(ArrayPtr<T> &array, ArrayPtr<T> other) noexcept {
  assert(array);
  assert(other);
  if (array->len + other->len > array->cap) {
    auto newArray = make_array_len<T>(array->len + other->len);
    std::uninitialized_copy_n(array->data(), array->len, newArray->data());
    std::uninitialized_copy_n(other->data(), other->len, newArray->data() + array->len);
    array = newArray;
  } else {
    std::uninitialized_copy_n(other->data(), other->len, array->data() + array->len);
    array->len += other->len;
  }
}

template <typename T>
void pop_back(ArrayPtr<T> array) noexcept {
  assert(array);
  if (UNLIKELY(array->len == 0)) {
    panic("pop_back from empty array");
  } else {
    --array->len;
    std::destroy_at(array->data() + array->len);
  }
}

template <typename T>
void resize(ArrayPtr<T> &array, const t_uint size) noexcept {
  assert(array);
  if (size <= array->len) {
    std::destroy_n(array->data() + size, array->len - size);
    array->len = size;
  } else if (size <= array->cap) {
    std::uninitialized_value_construct_n(array->data() + array->len, size - array->len);
    array->len = size;
  } else {
    auto newArray = make_array_len<T>(size);
    std::uninitialized_copy_n(array->data(), array->len, newArray->data());
    std::uninitialized_value_construct_n(newArray->data() + array->len, size - array->len);
    array = newArray;
  }
}

template <typename T>
void reserve(ArrayPtr<T> &array, const t_uint cap) noexcept {
  assert(array);
  if (cap > array->cap) {
    auto newArray = make_array<T>(cap, array->len);
    std::uninitialized_copy_n(array->data(), array->len, newArray->data());
    array = newArray;
  }
}

template <typename T>
T &index(ArrayPtr<T> array, const t_uint index) noexcept {
  if (UNLIKELY(index >= array->len)) {
    panic("Indexing past end of array");
  } else {
    return array->data()[index];
  }
}

struct ClosureData : ref_count {
  virtual ~ClosureData() = default;
};

// Copy the captured data when assigned
//  - std::function
// Share the captured data when assigned
//  - Go
//  - JavaScript

using ClosureDataPtr = retain_ptr<ClosureData>;

struct FuncClosureData : ClosureData {};

}

// END BUILTIN LIBRARY

)delimiter";

void stela::appendBuiltinCode(std::string &src) {
  src += builtinCode;
}

void stela::appendTypes(std::string &bin, const gen::String &types) {
  bin += "// BEGIN TYPE DECLARATIONS\n\n";
  bin += types.str();
  bin += "\n// END TYPE DECLARATIONS\n\n";
}

void stela::appendFuncs(std::string &bin, const gen::String &funcs) {
  bin += "// BEGIN FUNCTION DECLARATIONS\n\n";
  bin += funcs.str();
  bin += "\n// END FUNCTION DECLARATIONS\n\n";
}

void stela::appendCode(std::string &bin, const gen::String &code) {
  bin += "// BEGIN CODE\n\n";
  bin += code.str();
  bin += "\n// END CODE\n\n";
}
