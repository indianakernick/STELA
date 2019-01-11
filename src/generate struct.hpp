//
//  generate struct.hpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_struct_hpp
#define stela_generate_struct_hpp

#include "func instantiations.hpp"

namespace stela {

llvm::Function *genSrtDtor(InstData, ast::StructType *);
llvm::Function *genSrtDefCtor(InstData, ast::StructType *);
llvm::Function *genSrtCopCtor(InstData, ast::StructType *);
llvm::Function *genSrtCopAsgn(InstData, ast::StructType *);
llvm::Function *genSrtMovCtor(InstData, ast::StructType *);
llvm::Function *genSrtMovAsgn(InstData, ast::StructType *);
llvm::Function *genSrtEq(InstData, ast::StructType *);
llvm::Function *genSrtLt(InstData, ast::StructType *);

}

#endif
