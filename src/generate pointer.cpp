//
//  generate pointer.cpp
//  STELA
//
//  Created by Indi Kernick on 11/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include "generate pointer.hpp"

#include "gen helpers.hpp"

using namespace stela;

namespace {

enum class RefChg {
  inc,
  dec
};

llvm::Value *getRef(llvm::IRBuilder<> &ir, llvm::Value *ptr) {
  return ir.CreateStructGEP(ptr, 0);
}

llvm::Value *refChange(llvm::IRBuilder<> &ir, llvm::Value *ptr, const RefChg chg) {
  llvm::Value *ref = getRef(ir, ptr);
  llvm::Value *refVal = ir.CreateLoad(ref);
  llvm::Constant *one = constantFor(refVal, 1);
  llvm::Value *changed = chg == RefChg::inc ? ir.CreateNSWAdd(refVal, one)
                                            : ir.CreateNSWSub(refVal, one);
  ir.CreateStore(changed, ref);
  return changed;
}

void resetPtr(
  FuncInst &inst,
  FuncBuilder &funcBdr,
  llvm::Value *ptr,
  llvm::Function *dtor,
  llvm::BasicBlock *done
) {
  /*
  ptr.ref--
  if ptr.ref == 0
    dtor
    free ptr
    done
  else
    done
  */
  llvm::BasicBlock *del = funcBdr.makeBlock();
  llvm::Value *subed = refChange(funcBdr.ir, ptr, RefChg::dec);
  llvm::Value *isZero = funcBdr.ir.CreateICmpEQ(subed, constantFor(subed, 0));
  funcBdr.ir.CreateCondBr(isZero, del, done);
  funcBdr.setCurr(del);
  funcBdr.ir.CreateCall(dtor, {ptr});
  callFree(funcBdr.ir, inst.free(), ptr);
  funcBdr.branch(done);
}

}

llvm::Value *stela::ptrDefCtor(
  FuncInst &inst, FuncBuilder &funcBdr, llvm::Type *elemType
) {
  /*
  ptr = malloc elemType
  ptr.ref = 1
  */
  llvm::Value *ptr = callAlloc(funcBdr.ir, inst.alloc(), elemType);
  llvm::Value *ref = getRef(funcBdr.ir, ptr);
  funcBdr.ir.CreateStore(constantForPtr(ref, 1), ref);
  return ptr;
}

void stela::ptrDtor(
  FuncInst &inst,
  FuncBuilder &funcBdr,
  llvm::Value *objPtr,
  llvm::Function *dtor,
  llvm::BasicBlock *done
) {
  /*
  if ptr == null
    done
  else
    reset ptr
    done
  */
  llvm::BasicBlock *decr = funcBdr.makeBlock();
  llvm::Value *ptr = funcBdr.ir.CreateLoad(objPtr);
  funcBdr.ir.CreateCondBr(funcBdr.ir.CreateIsNull(ptr), done, decr);
  funcBdr.setCurr(decr);
  resetPtr(inst, funcBdr, ptr, dtor, done);
}

void stela::ptrCopCtor(
  FuncBuilder &funcBdr, llvm::Value *objPtr, llvm::Value *otherPtr
) {
  /*
  other.ref++
  obj = other
  */
  llvm::Value *other = funcBdr.ir.CreateLoad(otherPtr);
  refChange(funcBdr.ir, other, RefChg::inc);
  funcBdr.ir.CreateStore(other, objPtr);
}

void stela::ptrMovCtor(
  FuncBuilder &funcBdr, llvm::Value *objPtr, llvm::Value *otherPtr
) {
  /*
  obj = other
  other = null
  */
  funcBdr.ir.CreateStore(funcBdr.ir.CreateLoad(otherPtr), objPtr);
  setNull(funcBdr.ir, otherPtr);
}

void stela::ptrCopAsgn(
  FuncInst &inst,
  FuncBuilder &funcBdr,
  llvm::Value *leftPtr,
  llvm::Value *rightPtr,
  llvm::Function *dtor,
  llvm::BasicBlock *done
) {
  /*
  right.ref++
  reset left
  left = right
  */
  llvm::BasicBlock *innerDone = funcBdr.makeBlock();
  llvm::Value *right = funcBdr.ir.CreateLoad(rightPtr);
  refChange(funcBdr.ir, right, RefChg::inc);
  llvm::Value *left = funcBdr.ir.CreateLoad(leftPtr);
  resetPtr(inst, funcBdr, left, dtor, innerDone);
  funcBdr.ir.CreateStore(right, leftPtr);
  funcBdr.branch(done);
}

void stela::ptrMovAsgn(
  FuncInst &inst,
  FuncBuilder &funcBdr,
  llvm::Value *leftPtr,
  llvm::Value *rightPtr,
  llvm::Function *dtor,
  llvm::BasicBlock *done
) {
  /*
  reset left
  left = right
  right = null
  */
  llvm::BasicBlock *innerDone = funcBdr.makeBlock();
  llvm::Value *left = funcBdr.ir.CreateLoad(leftPtr);
  resetPtr(inst, funcBdr, left, dtor, innerDone);
  funcBdr.ir.CreateStore(funcBdr.ir.CreateLoad(rightPtr), leftPtr);
  setNull(funcBdr.ir, rightPtr);
  funcBdr.branch(done);
}
