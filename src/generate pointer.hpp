//
//  generate pointer.hpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_pointer_hpp
#define stela_generate_pointer_hpp

#include "func instantiations.hpp"

namespace stela {

llvm::Function *genPtrDtor(InstData);
llvm::Function *genPtrCopCtor(InstData);
llvm::Function *genPtrCopAsgn(InstData);
llvm::Function *genPtrMovCtor(InstData);
llvm::Function *genPtrMovAsgn(InstData);

}

#endif
