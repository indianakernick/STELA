//
//  lower expressions.cpp
//  STELA
//
//  Created by Indi Kernick on 14/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "lower expressions.hpp"

#include "unreachable.hpp"
#include "generate type.hpp"
#include "assert down cast.hpp"

using namespace stela;

namespace {

// @TODO lower some other stuff

class Visitor final : public ast::Visitor {
public:
  void visit(ast::Block &block) override {
    for (ast::StatPtr &stat : block.nodes) {
      visitPtr(stat);
    }
  }
  void visit(ast::If &fi) override {
    visitPtr(fi.body);
    if (fi.elseBody) {
      visitPtr(fi.elseBody);
    }
  }
  void visit(ast::Switch &swich) override {
    for (ast::SwitchCase &cse : swich.cases) {
      visitPtr(cse.body);
    }
  }
  void visit(ast::While &wile) override {
    visitPtr(wile.body);
  }
  void visit(ast::For &four) override {
    if (four.init) {
      visitPtr(four.init);
    }
    visitPtr(four.body);
    if (four.incr) {
      visitPtr(four.incr);
    }
  }
  
  static ast::BinOp convert(const ast::AssignOp op) {
    #define CASE(OP) case ast::AssignOp::OP: return ast::BinOp::OP
    
    switch (op) {
      CASE(add);
      CASE(sub);
      CASE(mul);
      CASE(div);
      CASE(mod);
      CASE(bit_or);
      CASE(bit_xor);
      CASE(bit_and);
      CASE(bit_shl);
      CASE(bit_shr);
    }
    
    #undef CASE
    
    UNREACHABLE();
  }
  
  void visit(ast::CompAssign &compAssign) override {
    auto binExpr = make_retain<ast::BinaryExpr>();
    binExpr->lhs = compAssign.dst;
    binExpr->rhs = compAssign.src;
    binExpr->oper = convert(compAssign.oper);
    auto assign = make_retain<ast::Assign>();
    assign->dst = compAssign.dst;
    assign->src = std::move(binExpr);
    changePtr(assign);
  }
  
  static NumberVariant getOne(const ast::BtnTypeEnum type) {
    switch (type) {
      // Arithmetic types
      case ast::BtnTypeEnum::Char:
        return Char{1};
      case ast::BtnTypeEnum::Real:
        return Real{1};
      case ast::BtnTypeEnum::Sint:
        return Sint{1};
      case ast::BtnTypeEnum::Uint:
        return Uint{1};
      default: UNREACHABLE();
    }
  }
  
  void visit(ast::IncrDecr &incrDecr) override {
    auto *type = assertConcreteType<ast::BtnType>(incrDecr.expr->exprType.get());
    auto one = make_retain<ast::NumberLiteral>();
    one->value = getOne(type->value);
    one->exprType = incrDecr.expr->exprType;
    auto binExpr = make_retain<ast::BinaryExpr>();
    binExpr->lhs = incrDecr.expr;
    binExpr->rhs = std::move(one);
    if (incrDecr.incr) {
      binExpr->oper = ast::BinOp::add;
    } else {
      binExpr->oper = ast::BinOp::sub;
    }
    auto assign = make_retain<ast::Assign>();
    assign->dst = incrDecr.expr;
    assign->src = std::move(binExpr);
    changePtr(assign);
  }

private:
  ast::StatPtr *statPtr = nullptr;
  ast::AsgnPtr *asgnPtr = nullptr;
  
  void visitPtr(ast::StatPtr &stat) {
    statPtr = &stat;
    asgnPtr = nullptr;
    stat->accept(*this);
  }
  void visitPtr(ast::AsgnPtr &asgn) {
    statPtr = nullptr;
    asgnPtr = &asgn;
    asgn->accept(*this);
  }
  void changePtr(ast::AsgnPtr asgn) {
    if (statPtr) {
      *statPtr = asgn;
    } else if (asgnPtr) {
      *asgnPtr = asgn;
    } else {
      UNREACHABLE();
    }
  }
};

}

void stela::lowerExpressions(ast::Block &block) {
  Visitor visitor{};
  visitor.visit(block);
}
