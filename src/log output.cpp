//
//  log output.cpp
//  STELA
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "log output.hpp"

stela::LogStream::LogStream(LogBuf &buf, const LogCat cat, const LogPri pri)
  : stream{buf.getStreambuf(cat, pri)},
    buf{buf},
    category{cat},
    priority{pri} {}

void stela::LogStream::operator<<(endlog_t) {
  buf.endLog(category, priority);
}

void stela::LogStream::operator<<(fatal_t) {
  operator<<(endlog);
  throw FatalError{};
}

stela::Log::Log(LogBuf &buf, const LogCat cat)
  : buf{buf}, cat{cat} {}

void stela::Log::module(const LogMod module) {
  mod = module;
}

stela::LogStream stela::Log::verbose(const Loc loc) {
  return log(LogPri::verbose, loc);
}

stela::LogStream stela::Log::status(const Loc loc) {
  return log(LogPri::status, loc);
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

stela::LogStream stela::Log::log(const LogPri pri, const Loc loc) {
  buf.beginLog(cat, pri, mod, loc);
  return {buf, cat, pri};
}

stela::LogStream stela::Log::verbose() {
  return log(LogPri::verbose);
}

stela::LogStream stela::Log::status() {
  return log(LogPri::status);
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

stela::LogStream stela::Log::log(const LogPri pri) {
  buf.beginLog(cat, pri, mod);
  return {buf, cat, pri};
}
