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

using namespace stela;

LifetimeExpr::LifetimeExpr(gen::Ctx ctx, llvm::IRBuilder<> &ir)
  : ctx{ctx}, ir{ir} {}

void LifetimeExpr::defConstruct(ast::Type *type, llvm::Value *obj) {
  ast::Type *concrete = concreteType(type);
  llvm::Type *objType = obj->getType()->getPointerElementType();
  if (dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *defCtor = ctx.inst.arrayDefCtor(objType);
    ir.CreateCall(defCtor, {obj});
  } else if (auto *strut = dynamic_cast<ast::StructType *>(concrete)) {
    const unsigned members = objType->getNumContainedTypes();
    for (unsigned m = 0; m != members; ++m) {
      llvm::Value *memPtr = ir.CreateStructGEP(obj, m);
      defConstruct(strut->fields[m].type.get(), memPtr);
    }
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
  // ir.CreateLifetimeStart(obj);
}

void LifetimeExpr::copyConstruct(ast::Type *type, llvm::Value *obj, llvm::Value *other) {
  ast::Type *concrete = concreteType(type);
  llvm::Type *objType = obj->getType()->getPointerElementType();
  if (dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *copCtor = ctx.inst.arrayCopCtor(objType);
    ir.CreateStore(ir.CreateCall(copCtor, {ir.CreateLoad(other)}), obj);
  } else if (auto *strut = dynamic_cast<ast::StructType *>(concrete)) {
    const unsigned members = objType->getNumContainedTypes();
    for (unsigned m = 0; m != members; ++m) {
      llvm::Value *dstPtr = ir.CreateStructGEP(obj, m);
      llvm::Value *srcPtr = ir.CreateStructGEP(other, m);
      copyConstruct(strut->fields[m].type.get(), dstPtr, srcPtr);
    }
  } else if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(other), obj);
  } else {
    assert(false);
  }
  // ir.CreateLifetimeStart(obj);
}

void LifetimeExpr::moveConstruct(ast::Type *type, llvm::Value *obj, llvm::Value *other) {
  ast::Type *concrete = concreteType(type);
  llvm::Type *objType = obj->getType()->getPointerElementType();
  if (dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *movCtor = ctx.inst.arrayMovCtor(objType);
    ir.CreateCall(movCtor, {obj, other});
  } else if (auto *strut = dynamic_cast<ast::StructType *>(concrete)) {
    const unsigned members = objType->getNumContainedTypes();
    for (unsigned m = 0; m != members; ++m) {
      llvm::Value *dstPtr = ir.CreateStructGEP(obj, m);
      llvm::Value *srcPtr = ir.CreateStructGEP(other, m);
      moveConstruct(strut->fields[m].type.get(), dstPtr, srcPtr);
    }
  } else if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(other), obj);
  } else {
    assert(false);
  }
  // ir.CreateLifetimeStart(obj);
}

void LifetimeExpr::copyAssign(ast::Type *type, llvm::Value *left, llvm::Value *right) {
  ast::Type *concrete = concreteType(type);
  llvm::Type *leftType = left->getType()->getPointerElementType();
  // @TODO visitor?
  if (dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *copAsgn = ctx.inst.arrayCopAsgn(leftType);
    ir.CreateCall(copAsgn, {left, ir.CreateLoad(right)});
  } else if (auto *strut = dynamic_cast<ast::StructType *>(concrete)) {
    const unsigned members = leftType->getNumContainedTypes();
    for (unsigned m = 0; m != members; ++m) {
      llvm::Value *dstPtr = ir.CreateStructGEP(left, m);
      llvm::Value *srcPtr = ir.CreateStructGEP(right, m);
      copyAssign(strut->fields[m].type.get(), dstPtr, srcPtr);
    }
  } else if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(right), left);
  } else {
    assert(false);
  }
}

void LifetimeExpr::moveAssign(ast::Type *type, llvm::Value *left, llvm::Value *right) {
  ast::Type *concrete = concreteType(type);
  llvm::Type *leftType = left->getType()->getPointerElementType();
  if (dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *movAsgn = ctx.inst.arrayMovAsgn(leftType);
    ir.CreateCall(movAsgn, {left, right});
  } else if (auto *strut = dynamic_cast<ast::StructType *>(concrete)) {
    const unsigned members = leftType->getNumContainedTypes();
    for (unsigned m = 0; m != members; ++m) {
      llvm::Value *dstPtr = ir.CreateStructGEP(left, m);
      llvm::Value *srcPtr = ir.CreateStructGEP(right, m);
      moveAssign(strut->fields[m].type.get(), dstPtr, srcPtr);
    }
  } else if (dynamic_cast<ast::BtnType *>(concrete)) {
    ir.CreateStore(ir.CreateLoad(right), left);
  } else {
    assert(false);
  }
}

void LifetimeExpr::relocate(ast::Type *, llvm::Value *obj, llvm::Value *other) {
  /*
  if (type is not trivally relocatable) {
    moveConstruct(type, obj, other);
    destroy(type, other);
  }
  */
  ir.CreateStore(ir.CreateLoad(other), obj);
}

void LifetimeExpr::destroy(ast::Type *type, llvm::Value *obj) {
  ast::Type *concrete = concreteType(type);
  llvm::Type *objType = obj->getType()->getPointerElementType();
  if (dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *dtor = ctx.inst.arrayDtor(objType);
    ir.CreateCall(dtor, {ir.CreateLoad(obj)});
  } else if (auto *strut = dynamic_cast<ast::StructType *>(concrete)) {
    const unsigned members = objType->getNumContainedTypes();
    for (unsigned m = 0; m != members; ++m) {
      llvm::Value *memPtr = ir.CreateStructGEP(obj, m);
      destroy(strut->fields[m].type.get(), memPtr);
    }
  } else if (dynamic_cast<ast::BtnType *>(concrete)) {
    // do nothing
  } else {
    assert(false);
  }
  // ir.CreateLifetimeEnd(obj);
}
