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

struct Type : Node {};
using TypePtr = std::unique_ptr<Type>;

struct Statement : Node {};
using StatPtr = std::unique_ptr<Statement>;

// expressions are allowed where statements are allowed
struct Expression : Statement {};
using ExprPtr = std::unique_ptr<Expression>;

struct Declaration : Statement {};
using DeclPtr = std::unique_ptr<Declaration>;

using Name = std::string_view;

//--------------------------------- Types --------------------------------------

struct ArrayType final : Type {
  TypePtr elem;
};

struct DictType final : Type {
  TypePtr key;
  TypePtr val;
};

/// A function parameter can be passed by value or by reference (inout)
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

struct BinaryExpr final : Expression {
  ExprPtr left;
  BinOp oper;
  ExprPtr right;
};

struct UnaryExpr final : Expression {
  UnOp oper;
  ExprPtr expr;
};

struct Assignment final : Expression {
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

struct InitCall final : Expression {
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

struct Block final : Statement {
  std::vector<StatPtr> nodes;
};

struct EmptyStatement final : Statement {};

struct If final : Statement {
  ExprPtr cond;
  StatPtr body;
  StatPtr elseBody;
};

struct SwitchCase final : Statement {
  ExprPtr expr;
};

struct SwitchDefault final : Statement {};

struct Switch final : Statement {
  ExprPtr expr;
  Block body;
};

struct Break final : Statement {};
struct Continue final : Statement {};
struct Fallthrough final : Statement {};

struct Return final : Statement {
  ExprPtr expr;
};

struct While final : Statement {
  ExprPtr cond;
  StatPtr body;
};

struct RepeatWhile final : Statement {
  StatPtr body;
  ExprPtr cond;
};

struct For final : Statement {
  StatPtr init;
  ExprPtr cond;
  ExprPtr incr;
  StatPtr body;
};

//------------------------------ Declarations ----------------------------------

struct FuncParam {
  Name name;
  ParamRef ref;
  TypePtr type;
};
using FuncParams = std::vector<FuncParam>;

struct Func final : Declaration {
  Name name;
  FuncParams params;
  TypePtr ret;
  StatPtr body;
};

struct Var final : Declaration {
  Name name;
  TypePtr type;
  ExprPtr expr;
};

struct Let final : Declaration {
  Name name;
  TypePtr type;
  ExprPtr expr;
};

struct Init final : Declaration {
  FuncParams params;
  Block body;
};

struct TypeAlias final : Declaration {
  Name name;
  TypePtr type;
};

/// Access level of a member
enum class MemAccess {
  public_,
  private_
};

/// The scope of a member is either in the object or on the type
enum class MemScope {
  not_applicable, // neither a variable nor a function
  member,
  static_
};

struct Member final : Declaration {
  MemAccess access;
  MemScope scope;
  DeclPtr node;
};

struct Struct final : Declaration {
  Name name;
  std::vector<Member> body;
};

struct EnumCase final : Declaration {
  Name name;
  ExprPtr value;
};

struct Enum final : Declaration {
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
  std::vector<ast::StatPtr> global;
};

}

#endif
