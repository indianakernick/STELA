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
#include "unreachable.hpp"
#include "type builder.hpp"
#include "generate type.hpp"
#include "operator name.hpp"
#include "generate func.hpp"
#include "assert down cast.hpp"
#include "generate zero expr.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, FuncBuilder &builder)
    : ctx{ctx}, funcBdr{builder} {}

  llvm::Value *visitValue(ast::Expression *expr) {
    expr->accept(*this);
    if (value->getType()->isPointerTy()) {
      return funcBdr.ir.CreateLoad(value);
    } else {
      return value;
    }
  }
  llvm::Value *visitAddr(ast::Expression *expr) {
    expr->accept(*this);
    if (value->getType()->isPointerTy()) {
      return value;
    } else {
      return funcBdr.allocStore(value);
    }
  }

  void visit(ast::BinaryExpr &expr) override {
    llvm::Value *left = visitValue(expr.left.get());
    llvm::Value *right = visitValue(expr.right.get());
    const ArithNumber arith = classifyArith(expr.left.get());
    
    #define INT_FLOAT_OP(INT_OP, FLOAT_OP)                                      \
      if (arith == ArithNumber::floating_point) {                               \
        value = funcBdr.ir.FLOAT_OP(left, right);                               \
      } else {                                                                  \
        value = funcBdr.ir.INT_OP(left, right);                                 \
      }                                                                         \
      return
    
    #define SIGNED_UNSIGNED_FLOAT_OP(S_OP, U_OP, F_OP)                          \
      if (arith == ArithNumber::signed_int) {                                   \
        value = funcBdr.ir.S_OP(left, right);                                   \
      } else if (arith == ArithNumber::unsigned_int) {                          \
        value = funcBdr.ir.U_OP(left, right);                                   \
      } else {                                                                  \
        value = funcBdr.ir.F_OP(left, right);                                   \
      }                                                                         \
      return
    
    switch (expr.oper) {
      case ast::BinOp::bool_or:
      case ast::BinOp::bit_or:
        value = funcBdr.ir.CreateOr(left, right); return;
      case ast::BinOp::bit_xor:
        value = funcBdr.ir.CreateXor(left, right); return;
      case ast::BinOp::bool_and:
      case ast::BinOp::bit_and:
        value = funcBdr.ir.CreateAnd(left, right); return;
      case ast::BinOp::bit_shl:
        value = funcBdr.ir.CreateShl(left, right); return;
      case ast::BinOp::bit_shr:
        value = funcBdr.ir.CreateLShr(left, right); return;
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
    }
    UNREACHABLE();
    
    #undef SIGNED_UNSIGNED_FLOAT_OP
    #undef INT_FLOAT_OP
  }
  void visit(ast::UnaryExpr &expr) override {
    llvm::Value *operand = visitValue(expr.expr.get());
    const ArithNumber arith = classifyArith(expr.expr.get());
    
    switch (expr.oper) {
      case ast::UnOp::neg:
        if (arith == ArithNumber::signed_int) {
          value = funcBdr.ir.CreateNSWNeg(operand);
        } else if (arith == ArithNumber::unsigned_int) {
          value = funcBdr.ir.CreateNeg(operand);
        } else {
          value = funcBdr.ir.CreateFNeg(operand);
        }
        return;
      case ast::UnOp::bool_not:
      case ast::UnOp::bit_not:
        value = funcBdr.ir.CreateNot(operand); return;
    }
    UNREACHABLE();
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
  llvm::Value *visitParam(const ast::ParamRef ref, ast::Expression *expr, llvm::Type *type) {
    if (ref == ast::ParamRef::ref) {
      return visitAddr(expr);
    } else if (type->isPointerTy()) {
      return visitAddr(expr);
    } else {
      return visitValue(expr);
    }
  }
  void pushArgs(
    std::vector<llvm::Value *> &args,
    const ast::FuncArgs &callArgs,
    const ast::FuncParams &params,
    llvm::ArrayRef<llvm::Type *> paramTypes
  ) {
    for (size_t a = 0; a != callArgs.size(); ++a) {
      args.push_back(visitParam(params[a].ref, callArgs[a].get(), paramTypes[a + 1]));
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
      if (func->receiver) {
        const ast::ParamRef ref = func->receiver->ref;
        ast::Expression *self = assertDownCast<ast::MemberIdent>(call.func.get())->object.get();
        args.push_back(visitParam(ref, self, funcType->getParamType(0)));
      } else {
        TypeBuilder typeBdr{ctx.llvm};
        args.push_back(typeBdr.nullPtr(typeBdr.voidPtr()));
      }
      pushArgs(args, call.args, func->params, funcType->params());
      
      if (funcType->params().size() == args.size() + 1) {
        llvm::Type *retType = funcType->params().back()->getPointerElementType();
        llvm::Value *retAddr = funcBdr.alloc(retType);
        args.push_back(retAddr);
        funcBdr.ir.CreateCall(func->llvmFunc, args);
        value = funcBdr.ir.CreateLoad(retAddr);
      } else {
        value = funcBdr.ir.CreateCall(func->llvmFunc, args);
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
    mem.object->accept(*this);
    value = funcBdr.ir.CreateStructGEP(value, mem.index);
  }
  /*
  void visit(ast::Subscript &sub) override {
    str += "index(";
    sub.object->accept(*this);
    str += ", ";
    sub.index->accept(*this);
    str += ')';
  }
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
  }
  
  void visit(ast::Ternary &tern) override {
    auto *condBlock = funcBdr.nextEmpty();
    auto *trooBlock = funcBdr.makeBlock();
    auto *folsBlock = funcBdr.makeBlock();
    auto *doneBlock = funcBdr.makeBlock();
    
    funcBdr.setCurr(condBlock);
    funcBdr.ir.CreateCondBr(visitValue(tern.cond.get()), trooBlock, folsBlock);
    
    funcBdr.setCurr(trooBlock);
    tern.tru->accept(*this);
    llvm::Value *tru = value;
    
    funcBdr.setCurr(folsBlock);
    tern.fals->accept(*this);
    llvm::Value *fals = value;
    
    if (tru->getType()->isPointerTy() && !fals->getType()->isPointerTy()) {
      funcBdr.setCurr(trooBlock);
      tru = funcBdr.ir.CreateLoad(tru);
    } else if (!tru->getType()->isPointerTy() && fals->getType()->isPointerTy()) {
      funcBdr.setCurr(folsBlock);
      fals = funcBdr.ir.CreateLoad(fals);
    }
    
    funcBdr.link(trooBlock, doneBlock);
    funcBdr.link(folsBlock, doneBlock);
    
    funcBdr.setCurr(doneBlock);
    llvm::PHINode *phi = funcBdr.ir.CreatePHI(tru->getType(), 2);
    phi->addIncoming(tru, trooBlock);
    phi->addIncoming(fals, folsBlock);
    value = phi;
  }
  void visit(ast::Make &make) override {
    if (!make.cast) {
      make.expr->accept(*this);
      return;
    }
    llvm::Type *type = generateType(ctx, make.type.get());
    llvm::Value *srcVal = visitValue(make.expr.get());
    const ArithNumber dst = classifyArith(make.type.get());
    const ArithNumber src = classifyArith(make.expr.get());
    if (src == ArithNumber::signed_int) {
      if (dst == ArithNumber::signed_int) {
        value = funcBdr.ir.CreateIntCast(srcVal, type, true);
      } else if (dst == ArithNumber::unsigned_int) {
        value = funcBdr.ir.CreateIntCast(srcVal, type, true);
      } else {
        value = funcBdr.ir.CreateCast(llvm::Instruction::SIToFP, srcVal, type);
      }
    } else if (src == ArithNumber::unsigned_int) {
      if (dst == ArithNumber::signed_int) {
        value = funcBdr.ir.CreateIntCast(srcVal, type, false);
      } else if (dst == ArithNumber::unsigned_int) {
        value = funcBdr.ir.CreateIntCast(srcVal, type, false);
      } else {
        value = funcBdr.ir.CreateCast(llvm::Instruction::UIToFP, srcVal, type);
      }
    } else {
      if (dst == ArithNumber::signed_int) {
        value = funcBdr.ir.CreateCast(llvm::Instruction::FPToSI, srcVal, type);
      } else if (dst == ArithNumber::unsigned_int) {
        value = funcBdr.ir.CreateCast(llvm::Instruction::FPToUI, srcVal, type);
      } else {
        value = funcBdr.ir.CreateFPCast(srcVal, type);
      }
    }
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
  }
  void visit(ast::BoolLiteral &bol) override {
    llvm::Type *boolType = llvm::IntegerType::getInt1Ty(ctx.llvm);
    if (bol.value) {
      value = llvm::ConstantInt::getTrue(boolType);
    } else {
      value = llvm::ConstantInt::getFalse(boolType);
    }
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
    if (list.exprs.empty()) {
      value = generateZeroExpr(ctx, funcBdr, type);
    } else {
      llvm::Type *strut = generateType(ctx, type);
      llvm::Value *addr = funcBdr.alloc(strut);
      for (unsigned e = 0; e != list.exprs.size(); ++e) {
        llvm::Value *fieldAddr = funcBdr.ir.CreateStructGEP(addr, e);
        funcBdr.ir.CreateStore(visitValue(list.exprs[e].get()), fieldAddr);
      }
      value = funcBdr.ir.CreateLoad(addr);
    }
  }
  /*
  void writeCapture(const sym::ClosureCap &cap) {
    if (!writeCapture(cap.index)) {
      writeID(cap.object, cap.type.get(), nullptr);
    }
  }
  void visit(ast::Lambda &lambda) override {
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

private:
  gen::Ctx ctx;
  FuncBuilder &funcBdr;
};

}

ArithNumber stela::classifyArith(ast::Type *type) {
  ast::BtnType *btn = assertConcreteType<ast::BtnType>(type);
  switch (btn->value) {
    case ast::BtnTypeEnum::Char:
    case ast::BtnTypeEnum::Sint:
      return ArithNumber::signed_int;
    case ast::BtnTypeEnum::Bool:
    case ast::BtnTypeEnum::Byte:
    case ast::BtnTypeEnum::Uint:
      return ArithNumber::unsigned_int;
    case ast::BtnTypeEnum::Real:
      return ArithNumber::floating_point;
    case ast::BtnTypeEnum::Void: ;
  }
  UNREACHABLE();
}

ArithNumber stela::classifyArith(ast::Expression *expr) {
  return classifyArith(expr->exprType.get());
}

llvm::Value *stela::generateAddrExpr(
  gen::Ctx ctx,
  FuncBuilder &builder,
  ast::Expression *expr
) {
  Visitor visitor{ctx, builder};
  return visitor.visitAddr(expr);
}

llvm::Value *stela::generateValueExpr(
  gen::Ctx ctx,
  FuncBuilder &builder,
  ast::Expression *expr
) {
  Visitor visitor{ctx, builder};
  return visitor.visitValue(expr);
}

llvm::Value *stela::generateExpr(
  gen::Ctx ctx,
  FuncBuilder &builder,
  ast::Expression *expr
) {
  Visitor visitor{ctx, builder};
  expr->accept(visitor);
  return visitor.value;
}
