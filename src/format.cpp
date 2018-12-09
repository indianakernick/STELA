//
//  format.cpp
//  STELA
//
//  Created by Indi Kernick on 14/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "format.hpp"

#include "operator name.hpp"
#include "iterator range.hpp"
#include "syntax analysis.hpp"

using namespace stela;
using namespace stela::fmt;

namespace {

class Visitor final : public stela::ast::Visitor {
public:
  void visit(ast::BtnType &type) override {
    push(Tag::type_name, typeName(type.value));
  }
  void visit(ast::ArrayType &type) override {
    pushOp("[");
    type.elem->accept(*this);
    pushOp("]");
  }
  void visit(ast::FuncType &type) override {
    pushKey("func");
    pushOp("(");
    for (auto p : citer_range(type.params)) {
      if (p->ref == ast::ParamRef::ref) {
        pushKey("ref");
      }
      p->type->accept(*this);
      if (p != type.params.cend() - 1) {
        pushOp(",");
        pushSpace();
      }
    }
    pushOp(")");
    if (type.ret) {
      pushSpace();
      pushOp("->");
      pushSpace();
      type.ret->accept(*this);
    }
  }
  void visit(ast::NamedType &type) override {
    push(Tag::type_name, type.name);
  }
  void visit(ast::StructType &strt) override {
    pushKey("struct");
    pushOp("{");
    if (strt.fields.empty()) {
      pushOp("}");
      return;
    }
    pushNewline();
    ++indent;
    for (const ast::Field &field : strt.fields) {
      pushIndent();
      push(Tag::plain, field.name);
      pushOp(":");
      pushSpace();
      field.type->accept(*this);
      pushOp(";");
      pushNewline();
    }
    --indent;
    pushIndent();
    pushOp("}");
  }
  
  void visit(ast::BinaryExpr &bin) override {
    bin.left->accept(*this);
    pushSpace();
    pushOp(opName(bin.oper));
    pushSpace();
    bin.right->accept(*this);
  }
  void visit(ast::UnaryExpr &un) override {
    pushOp(opName(un.oper));
    un.expr->accept(*this);
  }
  void visit(ast::FuncCall &call) override {
    call.func->accept(*this);
    pushOp("(");
    pushExprs(call.args);
    pushOp(")");
  }
  void visit(ast::MemberIdent &mem) override {
    mem.object->accept(*this);
    pushOp(".");
    push(Tag::member, mem.member);
  }
  void visit(ast::Subscript &sub) override {
    sub.object->accept(*this);
    pushOp("[");
    sub.index->accept(*this);
    pushOp("]");
  }
  void visit(ast::Identifier &id) override {
    push(Tag::plain, id.name);
  }
  void visit(ast::Ternary &tern) override {
    tern.cond->accept(*this);
    pushSpace();
    pushOp("?");
    pushSpace();
    tern.tru->accept(*this);
    pushSpace();
    pushOp(":");
    pushSpace();
    tern.fals->accept(*this);
  }
  void visit(ast::Make &make) override {
    pushKey("make");
    make.type->accept(*this);
    pushSpace();
    pushOp("(");
    make.expr->accept(*this);
    pushOp(")");
  }
  
  void visit(ast::Block &block) override {
    pushOp("{");
    if (block.nodes.empty()) {
      pushOp("}");
      return;
    }
    pushNewline();
    ++indent;
    pushBlockBody(block);
    --indent;
    pushIndent();
    pushOp("}");
  }
  void visit(ast::If &fi) override {
    pushKey("if");
    pushOp("(");
    fi.cond->accept(*this);
    pushOp(")");
    pushSpace();
    fi.body->accept(*this);
    if (fi.elseBody) {
      pushSpace();
      pushKey("else");
      fi.elseBody->accept(*this);
    }
  }
  void visit(ast::Switch &swtch) override {
    pushKey("switch");
    pushOp("(");
    swtch.expr->accept(*this);
    pushOp(")");
    pushSpace();
    pushOp("{");
    if (swtch.cases.empty()) {
      pushOp("}");
      return;
    }
    pushNewline();
    ++indent;
    for (ast::SwitchCase &cs : swtch.cases) {
      pushIndent();
      if (cs.expr) {
        pushKey("case");
        pushOp("(");
        cs.expr->accept(*this);
        pushOp(")");
        pushSpace();
      } else {
        pushKey("default");
      }
      cs.body->accept(*this);
      pushNewline();
    }
    --indent;
    pushIndent();
    pushOp("}");
  }
  void visit(ast::Break &) override {
    push(Tag::keyword, "break");
    pushOp(";");
  }
  void visit(ast::Continue &) override {
    push(Tag::keyword, "continue");
    pushOp(";");
  }
  void visit(ast::Return &ret) override {
    push(Tag::keyword, "return");
    if (ret.expr) {
      pushSpace();
      ret.expr->accept(*this);
    }
    pushOp(";");
  }
  void visit(ast::While &whl) override {
    pushKey("while");
    pushOp("(");
    whl.cond->accept(*this);
    pushOp(")");
    pushSpace();
    whl.body->accept(*this);
  }
  void visit(ast::For &fr) override {
    pushKey("for");
    pushOp("(");
    if (fr.init) {
      fr.init->accept(*this);
    } else {
      pushOp(";");
    }
    pushSpace();
    fr.cond->accept(*this);
    pushOp(";");
    pushSpace();
    if (fr.incr) {
      fr.incr->accept(*this);
      assert(!tokens.empty());
      assert(tokens.back().tag == Tag::oper);
      assert(tokens.back().text == ";");
      tokens.pop_back();
    }
    pushOp(")");
    pushSpace();
    fr.body->accept(*this);
  }
  
  void visit(ast::Func &func) override {
    pushKey("func");
    if (func.receiver) {
      pushOp("(");
      pushParam(*func.receiver);
      pushOp(")");
      pushSpace();
    }
    push(Tag::plain, func.name);
    pushParams(func.params);
    pushSpace();
    if (func.ret) {
      pushOp("->");
      pushSpace();
      func.ret->accept(*this);
      pushSpace();
    }
    func.body.accept(*this);
  }
  void visit(ast::Var &var) override {
    pushKeyName("var", var.name);
    if (var.type) {
      pushOp(":");
      pushSpace();
      var.type->accept(*this);
    }
    if (var.expr) {
      pushSpace();
      pushOp("=");
      pushSpace();
      var.expr->accept(*this);
    }
    pushOp(";");
  }
  void visit(ast::Let &let) override {
    pushKeyName("let", let.name);
    if (let.type) {
      pushOp(":");
      pushSpace();
      let.type->accept(*this);
    }
    pushSpace();
    pushOp("=");
    pushSpace();
    let.expr->accept(*this);
    pushOp(";");
  }
  void visit(ast::TypeAlias &alias) override {
    pushKeyName("type", alias.name);
    pushSpace();
    if (!alias.strong) {
      pushOp("=");
      pushSpace();
    }
    alias.type->accept(*this);
    pushOp(";");
  }
  
  void visit(ast::CompAssign &as) override {
    as.left->accept(*this);
    pushSpace();
    pushOp(opName(as.oper));
    pushSpace();
    as.right->accept(*this);
    pushOp(";");
  }
  void visit(ast::IncrDecr &incrDecr) override {
    incrDecr.expr->accept(*this);
    if (incrDecr.incr) {
      pushOp("++");
    } else {
      pushOp("--");
    }
    pushOp(";");
  }
  void visit(ast::Assign &as) override {
    as.left->accept(*this);
    pushSpace();
    pushOp("=");
    pushSpace();
    as.right->accept(*this);
    pushOp(";");
  }
  void visit(ast::DeclAssign &as) override {
    push(Tag::plain, as.name);
    pushSpace();
    pushOp(":=");
    pushSpace();
    as.expr->accept(*this);
    pushOp(";");
  }
  void visit(ast::CallAssign &as) override {
    as.call.accept(*this);
    pushOp(";");
  }
  
  void visit(ast::StringLiteral &str) override {
    push(Tag::string, "\"");
    push(Tag::string, str.value);
    push(Tag::string, "\"");
  }
  void visit(ast::CharLiteral &ch) override {
    push(Tag::character, "'");
    push(Tag::character, ch.value);
    push(Tag::character, "'");
  }
  void visit(ast::NumberLiteral &num) override {
    push(Tag::number, num.value);
  }
  void visit(ast::BoolLiteral &bol) override {
    if (bol.value) {
      push(Tag::keyword, "true");
    } else {
      push(Tag::keyword, "false");
    }
  }
  void visit(ast::ArrayLiteral &arr) override {
    pushOp("[");
    pushExprs(arr.exprs);
    pushOp("]");
  }
  void visit(ast::InitList &list) override {
    pushOp("{");
    pushExprs(list.exprs);
    pushOp("}");
  }
  void visit(ast::Lambda &lam) override {
    pushKey("func");
    pushParams(lam.params);
    pushSpace();
    if (lam.ret) {
      pushOp("->");
      pushSpace();
      lam.ret->accept(*this);
      pushSpace();
    }
    lam.body.accept(*this);
  }
  
  fmt::Tokens tokens;

private:
  uint32_t indent = 0;
  bool indented = false;
  
  void pushNewline(const uint32_t count = 1) {
    if (count != 0) {
      fmt::Token tok {};
      tok.tag = fmt::Tag::newline;
      tok.count = count;
      tokens.push_back(tok);
    }
    indented = false;
  }
  void pushIndent() {
    if (!indented && indent != 0) {
      fmt::Token tok {};
      tok.tag = fmt::Tag::indent;
      tok.count = indent;
      tokens.push_back(tok);
    }
    indented = true;
  }
  void push(const fmt::Tag tag, const std::string_view text) {
    if (text.size() != 0) {
      fmt::Token tok {};
      tok.tag = tag;
      tok.text = text;
      tokens.push_back(tok);
    }
  }
  void pushSpace() {
    push(Tag::plain, " ");
  }
  void pushOp(const std::string_view op) {
    push(Tag::oper, op);
  }
  void pushKey(const std::string_view keyword) {
    push(Tag::keyword, keyword);
    pushSpace();
  }
  
  void pushKeyName(const std::string_view keyword, const std::string_view name) {
    pushIndent();
    pushKey(keyword);
    push(Tag::plain, name);
  }
  void pushParam(const ast::FuncParam &param) {
    push(Tag::plain, param.name);
    pushOp(":");
    pushSpace();
    if (param.ref == ast::ParamRef::ref) {
      pushKey("ref");
    }
    param.type->accept(*this);
  }
  void pushParams(const ast::FuncParams &params) {
    pushOp("(");
    for (auto p : citer_range(params)) {
      pushParam(*p);
      if (p != params.cend() - 1) {
        pushOp(",");
        pushSpace();
      }
    }
    pushOp(")");
  }
  void pushExprs(const std::vector<ast::ExprPtr> &exprs) {
    for (auto e : citer_range(exprs)) {
      (*e)->accept(*this);
      if (e != exprs.cend() - 1) {
        pushOp(",");
        pushSpace();
      }
    }
  }
  void pushBlockBody(ast::Block &block) {
    for (ast::StatPtr &stat : block.nodes) {
      pushIndent();
      stat->accept(*this);
      pushNewline();
    }
  }
};

}

stela::fmt::Tokens stela::format(ast::Node *node) {
  assert(node);
  Visitor visitor;
  node->accept(visitor);
  return visitor.tokens;
}

stela::fmt::Tokens stela::format(const AST &ast) {
  Visitor visitor;
  for (const ast::DeclPtr &node : ast.global) {
    node->accept(visitor);
    visitor.tokens.push_back(fmt::Token{{1}, fmt::Tag::newline});
    visitor.tokens.push_back(fmt::Token{{1}, fmt::Tag::newline});
  }
  if (!ast.global.empty()) {
    visitor.tokens.pop_back();
  }
  return visitor.tokens;
}

stela::fmt::Tokens stela::format(const std::string_view source, LogSink &sink) {
  return format(createAST(source, sink));
}
