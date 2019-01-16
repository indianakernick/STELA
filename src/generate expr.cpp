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
#include "gen types.hpp"
#include "categories.hpp"
#include "unreachable.hpp"
#include "gen helpers.hpp"
#include "generate type.hpp"
#include "operator name.hpp"
#include "compare exprs.hpp"
#include "lifetime exprs.hpp"
#include "generate closure.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(Scope &temps, gen::Ctx ctx, FuncBuilder &builder, llvm::Value *closure)
    : temps{temps},
      ctx{ctx},
      builder{builder},
      lifetime{ctx.inst, builder.ir},
      closure{closure} {}

  gen::Expr visitValue(ast::Expression *expr) {
    result = nullptr;
    expr->accept(*this);
    const ValueCat valueCat = classifyValue(expr);
    const TypeCat typeCat = classifyType(expr->exprType.get());
    if (glvalue(valueCat) && typeCat == TypeCat::trivially_copyable) {
      return {builder.ir.CreateLoad(value), valueCat};
    } else {
      return {value, valueCat};
    }
  }
  gen::Expr visitExpr(ast::Expression *expr, llvm::Value *resultAddr) {
    result = resultAddr;
    expr->accept(*this);
    return {value, classifyValue(expr)};
  }
  gen::Expr visitBool(ast::Expression *expr) {
    result = nullptr;
    expr->accept(*this);
    const ValueCat valueCat = classifyValue(expr);
    return {convertToBool(expr->exprType.get(), {value, valueCat}), ValueCat::prvalue};
  }
  
  llvm::Value *convertToBool(ast::Type *exprType, gen::Expr expr) {
    if (isBoolType(exprType)) {
      if (glvalue(expr.cat)) {
        return builder.ir.CreateLoad(expr.obj);
      } else {
        return expr.obj;
      }
    }
    
    auto *funcType = concreteType<ast::FuncType>(exprType);
    assert(funcType);
    llvm::Function *conv = ctx.inst.get<PFGI::clo_bool>(funcType);
    llvm::Value *boolValue = builder.ir.CreateCall(conv, {expr.obj});
    if (expr.cat == ValueCat::prvalue) {
      lifetime.destroy(exprType, expr.obj);
    }
    return boolValue;
  }
  
  llvm::Value *boolOr(ast::Expression *left, ast::Expression *right) {
    llvm::BasicBlock *leftBlock = builder.ir.GetInsertBlock();
    llvm::BasicBlock *rightBlock = builder.makeBlock();
    llvm::BasicBlock *doneBlock = builder.makeBlock();
    
    gen::Expr leftExpr = visitBool(left);
    llvm::Type *boolType = leftExpr.obj->getType();
    builder.ir.CreateCondBr(leftExpr.obj, doneBlock, rightBlock);
    
    builder.setCurr(rightBlock);
    gen::Expr rightExpr = visitBool(right);
    builder.branch(doneBlock);
    llvm::PHINode *phi = builder.ir.CreatePHI(boolType, 2);
    phi->addIncoming(llvm::ConstantInt::getTrue(boolType), leftBlock);
    phi->addIncoming(rightExpr.obj, rightBlock);
    return phi;
  }
  
  llvm::Value *boolAnd(ast::Expression *left, ast::Expression *right) {
    llvm::BasicBlock *leftBlock = builder.ir.GetInsertBlock();
    llvm::BasicBlock *rightBlock = builder.makeBlock();
    llvm::BasicBlock *doneBlock = builder.makeBlock();
    
    gen::Expr leftExpr = visitBool(left);
    llvm::Type *boolType = leftExpr.obj->getType();
    builder.ir.CreateCondBr(leftExpr.obj, rightBlock, doneBlock);
    
    builder.setCurr(rightBlock);
    gen::Expr rightExpr = visitBool(right);
    builder.branch(doneBlock);
    llvm::PHINode *phi = builder.ir.CreatePHI(boolType, 2);
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
    
    #define SIGNED_UNSIGNED_FLOAT_OP(S_OP, U_OP, F_OP)                          \
      {                                                                         \
        const ArithCat arith = classifyArith(expr.left.get());                  \
        if (arith == ArithCat::signed_int) {                                    \
          value = builder.ir.S_OP(left.obj, right.obj);                         \
        } else if (arith == ArithCat::unsigned_int) {                           \
          value = builder.ir.U_OP(left.obj, right.obj);                         \
        } else {                                                                \
          value = builder.ir.F_OP(left.obj, right.obj);                         \
        }                                                                       \
      }                                                                         \
      break
    
    CompareExpr compare{ctx.inst, builder.ir};
    ast::Type *type = expr.left->exprType.get();
    
    switch (expr.oper) {
      case ast::BinOp::bool_or:
      case ast::BinOp::bit_or:
        value = builder.ir.CreateOr(left.obj, right.obj); break;
      case ast::BinOp::bit_xor:
        value = builder.ir.CreateXor(left.obj, right.obj); break;
      case ast::BinOp::bool_and:
      case ast::BinOp::bit_and:
        value = builder.ir.CreateAnd(left.obj, right.obj); break;
      case ast::BinOp::bit_shl:
        value = builder.ir.CreateShl(left.obj, right.obj); break;
      case ast::BinOp::bit_shr:
        value = builder.ir.CreateLShr(left.obj, right.obj); break;
      case ast::BinOp::eq:
        value = compare.eq(type, left, right); break;
      case ast::BinOp::ne:
        value = compare.ne(type, left, right); break;
      case ast::BinOp::lt:
        value = compare.lt(type, left, right); break;
      case ast::BinOp::le:
        value = compare.le(type, left, right); break;
      case ast::BinOp::gt:
        value = compare.gt(type, left, right); break;
      case ast::BinOp::ge:
        value = compare.ge(type, left, right); break;
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
      default: UNREACHABLE();
    }
    
    #undef SIGNED_UNSIGNED_FLOAT_OP
    
    storeValueAsResult(resultAddr);
  }
  void visit(ast::UnaryExpr &expr) override {
    llvm::Value *resultAddr = result;
    switch (expr.oper) {
      case ast::UnOp::neg: {
        gen::Expr operand = visitValue(expr.expr.get());
        const ArithCat arith = classifyArith(expr.expr.get());
        if (arith == ArithCat::signed_int) {
          value = builder.ir.CreateNSWNeg(operand.obj);
        } else if (arith == ArithCat::unsigned_int) {
          value = builder.ir.CreateNeg(operand.obj);
        } else {
          value = builder.ir.CreateFNeg(operand.obj);
        }
        break;
      }
      case ast::UnOp::bool_not: {
        gen::Expr operand = visitBool(expr.expr.get());
        value = builder.ir.CreateNot(operand.obj);
        break;
      }
      case ast::UnOp::bit_not: {
        gen::Expr operand = visitValue(expr.expr.get());
        value = builder.ir.CreateNot(operand.obj);
        break;
      }
      default: UNREACHABLE();
    }
    storeValueAsResult(resultAddr);
  }
  
  llvm::Function *getBtnFunc(const ast::BtnFuncEnum f, ast::ArrayType *arr) {
    switch (f) {
      case ast::BtnFuncEnum::capacity:
        return ctx.inst.get<PFGI::btn_capacity>(arr);
      case ast::BtnFuncEnum::size:
        return ctx.inst.get<PFGI::btn_size>(arr);
      case ast::BtnFuncEnum::push_back:
        return ctx.inst.get<PFGI::btn_push_back>(arr);
      case ast::BtnFuncEnum::append:
        return ctx.inst.get<PFGI::btn_append>(arr);
      case ast::BtnFuncEnum::pop_back:
        return ctx.inst.get<PFGI::btn_pop_back>(arr);
      case ast::BtnFuncEnum::resize:
        return ctx.inst.get<PFGI::btn_resize>(arr);
      case ast::BtnFuncEnum::reserve:
        return ctx.inst.get<PFGI::btn_reserve>(arr);
    }
    UNREACHABLE();
  }
  llvm::Value *visitParam(ast::Type *type, ast::ParamRef ref, ast::Expression *expr, Object *destroy) {
    if (ref == ast::ParamRef::ref) {
      const gen::Expr evalExpr = visitExpr(expr, nullptr);
      assert(evalExpr.cat == ValueCat::lvalue);
      return evalExpr.obj;
    }
    const TypeCat typeCat = classifyType(type);
    if (typeCat == TypeCat::trivially_copyable) {
      const gen::Expr evalExpr = visitExpr(expr, nullptr);
      if (evalExpr.cat == ValueCat::lvalue || evalExpr.cat == ValueCat::xvalue) {
        return builder.ir.CreateLoad(evalExpr.obj);
      } else { // prvalue
        return evalExpr.obj;
      }
    } else { // trivially_relocatable or nontrivial
      llvm::Value *addr = builder.alloc(generateType(ctx.llvm, type));
      visitExpr(expr, addr);
      destroy->addr = addr;
      destroy->type = type;
      return addr;
    }
  }
  void pushArgs(
    std::vector<llvm::Value *> &args,
    const ast::FuncArgs &callArgs,
    const ast::FuncParams &params,
    std::vector<Object> &dtors
  ) {
    for (size_t a = 0; a != callArgs.size(); ++a) {
      args.push_back(visitParam(
        params[a].type.get(), params[a].ref, callArgs[a].get(), &dtors[a + 1]
      ));
    }
  }
  void destroyArgs(const std::vector<Object> &args) {
    for (const Object &obj : args) {
      if (obj.addr) {
        lifetime.destroy(obj.type, obj.addr);
      }
    }
  }
  void visit(ast::FuncCall &call) override {
    // @TODO refactor
    if (call.definition == nullptr) {
      llvm::Value *resultAddr = result;
      const gen::Expr func = visitExpr(call.func.get(), nullptr);
      std::vector<llvm::Value *> args;
      args.reserve(1 + call.args.size());
      ast::FuncType *funcType = assertDownCast<ast::FuncType>(call.func->exprType.get());
      const Signature sig = getSignature(*funcType);
      llvm::FunctionType *fnType = generateSig(ctx.llvm, sig);
      std::vector<Object> dtors;
      dtors.resize(call.args.size());
      llvm::Value *fun = closureFun(builder, func.obj);
      args.push_back(closureDat(builder, func.obj));
      for (size_t a = 0; a != call.args.size(); ++a) {
        const ast::ParamType &param = funcType->params[a];
        args.push_back(visitParam(
          param.type.get(), param.ref, call.args[a].get(), &dtors[a]
        ));
      }
      if (fnType->getNumParams() == args.size() + 1) {
        if (resultAddr) {
          args.push_back(resultAddr);
          builder.ir.CreateCall(fun, args);
          value = nullptr;
        } else {
          llvm::Type *retType = fnType->params().back()->getPointerElementType();
          llvm::Value *retAddr = builder.alloc(retType);
          args.push_back(retAddr);
          builder.ir.CreateCall(fun, args);
          value = retAddr;
        }
      } else {
        value = builder.ir.CreateCall(fun, args);
        constructResultFromValue(resultAddr, &call);
      }
      destroyArgs(dtors);
      if (func.cat == ValueCat::prvalue) {
        lifetime.destroy(funcType, func.obj);
      }
    } else if (auto *func = dynamic_cast<ast::Func *>(call.definition)) {
      llvm::Value *resultAddr = result;
      std::vector<llvm::Value *> args;
      args.reserve(1 + call.args.size());
      llvm::FunctionType *funcType = func->llvmFunc->getFunctionType();
      std::vector<Object> dtors;
      dtors.resize(1 + call.args.size());
      if (func->receiver) {
        ast::Expression *self = assertDownCast<ast::MemberIdent>(call.func.get())->object.get();
        ast::FuncParam &rec = *func->receiver;
        args.push_back(visitParam(rec.type.get(), rec.ref, self, &dtors[0]));
      } else {
        args.push_back(llvm::UndefValue::get(voidPtrTy(ctx.llvm)));
      }
      pushArgs(args, call.args, func->params, dtors);
      
      if (funcType->getNumParams() == args.size() + 1) {
        if (resultAddr) {
          args.push_back(resultAddr);
          builder.ir.CreateCall(func->llvmFunc, args);
          value = nullptr;
        } else {
          llvm::Type *retType = funcType->params().back()->getPointerElementType();
          llvm::Value *retAddr = builder.alloc(retType);
          args.push_back(retAddr);
          builder.ir.CreateCall(func->llvmFunc, args);
          value = retAddr;
        }
      } else {
        value = builder.ir.CreateCall(func->llvmFunc, args);
        constructResultFromValue(resultAddr, &call);
      }
      
      destroyArgs(dtors);
    } else if (auto *btnFunc = dynamic_cast<ast::BtnFunc *>(call.definition)) {
      // @TODO this is highly dependent on the array builtin functions
      llvm::Value *resultAddr = result;
      ast::ArrayType *arr = concreteType<ast::ArrayType>(call.args[0]->exprType.get());
      std::vector<llvm::Value *> args;
      args.reserve(call.args.size());
      std::vector<Object> dtors;
      dtors.resize(call.args.size());
      if (btnFunc->value == ast::BtnFuncEnum::capacity || btnFunc->value == ast::BtnFuncEnum::size) {
        args.push_back(visitParam(arr, ast::ParamRef::val, call.args[0].get(), &dtors[0]));
      } else {
        args.push_back(visitParam(arr, ast::ParamRef::ref, call.args[0].get(), &dtors[0]));
      }
      if (call.args.size() == 2) {
        ast::Type *secType = call.args[1]->exprType.get();
        args.push_back(visitParam(secType, ast::ParamRef::val, call.args[1].get(), &dtors[1]));
      }
      value = builder.ir.CreateCall(getBtnFunc(btnFunc->value, arr), args);
      constructResultFromValue(resultAddr, &call);
      destroyArgs(dtors);
    }
  }
  
  llvm::Value *materialize(ast::Expression *expr) {
    if (classifyValue(expr) == ValueCat::prvalue) {
      ast::Type *type = expr->exprType.get();
      llvm::Value *object = builder.alloc(generateType(ctx.llvm, type));
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
    value = builder.ir.CreateStructGEP(object, mem.index);
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
      indexFn = ctx.inst.get<PFGI::arr_idx_s>(arr);
    } else {
      indexFn = ctx.inst.get<PFGI::arr_idx_u>(arr);
    }
    value = builder.ir.CreateCall(indexFn, {object, index.obj});
    
    constructResultFromValue(resultAddr, &sub);
  }
  
  void constructIdent(
    ast::Statement *definition,
    ast::Type *exprType,
    llvm::Value *resultAddr
  ) {
    value = nullptr;
    if (auto *param = dynamic_cast<ast::FuncParam *>(definition)) {
      value = param->llvmAddr;
    } else if (auto *decl = dynamic_cast<ast::DeclAssign *>(definition)) {
      value = decl->llvmAddr;
    } else if (auto *var = dynamic_cast<ast::Var *>(definition)) {
      value = var->llvmAddr;
    } else if (auto *let = dynamic_cast<ast::Let *>(definition)) {
      value = let->llvmAddr;
    }
    if (value) {
      if (resultAddr) {
        lifetime.construct(exprType, resultAddr, {value, ValueCat::lvalue});
        value = nullptr;
      }
      return;
    }
    if (auto *func = dynamic_cast<ast::Func *>(definition)) {
      auto *funcType = assertDownCast<ast::FuncType>(exprType);
      llvm::Function *funCtor = ctx.inst.get<PFGI::clo_fun_ctor>(funcType);
      if (resultAddr) {
        builder.ir.CreateCall(funCtor, {resultAddr, func->llvmFunc});
        value = nullptr;
      } else {
        llvm::Value *fnAddr = builder.alloc(generateType(ctx.llvm, funcType));
        builder.ir.CreateCall(funCtor, {fnAddr, func->llvmFunc});
        value = fnAddr;
      }
      // @TODO lifetime.startLife
    }
  }
  void visit(ast::Identifier &ident) override {
    llvm::Value *resultAddr = result;
    if (ident.captureIndex == ~uint32_t{}) {
      constructIdent(
        ident.definition,
        ident.exprType.get(),
        resultAddr
      );
    } else {
      assert(closure);
      value = closureCapAddr(builder, closure, ident.captureIndex);
      constructResultFromValue(resultAddr, &ident);
    }
  }
  void visit(ast::Ternary &tern) override {
    auto *condBlock = builder.nextEmpty();
    auto *trooBlock = builder.makeBlock();
    auto *folsBlock = builder.makeBlock();
    auto *doneBlock = builder.makeBlock();
    llvm::Value *resultAddr = result;
    
    builder.setCurr(condBlock);
    builder.ir.CreateCondBr(visitBool(tern.cond.get()).obj, trooBlock, folsBlock);
  
    gen::Expr troo{nullptr, {}};
    gen::Expr fols{nullptr, {}};
    const ValueCat trooCat = classifyValue(tern.troo.get());
    const ValueCat folsCat = classifyValue(tern.fols.get());
    
    if (resultAddr) {
      builder.setCurr(trooBlock);
      visitExpr(tern.troo.get(), resultAddr);
      builder.setCurr(folsBlock);
      visitExpr(tern.fols.get(), resultAddr);
      value = nullptr;
    } else if (trooCat == ValueCat::lvalue && folsCat == ValueCat::lvalue) {
      builder.setCurr(trooBlock);
      troo = visitExpr(tern.troo.get(), nullptr);
      builder.setCurr(folsBlock);
      fols = visitExpr(tern.fols.get(), nullptr);
    } else if (classifyType(tern.exprType.get()) == TypeCat::trivially_copyable) {
      builder.setCurr(trooBlock);
      troo = visitValue(tern.troo.get());
      builder.setCurr(folsBlock);
      fols = visitValue(tern.fols.get());
    } else {
      llvm::Value *addr = builder.alloc(generateType(ctx.llvm, tern.exprType.get()));
      builder.setCurr(trooBlock);
      visitExpr(tern.troo.get(), addr);
      builder.setCurr(folsBlock);
      visitExpr(tern.fols.get(), addr);
      value = addr;
    }
    
    builder.link(trooBlock, doneBlock);
    builder.link(folsBlock, doneBlock);
    builder.setCurr(doneBlock);
    
    if (troo.obj) {
      assert(fols.obj);
      llvm::PHINode *phi = builder.ir.CreatePHI(troo.obj->getType(), 2);
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
    llvm::Value *resultAddr = result;
    ast::Type *exprType = make.expr->exprType.get();
    if (isBoolType(make.type.get()) && !concreteType<ast::BtnType>(exprType)) {
      value = visitBool(make.expr.get()).obj;
      storeValueAsResult(resultAddr);
      return;
    }
    llvm::Type *type = generateType(ctx.llvm, make.type.get());
    gen::Expr srcVal = visitValue(make.expr.get());
    const ArithCat dst = classifyArith(make.type.get());
    const ArithCat src = classifyArith(make.expr.get());
    if (src == ArithCat::signed_int) {
      if (dst == ArithCat::signed_int) {
        value = builder.ir.CreateIntCast(srcVal.obj, type, true);
      } else if (dst == ArithCat::unsigned_int) {
        value = builder.ir.CreateIntCast(srcVal.obj, type, true);
      } else {
        value = builder.ir.CreateCast(llvm::Instruction::SIToFP, srcVal.obj, type);
      }
    } else if (src == ArithCat::unsigned_int) {
      if (dst == ArithCat::signed_int) {
        value = builder.ir.CreateIntCast(srcVal.obj, type, false);
      } else if (dst == ArithCat::unsigned_int) {
        value = builder.ir.CreateIntCast(srcVal.obj, type, false);
      } else {
        value = builder.ir.CreateCast(llvm::Instruction::UIToFP, srcVal.obj, type);
      }
    } else {
      if (dst == ArithCat::signed_int) {
        value = builder.ir.CreateCast(llvm::Instruction::FPToSI, srcVal.obj, type);
      } else if (dst == ArithCat::unsigned_int) {
        value = builder.ir.CreateCast(llvm::Instruction::FPToUI, srcVal.obj, type);
      } else {
        value = builder.ir.CreateFPCast(srcVal.obj, type);
      }
    }
    storeValueAsResult(resultAddr);
  }
  
  void visit(ast::StringLiteral &str) override {
    ast::ArrayType *type = assertDownCast<ast::ArrayType>(str.exprType.get());
    llvm::Value *addr = result ? result : builder.alloc(generateType(ctx.llvm, type));
    if (str.value.empty()) {
      lifetime.defConstruct(type, addr);
    } else {
      llvm::Function *ctor = ctx.inst.get<PFGI::arr_len_ctor>(type);
      llvm::Constant *size = llvm::ConstantInt::get(
        lenTy(addr->getContext()), str.value.size()
      );
      llvm::Value *basePtr = builder.ir.CreateCall(ctor, {addr, size});
      llvm::Value *strPtr = builder.ir.CreateGlobalStringPtr(str.value);
      builder.ir.CreateMemCpy(basePtr, 1, strPtr, 1, str.value.size());
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
    llvm::Value *addr = result ? result : builder.alloc(generateType(ctx.llvm, type));
    if (arr.exprs.empty()) {
      lifetime.defConstruct(type, addr);
    } else {
      llvm::Function *ctor = ctx.inst.get<PFGI::arr_len_ctor>(type);
      llvm::Constant *size = llvm::ConstantInt::get(
        lenTy(addr->getContext()), arr.exprs.size()
      );
      llvm::Value *basePtr = builder.ir.CreateCall(ctor, {addr, size});
      llvm::Type *elemTy = basePtr->getType()->getPointerElementType();
      llvm::Type *sizeTy = getType<size_t>(addr->getContext());
      for (unsigned e = 0; e != arr.exprs.size(); ++e) {
        llvm::Constant *idx = llvm::ConstantInt::get(sizeTy, e);
        llvm::Value *elemPtr = builder.ir.CreateInBoundsGEP(elemTy, basePtr, idx);
        visitExpr(arr.exprs[e].get(), elemPtr);
      }
      // @TODO lifetime.startLife
    }
    value = addr;
  }
  void visit(ast::InitList &list) override {
    ast::Type *type = list.exprType.get();
    llvm::Value *resultAddr = result;
    llvm::Value *addr = result ? result : builder.alloc(generateType(ctx.llvm, type));
    if (list.exprs.empty()) {
      lifetime.defConstruct(type, addr);
    } else {
      for (unsigned e = 0; e != list.exprs.size(); ++e) {
        visitExpr(list.exprs[e].get(), builder.ir.CreateStructGEP(addr, e));
      }
    }
    if (resultAddr) {
      value = nullptr;
    } else {
      if (classifyType(type) == TypeCat::trivially_copyable) {
        // @TODO don't do this loading and storing for trivially copyable types
        // maybe something like lifetime.defValue
        value = builder.ir.CreateLoad(addr);
      } else {
        value = addr;
      }
    }
  }

  void visit(ast::Lambda &lambda) override {
    llvm::Value *resultAddr = result;
    llvm::Function *body = genLambdaBody(ctx, lambda);
    llvm::Function *alloc = ctx.inst.get<FGI::alloc>();
    llvm::Type *capTy = generateLambdaCapture(ctx.llvm, lambda);
    llvm::Value *captures = callAlloc(builder.ir, alloc, capTy);
    initRefCount(builder.ir, captures);
    std::vector<sym::ClosureCap> &caps = lambda.symbol->captures;
    llvm::Function *dtor = getVirtualDtor(ctx, lambda);
    setDtor(builder, captures, dtor);
    for (uint32_t c = 0; c != caps.size(); ++c) {
      llvm::Value *capAddr = closureCapAddr(builder, captures, c);
      sym::ClosureCap &cap = caps[c];
      if (cap.index == ~uint32_t{}) {
        constructIdent(
          cap.object,
          cap.type.get(),
          capAddr
        );
      } else {
        assert(closure);
        llvm::Value *recapture = closureCapAddr(builder, closure, cap.index);
        lifetime.construct(cap.type.get(), capAddr, {recapture, ValueCat::lvalue});
      }
    }
    ast::FuncType *funType = assertDownCast<ast::FuncType>(lambda.exprType.get());
    llvm::Function *lamCtor = ctx.inst.get<PFGI::clo_lam_ctor>(funType);
    if (resultAddr) {
      value = nullptr;
    } else {
      resultAddr = builder.alloc(generateType(ctx.llvm, funType));
      value = resultAddr;
    }
    llvm::Value *opaqueCaptures = builder.ir.CreatePointerCast(captures, voidPtrTy(ctx.llvm));
    builder.ir.CreateCall(lamCtor, {resultAddr, body, opaqueCaptures});
    // @TODO lifetime.startLife
  }

private:
  Scope &temps;
  gen::Ctx ctx;
  FuncBuilder &builder;
  LifetimeExpr lifetime;
  llvm::Value *closure = nullptr;
  llvm::Value *value = nullptr;
  llvm::Value *result = nullptr;
  
  void storeValueAsResult(llvm::Value *resultAddr) {
    if (resultAddr) {
      // @TODO start the lifetime of the result object
      // maybe call lifetime.construct or lifetime.startLife
      builder.ir.CreateStore(value, resultAddr);
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
  FuncCtx funcCtx,
  ast::Expression *expr
) {
  Visitor visitor{temps, ctx, funcCtx.builder, funcCtx.ctx};
  return visitor.visitValue(expr);
}

gen::Expr stela::generateBoolExpr(
  Scope &temps,
  gen::Ctx ctx,
  FuncCtx funcCtx,
  ast::Expression *expr
) {
  Visitor visitor{temps, ctx, funcCtx.builder, funcCtx.ctx};
  return visitor.visitBool(expr);
}

gen::Expr stela::generateExpr(
  Scope &temps,
  gen::Ctx ctx,
  FuncCtx funcCtx,
  ast::Expression *expr,
  llvm::Value *result
) {
  Visitor visitor{temps, ctx, funcCtx.builder, funcCtx.ctx};
  return visitor.visitExpr(expr, result);
}
