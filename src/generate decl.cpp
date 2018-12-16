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
#include "generate expr.hpp"
#include <llvm/IR/Function.h>
#include "generate zero expr.hpp"

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(gen::Ctx ctx, llvm::Module *module)
    : ctx{ctx}, module{module} {}
  
  llvm::GlobalObject::LinkageTypes linkage(const bool ext) {
    return ext ? llvm::GlobalObject::ExternalLinkage : llvm::GlobalObject::InternalLinkage;
  }
  
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
      linkage(func.external),
      llvm::StringRef{func.name.data(), func.name.size()},
      module
    );
    
    func.llvmFunc->addFnAttr(llvm::Attribute::NoUnwind);
    for (unsigned p = 0; p != paramAttrs.size(); ++p) {
      func.llvmFunc->addParamAttrs(p, paramAttrs[p]);
    }
    
    generateStat(ctx, func.llvmFunc, func.receiver, func.params, func.body);
  }
  
  llvm::Value *createGlobalVar(
    ast::Type *type,
    const std::string_view name,
    ast::Expression *expr,
    const bool external
  ) {
    llvm::Type *llvmType = generateType(ctx, type);
    llvm::Value *llvmAddr = new llvm::GlobalVariable{
      *module,
      llvmType,
      false,
      linkage(external),
      llvm::UndefValue::get(llvmType),
      llvm::StringRef{name.data(), name.size()}
    };
    
    llvm::FunctionType *ctorType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(ctx.llvm), false
    );
    llvm::Function *ctor = llvm::Function::Create(
      ctorType,
      llvm::Function::PrivateLinkage,
      "",
      module
    );
    ctor->addFnAttr(llvm::Attribute::NoUnwind);
    ctor->addFnAttr(llvm::Attribute::NoRecurse);
    ctor->addFnAttr(llvm::Attribute::ReadNone);
    
    FuncBuilder builder(ctor);
    llvm::Value *init;
    if (expr) {
      init = generateValueExpr(ctx, builder, expr);
    } else {
      init = generateZeroExpr(ctx, builder, type);
    }
    builder.ir.CreateStore(init, llvmAddr);
    builder.ir.CreateRetVoid();
    
    llvm::Twine fnName = llvm::StringRef{name.data(), name.size()};
    ctor->setName(fnName + "_ctor");
    ctors.push_back(ctor);
    
    return llvmAddr;
  }
  void visit(ast::Var &var) override {
    var.llvmAddr = createGlobalVar(
      var.symbol->etype.type.get(), var.name, var.expr.get(), var.external
    );
  }
  void visit(ast::Let &let) override {
    let.llvmAddr = createGlobalVar(
      let.symbol->etype.type.get(), let.name, let.expr.get(), let.external
    );
  }
  
  void writeCtorList() {
    if (ctors.empty()) {
      return;
    }
    llvm::FunctionType *ctorType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(ctx.llvm), false
    );
    llvm::StructType *ctorEntryType = llvm::StructType::get(
      llvm::IntegerType::getInt32Ty(ctx.llvm),
      ctorType->getPointerTo(),
      llvm::IntegerType::getInt8PtrTy(ctx.llvm)
    );
    llvm::ArrayType *ctorListType = llvm::ArrayType::get(ctorEntryType, 1);
    
    std::vector<llvm::Constant *> entries;
    entries.reserve(ctors.size());
    for (size_t c = 0; c != ctors.size(); ++c) {
      entries.push_back(llvm::ConstantStruct::get(ctorEntryType,
        llvm::ConstantInt::get(llvm::IntegerType::getInt32Ty(ctx.llvm), c),
        ctors[c],
        llvm::ConstantPointerNull::get(llvm::IntegerType::getInt8PtrTy(ctx.llvm))
      ));
    }
    
    new llvm::GlobalVariable{
      *module,
      ctorListType,
      true,
      llvm::Function::AppendingLinkage,
      llvm::ConstantArray::get(ctorListType, entries),
      "llvm.global_ctors"
    };
  }
  
private:
  gen::Ctx ctx;
  llvm::Module *module;
  std::vector<llvm::Function *> ctors;
};

}

void stela::generateDecl(gen::Ctx ctx, llvm::Module *module, const ast::Decls &decls) {
  Visitor visitor{ctx, module};
  for (const ast::DeclPtr &decl : decls) {
    decl->accept(visitor);
  }
  visitor.writeCtorList();
}
