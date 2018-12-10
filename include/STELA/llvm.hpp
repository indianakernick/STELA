//
//  llvm.hpp
//  STELA
//
//  Created by Indi Kernick on 10/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_llvm_hpp
#define stela_llvm_hpp

namespace llvm {
  class LLVMContext;
}

namespace stela {

void initLLVM();
void quitLLVM();
[[nodiscard]] llvm::LLVMContext &getLLVM();

}

#endif
