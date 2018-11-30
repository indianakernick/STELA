//
//  type instantiations.hpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_type_instantiations_hpp
#define stela_type_instantiations_hpp

#include <unordered_set>
#include "gen string.hpp"

namespace stela::gen {

class TypeInst {
public:
  TypeInst();

  // return false and insert if not instantiated
  // return true and dont insert if instantiated
  
  bool arrayNotInst(const gen::String &);
  bool funcNotInst(const gen::String &);
  bool structNotInst(const gen::String &);

private:
  std::unordered_set<String> arrays;
  std::unordered_set<String> funcs;
  std::unordered_set<String> structs;
};

}

#endif
