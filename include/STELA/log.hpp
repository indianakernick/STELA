//
//  log.hpp
//  STELA
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_log_hpp
#define stela_log_hpp

#include <iosfwd>
#include <exception>
#include <string_view>
#include "location.hpp"

namespace stela {

/// Logging category
enum class LogCat : uint8_t {
  lexical,
  syntax,
  semantic,
  generate
};

/// Logging priority
enum class LogPri : uint8_t {
  verbose,
  status,
  info,
  warning,
  error,
  nothing
};

/// Logging module
using LogMod = std::string_view;

struct LogInfo {
  LogMod mod;
  Loc loc;
  LogCat cat;
  LogPri pri;
};

std::ostream &operator<<(std::ostream &, LogCat);
std::ostream &operator<<(std::ostream &, LogPri);
std::ostream &operator<<(std::ostream &, Loc);

/// An exception that is thrown when a fatal error occurs
class FatalError : public std::exception {
public:
  const char *what() const noexcept override;
};

/// Get a reference to a silent output stream buf
std::streambuf *silentBuf();

/// Controls the output of the logger
class LogSink {
public:
  virtual ~LogSink();
  
  virtual bool writeHead(const LogInfo &) = 0;
  virtual std::streambuf *getBuf(const LogInfo &) = 0;
  virtual void writeTail(const LogInfo &) = 0;
};

/// Write to a stream
class StreamSink final : public LogSink {
public:
  /// Write to std::cerr
  StreamSink();
  /// Write to a stream
  explicit StreamSink(const std::ostream &);
  
  bool writeHead(const LogInfo &) override;
  std::streambuf *getBuf(const LogInfo &) override;
  void writeTail(const LogInfo &) override;
  
private:
  std::streambuf *buf;
};

/// Write colored text to stderr
class ColorSink final : public LogSink {
public:
  bool writeHead(const LogInfo &) override;
  std::streambuf *getBuf(const LogInfo &) override;
  void writeTail(const LogInfo &) override;
};

/// Do nothing
class NullSink final : public LogSink {
public:
  bool writeHead(const LogInfo &) override;
  std::streambuf *getBuf(const LogInfo &) override;
  void writeTail(const LogInfo &) override;
};

class FilterSink final : public LogSink {
public:
  FilterSink(LogSink &, LogPri);
  
  void pri(LogPri);
  
  bool writeHead(const LogInfo &) override;
  std::streambuf *getBuf(const LogInfo &) override;
  void writeTail(const LogInfo &) override;

private:
  LogSink &child;
  LogPri priority;
};

}

#endif
