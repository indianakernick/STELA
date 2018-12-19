//
//  type builder.hpp
//  STELA
//
//  Created by Indi Kernick on 17/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_type_builder_hpp
#define stela_type_builder_hpp

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

class TypeBuilder {
public:
  explicit TypeBuilder(llvm::LLVMContext &);

  /// Integer for storing length of array
  llvm::IntegerType *len() const;
  /// Integer for storing reference count
  llvm::IntegerType *ref() const;
  /// Opaque pointer
  llvm::PointerType *voidPtr() const;
  /// Destructor
  llvm::FunctionType *dtor() const;
  /// Closure data
  llvm::StructType *cloData() const;
  /// Array of elements
  llvm::StructType *arrayOf(llvm::Type *) const;
  /// Constant null pointer
  llvm::ConstantPointerNull *nullPtr(llvm::PointerType *) const;
  /// Constant null pointer to a type
  llvm::ConstantPointerNull *nullPtrTo(llvm::Type *) const;
  /// Wrap a type in an array of size 1
  llvm::ArrayType *wrap(llvm::Type *) const;
  /// Wrap a pointer to a type in an array of size 1
  llvm::ArrayType *wrapPtrTo(llvm::Type *) const;
  /// Wrap a pointer to an array of a type in an array of size 1
  llvm::ArrayType *wrapPtrToArrayOf(llvm::Type *) const;
  
  
private:
  llvm::LLVMContext &ctx;
};

}

#endif
