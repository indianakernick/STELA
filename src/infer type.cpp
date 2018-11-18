//
//  infer type.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "infer type.hpp"

#include "scope lookup.hpp"
#include "operator name.hpp"
#include "compare types.hpp"
#include "number literal.hpp"
#include "builtin symbols.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  explicit Visitor(sym::Ctx ctx)
    : lkp{ctx}, ctx{ctx} {}

  void visit(ast::BinaryExpr &bin) override {
    const sym::ExprType left = visitValueExpr(bin.left.get());
    const sym::ExprType right = visitValueExpr(bin.right.get());
    if (auto builtinLeft = lookupConcrete<ast::BuiltinType>(ctx, left.type)) {
      if (auto builtinRight = lookupConcrete<ast::BuiltinType>(ctx, right.type)) {
        if (auto retType = validOp(ctx.btn, bin.oper, builtinLeft, builtinRight)) {
          sym::ExprType retExpr;
          retExpr.type = retType;
          retExpr.mut = sym::ValueMut::let;
          retExpr.ref = sym::ValueRef::val;
          lkp.setExpr(retExpr);
          return;
        }
      }
    }
    ctx.log.error(bin.loc) << "Invalid operands to binary expression " << opName(bin.oper) << fatal;
  }
  void visit(ast::UnaryExpr &un) override {
    const sym::ExprType etype = visitValueExpr(un.expr.get());
    if (auto builtin = lookupConcrete<ast::BuiltinType>(ctx, etype.type)) {
      if (validOp(un.oper, builtin)) {
        sym::ExprType retExpr;
        retExpr.type = etype.type;
        retExpr.mut = sym::ValueMut::let;
        retExpr.ref = sym::ValueRef::val;
        lkp.setExpr(retExpr);
        return;
      }
    }
    ctx.log.error(un.loc) << "Invalid operand to unary expression " << opName(un.oper) << fatal;
  }
  sym::FuncParams argTypes(const ast::FuncArgs &args) {
    sym::FuncParams params;
    for (const ast::ExprPtr &expr : args) {
      // @TODO use visitValueExpr here
      params.push_back(getExprType(ctx, expr.get()));
    }
    return params;
  }
  void visit(ast::FuncCall &call) override {
    lkp.call();
    call.func->accept(*this);
    call.definition = lkp.lookupFunc(argTypes(call.args), call.loc);
  }
  void visit(ast::MemberIdent &mem) override {
    lkp.member(sym::Name(mem.member));
    mem.object->accept(*this);
    mem.definition = lkp.lookupMember(mem.loc);
  }
  void visit(ast::Subscript &sub) override {
    const sym::ExprType obj = visitValueExpr(sub.object.get());
    const sym::ExprType idx = visitValueExpr(sub.index.get());
    if (auto builtinIdx = lookupConcrete<ast::BuiltinType>(ctx, idx.type)) {
      if (validSubscript(builtinIdx)) {
        if (auto array = lookupConcrete<ast::ArrayType>(ctx, obj.type)) {
          lkp.setExpr(sym::memberType(obj, lookupStrongType(ctx, array->elem)));
          return;
        }
        ctx.log.error(sub.object->loc) << "Subscripted value is not an array" << fatal;
      }
    }
    ctx.log.error(sub.index->loc) << "Invalid subscript index" << fatal;
  }
  void visit(ast::Identifier &id) override {
    id.definition = lkp.lookupIdent(sym::Name(id.module), sym::Name(id.name), id.loc);
  }
  void visit(ast::Ternary &tern) override {
    const sym::ExprType cond = visitValueExpr(tern.cond.get());
    if (!compareTypes(ctx, cond.type, ctx.btn.Bool)) {
      ctx.log.error(tern.loc) << "Condition expression must be of type Bool" << fatal;
    }
    const sym::ExprType tru = visitValueExpr(tern.tru.get());
    const sym::ExprType fals = visitValueExpr(tern.fals.get());
    if (!compareTypes(ctx, tru.type, fals.type)) {
      ctx.log.error(tern.loc) << "True and false branch of ternary condition must have same type" << fatal;
    }
    sym::ExprType etype;
    etype.type = tru.type;
    etype.mut = sym::common(tru.mut, fals.mut);
    etype.ref = sym::common(tru.ref, fals.ref);
    lkp.setExpr(etype);
  }
  
  void visit(ast::StringLiteral &) override {
    sym::ExprType etype;
    etype.type = ctx.btn.string;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::CharLiteral &) override {
    sym::ExprType etype;
    etype.type = ctx.btn.Char;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::NumberLiteral &n) override {
    const NumberVariant num = parseNumberLiteral(n.value, ctx.log);
    sym::ExprType etype;
    if (std::holds_alternative<Byte>(num)) {
      etype.type = ctx.btn.Byte;
    } else if (std::holds_alternative<Char>(num)) {
      etype.type = ctx.btn.Char;
    } else if (std::holds_alternative<Real>(num)) {
      etype.type = ctx.btn.Real;
    } else if (std::holds_alternative<Sint>(num)) {
      etype.type = ctx.btn.Sint;
    } else if (std::holds_alternative<Uint>(num)) {
      etype.type = ctx.btn.Uint;
    }
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::BoolLiteral &) override {
    sym::ExprType etype;
    etype.type = ctx.btn.Bool;
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::ArrayLiteral &arr) override {
    if (arr.exprs.empty()) {
      ctx.log.error(arr.loc) << "Empty array" << fatal;
    }
    const sym::ExprType expr = getExprType(ctx, arr.exprs.front().get());
    for (auto e = arr.exprs.cbegin() + 1; e != arr.exprs.cend(); ++e) {
      const sym::ExprType currExpr = getExprType(ctx, e->get());
      if (!compareTypes(ctx, expr.type, currExpr.type)) {
        ctx.log.error(e->get()->loc) << "Unmatching types in array literal" << fatal;
      }
    }
    auto array = make_retain<ast::ArrayType>();
    array->loc = arr.loc;
    array->elem = lookupStrongType(ctx, expr.type);
    sym::ExprType etype;
    etype.type = std::move(array);
    etype.mut = sym::ValueMut::let;
    etype.ref = sym::ValueRef::val;
    lkp.setExpr(etype);
  }
  void visit(ast::Lambda &) override {}

  sym::ExprType visitValueExpr(ast::Expression *const node) {
    lkp.enterSubExpr();
    node->accept(*this);
    return lkp.leaveSubExpr();
  }

private:
  ExprLookup lkp;
  sym::Ctx ctx;
};

}

sym::ExprType stela::getExprType(sym::Ctx ctx, ast::Expression *expr) {
  return Visitor{ctx}.visitValueExpr(expr);
}
