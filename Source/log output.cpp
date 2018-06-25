//
//  log output.cpp
//  STELA
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "log output.hpp"

stela::LogStream::LogStream(LogBuf &buf, const LogCat cat, const LogPri pri)
  : std::ostream{buf.getStreambuf(cat, pri)},
    buf{buf},
    category{cat},
    priority{pri} {}

void stela::LogStream::operator<<(endlog_t) {
  #ifndef NDEBUG
  ended = true;
  #endif
  buf.endLog(category, priority);
  if (priority == LogPri::fatal_error) {
    throw FatalError{};
  }
}

stela::Log::Log(LogBuf &buf, const LogCat cat)
  : buf{buf}, category{cat} {}

stela::LogStream stela::Log::verbose(const Loc loc) {
  return log(LogPri::verbose, loc);
}

stela::LogStream stela::Log::info(const Loc loc) {
  return log(LogPri::info, loc);
}

stela::LogStream stela::Log::warn(const Loc loc) {
  return log(LogPri::warning, loc);
}

stela::LogStream stela::Log::error(const Loc loc) {
  return log(LogPri::error, loc);
}

stela::LogStream stela::Log::ferror(const Loc loc) {
  return log(LogPri::fatal_error, loc);
}

stela::LogStream stela::Log::log(const LogPri pri, const Loc loc) {
  buf.beginLog(category, pri, loc);
  return {buf, category, pri};
}

stela::LogStream stela::Log::verbose() {
  return log(LogPri::verbose);
}

stela::LogStream stela::Log::info() {
  return log(LogPri::info);
}

stela::LogStream stela::Log::warn() {
  return log(LogPri::warning);
}

stela::LogStream stela::Log::error() {
  return log(LogPri::error);
}

stela::LogStream stela::Log::ferror() {
  return log(LogPri::fatal_error);
}

stela::LogStream stela::Log::log(const LogPri pri) {
  buf.beginLog(category, pri);
  return {buf, category, pri};
}
