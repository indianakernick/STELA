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
#include "generate expr.hpp"
#include "generate type.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/BasicBlock.h>
#include "generate zero expr.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, llvm::Function *func)
    : ctx{ctx}, func{func}, builder{getLLVM()} {
    builder.SetInsertPoint(llvm::BasicBlock::Create(getLLVM(), "entry", func));
    currBlock = llvm::BasicBlock::Create(getLLVM(), "", func);
    builder.CreateBr(currBlock);
    builder.SetInsertPoint(currBlock);
  }

  /*void ensureCurrBlock() {
    if (!currBlock) {
      currBlock = llvm::BasicBlock::Create(getLLVM(), "", func);
      builder.SetInsertPoint(currBlock);
    }
  }*/

  void visitFlow(ast::Statement *body, llvm::BasicBlock *brake, llvm::BasicBlock *continu) {
    llvm::BasicBlock *oldBrake = std::exchange(breakBlock, brake);
    llvm::BasicBlock *oldContinue = std::exchange(continueBlock, continu);
    body->accept(*this);
    continueBlock = oldContinue;
    breakBlock = oldBrake;
  }
  
  void visit(ast::Block &block) override {
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
  }
  void visit(ast::If &fi) override {
    auto *troo = llvm::BasicBlock::Create(getLLVM(), "", func);
    auto *folse = llvm::BasicBlock::Create(getLLVM(), "", func);
    builder.CreateCondBr(generateExpr(ctx, builder, fi.cond.get()), troo, folse);
    setCurr(troo);
    fi.body->accept(*this);
    setCurr(folse);
    if (fi.elseBody) {
      fi.elseBody->accept(*this);
    }
  }
  void visit(ast::Switch &swich) override {
    
  }
  void visit(ast::Break &) override {
    assert(breakBlock);
    builder.CreateBr(breakBlock);
  }
  void visit(ast::Continue &) override {
    assert(continueBlock);
    builder.CreateBr(continueBlock);
  }
  void visit(ast::Return &ret) override {
    if (ret.expr) {
      builder.CreateRet(generateExpr(ctx, builder, ret.expr.get()));
    } else {
      builder.CreateRetVoid();
    }
  }
  void visit(ast::While &wile) override {
    auto *cond = nextEmpty();
    auto *body = llvm::BasicBlock::Create(getLLVM(), "", func);
    auto *done = llvm::BasicBlock::Create(getLLVM(), "", func);
    setCurr(body);
    visitFlow(wile.body.get(), done, cond);
    if (currBlock->empty() || !currBlock->back().isTerminator()) {
      builder.CreateBr(cond);
    }
    setCurr(cond);
    builder.CreateCondBr(generateExpr(ctx, builder, wile.cond.get()), body, done);
    setCurr(done);
  }
  void visit(ast::For &four) override {}
  
  llvm::Value *insertVar(ast::Type *type, ast::Expression *expr) {
    llvm::BasicBlock *entry = &func->getEntryBlock();
    builder.SetInsertPoint(entry, entry->begin());
    llvm::Type *llvmType = generateType(ctx, type);
    llvm::Value *llvmAddr = builder.CreateAlloca(llvmType);
    builder.SetInsertPoint(currBlock);
    if (expr) {
      builder.CreateStore(generateExpr(ctx, builder, expr), llvmAddr);
    } else {
      builder.CreateStore(generateZeroExpr(ctx, builder, type), llvmAddr);
    }
    return llvmAddr;
  }
  void visit(ast::Var &var) override {
    var.llvmAddr = insertVar(var.symbol->etype.type.get(), var.expr.get());
  }
  void visit(ast::Let &let) override {
    let.llvmAddr = insertVar(let.symbol->etype.type.get(), let.expr.get());
  }
  
  void visit(ast::IncrDecr &assign) override {
    //llvm::Value *addr = generateExpr(ctx, builder, assign.expr.get());
  }
  void visit(ast::Assign &assign) override {
    llvm::Value *addr = generateExpr(ctx, builder, assign.left.get());
    llvm::Value *value = generateExpr(ctx, builder, assign.right.get());
    builder.CreateStore(value, addr);
  }
  void visit(ast::DeclAssign &assign) override {
    assign.llvmAddr = insertVar(assign.symbol->etype.type.get(), assign.expr.get());
  }
  void visit(ast::CallAssign &assign) override {
    generateExpr(ctx, builder, &assign.call);
  }
  
private:
  gen::Ctx ctx;
  llvm::Function *func;
  llvm::IRBuilder<> builder;
  llvm::BasicBlock *breakBlock = nullptr;
  llvm::BasicBlock *continueBlock = nullptr;
  llvm::BasicBlock *currBlock = nullptr;
  
  void setCurr(llvm::BasicBlock *block) {
    currBlock = block;
    builder.SetInsertPoint(block);
  }
  
  llvm::BasicBlock *nextEmpty() {
    llvm::BasicBlock *nextBlock;
    if (currBlock->empty()) {
      nextBlock = currBlock;
    } else {
      nextBlock = llvm::BasicBlock::Create(getLLVM(), "", func);
      builder.CreateBr(nextBlock);
    }
    return nextBlock;
  }
};

}

void stela::generateStat(gen::Ctx ctx, llvm::Function *func, ast::Block &block) {
  Visitor visitor{ctx, func};
  for (const ast::StatPtr &stat : block.nodes) {
    stat->accept(visitor);
  }
}

int idoLoop(int x, int y) {
  int z = 0;

  while (x < 24) {
    x *= 3;
    while (y < 81) {
      y *= x;
    }
    z += y;
    y = x;
  }
  return z;
}

int asmLoop(int x, int y) {
  int z = 0;

  outer_loop_cond:
  if (x < 24) goto outer_loop_begin; else goto outer_loop_end;
  outer_loop_begin:
    x = x * 3;
    inner_loop_cond:
    if (y < 31) goto inner_loop_begin; else goto inner_loop_end;
    inner_loop_begin:
      y = y * x;
      goto inner_loop_cond;
    inner_loop_end:
    z = z + y;
    y = x;
  goto outer_loop_cond;
  outer_loop_end:
  
  return z;
}

int ido_llvm(int x, int y) {
  int z = 1;
  goto label_6;

  label_6:
  if (x < 24) goto label_9; else goto label_24;

  label_9:
  x = x * 3;
  goto label_12;

  label_12:
  if (y < 81) goto label_15; else goto label_19;

  label_15:
  y = y * x;
  goto label_12;

  label_19:
  z = z + y;
  y = x;
  goto label_6;

  label_24:
  return z;
}

int plain(int x) {
  int y = 1;
  while (x < 19) {
    y *= x;
    ++x;
  }
  return y;
}

int plain_llvm(int x) {
  int y = 1;
  goto label_4;
  
  label_4:
  if (x < 19) goto label_7; else goto label_13;
  
  label_7:
  y = y * x;
  x = x + 1;
  goto label_4;
  
  label_13:
  return y;
}

int doublePlain(int x) {
  int y = 1;
  while (x < 19) {
    y *= x;
    ++x;
  }
  while (y > 4) {
    y /= x;
  }
  return x + y;
}

int doublePlain_llvm(int x) {
  int y = 1;
  goto label_4;
  
  label_4:
  if (x < 19) goto label_7; else goto label_13;
  
  label_7:
  y = y * x;
  x = x + 1;
  goto label_4;
  
  label_13:
  goto label_14;
  
  label_14:
  if (y > 4) goto label_17; else goto label_21;
  
  label_17:
  y = y / x;
  goto label_14;
  
  label_21:
  return x + y;
}