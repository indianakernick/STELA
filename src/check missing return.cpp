//
//  check missing return.cpp
//  STELA
//
//  Created by Indi Kernick on 12/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "check missing return.hpp"

#include "scope lookup.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  explicit Visitor(sym::Ctx ctx)
    : ctx{ctx} {}

  void visit(ast::Block &block) override {
    occurs = Occurs::no;
    bool maybeLeave = false;
    Term blockTerm = {};
    for (auto s = block.nodes.cbegin(); s != block.nodes.cend(); ++s) {
      ast::Statement *stat = s->get();
      stat->accept(*this);
      if (occurs == Occurs::yes && std::next(s) != block.nodes.cend()) {
        ast::Statement *nextStat = std::next(s)->get();
        ctx.log.error(nextStat->loc) << "Unreachable code" << fatal;
      }
      if (occurs == Occurs::maybe && term != Term::returns) {
        maybeLeave = true;
      }
      blockTerm = maxTerm(blockTerm, term);
    }
    term = blockTerm;
    if (maybeLeave) {
      occurs = Occurs::maybe;
    }
  }
  void visit(ast::If &fi) override {
    fi.body->accept(*this);
    const Term truTerm = term;
    const Occurs truOccurs = occurs;
    if (fi.elseBody) {
      fi.elseBody->accept(*this);
    } else {
      occurs = Occurs::no;
    }
    if (occurs != truOccurs || term != truTerm) {
      occurs = Occurs::maybe;
    }
    if (truTerm == Term::returns) {
      term = Term::returns;
    }
  }
  void visit(ast::Switch &swich) override {
    bool foundDefault = false;
    size_t retCount = 0;
    size_t totalCases = swich.cases.size();
    bool maybeRet = false;
    for (auto c = swich.cases.cbegin(); c != swich.cases.cend(); ++c) {
      if (!c->expr) {
        foundDefault = true;
      }
      c->body->accept(*this);
      if (term == Term::returns) {
        if (occurs == Occurs::maybe) {
          maybeRet = true;
        } else if (occurs == Occurs::yes) {
          ++retCount;
        }
      } else if (term == Term::continues) {
        if (occurs == Occurs::yes) {
          --totalCases;
        }
        if (occurs != Occurs::no && std::next(c) == swich.cases.cend()) {
          ctx.log.error(c->loc) << "Case may continue but this case does not precede another" << fatal;
        }
      }
    }
    
    term = Term::returns;
    if (retCount == totalCases && foundDefault) {
      occurs = Occurs::yes;
    } else if (maybeRet || retCount != 0) {
      occurs = Occurs::maybe;
    } else {
      occurs = Occurs::no;
    }
  }
  void visit(ast::Break &) override {
    term = Term::breaks;
    occurs = Occurs::yes;
  }
  void visit(ast::Continue &) override {
    term = Term::continues;
    occurs = Occurs::yes;
  }
  void visit(ast::Return &) override {
    term = Term::returns;
    occurs = Occurs::yes;
  }
  void visit(ast::While &wile) override {
    wile.body->accept(*this);
    afterLoop();
  }
  void visit(ast::For &four) override {
    four.body->accept(*this);
    afterLoop();
  }
  void afterLoop() {
    if (term == Term::returns) {
      if (occurs != Occurs::no) {
        occurs = Occurs::maybe;
      }
    } else {
      term = Term::returns;
      occurs = Occurs::no;
    }
  }

  bool noReturn() const {
    return occurs == Occurs::no;
  }
  bool maybeReturn() const {
    return occurs == Occurs::maybe;
  }

private:
  sym::Ctx ctx;
  
  enum class Term {
    continues,
    breaks,
    returns,
  };
  enum class Occurs {
    no,
    maybe,
    yes
  };
  Term term {};
  Occurs occurs {};
  
  Term maxTerm(const Term a, const Term b) {
    return static_cast<Term>(std::max(
      static_cast<int>(a), static_cast<int>(b)
    ));
  }
  Occurs maxOccurs(const Occurs a, const Occurs b) {
    return static_cast<Occurs>(std::max(
      static_cast<int>(a), static_cast<int>(b)
    ));
  }
};

}

void stela::checkMissingRet(sym::Ctx ctx, ast::Block &body, const ast::TypePtr &ret, const Loc loc) {
  Visitor visitor{ctx};
  body.accept(visitor);
  bool retVoid = false;
  if (auto *btnType = lookupConcrete<ast::BtnType>(ctx, ret).get()) {
    retVoid = btnType->value == ast::BtnTypeEnum::Void;
  }
  if (retVoid && (visitor.noReturn() || visitor.maybeReturn())) {
    body.nodes.push_back(make_retain<ast::Return>());
    return;
  }
  if (visitor.noReturn()) {
    ctx.log.error(loc) << "Non-void function does not return" << fatal;
  }
  if (visitor.maybeReturn()) {
    ctx.log.error(loc) << "Non-void function may not return" << fatal;
  }
}
