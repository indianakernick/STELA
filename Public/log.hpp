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

/// A logging class. The compiler writes logs to a user provided logging class
class Log {
public:
  Log() = default;
  virtual ~Log() = default;
  
  void cat(LogCat);
  void pri(LogPri);
  
  std::ostream &info(Loc);
  std::ostream &warn(Loc);
  std::ostream &error(Loc);
  std::ostream &log(LogPri, Loc);
  
  std::ostream &info();
  std::ostream &warn();
  std::ostream &error();
  std::ostream &log(LogPri);
  
  void endInfo();
  void endWarn();
  void endError();
  void end(LogPri);
  
private:
  LogCat category = LogCat::lexical;
  LogPri priority = LogPri::nothing;

  virtual std::ostream &log(LogCat, LogPri, Loc) = 0;
  virtual std::ostream &log(LogCat, LogPri) = 0;
  virtual void endLog(LogCat, LogPri) = 0;
  
  bool canLog(LogPri) const;
};

/// A logger that simply writes errors to the given stream
class StreamLog final : public Log {
public:
  /// Log errors to std::cerr
  StreamLog();
  /// Log errors to a stream
  explicit StreamLog(std::ostream &);
  
private:
  std::ostream &stream;
  
  std::ostream &log(LogCat, LogPri, Loc) override;
  std::ostream &log(LogCat, LogPri) override;
  void endLog(LogCat, LogPri) override;
};

/// Get a reference to a silent std::ostream. Used internally by NoLog
std::ostream &silentStream();

/// A logger that does nothing
class NoLog final : public Log {
public:
  NoLog() = default;
  
private:
  std::ostream &log(LogCat, LogPri, Loc) override;
  std::ostream &log(LogCat, LogPri) override;
  void endLog(LogCat, LogPri) override;
};

}

#endif
