#ifndef BDDD_AST_H
#define BDDD_AST_H

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

/* operators defined in SysY, also used as type indicator of ExprAST instance */
enum class Op {
  // Arithmetic unary operators
  POSITIVE,
  NEGATIVE,

  // Arithmetic binary operators
  PLUS,
  MINUS,
  MULTI,
  DIV,
  MOD,

  // relational binary operators
  LE,
  LEQ,
  GE,
  GEQ,
  EQ,
  NEQ,

  // Logical binary operators
  AND,
  OR,

  // Logical unary operators
  NOT,

  // Special indicator
  CONST_INT,
  CONST_FLOAT,
  LVAL,
  FuncCall,
};

enum class VarType {
  INT,
  FLOAT,
  VOID,
};

class Node {
public:
  virtual ~Node() = default;
};

class StmtAST : public Node {};

class DeclAST;

class ExprAST;

class InitVal : public Node {
public:
  std::unique_ptr<ExprAST> expr;
  std::vector<std::unique_ptr<InitVal>> vals;
  explicit InitVal() : expr(nullptr), vals() {}

  explicit InitVal(std::unique_ptr<ExprAST> expr) : expr(std::move(expr)), vals() {}

  explicit InitVal(std::unique_ptr<InitVal> val) : expr(nullptr), vals() { vals.push_back(std::move(val)); }

  void appendVal(std::unique_ptr<InitVal> val) { vals.push_back(std::move(val)); }
};

class LVal : public Node {
public:
  std::unique_ptr<DeclAST> decl;
  std::string name;
  std::vector<std::unique_ptr<ExprAST>> dimensions;

  explicit LVal(std::string name) : name(std::move(name)), decl(nullptr), dimensions() {}

  explicit LVal(std::unique_ptr<DeclAST> decl) : decl(std::move(decl)), name(), dimensions() {}

  void addDimension(int x);

  void addDimension(std::unique_ptr<ExprAST> expr);
};

class FuncCall;

class ExprAST : public Node {
public:
  enum class EvalType { INT, FLOAT, BOOL, ERROR };

  Op op;
  std::unique_ptr<ExprAST> lhs;
  std::unique_ptr<ExprAST> rhs;
  std::unique_ptr<FuncCall> func_call;
  int int_val;
  float float_val;
  std::unique_ptr<LVal> lval;

  explicit ExprAST(Op op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs = nullptr)
      : op(op),
        lhs(std::move(lhs)),
        rhs(std::move(rhs)),
        func_call(nullptr),
        int_val(0),
        float_val(0.0),
        lval(nullptr) {}

  explicit ExprAST(std::unique_ptr<FuncCall> func_call)
      : op(Op::FuncCall),
        lhs(nullptr),
        rhs(nullptr),
        func_call(std::move(func_call)),
        int_val(0),
        float_val(0.0),
        lval(nullptr) {}

  explicit ExprAST(int val)
      : op(Op::CONST_INT),
        lhs(nullptr),
        rhs(nullptr),
        func_call(nullptr),
        int_val(val),
        float_val(0.0),
        lval(nullptr) {}

  explicit ExprAST(float val)
      : op(Op::CONST_FLOAT),
        lhs(nullptr),
        rhs(nullptr),
        func_call(nullptr),
        int_val(0),
        float_val(val),
        lval(nullptr) {}

  explicit ExprAST(std::unique_ptr<LVal> lval)
      : op(Op::LVAL),
        lhs(nullptr),
        rhs(nullptr),
        func_call(nullptr),
        int_val(0),
        float_val(0.0),
        lval(std::move(lval)) {}
};

class DeclAST : public Node {
public:
  bool is_const;
  VarType type;
  std::string varname;
  std::unique_ptr<InitVal> initval;
  std::vector<std::unique_ptr<ExprAST>> dimensions;
  explicit DeclAST(std::string varname, std::unique_ptr<InitVal> initval = nullptr)
      : is_const(false), type(VarType::INT), varname(std::move(varname)), initval(std::move(initval)) {}

  void addDimension(std::unique_ptr<ExprAST> expr);
};

class FuncCall : public Node {
public:
  std::string callname;
  std::vector<std::unique_ptr<ExprAST>> params;

  explicit FuncCall(std::string callname) : callname(std::move(callname)), params() {}
};

class Cond : public Node {
public:
  std::unique_ptr<ExprAST> expr;

  explicit Cond(std::unique_ptr<ExprAST> expr = nullptr) : expr(std::move(expr)) {}
};

class FuncFParam : public Node {
public:
  VarType type;
  std::string name;
  std::vector<std::unique_ptr<ExprAST>> dimensions;

  explicit FuncFParam(VarType type, std::string name) : type(type), name(std::move(name)), dimensions() {}

  void addDimension(int x);

  void addDimension(std::unique_ptr<ExprAST> expr);
};

class BlockAST : public StmtAST {
public:
  std::vector<std::unique_ptr<Node>> nodes;

  void appendNodes(std::vector<std::unique_ptr<Node>> appended_nodes);
};

class FuncDefAST : public Node {
public:
  VarType returnType;
  std::string funcName;
  std::vector<std::unique_ptr<FuncFParam>> fParams;
  std::unique_ptr<BlockAST> block;

  explicit FuncDefAST(VarType returnType, std::string funcName, std::vector<std::unique_ptr<FuncFParam>> fParams,
                      std::unique_ptr<BlockAST> block)
      : returnType(returnType), funcName(std::move(funcName)), fParams(std::move(fParams)), block(std::move(block)) {}

  explicit FuncDefAST(VarType returnType, std::string funcName, std::unique_ptr<BlockAST> block)
      : returnType(returnType), funcName(std::move(funcName)), fParams(), block(std::move(block)) {}
};

class AssignStmtAST : public StmtAST {
public:
  std::unique_ptr<LVal> lval;
  std::unique_ptr<ExprAST> rhs;

  explicit AssignStmtAST(std::unique_ptr<LVal> lval, std::unique_ptr<ExprAST> rhs)
      : lval(std::move(lval)), rhs(std::move(rhs)) {}
};

class EvalStmtAST : public StmtAST {
public:
  std::unique_ptr<ExprAST> expr;

  explicit EvalStmtAST(std::unique_ptr<ExprAST> expr = nullptr) : expr(std::move(expr)) {}
};

class IfStmtAST : public StmtAST {
public:
  std::unique_ptr<Cond> cond;
  std::unique_ptr<StmtAST> then_stmt;
  std::unique_ptr<StmtAST> else_stmt;

  explicit IfStmtAST(std::unique_ptr<Cond> cond, std::unique_ptr<StmtAST> then_stmt,
                     std::unique_ptr<StmtAST> else_stmt = nullptr)
      : cond(std::move(cond)), then_stmt(std::move(then_stmt)), else_stmt(std::move(else_stmt)) {}
};

class ReturnStmtAST : public StmtAST {
public:
  std::unique_ptr<ExprAST> ret;

  explicit ReturnStmtAST(std::unique_ptr<ExprAST> ret = nullptr) : ret(std::move(ret)) {}
};

class WhileStmtAST : public StmtAST {
public:
  std::unique_ptr<Cond> cond;
  std::unique_ptr<StmtAST> stmt;

  explicit WhileStmtAST(std::unique_ptr<Cond> cond, std::unique_ptr<StmtAST> stmt)
      : cond(std::move(cond)), stmt(std::move(stmt)) {}
};

class BreakStmtAST : public StmtAST {};

class ContinueStmtAST : public StmtAST {};

class CompUnit : public Node {
public:
  std::vector<std::unique_ptr<Node>> nodes;
  void appendDecls(std::vector<std::unique_ptr<DeclAST>> decls);

  void appendFuncDef(std::unique_ptr<FuncDefAST> funcDef);
};

#endif  // BDDD_AST_H