//
//  generate func.hpp
//  STELA
//
//  Created by Indi Kernick on 2/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_generate_func_hpp
#define stela_generate_func_hpp

#include "ast.hpp"
#include "symbols.hpp"
#include "gen context.hpp"

namespace llvm {

class Type;
class Function;
class Module;
class LLVMContext;
class IntegerType;
class PointerType;

}

namespace stela {

namespace gen {

class FuncInst;

}

std::string generateNullFunc(gen::Ctx, const ast::FuncType &);
std::string generateMakeFunc(gen::Ctx, ast::FuncType &);
std::string generateLambda(gen::Ctx, const ast::Lambda &);
std::string generateMakeLam(gen::Ctx, const ast::Lambda &);

template <size_t Size>
llvm::IntegerType *getSizedType(llvm::LLVMContext &);

template <typename Int>
llvm::IntegerType *getType(llvm::LLVMContext &ctx) {
  return getSizedType<sizeof(Int)>(ctx);
}

template <typename Int>
llvm::PointerType *getTypePtr(llvm::LLVMContext &ctx) {
  return getType<Int>(ctx)->getPointerTo();
}

llvm::Function *generatePanic(InstData);
llvm::Function *generateAlloc(InstData);
llvm::Function *generateFree(InstData);
llvm::Function *genCeilToPow2(InstData);

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
