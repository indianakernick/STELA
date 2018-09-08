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
#include <experimental/optional>

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

struct Expression : Node {};
using ExprPtr = std::unique_ptr<Expression>;

struct Statement : Node {};
using StatPtr = std::unique_ptr<Statement>;

struct Declaration : Statement {};
using DeclPtr = std::unique_ptr<Declaration>;

struct Assignment : Statement {};
using AsgnPtr = std::unique_ptr<Assignment>;

struct Literal : Expression {};
using LitrPtr = std::unique_ptr<Literal>;

using Name = std::string_view;

//--------------------------------- Types --------------------------------------

struct BuiltinType final : Type {
  // not a strong enum
  // BuiltinType::Int instead of BuiltinType::TypeEnum::Int
  enum Enum {
    Char,
    Bool,
    Float,
    Double,
    String,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64
  } value;

  void accept(Visitor &) override;
};

struct ArrayType final : Type {
  TypePtr elem;
  
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

struct TypeAlias;

struct NamedType final : Type {
  Name name;
  TypeAlias *definition = nullptr;
  
  void accept(Visitor &) override;
};

struct NamespacedType final : Type {
  TypePtr parent;
  Name name;
  TypeAlias *definition = nullptr;
  
  void accept(Visitor &) override;
};

struct Field {
  Name name;
  TypePtr type;
};

struct StructType final : Type {
  std::vector<Field> fields;
  
  void accept(Visitor &) override;
};

//------------------------------ Expressions -----------------------------------

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
  neg, bool_not, bit_not
};

struct BinaryExpr final : Expression {
  ExprPtr left;
  BinOp oper;
  ExprPtr right;
  
  void accept(Visitor &) override;
};

struct UnaryExpr final : Expression {
  UnOp oper;
  ExprPtr expr;
  
  void accept(Visitor &) override;
};

using FuncArgs = std::vector<ExprPtr>;

struct Func;

struct FuncCall final : Expression {
  ExprPtr func;
  FuncArgs args;
  Func *definition = nullptr;
  
  void accept(Visitor &) override;
};

struct MemberIdent final : Expression {
  ExprPtr object;
  Name member;
  Field *definition = nullptr;
  
  void accept(Visitor &) override;
};

struct Subscript final : Expression {
  ExprPtr object;
  ExprPtr index;
  
  void accept(Visitor &) override;
};

struct Identifier final : Expression {
  Name name;
  // either DeclAssign, Var or Let
  // lowest common base is Statement
  // null if the identifier refers to a function
  // error if the identifier refers to a type
  Statement *definition = nullptr;
  
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

struct If final : Statement {
  ExprPtr cond;
  StatPtr body;
  StatPtr elseBody;
  
  void accept(Visitor &) override;
};

struct SwitchCase {
  ExprPtr expr;
  StatPtr body;
  Loc loc;
};

struct Switch final : Statement {
  ExprPtr expr;
  std::vector<SwitchCase> cases;
  
  void accept(Visitor &) override;
};

struct Break final : Statement {
  void accept(Visitor &) override;
};

struct Continue final : Statement {
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

struct For final : Statement {
  AsgnPtr init;
  ExprPtr cond;
  AsgnPtr incr;
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
  std::experimental::optional<FuncParam> receiver;
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
  bool strong;
  
  void accept(Visitor &) override;
};

//------------------------------- Assignments ----------------------------------

/// Assignment operator
enum class AssignOp {
  add, sub, mul, div, mod, pow,
  bit_or, bit_xor, bit_and, bit_shl, bit_shr
};

/// Compound assignment a *= 4
struct CompAssign final : Assignment {
  ExprPtr left;
  AssignOp oper;
  ExprPtr right;
  
  void accept(Visitor &) override;
};

/// Increment and decrement operators a++
struct IncrDecr final : Assignment {
  ExprPtr expr;
  bool incr;
  
  void accept(Visitor &) override;
};

/// Regular assignment a = 4
struct Assign final : Assignment {
  ExprPtr left;
  ExprPtr right;
  
  void accept(Visitor &) override;
};

/// Short-hand variable declaration a := 4
struct DeclAssign final : Assignment {
  Name name;
  ExprPtr expr;
  
  void accept(Visitor &) override;
};

/// A function call with a discarded return value a()
/// This allows function calls to be statements
struct CallAssign final : Assignment {
  FuncCall call;

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
  
  // types
  virtual void visit(BuiltinType &) {}
  virtual void visit(ArrayType &) {}
  virtual void visit(FuncType &) {}
  virtual void visit(NamedType &) {}
  virtual void visit(NamespacedType &) {}
  virtual void visit(StructType &) {}
  
  // expressions
  virtual void visit(BinaryExpr &) {}
  virtual void visit(UnaryExpr &) {}
  virtual void visit(FuncCall &) {}
  virtual void visit(MemberIdent &) {}
  virtual void visit(Subscript &) {}
  virtual void visit(Identifier &) {}
  virtual void visit(Ternary &) {}
  
  // statements
  virtual void visit(Block &) {}
  virtual void visit(If &) {}
  virtual void visit(Switch &) {}
  virtual void visit(Break &) {}
  virtual void visit(Continue &) {}
  virtual void visit(Return &) {}
  virtual void visit(While &) {}
  virtual void visit(For &) {}
  
  // declarations
  virtual void visit(Func &) {}
  virtual void visit(Var &) {}
  virtual void visit(Let &) {}
  virtual void visit(TypeAlias &) {}
  
  // assignments
  virtual void visit(CompAssign &) {}
  virtual void visit(IncrDecr &) {}
  virtual void visit(Assign &) {}
  virtual void visit(DeclAssign &) {}
  virtual void visit(CallAssign &) {}
  
  // literals
  virtual void visit(StringLiteral &) {}
  virtual void visit(CharLiteral &) {}
  virtual void visit(NumberLiteral &) {}
  virtual void visit(BoolLiteral &) {}
  virtual void visit(ArrayLiteral &) {}
  virtual void visit(Lambda &) {}
  
  /* LCOV_EXCL_END */
};

using Decls = std::vector<ast::DeclPtr>;

}

namespace stela {

struct AST {
  ast::Decls builtin;
  ast::Decls global;
};

}

#endif
