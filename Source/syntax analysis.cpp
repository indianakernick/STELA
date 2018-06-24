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

//--------------------------------- Types --------------------------------------

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
      auto dictType = std::make_unique<ast::DictType>();
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

//------------------------------ Expressions -----------------------------------

ast::ExprPtr parseExpr(ParseTokens &tok) {
  tok.expectID();
  return std::make_unique<ast::BinaryExpr>();
}

//------------------------------- Statements -----------------------------------

ast::Block parseBlock(ParseTokens &);

ast::StatPtr parseVar(ParseTokens &tok) {
  if (!tok.checkKeyword("var")) {
    return nullptr;
  }
  auto var = std::make_unique<ast::Var>();
  var->name = tok.expectID();
  if (tok.checkOp(":")) {
    var->type = tok.expectNode(parseType, "type after :");
  }
  if (tok.checkOp("=")) {
    var->expr = tok.expectNode(parseExpr, "expression after =");
  }
  tok.expectOp(";");
  return var;
}

ast::StatPtr parseLet(ParseTokens &tok) {
  if (!tok.checkKeyword("let")) {
    return nullptr;
  }
  auto let = std::make_unique<ast::Let>();
  let->name = tok.expectID();
  if (tok.checkOp(":")) {
    let->type = tok.expectNode(parseType, "type after :");
  }
  tok.expectOp("=");
  let->expr = tok.expectNode(parseExpr, "expression after =");
  tok.expectOp(";");
  return let;
}

ast::StatPtr parseIf(ParseTokens &tok) {
  if (!tok.checkKeyword("if")) {
    return nullptr;
  }
  auto ifNode = std::make_unique<ast::If>();
  tok.expectOp("(");
  ifNode->cond = parseExpr(tok);
  tok.expectOp(")");
  ifNode->body = parseBlock(tok);
  if (tok.checkKeyword("else")) {
    ifNode->elseBody = parseBlock(tok);
  }
  return ifNode;
}

template <typename Type>
ast::StatPtr parseKeywordStatement(ParseTokens &tok, const std::string_view name) {
  if (tok.checkKeyword(name)) {
    tok.expectOp(";");
    return std::make_unique<Type>();
  } else {
    return nullptr;
  }
}

ast::StatPtr parseBreak(ParseTokens &tok) {
  return parseKeywordStatement<ast::Break>(tok, "break");
}

ast::StatPtr parseContinue(ParseTokens &tok) {
  return parseKeywordStatement<ast::Continue>(tok, "continue");
}

ast::StatPtr parseFallthrough(ParseTokens &tok) {
  return parseKeywordStatement<ast::Fallthrough>(tok, "fallthrough");
}

ast::StatPtr parseReturn(ParseTokens &tok) {
  if (tok.checkKeyword("return")) {
    auto ret = std::make_unique<ast::Return>();
    ret->expr = parseExpr(tok);
    tok.expectOp(";");
    return ret;
  } else {
    return nullptr;
  }
}

ast::StatPtr parseStatement(ParseTokens &tok) {
  if (ast::StatPtr node = parseVar(tok)) return node;
  if (ast::StatPtr node = parseLet(tok)) return node;
  if (ast::StatPtr node = parseIf(tok)) return node;
  if (ast::StatPtr node = parseBreak(tok)) return node;
  if (ast::StatPtr node = parseContinue(tok)) return node;
  if (ast::StatPtr node = parseFallthrough(tok)) return node;
  if (ast::StatPtr node = parseReturn(tok)) return node;
  return nullptr;
}

ast::Block parseBlock(ParseTokens &tok) {
  ast::Block block;
  
  if (tok.checkOp("{")) {
    while (ast::StatPtr node = parseStatement(tok)) {
      block.nodes.emplace_back(std::move(node));
    }
    tok.expectOp("}");
  } else {
    ast::StatPtr node = parseStatement(tok);
    if (node == nullptr) {
      tok.log().ferror(tok.front().loc) << "Expected statement" << endlog;
    } else {
      block.nodes.emplace_back(std::move(node));
    }
  }
  
  return block;
}

//------------------------------- Functions ------------------------------------

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

ast::StatPtr parseFunc(ParseTokens &tok) {
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

//--------------------------- Type Declarations --------------------------------

ast::StatPtr parseEnum(ParseTokens &tok) {
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

ast::StatPtr parseStruct(ParseTokens &) {
  return nullptr;
}

ast::StatPtr parseTypealias(ParseTokens &tok) {
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

//---------------------------------- Base --------------------------------------

AST createASTimpl(const Tokens &tokens, Log &log) {
  AST ast;
  ParseTokens tok(tokens, log);
  
  while (!tok.empty()) {
    ast::StatPtr node;
    if ((node = parseFunc(tok))) {}
    else if ((node = parseEnum(tok))) {}
    else if ((node = parseStruct(tok))) {}
    else if ((node = parseTypealias(tok))) {}
    else if ((node = parseLet(tok))) {}
    else {
      const Token &token = tok.front();
      log.info(token.loc) << "Only functions, type declarations and constants "
        "are allowed at global scope" << endlog;
      log.ferror(token.loc) << "Unexpected token `" << token.view << "` "
        << static_cast<int>(token.type) << " in global scope" << endlog;
    };
    ast.global.emplace_back(std::move(node));
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
