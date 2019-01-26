//
//  func instantiations.hpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_func_instantiations_hpp
#define stela_func_instantiations_hpp

#include <array>
#include "inst data.hpp"
#include <unordered_map>

namespace llvm {

class Type;

}

namespace stela {

class FuncInst {
public:
  explicit FuncInst(llvm::Module *);
  
  template <FGI Fn>
  llvm::Function *get() {
    llvm::Function *&func = fns[static_cast<size_t>(Fn)];
    if (!func) {
      func = genFn<Fn>({*this, module});
    }
    return func;
  }
  
  template <PFGI Fn, typename Param>
  llvm::Function *get(Param *param) {
    FuncMap &map = paramFns[static_cast<size_t>(Fn)];
    llvm::Type *type = getType(param);
    const auto iter = map.find(type);
    if (iter == map.end()) {
      llvm::Function *func = genFn<Fn>({*this, module}, param);
      map.insert({type, func});
      return func;
    } else {
      return iter->second;
    }
  }
  
private:
  using FuncMap = std::unordered_map<llvm::Type *, llvm::Function *>;

  llvm::Module *module;
  std::array<llvm::Function *, static_cast<size_t>(FGI::count_)> fns;
  std::array<FuncMap, static_cast<size_t>(PFGI::count_)> paramFns;
  
  llvm::Type *getType(ast::Type *);
};

}

#endif
