//
//  log output.hpp
//  STELA
//
//  Created by Indi Kernick on 23/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_log_output_hpp
#define stela_log_output_hpp

#include "log.hpp"
#include <cassert>
#include <iostream>

namespace stela {

struct endlog_t {};
constexpr endlog_t endlog {};

struct fatal_t {};
constexpr fatal_t fatal {};

class LogStream {
public:
  LogStream(LogSink &, const LogInfo &);

  void operator<<(endlog_t);
  [[noreturn]] void operator<<(fatal_t);
  
  template <typename T>
  [[nodiscard]] LogStream &operator<<(const T &value) {
    stream << value;
    return *this;
  }

private:
  std::ostream stream;
  LogSink &sink;
  LogInfo head;
  bool silent;
};

class Log {
public:
  Log(LogSink &, LogCat);
  
  void module(LogMod);
  
  LogStream verbose(Loc);
  LogStream status(Loc);
  LogStream info(Loc);
  LogStream warn(Loc);
  LogStream error(Loc);
  LogStream log(LogPri, Loc);
  
  LogStream verbose();
  LogStream status();
  LogStream info();
  LogStream warn();
  LogStream error();
  LogStream log(LogPri);
  
private:
  LogSink &sink;
  LogCat cat;
  LogMod mod;
};

}

#endif
