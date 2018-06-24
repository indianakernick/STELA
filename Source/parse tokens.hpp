//
//  parse tokens.hpp
//  STELA
//
//  Created by Indi Kernick on 24/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_parse_tokens_hpp
#define stela_parse_tokens_hpp

#include "token.hpp"
#include "log output.hpp"

namespace stela {

std::ostream &operator<<(std::ostream &, Token::Type);
std::ostream &operator<<(std::ostream &, const Token &);

class ParseTokens {
public:
  ParseTokens(const Tokens &, Log &);
  
  Log &log() const;
  bool empty() const;
  const Token &front() const;
  Loc lastLoc() const;
  
  void unget();
  
  template <typename ParseFunc>
  auto expectNode(ParseFunc &&parse, const std::string_view msg) {
    auto node = parse(*this);
    if (node == nullptr) {
      log_.ferror(beg->loc) << "Expected " << msg << endlog;
    }
    return node;
  }
  
  bool check(Token::Type, std::string_view);
  bool checkKeyword(std::string_view);
  bool checkOp(std::string_view);
  
  std::string_view expect(Token::Type);
  void expect(Token::Type, std::string_view);
  std::string_view expectEither(Token::Type, std::string_view, std::string_view);
  
  std::string_view expectID();
  std::string_view expectOp();
  void expectOp(std::string_view);
  std::string_view expectEitherOp(std::string_view, std::string_view);
  void expectKeyword(std::string_view);

private:
  const Token *beg;
  const Token *end;
  Log &log_;
  
  void expectToken();
};

}

#endif
