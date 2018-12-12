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
#include "generate type.hpp"
#include "operator name.hpp"
#include "generate func.hpp"
#include <llvm/IR/IRBuilder.h>
#include "assert down cast.hpp"
#include "generate zero expr.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, llvm::IRBuilder<> &builder)
    : ctx{ctx}, builder{builder} {}

  enum class ArithNumber {
    signed_int,
    unsigned_int,
    floating_point
  };
  static ArithNumber classifyArith(ast::Expression *expr) {
    ast::BtnType *type = concreteType<ast::BtnType>(expr->exprType.get());
    assert(type);
    switch (type->value) {
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

  void visit(ast::BinaryExpr &expr) override {
    expr.left->accept(*this);
    llvm::Value *left = value;
    expr.right->accept(*this);
    llvm::Value *right = value;
    const ArithNumber arith = classifyArith(expr.left.get());
    
    #define INT_FLOAT_OP(INT_OP, FLOAT_OP)                                      \
      if (arith == ArithNumber::floating_point) {                               \
        value = builder.FLOAT_OP(left, right);                                  \
      } else {                                                                  \
        value = builder.INT_OP(left, right);                                    \
      }                                                                         \
      return
    
    #define SIGNED_UNSIGNED_FLOAT_OP(S_OP, U_OP, F_OP) \
      if (arith == ArithNumber::signed_int) { \
        value = builder.S_OP(left, right); \
      } else if (arith == ArithNumber::unsigned_int) { \
        value = builder.U_OP(left, right); \
      } else { \
        value = builder.F_OP(left, right); \
      } \
      return
    
    switch (expr.oper) {
      case ast::BinOp::bool_or:
      case ast::BinOp::bit_or:
        value = builder.CreateOr(left, right); return;
      case ast::BinOp::bit_xor:
        value = builder.CreateXor(left, right); return;
      case ast::BinOp::bool_and:
      case ast::BinOp::bit_and:
        value = builder.CreateAnd(left, right); return;
      case ast::BinOp::bit_shl:
        value = builder.CreateShl(left, right); return;
      case ast::BinOp::bit_shr:
        value = builder.CreateLShr(left, right); return;
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
    expr.expr->accept(*this);
    llvm::Value *operand = value;
    const ArithNumber arith = classifyArith(expr.expr.get());
    
    switch (expr.oper) {
      case ast::UnOp::neg:
        if (arith == ArithNumber::signed_int) {
          value = builder.CreateNSWNeg(operand);
        } else if (arith == ArithNumber::unsigned_int) {
          value = builder.CreateNeg(operand);
        } else {
          value = builder.CreateFNeg(operand);
        }
        return;
      case ast::UnOp::bool_not:
        value = builder.CreateXor(operand, 1); return;
      case ast::UnOp::bit_not:
        value = builder.CreateNot(operand); return;
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
  void visit(ast::FuncCall &call) override {
    if (call.definition == nullptr) {
      call.func->accept(*this);
      gen::String func = std::move(str);
      str += func;
      str += ".func(";
      str += func;
      str += ".data.get()";
      // @TODO get the parameter types of a function pointer
      pushArgs(call.args, {});
      str += ")";
    } else if (auto *func = dynamic_cast<ast::Func *>(call.definition)) {
      str += "f_";
      str += func->id;
      str += '(';
      if (func->receiver) {
        str += '(';
        assertDownCast<ast::MemberIdent>(call.func.get())->object->accept(*this);
        str += ')';
      } else {
        str += "nullptr";
      }
      pushArgs(call.args, func->symbol->params);
      str += ')';
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
    }
  }
  */
  void visit(ast::MemberIdent &mem) override {
    mem.object->accept(*this);
    value = builder.CreateStructGEP(value, mem.index);
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
  void visit(ast::Identifier &ident) override {
    if (!writeCapture(ident.captureIndex)) {
      writeID(ident.definition, ident.exprType.get(), ident.expectedType.get());
    }
  }
  void visit(ast::Ternary &tern) override {
    str += '(';
    tern.cond->accept(*this);
    str += ") ? (";
    tern.tru->accept(*this);
    str += ") : (";
    tern.fals->accept(*this);
    str += ')';
  }
  void visit(ast::Make &make) override {
    str += generateType(ctx, make.type.get());
    str += '(';
    make.expr->accept(*this);
    str += ')';
  }
  
  void visit(ast::StringLiteral &string) override {
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
      llvm::IntegerType::getInt8Ty(getLLVM()),
      static_cast<uint64_t>(chr.number),
      true
    );
  }
  void visit(ast::NumberLiteral &num) override {
    llvm::Type *type = generateType(ctx, num.exprType.get());
    std::visit([this, type] (auto val) {
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
    llvm::Type *boolType = llvm::IntegerType::getInt8Ty(getLLVM());
    if (bol.value) {
      value = llvm::ConstantInt::get(boolType, 1);
    } else {
      value = llvm::ConstantInt::get(boolType, 0);
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
  void visit(ast::InitList &list) override {
    if (list.exprs.empty()) {
      str += generateZeroExpr(ctx, list.exprType.get());
    } else {
      str += generateType(ctx, list.exprType.get());
      str += '{';
      pushExprs(list.exprs);
      str += '}';
    }
  }
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

  llvm::Value *value;

private:
  gen::Ctx ctx;
  llvm::IRBuilder<> &builder;
};

}

llvm::Value *stela::generateExpr(
  gen::Ctx ctx,
  llvm::IRBuilder<> &builder,
  ast::Expression *expr
) {
  Visitor visitor{ctx, builder};
  expr->accept(visitor);
  return visitor.value;
}
