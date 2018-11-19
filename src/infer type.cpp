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
    const sym::ExprType left = visitValueExpr(bin.left);
    const sym::ExprType right = visitValueExpr(bin.right);
    if (auto builtinLeft = lookupConcrete<ast::BtnType>(ctx, left.type)) {
      if (auto builtinRight = lookupConcrete<ast::BtnType>(ctx, right.type)) {
        if (auto retType = validOp(ctx.btn, bin.oper, builtinLeft, builtinRight)) {
          lkp.setExpr(sym::makeLetVal(std::move(retType)));
          return;
        }
      }
    }
    ctx.log.error(bin.loc) << "Invalid operands to binary expression " << opName(bin.oper) << fatal;
  }
  void visit(ast::UnaryExpr &un) override {
    sym::ExprType etype = visitValueExpr(un.expr);
    if (auto builtin = lookupConcrete<ast::BtnType>(ctx, etype.type)) {
      if (validOp(un.oper, builtin)) {
        lkp.setExpr(sym::makeLetVal(std::move(etype.type)));
        return;
      }
    }
    ctx.log.error(un.loc) << "Invalid operand to unary expression " << opName(un.oper) << fatal;
  }
  sym::FuncParams argTypes(const ast::FuncArgs &args) {
    sym::FuncParams params;
    for (const ast::ExprPtr &expr : args) {
      // @TODO use visitValueExpr here
      params.push_back(getExprType(ctx, expr, nullptr));
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
    const sym::ExprType obj = visitValueExpr(sub.object);
    const sym::ExprType idx = visitValueExpr(sub.index);
    if (auto builtinIdx = lookupConcrete<ast::BtnType>(ctx, idx.type)) {
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
    ast::TypePtr resultType = type;
    const sym::ExprType cond = visitValueExpr(tern.cond, ctx.btn.Bool);
    if (!compareTypes(ctx, cond.type, ctx.btn.Bool)) {
      ctx.log.error(tern.loc) << "Condition expression must be of type Bool" << fatal;
    }
    const sym::ExprType tru = visitValueExpr(tern.tru, resultType);
    const sym::ExprType fals = visitValueExpr(tern.fals, resultType);
    if (!compareTypes(ctx, tru.type, fals.type)) {
      ctx.log.error(tern.loc) << "Branches of ternary have different types" << fatal;
    }
    sym::ExprType etype;
    etype.type = tru.type;
    etype.mut = sym::common(tru.mut, fals.mut);
    etype.ref = sym::common(tru.ref, fals.ref);
    lkp.setExpr(etype);
  }
  void visit(ast::Make &make) override {
    validateType(ctx, make.type);
    const sym::ExprType etype = visitValueExpr(make.expr, make.type);
    if (lookupConcrete<ast::BtnType>(ctx, make.type)) {
      if (lookupConcrete<ast::BtnType>(ctx, etype.type)) {
        return lkp.setExpr(sym::makeLetVal(make.type));
      }
    }
    if (compareTypes(ctx, lookupConcreteType(ctx, etype.type), lookupConcreteType(ctx, make.type))) {
      return lkp.setExpr(sym::makeLetVal(make.type));
    }
    ctx.log.error(make.loc) << "Invalid make expression" << fatal;
  }
  
  void visit(ast::StringLiteral &) override {
    lkp.setExpr(sym::makeLetVal(ctx.btn.string));
  }
  void visit(ast::CharLiteral &) override {
    lkp.setExpr(sym::makeLetVal(ctx.btn.Char));
  }
  void visit(ast::NumberLiteral &n) override {
    const NumberVariant num = parseNumberLiteral(n.value, ctx.log);
    ast::TypePtr type;
    if (std::holds_alternative<Byte>(num)) {
      type = ctx.btn.Byte;
    } else if (std::holds_alternative<Char>(num)) {
      type = ctx.btn.Char;
    } else if (std::holds_alternative<Real>(num)) {
      type = ctx.btn.Real;
    } else if (std::holds_alternative<Sint>(num)) {
      type = ctx.btn.Sint;
    } else if (std::holds_alternative<Uint>(num)) {
      type = ctx.btn.Uint;
    }
    lkp.setExpr(sym::makeLetVal(std::move(type)));
  }
  void visit(ast::BoolLiteral &) override {
    lkp.setExpr(sym::makeLetVal(ctx.btn.Bool));
  }
  void visit(ast::ArrayLiteral &arr) override {
    ast::TypePtr elem = nullptr;
    if (type) {
      if (auto array = lookupConcrete<ast::ArrayType>(ctx, type)) {
        elem = lookupStrongType(ctx, array->elem);
      } else {
        ctx.log.error(arr.loc) << "Array literal can only initialize arrays" << fatal;
      }
    }
    if (arr.exprs.empty()) {
      if (!elem) {
        ctx.log.error(arr.loc) << "Could not infer type of empty array" << fatal;
      }
    } else {
      sym::ExprType expr = getExprType(ctx, arr.exprs.front(), elem);
      for (auto e = arr.exprs.cbegin() + 1; e != arr.exprs.cend(); ++e) {
        const sym::ExprType currExpr = getExprType(ctx, *e, elem);
        if (!compareTypes(ctx, expr.type, currExpr.type)) {
          ctx.log.error(e->get()->loc) << "Unmatching types in array literal" << fatal;
        }
      }
      elem = lookupStrongType(ctx, std::move(expr.type));
    }
    auto array = make_retain<ast::ArrayType>();
    array->loc = arr.loc;
    array->elem = std::move(elem);
    lkp.setExpr(sym::makeLetVal(std::move(array)));
  }
  void visit(ast::InitList &list) override {
    if (!type) {
      ctx.log.error(list.loc) << "Could not infer type of init list" << fatal;
    }
    if (!list.exprs.empty()) {
      if (auto strut = lookupConcrete<ast::StructType>(ctx, type)) {
        if (list.exprs.size() > strut->fields.size()) {
          ctx.log.error(list.loc) << "Too many expressions in initializer list" << fatal;
        } else if (list.exprs.size() < strut->fields.size()) {
          ctx.log.error(list.loc) << "Too few expressions in initializer list" << fatal;
        }
        for (size_t i = 0; i != list.exprs.size(); ++i) {
          const ast::ExprPtr &expr = list.exprs[i];
          const ast::Field &field = strut->fields[i];
          const sym::ExprType exprType = getExprType(ctx, expr, field.type);
          if (!compareTypes(ctx, exprType.type, field.type)) {
            ctx.log.error(expr->loc) << "Initializer list type doesn't match struct field type" << fatal;
          }
        }
      } else {
        ctx.log.error(list.loc) << "Initializer list can only initialize structs" << fatal;
      }
    }
    lkp.setExpr(sym::makeLetVal(std::move(type)));
  }
  void visit(ast::Lambda &) override {}

  sym::ExprType visitValueExpr(const ast::ExprPtr &expr, const ast::TypePtr &type = nullptr) {
    this->type = type;
    lkp.enterSubExpr();
    expr->accept(*this);
    return lkp.leaveSubExpr();
  }

private:
  ExprLookup lkp;
  sym::Ctx ctx;
  ast::TypePtr type;
};

}

sym::ExprType stela::getExprType(sym::Ctx ctx, const ast::ExprPtr &expr, const ast::TypePtr &type) {
  return Visitor{ctx}.visitValueExpr(expr, type);
}
