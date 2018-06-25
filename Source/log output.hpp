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
#include <iostream>

namespace stela {

struct endlog_t {};
constexpr endlog_t endlog {};

class LogStream final : public std::ostream {
public:
  LogStream(LogBuf &, LogCat, LogPri);
  
  #ifndef NDEBUG
  ~LogStream() {
    // must << endlog before stream is destroyed
    assert(ended);
  }
  #endif

  void operator<<(endlog_t);
  
  template <typename T>
  LogStream &operator<<(const T &value) {
    *static_cast<std::ostream *>(this) << value;
    return *this;
  }

private:
  LogBuf &buf;
  LogCat category;
  LogPri priority;
  
  #ifndef NDEBUG
  bool ended = false;
  #endif
};

class Log {
public:
  Log(LogBuf &, LogCat);
  
  LogStream verbose(Loc);
  LogStream info(Loc);
  LogStream warn(Loc);
  LogStream error(Loc);
  LogStream ferror(Loc);
  LogStream log(LogPri, Loc);
  
  LogStream verbose();
  LogStream info();
  LogStream warn();
  LogStream error();
  LogStream ferror();
  LogStream log(LogPri);
  
private:
  LogBuf &buf;
  LogCat category;
};

}

#endif
