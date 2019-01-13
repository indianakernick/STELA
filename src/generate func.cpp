//
//  generate func.cpp
//  STELA
//
//  Created by Indi Kernick on 2/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "ast.hpp"
#include "inst data.hpp"
#include "gen types.hpp"
#include "gen context.hpp"
#include "gen helpers.hpp"
#include "compare exprs.hpp"
#include "lifetime exprs.hpp"
#include "function builder.hpp"
#include "func instantiations.hpp"

using namespace stela;

// create function object from function
std::string generateMakeFunc(gen::Ctx ctx, ast::FuncType &type) {
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

// body of a lambda
std::string generateLambda(gen::Ctx ctx, const ast::Lambda &lambda) {
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

// create function object from lambda
std::string generateMakeLam(gen::Ctx ctx, const ast::Lambda &lambda) {
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
  
  llvm::Type *strTy = getType<char>(ctx)->getPointerTo();
  llvm::Type *intTy = getType<int>(ctx);
  llvm::FunctionType *putsType = llvm::FunctionType::get(intTy, {strTy}, false);
  llvm::FunctionType *exitType = llvm::FunctionType::get(voidTy(ctx), {intTy}, false);
  llvm::FunctionType *panicType = llvm::FunctionType::get(voidTy(ctx), {strTy}, false);
  
  llvm::Function *puts = declareCFunc(data.mod, putsType, "puts");
  llvm::Function *exit = declareCFunc(data.mod, exitType, "exit");
  llvm::Function *panic = makeInternalFunc(data.mod, panicType, "panic");
  puts->addParamAttr(0, llvm::Attribute::ReadOnly);
  puts->addParamAttr(0, llvm::Attribute::NoCapture);
  exit->addFnAttr(llvm::Attribute::NoReturn);
  exit->addFnAttr(llvm::Attribute::Cold);
  panic->addFnAttr(llvm::Attribute::NoReturn);
  
  FuncBuilder builder{panic};
  builder.ir.CreateCall(puts, panic->arg_begin());
  builder.ir.CreateCall(exit, constantFor(intTy, EXIT_FAILURE));
  builder.ir.CreateUnreachable();
  
  return panic;
}

template <>
llvm::Function *stela::genFn<FGI::alloc>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  
  llvm::Type *memTy = voidPtrTy(ctx);
  llvm::Type *sizeTy = getType<size_t>(ctx);
  llvm::FunctionType *mallocType = llvm::FunctionType::get(memTy, {sizeTy}, false);
  llvm::FunctionType *allocType = mallocType;
  
  llvm::Function *malloc = declareCFunc(data.mod, mallocType, "malloc");
  llvm::Function *alloc = makeInternalFunc(data.mod, allocType, "alloc");
  malloc->addAttribute(0, llvm::Attribute::NoAlias);
  alloc->addAttribute(0, llvm::Attribute::NoAlias);
  
  FuncBuilder builder{alloc};
  llvm::BasicBlock *okBlock = builder.makeBlock();
  llvm::BasicBlock *errorBlock = builder.makeBlock();
  llvm::Value *ptr = builder.ir.CreateCall(malloc, alloc->arg_begin());
  llvm::Value *isNotNull = builder.ir.CreateIsNotNull(ptr);
  likely(builder.ir.CreateCondBr(isNotNull, okBlock, errorBlock));
  
  builder.setCurr(okBlock);
  builder.ir.CreateRet(ptr);
  builder.setCurr(errorBlock);
  callPanic(builder.ir, data.inst.get<FGI::panic>(), "Out of memory");
  
  return alloc;
}

template <>
llvm::Function *stela::genFn<FGI::free>(InstData data) {
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *memTy = voidPtrTy(ctx);
  llvm::FunctionType *freeType = llvm::FunctionType::get(voidTy(ctx), {memTy}, false);
  llvm::Function *free = declareCFunc(data.mod, freeType, "free");
  free->addParamAttr(0, llvm::Attribute::NoCapture);
  return free;
}

template <>
llvm::Function *stela::genFn<FGI::ceil_to_pow_2>(InstData data) {
  // this returns 2 when the input is 1
  // we could subtract val == 1 but that's just more instructions for no reason!
  llvm::LLVMContext &ctx = data.mod->getContext();
  llvm::Type *type = lenTy(ctx);
  llvm::FunctionType *fnType = llvm::FunctionType::get(
    type, {type}, false
  );
  llvm::Function *func = makeInternalFunc(data.mod, fnType, "ceil_to_pow_2");
  FuncBuilder builder{func};
  
  llvm::DataLayout layout{data.mod};
  const uint64_t bitSize = layout.getTypeSizeInBits(type);
  llvm::FunctionType *ctlzType = llvm::FunctionType::get(
    type, {type, builder.ir.getInt1Ty()}, false
  );
  llvm::Twine name = llvm::Twine{"llvm.ctlz.i"} + llvm::Twine{bitSize};
  llvm::Function *ctlz = declareCFunc(data.mod, ctlzType, name);
  llvm::Value *minus1 = builder.ir.CreateNSWSub(func->arg_begin(), constantFor(type, 1));
  llvm::Value *leadingZeros = builder.ir.CreateCall(ctlz, {minus1, builder.ir.getTrue()});
  llvm::Value *bits = constantFor(type, bitSize);
  llvm::Value *log2 = builder.ir.CreateNSWSub(bits, leadingZeros);
  llvm::Value *ceiled = builder.ir.CreateShl(constantFor(type, 1), log2);
  
  builder.ir.CreateRet(ceiled);
  return func;
}
