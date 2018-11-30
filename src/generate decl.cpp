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

using namespace stela;

namespace {

class Visitor final : public ast::Visitor {
public:
  Visitor(std::string &bin, Log &log)
    : bin{bin}, log{log} {}
  
  std::string exprType(const sym::ExprType &etype) {
    if (etype.type == nullptr) {
      return "void *";
    } else {
      std::string name;
      if (etype.mut == sym::ValueMut::let) {
        name.append("const ");
      }
      name.append(generateType(bin, log, etype.type));
      name.push_back(' ');
      if (etype.ref == sym::ValueRef::ref) {
        name.push_back('&');
      }
      return name;
    }
  }
  
  void visit(ast::Func &func) override {
    sym::Func *symbol = func.symbol;
    std::string ret = exprType(symbol->ret);
    std::vector<std::string> args;
    args.reserve(symbol->params.size());
    for (const sym::ExprType &param : symbol->params) {
      args.push_back(exprType(param));
    }
    bin.append(ret);
    bin.append("f_");
    bin.append(func.name);
    bin.push_back('(');
    bin.append(args.front());
    if (func.receiver) {
      bin.append("v_");
      bin.append(func.receiver->name);
    }
    for (size_t p = 0; p != func.params.size(); ++p) {
      bin.append(", ");
      bin.append(args[p + 1]);
      bin.append("v_");
      bin.append(func.params[p].name);
    }
    bin.append(") noexcept {\n");
    func.body.accept(*this);
    bin.append("}\n");
  }
  
private:
  std::string &bin;
  Log &log;
};

}

void stela::generateDecl(std::string &bin, Log &log, const ast::Decls &decls) {
  Visitor visitor{bin, log};
  for (const ast::DeclPtr &decl : decls) {
    decl->accept(visitor);
  }
}
