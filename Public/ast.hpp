//
//  ast.hpp
//  STELA
//
//  Created by Indi Kernick on 21/6/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_ast_hpp
#define stela_ast_hpp

#include <vector>
#include <string_view>

namespace stela::ast {

struct Type {
  std::string_view name;
};
struct ParamIndex { size_t i; };
struct BlockIndex { size_t i; };

struct Function {
  std::vector<ParamIndex> params;
  Type ret;
  BlockIndex body;
};

struct Param {
  
};

}

namespace stela {

class AST {
public:
  
};

}

#endif
