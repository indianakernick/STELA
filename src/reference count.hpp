//
//  reference count.hpp
//  STELA
//
//  Created by Indi Kernick on 17/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_reference_count_hpp
#define stela_reference_count_hpp

#include <llvm/IR/IRBuilder.h>

namespace stela {

class ReferenceCount {
public:
  explicit ReferenceCount(llvm::IRBuilder<> &);

  /// Increment the first member of a struct
  void incr(llvm::Value *);
  /// Decrement the first member of a struct and return a i1 that is true if
  /// the number reached zero
  llvm::Value *decr(llvm::Value *);

private:
  llvm::IRBuilder<> &ir;
};

}

#endif
