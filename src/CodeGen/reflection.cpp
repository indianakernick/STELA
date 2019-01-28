//
//  reflection.cpp
//  STELA
//
//  Created by Indi Kernick on 28/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "reflection.hpp"

std::string stela::mangledName(const std::string_view name) {
  static std::unordered_map<std::string_view, size_t> indicies;
  size_t &index = indicies[name];
  std::string mangled = "stela_ext_fun_";
  mangled += name;
  mangled += '_';
  mangled += std::to_string(index);
  ++index;
  return mangled;
}

std::string stela::mangledName(const std::string &name) {
  return mangledName(std::string_view{name.data(), name.size()});
}
