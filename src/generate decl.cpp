//
//  generate decl.cpp
//  STELA
//
//  Created by Indi Kernick on 30/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "generate decl.hpp"

#include "symbols.hpp"
#include "generate type.hpp"
#include "generate stat.hpp"
#include <llvm/IR/Function.h>
#include "generate zero expr.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, llvm::Module *module)
    : ctx{ctx}, module{module} {}
  
  /*llvm::Type *exprType(const sym::ExprType &etype) {
    if (etype.type == nullptr) {
      return getVoidPtr(ctx);
    } else {
      llvm::Type *type = generateType(ctx, etype.type.get());
      if (etype.ref == sym::ValueRef::ref) {
        return type->getPointerTo();
      }
      return type;
    }
  }

  void visit(ast::Switch &swich) override {
    str += "{\n";
    str += "const auto v_swh_";
    str += swich.id;
    str += " = ";
    str += generateExpr(ctx, swich.expr.get());
    str += ";\n";
    
    const ast::SwitchCase *defaultCase = nullptr;
    for (const ast::SwitchCase &cse : swich.cases) {
      if (!cse.expr) {
        defaultCase = &cse;
        continue;
      }
      str += "if (v_swh_";
      str += swich.id;
      str += " == (";
      str += generateExpr(ctx, cse.expr.get());
      str += ")) goto CASE_LABEL_";
      str += cse.id;
      str += ";\n";
    }
    if (defaultCase) {
      str += "goto CASE_LABEL_";
      str += defaultCase->id;
      str += ";\n";
    }
    
    for (ast::SwitchCase &cse : swich.cases) {
      str += "CASE_LABEL_";
      str += cse.id;
      str += ": ;\n";
      visitFlow(cse, cse.body);
      str += "goto BREAK_LABEL_";
      str += swich.id;
      str += ";\n";
      str += "CONTINUE_LABEL_";
      str += cse.id;
      str += ": ;\n";
    }
    
    str += "}\n";
    str += "BREAK_LABEL_";
    str += swich.id;
    str += ": ;\n";
  }
  */
  void visit(ast::Func &func) override {
    func.llvmFunc = llvm::Function::Create(
      generateFuncSig(ctx, func),
      llvm::Function::ExternalLinkage,
      llvm::StringRef{func.name.data(), func.name.size()},
      module
    );
    func.llvmFunc->setCallingConv(llvm::CallingConv::C);
    generateStat(ctx, func.llvmFunc, func.receiver, func.params, func.body);
  }
  void appendVar(const sym::ExprType &etype, const uint32_t id, ast::Expression *expr) {
    /*str += exprType(etype);
    str += "v_";
    str += id;
    str += " = ";
    if (expr) {
      str += generateExpr(ctx, expr);
    } else {
      str += generateZeroExpr(ctx, etype.type.get());
    }
    str += ";\n";*/
  }
  void visit(ast::Var &var) override {
    //appendVar(var.symbol->etype, var.id, var.expr.get());
  }
  void visit(ast::Let &let) override {
    //appendVar(let.symbol->etype, let.id, let.expr.get());
  }
  
  /*
  void visit(ast::CompAssign &assign) override {
    str += generateExpr(ctx, assign.left.get());
    str += ' ';
    str += opName(assign.oper);
    str += ' ';
    str += generateExpr(ctx, assign.right.get());
    str += ";\n";
  }
  */
  
private:
  gen::Ctx ctx;
  llvm::Module *module;
};

}

void stela::generateDecl(gen::Ctx ctx, llvm::Module *module, const ast::Decls &decls) {
  Visitor visitor{ctx, module};
  for (const ast::DeclPtr &decl : decls) {
    decl->accept(visitor);
  }
}
