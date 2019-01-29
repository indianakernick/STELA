//
//  lifetime exprs.cpp
//  STELA
//
//  Created by Indi Kernick on 25/12/18.
//  Copysrc © 2018 Indi Kernick. All srcs reserved.
//

#include "lifetime exprs.hpp"

#include "generate type.hpp"
#include "Utils/unreachable.hpp"
#include "func instantiations.hpp"

using namespace stela;

LifetimeExpr::LifetimeExpr(FuncInst &inst, llvm::IRBuilder<> &ir)
  : inst{inst}, ir{ir} {}

void LifetimeExpr::defConstruct(ast::Type *type, llvm::Value *dst) {
  ast::Type *concrete = concreteType(type);
  if (auto *btn = dynamic_cast<ast::BtnType *>(concrete)) {
    llvm::Type *dstType = dst->getType()->getPointerElementType();
    llvm::Value *value = nullptr;
    switch (btn->value) {
      case ast::BtnTypeEnum::Bool:
      case ast::BtnTypeEnum::Byte:
      case ast::BtnTypeEnum::Uint:
        value = llvm::ConstantInt::get(dstType, 0, false);
        break;
      case ast::BtnTypeEnum::Char:
      case ast::BtnTypeEnum::Sint:
        value = llvm::ConstantInt::get(dstType, 0, true);
        break;
      case ast::BtnTypeEnum::Real:
        value = llvm::ConstantFP::get(dstType, 0.0);
        break;
      case ast::BtnTypeEnum::Void:
      default:
        UNREACHABLE();
    }
    ir.CreateStore(value, dst);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_def_ctor>(arr), {dst});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::clo_def_ctor>(clo), {dst});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_def_ctor>(srt), {dst});
  } else if (auto *usr = dynamic_cast<ast::UserType *>(concrete)) {
    assert(usr->defCtor.addr != ast::UserCtor::none); // @TODO deal with this in semantic analysis
    if (usr->defCtor.addr == ast::UserCtor::trivial) {
      ir.CreateMemSet(dst, ir.getInt8(0), usr->size, static_cast<unsigned>(usr->align));
    } else {
      ir.CreateCall(inst.get<PFGI::usr_def_ctor>(usr), {dst});
    }
  } else {
    UNREACHABLE();
  }
  // startLife(dst)
}

void LifetimeExpr::copyConstruct(ast::Type *type, llvm::Value *dst, llvm::Value *src) {
  ast::Type *concrete = concreteType(type);
  if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(src), dst);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_cop_ctor>(arr), {dst, src});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::clo_cop_ctor>(clo), {dst, src});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_cop_ctor>(srt), {dst, src});
  } else if (auto *usr = dynamic_cast<ast::UserType *>(concrete)) {
    assert(usr->copCtor.addr != ast::UserCtor::none); // @TODO deal with this in semantic analysis
    if (usr->copCtor.addr == ast::UserCtor::trivial) {
      triviallyCopy(usr->size, usr->align, dst, src);
    } else {
      ir.CreateCall(inst.get<PFGI::usr_cop_ctor>(usr), {dst, src});
    }
  } else {
    UNREACHABLE();
  }
  // startLife(dst)
}

void LifetimeExpr::moveConstruct(ast::Type *type, llvm::Value *dst, llvm::Value *src) {
  ast::Type *concrete = concreteType(type);
  if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(src), dst);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_mov_ctor>(arr), {dst, src});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::clo_mov_ctor>(clo), {dst, src});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_mov_ctor>(srt), {dst, src});
  } else if (auto *usr = dynamic_cast<ast::UserType *>(concrete)) {
    assert(usr->movCtor.addr != ast::UserCtor::none); // @TODO deal with this in semantic analysis
    if (usr->movCtor.addr == ast::UserCtor::trivial) {
      triviallyCopy(usr->size, usr->align, dst, src);
    } else {
      ir.CreateCall(inst.get<PFGI::usr_mov_ctor>(usr), {dst, src});
    }
  } else {
    UNREACHABLE();
  }
  // startLife(dst)
}

void LifetimeExpr::copyAssign(ast::Type *type, llvm::Value *dst, llvm::Value *src) {
  ast::Type *concrete = concreteType(type);
  // @TODO visitor?
  if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(src), dst);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_cop_asgn>(arr), {dst, src});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::clo_cop_asgn>(clo), {dst, src});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_cop_asgn>(srt), {dst, src});
  } else if (auto *usr = dynamic_cast<ast::UserType *>(concrete)) {
    assert(usr->copAsgn.addr != ast::UserCtor::none); // @TODO deal with this in semantic analysis
    if (usr->copAsgn.addr == ast::UserCtor::trivial) {
      triviallyCopy(usr->size, usr->align, dst, src);
    } else {
      ir.CreateCall(inst.get<PFGI::usr_cop_asgn>(usr), {dst, src});
    }
  } else {
    UNREACHABLE();
  }
}

void LifetimeExpr::moveAssign(ast::Type *type, llvm::Value *dst, llvm::Value *src) {
  ast::Type *concrete = concreteType(type);
  if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(src), dst);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_mov_asgn>(arr), {dst, src});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::clo_mov_asgn>(clo), {dst, src});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_mov_asgn>(srt), {dst, src});
  } else if (auto *usr = dynamic_cast<ast::UserType *>(concrete)) {
    assert(usr->movAsgn.addr != ast::UserCtor::none); // @TODO deal with this in semantic analysis
    if (usr->movAsgn.addr == ast::UserCtor::trivial) {
      triviallyCopy(usr->size, usr->align, dst, src);
    } else {
      ir.CreateCall(inst.get<PFGI::usr_mov_asgn>(usr), {dst, src});
    }
  } else {
    UNREACHABLE();
  }
}

void LifetimeExpr::destroy(ast::Type *type, llvm::Value *dst) {
  ast::Type *concrete = concreteType(type);
  if (dynamic_cast<ast::BtnType *>(concrete)) {
    // do nothing
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_dtor>(arr), {dst});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::clo_dtor>(clo), {dst});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_dtor>(srt), {dst});
  } else if (auto *usr = dynamic_cast<ast::UserType *>(concrete)) {
    assert(usr->dtor.addr != ast::UserCtor::none); // @TODO deal with this in semantic analysis
    if (usr->dtor.addr == ast::UserCtor::trivial) {
      // do nothing
    } else {
      ir.CreateCall(inst.get<PFGI::usr_dtor>(usr), {dst});
    }
  } else {
    UNREACHABLE();
  }
  // endLife(dst)
}

void LifetimeExpr::construct(ast::Type *type, llvm::Value *dst, gen::Expr src) {
  const TypeCat cat = classifyType(type);
  if (cat == TypeCat::trivially_copyable) {
    if (glvalue(src.cat)) {
      ir.CreateStore(ir.CreateLoad(src.obj), dst);
    } else {
      ir.CreateStore(src.obj, dst);
    }
    // startLife(dst);
  } else { // trivially_relocatable or nontrivial
    if (src.cat == ValueCat::lvalue) {
      copyConstruct(type, dst, src.obj);
    } else if (src.cat == ValueCat::xvalue) {
      moveConstruct(type, dst, src.obj);
    }
    // the constructed object becomes the result object of the prvalue so
    // there's no need to do anything here
  }
}

void LifetimeExpr::assign(ast::Type *type, llvm::Value *dst, gen::Expr src) {
  const TypeCat cat = classifyType(type);
  if (cat == TypeCat::trivially_copyable) {
    if (glvalue(src.cat)) {
      ir.CreateStore(ir.CreateLoad(src.obj), dst);
    } else {
      ir.CreateStore(src.obj, dst);
    }
  } else {
    if (src.cat == ValueCat::lvalue) {
      copyAssign(type, dst, src.obj);
    } else if (src.cat == ValueCat::xvalue) {
      moveAssign(type, dst, src.obj);
    } else { // prvalue
      moveAssign(type, dst, src.obj);
      destroy(type, src.obj);
    }
  }
}

void LifetimeExpr::startLife(llvm::Value *addr) {
  ir.CreateLifetimeStart(addr, objectSize(addr));
}

void LifetimeExpr::endLife(llvm::Value *addr) {
  ir.CreateLifetimeEnd(addr, objectSize(addr));
}

llvm::ConstantInt *LifetimeExpr::objectSize(llvm::Value *addr) {
  assert(addr->getType()->isPointerTy());
  llvm::Type *objType = addr->getType()->getPointerElementType();
  llvm::Constant *size = llvm::ConstantExpr::getSizeOf(objType);
  // @TODO find a way to convert llvm::Constant to llvm::ConstantInt
  // or call llvm.lifetime directly
  
  // I thought llvm.lifetime would allow for optimization but it seems
  // to prevent it in some cases
  return llvm::dyn_cast<llvm::ConstantInt>(size);
}

void LifetimeExpr::triviallyCopy(
  const size_t size,
  const size_t align,
  llvm::Value *dst,
  llvm::Value *src
) {
  auto castedAlign = static_cast<unsigned>(align);
  if (size <= sizeof(void *)) {
    ir.CreateAlignedStore(ir.CreateAlignedLoad(src, castedAlign), dst, castedAlign);
  } else {
    ir.CreateMemCpy(dst, castedAlign, src, castedAlign, size);
  }
}
