//
//  generate ids.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate ids.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  
  void visit(ast::Block &block) override {
    for (const ast::StatPtr &stat : block.nodes) {
      stat->accept(*this);
    }
  }
  void visit(ast::If &fi) override {
    fi.body->accept(*this);
    if (fi.elseBody) {
      fi.elseBody->accept(*this);
    }
  }
  void visit(ast::Switch &swich) override {
    for (const ast::SwitchCase &cse : swich.cases) {
      cse.body->accept(*this);
    }
  }
  void visit(ast::While &wile) override {
    wile.body->accept(*this);
  }
  void visit(ast::For &four) override {
    if (four.init) {
      four.init->accept(*this);
    }
    if (four.incr) {
      four.incr->accept(*this);
    }
    four.body->accept(*this);
  }
  
  void visit(ast::Func &func) override {
    func.id = funID++;
    func.body.accept(*this);
  }
  void visit(ast::Var &var) override {
    var.id = varID++;
  }
  void visit(ast::Let &let) override {
    let.id = varID++;
  }
  void visit(ast::DeclAssign &assign) override {
    assign.id = varID++;
  }
  
private:
  uint32_t varID = 0;
  uint32_t funID = 0;
};

}

void stela::generateIDs(const ast::Decls &decls) {
  Visitor visitor;
  for (const ast::DeclPtr &decl : decls) {
    decl->accept(visitor);
  }
}
