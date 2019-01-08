//
//  lifetime exprs.cpp
//  STELA
//
//  Created by Indi Kernick on 25/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "lifetime exprs.hpp"

#include "categories.hpp"
#include "unreachable.hpp"
#include "generate type.hpp"
#include "func instantiations.hpp"

using namespace stela;

LifetimeExpr::LifetimeExpr(gen::FuncInst &inst, llvm::IRBuilder<> &ir)
  : inst{inst}, ir{ir} {}

void LifetimeExpr::defConstruct(ast::Type *type, llvm::Value *obj) {
  ast::Type *concrete = concreteType(type);
  llvm::Type *objType = obj->getType()->getPointerElementType();
  if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *defCtor = inst.arrayDefCtor(arr);
    ir.CreateCall(defCtor, {obj});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    llvm::Function *defCtor = inst.structDefCtor(srt);
    ir.CreateCall(defCtor, {obj});
  } else if (auto *btn = dynamic_cast<ast::BtnType *>(concrete)) {
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
  } else if (dynamic_cast<ast::FuncType *>(concrete)) {
    //str += generateMakeFunc(ctx, type);
    //str += "(&";
    //str += generateNullFunc(ctx, type);
    //str += ")";
    assert(false);
  } else {
    assert(false);
  }
  // startLife(obj)
}

void LifetimeExpr::copyConstruct(ast::Type *type, llvm::Value *obj, llvm::Value *other) {
  ast::Type *concrete = concreteType(type);
  if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *copCtor = inst.arrayCopCtor(arr);
    ir.CreateCall(copCtor, {obj, other});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    llvm::Function *copCtor = inst.structCopCtor(srt);
    ir.CreateCall(copCtor, {obj, other});
  } else if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(other), obj);
  } else {
    assert(false);
  }
  // startLife(obj)
}

void LifetimeExpr::moveConstruct(ast::Type *type, llvm::Value *obj, llvm::Value *other) {
  ast::Type *concrete = concreteType(type);
  if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *movCtor = inst.arrayMovCtor(arr);
    ir.CreateCall(movCtor, {obj, other});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    llvm::Function *movCtor = inst.structMovCtor(srt);
    ir.CreateCall(movCtor, {obj, other});
  } else if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(other), obj);
  } else {
    assert(false);
  }
  // startLife(obj)
}

void LifetimeExpr::copyAssign(ast::Type *type, llvm::Value *left, llvm::Value *right) {
  ast::Type *concrete = concreteType(type);
  // @TODO visitor?
  if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *copAsgn = inst.arrayCopAsgn(arr);
    ir.CreateCall(copAsgn, {left, right});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    llvm::Function *copAsgn = inst.structCopAsgn(srt);
    ir.CreateCall(copAsgn, {left, right});
  } else if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(right), left);
  } else {
    assert(false);
  }
}

void LifetimeExpr::moveAssign(ast::Type *type, llvm::Value *left, llvm::Value *right) {
  ast::Type *concrete = concreteType(type);
  if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *movAsgn = inst.arrayMovAsgn(arr);
    ir.CreateCall(movAsgn, {left, right});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    llvm::Function *movAsgn = inst.structMovAsgn(srt);
    ir.CreateCall(movAsgn, {left, right});
  } else if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(right), left);
  } else {
    assert(false);
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
  if (auto *arr = dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *dtor = inst.arrayDtor(arr);
    ir.CreateCall(dtor, {obj});
  } else if (auto *srt = dynamic_cast<ast::StructType *>(concrete)) {
    llvm::Function *dtor = inst.structDtor(srt);
    ir.CreateCall(dtor, {obj});
  } else if (dynamic_cast<ast::BtnType *>(concrete)) {
    // do nothing
  } else {
    assert(false);
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

void LifetimeExpr::startLife(llvm::Value *addr) {
  ir.CreateLifetimeStart(addr, objectSize(addr));
}

void LifetimeExpr::endLife(llvm::Value *addr) {
  ir.CreateLifetimeEnd(addr, objectSize(addr));
}
