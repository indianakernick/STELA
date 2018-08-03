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
  LogStream(LogBuf &, LogCat, LogPri);

  void operator<<(endlog_t);
  [[noreturn]] void operator<<(fatal_t);
  
  template <typename T>
  [[nodiscard]] LogStream &operator<<(const T &value) {
    stream << value;
    return *this;
  }

private:
  std::ostream stream;
  LogBuf &buf;
  LogCat category;
  LogPri priority;
};

class Log {
public:
  Log(LogBuf &, LogCat);
  
  LogStream verbose(Loc);
  LogStream info(Loc);
  LogStream warn(Loc);
  LogStream error(Loc);
  LogStream log(LogPri, Loc);
  
  LogStream verbose();
  LogStream info();
  LogStream warn();
  LogStream error();
  LogStream log(LogPri);
  
private:
  LogBuf &buf;
  LogCat category;
};

}

#endif
