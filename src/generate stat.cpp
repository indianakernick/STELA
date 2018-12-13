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
    : ctx{ctx}, func{func}, builder{ctx.llvm} {
    builder.SetInsertPoint(llvm::BasicBlock::Create(ctx.llvm, "entry", func));
    currBlock = llvm::BasicBlock::Create(ctx.llvm, "", func);
    builder.CreateBr(currBlock);
    builder.SetInsertPoint(currBlock);
  }

  /*void ensureCurrBlock() {
    if (!currBlock) {
      currBlock = llvm::BasicBlock::Create(ctx.llvm, "", func);
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
  void appendBr(llvm::BasicBlock *dest) {
    if (currBlock->empty() || !currBlock->back().isTerminator()) {
      builder.CreateBr(dest);
    }
  }
  llvm::BasicBlock *lazyAppendBr(llvm::BasicBlock *dest) {
    if (currBlock->empty() || !currBlock->back().isTerminator()) {
      if (!dest) {
        dest = llvm::BasicBlock::Create(ctx.llvm, "", func);
      }
      builder.CreateBr(dest);
    }
    return dest;
  }
  std::vector<llvm::BasicBlock *> makeBlocks(size_t count) {
    std::vector<llvm::BasicBlock *> blocks;
    blocks.reserve(count);
    while (count--) {
      blocks.push_back(llvm::BasicBlock::Create(ctx.llvm, "", func));
    }
    return blocks;
  }
  llvm::Value *equalTo(llvm::Value *value, ast::Expression *expr) {
    llvm::Value *exprValue = generateValueExpr(ctx, builder, expr);
    const ArithNumber arith = classifyArith(expr);
    if (arith == ArithNumber::floating_point) {
      return builder.CreateFCmpOEQ(value, exprValue);
    } else {
      return builder.CreateICmpEQ(value, exprValue);
    }
  }
  
  void visit(ast::Block &block) override {
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
  }
  void visit(ast::If &fi) override {
    auto *cond = nextEmpty();
    auto *troo = llvm::BasicBlock::Create(ctx.llvm, "", func);
    auto *folse = llvm::BasicBlock::Create(ctx.llvm, "", func);
    llvm::BasicBlock *done = nullptr;
    setCurr(cond);
    builder.CreateCondBr(generateValueExpr(ctx, builder, fi.cond.get()), troo, folse);
    
    setCurr(troo);
    fi.body->accept(*this);
    done = lazyAppendBr(done);
    
    setCurr(folse);
    if (fi.elseBody) {
      fi.elseBody->accept(*this);
    }
    done = lazyAppendBr(done);
    
    if (done) {
      setCurr(done);
    }
    
    /*auto *troo = llvm::BasicBlock::Create(ctx.llvm, "", func);
    auto *folse = llvm::BasicBlock::Create(ctx.llvm, "", func);
    builder.SetInsertPoint(currBlock);
    builder.CreateCondBr(generateValueExpr(ctx, builder, fi.cond.get()), troo, folse);
    auto *done = llvm::BasicBlock::Create(ctx.llvm);
    setCurr(troo);
    fi.body->accept(*this);
    appendBr(done);
    setCurr(folse);
    if (fi.elseBody) {
      fi.elseBody->accept(*this);
    }
    appendBr(done);
    setCurr(done);*/
  }
  void visit(ast::Switch &swich) override {
    /*
    
    switch (expr) {
      case (case_expr_0) {
        // do stuff 0
      }
      default {
        // do stuff 2
      }
      case (case_expr_1) {
        // do stuff 1
      }
    }
    
    const value = expr;
    
    check_block_0:
    if (value == case_expr_0) goto case_block_0; else goto check_block_1;
    
    check_block_1:
    if (value == case_expr_1) goto case_block_2; else goto check_block_2;
    
    check_block_2:
    goto case_block_1
    
    case_block_0:
    // do stuff 0
    goto done;
    
    case_block_1:
    // do stuff 2
    goto done;
    
    case_block_2:
    // do stuff 1
    goto done;
    
    done:
    
    */
    llvm::Value *value = generateValueExpr(ctx, builder, swich.expr.get());
    if (swich.cases.empty()) {
      return;
    }
    
    std::vector<llvm::BasicBlock *> checkBlocks = makeBlocks(swich.cases.size());
    std::vector<llvm::BasicBlock *> caseBlocks = makeBlocks(swich.cases.size());
    llvm::BasicBlock *done = llvm::BasicBlock::Create(ctx.llvm, "", func);
    size_t defaultIndex = ~size_t{};
    bool foundDefault = false;
    builder.CreateBr(checkBlocks[0]);
    
    for (size_t c = 0; c != swich.cases.size(); ++c) {
      ast::Expression *expr = swich.cases[c].expr.get();
      if (!expr) {
        defaultIndex = c;
        foundDefault = true;
        continue;
      }
      const size_t checkIndex = c - foundDefault;
      const size_t caseIndex = c;
      
      setCurr(checkBlocks[checkIndex]);
      llvm::Value *cond = equalTo(value, expr);
      
      llvm::BasicBlock *nextCheck;
      if (!foundDefault && c == swich.cases.size() - 1) {
        nextCheck = done;
      } else {
        nextCheck = checkBlocks[checkIndex + 1];
      }
      
      builder.CreateCondBr(cond, caseBlocks[caseIndex], nextCheck);
    }
    
    if (foundDefault) {
      setCurr(checkBlocks.back());
      builder.CreateBr(caseBlocks[defaultIndex]);
    }
    
    for (size_t c = 0; c != swich.cases.size(); ++c) {
      setCurr(caseBlocks[c]);
      llvm::BasicBlock *nextBlock = nullptr;
      if (c != swich.cases.size() - 1) {
        nextBlock = caseBlocks[c + 1];
      }
      visitFlow(swich.cases[c].body.get(), done, nextBlock);
      appendBr(done);
    }
    
    setCurr(done);
    if (swich.alwaysReturns) {
      builder.CreateUnreachable();
    }
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
      llvm::Value *value = generateValueExpr(ctx, builder, ret.expr.get());
      if (value->getType()->isVoidTy()) {
        builder.CreateRetVoid();
      } else {
        builder.CreateRet(value);
      }
    } else {
      builder.CreateRetVoid();
    }
  }
  void visit(ast::While &wile) override {
    auto *cond = nextEmpty();
    auto *body = llvm::BasicBlock::Create(ctx.llvm, "", func);
    auto *done = llvm::BasicBlock::Create(ctx.llvm, "", func);
    setCurr(body);
    visitFlow(wile.body.get(), done, cond);
    appendBr(cond);
    setCurr(cond);
    builder.CreateCondBr(generateValueExpr(ctx, builder, wile.cond.get()), body, done);
    setCurr(done);
  }
  void visit(ast::For &four) override {
    if (four.init) {
      four.init->accept(*this);
    }
    auto *cond = nextEmpty();
    auto *body = llvm::BasicBlock::Create(ctx.llvm, "", func);
    auto *incr = llvm::BasicBlock::Create(ctx.llvm, "", func);
    auto *done = llvm::BasicBlock::Create(ctx.llvm, "", func);
    setCurr(body);
    visitFlow(four.body.get(), done, incr);
    appendBr(incr);
    setCurr(cond);
    builder.CreateCondBr(generateValueExpr(ctx, builder, four.cond.get()), body, done);
    setCurr(incr);
    if (four.incr) {
      four.incr->accept(*this);
      builder.CreateBr(cond);
    }
    setCurr(done);
  }
  
  llvm::Value *insertAlloca(ast::Type *type) {
    llvm::BasicBlock *entry = &func->getEntryBlock();
    builder.SetInsertPoint(entry, std::prev(entry->end()));
    llvm::Type *llvmType = generateType(ctx, type);
    llvm::Value *llvmAddr = builder.CreateAlloca(llvmType);
    builder.SetInsertPoint(currBlock);
    return llvmAddr;
  }
  llvm::Value *insertVar(ast::Type *type, ast::Expression *expr) {
    llvm::Value *llvmAddr = insertAlloca(type);
    if (expr) {
      builder.CreateStore(generateValueExpr(ctx, builder, expr), llvmAddr);
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
    //llvm::Value *addr = generateAddrExpr(ctx, builder, assign.expr.get());
    //classifyArrith(assign.expr->exprType.get())
  }
  void visit(ast::Assign &assign) override {
    llvm::Value *addr = generateAddrExpr(ctx, builder, assign.left.get());
    llvm::Value *value = generateValueExpr(ctx, builder, assign.right.get());
    builder.CreateStore(value, addr);
  }
  void visit(ast::DeclAssign &assign) override {
    assign.llvmAddr = insertVar(assign.symbol->etype.type.get(), assign.expr.get());
  }
  void visit(ast::CallAssign &assign) override {
    generateAddrExpr(ctx, builder, &assign.call);
  }
  
  void insert(ast::FuncParam &param, const size_t index) {
    if (param.ref == ast::ParamRef::ref) {
      param.llvmAddr = func->arg_begin() + index;
    } else {
      param.llvmAddr = insertAlloca(param.type.get());
      builder.CreateStore(func->arg_begin() + index, param.llvmAddr);
    }
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
      nextBlock = llvm::BasicBlock::Create(ctx.llvm, "", func);
      builder.CreateBr(nextBlock);
    }
    return nextBlock;
  }
};

}

void stela::generateStat(
  gen::Ctx ctx,
  llvm::Function *func,
  ast::Receiver &rec,
  ast::FuncParams &params,
  ast::Block &block
) {
  Visitor visitor{ctx, func};
  // @TODO maybe do parameter insersion in a separate function
  if (rec) {
    visitor.insert(rec.value(), 0);
  }
  for (size_t p = 0; p != params.size(); ++p) {
    visitor.insert(params[p], p + 1);
  }
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
