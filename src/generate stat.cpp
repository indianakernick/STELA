//
//  generate stat.cpp
//  STELA
//
//  Created by Indi Kernick on 10/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate stat.hpp"

#include "llvm.hpp"
#include "symbols.hpp"
#include "generate type.hpp"
#include "generate expr.hpp"
#include "function builder.hpp"
#include "lower expressions.hpp"
#include "expression builder.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, llvm::Function *func)
    : ctx{ctx}, funcBdr{func}, exprBdr{ctx, funcBdr} {}

  void visitFlow(ast::Statement *body, llvm::BasicBlock *brake, llvm::BasicBlock *continu) {
    llvm::BasicBlock *oldBrake = std::exchange(breakBlock, brake);
    llvm::BasicBlock *oldContinue = std::exchange(continueBlock, continu);
    body->accept(*this);
    continueBlock = oldContinue;
    breakBlock = oldBrake;
  }
  llvm::Value *equalTo(llvm::Value *value, ast::Expression *expr) {
    const ArithNumber arith = classifyArith(expr);
    if (arith == ArithNumber::floating_point) {
      return funcBdr.ir.CreateFCmpOEQ(value, exprBdr.value(expr));
    } else {
      return funcBdr.ir.CreateICmpEQ(value, exprBdr.value(expr));
    }
  }
  
  void visit(ast::Block &block) override {
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
  }
  void visit(ast::If &fi) override {
    auto *cond = funcBdr.nextEmpty();
    auto *troo = funcBdr.makeBlock();
    auto *fols = funcBdr.makeBlock();
    llvm::BasicBlock *done = nullptr;
    funcBdr.setCurr(cond);
    exprBdr.condBr(fi.cond.get(), troo, fols);
    
    funcBdr.setCurr(troo);
    fi.body->accept(*this);
    done = funcBdr.terminateLazy(done);
    
    funcBdr.setCurr(fols);
    if (fi.elseBody) {
      fi.elseBody->accept(*this);
    }
    done = funcBdr.terminateLazy(done);
    
    if (done) {
      funcBdr.setCurr(done);
    }
  }
  void visit(ast::Switch &swich) override {
    llvm::Value *value = exprBdr.value(swich.expr.get());
    if (swich.cases.empty()) {
      return;
    }
    
    auto checkBlocks = funcBdr.makeBlocks(swich.cases.size());
    auto caseBlocks = funcBdr.makeBlocks(swich.cases.size());
    llvm::BasicBlock *done = funcBdr.makeBlock();
    size_t defaultIndex = ~size_t{};
    bool foundDefault = false;
    funcBdr.ir.CreateBr(checkBlocks[0]);
    
    for (size_t c = 0; c != swich.cases.size(); ++c) {
      ast::Expression *expr = swich.cases[c].expr.get();
      if (!expr) {
        defaultIndex = c;
        foundDefault = true;
        continue;
      }
      const size_t checkIndex = c - foundDefault;
      const size_t caseIndex = c;
      
      funcBdr.setCurr(checkBlocks[checkIndex]);
      llvm::Value *cond = equalTo(value, expr);
      
      llvm::BasicBlock *nextCheck;
      if (!foundDefault && c == swich.cases.size() - 1) {
        nextCheck = done;
      } else {
        nextCheck = checkBlocks[checkIndex + 1];
      }
      
      funcBdr.ir.CreateCondBr(cond, caseBlocks[caseIndex], nextCheck);
    }
    
    if (foundDefault) {
      funcBdr.link(checkBlocks.back(), caseBlocks[defaultIndex]);
    }
    
    for (size_t c = 0; c != swich.cases.size(); ++c) {
      funcBdr.setCurr(caseBlocks[c]);
      llvm::BasicBlock *nextBlock = nullptr;
      if (c != swich.cases.size() - 1) {
        nextBlock = caseBlocks[c + 1];
      }
      visitFlow(swich.cases[c].body.get(), done, nextBlock);
      funcBdr.terminate(done);
    }
    
    funcBdr.setCurr(done);
    if (swich.alwaysReturns) {
      funcBdr.ir.CreateUnreachable();
    }
  }
  void visit(ast::Break &) override {
    assert(breakBlock);
    funcBdr.ir.CreateBr(breakBlock);
  }
  void visit(ast::Continue &) override {
    assert(continueBlock);
    funcBdr.ir.CreateBr(continueBlock);
  }
  void visit(ast::Return &ret) override {
    if (ret.expr) {
      llvm::Value *value = exprBdr.value(ret.expr.get());
      if (value->getType()->isVoidTy()) {
        funcBdr.ir.CreateRetVoid();
      } else if (value->getType()->isStructTy()) {
        funcBdr.ir.CreateStore(value, &funcBdr.args().back());
        funcBdr.ir.CreateRetVoid();
      } else {
        funcBdr.ir.CreateRet(value);
      }
    } else {
      funcBdr.ir.CreateRetVoid();
    }
  }
  void visit(ast::While &wile) override {
    auto *cond = funcBdr.nextEmpty();
    auto *body = funcBdr.makeBlock();
    auto *done = funcBdr.makeBlock();
    funcBdr.setCurr(body);
    visitFlow(wile.body.get(), done, cond);
    funcBdr.terminate(cond);
    funcBdr.setCurr(cond);
    exprBdr.condBr(wile.cond.get(), body, done);
    funcBdr.setCurr(done);
  }
  void visit(ast::For &four) override {
    if (four.init) {
      four.init->accept(*this);
    }
    auto *cond = funcBdr.nextEmpty();
    auto *body = funcBdr.makeBlock();
    auto *incr = funcBdr.makeBlock();
    auto *done = funcBdr.makeBlock();
    funcBdr.setCurr(body);
    visitFlow(four.body.get(), done, incr);
    funcBdr.terminate(incr);
    funcBdr.setCurr(cond);
    exprBdr.condBr(four.cond.get(), body, done);
    funcBdr.setCurr(incr);
    if (four.incr) {
      four.incr->accept(*this);
      funcBdr.ir.CreateBr(cond);
    }
    funcBdr.setCurr(done);
  }
  
  llvm::Value *insertVar(sym::Object *obj, ast::Expression *expr) {
    if (expr) {
      return funcBdr.allocStore(exprBdr.value(expr));
    } else {
      return funcBdr.allocStore(exprBdr.zero(obj->etype.type.get()));
    }
  }
  
  void visit(ast::Var &var) override {
    var.llvmAddr = insertVar(var.symbol, var.expr.get());
  }
  void visit(ast::Let &let) override {
    let.llvmAddr = insertVar(let.symbol, let.expr.get());
  }
  
  void doAssign(ast::Type *type, llvm::Value *left, llvm::Value *right) {
    if (concreteType<ast::ArrayType>(type)) {
      llvm::Type *wrapperType = left->getType()->getPointerElementType();
      if (right->getType()->isPointerTy()) {
        llvm::Function *copAsgn = ctx.inst.arrayCopAsgn(wrapperType);
        funcBdr.ir.CreateCall(copAsgn, {left, funcBdr.ir.CreateLoad(right)});
      } else {
        llvm::Function *dtor = ctx.inst.arrayDtor(wrapperType);
        funcBdr.ir.CreateCall(dtor, {funcBdr.ir.CreateLoad(left)});
        funcBdr.ir.CreateStore(right, left);
      }
    } else if (ast::StructType *strut = concreteType<ast::StructType>(type)) {
      llvm::Type *structType = left->getType()->getPointerElementType();
      const unsigned members = structType->getNumContainedTypes();
      const bool isPointer = right->getType()->isPointerTy();
      for (unsigned m = 0; m != members; ++m) {
        llvm::Value *dstPtr = funcBdr.ir.CreateStructGEP(left, m);
        llvm::Value *src;
        if (isPointer) {
          src = funcBdr.ir.CreateStructGEP(right, m);
        } else {
          src = funcBdr.ir.CreateExtractValue(right, {m});
        }
        doAssign(strut->fields[m].type.get(), dstPtr, src);
      }
    } else {
      funcBdr.ir.CreateStore(exprBdr.value(right), left);
    }
  }
  
  void visit(ast::Assign &assign) override {
    ast::Type *type = assign.left->exprType.get();
    llvm::Value *left = exprBdr.addr(assign.left.get());
    llvm::Value *right = exprBdr.expr(assign.right.get());
    doAssign(type, left, right);
  }
  void visit(ast::DeclAssign &assign) override {
    assign.llvmAddr = insertVar(assign.symbol, assign.expr.get());
  }
  void visit(ast::CallAssign &assign) override {
    exprBdr.expr(&assign.call);
  }
  
  void insert(ast::FuncParam &param, const size_t index) {
    llvm::Value *arg = funcBdr.args().begin() + index;
    if (param.ref == ast::ParamRef::ref) {
      param.llvmAddr = arg;
    } else if (arg->getType()->isPointerTy()) {
      param.llvmAddr = funcBdr.allocStore(funcBdr.ir.CreateLoad(arg));
    } else {
      param.llvmAddr = funcBdr.allocStore(arg);
    }
  }
  
private:
  gen::Ctx ctx;
  FuncBuilder funcBdr;
  ExprBuilder exprBdr;
  llvm::BasicBlock *breakBlock = nullptr;
  llvm::BasicBlock *continueBlock = nullptr;
};

}

void stela::generateStat(
  gen::Ctx ctx,
  llvm::Function *func,
  ast::Receiver &rec,
  ast::FuncParams &params,
  ast::Block &block
) {
  lowerExpressions(block);
  Visitor visitor{ctx, func};
  // @TODO maybe do parameter insersion in a separate function
  if (rec) {
    visitor.insert(*rec, 0);
  }
  for (size_t p = 0; p != params.size(); ++p) {
    visitor.insert(params[p], p + 1);
  }
  for (const ast::StatPtr &stat : block.nodes) {
    stat->accept(visitor);
  }
}
