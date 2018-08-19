//
//  format.cpp
//  STELA
//
//  Created by Indi Kernick on 14/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "format.hpp"

#include "syntax analysis.hpp"

using namespace stela;
using namespace stela::fmt;

namespace {

class Visitor final : public stela::ast::Visitor {
public:
  void visit(ast::ArrayType &type) override {
    pushOp("[");
    type.elem->accept(*this);
    pushOp("]");
  }
  void visit(ast::MapType &type) override {
    pushOp("{");
    type.key->accept(*this);
    pushOp(":");
    pushSpace();
    type.val->accept(*this);
    pushOp("}");
  }
  void visit(ast::FuncType &type) override {
    pushOp("(");
    for (auto p = type.params.cbegin(); p != type.params.cend(); ++p) {
      if (p->ref == ast::ParamRef::inout) {
        pushKey("inout");
      }
      p->type->accept(*this);
      if (p != type.params.cend() - 1) {
        pushOp(",");
        pushSpace();
      }
    }
    pushOp(")");
    pushSpace();
    pushOp("->");
    pushSpace();
    type.ret->accept(*this);
  }
  void visit(ast::NamedType &type) override {
    push(Tag::type_name, type.name);
  }
  void visit(ast::NestedType &type) override {
    type.parent->accept(*this);
    pushOp(".");
    push(Tag::type_name, type.name);
  }
  
  void visit(ast::Assignment &as) override {
    as.left->accept(*this);
    pushSpace();
    switch (as.oper) {
      case ast::AssignOp::assign:
        pushOp("="); break;
      case ast::AssignOp::add:
        pushOp("+="); break;
      case ast::AssignOp::sub:
        pushOp("-="); break;
      case ast::AssignOp::mul:
        pushOp("*="); break;
      case ast::AssignOp::div:
        pushOp("/="); break;
      case ast::AssignOp::mod:
        pushOp("%="); break;
      case ast::AssignOp::pow:
        pushOp("**="); break;
      case ast::AssignOp::bit_or:
        pushOp("|="); break;
      case ast::AssignOp::bit_xor:
        pushOp("^="); break;
      case ast::AssignOp::bit_and:
        pushOp("&="); break;
      case ast::AssignOp::bit_shl:
        pushOp("<<="); break;
      case ast::AssignOp::bit_shr:
        pushOp(">>="); break;
    }
    pushSpace();
    as.right->accept(*this);
  }
  void visit(ast::BinaryExpr &bin) override {
    bin.left->accept(*this);
    pushSpace();
    switch (bin.oper) {
      case ast::BinOp::bool_or:
        pushOp("||"); break;
      case ast::BinOp::bool_and:
        pushOp("&&"); break;
      case ast::BinOp::bit_or:
        pushOp("|"); break;
      case ast::BinOp::bit_xor:
        pushOp("^"); break;
      case ast::BinOp::bit_and:
        pushOp("&"); break;
      case ast::BinOp::eq:
        pushOp("=="); break;
      case ast::BinOp::ne:
        pushOp("!="); break;
      case ast::BinOp::lt:
        pushOp("<"); break;
      case ast::BinOp::le:
        pushOp("<="); break;
      case ast::BinOp::gt:
        pushOp(">"); break;
      case ast::BinOp::ge:
        pushOp(">="); break;
      case ast::BinOp::bit_shl:
        pushOp("<<"); break;
      case ast::BinOp::bit_shr:
        pushOp(">>"); break;
      case ast::BinOp::add:
        pushOp("+"); break;
      case ast::BinOp::sub:
        pushOp("-"); break;
      case ast::BinOp::mul:
        pushOp("*"); break;
      case ast::BinOp::div:
        pushOp("/"); break;
      case ast::BinOp::mod:
        pushOp("%"); break;
      case ast::BinOp::pow:
        pushOp("**"); break;
    }
    pushSpace();
    bin.right->accept(*this);
  }
  void visit(ast::UnaryExpr &un) override {
    switch (un.oper) {
      case ast::UnOp::neg:
        pushOp("-"); break;
      case ast::UnOp::bool_not:
        pushOp("!"); break;
      case ast::UnOp::bit_not:
        pushOp("~"); break;
      case ast::UnOp::pre_incr:
        pushOp("++"); break;
      case ast::UnOp::pre_decr:
        pushOp("--"); break;
      default: ;
    }
    un.expr->accept(*this);
    if (un.oper == ast::UnOp::post_incr) {
      pushOp("++");
    } else if (un.oper == ast::UnOp::post_decr) {
      pushOp("--");
    }
  }
  void visit(ast::FuncCall &call) override {
    call.func->accept(*this);
    pushArgs(call.args);
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
  void visit(ast::Self &) override {
    push(Tag::keyword, "self");
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
  void visit(ast::EmptyStatement &) override {
    if (tokens.back().tag == Tag::plain && tokens.back().text == " ") {
      tokens.pop_back();
    }
    pushOp(";");
  }
  void visit(ast::If &fi) override {
    pushKey("if");
    pushOp("(");
    fi.cond->accept(*this);
    pushOp(")");
    pushSpace();
    pushStat(fi.body);
    if (fi.elseBody) {
      pushSpace();
      pushKey("else");
      pushStat(fi.elseBody);
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
      pushStat(cs.body);
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
    pushStat(whl.body);
  }
  void visit(ast::RepeatWhile &rep) override {
    pushKey("repeat");
    pushStat(rep.body);
    pushSpace();
    pushKey("while");
    pushOp("(");
    rep.cond->accept(*this);
    pushOp(")");
    pushOp(";");
  }
  void visit(ast::For &fr) override {
    pushKey("for");
    pushOp("(");
    if (fr.init) {
      pushStat(fr.init);
    } else {
      pushOp(";");
    }
    pushSpace();
    fr.cond->accept(*this);
    pushOp(";");
    pushSpace();
    if (fr.incr) {
      fr.incr->accept(*this);
    }
    pushOp(")");
    pushSpace();
    fr.body->accept(*this);
  }
  
  void visit(ast::Func &func) override {
    pushKeyName("func", func.name);
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
    pushKeyName("typealias", alias.name);
    pushSpace();
    pushOp("=");
    pushSpace();
    alias.type->accept(*this);
    pushOp(";");
  }
  void visit(ast::Init &init) override {
    push(Tag::keyword, "init");
    pushParams(init.params);
    pushSpace();
    init.body.accept(*this);
  }
  void visit(ast::Struct &strt) override {
    pushKeyName("struct", strt.name);
    pushSpace();
    pushOp("{");
    if (strt.body.empty()) {
      pushOp("}");
      return;
    }
    pushNewline();
    ++indent;
    for (const ast::Member &mem : strt.body) {
      pushIndent();
      if (mem.access == ast::MemAccess::public_) {
        pushKey("public");
      } else if (mem.access == ast::MemAccess::private_) {
        pushKey("private");
      }
      if (mem.scope == ast::MemScope::static_) {
        pushKey("static");
      }
      mem.node->accept(*this);
      pushNewline();
    }
    --indent;
    pushIndent();
    pushOp("}");
  }
  void visit(ast::Enum &enm) override {
    pushKeyName("enum", enm.name);
    pushSpace();
    pushOp("{");
    if (enm.cases.empty()) {
      pushOp("}");
      return;
    }
    pushNewline();
    ++indent;
    for (auto cs = enm.cases.cbegin(); cs != enm.cases.cend(); ++cs) {
      pushIndent();
      push(Tag::plain, cs->name);
      if (cs->value) {
        pushSpace();
        pushOp("=");
        pushSpace();
        cs->value->accept(*this);
      }
      if (cs != enm.cases.cend() - 1) {
        pushOp(",");
      }
      pushNewline();
    }
    --indent;
    pushIndent();
    pushOp("}");
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
    for (auto e = arr.exprs.cbegin(); e != arr.exprs.cend(); ++e) {
      (*e)->accept(*this);
      if (e != arr.exprs.cend() - 1) {
        pushOp(",");
        pushSpace();
      }
    }
    pushOp("]");
  }
  void visit(ast::MapLiteral &map) override {
    pushOp("{");
    if (map.pairs.empty()) {
      pushOp("}");
      return;
    }
    pushNewline();
    ++indent;
    for (auto p = map.pairs.cbegin(); p != map.pairs.cend(); ++p) {
      pushIndent();
      p->key->accept(*this);
      pushOp(":");
      pushSpace();
      p->val->accept(*this);
      if (p != map.pairs.cend() - 1) {
        pushOp(",");
        pushSpace();
      }
      pushNewline();
    }
    --indent;
    pushIndent();
    pushOp("}");
  }
  void visit(ast::Lambda &lam) override {
    pushOp("{");
    pushParams(lam.params);
    pushSpace();
    pushKey("in");
    pushNewline();
    ++indent;
    pushBlockBody(lam.body);
    --indent;
    pushOp("}");
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
  void pushParams(const ast::FuncParams &params) {
    pushOp("(");
    for (auto p = params.cbegin(); p != params.cend(); ++p) {
      push(Tag::plain, p->name);
      pushOp(":");
      pushSpace();
      if (p->ref == ast::ParamRef::inout) {
        pushKey("inout");
      }
      p->type->accept(*this);
      if (p != params.cend() - 1) {
        pushOp(",");
        pushSpace();
      }
    }
    pushOp(")");
  }
  void pushArgs(const ast::FuncArgs &args) {
    pushOp("(");
    for (auto a = args.cbegin(); a != args.cend(); ++a) {
      (*a)->accept(*this);
      if (a != args.cend() - 1) {
        pushOp(",");
        pushSpace();
      }
    }
    pushOp(")");
  }
  void pushStat(ast::StatPtr &stat) {
    stat->accept(*this);
    if (auto *const expr = dynamic_cast<ast::Expression *>(stat.get())) {
      pushOp(";");
    }
  }
  void pushBlockBody(ast::Block &block) {
    for (ast::StatPtr &stat : block.nodes) {
      pushIndent();
      pushStat(stat);
      pushNewline();
    }
  }
};

}

stela::fmt::Tokens stela::format(const std::string_view source, LogBuf &buf) {
  AST ast = createAST(source, buf);
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
