//
//  gen types.hpp
//  STELA
//
//  Created by Indi Kernick on 17/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_gen_types_hpp
#define stela_gen_types_hpp

#include <cstddef>

namespace llvm {

class LLVMContext;
class Type;
class IntegerType;
class PointerType;
class StructType;
class FunctionType;
class ArrayType;
class ConstantPointerNull;

}

namespace stela {

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

/// Integer for storing length of array
llvm::IntegerType *lenTy(llvm::LLVMContext &);
/// Integer for storing reference count
llvm::IntegerType *refTy(llvm::LLVMContext &);
/// Opaque pointer
llvm::PointerType *voidPtrTy(llvm::LLVMContext &);

/// Destructor
llvm::FunctionType *dtorTy(llvm::LLVMContext &);
/// Closure data
llvm::StructType *cloDataTy(llvm::LLVMContext &);
/// Pointer to closure data
llvm::PointerType *ptrToCloDataTy(llvm::LLVMContext &);
/// Array of elements
llvm::StructType *arrayTy(llvm::Type *);
/// Pointer to array of elements
llvm::PointerType *ptrToArrayTy(llvm::Type *);

/// Constant null pointer
llvm::ConstantPointerNull *nullPtr(llvm::PointerType *);
/// Constant null pointer to a type
llvm::ConstantPointerNull *nullPtrTo(llvm::Type *);

}

#endif
