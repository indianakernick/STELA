//
//  log output.cpp
//  STELA
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#include "log output.hpp"

stela::LogStream::LogStream(LogSink &sink, const LogInfo &head)
  : stream{nullptr}, sink{sink}, head{head}, silent{!sink.writeHead(head)} {
  if (silent) {
    stream.rdbuf(silentBuf());
  } else {
    stream.rdbuf(sink.getBuf(head));
  }
}

void stela::LogStream::operator<<(endlog_t) {
  if (!silent) {
    sink.writeTail(head);
  }
}

void stela::LogStream::operator<<(fatal_t) {
  operator<<(endlog);
  throw FatalError{};
}

stela::Log::Log(LogSink &sink, const LogCat cat)
  : sink{sink}, cat{cat} {}

void stela::Log::module(const LogMod module) {
  mod = module;
}

/* LCOV_EXCL_START */
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
/* LCOV_EXCL_END */

stela::LogStream stela::Log::log(const LogPri pri, const Loc loc) {
  return {sink, {mod, loc, cat, pri}};
}

/* LCOV_EXCL_START */
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
/* LCOV_EXCL_END */

stela::LogStream stela::Log::log(const LogPri pri) {
  return {sink, {mod, Loc{}, cat, pri}};
}
