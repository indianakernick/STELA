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

llvm::Function *generatePanic(llvm::Module *);
llvm::Function *generateAlloc(gen::FuncInst &, llvm::Module *);
llvm::Function *generateFree(llvm::Module *);

llvm::Function *generateArrayDtor(gen::FuncInst &, llvm::Module *, llvm::Type *);
llvm::Function *generateArrayDefCtor(gen::FuncInst &, llvm::Module *, llvm::Type *);
llvm::Function *generateArrayCopCtor(gen::FuncInst &, llvm::Module *, llvm::Type *);
llvm::Function *generateArrayCopAsgn(gen::FuncInst &, llvm::Module *, llvm::Type *);
llvm::Function *generateArrayMovCtor(gen::FuncInst &, llvm::Module *, llvm::Type *);
llvm::Function *generateArrayMovAsgn(gen::FuncInst &, llvm::Module *, llvm::Type *);
llvm::Function *generateArrayIdxS(gen::FuncInst &, llvm::Module *, llvm::Type *);
llvm::Function *generateArrayIdxU(gen::FuncInst &, llvm::Module *, llvm::Type *);

llvm::Function *genSrtDtor(gen::FuncInst &, llvm::Module *, llvm::Type *, ast::StructType *);
llvm::Function *genSrtDefCtor(gen::FuncInst &, llvm::Module *, llvm::Type *, ast::StructType *);
llvm::Function *genSrtCopCtor(gen::FuncInst &, llvm::Module *, llvm::Type *, ast::StructType *);
llvm::Function *genSrtCopAsgn(gen::FuncInst &, llvm::Module *, llvm::Type *, ast::StructType *);
llvm::Function *genSrtMovCtor(gen::FuncInst &, llvm::Module *, llvm::Type *, ast::StructType *);
llvm::Function *genSrtMovAsgn(gen::FuncInst &, llvm::Module *, llvm::Type *, ast::StructType *);

}

#endif
