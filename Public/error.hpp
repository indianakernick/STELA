//
//  error.hpp
//  STELA
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_error_hpp
#define stela_error_hpp

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
  info,
  warning,
  error,
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

/// Pass a reference to a Logger to the compiler to get warnings and
/// errors. Pass an instance of NoLog if you don't want error reporting
class Logger {
public:
  Logger() = default;
  virtual ~Logger() = default;
  
  void cat(LogCat);
  void pri(LogPri);
  
  std::ostream &info(Loc);
  std::ostream &warn(Loc);
  std::ostream &error(Loc);
  std::ostream &log(LogPri, Loc);
  
  void endInfo();
  void endWarn();
  void endError();
  void end(LogPri);
  
private:
  virtual std::ostream &log(LogCat, LogPri, Loc) = 0;
  virtual void endLog(LogCat, LogPri) = 0;
  
  LogCat category = LogCat::lexical;
  LogPri priority = LogPri::nothing;
};

/// An implementation of Logger that simply writes errors to the given stream
class StreamLog final : public Logger {
public:
  /// Log errors to std::cerr
  StreamLog();
  /// Log errors to a stream
  explicit StreamLog(std::ostream &);
  
private:
  std::ostream &stream;
  
  std::ostream &log(LogCat, LogPri, Loc) override;
  void endLog(LogCat, LogPri) override;
};

/// Get a reference to a silent std::ostream. Used internally by NoLog
std::ostream &silentStream();

/// An implementation of Logger that does nothing
class NoLog final : public Logger {
public:
  NoLog() = default;

private:
  std::ostream &log(LogCat, LogPri, Loc) override;
  void endLog(LogCat, LogPri) override;
};

}

#endif
