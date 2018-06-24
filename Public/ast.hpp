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

using Name = std::string_view;

//--------------------------------- Types --------------------------------------

struct Type : Node {};
using TypePtr = std::unique_ptr<Type>;

struct ArrayType final : Type {
  TypePtr elem;
};

struct DictType final : Type {
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

struct FuncType final : Type {
  std::vector<ParamType> params;
  TypePtr ret;
};

struct NamedType final : Type {
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

struct Expression : Node {};
using ExprPtr = std::unique_ptr<Expression>;

struct BinaryExpr final : Expression {
  ExprPtr left;
  BinOp oper;
  ExprPtr right;
};

struct UnaryExpr final : Expression {
  UnOp oper;
  ExprPtr expr;
};

struct AssignExpr final : Expression {
  ExprPtr left;
  AssignOp oper;
  ExprPtr right;
};

struct FuncArg {
  ExprPtr expr;
};
using FuncArgs = std::vector<FuncArg>;

struct FuncCall final : Expression {
  ExprPtr func;
  FuncArgs args;
};

struct ObjectMember final : Expression {
  ExprPtr object;
  Name name;
};

struct ConstructorCall final : Expression {
  TypePtr type;
  FuncArgs args;
};

struct Subscript final : Expression {
  ExprPtr object;
  ExprPtr index;
};

struct Identifier final : Expression {
  Name name;
};

struct Ternary final : Expression {
  ExprPtr cond;
  ExprPtr tru;
  ExprPtr fals;
};

//------------------------------- Statements -----------------------------------

struct Statement : Node {};
using StatPtr = std::unique_ptr<Statement>;

struct Block final : Statement {
  std::vector<StatPtr> nodes;
};

struct Var final : Statement {
  Name name;
  TypePtr type;
  ExprPtr expr;
};

struct Let final : Statement {
  Name name;
  TypePtr type;
  ExprPtr expr;
};

struct If final : Statement {
  ExprPtr cond;
  Block body;
  Block elseBody;
};

struct SwitchCase final : Statement {
  NodePtr expr;
};

struct SwitchDefault final : Statement {};

struct Switch final : Statement {
  NodePtr expr;
  // pointers point to SwitchCases in block
  std::vector<SwitchCase *> cases;
  SwitchDefault *def;
  Block body; // SwitchCases are in block
};

struct Break final : Statement {};
struct Continue final : Statement {};
struct Fallthrough final : Statement {};

struct Return final : Statement {
  NodePtr expr;
};

struct While final : Statement {
  ExprPtr cond;
  Block body;
};

struct RepeatWhile final : Statement {
  Block body;
  ExprPtr cond;
};

struct For final : Statement {
  ExprPtr init;
  ExprPtr cond;
  ExprPtr incr;
  Block body;
};

struct ForIn final : Statement {
  ExprPtr init;
  ExprPtr source;
  Block body;
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

//--------------------------- Type Declarations --------------------------------

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
  NodePtr node; // Var, Let, Func, Init
};

struct Init final : Node {
  FuncParams params;
  Block body;
};

struct TypeAlias final : Statement {
  Name name;
  TypePtr type;
};

struct Struct final : Statement {
  Name name;
  Block body;
};

struct EnumCase {
  Name name;
  NodePtr value;
};

struct Enum final : Statement {
  Name name;
  std::vector<EnumCase> cases;
};

//-------------------------------- Literals ------------------------------------

struct StringLiteral final : Expression {
  std::string_view value;
};

struct CharLiteral final : Expression {
  std::string_view value;
};

struct NumberLiteral final : Expression {
  std::string_view value;
};

struct ArrayLiteral final : Expression {
  std::vector<ExprPtr> exprs;
};

struct DictPair {
  ExprPtr key;
  ExprPtr val;
};

struct DictLiteral final : Expression {
  std::vector<DictPair> pairs;
};

struct Lambda final : Expression {
  FuncParams params;
  TypePtr ret;
  Block body;
};

}

namespace stela {

struct AST {
  std::vector<ast::NodePtr> topNodes;
};

}

#endif
