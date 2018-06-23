//
//  syntax analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "syntax analysis.hpp"

#include "log output.hpp"
#include "lexical analysis.hpp"

using namespace stela;
using namespace stela::ast;

namespace {

stela::LogStream &operator<<(stela::LogStream &stream, const Token::Type type) {
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

stela::LogStream &operator<<(stela::LogStream &stream, const Token &tok) {
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
  Loc lastLoc() const {
    return beg[-1].loc;
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
      log_.ferror(beg->loc) << "Expected " << type << " but found " << *beg << endlog;
    }
    const std::string_view view = beg->view;
    ++beg;
    return view;
  }
  void expect(const Token::Type type, const std::string_view view) {
    expectToken();
    if (beg->type != type || beg->view != view) {
      log_.ferror(beg->loc) << "Expected " << type << ' ' << view
        << " but found " << *beg << endlog;
    }
    ++beg;
  }
  std::string_view expectEither(const Token::Type type, const std::string_view a, const std::string_view b) {
    expectToken();
    if (beg->type != type || (beg->view != a && beg->view != b)) {
      log_.ferror(beg->loc) << "Expected " << type << ' ' << a << " or " << b
        << " but found " << *beg << endlog;
    }
    const std::string_view view = beg->view;
    ++beg;
    return view;
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
  std::string_view expectEitherOp(const std::string_view a, const std::string_view b) {
    return expectEither(Token::Type::oper, a, b);
  }

private:
  const Token *beg;
  const Token *end;
  Log &log_;
  
  void expectToken() {
    if (empty()) {
      log_.ferror() << "Unexpected end of input" << endlog;
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
  
  if (!tok.checkOp("}")) {
    do {
      EnumCase &ecase = enumNode->cases.emplace_back();
      ecase.name = tok.expectID();
      if (tok.checkOp("=")) {
        ecase.value = parseExpr(tok);
      }
    } while (tok.expectEitherOp("}", ",") == ",");
  }
  
  if (tok.checkOp(";")) {
    tok.log().warn(tok.lastLoc()) << "Unnecessary ;" << endlog;
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
      log.error(token.loc) << "Unexpected token in global scope" << endlog;
      log.info(token.loc) << "Only functions, type declarations and constants "
        "are allowed at global scope" << endlog;
      throw FatalError{};
    };
    ast.topNodes.emplace_back(std::move(node));
  }
  
  return ast;
}

}

AST stela::createAST(const Tokens &tokens, LogBuf &buf) {
  Log log{buf};
  try {
    log.cat(stela::LogCat::syntax);
    return createASTimpl(tokens, log);
  } catch (FatalError &) {
    throw;
  } catch (std::exception &e) {
    log.error() << e.what() << endlog;
    throw;
  }
}

AST stela::createAST(const std::string_view source, LogBuf &buf) {
  return createAST(lex(source, buf), buf);
}
