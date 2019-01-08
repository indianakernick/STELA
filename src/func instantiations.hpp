//
//  func instantiations.hpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_func_instantiations_hpp
#define stela_func_instantiations_hpp

#include <unordered_map>

namespace llvm {

class Type;
class Function;
class Module;

}

namespace stela::ast {

struct StructType;
struct ArrayType;

}

namespace stela::gen {

class FuncInst {
public:
  explicit FuncInst(llvm::Module *);
  
  /// Get the destructor for an array
  llvm::Function *arrayDtor(ast::ArrayType *);
  /// Get the default constructor for an array
  llvm::Function *arrayDefCtor(ast::ArrayType *);
  /// Get the copy constructor for an array
  llvm::Function *arrayCopCtor(ast::ArrayType *);
  /// Get the copy assignment function for an array
  llvm::Function *arrayCopAsgn(ast::ArrayType *);
  /// Get the move constructor for an array
  llvm::Function *arrayMovCtor(ast::ArrayType *);
  /// Get the move assignment function for an array
  llvm::Function *arrayMovAsgn(ast::ArrayType *);
  /// Get the signed indexing function for an array
  llvm::Function *arrayIdxS(ast::ArrayType *);
  /// Get the unsigned indexing function for an array
  llvm::Function *arrayIdxU(ast::ArrayType *);
  /// Get the length construct for an array
  llvm::Function *arrayLenCtor(ast::ArrayType *);
  
  /// Get the destructor for a struct
  llvm::Function *structDtor(ast::StructType *);
  /// Get the default construct for a struct
  llvm::Function *structDefCtor(ast::StructType *);
  /// Get the copy constructor for a struct
  llvm::Function *structCopCtor(ast::StructType *);
  /// Get the copy assignment function for a struct
  llvm::Function *structCopAsgn(ast::StructType *);
  /// Get the move constructor for a struct
  llvm::Function *structMovCtor(ast::StructType *);
  /// Get the move assignment function for a struct
  llvm::Function *structMovAsgn(ast::StructType *);
  
  /// Get the panic function
  llvm::Function *panic();
  /// Get the memory allocation function
  llvm::Function *alloc();
  /// Get the memory deallocation function
  llvm::Function *free();
  
private:
  using FuncMap = std::unordered_map<llvm::Type *, llvm::Function *>;

  llvm::Module *module;
  
  FuncMap arrayDtors;
  FuncMap arrayDefCtors;
  FuncMap arrayCopCtors;
  FuncMap arrayCopAsgns;
  FuncMap arrayMovCtors;
  FuncMap arrayMovAsgns;
  FuncMap arrayIdxSs;
  FuncMap arrayIdxUs;
  FuncMap arrayLenCtors;
  
  FuncMap structDtors;
  FuncMap structDefCtors;
  FuncMap structCopCtors;
  FuncMap structCopAsgns;
  FuncMap structMovCtors;
  FuncMap structMovAsgns;
  
  llvm::Function *panicFn = nullptr;
  llvm::Function *allocFn = nullptr;
  llvm::Function *freeFn = nullptr;
  
  template <typename Make, typename Type>
  llvm::Function *getCached(FuncMap &, Make *, Type *);
};

}

#endif
