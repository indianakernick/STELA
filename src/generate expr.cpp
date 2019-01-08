//
//  generate expr.cpp
//  STELA
//
//  Created by Indi Kernick on 1/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate expr.hpp"

#include "llvm.hpp"
#include "symbols.hpp"
#include "categories.hpp"
#include "unreachable.hpp"
#include "type builder.hpp"
#include "generate type.hpp"
#include "operator name.hpp"
#include "generate func.hpp"
#include "compare exprs.hpp"
#include "lifetime exprs.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(Scope &temps, gen::Ctx ctx, FuncBuilder &funcBdr)
    : temps{temps}, ctx{ctx}, funcBdr{funcBdr}, lifetime{ctx.inst, funcBdr.ir} {}

  gen::Expr visitValue(ast::Expression *expr) {
    assert(classifyType(expr->exprType.get()) == TypeCat::trivially_copyable);
    result = nullptr;
    expr->accept(*this);
    const ValueCat cat = classifyValue(expr);
    if (glvalue(cat)) {
      return {funcBdr.ir.CreateLoad(value), cat};
    } else {
      return {value, cat};
    }
  }
  gen::Expr visitExpr(ast::Expression *expr, llvm::Value *resultAddr) {
    result = resultAddr;
    expr->accept(*this);
    return {value, classifyValue(expr)};
  }
  
  llvm::Value *boolOr(ast::Expression *left, ast::Expression *right) {
    llvm::BasicBlock *leftBlock = funcBdr.ir.GetInsertBlock();
    llvm::BasicBlock *rightBlock = funcBdr.makeBlock();
    llvm::BasicBlock *doneBlock = funcBdr.makeBlock();
    
    gen::Expr leftExpr = visitValue(left);
    llvm::Type *boolType = leftExpr.obj->getType();
    funcBdr.ir.CreateCondBr(leftExpr.obj, doneBlock, rightBlock);
    
    funcBdr.setCurr(rightBlock);
    gen::Expr rightExpr = visitValue(right);
    funcBdr.ir.CreateBr(doneBlock);
    
    funcBdr.setCurr(doneBlock);
    llvm::PHINode *phi = funcBdr.ir.CreatePHI(boolType, 2);
    phi->addIncoming(llvm::ConstantInt::getTrue(boolType), leftBlock);
    phi->addIncoming(rightExpr.obj, rightBlock);
    return phi;
  }
  
  llvm::Value *boolAnd(ast::Expression *left, ast::Expression *right) {
    llvm::BasicBlock *leftBlock = funcBdr.ir.GetInsertBlock();
    llvm::BasicBlock *rightBlock = funcBdr.makeBlock();
    llvm::BasicBlock *doneBlock = funcBdr.makeBlock();
    
    gen::Expr leftExpr = visitValue(left);
    llvm::Type *boolType = leftExpr.obj->getType();
    funcBdr.ir.CreateCondBr(leftExpr.obj, rightBlock, doneBlock);
    
    funcBdr.setCurr(rightBlock);
    gen::Expr rightExpr = visitValue(right);
    funcBdr.ir.CreateBr(doneBlock);
    
    funcBdr.setCurr(doneBlock);
    llvm::PHINode *phi = funcBdr.ir.CreatePHI(boolType, 2);
    phi->addIncoming(llvm::ConstantInt::getFalse(boolType), leftBlock);
    phi->addIncoming(rightExpr.obj, rightBlock);
    return phi;
  }

  void visit(ast::BinaryExpr &expr) override {
    llvm::Value *resultAddr = result;
    if (expr.oper == ast::BinOp::bool_or) {
      value = boolOr(expr.left.get(), expr.right.get());
      storeValueAsResult(resultAddr);
      return;
    } else if (expr.oper == ast::BinOp::bool_and) {
      value = boolAnd(expr.left.get(), expr.right.get());
      storeValueAsResult(resultAddr);
      return;
    }
    
    gen::Expr left = visitValue(expr.left.get());
    gen::Expr right = visitValue(expr.right.get());
    left.cat = ValueCat::prvalue;
    right.cat = ValueCat::prvalue;
    const ArithCat arith = classifyArith(expr.left.get());
    
    #define SIGNED_UNSIGNED_FLOAT_OP(S_OP, U_OP, F_OP)                          \
      if (arith == ArithCat::signed_int) {                                      \
        value = funcBdr.ir.S_OP(left.obj, right.obj);                           \
      } else if (arith == ArithCat::unsigned_int) {                             \
        value = funcBdr.ir.U_OP(left.obj, right.obj);                           \
      } else {                                                                  \
        value = funcBdr.ir.F_OP(left.obj, right.obj);                           \
      }                                                                         \
      break
    
    CompareExpr compare{ctx.inst, funcBdr.ir};
    ast::Type *type = expr.left->exprType.get();
    
    switch (expr.oper) {
      case ast::BinOp::bool_or:
      case ast::BinOp::bit_or:
        value = funcBdr.ir.CreateOr(left.obj, right.obj); break;
      case ast::BinOp::bit_xor:
        value = funcBdr.ir.CreateXor(left.obj, right.obj); break;
      case ast::BinOp::bool_and:
      case ast::BinOp::bit_and:
        value = funcBdr.ir.CreateAnd(left.obj, right.obj); break;
      case ast::BinOp::bit_shl:
        value = funcBdr.ir.CreateShl(left.obj, right.obj); break;
      case ast::BinOp::bit_shr:
        value = funcBdr.ir.CreateLShr(left.obj, right.obj); break;
      case ast::BinOp::eq:
        value = compare.equal(type, left, right); break;
      case ast::BinOp::ne:
        value = compare.notEqual(type, left, right); break;
      case ast::BinOp::lt:
        value = compare.less(type, left, right); break;
      case ast::BinOp::le:
        value = compare.lessEqual(type, left, right); break;
      case ast::BinOp::gt:
        value = compare.greater(type, left, right); break;
      case ast::BinOp::ge:
        value = compare.greaterEqual(type, left, right); break;
      case ast::BinOp::add:
        SIGNED_UNSIGNED_FLOAT_OP(CreateNSWAdd, CreateAdd, CreateFAdd);
      case ast::BinOp::sub:
        SIGNED_UNSIGNED_FLOAT_OP(CreateNSWSub, CreateSub, CreateFSub);
      case ast::BinOp::mul:
        SIGNED_UNSIGNED_FLOAT_OP(CreateNSWMul, CreateMul, CreateFMul);
      case ast::BinOp::div:
        SIGNED_UNSIGNED_FLOAT_OP(CreateSDiv, CreateUDiv, CreateFDiv);
      case ast::BinOp::mod:
        SIGNED_UNSIGNED_FLOAT_OP(CreateSRem, CreateURem, CreateFRem);
      case ast::BinOp::pow:
        assert(false);
      default: UNREACHABLE();
    }
    
    #undef SIGNED_UNSIGNED_FLOAT_OP
    
    storeValueAsResult(resultAddr);
  }
  void visit(ast::UnaryExpr &expr) override {
    llvm::Value *resultAddr = result;
    gen::Expr operand = visitValue(expr.expr.get());
    const ArithCat arith = classifyArith(expr.expr.get());
    
    switch (expr.oper) {
      case ast::UnOp::neg:
        if (arith == ArithCat::signed_int) {
          value = funcBdr.ir.CreateNSWNeg(operand.obj);
        } else if (arith == ArithCat::unsigned_int) {
          value = funcBdr.ir.CreateNeg(operand.obj);
        } else {
          value = funcBdr.ir.CreateFNeg(operand.obj);
        }
        break;
      case ast::UnOp::bool_not:
      case ast::UnOp::bit_not:
        value = funcBdr.ir.CreateNot(operand.obj); break;
      default: UNREACHABLE();
    }
    storeValueAsResult(resultAddr);
  }
  
  /*
  void pushBtnFunc(const ast::BtnFuncEnum e) {
    switch (e) {
      case ast::BtnFuncEnum::capacity:
        str += "capacity"; return;
      case ast::BtnFuncEnum::size:
        str += "size"; return;
      case ast::BtnFuncEnum::push_back:
        str += "push_back"; return;
      case ast::BtnFuncEnum::append:
        str += "append"; return;
      case ast::BtnFuncEnum::pop_back:
        str += "pop_back"; return;
      case ast::BtnFuncEnum::resize:
        str += "resize"; return;
      case ast::BtnFuncEnum::reserve:
        str += "reserve"; return;
    }
    UNREACHABLE();
  }
  */
  llvm::Value *visitParam(const ast::FuncParam &param, ast::Expression *expr, llvm::Value **destroy) {
    if (param.ref == ast::ParamRef::ref) {
      const gen::Expr evalExpr = visitExpr(expr, nullptr);
      assert(evalExpr.cat == ValueCat::lvalue);
      return evalExpr.obj;
    }
    const TypeCat typeCat = classifyType(param.type.get());
    if (typeCat == TypeCat::trivially_copyable) {
      const gen::Expr evalExpr = visitExpr(expr, nullptr);
      if (evalExpr.cat == ValueCat::lvalue || evalExpr.cat == ValueCat::xvalue) {
        return funcBdr.ir.CreateLoad(evalExpr.obj);
      } else { // prvalue
        return evalExpr.obj;
      }
    } else { // trivially_relocatable or nontrivial
      llvm::Value *addr = funcBdr.alloc(generateType(ctx.llvm, param.type.get()));
      visitExpr(expr, addr);
      *destroy = addr;
      return addr;
    }
  }
  void pushArgs(
    std::vector<llvm::Value *> &args,
    const ast::FuncArgs &callArgs,
    const ast::FuncParams &params,
    std::vector<llvm::Value *> &destroyList
  ) {
    for (size_t a = 0; a != callArgs.size(); ++a) {
      args.push_back(visitParam(params[a], callArgs[a].get(), &destroyList[a + 1]));
    }
  }
  void visit(ast::FuncCall &call) override {
    if (call.definition == nullptr) {
      // call a function pointer
      assert(false);
    } else if (auto *func = dynamic_cast<ast::Func *>(call.definition)) {
      llvm::Value *resultAddr = result;
      std::vector<llvm::Value *> args;
      args.reserve(1 + call.args.size());
      llvm::FunctionType *funcType = func->llvmFunc->getFunctionType();
      std::vector<llvm::Value *> destroyList;
      destroyList.resize(1 + call.args.size(), nullptr);
      if (func->receiver) {
        ast::Expression *self = assertDownCast<ast::MemberIdent>(call.func.get())->object.get();
        args.push_back(visitParam(*func->receiver, self, &destroyList[0]));
      } else {
        TypeBuilder typeBdr{ctx.llvm};
        args.push_back(llvm::UndefValue::get(typeBdr.voidPtr()));
      }
      pushArgs(args, call.args, func->params, destroyList);
      
      if (funcType->params().size() == args.size() + 1) {
        if (resultAddr) {
          args.push_back(resultAddr);
          funcBdr.ir.CreateCall(func->llvmFunc, args);
          value = nullptr;
        } else {
          llvm::Type *retType = funcType->params().back()->getPointerElementType();
          llvm::Value *retAddr = funcBdr.alloc(retType);
          args.push_back(retAddr);
          funcBdr.ir.CreateCall(func->llvmFunc, args);
          value = retAddr;
        }
      } else {
        value = funcBdr.ir.CreateCall(func->llvmFunc, args);
        constructResultFromValue(resultAddr, &call);
      }
      
      if (destroyList[0]) {
        lifetime.destroy(func->receiver->type.get(), destroyList[0]);
      }
      for (size_t a = 1; a != destroyList.size(); ++a) {
        if (destroyList[a]) {
          lifetime.destroy(func->params[a - 1].type.get(), destroyList[a]);
        }
      }
    } else if (auto *btnFunc = dynamic_cast<ast::BtnFunc *>(call.definition)) {
      // call a builtin function
      assert(false);
    }
    /*if (call.definition == nullptr) {
      call.func->accept(*this);
      gen::String func = std::move(str);
      str += func;
      str += ".func(";
      str += func;
      str += ".data.get()";
      pushArgs(call.args, {});
      str += ")";
    } else if (auto *btnFunc = dynamic_cast<ast::BtnFunc *>(call.definition)) {
      pushBtnFunc(btnFunc->value);
      str += '(';
      if (call.args.empty()) {
        str += ')';
        return;
      }
      str += '(';
      call.args[0]->accept(*this);
      str += ')';
      for (auto a = call.args.cbegin() + 1; a != call.args.cend(); ++a) {
        str += ", (";
        (*a)->accept(*this);
        str += ')';
      }
      str += ')';
    }*/
  }
  
  llvm::Value *materialize(ast::Expression *expr) {
    if (classifyValue(expr) == ValueCat::prvalue) {
      ast::Type *type = expr->exprType.get();
      llvm::Value *object = funcBdr.alloc(generateType(ctx.llvm, type));
      visitExpr(expr, object);
      temps.push_back({object, type});
      return object;
    } else {
      return visitExpr(expr, nullptr).obj;
    }
  }
  
  void visit(ast::MemberIdent &mem) override {
    llvm::Value *resultAddr = result;
    llvm::Value *object = materialize(mem.object.get());
    value = funcBdr.ir.CreateStructGEP(object, mem.index);
    constructResultFromValue(resultAddr, &mem);
  }
  
  void visit(ast::Subscript &sub) override {
    llvm::Value *resultAddr = result;
    llvm::Value *object = materialize(sub.object.get());
    gen::Expr index = visitValue(sub.index.get());
    ast::BtnType *indexType = concreteType<ast::BtnType>(sub.index->exprType.get());
    
    auto *arr = concreteType<ast::ArrayType>(sub.object->exprType.get());
    assert(arr);
    llvm::Function *indexFn;
    if (indexType->value == ast::BtnTypeEnum::Sint) {
      indexFn = ctx.inst.arrayIdxS(arr);
    } else {
      indexFn = ctx.inst.arrayIdxU(arr);
    }
    value = funcBdr.ir.CreateCall(indexFn, {object, index.obj});
    
    constructResultFromValue(resultAddr, &sub);
  }
  /*
  void writeID(ast::Statement *definition, ast::Type *exprType, ast::Type *expectedType) {
    assert(definition);
    gen::String name;
    if (auto *param = dynamic_cast<ast::FuncParam *>(definition)) {
      //str += "(";
      //if (param->ref == ast::ParamRef::ref) {
      //  str += "*";
      //}
      name += "p_";
      name += param->index;
      //str += ")";
    } else if (auto *decl = dynamic_cast<ast::DeclAssign *>(definition)) {
      name += "v_";
      name += decl->id;
    } else if (auto *var = dynamic_cast<ast::Var *>(definition)) {
      name += "v_";
      name += var->id;
    } else if (auto *let = dynamic_cast<ast::Let *>(definition)) {
      name += "v_";
      name += let->id;
    }
    if (!name.empty()) {
      auto *funcType = concreteType<ast::FuncType>(exprType);
      auto *btnType = concreteType<ast::BtnType>(expectedType);
      if (funcType && btnType && btnType->value == ast::BtnTypeEnum::Bool) {
        str += "(";
        str += name;
        str += ".func != &";
        str += generateNullFunc(ctx, *funcType);
        str += ")";
        return;
      }
      str += name;
    }
    if (auto *func = dynamic_cast<ast::Func *>(definition)) {
      auto *funcType = assertDownCast<ast::FuncType>(exprType);
      str += generateMakeFunc(ctx, *funcType);
      str += "(&f_";
      str += func->id;
      str += ")";
      return;
    }
    assert(!name.empty());
  }
  bool writeCapture(const uint32_t index) {
    if (index == ~uint32_t{}) {
      return false;
    } else {
      str += "capture.c_";
      str += index;
      return true;
    }
  }
  */
  
  llvm::Value *getAddr(ast::Statement *definition) {
    if (auto *param = dynamic_cast<ast::FuncParam *>(definition)) {
      return param->llvmAddr;
    } else if (auto *decl = dynamic_cast<ast::DeclAssign *>(definition)) {
      return decl->llvmAddr;
    } else if (auto *var = dynamic_cast<ast::Var *>(definition)) {
      return var->llvmAddr;
    } else if (auto *let = dynamic_cast<ast::Let *>(definition)) {
      return let->llvmAddr;
    }
    return nullptr;
  }
  void visit(ast::Identifier &ident) override {
    /*if (!writeCapture(ident.captureIndex)) {
      writeID(ident.definition, ident.exprType.get(), ident.expectedType.get());
    }*/
    value = getAddr(ident.definition);
    constructResultFromValue(result, &ident);
  }
  void visit(ast::Ternary &tern) override {
    auto *condBlock = funcBdr.nextEmpty();
    auto *trooBlock = funcBdr.makeBlock();
    auto *folsBlock = funcBdr.makeBlock();
    auto *doneBlock = funcBdr.makeBlock();
    llvm::Value *resultAddr = result;
    
    funcBdr.setCurr(condBlock);
    funcBdr.ir.CreateCondBr(visitValue(tern.cond.get()).obj, trooBlock, folsBlock);
  
    gen::Expr troo{nullptr, {}};
    gen::Expr fols{nullptr, {}};
    const ValueCat trooCat = classifyValue(tern.troo.get());
    const ValueCat folsCat = classifyValue(tern.fols.get());
    
    if (resultAddr) {
      funcBdr.setCurr(trooBlock);
      visitExpr(tern.troo.get(), resultAddr);
      funcBdr.setCurr(folsBlock);
      visitExpr(tern.fols.get(), resultAddr);
      value = nullptr;
    } else if (trooCat == ValueCat::lvalue && folsCat == ValueCat::lvalue) {
      funcBdr.setCurr(trooBlock);
      troo = visitExpr(tern.troo.get(), nullptr);
      funcBdr.setCurr(folsBlock);
      fols = visitExpr(tern.fols.get(), nullptr);
    } else if (classifyType(tern.exprType.get()) == TypeCat::trivially_copyable) {
      funcBdr.setCurr(trooBlock);
      troo = visitValue(tern.troo.get());
      funcBdr.setCurr(folsBlock);
      fols = visitValue(tern.fols.get());
    } else {
      llvm::Value *addr = funcBdr.alloc(generateType(ctx.llvm, tern.exprType.get()));
      funcBdr.setCurr(trooBlock);
      visitExpr(tern.troo.get(), addr);
      funcBdr.setCurr(folsBlock);
      visitExpr(tern.fols.get(), addr);
      value = addr;
    }
    
    funcBdr.link(trooBlock, doneBlock);
    funcBdr.link(folsBlock, doneBlock);
    funcBdr.setCurr(doneBlock);
    
    if (troo.obj) {
      assert(fols.obj);
      llvm::PHINode *phi = funcBdr.ir.CreatePHI(troo.obj->getType(), 2);
      phi->addIncoming(troo.obj, trooBlock);
      phi->addIncoming(fols.obj, folsBlock);
      value = phi;
    }
  }
  void visit(ast::Make &make) override {
    if (!make.cast) {
      make.expr->accept(*this);
      return;
    }
    llvm::Type *type = generateType(ctx.llvm, make.type.get());
    llvm::Value *resultAddr = result;
    gen::Expr srcVal = visitValue(make.expr.get());
    const ArithCat dst = classifyArith(make.type.get());
    const ArithCat src = classifyArith(make.expr.get());
    if (src == ArithCat::signed_int) {
      if (dst == ArithCat::signed_int) {
        value = funcBdr.ir.CreateIntCast(srcVal.obj, type, true);
      } else if (dst == ArithCat::unsigned_int) {
        value = funcBdr.ir.CreateIntCast(srcVal.obj, type, true);
      } else {
        value = funcBdr.ir.CreateCast(llvm::Instruction::SIToFP, srcVal.obj, type);
      }
    } else if (src == ArithCat::unsigned_int) {
      if (dst == ArithCat::signed_int) {
        value = funcBdr.ir.CreateIntCast(srcVal.obj, type, false);
      } else if (dst == ArithCat::unsigned_int) {
        value = funcBdr.ir.CreateIntCast(srcVal.obj, type, false);
      } else {
        value = funcBdr.ir.CreateCast(llvm::Instruction::UIToFP, srcVal.obj, type);
      }
    } else {
      if (dst == ArithCat::signed_int) {
        value = funcBdr.ir.CreateCast(llvm::Instruction::FPToSI, srcVal.obj, type);
      } else if (dst == ArithCat::unsigned_int) {
        value = funcBdr.ir.CreateCast(llvm::Instruction::FPToUI, srcVal.obj, type);
      } else {
        value = funcBdr.ir.CreateFPCast(srcVal.obj, type);
      }
    }
    storeValueAsResult(resultAddr);
  }
  
  void visit(ast::StringLiteral &str) override {
    ast::ArrayType *type = assertDownCast<ast::ArrayType>(str.exprType.get());
    llvm::Value *addr = result ? result : funcBdr.alloc(generateType(ctx.llvm, type));
    if (str.value.empty()) {
      lifetime.defConstruct(type, addr);
    } else {
      llvm::Function *ctor = ctx.inst.arrayLenCtor(type);
      TypeBuilder typeBdr{addr->getContext()};
      llvm::Constant *size = llvm::ConstantInt::get(typeBdr.len(), str.value.size());
      llvm::Value *basePtr = funcBdr.ir.CreateCall(ctor, {addr, size});
      llvm::Value *strPtr = funcBdr.ir.CreateGlobalStringPtr(str.value);
      funcBdr.ir.CreateMemCpy(basePtr, 1, strPtr, 1, str.value.size());
    }
    value = addr;
  }
  void visit(ast::CharLiteral &chr) override {
    value = llvm::ConstantInt::get(
      llvm::IntegerType::getInt8Ty(ctx.llvm),
      static_cast<uint64_t>(chr.value),
      true
    );
    storeValueAsResult(result);
  }
  
  // @TODO Use std::visit when we get MacOS 10.14 on Travis
  template <typename Type, typename Callable, typename Variant>
  static void visitImpl(Callable &&fn, const Variant &variant) {
    if (std::holds_alternative<Type>(variant)) {
      // MacOS 10.13 doesn't have std::get
      fn(*std::get_if<Type>(&variant));
    }
  }
  
  template <typename Callable, typename... Types>
  static void poorMansVisit(Callable &&fn, const std::variant<Types...> &variant) {
    (visitImpl<Types>(fn, variant), ...);
  }
  
  void visit(ast::NumberLiteral &num) override {
    llvm::Type *type = generateType(ctx.llvm, num.exprType.get());
    poorMansVisit([this, type] (auto val) {
      using Type = std::decay_t<decltype(val)>;
      if constexpr (std::is_floating_point_v<Type>) {
        value = llvm::ConstantFP::get(
          type,
          static_cast<double>(val)
        );
      } else if constexpr (!std::is_same_v<Type, std::monostate>){
        value = llvm::ConstantInt::get(
          type,
          static_cast<uint64_t>(val),
          std::is_signed_v<Type>
        );
      }
    }, num.value);
    storeValueAsResult(result);
  }
  void visit(ast::BoolLiteral &bol) override {
    llvm::Type *boolType = llvm::IntegerType::getInt1Ty(ctx.llvm);
    if (bol.value) {
      value = llvm::ConstantInt::getTrue(boolType);
    } else {
      value = llvm::ConstantInt::getFalse(boolType);
    }
    storeValueAsResult(result);
  }
  void visit(ast::ArrayLiteral &arr) override {
    ast::ArrayType *type = assertDownCast<ast::ArrayType>(arr.exprType.get());
    llvm::Value *addr = result ? result : funcBdr.alloc(generateType(ctx.llvm, type));
    if (arr.exprs.empty()) {
      lifetime.defConstruct(type, addr);
    } else {
      llvm::Function *ctor = ctx.inst.arrayLenCtor(type);
      TypeBuilder typeBdr{addr->getContext()};
      llvm::Constant *size = llvm::ConstantInt::get(typeBdr.len(), arr.exprs.size());
      llvm::Value *basePtr = funcBdr.ir.CreateCall(ctor, {addr, size});
      llvm::Type *elemTy = basePtr->getType()->getPointerElementType();
      llvm::Type *sizeTy = getType<size_t>(addr->getContext());
      for (unsigned e = 0; e != arr.exprs.size(); ++e) {
        llvm::Constant *idx = llvm::ConstantInt::get(sizeTy, e);
        llvm::Value *elemPtr = funcBdr.ir.CreateInBoundsGEP(elemTy, basePtr, idx);
        visitExpr(arr.exprs[e].get(), elemPtr);
      }
      // @TODO lifetime.startLife
    }
    value = addr;
  }
  void visit(ast::InitList &list) override {
    ast::Type *type = list.exprType.get();
    llvm::Value *resultAddr = result;
    llvm::Value *addr = result ? result : funcBdr.alloc(generateType(ctx.llvm, type));
    if (list.exprs.empty()) {
      lifetime.defConstruct(type, addr);
    } else {
      for (unsigned e = 0; e != list.exprs.size(); ++e) {
        visitExpr(list.exprs[e].get(), funcBdr.ir.CreateStructGEP(addr, e));
      }
    }
    if (resultAddr) {
      value = nullptr;
    } else {
      if (classifyType(type) == TypeCat::trivially_copyable) {
        // @TODO don't do this loading and storing for trivially copyable types
        // maybe something like lifetime.defValue
        value = funcBdr.ir.CreateLoad(addr);
      } else {
        value = addr;
      }
    }
  }
  /*
  void writeCapture(const sym::ClosureCap &cap) {
    if (!writeCapture(cap.index)) {
      writeID(cap.object, cap.type.get(), nullptr);
    }
  }
  void visit(ast::Lambda &lambda) override {
    // prvalue
  
    str += generateMakeLam(ctx, lambda);
    str += "({";
    sym::Lambda *symbol = lambda.symbol;
    if (symbol->captures.empty()) {
      str += "})";
      return;
    } else {
      writeCapture(symbol->captures[0]);
    }
    for (auto c = symbol->captures.cbegin() + 1; c != symbol->captures.cend(); ++c) {
      str += ", ";
      writeCapture(*c);
    }
    str += "})";
  }*/

private:
  Scope &temps;
  gen::Ctx ctx;
  FuncBuilder &funcBdr;
  LifetimeExpr lifetime;
  llvm::Value *value = nullptr;
  llvm::Value *result = nullptr;
  
  void storeValueAsResult(llvm::Value *resultAddr) {
    if (resultAddr) {
      // @TODO start the lifetime of the result object
      // maybe call lifetime.construct or lifetime.startLife
      funcBdr.ir.CreateStore(value, resultAddr);
      value = nullptr;
    }
  }
  void constructResultFromValue(llvm::Value *resultAddr, ast::Expression *expr) {
    if (resultAddr) {
      gen::Expr other{value, classifyValue(expr)};
      lifetime.construct(expr->exprType.get(), resultAddr, other);
      value = nullptr;
    }
  }
};

}

gen::Expr stela::generateValueExpr(
  Scope &temps,
  gen::Ctx ctx,
  FuncBuilder &builder,
  ast::Expression *expr
) {
  Visitor visitor{temps, ctx, builder};
  return visitor.visitValue(expr);
}

gen::Expr stela::generateExpr(
  Scope &temps,
  gen::Ctx ctx,
  FuncBuilder &builder,
  ast::Expression *expr,
  llvm::Value *result
) {
  Visitor visitor{temps, ctx, builder};
  return visitor.visitExpr(expr, result);
}
