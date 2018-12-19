//
//  generate func.cpp
//  STELA
//
//  Created by Indi Kernick on 2/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate func.hpp"

#include "unreachable.hpp"
#include "type builder.hpp"
#include "generate type.hpp"
#include "generate decl.hpp"
#include <llvm/IR/Function.h>
#include "reference count.hpp"
#include "assert down cast.hpp"
#include "function builder.hpp"

using namespace stela;

std::string stela::generateNullFunc(gen::Ctx ctx, const ast::FuncType &type) {
  /*gen::String name;
  name += "f_null_";
  // @TODO maybe consider caching type names
  // generateFuncName does that same work that generateNullFunc does
  name += generateFuncName(ctx, type);
  if (ctx.inst.funcNotInst(name)) {
    ctx.func += "[[noreturn]] static ";
    ctx.func += generateType(ctx, type.ret.get());
    ctx.func += ' ';
    ctx.func += name;
    ctx.func += "(void *";
    for (const ast::ParamType &param : type.params) {
      ctx.func += ", ";
      ctx.func += generateType(ctx, param.type.get());
      if (param.ref == ast::ParamRef::ref) {
        ctx.func += " &";
      }
    }
    ctx.func += ") noexcept {\n";
    ctx.func += "panic(\"Calling null function pointer\");\n";
    ctx.func += "}\n";
  }
  return name;*/
  return {};
}

std::string stela::generateMakeFunc(gen::Ctx ctx, ast::FuncType &type) {
  /*gen::String name;
  name += "f_makefunc_";
  name += generateFuncName(ctx, type);
  if (ctx.inst.funcNotInst(name)) {
    ctx.func += "static ";
    ctx.func += generateType(ctx, &type);
    ctx.func += " ";
    ctx.func += name;
    ctx.func += "(const ";
    ctx.func += generateFuncSig(ctx, type);
    ctx.func += R"( func) noexcept {
auto *ptr = static_cast<FuncClosureData *>(allocate(sizeof(FuncClosureData)));
new (ptr) FuncClosureData(); // setup virtual destructor
ptr->count = 1;
return {func, ClosureDataPtr{static_cast<ClosureData *>(ptr)}};
}
)";
  }
  return name;*/
  return {};
}

std::string stela::generateLambda(gen::Ctx ctx, const ast::Lambda &lambda) {
  /*gen::String func;
  func += "static ";
  func += generateType(ctx, lambda.ret.get());
  func += " ";
  gen::String name;
  name += "f_lam_";
  name += lambda.id;
  func += name;
  func += "(";
  func += generateLambdaCapture(ctx, lambda);
  func += " &capture";
  sym::Lambda *symbol = lambda.symbol;
  for (size_t p = 0; p != symbol->params.size(); ++p) {
    func += ", ";
    func += generateType(ctx, symbol->params[p].type.get());
    func += ' ';
    if (symbol->params[p].ref == sym::ValueRef::ref) {
      func += '&';
    }
    func += "p_";
    func += (p + 1);
  }
  func += ") noexcept {\n";
  func += generateDecl(ctx, lambda.body);
  func += "}\n";
  ctx.func += func;
  return name;*/
  return {};
}

std::string stela::generateMakeLam(gen::Ctx ctx, const ast::Lambda &lambda) {
  /*gen::String func;
  func += "static inline ";
  func += generateType(ctx, lambda.exprType.get());
  func += " ";
  gen::String name;
  name += "f_makelam_";
  name += lambda.id;
  func += name;
  func += "(";
  const gen::String capture = generateLambdaCapture(ctx, lambda);
  func += capture;
  func += " capture) noexcept {\n";
  
  func += "auto *ptr = static_cast<";
  func += capture;
  func += " *>(allocate(sizeof(";
  func += capture;
  func += ")));\n";
  
  func += "new (ptr) ";
  func += capture;
  func += "(capture);\n";
  
  func += "ptr->count = 1;\n";
  
  func += "return {reinterpret_cast<";
  auto *funcType = assertDownCast<ast::FuncType>(lambda.exprType.get());
  func += generateFuncSig(ctx, *funcType);
  func += ">(";
  func += generateLambda(ctx, lambda);
  func += "), ClosureDataPtr{static_cast<ClosureData *>(ptr)}};\n";
  
  func += "}\n";
  
  ctx.func += func;
  return name;*/
  return {};
}

namespace {

llvm::Function *makeInternalFunc(
  llvm::Module *module,
  llvm::FunctionType *type,
  const llvm::Twine &name
) {
  llvm::Function *func = llvm::Function::Create(
    type,
    llvm::Function::InternalLinkage,
    name,
    module
  );
  func->addFnAttr(llvm::Attribute::NoUnwind);
  func->addFnAttr(llvm::Attribute::AlwaysInline);
  return func;
}

llvm::FunctionType *dtorFor(llvm::Type *type) {
  return llvm::FunctionType::get(
    llvm::Type::getVoidTy(type->getContext()),
    {type},
    false
  );
}

llvm::FunctionType *defCtorFor(llvm::Type *type) {
  return llvm::FunctionType::get(type, {}, false);
}

llvm::Value *wrapPtr(llvm::IRBuilder<> &ir, llvm::Value *ptr) {
  TypeBuilder typeBdr{ir.getContext()};
  llvm::Value *undef = llvm::UndefValue::get(typeBdr.wrap(ptr->getType()));
  return ir.CreateInsertValue(undef, ptr, {0u});
}

}

llvm::Function *stela::generateArrayDtor(llvm::Module *module, llvm::Type *type) {
  llvm::Function *func = makeInternalFunc(module, dtorFor(type), "array_dtor");
  FuncBuilder funcBdr{func};
  llvm::BasicBlock *deleteBlock = funcBdr.makeBlock();
  llvm::BasicBlock *doneBlock = funcBdr.makeBlock();
  ReferenceCount refCount{funcBdr.ir};
  llvm::Value *array = funcBdr.ir.CreateExtractValue(func->arg_begin(), {0});
  // Decrement reference count
  llvm::Value *deleteArray = refCount.decr(array);
  // If reference count reached zero...
  funcBdr.ir.CreateCondBr(deleteArray, deleteBlock, doneBlock);
  funcBdr.setCurr(deleteBlock);
  // Deallocate
  funcBdr.callFree(array);
  funcBdr.ir.CreateBr(doneBlock);
  funcBdr.setCurr(doneBlock);
  funcBdr.ir.CreateRetVoid();
  return func;
}




// this wrapping and unwrapping is kind of confusing. Sort it out





llvm::Function *stela::generateArrayDefCtor(llvm::Module *module, llvm::Type *type) {
  llvm::Function *func = makeInternalFunc(module, defCtorFor(type), "array_def_ctor");
  FuncBuilder funcBdr{func};
  TypeBuilder typeBdr{type->getContext()};
  llvm::Type *arrayPtrType = type->getArrayElementType();
  llvm::Type *arrayStructType = arrayPtrType->getPointerElementType();
  llvm::Value *array = funcBdr.callMalloc(arrayStructType);

  llvm::Value *ref = funcBdr.ir.CreateStructGEP(array, 0);
  funcBdr.ir.CreateStore(llvm::ConstantInt::get(typeBdr.ref(), 1), ref);
  llvm::Value *cap = funcBdr.ir.CreateStructGEP(array, 1);
  funcBdr.ir.CreateStore(llvm::ConstantInt::get(typeBdr.len(), 0), cap);
  llvm::Value *len = funcBdr.ir.CreateStructGEP(array, 2);
  funcBdr.ir.CreateStore(llvm::ConstantInt::get(typeBdr.len(), 0), len);
  llvm::Value *dat = funcBdr.ir.CreateStructGEP(array, 3);
  llvm::Type *elem = arrayStructType->getStructElementType(3);
  funcBdr.ir.CreateStore(typeBdr.nullPtrTo(elem->getPointerElementType()), dat);

  funcBdr.ir.CreateRet(wrapPtr(funcBdr.ir, array));
  return func;
}
