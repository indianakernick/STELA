//
//  generate func.cpp
//  STELA
//
//  Created by Indi Kernick on 2/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate func.hpp"

#include "inst data.hpp"
#include "gen helpers.hpp"
#include "type builder.hpp"
#include "compare exprs.hpp"
#include "lifetime exprs.hpp"
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

// stub
std::string stela::generateNullFunc(gen::Ctx ctx, const ast::FuncType &type) {
  /*gen::String name;
  name += "f_null_";
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

template <>
llvm::Function *stela::genFn<FGI::panic>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::Type *voidTy = llvm::Type::getVoidTy(ctx);
  llvm::Type *strTy = getTypePtr<char>(ctx);
  llvm::Type *intTy = getType<int>(ctx);
  llvm::FunctionType *putsType = llvm::FunctionType::get(intTy, {strTy}, false);
  llvm::FunctionType *exitType = llvm::FunctionType::get(voidTy, {intTy}, false);
  llvm::FunctionType *panicType = llvm::FunctionType::get(voidTy, {strTy}, false);
  
  llvm::Function *puts = declareCFunc(data.mod, putsType, "puts");
  llvm::Function *exit = declareCFunc(data.mod, exitType, "exit");
  llvm::Function *panic = makeInternalFunc(data.mod, panicType, "panic");
  puts->addParamAttr(0, llvm::Attribute::ReadOnly);
  puts->addParamAttr(0, llvm::Attribute::NoCapture);
  exit->addFnAttr(llvm::Attribute::NoReturn);
  exit->addFnAttr(llvm::Attribute::Cold);
  panic->addFnAttr(llvm::Attribute::NoReturn);
  
  FuncBuilder funcBdr{panic};
  funcBdr.ir.CreateCall(puts, panic->arg_begin());
  funcBdr.ir.CreateCall(exit, constantFor(intTy, EXIT_FAILURE));
  funcBdr.ir.CreateUnreachable();
  
  return panic;
}

template <>
llvm::Function *stela::genFn<FGI::alloc>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::Type *memTy = llvm::Type::getInt8PtrTy(ctx);
  llvm::Type *sizeTy = getType<size_t>(ctx);
  llvm::FunctionType *mallocType = llvm::FunctionType::get(memTy, {sizeTy}, false);
  llvm::FunctionType *allocType = mallocType;
  
  llvm::Function *malloc = declareCFunc(data.mod, mallocType, "malloc");
  llvm::Function *alloc = makeInternalFunc(data.mod, allocType, "alloc");
  malloc->addAttribute(0, llvm::Attribute::NoAlias);
  alloc->addAttribute(0, llvm::Attribute::NoAlias);
  
  FuncBuilder funcBdr{alloc};
  llvm::BasicBlock *okBlock = funcBdr.makeBlock();
  llvm::BasicBlock *errorBlock = funcBdr.makeBlock();
  llvm::Value *ptr = funcBdr.ir.CreateCall(malloc, alloc->arg_begin());
  llvm::Value *isNotNull = funcBdr.ir.CreateIsNotNull(ptr);
  likely(funcBdr.ir.CreateCondBr(isNotNull, okBlock, errorBlock));
  
  funcBdr.setCurr(okBlock);
  funcBdr.ir.CreateRet(ptr);
  funcBdr.setCurr(errorBlock);
  callPanic(funcBdr.ir, data.inst.get<FGI::panic>(), "Out of memory");
  
  return alloc;
}

template <>
llvm::Function *stela::genFn<FGI::free>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *voidTy = llvm::Type::getVoidTy(ctx);
  llvm::Type *memTy = llvm::Type::getInt8PtrTy(ctx);
  llvm::FunctionType *freeType = llvm::FunctionType::get(voidTy, {memTy}, false);
  llvm::Function *free = declareCFunc(data.mod, freeType, "free");
  free->addParamAttr(0, llvm::Attribute::NoCapture);
  return free;
}

template <>
llvm::Function *stela::genFn<FGI::ceil_to_pow_2>(InstData data) {
  // this returns 2 when the input is 1
  // we could subtract val == 1 but that's just more instructions for no reason!
  llvm::LLVMContext &ctx = data.mod->getContext();
  TypeBuilder typeBdr{ctx};
  llvm::Type *type = typeBdr.len();
  llvm::FunctionType *fnType = llvm::FunctionType::get(
    type, {type}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, fnType, "ceil_to_pow_2");
  FuncBuilder funcBdr{func};
  
  llvm::DataLayout layout{data.mod};
  const uint64_t bitSize = layout.getTypeSizeInBits(type);
  llvm::FunctionType *ctlzType = llvm::FunctionType::get(
    type, {type, funcBdr.ir.getInt1Ty()}, false
  );
  llvm::Twine name = llvm::Twine{"llvm.ctlz.i"} + llvm::Twine{bitSize};
  llvm::Function *ctlz = declareCFunc(data.mod, ctlzType, name);
  llvm::Value *minus1 = funcBdr.ir.CreateNSWSub(func->arg_begin(), constantFor(type, 1));
  llvm::Value *leadingZeros = funcBdr.ir.CreateCall(ctlz, {minus1, funcBdr.ir.getTrue()});
  llvm::Value *bits = constantFor(type, bitSize);
  llvm::Value *log2 = funcBdr.ir.CreateNSWSub(bits, leadingZeros);
  llvm::Value *ceiled = funcBdr.ir.CreateShl(constantFor(type, 1), log2);
  
  funcBdr.ir.CreateRet(ceiled);
  return func;
}
