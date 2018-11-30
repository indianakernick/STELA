//
//  type instantiations.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "type instantiations.hpp"

using namespace stela;

gen::TypeInst::TypeInst() {
  arrays.reserve(32);
  funcs.reserve(32);
  structs.reserve(32);
}

namespace {

bool notInst(std::unordered_set<gen::String> &set, const gen::String &string) {
  // second is true if string was inserted
  // if string was inserted, type was not instantiated
  return set.insert(string.dup()).second;
}

}

bool gen::TypeInst::arrayNotInst(const String &string) {
  return notInst(arrays, string);
}

bool gen::TypeInst::funcNotInst(const String &string) {
  return notInst(funcs, string);
}

bool gen::TypeInst::structNotInst(const String &string) {
  return notInst(structs, string);
}
