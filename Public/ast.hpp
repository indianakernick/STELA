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
#include "location.hpp"

namespace stela::ast {

//---------------------------------- Base --------------------------------------

struct Node {
  virtual ~Node() = default;
  
  Loc loc;
};
using NodePtr = std::unique_ptr<Node>;

struct Block final : Node {
  std::vector<NodePtr> nodes;
};

using Name = std::string_view;

//--------------------------------- Types --------------------------------------

struct Type : Node {};
using TypePtr = std::unique_ptr<Type>;

struct ArrayType final : public Type {
  TypePtr elem;
};

struct DictionaryType final : public Type {
  TypePtr key;
  TypePtr val;
};

enum class ParamRef {
  value,
  inout
};

struct ParamType {
  ParamRef ref;
  TypePtr type;
};

struct FuncType final : public Type {
  std::vector<ParamType> params;
  TypePtr ret;
};

struct NamedType final : public Type {
  Name name;
};

//------------------------------ Expressions -----------------------------------

/// Binary operator
enum class BinOp {
  eq, ne, ls, ge, lt, gt,
  bool_and, bool_or,
  bit_and, bit_or, bit_xor, bit_shl, bit_shr,
  add, sub, mul, div, mod
};

/// Unary operator
enum class UnOp {
  bool_not, bit_not,
  pre_incr, pre_decr,
  post_incr, post_decr,
  neg
};

/// Assignment operator
enum class AssignOp {
  add, sub, mul, div, mod,
  bit_and, bit_or, bit_xor, bit_shl, bit_shr,
  assign,
};

struct BinaryExpr final : public Node {
  NodePtr left;
  BinOp oper;
  NodePtr right;
};

struct UnaryExpr final : public Node {
  UnOp oper;
  NodePtr expr;
};

struct AssignExpr final : public Node {
  NodePtr left;
  AssignOp oper;
  NodePtr right;
};

struct FuncArg {
  NodePtr expr;
};
using FuncArgs = std::vector<FuncArg>;

struct FuncCall final : public Node {
  NodePtr func;
  FuncArgs args;
};

struct ObjectMember final : public Node {
  NodePtr object;
  Name name;
};

struct ConstructorCall final : public Node {
  TypePtr type;
  FuncArgs args;
};

struct Subscript final : public Node {
  NodePtr object;
  NodePtr index;
};

struct Identifier final : public Node {
  Name name;
};

//------------------------------- Functions ------------------------------------

struct FuncParam {
  Name name;
  ParamRef ref;
  TypePtr type;
};
using FuncParams = std::vector<FuncParam>;

struct Func final : Node {
  Name name;
  FuncParams params;
  TypePtr ret;
  Block body;
};

struct Return final : Node {
  NodePtr expr;
};

//-------------------------------- Literals ------------------------------------

struct StringLiteral final : public Node {
  std::string_view value;
};

struct CharLiteral final : public Node {
  std::string_view value;
};

struct NumberLiteral final : public Node {
  std::string_view value;
};

struct ArrayLiteral final : public Node {
  std::vector<NodePtr> exprs;
};

struct DictPair {
  NodePtr key;
  NodePtr val;
};

struct DictLiteral final : public Node {
  std::vector<DictPair> pairs;
};

struct Lambda final : public Node {
  FuncParams params;
  TypePtr ret;
  Block body;
};

//---------------------------------- Data --------------------------------------

enum class MemAccess {
  pub,
  priv
};

enum class MemScope {
  member,
  stat
};

struct Member final : Node {
  MemAccess access;
  MemScope scope;
  NodePtr node; // Variable, Constant, Function, Init
};

struct Variable final : Node {
  Name name;
  TypePtr type;
  NodePtr expr;
};

struct Constant final : Node {
  Name name;
  TypePtr type;
  NodePtr expr;
};

struct Init final : Node {
  FuncParams params;
  Block body;
};

struct TypeAlias final : public Node {
  Name name;
  TypePtr type;
};

struct Struct final : Node {
  Name name;
  Block body;
};

struct EnumCase {
  Name name;
  NodePtr value;
};

struct Enum final : Node {
  Name name;
  std::vector<EnumCase> cases;
};

//------------------------------ Control flow ----------------------------------

struct If final : Node {
  NodePtr cond;
  Block body;
};

struct Else final : Node {
  Block body;
};

struct Ternary final : Node {
  NodePtr cond;
  NodePtr tru;
  NodePtr fals;
};

struct SwitchCase {
  NodePtr expr;
  Block body;
};

struct Switch final : Node {
  NodePtr expr;
  std::vector<SwitchCase> cases;
  Block def;
};

struct Break final : Node {};
struct Continue final : Node {};
struct Fallthrough final : Node {};

struct While final : Node {
  NodePtr cond;
  Block body;
};

struct RepeatWhile final : Node {
  Block body;
  NodePtr cond;
};

struct For final : Node {
  NodePtr init;
  NodePtr cond;
  NodePtr incr;
  Block body;
};

struct ForIn final : Node {
  NodePtr init;
  NodePtr source;
  Block body;
};

}

namespace stela {

struct AST {
  std::vector<ast::NodePtr> topNodes;
};

}

#endif
