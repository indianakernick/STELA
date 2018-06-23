//
//  log.cpp
//  STELA
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "log.hpp"

#include <iomanip>
#include <sstream>
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

void stela::Log::cat(const LogCat newCat) {
  category = newCat;
}

void stela::Log::pri(const LogPri newPri) {
  priority = newPri;
}

std::ostream &stela::Log::info(const Loc loc) {
  return log(LogPri::info, loc);
}

std::ostream &stela::Log::warn(const Loc loc) {
  return log(LogPri::warning, loc);
}

std::ostream &stela::Log::error(const Loc loc) {
  return log(LogPri::error, loc);
}

std::ostream &stela::Log::log(const LogPri pri, const Loc loc) {
  if (canLog(pri)) {
    return log(category, pri, loc);
  } else {
    return silentStream();
  }
}

std::ostream &stela::Log::info() {
  return log(LogPri::info);
}

std::ostream &stela::Log::warn() {
  return log(LogPri::warning);
}

std::ostream &stela::Log::error() {
  return log(LogPri::error);
}

std::ostream &stela::Log::log(const LogPri pri) {
  if (canLog(pri)) {
    return log(category, pri);
  } else {
    return silentStream();
  }
}

void stela::Log::endInfo() {
  end(LogPri::info);
}

void stela::Log::endWarn() {
  end(LogPri::warning);
}

void stela::Log::endError() {
  end(LogPri::error);
}

void stela::Log::end(const LogPri pri) {
  if (canLog(pri)) {
    endLog(category, pri);
  }
}

bool stela::Log::canLog(const LogPri pri) const {
  return static_cast<uint8_t>(pri) >= static_cast<uint8_t>(priority);
}

stela::StreamLog::StreamLog()
  : StreamLog{std::cerr} {}

stela::StreamLog::StreamLog(std::ostream &stream)
  : stream{stream} {}

std::ostream &stela::StreamLog::log(const LogCat cat, const LogPri pri, const Loc loc) {
  std::ostringstream str;
  str << loc;
  stream << std::left << std::setw(8) << str.str();
  return stream << cat << ' ' << pri << ": ";
}

std::ostream &stela::StreamLog::log(const LogCat cat, const LogPri pri) {
  return stream << cat << ' ' << pri << ": ";
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

std::ostream &stela::NoLog::log(LogCat, LogPri) {
  return silentStream();
}

void stela::NoLog::endLog(LogCat, LogPri) {}
