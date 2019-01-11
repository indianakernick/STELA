//
//  generate array.hpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_array_hpp
#define stela_generate_array_hpp

#include "func instantiations.hpp"

namespace stela {

llvm::Function *genArrDtor(InstData, ast::ArrayType *);
llvm::Function *genArrDefCtor(InstData, ast::ArrayType *);
llvm::Function *genArrCopCtor(InstData, ast::ArrayType *);
llvm::Function *genArrCopAsgn(InstData, ast::ArrayType *);
llvm::Function *genArrMovCtor(InstData, ast::ArrayType *);
llvm::Function *genArrMovAsgn(InstData, ast::ArrayType *);
llvm::Function *genArrIdxS(InstData, ast::ArrayType *);
llvm::Function *genArrIdxU(InstData, ast::ArrayType *);
llvm::Function *genArrLenCtor(InstData, ast::ArrayType *);
llvm::Function *genArrStrgDtor(InstData, ast::ArrayType *);
llvm::Function *genArrEq(InstData, ast::ArrayType *);
llvm::Function *genArrLt(InstData, ast::ArrayType *);

}

#endif
