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

namespace stela {

class FuncInst;

struct InstData {
  FuncInst &inst;
  llvm::Module *const mod;
};

namespace ast {

struct StructType;
struct ArrayType;

}

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
  /// Get the length constructor for an array
  llvm::Function *arrayLenCtor(ast::ArrayType *);
  /// Get the storage destructor for an array
  llvm::Function *arrayStrgDtor(ast::ArrayType *);
  /// Get the equality comparision function for an array
  llvm::Function *arrayEq(ast::ArrayType *);
  /// Get the less-than comparision function for an array
  llvm::Function *arrayLt(ast::ArrayType *);
  
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
  /// Get the equality comparison function for a struct
  llvm::Function *structEq(ast::StructType *);
  /// Get the less-than comparison function for a struct
  llvm::Function *structLt(ast::StructType *);
  
  /// Get the destructor for a reference counted pointer
  llvm::Function *pointerDtor();
  /// Get the copy constructor for a reference counted pointer
  llvm::Function *pointerCopCtor();
  /// Get the copy assignment function for a reference counted pointer
  llvm::Function *pointerCopAsgn();
  /// Get the move constructor for a reference counted pointer
  llvm::Function *pointerMovCtor();
  /// Get the move assignment function for a reference counted pointer
  llvm::Function *pointerMovAsgn();
  
  /// Get the panic function
  llvm::Function *panic();
  /// Get the memory allocation function
  llvm::Function *alloc();
  /// Get the memory deallocation function
  llvm::Function *free();
  /// Get the function used to ceil array lengths when calculating capacity
  llvm::Function *ceilToPow2();
  
private:
  using FuncMap = std::unordered_map<llvm::Type *, llvm::Function *>;

  llvm::Module *module;
  
  // @TODO std::array<std::unordered_map>
  // an enum will identify all of the functions
  // perhaps we could lookup the generator function from the enum
  // something like gen<Enum::arr_dtor>()
  
  FuncMap arrayDtors;
  FuncMap arrayDefCtors;
  FuncMap arrayCopCtors;
  FuncMap arrayCopAsgns;
  FuncMap arrayMovCtors;
  FuncMap arrayMovAsgns;
  FuncMap arrayIdxSs;
  FuncMap arrayIdxUs;
  FuncMap arrayLenCtors;
  FuncMap arrayStrgDtors;
  FuncMap arrayEqs;
  FuncMap arrayLts;
  
  FuncMap structDtors;
  FuncMap structDefCtors;
  FuncMap structCopCtors;
  FuncMap structCopAsgns;
  FuncMap structMovCtors;
  FuncMap structMovAsgns;
  FuncMap structEqs;
  FuncMap structLts;
  
  llvm::Function *pointerDtorFn = nullptr;
  llvm::Function *pointerCopCtorFn = nullptr;
  llvm::Function *pointerCopAsgnFn = nullptr;
  llvm::Function *pointerMovCtorFn = nullptr;
  llvm::Function *pointerMovAsgnFn = nullptr;
  
  llvm::Function *panicFn = nullptr;
  llvm::Function *allocFn = nullptr;
  llvm::Function *freeFn = nullptr;
  llvm::Function *ceilToPow2Fn = nullptr;
  
  template <typename Make, typename Type>
  llvm::Function *getCached(FuncMap &, Make *, Type *);
  llvm::Function *getCached(llvm::Function *&, llvm::Function *(*)(InstData));
};

}

#endif
