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

//---------------------------------- Base --------------------------------------

struct Node {
  virtual ~Node() = default;
};
using NodePtr = std::unique_ptr<Node>;

struct Block final : Node {
  std::vector<NodePtr> nodes;
};

using Name = std::string_view;

using Modifiers = std::vector<Name>;

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

struct FunctionType final : public Type {
  std::vector<TypePtr> params;
  TypePtr ret;
};

struct NamedType final : public Type {
  Name name;
};

//------------------------------ Expressions -----------------------------------

struct BinaryOper final : public Node {
  NodePtr left;
  std::string_view oper;
  NodePtr right;
};

struct UnaryOper final : public Node {
  std::string_view oper;
  NodePtr expr;
};

struct FunctionArg {
  NodePtr expr;
};
using FunctionArgs = std::vector<FunctionArg>;

struct FunctionCall final : public Node {
  NodePtr object;
  Name name;
  FunctionArgs args;
};

struct ConstructorCall final : public Node {
  TypePtr type;
  FunctionArgs args;
};

//-------------------------------- Literals ------------------------------------

struct Literal : Node {};
using LiteralPtr = std::unique_ptr<Literal>;

struct StringLiteral final : public Literal {
  std::string_view value;
};

struct CharLiteral final : public Literal {
  std::string_view value;
};

struct NumberLiteral final : public Literal {
  std::string_view value;
};

struct ArrayLiteral final : public Literal {
  std::vector<NodePtr> exprs;
};

struct DictPair {
  NodePtr key;
  NodePtr val;
};

struct DictionaryLiteral final : public Literal {
  std::vector<DictPair> pairs;
};

//------------------------------- Functions ------------------------------------

struct FunctionParam {
  Name name;
  Modifiers mods;
  TypePtr type;
};
using FunctionParams = std::vector<FunctionParam>;

struct Function final : Node {
  Modifiers mods;
  Name name;
  FunctionParams params;
  TypePtr ret;
  Block body;
};

struct Return final : Node {
  NodePtr expr;
};

//---------------------------------- Data --------------------------------------

struct Variable final : Node {
  Modifiers mods;
  Name name;
  TypePtr type;
  NodePtr expr;
};

struct Class final : Node {
  Modifiers mods;
  Name name;
  NamedType base;
  Block body;
};

struct Init final : Node {
  FunctionParams params;
  Block body;
};

struct Deinit final : Node {
  Block body;
};

struct Struct final : Node {
  Modifiers mods;
  Name name;
  Block body;
};

struct EnumCase {
  Name name;
  NodePtr value;
};

struct Enum final : Node {
  Modifiers mods;
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

struct While final : Node {
  NodePtr expr;
  Block body;
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
  std::vector<ast::NodePtr> top;
};

}

#endif
