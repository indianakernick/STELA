//
//  infer type.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "infer type.hpp"

#include "operator name.hpp"
#include "number literal.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  /// Context is the parent ast::Node
  enum class Ctx {
    call,        // parent is ast::FuncCall
    member,      // parent is ast::MemberIdent
    expr
  };
  
  enum class Func {
    free,
    member,
    static_
  };

  Visitor(
    stela::SymbolMan &man,
    const Func func,
    sym::StructType *strut = nullptr
  ) : man{man}, log{man.logger()}, strut{strut}, func{func} {}

  void visit(ast::Assignment &as) override {
    as.left->accept(*this);
    const sym::ExprType left = etype;
    as.right->accept(*this);
    const sym::ExprType right = etype;
    sym::Func *func = man.lookup(sym::Name(opName(as.oper)), {left, right}, as.loc);
    etype = func->ret;
  }
  void visit(ast::BinaryExpr &bin) override {
    bin.left->accept(*this);
    const sym::ExprType left = etype;
    bin.right->accept(*this);
    const sym::ExprType right = etype;
    sym::Func *func = man.lookup(sym::Name(opName(bin.oper)), {left, right}, bin.loc);
    etype = func->ret;
  }
  void visit(ast::UnaryExpr &un) override {
    un.expr->accept(*this);
    sym::Func *func = man.lookup(sym::Name(opName(un.oper)), {etype}, un.loc);
    etype = func->ret;
  }
  sym::FuncParams argTypes(const ast::FuncArgs &args) {
    const Ctx oldCtx = std::exchange(ctx, Ctx::expr);
    sym::FuncParams params;
    for (const ast::ExprPtr &expr : args) {
      expr->accept(*this);
      params.push_back(etype);
    }
    ctx = oldCtx;
    return params;
  }
  void visit(ast::FuncCall &call) override {
    const Ctx oldCtx = std::exchange(ctx, Ctx::call);
    call.func->accept(*this);
    ctx = oldCtx;
    const std::string_view funcName = name;
    const sym::ExprType funcType = etype;
    const sym::FuncParams params = argTypes(call.args);
    if (funcType.type == nullptr) {
      sym::Func *func = man.lookup(sym::Name(funcName), params, call.loc);
      etype = func->ret;
    } else {
      if (auto *strut = dynamic_cast<sym::StructType *>(etype.type)) {
        sym::Func *func = man.memberLookup(strut, sym::Name(funcName), params, call.loc);
        etype = func->ret;
      } else {
        assert(false);
      }
    }
  }
  void visit(ast::MemberIdent &mem) override {
    const Ctx oldCtx = std::exchange(ctx, Ctx::member);
    mem.object->accept(*this);
    ctx = oldCtx;
    if (ctx == Ctx::call) {
      name = mem.member;
    } else {
      if (auto *strut = dynamic_cast<sym::StructType *>(etype.type)) {
        sym::Symbol *symbol = man.memberLookup(strut, sym::Name(mem.member), mem.loc);
        if (auto *obj = dynamic_cast<sym::Object *>(symbol)) {
          etype = obj->etype;
        } else {
          assert(false);
        }
      } else {
        assert(false);
      }
    }
  }
  void visit(ast::InitCall &init) override {
    etype.type = man.type(init.type);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(ast::Identifier &id) override {
    name = id.name;
    if (ctx == Ctx::call) {
      // null type indicates that `name` is the name of a free function
      etype.type = nullptr;
    } else {
      sym::Symbol *symbol = man.lookup(sym::Name(id.name), id.loc);
      if (auto *obj = dynamic_cast<sym::Object *>(symbol)) {
        etype = obj->etype;
      } else {
        log.ferror(id.loc) << '"' << id.name << "\" is not an object" << endlog;
      }
    }
  }
  void visit(ast::Self &self) override {
    if (func == Func::member) {
      etype.type = strut;
      etype.cat = sym::ValueCat::lvalue_var;
    } else {
      log.ferror(self.loc) << "Invalid usage of `self` outside of a member function" << endlog;
    }
  }
  void visit(ast::Ternary &tern) override {
    tern.cond->accept(*this);
    const sym::ExprType cond = etype;
    if (cond.type != man.lookup("Bool", {})) {
      log.ferror(tern.loc) << "Condition of ternary conditional must be a Bool" << endlog;
    }
    tern.tru->accept(*this);
    const sym::ExprType tru = etype;
    tern.fals->accept(*this);
    const sym::ExprType fals = etype;
    if (tru.type != fals.type) {
      log.ferror(tern.loc) << "True and false branch of ternary condition must have same type" << endlog;
    }
    etype.type = tru.type;
    etype.cat = mostRestrictive(tru.cat, fals.cat);
  }
  
  void visit(ast::StringLiteral &s) override {
    etype.type = man.lookup("String", s.loc);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(ast::CharLiteral &c) override {
    etype.type = man.lookup("Char", c.loc);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(ast::NumberLiteral &n) override {
    const NumberVariant num = parseNumberLiteral(n.value, log);
    if (num.type == NumberVariant::Float) {
      etype.type = man.lookup("Double", n.loc);
    } else if (num.type == NumberVariant::Int) {
      etype.type = man.lookup("Int64", n.loc);
    } else if (num.type == NumberVariant::UInt) {
      etype.type = man.lookup("UInt64", n.loc);
    }
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(ast::BoolLiteral &b) override {
    etype.type = man.lookup("Bool", b.loc);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(ast::ArrayLiteral &) override {}
  void visit(ast::MapLiteral &) override {}
  void visit(ast::Lambda &) override {}
  
  sym::ExprType etype;

private:
  SymbolMan &man;
  Log &log;
  std::string_view name;
  sym::StructType *strut;
  Func func;
  Ctx ctx = Ctx::expr;
};

}

sym::ExprType stela::exprFunc(SymbolMan &man, ast::Expression *expr) {
  Visitor visitor{man, Visitor::Func::free};
  expr->accept(visitor);
  return visitor.etype;
}

sym::ExprType stela::exprMemFunc(
  SymbolMan &man,
  ast::Expression *expr,
  sym::StructType *strut
) {
  Visitor visitor{man, Visitor::Func::member, strut};
  expr->accept(visitor);
  return visitor.etype;
}

sym::ExprType stela::exprStatFunc(
  SymbolMan &man,
  ast::Expression *expr,
  sym::StructType *strut
) {
  Visitor visitor{man, Visitor::Func::static_, strut};
  expr->accept(visitor);
  return visitor.etype;
}
