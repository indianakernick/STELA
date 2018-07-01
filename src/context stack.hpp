//
//  context stack.hpp
//  STELA
//
//  Created by Indi Kernick on 1/7/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_context_stack_hpp
#define stela_context_stack_hpp

#include <vector>
#include <iosfwd>
#include <string_view>

namespace stela {

class ContextStack;

class Context {
  friend ContextStack;
public:
  ~Context();
  
  void desc(std::string_view);
  void ident(std::string_view);
  
private:
  Context(ContextStack &, size_t);
  ContextStack &stack;
  size_t index;
};

class ContextStack {
  friend Context;
public:
  Context context(std::string_view);
  
private:
  struct ContextData {
    std::string_view desc;
    std::string_view ident;
  };
  
  std::vector<ContextData> stack;
  
  friend std::ostream &operator<<(std::ostream &, const ContextStack &);
};

}

#endif
