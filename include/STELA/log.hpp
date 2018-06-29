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
#include "location.hpp"

namespace stela {

/// Logging category
enum class LogCat : uint8_t {
  lexical,
  syntax,
  semantic
};

/// Logging priority
enum class LogPri : uint8_t {
  verbose,
  info,
  warning,
  error,
  fatal_error,
  nothing
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
class LogBuf {
public:
  virtual ~LogBuf() = default;
  
  void pri(LogPri);
  
  void beginLog(LogCat, LogPri, Loc);
  void beginLog(LogCat, LogPri);
  // if the priority passed to endLog is fatal_error then throw FatalError
  void endLog(LogCat, LogPri);
  std::streambuf *getStreambuf(LogCat, LogPri);
  
private:
  virtual void begin(LogCat, LogPri, Loc) = 0;
  virtual void begin(LogCat, LogPri) = 0;
  virtual void end(LogCat, LogPri) = 0;
  virtual std::streambuf *getBuf(LogCat, LogPri) = 0;
  
  bool canLog(LogPri) const;
  
  LogPri priority = LogPri::info;
};

/// Write to a stream
class StreamLog final : public LogBuf {
public:
  /// Write to std::cerr
  StreamLog();
  /// Write to a stream
  explicit StreamLog(const std::ostream &);
  
private:
  std::streambuf *buf;
  
  std::streambuf *getBuf(LogCat, LogPri) override;
  void begin(LogCat, LogPri, Loc) override;
  void begin(LogCat, LogPri) override;
  void end(LogCat, LogPri) override;
};

/// Write colored text to stderr
class ColorLog final : public LogBuf {
  std::streambuf *getBuf(LogCat, LogPri) override;
  void begin(LogCat, LogPri, Loc) override;
  void begin(LogCat, LogPri) override;
  void end(LogCat, LogPri) override;
};

/// Do nothing
class NoLog final : public LogBuf {
  std::streambuf *getBuf(LogCat, LogPri) override;
  void begin(LogCat, LogPri, Loc) override;
  void begin(LogCat, LogPri) override;
  void end(LogCat, LogPri) override;
};

}

#endif