//
//  infer type.cpp
//  STELA
//
//  Created by Indi Kernick on 15/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "infer type.hpp"

#include "traverse.hpp"
#include "symbol desc.hpp"
#include "expr lookup.hpp"
#include "scope insert.hpp"
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
    ast::TypePtr expType = boolOp(bin.oper) ? ctx.btn.Bool : nullptr;
    const sym::ExprType left = visitExprCheck(bin.left, expType);
    const sym::ExprType right = visitExprCheck(bin.right, expType);
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
    ast::TypePtr expType = boolOp(un.oper) ? ctx.btn.Bool : nullptr;
    sym::ExprType etype = visitExprCheck(un.expr, expType);
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
    params.reserve(args.size());
    for (const ast::ExprPtr &expr : args) {
      // Can't call visitValueExpr because we'll lose the receiver type from the
      // expression stack
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
    lkp.member(sym::Name{mem.member});
    mem.object->accept(*this);
    mem.index = lkp.lookupMember(mem.loc);
  }
  void visit(ast::Subscript &sub) override {
    const sym::ExprType obj = visitExprCheck(sub.object);
    const sym::ExprType idx = visitExprCheck(sub.index);
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
    lkp.expected(expected);
    lkp.lookupIdent(id);
  }
  void visit(ast::Ternary &tern) override {
    visitExprCheck(tern.cond, ctx.btn.Bool);
    const sym::ExprType tru = visitExprCheck(tern.tru, expected);
    const sym::ExprType fals = visitExprCheck(tern.fals, expected);
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
    const sym::ExprType etype = visitExprNoCheckBool(make.expr, make.type);
    if (auto dst = lookupConcrete<ast::BtnType>(ctx, make.type)) {
      if (auto src = lookupConcrete<ast::BtnType>(ctx, etype.type)) {
        if (validCast(dst, src)) {
          return lkp.setExpr(sym::makeLetVal(make.type));
        }
      }
    }
    if (compareTypes(ctx, lookupConcreteType(ctx, etype.type), lookupConcreteType(ctx, make.type))) {
      return lkp.setExpr(sym::makeLetVal(make.type));
    }
    ctx.log.error(make.loc) << "Cannot make " << typeDesc(make.type)
      << " from " << typeDesc(etype.type) << fatal;
  }
  
  void visit(ast::StringLiteral &) override {
    // @TODO deal with \n \t and others
    lkp.setExpr(sym::makeLetVal(ctx.btn.string));
  }
  void visit(ast::CharLiteral &chr) override {
    if (chr.value.size() == 1) {
      // @TODO deal with \n \t and others
      chr.number = chr.value[0];
    }
    lkp.setExpr(sym::makeLetVal(ctx.btn.Char));
  }
  void visit(ast::NumberLiteral &n) override {
    n.number = parseNumberLiteral(n.value, ctx.log);
    ast::TypePtr type;
    if (std::holds_alternative<Byte>(n.number)) {
      type = ctx.btn.Byte;
    } else if (std::holds_alternative<Char>(n.number)) {
      type = ctx.btn.Char;
    } else if (std::holds_alternative<Real>(n.number)) {
      type = ctx.btn.Real;
    } else if (std::holds_alternative<Sint>(n.number)) {
      type = ctx.btn.Sint;
    } else if (std::holds_alternative<Uint>(n.number)) {
      type = ctx.btn.Uint;
    }
    lkp.setExpr(sym::makeLetVal(std::move(type)));
  }
  void visit(ast::BoolLiteral &) override {
    lkp.setExpr(sym::makeLetVal(ctx.btn.Bool));
  }
  void visit(ast::ArrayLiteral &arr) override {
    ast::TypePtr elem = nullptr;
    if (expected) {
      if (auto array = lookupConcrete<ast::ArrayType>(ctx, expected)) {
        elem = lookupStrongType(ctx, array->elem);
      } else {
        ctx.log.error(arr.loc) << "Array literal cannot initialize " << typeDesc(expected) << fatal;
      }
    }
    if (arr.exprs.empty()) {
      if (!elem) {
        ctx.log.error(arr.loc) << "Could not infer type of empty array" << fatal;
      }
    } else {
      sym::ExprType expr = visitExprCheck(arr.exprs.front(), elem);
      for (auto e = arr.exprs.cbegin() + 1; e != arr.exprs.cend(); ++e) {
        const sym::ExprType currExpr = visitExprNoCheck(*e, elem);
        if (!compareTypes(ctx, expr.type, currExpr.type)) {
          ctx.log.error(e->get()->loc) << "Unmatching types in array literal" << fatal;
        }
      }
      elem = lookupStrongType(ctx, std::move(expr.type));
      if (auto btn = lookupConcrete<ast::BtnType>(ctx, elem)) {
        if (btn->value == ast::BtnTypeEnum::Void) {
          ctx.log.error(arr.loc) << "Array element type cannot be void" << fatal;
        }
      }
    }
    auto array = make_retain<ast::ArrayType>();
    array->loc = arr.loc;
    array->elem = std::move(elem);
    lkp.setExpr(sym::makeLetVal(std::move(array)));
  }
  void visit(ast::InitList &list) override {
    if (!expected) {
      ctx.log.error(list.loc) << "Could not infer type of init list" << fatal;
    }
    if (!list.exprs.empty()) {
      if (auto strut = lookupConcrete<ast::StructType>(ctx, expected)) {
        if (list.exprs.size() > strut->fields.size()) {
          ctx.log.error(list.loc) << "Too many expressions in initializer list" << fatal;
        } else if (list.exprs.size() < strut->fields.size()) {
          ctx.log.error(list.loc) << "Too few expressions in initializer list" << fatal;
        }
        for (size_t i = 0; i != list.exprs.size(); ++i) {
          const ast::ExprPtr &expr = list.exprs[i];
          const ast::Field &field = strut->fields[i];
          visitExprCheck(expr, field.type);
        }
      } else {
        ctx.log.error(list.loc) << "Initializer list can only initialize structs" << fatal;
      }
    }
    lkp.setExpr(sym::makeLetVal(std::move(expected)));
  }
  void visit(ast::Lambda &lam) override {
    sym::Lambda *const lamSym = insert(ctx, lam);
    lamSym->scope = ctx.man.enterScope(sym::ScopeType::closure, lamSym);
    enterLambdaScope(lamSym, lam);
    traverse(ctx, lam.body);
    leaveLambdaScope(ctx, lamSym, lam);
    ctx.man.leaveScope();
    lam.exprType = getLambdaType(ctx, lam);
    lkp.setExpr(sym::makeLetVal(lam.exprType));
  }

  sym::ExprType visitExprNoCheck(const ast::ExprPtr &expr, const ast::TypePtr &type) {
    expr->expectedType = type;
    lkp.enterSubExpr();
    ast::TypePtr oldExpected = std::move(expected);
    expected = type;
    expr->accept(*this);
    expected = std::move(oldExpected);
    sym::ExprType etype = lkp.leaveSubExpr();
    expr->exprType = etype.type;
    return etype;
  }

  bool convertibleToBool(const ast::TypePtr &type) {
    return dynamic_cast<ast::FuncType *>(type.get());
  }

  sym::ExprType visitExprNoCheckBool(const ast::ExprPtr &expr, const ast::TypePtr &type) {
    sym::ExprType etype = visitExprNoCheck(expr, type);
    if (type && !compareTypes(ctx, type, etype.type)) {
      if (compareTypes(ctx, type, ctx.btn.Bool) && convertibleToBool(etype.type)) {
        return sym::makeLetVal(ctx.btn.Bool);
      }
    }
    return etype;
  }

  sym::ExprType visitExprCheck(const ast::ExprPtr &expr, const ast::TypePtr &type = nullptr) {
    sym::ExprType etype = visitExprNoCheck(expr, type);
    if (type && !compareTypes(ctx, type, etype.type)) {
      if (compareTypes(ctx, type, ctx.btn.Bool) && convertibleToBool(etype.type)) {
        return sym::makeLetVal(ctx.btn.Bool);
      } else {
        ctx.log.error(expr->loc) << "Expected " << typeDesc(type) << " but got "
          << typeDesc(etype.type) << fatal;
      }
    }
    return etype;
  }

private:
  ExprLookup lkp;
  sym::Ctx ctx;
  ast::TypePtr expected;
};

}

sym::ExprType stela::getExprType(sym::Ctx ctx, const ast::ExprPtr &expr, const ast::TypePtr &type) {
  return Visitor{ctx}.visitExprCheck(expr, type);
}
