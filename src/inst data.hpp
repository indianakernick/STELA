//
//  inst data.hpp
//  STELA
//
//  Created by Indi Kernick on 12/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#ifndef stela_inst_data_hpp
#define stela_inst_data_hpp

#include <cstddef>

namespace llvm {

class Module;
class Function;

}

namespace stela {

class FuncInst;

struct InstData {
  FuncInst &inst;
  llvm::Module *const mod;
};

namespace ast {

struct Type;
struct StructType;
struct ArrayType;

}

/// Function generator identifier
enum class FGI : size_t {
  ptr_dtor,
  ptr_cop_ctor,
  ptr_cop_asgn,
  ptr_mov_ctor,
  ptr_mov_asgn,
  
  panic,
  alloc,
  free,
  ceil_to_pow_2,
  
  count_
};

/// Parametized function generator identifier
enum class PFGI : size_t {
  arr_dtor,
  arr_def_ctor,
  arr_cop_ctor,
  arr_cop_asgn,
  arr_mov_ctor,
  arr_mov_asgn,
  arr_idx_s,
  arr_idx_u,
  arr_len_ctor,
  arr_strg_dtor,
  arr_eq,
  arr_lt,
  
  srt_dtor,
  srt_def_ctor,
  srt_cop_ctor,
  srt_cop_asgn,
  srt_mov_ctor,
  srt_mov_asgn,
  srt_eq,
  srt_lt,
  
  count_
};

template <FGI Fn>
llvm::Function *genFn(InstData);

template <PFGI Fn, typename Param>
llvm::Function *genFn(InstData, Param *);

}

#endif
