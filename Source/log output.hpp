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
};

class Log {
public:
  explicit Log(LogBuf &);

  void cat(LogCat);
  
  LogStream info(Loc);
  LogStream warn(Loc);
  LogStream error(Loc);
  LogStream ferror(Loc);
  LogStream log(LogPri, Loc);
  
  LogStream info();
  LogStream warn();
  LogStream error();
  LogStream ferror();
  LogStream log(LogPri);
  
private:
  LogCat category = LogCat::lexical;
  LogBuf &buf;
};

}

#endif
