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
#include "function builder.hpp"
#include "lower expressions.hpp"
#include "generate zero expr.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, llvm::Function *func)
    : ctx{ctx}, builder{func} {}

  void visitFlow(ast::Statement *body, llvm::BasicBlock *brake, llvm::BasicBlock *continu) {
    llvm::BasicBlock *oldBrake = std::exchange(breakBlock, brake);
    llvm::BasicBlock *oldContinue = std::exchange(continueBlock, continu);
    body->accept(*this);
    continueBlock = oldContinue;
    breakBlock = oldBrake;
  }
  llvm::Value *equalTo(llvm::Value *value, ast::Expression *expr) {
    llvm::Value *exprValue = generateValueExpr(ctx, builder, expr);
    const ArithNumber arith = classifyArith(expr);
    if (arith == ArithNumber::floating_point) {
      return builder.ir.CreateFCmpOEQ(value, exprValue);
    } else {
      return builder.ir.CreateICmpEQ(value, exprValue);
    }
  }
  
  void visit(ast::Block &block) override {
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
  }
  void visit(ast::If &fi) override {
    auto *cond = builder.nextEmpty();
    auto *troo = builder.makeBlock();
    auto *folse = builder.makeBlock();
    llvm::BasicBlock *done = nullptr;
    builder.setCurr(cond);
    builder.ir.CreateCondBr(generateValueExpr(ctx, builder, fi.cond.get()), troo, folse);
    
    builder.setCurr(troo);
    fi.body->accept(*this);
    done = builder.terminateLazy(done);
    
    builder.setCurr(folse);
    if (fi.elseBody) {
      fi.elseBody->accept(*this);
    }
    done = builder.terminateLazy(done);
    
    if (done) {
      builder.setCurr(done);
    }
  }
  void visit(ast::Switch &swich) override {
    llvm::Value *value = generateValueExpr(ctx, builder, swich.expr.get());
    if (swich.cases.empty()) {
      return;
    }
    
    auto checkBlocks = builder.makeBlocks(swich.cases.size());
    auto caseBlocks = builder.makeBlocks(swich.cases.size());
    llvm::BasicBlock *done = builder.makeBlock();
    size_t defaultIndex = ~size_t{};
    bool foundDefault = false;
    builder.ir.CreateBr(checkBlocks[0]);
    
    for (size_t c = 0; c != swich.cases.size(); ++c) {
      ast::Expression *expr = swich.cases[c].expr.get();
      if (!expr) {
        defaultIndex = c;
        foundDefault = true;
        continue;
      }
      const size_t checkIndex = c - foundDefault;
      const size_t caseIndex = c;
      
      builder.setCurr(checkBlocks[checkIndex]);
      llvm::Value *cond = equalTo(value, expr);
      
      llvm::BasicBlock *nextCheck;
      if (!foundDefault && c == swich.cases.size() - 1) {
        nextCheck = done;
      } else {
        nextCheck = checkBlocks[checkIndex + 1];
      }
      
      builder.ir.CreateCondBr(cond, caseBlocks[caseIndex], nextCheck);
    }
    
    if (foundDefault) {
      builder.setCurr(checkBlocks.back());
      builder.ir.CreateBr(caseBlocks[defaultIndex]);
    }
    
    for (size_t c = 0; c != swich.cases.size(); ++c) {
      builder.setCurr(caseBlocks[c]);
      llvm::BasicBlock *nextBlock = nullptr;
      if (c != swich.cases.size() - 1) {
        nextBlock = caseBlocks[c + 1];
      }
      visitFlow(swich.cases[c].body.get(), done, nextBlock);
      builder.terminate(done);
    }
    
    builder.setCurr(done);
    if (swich.alwaysReturns) {
      builder.ir.CreateUnreachable();
    }
  }
  void visit(ast::Break &) override {
    assert(breakBlock);
    builder.ir.CreateBr(breakBlock);
  }
  void visit(ast::Continue &) override {
    assert(continueBlock);
    builder.ir.CreateBr(continueBlock);
  }
  void visit(ast::Return &ret) override {
    if (ret.expr) {
      llvm::Value *value = generateValueExpr(ctx, builder, ret.expr.get());
      if (value->getType()->isVoidTy()) {
        builder.ir.CreateRetVoid();
      } else if (value->getType()->isStructTy()) {
        builder.ir.CreateStore(value, &builder.args().back());
        builder.ir.CreateRetVoid();
      } else {
        builder.ir.CreateRet(value);
      }
    } else {
      builder.ir.CreateRetVoid();
    }
  }
  void visit(ast::While &wile) override {
    auto *cond = builder.nextEmpty();
    auto *body = builder.makeBlock();
    auto *done = builder.makeBlock();
    builder.setCurr(body);
    visitFlow(wile.body.get(), done, cond);
    builder.terminate(cond);
    builder.setCurr(cond);
    builder.ir.CreateCondBr(generateValueExpr(ctx, builder, wile.cond.get()), body, done);
    builder.setCurr(done);
  }
  void visit(ast::For &four) override {
    if (four.init) {
      four.init->accept(*this);
    }
    auto *cond = builder.nextEmpty();
    auto *body = builder.makeBlock();
    auto *incr = builder.makeBlock();
    auto *done = builder.makeBlock();
    builder.setCurr(body);
    visitFlow(four.body.get(), done, incr);
    builder.terminate(incr);
    builder.setCurr(cond);
    builder.ir.CreateCondBr(generateValueExpr(ctx, builder, four.cond.get()), body, done);
    builder.setCurr(incr);
    if (four.incr) {
      four.incr->accept(*this);
      builder.ir.CreateBr(cond);
    }
    builder.setCurr(done);
  }
  
  llvm::Type *genType(ast::Type *type) {
    return generateType(ctx, type);
  }
  llvm::Value *insertVar(ast::Type *type, ast::Expression *expr) {
    if (expr) {
      return builder.allocStore(genType(type), generateValueExpr(ctx, builder, expr));
    } else {
      return builder.allocStore(genType(type), generateZeroExpr(ctx, builder, type));
    }
  }
  
  void visit(ast::Var &var) override {
    var.llvmAddr = insertVar(var.symbol->etype.type.get(), var.expr.get());
  }
  void visit(ast::Let &let) override {
    let.llvmAddr = insertVar(let.symbol->etype.type.get(), let.expr.get());
  }
  
  void visit(ast::Assign &assign) override {
    llvm::Value *addr = generateAddrExpr(ctx, builder, assign.left.get());
    llvm::Value *value = generateValueExpr(ctx, builder, assign.right.get());
    builder.ir.CreateStore(value, addr);
  }
  void visit(ast::DeclAssign &assign) override {
    assign.llvmAddr = insertVar(assign.symbol->etype.type.get(), assign.expr.get());
  }
  void visit(ast::CallAssign &assign) override {
    generateDiscardExpr(ctx, builder, &assign.call);
  }
  
  void insert(ast::FuncParam &param, const size_t index) {
    llvm::Value *arg = builder.args().begin() + index;
    if (param.ref == ast::ParamRef::ref) {
      param.llvmAddr = arg;
    } else if (arg->getType()->isPointerTy()) {
      llvm::Value *localCopy = builder.ir.CreateLoad(arg);
      param.llvmAddr = builder.allocStore(localCopy->getType(), localCopy);
    } else {
      param.llvmAddr = builder.allocStore(genType(param.type.get()), arg);
    }
  }
  
private:
  gen::Ctx ctx;
  FunctionBuilder builder;
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
