//
//  type instantiations.hpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_type_instantiations_hpp
#define stela_type_instantiations_hpp

#include <unordered_map>

namespace llvm {

class Type;
class Function;
class Module;

}

namespace stela::gen {

class TypeInst {
public:
  explicit TypeInst(llvm::Module *);

  /// Get the destructor for an array. Key is a pointer to an array struct
  llvm::Function *array(llvm::Type *);
  
private:
  llvm::Module *module;
  std::unordered_map<llvm::Type *, llvm::Function *> arrayDtors;
};

}

#endif
