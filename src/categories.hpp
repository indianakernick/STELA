//
//  categories.hpp
//  STELA
//
//  Created by Indi Kernick on 28/12/18.
//  Copyright © 2018 Indi Kernick. All rights reserved.
//

#ifndef stela_categories_hpp
#define stela_categories_hpp

namespace stela {

namespace ast {

struct Type;
struct Expression;

}

enum class TypeCat {
  /// Primitive types like integers and floats.
  /// Default constructor just sets to 0. Destructor does nothing.
  /// Other special functions just copy.
  /// Only needs an address when used as an lvalue.
  trivially_copyable,
  
  // @TODO rework this
  // maybe we should just treat trivially relocatable types the same as
  // nontrivial types. We'll let the compile inline constructors and maybe
  // it will be able to figure out trivially relocatation.
  
  /// Arrays
  /// Requires calls to special functions but can be relocated.
  /// Moving and then destroying is the same as copying.
  /// Doesn't need an address when used as a prvalue.
  trivially_relocatable,
  
  /// Structs and some user types. Closures are too big to be trivially relocatable
  /// Requires calls to special functions.
  /// Passed to functions by pointer and returned by pointer.
  /// Must always have an address.
  nontrivial
};

TypeCat classifyType(ast::Type *);

// @TODO maybe add functions to check for glvalues and rvalues

enum class ValueCat {
  /// Has an address.
  /// Can be assigned to.
  /// Cannot be moved from.
  /// Cannot be relocated from.
  /// Destroyed at end of scope.
  /// Can initialize a reference.
  lvalue,
  
  /// Has an address.
  /// Cannot be assigned to.
  /// Can be moved from.
  /// Cannot be relocated from.
  /// Destroyed at end of expression.
  xvalue,
  
  /// Has an address only if type is nontrivial.
  /// Cannot be assigned to.
  /// Can be moved from.
  /// Can be relocated from.
  /// Destroyed after being relocated.
  prvalue
};

enum class ArithCat {
  /// Char, Sint
  signed_int,
  /// Bool, Byte, Uint
  unsigned_int,
  /// Real
  floating_point
};

ArithCat classifyArith(ast::Type *);
ArithCat classifyArith(ast::Expression *);

}

#endif
