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
#include <memory>
#include <string_view>
#include "location.hpp"

namespace stela::sym {

struct Symbol;
struct Func;

}

namespace stela::ast {

class Visitor;

//---------------------------------- Base --------------------------------------

struct Node {
  virtual ~Node() = default;
  virtual void accept(Visitor &) = 0;
  
  Loc loc;
};
using NodePtr = std::unique_ptr<Node>;

struct Type : Node {};
using TypePtr = std::unique_ptr<Type>;

struct Statement : Node {};
using StatPtr = std::unique_ptr<Statement>;

struct Expression : Statement {};
using ExprPtr = std::unique_ptr<Expression>;

struct Literal : Expression {};
using LitrPtr = std::unique_ptr<Literal>;

struct Declaration : Statement {};
using DeclPtr = std::unique_ptr<Declaration>;

using Name = std::string_view;

//--------------------------------- Types --------------------------------------

struct ArrayType final : Type {
  TypePtr elem;
  
  void accept(Visitor &) override;
};

struct MapType final : Type {
  TypePtr key;
  TypePtr val;
  
  void accept(Visitor &) override;
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
  
  void accept(Visitor &) override;
};

struct NamedType final : Type {
  Name name;
  sym::Symbol *definition = nullptr;
  
  void accept(Visitor &) override;
};

struct NestedType final : Type {
  TypePtr parent;
  Name name;
  sym::Symbol *definition = nullptr;
  
  void accept(Visitor &) override;
};

//------------------------------ Expressions -----------------------------------

/// Assignment operator
enum class AssignOp {
  assign,
  add, sub, mul, div, mod, pow,
  bit_or, bit_xor, bit_and, bit_shl, bit_shr
};

/// Binary operator
enum class BinOp {
  bool_or, bool_and,
  bit_or, bit_xor, bit_and,
  eq, ne, lt, le, gt, ge,
  bit_shl, bit_shr,
  add, sub, mul, div, mod, pow
};

/// Unary operator
enum class UnOp {
  neg,
  bool_not, bit_not,
  pre_incr, pre_decr,
  post_incr, post_decr
};

struct Assignment final : Expression {
  ExprPtr left;
  AssignOp oper;
  ExprPtr right;
  sym::Func *definition = nullptr;
  
  void accept(Visitor &) override;
};

struct BinaryExpr final : Expression {
  ExprPtr left;
  BinOp oper;
  ExprPtr right;
  sym::Func *definition = nullptr;
  
  void accept(Visitor &) override;
};

struct UnaryExpr final : Expression {
  UnOp oper;
  ExprPtr expr;
  sym::Func *definition = nullptr;
  
  void accept(Visitor &) override;
};

using FuncArgs = std::vector<ExprPtr>;

struct FuncCall final : Expression {
  ExprPtr func;
  FuncArgs args;
  sym::Func *definition = nullptr;
  
  void accept(Visitor &) override;
};

struct MemberIdent final : Expression {
  ExprPtr object;
  Name member;
  sym::Symbol *definition = nullptr;
  
  void accept(Visitor &) override;
};

struct InitCall final : Expression {
  TypePtr type;
  FuncArgs args;
  sym::Func *definition = nullptr;
  
  void accept(Visitor &) override;
};

struct Subscript final : Expression {
  ExprPtr object;
  ExprPtr index;
  
  void accept(Visitor &) override;
};

struct Identifier final : Expression {
  Name name;
  sym::Symbol *definition = nullptr;
  
  void accept(Visitor &) override;
};

struct Self final : Expression {
  sym::Symbol *definition = nullptr;

  void accept(Visitor &) override;
};

struct Ternary final : Expression {
  ExprPtr cond;
  ExprPtr tru;
  ExprPtr fals;
  
  void accept(Visitor &) override;
};

//------------------------------- Statements -----------------------------------

struct Block final : Statement {
  std::vector<StatPtr> nodes;
  
  void accept(Visitor &) override;
};

struct EmptyStatement final : Statement {
  void accept(Visitor &) override;
};

struct If final : Statement {
  ExprPtr cond;
  StatPtr body;
  StatPtr elseBody;
  
  void accept(Visitor &) override;
};

struct SwitchCase final : Statement {
  ExprPtr expr;
  
  void accept(Visitor &) override;
};

struct SwitchDefault final : Statement {
  void accept(Visitor &) override;
};

struct Switch final : Statement {
  ExprPtr expr;
  Block body;
  
  void accept(Visitor &) override;
};

struct Break final : Statement {
  void accept(Visitor &) override;
};
struct Continue final : Statement {
  void accept(Visitor &) override;
};
struct Fallthrough final : Statement {
  void accept(Visitor &) override;
};

struct Return final : Statement {
  ExprPtr expr;
  
  void accept(Visitor &) override;
};

struct While final : Statement {
  ExprPtr cond;
  StatPtr body;
  
  void accept(Visitor &) override;
};

struct RepeatWhile final : Statement {
  StatPtr body;
  ExprPtr cond;
  
  void accept(Visitor &) override;
};

struct For final : Statement {
  StatPtr init;
  ExprPtr cond;
  ExprPtr incr;
  StatPtr body;
  
  void accept(Visitor &) override;
};

//------------------------------ Declarations ----------------------------------

struct FuncParam {
  Name name;
  ParamRef ref;
  TypePtr type;
  Loc loc;
};
using FuncParams = std::vector<FuncParam>;

struct Func final : Declaration {
  Name name;
  FuncParams params;
  TypePtr ret;
  Block body;
  
  void accept(Visitor &) override;
};

struct Var final : Declaration {
  Name name;
  TypePtr type;
  ExprPtr expr;
  
  void accept(Visitor &) override;
};

struct Let final : Declaration {
  Name name;
  TypePtr type;
  ExprPtr expr;
  
  void accept(Visitor &) override;
};

struct TypeAlias final : Declaration {
  Name name;
  TypePtr type;
  
  void accept(Visitor &) override;
};

struct Init final : Declaration {
  FuncParams params;
  Block body;
  
  void accept(Visitor &) override;
};

/// Access level of a member
enum class MemAccess {
  public_,
  private_,
  default_
};

/// The scope of a member is either in the object or on the type
enum class MemScope {
  member,
  static_
};

enum class MemMut {
  constant,
  mutating
};

struct Member {
  MemAccess access;
  MemScope scope;
  MemMut mut;
  DeclPtr node;
};

struct Struct final : Declaration {
  Name name;
  std::vector<Member> body;
  
  void accept(Visitor &) override;
};

struct EnumCase {
  Name name;
  ExprPtr value;
  Loc loc;
};

struct Enum final : Declaration {
  Name name;
  std::vector<EnumCase> cases;
  
  void accept(Visitor &) override;
};

//-------------------------------- Literals ------------------------------------

struct StringLiteral final : Literal {
  std::string_view value;
  
  void accept(Visitor &) override;
};

struct CharLiteral final : Literal {
  std::string_view value;
  
  void accept(Visitor &) override;
};

struct NumberLiteral final : Literal {
  std::string_view value;
  
  void accept(Visitor &) override;
};

struct BoolLiteral final : Literal {
  bool value;
  
  void accept(Visitor &) override;
};

struct ArrayLiteral final : Literal {
  std::vector<ExprPtr> exprs;
  
  void accept(Visitor &) override;
};

struct MapPair {
  ExprPtr key;
  ExprPtr val;
};

struct MapLiteral final : Literal {
  std::vector<MapPair> pairs;
  
  void accept(Visitor &) override;
};

struct Lambda final : Literal {
  FuncParams params;
  TypePtr ret;
  Block body;
  
  void accept(Visitor &) override;
};

//--------------------------------- Visitor ------------------------------------

class Visitor {
public:
  virtual ~Visitor() = default;
  
  /* LCOV_EXCL_START */
  
  virtual void visit(ArrayType &) {}
  virtual void visit(MapType &) {}
  virtual void visit(FuncType &) {}
  virtual void visit(NamedType &) {}
  virtual void visit(NestedType &) {}
  
  virtual void visit(Assignment &) {}
  virtual void visit(BinaryExpr &) {}
  virtual void visit(UnaryExpr &) {}
  virtual void visit(FuncCall &) {}
  virtual void visit(MemberIdent &) {}
  virtual void visit(InitCall &) {}
  virtual void visit(Subscript &) {}
  virtual void visit(Identifier &) {}
  virtual void visit(Self &) {}
  virtual void visit(Ternary &) {}
  
  virtual void visit(Block &) {}
  virtual void visit(EmptyStatement &) {}
  virtual void visit(If &) {}
  virtual void visit(SwitchCase &) {}
  virtual void visit(SwitchDefault &) {}
  virtual void visit(Switch &) {}
  virtual void visit(Break &) {}
  virtual void visit(Continue &) {}
  virtual void visit(Fallthrough &) {}
  virtual void visit(Return &) {}
  virtual void visit(While &) {}
  virtual void visit(RepeatWhile &) {}
  virtual void visit(For &) {}
  
  virtual void visit(Func &) {}
  virtual void visit(Var &) {}
  virtual void visit(Let &) {}
  virtual void visit(TypeAlias &) {}
  virtual void visit(Init &) {}
  virtual void visit(Struct &) {}
  virtual void visit(Enum &) {}
  
  virtual void visit(StringLiteral &) {}
  virtual void visit(CharLiteral &) {}
  virtual void visit(NumberLiteral &) {}
  virtual void visit(BoolLiteral &) {}
  virtual void visit(ArrayLiteral &) {}
  virtual void visit(MapLiteral &) {}
  virtual void visit(Lambda &) {}
  
  /* LCOV_EXCL_END */
};

}

namespace stela {

struct AST {
  std::vector<ast::DeclPtr> global;
};

}

#endif
