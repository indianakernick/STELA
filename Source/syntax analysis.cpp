//
//  syntax analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "syntax analysis.hpp"

#include <iostream>
#include "lexical analysis.hpp"

using namespace stela;
using namespace stela::ast;

namespace {

std::ostream &operator<<(std::ostream &stream, const Token::Type type) {
  switch (type) {
    case Token::Type::keyword:
      return stream << "keyword";
    case Token::Type::identifier:
      return stream << "identifier";
    case Token::Type::number:
      return stream << "number";
    case Token::Type::string:
      return stream << "string";
    case Token::Type::character:
      return stream << "character";
    case Token::Type::oper:
      return stream << "operator";
    default:
      return stream;
  }
}

std::ostream &operator<<(std::ostream &stream, const Token &tok) {
  return stream << tok.type << ' ' << tok.view;
}

class ParseTokens {
public:
  ParseTokens(const Tokens &tokens, Log &log)
    : beg{tokens.data()}, end{tokens.data() + tokens.size()}, log_{log} {}
  
  Log &log() {
    return log_;
  }
  bool empty() const {
    return beg == end;
  }
  const Token &front() const {
    return *beg;
  }
  
  bool check(const Token::Type type, const std::string_view view) {
    if (empty() || beg->type != type || beg->view != view) {
      return false;
    } else {
      ++beg;
      return true;
    }
  }
  bool checkKeyword(const std::string_view view) {
    return check(Token::Type::keyword, view);
  }
  bool checkOp(const std::string_view view) {
    return check(Token::Type::oper, view);
  }
  
  std::string_view expect(const Token::Type type) {
    expectToken();
    if (beg->type != type) {
      log_.error(beg->loc) << "Expected " << type << " but found " << *beg;
      log_.endError();
      throw FatalError{};
    }
    const std::string_view str = beg->view;
    ++beg;
    return str;
  }
  void expect(const Token::Type type, const std::string_view view) {
    expectToken();
    if (beg->type != type || beg->view != view) {
      log_.error(beg->loc) << "Expected " << type << ' ' << view
        << " but found " << *beg;
      log_.endError();
      throw FatalError{};
    }
    ++beg;
  }
  std::string_view expectID() {
    return expect(Token::Type::identifier);
  }
  std::string_view expectOp() {
    return expect(Token::Type::oper);
  }
  void expectOp(const std::string_view view) {
    expect(Token::Type::oper, view);
  }

private:
  const Token *beg;
  const Token *end;
  Log &log_;
  
  void expectToken() {
    if (empty()) {
      log_.error() << "Unexpected end of input";
      log_.endError();
      throw FatalError{};
    }
  }
};

NodePtr parseFunc(ParseTokens &) {
  return nullptr;
}

NodePtr parseExpr(ParseTokens &) {
  return nullptr;
}

NodePtr parseEnum(ParseTokens &tok) {
  if (!tok.checkKeyword("enum")) {
    return nullptr;
  }
  auto enumNode = std::make_unique<ast::Enum>();
  enumNode->name = tok.expectID();
  tok.expectOp("{");
  
  if (tok.checkOp("}")) {
    return enumNode;
  }
  
  while (true) {
    EnumCase ecase;
    ecase.name = tok.expectID();
    if (tok.checkOp("=")) {
      ecase.value = parseExpr(tok);
    }
    enumNode->cases.emplace_back(std::move(ecase));
    
    if (tok.checkOp("}")) {
      break;
    } else {
      tok.expectOp(",");
    }
  }
  
  return enumNode;
}

NodePtr parseStruct(ParseTokens &) {
  return nullptr;
}

NodePtr parseTypealias(ParseTokens &) {
  return nullptr;
}

NodePtr parseLet(ParseTokens &) {
  return nullptr;
}

AST createASTimpl(const Tokens &tokens, Log &log) {
  AST ast;
  ParseTokens tok(tokens, log);
  
  while (!tok.empty()) {
    ast::NodePtr node;
    if ((node = parseFunc(tok))) {}
    else if ((node = parseEnum(tok))) {}
    else if ((node = parseStruct(tok))) {}
    else if ((node = parseTypealias(tok))) {}
    else if ((node = parseLet(tok))) {}
    else {
      const Token &token = tok.front();
      log.error(token.loc) << "Unexpected token in global scope";
      log.endError();
      log.info(token.loc) << "Only functions, type declarations and constants are allowed at global scope";
      log.endInfo();
      throw FatalError{};
    };
    ast.topNodes.emplace_back(std::move(node));
  }
  
  return ast;
}

}

AST stela::createAST(const Tokens &tokens, Log &log) try {
  log.cat(stela::LogCat::syntax);
  return createASTimpl(tokens, log);
} catch (FatalError &) {
  throw;
} catch (std::exception &e) {
  log.error() << e.what();
  log.endError();
  throw;
}

AST stela::createAST(const std::string_view source, Log &log) {
  return createAST(lex(source, log), log);
}
