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
#include "lifetime exprs.hpp"

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
    llvm::FunctionType *fnType = generateFuncSig(ctx, func);
    func.llvmFunc = llvm::Function::Create(
      fnType,
      linkage(func.external),
      llvm::StringRef{func.name.data(), func.name.size()},
      module
    );
    assignAttributes(func.llvmFunc, func.symbol->params);
    
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
      llvm::Function::InternalLinkage,
      "",
      module
    );
    ctor->addFnAttr(llvm::Attribute::NoUnwind);
    ctor->addFnAttr(llvm::Attribute::NoRecurse);
    ctor->addFnAttr(llvm::Attribute::ReadNone);
    
    FuncBuilder builder{ctor};
    if (expr) {
      builder.ir.CreateStore(generateValueExpr(ctx, builder, expr), llvmAddr);
    } else {
      LifetimeExpr lifetime{ctx, builder.ir};
      lifetime.defConstruct(type, llvmAddr);
    }
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
