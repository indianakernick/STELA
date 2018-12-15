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
  
  void visit(ast::Func &func) override {
    std::vector<llvm::AttrBuilder> paramAttrs;
    llvm::FunctionType *fnType = generateFuncSig(ctx, func);
    std::vector<llvm::Type *> realParams;
    llvm::Type *realRet;
    
    for (llvm::Type *param : fnType->params()) {
      if (param->isStructTy()) {
        realParams.push_back(param->getPointerTo());
        paramAttrs.push_back({});
        paramAttrs.back().addAttribute(llvm::Attribute::ReadOnly);
        paramAttrs.back().addAttribute(llvm::Attribute::NonNull);
      } else if (param->isPointerTy()) {
        realParams.push_back(param);
        paramAttrs.push_back({});
        paramAttrs.back().addAttribute(llvm::Attribute::NonNull);
      } else {
        realParams.push_back(param);
        paramAttrs.push_back({});
      }
    }
    
    llvm::Type *ret = fnType->getReturnType();
    if (ret->isStructTy()) {
      realParams.push_back(ret->getPointerTo());
      realRet = llvm::Type::getVoidTy(ctx.llvm);
      paramAttrs.push_back({});
      paramAttrs.back().addAttribute(llvm::Attribute::NoAlias);
      paramAttrs.back().addAttribute(llvm::Attribute::WriteOnly);
      paramAttrs.back().addAttribute(llvm::Attribute::NonNull);
    } else {
      realRet = ret;
    }
    
    func.llvmFunc = llvm::Function::Create(
      llvm::FunctionType::get(realRet, realParams, false),
      llvm::Function::ExternalLinkage,
      llvm::StringRef{func.name.data(), func.name.size()},
      module
    );
    
    func.llvmFunc->addFnAttr(llvm::Attribute::NoUnwind);
    for (unsigned p = 0; p != paramAttrs.size(); ++p) {
      func.llvmFunc->addParamAttrs(p, paramAttrs[p]);
    }
    
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
