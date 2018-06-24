//
//  syntax analysis.cpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "syntax analysis.hpp"

#include "log output.hpp"
#include "parse tokens.hpp"
#include "lexical analysis.hpp"

using namespace stela;

namespace {

ast::ParamRef parseRef(ParseTokens &tok) {
  if (tok.checkKeyword("inout")) {
    return ast::ParamRef::inout;
  } else {
    return ast::ParamRef::value;
  }
}

ast::TypePtr parseType(ParseTokens &);

ast::TypePtr parseFuncType(ParseTokens &tok) {
  auto type = std::make_unique<ast::FuncType>();
  tok.expectOp("(");
  if (!tok.checkOp(")")) {
    do {
      ast::ParamType &param = type->params.emplace_back();
      param.ref = parseRef(tok);
      param.type = parseType(tok);
    } while (tok.expectEitherOp(")", ",") == ",");
  }
  tok.expectOp("->");
  type->ret = parseType(tok);
  return type;
}

ast::TypePtr parseType(ParseTokens &tok) {
  if (tok.checkOp("(")) {
    tok.unget();
    return parseFuncType(tok);
  } else if (tok.checkOp("[")) {
    // could be an array or a dictionary
    ast::TypePtr elem = parseType(tok);
    if (tok.checkOp(":")) {
      // it's a dictionary and `elem` is the key
      ast::TypePtr val = parseType(tok);
      tok.expectOp("]");
      auto dictType = std::make_unique<ast::DictionaryType>();
      dictType->key = std::move(elem);
      dictType->val = std::move(val);
      return dictType;
    } else {
      // it's an array and `elem` is the element type
      tok.expectOp("]");
      auto arrayType = std::make_unique<ast::ArrayType>();
      arrayType->elem = std::move(elem);
      return arrayType;
    }
  } else {
    // not a function,
    // not an array,
    // not a dictionary
    // so it must be a named type like `Int` or `MyClass`
    auto namedType = std::make_unique<ast::NamedType>();
    namedType->name = tok.expectID();
    return namedType;
  }
}

ast::Block parseBlock(ParseTokens &tok) {
  tok.expectOp("{");
  tok.expectOp("}");
  return {};
}

ast::FuncParams parseFuncParams(ParseTokens &tok) {
  tok.expectOp("(");
  if (tok.checkOp(")")) {
    return {};
  }
  ast::FuncParams params;
  do {
    ast::FuncParam &param = params.emplace_back();
    param.name = tok.expectID();
    tok.expectOp(":");
    param.ref = parseRef(tok);
    param.type = parseType(tok);
  } while (tok.expectEitherOp(")", ",") == ",");
  return params;
}

ast::TypePtr parseFuncRet(ParseTokens &tok) {
  if (tok.checkOp("->")) {
    return parseType(tok);
  } else {
    return nullptr;
  }
}

ast::NodePtr parseFunc(ParseTokens &tok) {
  if (!tok.checkKeyword("func")) {
    return nullptr;
  }
  
  auto funcNode = std::make_unique<ast::Func>();
  funcNode->name = tok.expectID();
  funcNode->params = parseFuncParams(tok);
  funcNode->ret = parseFuncRet(tok);
  funcNode->body = parseBlock(tok);
  
  return funcNode;
}

ast::NodePtr parseExpr(ParseTokens &) {
  return nullptr;
}

ast::NodePtr parseEnum(ParseTokens &tok) {
  if (!tok.checkKeyword("enum")) {
    return nullptr;
  }
  auto enumNode = std::make_unique<ast::Enum>();
  enumNode->name = tok.expectID();
  tok.expectOp("{");
  
  if (!tok.checkOp("}")) {
    do {
      ast::EnumCase &ecase = enumNode->cases.emplace_back();
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

ast::NodePtr parseStruct(ParseTokens &) {
  return nullptr;
}

ast::NodePtr parseTypealias(ParseTokens &tok) {
  if (!tok.checkKeyword("typealias")) {
    return nullptr;
  }
  auto aliasNode = std::make_unique<ast::TypeAlias>();
  aliasNode->name = tok.expectID();
  tok.expectOp("=");
  aliasNode->type = parseType(tok);
  tok.expectOp(";");
  return aliasNode;
}

ast::NodePtr parseLet(ParseTokens &) {
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
      log.info(token.loc) << "Only functions, type declarations and constants "
        "are allowed at global scope" << endlog;
      log.ferror(token.loc) << "Unexpected token in global scope" << endlog;
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
