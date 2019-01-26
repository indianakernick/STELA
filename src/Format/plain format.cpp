//
//  plain format.cpp
//  STELA
//
//  Created by Indi Kernick on 26/11/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "plain format.hpp"

#include <iostream>

std::string stela::plainFormat(const fmt::Tokens &tokens, const uint32_t indentSize) {
  std::string out;
  out.reserve(tokens.size() * 8);
  for (const fmt::Token &token : tokens) {
    if (token.tag == fmt::Tag::newline) {
      out.append(token.count, '\n');
    } else if (token.tag == fmt::Tag::indent) {
      out.append(token.count * indentSize, ' ');
    } else {
      out.append(token.text.data(), token.text.size());
    }
  }
  return out;
}

namespace {

void repeat(std::ostream &stream, const char c, uint32_t count) {
  while (count--) {
    stream.put(c);
  }
}

}

void stela::plainFormat(std::ostream &stream, const fmt::Tokens &tokens, const uint32_t indentSize) {
  for (const fmt::Token &token : tokens) {
    if (token.tag == fmt::Tag::newline) {
      repeat(stream, '\n', token.count);
    } else if (token.tag == fmt::Tag::indent) {
      repeat(stream, ' ', token.count * indentSize);
    } else {
      stream.write(token.text.data(), static_cast<std::streamsize>(token.text.size()));
    }
  }
}

std::string stela::plainFormatInline(const fmt::Tokens &tokens) {
  std::string out;
  out.reserve(tokens.size() * 8);
  for (const fmt::Token &token : tokens) {
    if (token.tag == fmt::Tag::newline) {
      out.push_back(' ');
    } else if (token.tag != fmt::Tag::indent) {
      out.append(token.text.data(), token.text.size());
    }
  }
  return out;
}

void stela::plainFormatInline(std::ostream &stream, const fmt::Tokens &tokens) {
  for (const fmt::Token &token : tokens) {
    if (token.tag == fmt::Tag::newline) {
      stream.put(' ');
    } else if (token.tag != fmt::Tag::indent) {
      stream.write(token.text.data(), static_cast<std::streamsize>(token.text.size()));
    }
  }
}
