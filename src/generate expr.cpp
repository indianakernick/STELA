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
#include "lifetime exprs.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, FuncBuilder &builder)
    : ctx{ctx}, funcBdr{builder} {}

  gen::Expr visitValue(ast::Expression *expr) {
    assert(classifyType(expr->exprType.get()) == TypeCat::trivially_copyable);
    expr->accept(*this);
    if (glvalue(cat)) {
      return {funcBdr.ir.CreateLoad(value), cat};
    } else {
      return {value, cat};
    }
  }
  gen::Expr visitExpr(ast::Expression *expr) {
    expr->accept(*this);
    return {value, cat};
  }

  void visit(ast::BinaryExpr &expr) override {
    gen::Expr left = visitValue(expr.left.get());
    gen::Expr right = visitValue(expr.right.get());
    const ArithCat arith = classifyArith(expr.left.get());
    
    #define INT_FLOAT_OP(INT_OP, FLOAT_OP)                                      \
      if (arith == ArithCat::floating_point) {                                  \
        value = funcBdr.ir.FLOAT_OP(left.obj, right.obj);                       \
      } else {                                                                  \
        value = funcBdr.ir.INT_OP(left.obj, right.obj);                         \
      }                                                                         \
      break
    
    #define SIGNED_UNSIGNED_FLOAT_OP(S_OP, U_OP, F_OP)                          \
      if (arith == ArithCat::signed_int) {                                      \
        value = funcBdr.ir.S_OP(left.obj, right.obj);                           \
      } else if (arith == ArithCat::unsigned_int) {                             \
        value = funcBdr.ir.U_OP(left.obj, right.obj);                           \
      } else {                                                                  \
        value = funcBdr.ir.F_OP(left.obj, right.obj);                           \
      }                                                                         \
      break
    
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
        INT_FLOAT_OP(CreateICmpEQ, CreateFCmpOEQ);
      case ast::BinOp::ne:
        INT_FLOAT_OP(CreateICmpNE, CreateFCmpONE);
      case ast::BinOp::lt:
        SIGNED_UNSIGNED_FLOAT_OP(CreateICmpSLT, CreateICmpULT, CreateFCmpOLT);
      case ast::BinOp::le:
        SIGNED_UNSIGNED_FLOAT_OP(CreateICmpSLE, CreateICmpULE, CreateFCmpOLE);
      case ast::BinOp::gt:
        SIGNED_UNSIGNED_FLOAT_OP(CreateICmpSGT, CreateICmpUGT, CreateFCmpOGT);
      case ast::BinOp::ge:
        SIGNED_UNSIGNED_FLOAT_OP(CreateICmpSGE, CreateICmpUGE, CreateFCmpOGE);
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
    #undef INT_FLOAT_OP
    
    cat = ValueCat::prvalue;
  }
  void visit(ast::UnaryExpr &expr) override {
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
    cat = ValueCat::prvalue;
  }
  
  /*void pushArgs(const ast::FuncArgs &args, const sym::FuncParams &) {
    for (size_t i = 0; i != args.size(); ++i) {
      // @TODO use address operator for ref parameters
      str += ", (";
      args[i]->accept(*this);
      str += ')';
    }
  }
  void pushBtnFunc(const ast::BtnFuncEnum e) {
    // @TODO X macros?
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
    const gen::Expr evalExpr = visitExpr(expr);
    if (param.ref == ast::ParamRef::ref) {
      assert(evalExpr.cat == ValueCat::lvalue);
      return evalExpr.obj;
    }
    // @TODO how similar is this to createTemp for ast::Ternary?
    const TypeCat typeCat = classifyType(param.type.get());
    if (typeCat == TypeCat::trivially_copyable) {
      if (evalExpr.cat == ValueCat::lvalue || evalExpr.cat == ValueCat::xvalue) {
        return funcBdr.ir.CreateLoad(evalExpr.obj);
      } else { // prvalue
        return evalExpr.obj;
      }
    } else { // trivially_relocatable or nontrivial
      llvm::Value *addr = funcBdr.alloc(evalExpr.obj->getType()->getPointerElementType());
      LifetimeExpr lifetime{ctx.inst, funcBdr.ir};
      lifetime.construct(param.type.get(), addr, evalExpr);
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
        llvm::Type *retType = funcType->params().back()->getPointerElementType();
        llvm::Value *retAddr = funcBdr.alloc(retType);
        args.push_back(retAddr);
        funcBdr.ir.CreateCall(func->llvmFunc, args);
        value = retAddr;
      } else {
        value = funcBdr.ir.CreateCall(func->llvmFunc, args);
      }
      
      LifetimeExpr lifetime{ctx.inst, funcBdr.ir};
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
    cat = ValueCat::prvalue;
    /*if (call.definition == nullptr) {
      call.func->accept(*this);
      gen::String func = std::move(str);
      str += func;
      str += ".func(";
      str += func;
      str += ".data.get()";
      // @TODO get the parameter types of a function pointer
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
  
  void visit(ast::MemberIdent &mem) override {
    const gen::Expr object = visitExpr(mem.object.get());
    value = funcBdr.ir.CreateStructGEP(object.obj, mem.index);
    if (object.cat == ValueCat::lvalue) {
      cat = ValueCat::lvalue;
    } else if (object.cat == ValueCat::xvalue) {
      cat = ValueCat::xvalue;
    } else if (object.cat == ValueCat::prvalue) {
      // @FIXME prvalue is materialized to an xvalue
      // add this object to a list of destructors for the expression
      cat = ValueCat::xvalue;
    }
  }
  
  void visit(ast::Subscript &sub) override {
    sub.object->accept(*this);
    llvm::Value *object = value;
    const ValueCat objectCat = cat;
    gen::Expr index = visitValue(sub.index.get());
    ast::BtnType *indexType = concreteType<ast::BtnType>(sub.index->exprType.get());
    
    llvm::Type *arrayType = object->getType()->getPointerElementType();
    llvm::Function *indexFn;
    if (indexType->value == ast::BtnTypeEnum::Sint) {
      indexFn = ctx.inst.arrayIdxS(arrayType);
    } else {
      indexFn = ctx.inst.arrayIdxU(arrayType);
    }
    
    llvm::Value *array = funcBdr.ir.CreateLoad(object);
    value = funcBdr.ir.CreateCall(indexFn, {array, index.obj});
    
    if (objectCat == ValueCat::lvalue) {
      cat = ValueCat::lvalue;
    } else if (objectCat == ValueCat::xvalue) {
      cat = ValueCat::xvalue;
    } else if (objectCat == ValueCat::prvalue) {
      // @FIXME prvalue is materialized to an xvalue
      // add this object to a list of destructors for the expression
      cat = ValueCat::xvalue;
    }
  }
  /*
  void writeID(ast::Statement *definition, ast::Type *exprType, ast::Type *expectedType) {
    // @TODO this function is too big. Chop, chop!
    assert(definition);
    gen::String name;
    if (auto *param = dynamic_cast<ast::FuncParam *>(definition)) {
      // @TODO uncomment when we move from references to pointers
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
    cat = ValueCat::lvalue;
    assert(value->getType()->isPointerTy());
  }
  
  llvm::Value *createTemp(ast::Type *type, const gen::Expr expr) {
    if (glvalue(expr.cat)) {
      if (classifyType(type) == TypeCat::trivially_copyable) {
        return funcBdr.ir.CreateLoad(expr.obj);
      } else {
        llvm::Value *addr = funcBdr.alloc(expr.obj->getType()->getPointerElementType());
        LifetimeExpr lifetime{ctx.inst, funcBdr.ir};
        lifetime.construct(type, addr, expr);
        return addr;
      }
    } else {
      return expr.obj;
    }
  }
  
  void visit(ast::Ternary &tern) override {
    auto *condBlock = funcBdr.nextEmpty();
    auto *trooBlock = funcBdr.makeBlock();
    auto *folsBlock = funcBdr.makeBlock();
    auto *doneBlock = funcBdr.makeBlock();
    
    funcBdr.setCurr(condBlock);
    funcBdr.ir.CreateCondBr(visitValue(tern.cond.get()).obj, trooBlock, folsBlock);
    
    funcBdr.setCurr(trooBlock);
    gen::Expr tru = visitExpr(tern.tru.get());
    
    funcBdr.setCurr(folsBlock);
    gen::Expr fals = visitExpr(tern.fals.get());
    
    if (tru.cat == ValueCat::lvalue && fals.cat == ValueCat::lvalue) {
      cat = ValueCat::lvalue;
    } else {
      cat = ValueCat::prvalue;
      funcBdr.setCurr(trooBlock);
      tru.obj = createTemp(tern.tru->exprType.get(), tru);
      funcBdr.setCurr(folsBlock);
      fals.obj = createTemp(tern.fals->exprType.get(), fals);
    }
    
    funcBdr.link(trooBlock, doneBlock);
    funcBdr.link(folsBlock, doneBlock);
    
    funcBdr.setCurr(doneBlock);
    llvm::PHINode *phi = funcBdr.ir.CreatePHI(tru.obj->getType(), 2);
    phi->addIncoming(tru.obj, trooBlock);
    phi->addIncoming(fals.obj, folsBlock);
    value = phi;
  }
  void visit(ast::Make &make) override {
    if (!make.cast) {
      make.expr->accept(*this);
      return;
    }
    llvm::Type *type = generateType(ctx, make.type.get());
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
    cat = ValueCat::prvalue;
  }
  
  /*void visit(ast::StringLiteral &string) override {
    if (string.value.empty()) {
      str += "make_null_string()";
    } else {
      str += "string_literal(\"";
      str += string.value;
      str += "\")";
    }
  }*/
  void visit(ast::CharLiteral &chr) override {
    value = llvm::ConstantInt::get(
      llvm::IntegerType::getInt8Ty(ctx.llvm),
      static_cast<uint64_t>(chr.number),
      true
    );
    cat = ValueCat::prvalue;
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
    llvm::Type *type = generateType(ctx, num.exprType.get());
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
    }, num.number);
    cat = ValueCat::prvalue;
  }
  void visit(ast::BoolLiteral &bol) override {
    llvm::Type *boolType = llvm::IntegerType::getInt1Ty(ctx.llvm);
    if (bol.value) {
      value = llvm::ConstantInt::getTrue(boolType);
    } else {
      value = llvm::ConstantInt::getFalse(boolType);
    }
    cat = ValueCat::prvalue;
  }
  /*void pushExprs(const std::vector<ast::ExprPtr> &exprs) {
    if (exprs.empty()) {
      return;
    }
    str += '(';
    exprs[0]->accept(*this);
    str += ')';
    for (auto e = exprs.cbegin() + 1; e != exprs.cend(); ++e) {
      str += ", (";
      (*e)->accept(*this);
      str += ')';
    }
  }
  void visit(ast::ArrayLiteral &arr) override {
    ast::TypePtr elem = assertDownCast<ast::ArrayType>(arr.exprType.get())->elem;
    if (arr.exprs.empty()) {
      str += "make_null_array<";
      str += generateType(ctx, elem.get());
      str += ">()";
    } else {
      str += "array_literal<";
      str += generateType(ctx, elem.get());
      str += ">(";
      pushExprs(arr.exprs);
      str += ')';
    }
  }
  */
  void visit(ast::InitList &list) override {
    ast::Type *type = list.exprType.get();
    llvm::Value *addr = funcBdr.alloc(generateType(ctx, type));
    LifetimeExpr lifetime{ctx.inst, funcBdr.ir};
    if (list.exprs.empty()) {
      lifetime.defConstruct(type, addr);
    } else {
      for (unsigned e = 0; e != list.exprs.size(); ++e) {
        llvm::Value *fieldAddr = funcBdr.ir.CreateStructGEP(addr, e);
        ast::Expression *fieldExpr = list.exprs[e].get();
        ast::Type *fieldType = fieldExpr->exprType.get();
        lifetime.construct(fieldType, fieldAddr, visitExpr(fieldExpr));
      }
    }
    if (classifyType(type) == TypeCat::trivially_copyable) {
      value = funcBdr.ir.CreateLoad(addr);
    } else {
      value = addr;
    }
    cat = ValueCat::prvalue;
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

  llvm::Value *value = nullptr;
  ValueCat cat;

private:
  gen::Ctx ctx;
  FuncBuilder &funcBdr;
};

}

gen::Expr stela::generateValueExpr(
  gen::Ctx ctx,
  FuncBuilder &builder,
  ast::Expression *expr
) {
  Visitor visitor{ctx, builder};
  return visitor.visitValue(expr);
}

gen::Expr stela::generateExpr(
  gen::Ctx ctx,
  FuncBuilder &builder,
  ast::Expression *expr
) {
  Visitor visitor{ctx, builder};
  return visitor.visitExpr(expr);
}
