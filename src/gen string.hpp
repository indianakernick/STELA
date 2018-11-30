//
//  gen string.hpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_gen_string_hpp
#define stela_gen_string_hpp

#include <string>
#include <string_view>

namespace stela::gen {

class String {
public:
  String() = default;
  String(String &&) = default;
  String &operator=(String &&) = default;
  ~String() = default;
  
  explicit String(const size_t cap) {
    storage.reserve(cap);
  }
  template <size_t Size>
  explicit String(const char (&string)[Size])
    : storage{&string[0], Size - 1} {}
  explicit String(const std::string &string)
    : storage{string} {}
  explicit String(std::string &&string)
    : storage{std::move(string)} {}
  
  template <size_t Size>
  String &operator=(const char (&string)[Size]) {
    storage.assign(&string[0], Size - 1);
    return *this;
  }
  
  bool operator==(const String &other) const {
    return storage == other.storage;
  }
  bool operator!=(const String &other) const {
    return storage != other.storage;
  }
  
  String dup() const {
    return String{storage};
  }
  
  void reserve(const size_t cap) {
    storage.reserve(cap);
  }
  char *data() {
    return storage.data();
  }
  const char *data() const {
    return storage.data();
  }
  size_t size() const {
    return storage.size();
  }
  
  template <size_t Size>
  void operator+=(const char (&string)[Size]) {
    static_assert(Size != 0, "Appending empty char array");
    static_assert(Size != 1, "Appending empty string");
    
    if constexpr (Size == 2) {
      storage.push_back(string[0]);
    } else {
      storage.append(&string[0], Size - 1);
    }
  }
  void operator+=(const char c) {
    storage.push_back(c);
  }
  void operator+=(const String &other) {
    storage.append(other.storage);
  }
  void operator+=(const std::string &other) {
    storage.append(other);
  }
  void operator+=(const std::string_view other) {
    storage.append(other.data(), other.size());
  }
  
  template <size_t Size>
  void prepend(const char (&string)[Size]) {
    static_assert(Size != 0, "Prepending empty char array");
    static_assert(Size != 1, "Prepending empty string");
    storage.insert(0, &string[0], Size - 1);
  }
  
  std::string &str() {
    return storage;
  }
  const std::string &str() const {
    return storage;
  }

private:
  std::string storage;
};

}

template <>
struct std::hash<stela::gen::String> {
  size_t operator()(const stela::gen::String &string) const noexcept {
    return std::hash<std::string>{}(string.str());
  }
};

#endif
