//
//  lifetime exprs.cpp
//  STELA
//
//  Created by Indi Kernick on 25/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "lifetime exprs.hpp"

#include "unreachable.hpp"
#include "generate type.hpp"
#include "func instantiations.hpp"

using namespace stela;

LifetimeExpr::LifetimeExpr(FuncInst &inst, llvm::IRBuilder<> &ir)
  : inst{inst}, ir{ir} {}

void LifetimeExpr::defConstruct(ast::Type *type, llvm::Value *obj) {
  ast::Type *concrete = concreteType(type);
  if (auto *btn = dynamic_cast<ast::BtnType *>(concrete)) {
    llvm::Type *objType = obj->getType()->getPointerElementType();
    llvm::Value *value = nullptr;
    switch (btn->value) {
      case ast::BtnTypeEnum::Bool:
      case ast::BtnTypeEnum::Byte:
      case ast::BtnTypeEnum::Uint:
        value = llvm::ConstantInt::get(objType, 0, false);
        break;
      case ast::BtnTypeEnum::Char:
      case ast::BtnTypeEnum::Sint:
        value = llvm::ConstantInt::get(objType, 0, true);
        break;
      case ast::BtnTypeEnum::Real:
        value = llvm::ConstantFP::get(objType, 0.0);
        break;
      case ast::BtnTypeEnum::Void:
      default:
        UNREACHABLE();
    }
    ir.CreateStore(value, obj);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_def_ctor>(arr), {obj});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall( inst.get<PFGI::clo_def_ctor>(clo), {obj});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_def_ctor>(srt), {obj});
  } else {
    UNREACHABLE();
  }
  // startLife(obj)
}

void LifetimeExpr::copyConstruct(ast::Type *type, llvm::Value *obj, llvm::Value *other) {
  ast::Type *concrete = concreteType(type);
  if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(other), obj);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_cop_ctor>(arr), {obj, other});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::clo_cop_ctor>(clo), {obj, other});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_cop_ctor>(srt), {obj, other});
  } else {
    UNREACHABLE();
  }
  // startLife(obj)
}

void LifetimeExpr::moveConstruct(ast::Type *type, llvm::Value *obj, llvm::Value *other) {
  ast::Type *concrete = concreteType(type);
  if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(other), obj);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_mov_ctor>(arr), {obj, other});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::clo_mov_ctor>(clo), {obj, other});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_mov_ctor>(srt), {obj, other});
  } else {
    UNREACHABLE();
  }
  // startLife(obj)
}

void LifetimeExpr::copyAssign(ast::Type *type, llvm::Value *left, llvm::Value *right) {
  ast::Type *concrete = concreteType(type);
  // @TODO visitor?
  if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(right), left);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_cop_asgn>(arr), {left, right});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::clo_cop_asgn>(clo), {left, right});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_cop_asgn>(srt), {left, right});
  } else {
    UNREACHABLE();
  }
}

void LifetimeExpr::moveAssign(ast::Type *type, llvm::Value *left, llvm::Value *right) {
  ast::Type *concrete = concreteType(type);
  if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(right), left);
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_mov_asgn>(arr), {left, right});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::clo_mov_asgn>(clo), {left, right});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_mov_asgn>(srt), {left, right});
  } else {
    UNREACHABLE();
  }
}

void LifetimeExpr::relocate(ast::Type *type, llvm::Value *obj, llvm::Value *other) {
  const TypeCat category = classifyType(type);
  if (category == TypeCat::nontrivial) {
    moveConstruct(type, obj, other);
    destroy(type, other);
  } else {
    ir.CreateStore(ir.CreateLoad(other), obj);
  }
}

void LifetimeExpr::destroy(ast::Type *type, llvm::Value *obj) {
  ast::Type *concrete = concreteType(type);
  if (dynamic_cast<ast::BtnType *>(concrete)) {
    // do nothing
  } else if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::arr_dtor>(arr), {obj});
  } else if (auto *clo = dynamic_cast<ast::FuncType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::clo_dtor>(clo), {obj});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    ir.CreateCall(inst.get<PFGI::srt_dtor>(srt), {obj});
  } else {
    UNREACHABLE();
  }
  // endLife(obj)
}

void LifetimeExpr::construct(ast::Type *type, llvm::Value *obj, gen::Expr other) {
  const TypeCat cat = classifyType(type);
  if (cat == TypeCat::trivially_copyable) {
    if (glvalue(other.cat)) {
      ir.CreateStore(ir.CreateLoad(other.obj), obj);
    } else {
      ir.CreateStore(other.obj, obj);
    }
    // startLife(obj);
  } else { // trivially_relocatable or nontrivial
    if (other.cat == ValueCat::lvalue) {
      copyConstruct(type, obj, other.obj);
    } else if (other.cat == ValueCat::xvalue) {
      moveConstruct(type, obj, other.obj);
    }
    // the constructed object becomes the result object of the prvalue so
    // there's no need to do anything here
  }
}

void LifetimeExpr::assign(ast::Type *type, llvm::Value *left, gen::Expr right) {
  const TypeCat cat = classifyType(type);
  if (cat == TypeCat::trivially_copyable) {
    if (glvalue(right.cat)) {
      ir.CreateStore(ir.CreateLoad(right.obj), left);
    } else {
      ir.CreateStore(right.obj, left);
    }
  } else {
    if (right.cat == ValueCat::lvalue) {
      copyAssign(type, left, right.obj);
    } else if (right.cat == ValueCat::xvalue) {
      moveAssign(type, left, right.obj);
    } else { // prvalue
      moveAssign(type, left, right.obj);
      destroy(type, right.obj);
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
