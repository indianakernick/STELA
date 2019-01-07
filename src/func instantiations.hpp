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

}

namespace stela::gen {

class FuncInst {
public:
  explicit FuncInst(llvm::Module *);
  
  /// Get the destructor for an array
  llvm::Function *arrayDtor(llvm::Type *);
  /// Get the default constructor for an array
  llvm::Function *arrayDefCtor(llvm::Type *);
  /// Get the copy constructor for an array
  llvm::Function *arrayCopCtor(llvm::Type *);
  /// Get the copy assignment function for an array
  llvm::Function *arrayCopAsgn(llvm::Type *);
  /// Get the move constructor for an array
  llvm::Function *arrayMovCtor(llvm::Type *);
  /// Get the move assignment function for an array
  llvm::Function *arrayMovAsgn(llvm::Type *);
  /// Get the signed indexing function for an array
  llvm::Function *arrayIdxS(llvm::Type *);
  /// Get the unsigned indexing function for an array
  llvm::Function *arrayIdxU(llvm::Type *);
  
  llvm::Function *structDtor(llvm::Type *, ast::StructType *);
  llvm::Function *structDefCtor(llvm::Type *, ast::StructType *);
  llvm::Function *structCopCtor(llvm::Type *, ast::StructType *);
  llvm::Function *structCopAsgn(llvm::Type *, ast::StructType *);
  llvm::Function *structMovCtor(llvm::Type *, ast::StructType *);
  llvm::Function *structMovAsgn(llvm::Type *, ast::StructType *);
  
  /// Get the panic function
  llvm::Function *panic();
  /// Get the memory allocation function
  llvm::Function *alloc();
  /// Get the memory deallocation function
  llvm::Function *free();
  
private:
  using FuncMap = std::unordered_map<llvm::Type *, llvm::Function *>;
  using MakeArray = llvm::Function *(FuncInst &, llvm::Module *, llvm::Type *);
  using MakeStruct = llvm::Function *(FuncInst &, llvm::Module *, llvm::Type *, ast::StructType *);

  llvm::Module *module;
  
  FuncMap arrayDtors;
  FuncMap arrayDefCtors;
  FuncMap arrayCopCtors;
  FuncMap arrayCopAsgns;
  FuncMap arrayMovCtors;
  FuncMap arrayMovAsgns;
  FuncMap arrayIdxSs;
  FuncMap arrayIdxUs;
  
  FuncMap structDtors;
  FuncMap structDefCtors;
  FuncMap structCopCtors;
  FuncMap structCopAsgns;
  FuncMap structMovCtors;
  FuncMap structMovAsgns;
  FuncMap structIdxSs;
  FuncMap structIdxUs;
  
  llvm::Function *panicFn = nullptr;
  llvm::Function *allocFn = nullptr;
  llvm::Function *freeFn = nullptr;
  
  llvm::Function *getCached(FuncMap &, MakeArray *, llvm::Type *);
  llvm::Function *getCached(FuncMap &, MakeStruct *, llvm::Type *, ast::StructType *);
};

}

#endif
