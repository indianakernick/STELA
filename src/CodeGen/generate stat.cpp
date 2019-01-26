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
#include "compare exprs.hpp"
#include "generate type.hpp"
#include "generate expr.hpp"
#include "lifetime exprs.hpp"
#include "function builder.hpp"
#include "lower expressions.hpp"
#include "Utils/iterator range.hpp"
#include "Semantic/scope traverse.hpp"

using namespace stela;

namespace {

struct FlowData {
  llvm::BasicBlock *breakBlock = nullptr;
  llvm::BasicBlock *continueBlock = nullptr;
  size_t scopeIndex = 0;
};

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, gen::Func func)
    : ctx{ctx},
      builder{func.builder},
      lifetime{ctx.inst, builder.ir},
      closure{func.closure},
      symbol{func.symbol} {}

  gen::Func makeFunc() {
    return {builder, closure, symbol};
  }
  gen::Expr genBool(ast::Expression *expr) {
    return generateBoolExpr(scopes.back(), ctx, makeFunc(), expr);
  }
  void genExpr(ast::Expression *expr, llvm::Value *result) {
    generateExpr(scopes.back(), ctx, makeFunc(), expr, result);
  }
  gen::Expr genExpr(ast::Expression *expr) {
    return generateExpr(scopes.back(), ctx, makeFunc(), expr, nullptr);
  }
  void genCondBr(
    ast::Expression *cond,
    llvm::BasicBlock *troo,
    llvm::BasicBlock *fols
  ) {
    const size_t exprScope = enterScope();
    llvm::Value *condExpr = genBool(cond).obj;
    destroy(exprScope);
    leaveScope();
    builder.ir.CreateCondBr(condExpr, troo, fols);
  }

  void visitFlow(ast::Statement *body, const FlowData newFlow) {
    FlowData oldFlow = std::exchange(flow, newFlow);
    body->accept(*this);
    flow = oldFlow;
  }
  llvm::Value *equalTo(llvm::Value *value, ast::Expression *expr, llvm::Type *type) {
    CompareExpr compare{ctx.inst, builder.ir};
    const size_t exprScope = enterScope();
    llvm::Value *exprAddr = builder.alloc(type);
    genExpr(expr, exprAddr);
    pushObj({exprAddr, expr->exprType.get()});
    llvm::Value *equalExpr = compare.eq(
      expr->exprType.get(),
      {value, ValueCat::lvalue},
      {exprAddr, ValueCat::lvalue}
    );
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
    auto *troo = builder.makeBlock();
    auto *fols = builder.makeBlock();
    llvm::BasicBlock *done = nullptr;
    genCondBr(fi.cond.get(), troo, fols);
    
    builder.setCurr(troo);
    fi.body->accept(*this);
    done = builder.terminateLazy(done);
    
    builder.setCurr(fols);
    if (fi.elseBody) {
      fi.elseBody->accept(*this);
    }
    done = builder.terminateLazy(done);
    
    if (done) {
      builder.setCurr(done);
    }
  }
  
  using Blocks = std::vector<llvm::BasicBlock *>;
  static constexpr size_t nodefault = ~size_t{};
  
  size_t findDefault(ast::Switch &swich) {
    for (size_t c = 0; c != swich.cases.size(); ++c) {
      if (swich.cases[c].expr == nullptr) {
        return c;
      }
    }
    return nodefault;
  }
  bool pastDefaultCase(const size_t caseIndex, const size_t defaultIndex) {
    return defaultIndex != nodefault && caseIndex > defaultIndex;
  }
  size_t checkIndexOfCase(const size_t caseIndex, const size_t defaultIndex) {
    return caseIndex - pastDefaultCase(caseIndex, defaultIndex);
  }
  
  void emitCaseChecks(
    ast::Switch &swich,
    const Blocks &checkBlocks,
    const Blocks &caseBlocks,
    llvm::BasicBlock *done,
    llvm::Value *value,
    const size_t defaultIndex
  ) {
    llvm::Type *type = value->getType()->getPointerElementType();
    for (size_t c = 0; c != swich.cases.size(); ++c) {
      ast::Expression *expr = swich.cases[c].expr.get();
      if (!expr) continue;
      
      const size_t checkIndex = checkIndexOfCase(c, defaultIndex);
      builder.setCurr(checkBlocks[checkIndex]);
      llvm::Value *cond = equalTo(value, expr, type);
    
      llvm::BasicBlock *nextCheck;
      if (!pastDefaultCase(c, defaultIndex) && c == swich.cases.size() - 1) {
        nextCheck = done;
      } else {
        nextCheck = checkBlocks[checkIndex + 1];
      }
    
      builder.ir.CreateCondBr(cond, caseBlocks[c], nextCheck);
    }
  }
  void emitCaseBodies(
    ast::Switch &swich,
    const Blocks &blocks,
    llvm::BasicBlock *done,
    const size_t scope
  ) {
    const size_t last = swich.cases.size() - 1;
    for (size_t c = 0; c != swich.cases.size(); ++c) {
      builder.setCurr(blocks[c]);
      enterScope();
      llvm::BasicBlock *next = (c == last ? nullptr : blocks[c + 1]);
      visitFlow(swich.cases[c].body.get(), {done, next, scope});
      leaveScope();
      builder.terminate(done);
    }
  }
  void visit(ast::Switch &swich) override {
    const size_t exprScope = enterScope();
    llvm::Type *type = generateType(ctx.llvm, swich.expr->exprType.get());
    llvm::Value *value = builder.alloc(type);
    genExpr(swich.expr.get(), value);
    if (swich.cases.empty()) {
      destroy(exprScope);
      leaveScope();
      return;
    }
    
    const size_t caseScope = scopes.size();
    auto checkBlocks = builder.makeBlocks(swich.cases.size());
    auto caseBlocks = builder.makeBlocks(swich.cases.size());
    llvm::BasicBlock *done = builder.makeBlock();
    builder.ir.CreateBr(checkBlocks[0]);
    
    const size_t defaultIndex = findDefault(swich);
    emitCaseChecks(swich, checkBlocks, caseBlocks, done, value, defaultIndex);
    
    if (defaultIndex != nodefault) {
      builder.link(checkBlocks.back(), caseBlocks[defaultIndex]);
    }
    
    emitCaseBodies(swich, caseBlocks, done, caseScope);
    
    builder.setCurr(done);
    if (swich.alwaysReturns) {
      builder.ir.CreateUnreachable();
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
    builder.ir.CreateBr(flow.breakBlock);
  }
  void visit(ast::Continue &) override {
    assert(flow.continueBlock);
    destroy(flow.scopeIndex);
    builder.ir.CreateBr(flow.continueBlock);
  }
  llvm::Value *trivialReturnObject(ast::Expression *expr) {
    gen::Expr evalExpr = genExpr(expr);
    if (glvalue(evalExpr.cat)) {
      return builder.ir.CreateLoad(evalExpr.obj);
    } else { // prvalue
      return evalExpr.obj;
    }
  }
  sym::Object *getObject(ast::Statement *def) {
    if (auto *param = dynamic_cast<ast::FuncParam *>(def)) {
      if (param->ref == ast::ParamRef::val) {
        return param->symbol;
      } else {
        return nullptr;
      }
    } else if (auto *decl = dynamic_cast<ast::DeclAssign *>(def)) {
      return decl->symbol;
    } else if (auto *var = dynamic_cast<ast::Var *>(def)) {
      return var->symbol;
    } else if (auto *let = dynamic_cast<ast::Let *>(def)) {
      return let->symbol;
    }
    UNREACHABLE();
  }
  bool canMoveReturn(sym::Object *object) {
    sym::Scope *objectScope = object->scope;
    assert(objectScope);
    sym::Scope *funcScope = symbol->scope;
    assert(funcScope);
    return findNearest(funcScope->type, objectScope) == funcScope;
  }
  llvm::Value *createReturnObject(ast::Expression *expr, const TypeCat cat) {
    if (cat == TypeCat::trivially_copyable) {
      return trivialReturnObject(expr);
    }
    if (ast::Identifier *ident = rootLvalue(expr)) {
      sym::Object *object = getObject(ident->definition);
      if (object && canMoveReturn(object)) {
        llvm::Value *retObj = &builder.args().back();
        lifetime.moveConstruct(expr->exprType.get(), retObj, genExpr(expr).obj);
        return retObj;
      }
    }
    llvm::Value *retObj = &builder.args().back();
    // @TODO NRVO
    // if there is one object that is always returned from a function,
    //   treat the return pointer as an lvalue of that object
    genExpr(expr, retObj);
    return retObj;
  }
  void returnObject(llvm::Value *retObj, const TypeCat cat) {
    if (retObj->getType()->isVoidTy()) {
      builder.ir.CreateRetVoid();
    } else if (cat == TypeCat::trivially_copyable) {
      builder.ir.CreateRet(retObj);
    } else { // trivially_relocatable or nontrivial
      builder.ir.CreateRetVoid();
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
      builder.ir.CreateRetVoid();
    }
  }
  void visit(ast::While &wile) override {
    auto *cond = builder.nextEmpty();
    auto *body = builder.makeBlock();
    auto *done = builder.makeBlock();
    builder.setCurr(body);
    const size_t scopeIndex = enterScope();
    visitFlow(wile.body.get(), {done, cond, scopeIndex});
    leaveScope();
    builder.terminate(cond);
    builder.setCurr(cond);
    genCondBr(wile.cond.get(), body, done);
    builder.setCurr(done);
  }
  void visit(ast::For &four) override {
    const size_t outerIndex = enterScope();
    if (four.init) {
      four.init->accept(*this);
    }
    auto *cond = builder.nextEmpty();
    auto *body = builder.makeBlock();
    auto *incr = builder.makeBlock();
    auto *done = builder.makeBlock();
    builder.setCurr(body);
    const size_t innerIndex = enterScope();
    visitFlow(four.body.get(), {done, incr, innerIndex});
    leaveScope();
    builder.terminate(incr);
    builder.setCurr(cond);
    genCondBr(four.cond.get(), body, done);
    builder.setCurr(incr);
    if (four.incr) {
      four.incr->accept(*this);
    }
    builder.ir.CreateBr(cond);
    builder.setCurr(done);
    destroy(outerIndex);
    leaveScope();
  }
  
  llvm::Value *insertVar(sym::Object *obj, ast::Expression *expr) {
    ast::Type *type = obj->etype.type.get();
    llvm::Value *addr = builder.alloc(generateType(ctx.llvm, type));
    if (expr) {
      const size_t exprScope = enterScope();
      if (isBoolCast(type, expr->exprType.get())) {
        builder.ir.CreateStore(genBool(expr).obj, addr);
        // @TODO lifetime.startLife
      } else {
        genExpr(expr, addr);
      }
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
    ast::Type *type = assign.dst->exprType.get();
    gen::Expr dst = genExpr(assign.dst.get());
    gen::Expr src = genExpr(assign.src.get());
    assert(glvalue(dst.cat));
    lifetime.assign(type, dst.obj, src);
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
    llvm::Value *arg = builder.args().begin() + index;
    if (param.ref == ast::ParamRef::ref) {
      param.llvmAddr = arg;
    } else {
      if (classifyType(param.type.get()) == TypeCat::trivially_copyable) {
        param.llvmAddr = builder.allocStore(arg);
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
  FuncBuilder builder;
  FlowData flow;
  std::vector<Scope> scopes;
  LifetimeExpr lifetime;
  llvm::Value *closure;
  sym::Symbol *symbol;
};

}

void stela::generateStat(
  gen::Ctx ctx,
  gen::Func func,
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
