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

namespace {

// @TODO we'll need to use this to determine how to call and return
enum class TypeCat {
  /// Primitive types like integers and floats.
  /// Default constructor just sets to 0. Destructor does nothing.
  /// Other special functions just copy
  trivially_copyable,
  /// Arrays and closures.
  /// Requires calls to special functions but can be relocated.
  /// Moving and then destroying is the same as copying
  trivially_relocatable,
  /// Structs and some user types.
  /// Requires calls to special functions.
  /// Passed to functions by pointer and returned by pointer.
  nontrivial
};

class Visitor final : public ast::Visitor {
public:
  void visit(ast::BtnType &) override {
    cat = TypeCat::trivially_copyable;
  }
  void visit(ast::ArrayType &) override {
    cat = TypeCat::trivially_relocatable;
  }
  void visit(ast::FuncType &) override {
    cat = TypeCat::trivially_relocatable;
  }
  void visit(ast::NamedType &name) override {
    name.definition->type->accept(*this);
  }
  void visit(ast::StructType &) override {
    cat = TypeCat::nontrivial;
  }
  // @TODO check for user types

  TypeCat cat;
};

TypeCat classifyType(ast::Type *type) {
  Visitor visitor;
  type->accept(visitor);
  return visitor.cat;
}

}

LifetimeExpr::LifetimeExpr(gen::FuncInst &inst, llvm::IRBuilder<> &ir)
  : inst{inst}, ir{ir} {}

void LifetimeExpr::defConstruct(ast::Type *type, llvm::Value *obj) {
  ast::Type *concrete = concreteType(type);
  llvm::Type *objType = obj->getType()->getPointerElementType();
  if (dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *defCtor = inst.arrayDefCtor(objType);
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
    llvm::Function *copCtor = inst.arrayCopCtor(objType);
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
    llvm::Function *movCtor = inst.arrayMovCtor(objType);
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
    llvm::Function *copAsgn = inst.arrayCopAsgn(leftType);
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
    llvm::Function *movAsgn = inst.arrayMovAsgn(leftType);
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
  llvm::Type *objType = obj->getType()->getPointerElementType();
  if (dynamic_cast<ast::ArrayType *>(concrete)) {
    llvm::Function *dtor = inst.arrayDtor(objType);
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
