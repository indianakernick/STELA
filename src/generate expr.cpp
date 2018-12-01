//
//  generate expr.cpp
//  STELA
//
//  Created by Indi Kernick on 1/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate expr.hpp"

#include "symbols.hpp"
#include "unreachable.hpp"
#include "generate type.hpp"
#include "operator name.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  explicit Visitor(gen::Ctx ctx)
    : ctx{ctx} {}

  void visit(ast::BinaryExpr &expr) override {
    str += '(';
    expr.left->accept(*this);
    str += ") ";
    str += opName(expr.oper);
    str += " (";
    expr.right->accept(*this);
    str += ')';
  }
  void visit(ast::UnaryExpr &expr) override {
    str += opName(expr.oper);
    str += '(';
    expr.expr->accept(*this);
    str += ')';
  }
  
  void pushArgs(const ast::FuncArgs &args, const sym::FuncParams &) {
    for (size_t i = 0; i != args.size(); ++i) {
      // @TODO use address operator for inout parameters
      str += ", (";
      args[i]->accept(*this);
      str += ')';
    }
  }
  void pushBtnFunc(const ast::BtnFuncEnum e) {
    // @TODO X macros?
    switch (e) {
      case ast::BtnFuncEnum::duplicate:
        str += "duplicate"; return;
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
      UNREACHABLE();
    }
    if (auto *func = dynamic_cast<ast::Func *>(call.definition)) {
      str += "f_";
      str += func->id;
      str += '(';
      if (func->receiver) {
        str += '(';
        auto *mem = dynamic_cast<ast::MemberIdent *>(call.func.get());
        assert(mem);
        mem->object->accept(*this);
        str += ')';
      } else {
        str += "nullptr";
      }
      pushArgs(call.args, func->symbol->params);
      str += ')';
    }
    if (auto *btnFunc = dynamic_cast<ast::BtnFunc *>(call.definition)) {
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
  void visit(ast::MemberIdent &mem) override {
    mem.object->accept(*this);
    str += ".m_";
    str += mem.index;
  }
  void visit(ast::Subscript &sub) override {
    str += "index(";
    sub.object->accept(*this);
    str += ", ";
    sub.index->accept(*this);
    str += ')';
  }
  void visit(ast::Identifier &ident) override {
    assert(ident.definition);
    if (auto *param = dynamic_cast<ast::FuncParam *>(ident.definition)) {
      str += "p_";
      str += param->index;
      return;
    }
    if (auto *var = dynamic_cast<ast::DeclAssign *>(ident.definition)) {
      str += "v_";
      str += var->id;
      return;
    }
    if (auto *var = dynamic_cast<ast::Var *>(ident.definition)) {
      str += "v_";
      str += var->id;
      return;
    }
    if (auto *let = dynamic_cast<ast::Let *>(ident.definition)) {
      str += "v_";
      str += let->id;
      return;
    }
    UNREACHABLE();
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
  }
  void visit(ast::CharLiteral &chr) override {
    str += '\'';
    str += chr.value;
    str += '\'';
  }
  void visit(ast::NumberLiteral &num) override {
    str += generateType(ctx, num.type.get());
    str += '(';
    str += num.value;
    str += ')';
  }
  void visit(ast::BoolLiteral &bol) override {
    if (bol.value) {
      str += "true";
    } else {
      str += "false";
    }
  }
  void pushExprs(const std::vector<ast::ExprPtr> &exprs) {
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
    if (arr.exprs.empty()) {
      str += "make_null_array<";
      str += generateType(ctx, arr.type.get());
      str += ">()";
    } else {
      str += "array_literal<";
      str += generateType(ctx, arr.type.get());
      str += ">(";
      pushExprs(arr.exprs);
      str += ')';
    }
  }
  void visit(ast::InitList &list) override {
    str += generateType(ctx, list.type.get());
    str += '{';
    pushExprs(list.exprs);
    str += '}';
  }

  gen::String str;

private:
  gen::Ctx ctx;
};

}

gen::String stela::generateExpr(gen::Ctx ctx, ast::Expression *expr) {
  Visitor visitor{ctx};
  expr->accept(visitor);
  return std::move(visitor.str);
}
