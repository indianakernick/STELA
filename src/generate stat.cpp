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
#include "categories.hpp"
#include "generate type.hpp"
#include "generate expr.hpp"
#include "iterator range.hpp"
#include "lifetime exprs.hpp"
#include "function builder.hpp"
#include "lower expressions.hpp"

using namespace stela;

namespace {

struct FlowData {
  llvm::BasicBlock *breakBlock = nullptr;
  llvm::BasicBlock *continueBlock = nullptr;
  size_t scopeIndex = 0;
};

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, llvm::Function *func)
    : ctx{ctx}, funcBdr{func}, lifetime{ctx.inst, funcBdr.ir} {}

  gen::Expr genValue(ast::Expression *expr) {
    return generateValueExpr(scopes.back(), ctx, funcBdr, expr);
  }
  void genExpr(ast::Expression *expr, llvm::Value *result) {
    generateExpr(scopes.back(), ctx, funcBdr, expr, result);
  }
  gen::Expr genExpr(ast::Expression *expr) {
    return generateExpr(scopes.back(), ctx, funcBdr, expr, nullptr);
  }
  void genCondBr(
    ast::Expression *cond,
    llvm::BasicBlock *troo,
    llvm::BasicBlock *fols
  ) {
    const size_t exprScope = enterScope();
    llvm::Value *condExpr = genValue(cond).obj;
    destroy(exprScope);
    leaveScope();
    funcBdr.ir.CreateCondBr(condExpr, troo, fols);
  }

  void visitFlow(ast::Statement *body, const FlowData newFlow) {
    FlowData oldFlow = std::exchange(flow, newFlow);
    body->accept(*this);
    flow = oldFlow;
  }
  // @TODO equality for structs and arrays
  llvm::Value *equalTo(llvm::Value *value, ast::Expression *expr) {
    const ArithCat arith = classifyArith(expr);
    const size_t exprScope = enterScope();
    llvm::Value *equalExpr;
    if (arith == ArithCat::floating_point) {
      equalExpr = funcBdr.ir.CreateFCmpOEQ(value, genValue(expr).obj);
    } else {
      equalExpr = funcBdr.ir.CreateICmpEQ(value, genValue(expr).obj);
    }
    destroy(exprScope);
    leaveScope();
    return equalExpr;
  }
  
  void visit(ast::Block &block) override {
    enterScope();
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
    leaveScope();
  }
  void visit(ast::If &fi) override {
    auto *troo = funcBdr.makeBlock();
    auto *fols = funcBdr.makeBlock();
    llvm::BasicBlock *done = nullptr;
    genCondBr(fi.cond.get(), troo, fols);
    
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
    const size_t exprScope = enterScope();
    gen::Expr value = genValue(swich.expr.get());
    if (swich.cases.empty()) {
      destroy(exprScope);
      leaveScope();
      return;
    }
    
    const size_t caseScope = scopes.size();
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
      llvm::Value *cond = equalTo(value.obj, expr);
      
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
      enterScope();
      visitFlow(swich.cases[c].body.get(), {done, nextBlock, caseScope});
      leaveScope();
      funcBdr.terminate(done);
    }
    
    funcBdr.setCurr(done);
    if (swich.alwaysReturns) {
      funcBdr.ir.CreateUnreachable();
    } else {
      destroy(exprScope);
    }
    leaveScope();
  }
  void visit(ast::Terminate &) override {
    destroy(scopes.size() - 1);
  }
  void visit(ast::Break &) override {
    assert(flow.breakBlock);
    destroy(flow.scopeIndex);
    funcBdr.ir.CreateBr(flow.breakBlock);
  }
  void visit(ast::Continue &) override {
    assert(flow.continueBlock);
    destroy(flow.scopeIndex);
    funcBdr.ir.CreateBr(flow.continueBlock);
  }
  llvm::Value *createReturnObject(ast::Expression *expr, const TypeCat cat) {
    llvm::Value *retObj = nullptr;
    if (cat == TypeCat::trivially_copyable) {
      gen::Expr evalExpr = genExpr(expr);
      if (glvalue(evalExpr.cat)) {
        retObj = funcBdr.ir.CreateLoad(evalExpr.obj);
      } else { // prvalue
        retObj = evalExpr.obj;
      }
    } else if (ast::Identifier *ident = rootLvalue(expr)) {
      // @TODO this is correct but not very efficient
      // maybe we could check the sym::Scope of the definition and the function.
      // we'd also have to check if the identifier refers to a reference parameter
      // lambda captures might complicate things
      llvm::Value *rootAddr = genExpr(ident).obj;
      for (const Scope &scope : scopes) {
        for (const Object obj : scope) {
          if (obj.addr == rootAddr) {
            retObj = &funcBdr.args().back();
            lifetime.moveConstruct(expr->exprType.get(), retObj, genExpr(expr).obj);
          }
        }
      }
    }
    if (retObj == nullptr) {
      retObj = &funcBdr.args().back();
      // @TODO NRVO
      // if there is one object that is always returned from a function,
      //   treat the return pointer as an lvalue of that object
      genExpr(expr, retObj);
    }
    return retObj;
  }
  void returnObject(llvm::Value *retObj, const TypeCat cat) {
    if (retObj->getType()->isVoidTy()) {
      funcBdr.ir.CreateRetVoid();
    } else if (cat == TypeCat::trivially_copyable) {
      funcBdr.ir.CreateRet(retObj);
    } else { // trivially_relocatable or nontrivial
      funcBdr.ir.CreateRetVoid();
    }
  }
  void visit(ast::Return &ret) override {
    if (ret.expr) {
      const TypeCat cat = classifyType(ret.expr->exprType.get());
      llvm::Value *retObj = createReturnObject(ret.expr.get(), cat);
      destroy(0);
      returnObject(retObj, cat);
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
    const size_t scopeIndex = enterScope();
    visitFlow(wile.body.get(), {done, cond, scopeIndex});
    leaveScope();
    funcBdr.terminate(cond);
    funcBdr.setCurr(cond);
    genCondBr(wile.cond.get(), body, done);
    funcBdr.setCurr(done);
  }
  void visit(ast::For &four) override {
    const size_t outerIndex = enterScope();
    if (four.init) {
      four.init->accept(*this);
    }
    auto *cond = funcBdr.nextEmpty();
    auto *body = funcBdr.makeBlock();
    auto *incr = funcBdr.makeBlock();
    auto *done = funcBdr.makeBlock();
    funcBdr.setCurr(body);
    const size_t innerIndex = enterScope();
    visitFlow(four.body.get(), {done, incr, innerIndex});
    leaveScope();
    funcBdr.terminate(incr);
    funcBdr.setCurr(cond);
    genCondBr(four.cond.get(), body, done);
    funcBdr.setCurr(incr);
    if (four.incr) {
      four.incr->accept(*this);
      funcBdr.ir.CreateBr(cond);
    }
    funcBdr.setCurr(done);
    destroy(outerIndex);
    leaveScope();
  }
  
  llvm::Value *insertVar(sym::Object *obj, ast::Expression *expr) {
    ast::Type *type = obj->etype.type.get();
    llvm::Value *addr = funcBdr.alloc(generateType(ctx, type));
    if (expr) {
      const size_t exprScope = enterScope();
      genExpr(expr, addr);
      destroy(exprScope);
      leaveScope();
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
  
  void destroy(const size_t index) {
    for (size_t i = scopes.size() - 1; i != index - 1; --i) {
      for (const Object obj : rev_range(scopes[i])) {
        lifetime.destroy(obj.type, obj.addr);
      }
    }
  }
  
  void visit(ast::Assign &assign) override {
    const size_t exprScope = enterScope();
    ast::Type *type = assign.left->exprType.get();
    gen::Expr left = genExpr(assign.left.get());
    gen::Expr right = genExpr(assign.right.get());
    assert(glvalue(left.cat));
    lifetime.assign(type, left.obj, right);
    destroy(exprScope);
    leaveScope();
  }
  void visit(ast::DeclAssign &assign) override {
    assign.llvmAddr = insertVar(assign.symbol, assign.expr.get());
  }
  void visit(ast::CallAssign &assign) override {
    const size_t exprScope = enterScope();
    genExpr(&assign.call);
    destroy(exprScope);
    leaveScope();
  }
  
  void insert(ast::FuncParam &param, const size_t index) {
    llvm::Value *arg = funcBdr.args().begin() + index;
    if (param.ref == ast::ParamRef::ref) {
      param.llvmAddr = arg;
    } else {
      if (classifyType(param.type.get()) == TypeCat::trivially_copyable) {
        param.llvmAddr = funcBdr.allocStore(arg);
      } else { // trivially_relocatable or nontrivial
        param.llvmAddr = arg;
      }
    }
  }
  
  size_t enterScope() {
    const size_t index = scopes.size();
    scopes.push_back({});
    return index;
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
  FlowData flow;
  std::vector<Scope> scopes;
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
