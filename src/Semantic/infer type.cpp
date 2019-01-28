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
#include "builtin symbols.hpp"
#include "Lex/number literal.hpp"
#include "check missing return.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  explicit Visitor(sym::Ctx ctx)
    : lkp{ctx}, ctx{ctx} {}

  void visit(ast::BinaryExpr &bin) override {
    ast::TypePtr expType = boolOp(bin.oper) ? ctx.btn.Bool : nullptr;
    const sym::ExprType lhs = visitExprCheck(bin.lhs, expType);
    const sym::ExprType rhs = visitExprCheck(bin.rhs, expType);
    if (!compareTypes(ctx, lhs.type, rhs.type)) {
      ctx.log.error(bin.loc) << "Operands to binary expression " << opName(bin.oper)
        << " must have same type" << fatal;
    }
    if (compOp(bin.oper)) {
      validComp(ctx, bin.oper, lhs.type, bin.loc);
      bin.exprType = ctx.btn.Bool;
      lkp.setExpr(sym::makeLetVal(ctx.btn.Bool));
      return;
    }
    if (auto builtinLeft = lookupConcrete<ast::BtnType>(ctx, lhs.type)) {
      if (auto builtinRight = lookupConcrete<ast::BtnType>(ctx, rhs.type)) {
        if (auto retType = validOp(bin.oper, builtinLeft, builtinRight)) {
          bin.exprType = retType;
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
        un.exprType = etype.type;
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
    call.exprType = lkp.topType();
  }
  void visit(ast::MemberIdent &mem) override {
    lkp.member(sym::Name{mem.member});
    mem.object->accept(*this);
    lkp.lookupMember(mem);
  }
  void visit(ast::Subscript &sub) override {
    const sym::ExprType obj = visitExprCheck(sub.object);
    const sym::ExprType idx = visitExprCheck(sub.index);
    if (auto builtinIdx = lookupConcrete<ast::BtnType>(ctx, idx.type)) {
      if (validSubscript(builtinIdx)) {
        if (auto array = lookupConcrete<ast::ArrayType>(ctx, obj.type)) {
          sub.exprType = lookupStrongType(ctx, array->elem);
          lkp.setExpr(sym::fieldType(obj, sub.exprType));
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
    const sym::ExprType troo = visitExprCheck(tern.troo, expected);
    const sym::ExprType fols = visitExprCheck(tern.fols, expected);
    if (!compareTypes(ctx, troo.type, fols.type)) {
      ctx.log.error(tern.loc) << "Branches of ternary have different types" << fatal;
    }
    sym::ExprType etype;
    etype.type = troo.type;
    etype.mut = sym::common(troo.mut, fols.mut);
    etype.ref = sym::common(troo.ref, fols.ref);
    tern.exprType = etype.type;
    lkp.setExpr(etype);
  }
  void visit(ast::Make &make) override {
    validateType(ctx, make.type);
    const sym::ExprType etype = visitExprNoCheckBool(make.expr, make.type);
    make.exprType = make.type;
    if (auto dst = lookupConcrete<ast::BtnType>(ctx, make.type)) {
      if (auto src = lookupConcrete<ast::BtnType>(ctx, etype.type)) {
        if (validCast(dst, src)) {
          make.cast = true;
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
  
  void visit(ast::StringLiteral &str) override {
    if (str.value.empty() && !str.literal.empty()) {
      str.value = parseStringLiteral(str.literal, str.loc, ctx.log);
    }
    str.exprType = ctx.btn.string;
    lkp.setExpr(sym::makeLetVal(ctx.btn.string));
  }
  void visit(ast::CharLiteral &chr) override {
    std::string str;
    if (chr.value == -1) {
      str = parseStringLiteral(chr.literal, chr.loc, ctx.log);
    } else {
      str = chr.value;
    }
    if (str.size() != 1) {
      ctx.log.error(chr.loc) << "Character literal must be one character" << fatal;
    }
    chr.value = str[0];
    chr.exprType = ctx.btn.Char;
    lkp.setExpr(sym::makeLetVal(ctx.btn.Char));
  }
  void visit(ast::NumberLiteral &n) override {
    if (std::holds_alternative<std::monostate>(n.value)) {
      n.value = parseNumberLiteral(n.literal, ctx.log);
    }
    ast::TypePtr type;
    if (std::holds_alternative<Byte>(n.value)) {
      type = ctx.btn.Byte;
    } else if (std::holds_alternative<Char>(n.value)) {
      type = ctx.btn.Char;
    } else if (std::holds_alternative<Real>(n.value)) {
      type = ctx.btn.Real;
    } else if (std::holds_alternative<Sint>(n.value)) {
      type = ctx.btn.Sint;
    } else if (std::holds_alternative<Uint>(n.value)) {
      type = ctx.btn.Uint;
    }
    n.exprType = type;
    lkp.setExpr(sym::makeLetVal(std::move(type)));
  }
  void visit(ast::BoolLiteral &bol) override {
    bol.exprType = ctx.btn.Bool;
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
    arr.exprType = array;
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
    list.exprType = expected;
    lkp.setExpr(sym::makeLetVal(std::move(expected)));
  }
  void visit(ast::Lambda &lam) override {
    if (lam.ret) {
      validateType(ctx, lam.ret);
    }
    for (const ast::FuncParam &param : lam.params) {
      validateType(ctx, param.type);
    }
    sym::Lambda *const lamSym = insert(ctx, lam);
    lamSym->scope = ctx.man.enterScope(sym::ScopeType::closure, lamSym);
    enterLambdaScope(lamSym, lam);
    traverse(ctx, lam.body);
    leaveLambdaScope(ctx, lamSym, lam);
    ctx.man.leaveScope();
    lam.exprType = getLambdaType(lam);
    lkp.setExpr(sym::makeLetVal(lam.exprType));
    checkMissingRet(ctx, lam.body, lam.ret, lam.loc);
  }

  sym::ExprType visitExprNoCheck(const ast::ExprPtr &expr, const ast::TypePtr &type) {
    expr->expectedType = type;
    lkp.enterSubExpr();
    ast::TypePtr oldExpected = std::move(expected);
    expected = type;
    expr->accept(*this);
    expected = std::move(oldExpected);
    return lkp.leaveSubExpr();
  }

  bool convertibleToBool(const ast::TypePtr &type) {
    ast::TypePtr concrete = lookupConcreteType(ctx, type);
    if (dynamic_cast<ast::FuncType *>(concrete.get())) {
      return true;
    } else if (auto *usr = dynamic_cast<ast::UserType *>(concrete.get())) {
      return usr->boolConv.addr != ast::UserCtor::none;
    } else {
      return false;
    }
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
