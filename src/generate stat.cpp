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
#include "lifetime exprs.hpp"
#include "function builder.hpp"
#include "lower expressions.hpp"
#include "expression builder.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, llvm::Function *func)
    : ctx{ctx}, funcBdr{func}, exprBdr{ctx, funcBdr}, lifetime{ctx, funcBdr.ir} {}

  struct Object {
    llvm::Value *addr;
    ast::Type *type;
  };
  using Scope = std::vector<Object>;
  using ScopeStack = std::vector<Scope>;

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
    enterScope();
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
    leaveScope();
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
  void visit(ast::Terminate &) override {
    destroy(scopes.size() - 1);
  }
  void visit(ast::Break &) override {
    assert(breakBlock);
    destroy(breakIndex);
    funcBdr.ir.CreateBr(breakBlock);
  }
  void visit(ast::Continue &) override {
    assert(continueBlock);
    destroy(continueIndex);
    funcBdr.ir.CreateBr(continueBlock);
  }
  void visit(ast::Return &ret) override {
    if (ret.expr) {
      llvm::Value *value = exprBdr.expr(ret.expr.get());
      llvm::Type *type = value->getType();
      if (type->isPointerTy()) {
        llvm::Value *retPtr = funcBdr.alloc(type->getPointerElementType());
        lifetime.moveConstruct(ret.expr->exprType.get(), retPtr, value);
        value = funcBdr.ir.CreateLoad(retPtr);
      }
      destroy(0);
      if (value->getType()->isVoidTy()) {
        funcBdr.ir.CreateRetVoid();
      } else if (value->getType()->isStructTy()) {
        funcBdr.ir.CreateStore(value, &funcBdr.args().back());
        funcBdr.ir.CreateRetVoid();
      } else {
        funcBdr.ir.CreateRet(value);
      }
    } else {
      destroy(0);
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
    ast::Type *type = obj->etype.type.get();
    llvm::Value *addr = funcBdr.alloc(generateType(ctx, type));
    if (expr) {
      lifetime.relocate(type, addr, exprBdr.addr(expr));
    } else {
      lifetime.defConstruct(type, addr);
    }
    pushObj({addr, type});
    return addr;
  }
  
  void visit(ast::Var &var) override {
    var.llvmAddr = insertVar(var.symbol, var.expr.get());
  }
  void visit(ast::Let &let) override {
    let.llvmAddr = insertVar(let.symbol, let.expr.get());
  }
  
  void doAssign(ast::Type *type, llvm::Value *left, llvm::Value *right) {
    if (right->getType()->isPointerTy()) {
      lifetime.copyAssign(type, left, right);
    } else {
      llvm::Value *rightPtr = exprBdr.addr(right);
      lifetime.moveAssign(type, left, rightPtr);
      lifetime.destroy(type, rightPtr);
    }
  }
  void destroy(const Object object) {
    lifetime.destroy(object.type, object.addr);
  }
  void destroy(const size_t index) {
    for (size_t i = scopes.size() - 1; i != index - 1; --i) {
      for (const Object obj : scopes[i]) {
        destroy(obj);
      }
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
    if (param.ref == ast::ParamRef::val) {
      pushObj({param.llvmAddr, param.type.get()});
    }
  }
  
  void enterScope() {
    scopes.push_back({});
  }
  void leaveScope() {
    scopes.pop_back();
  }
  void pushObj(const Object object) {
    scopes.back().push_back(object);
  }
  
private:
  gen::Ctx ctx;
  FuncBuilder funcBdr;
  ExprBuilder exprBdr;
  llvm::BasicBlock *breakBlock = nullptr;
  llvm::BasicBlock *continueBlock = nullptr;
  size_t breakIndex = 0;
  size_t continueIndex = 0;
  ScopeStack scopes;
  LifetimeExpr lifetime;
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
  visitor.enterScope();
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
  visitor.leaveScope();
}
