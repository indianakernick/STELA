//
//  error.hpp
//  STELA
//
//  Created by Indi Kernick on 22/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_error_hpp
#define stela_error_hpp

#include <exception>
#include "location.hpp"

namespace stela {

/// A base exception class for all exceptions thrown by STELA
class Error : public std::exception {
public:
  Error(const char *, Loc, const char *);
  
  Line line() const {
    return loc.l;
  }
  Col col() const {
    return loc.c;
  }
  
  const char *what() const noexcept override;
  
private:
  const char *type;
  const char *msg;
  Loc loc;
};

}

#endif
