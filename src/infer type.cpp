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
using namespace stela::ast;

namespace {

class InferVisitor final : public ast::Visitor {
public:
  /// Context is the parent ast::Node
  enum class Ctx {
    call, // parent is ast::FuncCall
    member, // parent is ast::MemberIdent
    expr // parent is anything
  };

  explicit InferVisitor(stela::SymbolMan &man)
    : man{man}, log{man.logger()} {}

  void visit(Assignment &as) override {
    as.left->accept(*this);
    const sym::ExprType left = etype;
    as.right->accept(*this);
    const sym::ExprType right = etype;
    sym::Func *func = man.lookup(opName(as.oper), {left, right}, as.loc);
    etype = func->ret;
  }
  void visit(BinaryExpr &bin) override {
    bin.left->accept(*this);
    const sym::ExprType left = etype;
    bin.right->accept(*this);
    const sym::ExprType right = etype;
    sym::Func *func = man.lookup(opName(bin.oper), {left, right}, bin.loc);
    etype = func->ret;
  }
  void visit(UnaryExpr &un) override {
    un.expr->accept(*this);
    sym::Func *func = man.lookup(opName(un.oper), {etype}, un.loc);
    etype = func->ret;
  }
  void visit(FuncCall &call) override {
    const Ctx oldCtx = std::exchange(ctx, Ctx::call);
    call.func->accept(*this);
    ctx = oldCtx;
    const std::string_view funcName = name;
    const sym::ExprType funcType = etype;
    sym::FuncParams params;
    for (const ast::ExprPtr &expr : call.args) {
      expr->accept(*this);
      params.push_back(etype);
    }
    if (funcType.type == nullptr) {
      sym::Func *func = man.lookup(funcName, params, call.loc);
      etype = func->ret;
    } else {
      if (auto *strut = dynamic_cast<sym::StructType *>(etype.type)) {
        //sym::Func *func = man.memberLookup
      }
    }
  }
  void visit(MemberIdent &mem) override {
    name = mem.member;
    if (ctx == Ctx::call) {
      
    }
    const Ctx oldCtx = std::exchange(ctx, Ctx::member);
    mem.object->accept(*this);
    ctx = oldCtx;
    // lookup members
  }
  void visit(InitCall &init) override {
    etype.type = man.type(init.type);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(Identifier &id) override {
    name = id.name;
    if (ctx == Ctx::call) {
      // null type indicates that `name` is the name of a free function
      etype.type = nullptr;
    } else {
      sym::Symbol *symbol = man.lookup(id.name, id.loc);
      if (auto *obj = dynamic_cast<sym::Object *>(symbol)) {
        etype = obj->etype;
      } else {
        log.ferror(id.loc) << '"' << id.name << "\" is not an object" << endlog;
      }
    }
  }
  void visit(Self &) override {
    
  }
  void visit(Ternary &tern) override {
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
  
  void visit(StringLiteral &s) override {
    etype.type = man.lookup("String", s.loc);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(CharLiteral &c) override {
    etype.type = man.lookup("Char", c.loc);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(NumberLiteral &n) override {
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
  void visit(BoolLiteral &b) override {
    etype.type = man.lookup("Bool", b.loc);
    etype.cat = sym::ValueCat::rvalue;
  }
  void visit(ArrayLiteral &) override {}
  void visit(MapLiteral &) override {}
  void visit(Lambda &) override {}
  
  sym::ExprType etype;

private:
  SymbolMan &man;
  Log &log;
  Ctx ctx = Ctx::expr;
  std::string_view name;
};

}

sym::ExprType stela::inferType(SymbolMan &man, ast::Expression *expr) {
  InferVisitor visitor{man};
  expr->accept(visitor);
  return std::move(visitor.etype);
}
