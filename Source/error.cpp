//
//  error.cpp
//  STELA
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "error.hpp"

#include <iostream>

std::ostream &stela::operator<<(std::ostream &stream, const LogCat cat) {
  switch (cat) {
    case LogCat::lexical:
      return stream << "Lexical";
    case LogCat::syntax:
      return stream << "Syntax";
    case LogCat::semantic:
      return stream << "Semantic";
    default:
      return stream;
  }
}

std::ostream &stela::operator<<(std::ostream &stream, const LogPri pri) {
  switch (pri) {
    case LogPri::info:
      return stream << "info";
    case LogPri::warning:
      return stream << "warning";
    case LogPri::error:
      return stream << "error";
    default:
      return stream;
  }
}

std::ostream &stela::operator<<(std::ostream &stream, const Loc loc) {
  return stream << loc.l << ':' << loc.c;
}

const char *stela::FatalError::what() const noexcept {
  return "A fatal error has occured";
}

void stela::Logger::cat(const LogCat newCat) {
  category = newCat;
}

void stela::Logger::pri(const LogPri newPri) {
  priority = newPri;
}

std::ostream &stela::Logger::info(const Loc loc) {
  return log(LogPri::info, loc);
}

std::ostream &stela::Logger::warn(const Loc loc) {
  return log(LogPri::warning, loc);
}

std::ostream &stela::Logger::error(const Loc loc) {
  return log(LogPri::error, loc);
}

std::ostream &stela::Logger::log(const LogPri pri, const Loc loc) {
  if (static_cast<uint8_t>(pri) >= static_cast<uint8_t>(priority)) {
    return log(category, pri, loc);
  } else {
    return silentStream();
  }
}

void stela::Logger::endInfo() {
  end(LogPri::info);
}

void stela::Logger::endWarn() {
  end(LogPri::warning);
}

void stela::Logger::endError() {
  end(LogPri::error);
}

void stela::Logger::end(const LogPri pri) {
  if (static_cast<uint8_t>(pri) >= static_cast<uint8_t>(priority)) {
    endLog(category, pri);
  }
}

stela::StreamLog::StreamLog()
  : StreamLog{std::cerr} {}

stela::StreamLog::StreamLog(std::ostream &stream)
  : stream{stream} {}

std::ostream &stela::StreamLog::log(const LogCat cat, const LogPri pri, const Loc loc) {
  return stream << loc << "  " << cat << ' ' << pri << ": ";
}

void stela::StreamLog::endLog(LogCat, LogPri) {
  stream << '\n';
}

namespace {

class SilentBuf : public std::streambuf {
public:
  std::streamsize xsputn(const char_type *, const std::streamsize n) override {
    return n;
  }
  int_type overflow(const int_type c) override {
    return c;
  }
};

}

std::ostream &stela::silentStream() {
  static SilentBuf buf;
  static std::ostream stream{&buf};
  return stream;
}

std::ostream &stela::NoLog::log(LogCat, LogPri, Loc) {
  return silentStream();
}

void stela::NoLog::endLog(LogCat, LogPri) {}
