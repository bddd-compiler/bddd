#ifndef BDDD_AST_H
#define BDDD_AST_H

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

/* operators defined in SysY, also used as var_type indicator of ExprAST
 * instance
 */
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
  UNKNOWN,
};

class Node {
public:
  virtual ~Node() = default;
};

class StmtAST : public Node {};

class DeclAST;

class ExprAST;

class InitVal : public Node {
private:
  std::unique_ptr<ExprAST> expr;
  std::vector<std::unique_ptr<InitVal>> vals;

public:
  explicit InitVal() : expr(nullptr), vals() {}

  explicit InitVal(std::unique_ptr<ExprAST> expr)
      : expr(std::move(expr)), vals() {}

  explicit InitVal(std::unique_ptr<InitVal> val) : expr(nullptr), vals() {
    vals.push_back(std::move(val));
  }

  void AppendVal(std::unique_ptr<InitVal> val) {
    vals.push_back(std::move(val));
  }
};

class LVal : public Node {
private:
  std::unique_ptr<DeclAST> decl;
  std::string name;
  std::vector<std::unique_ptr<ExprAST>> dimensions;

public:
  explicit LVal(std::string name)
      : name(std::move(name)), decl(nullptr), dimensions() {}

  explicit LVal(std::unique_ptr<DeclAST> decl)
      : decl(std::move(decl)), name(), dimensions() {}

  void AddDimension(int x);

  void AddDimension(std::unique_ptr<ExprAST> expr);
};

class FuncCall;

class ExprAST : public Node {
private:
  Op op;
  std::unique_ptr<ExprAST> lhs;
  std::unique_ptr<ExprAST> rhs;
  std::unique_ptr<FuncCall> func_call;
  int int_val;
  float float_val;
  std::unique_ptr<LVal> lval;

public:
  explicit ExprAST(Op op, std::unique_ptr<ExprAST> lhs,
                   std::unique_ptr<ExprAST> rhs = nullptr)
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
private:
  bool is_const;
  VarType var_type;

public:
  void setIsConst(bool isConst);
  void setVarType(VarType varType);

private:
  std::string varname;
  std::unique_ptr<InitVal> init_val;

public:
  void setInitVal(std::unique_ptr<InitVal> initVal);

private:
  std::vector<std::unique_ptr<ExprAST>> dimensions;

public:
  explicit DeclAST(std::string varname,
                   std::unique_ptr<InitVal> initval = nullptr)
      : is_const(false),
        var_type(VarType::UNKNOWN),
        varname(std::move(varname)),
        init_val(std::move(initval)) {}

  void AddDimension(std::unique_ptr<ExprAST> expr);
};

class FuncCall : public Node {
private:
  std::string callname;
  std::vector<std::unique_ptr<ExprAST>> params;

public:
  explicit FuncCall(std::string callname)
      : callname(std::move(callname)), params() {}

  void assignParams(std::vector<std::unique_ptr<ExprAST>> params) {
    this->params.assign(std::make_move_iterator(params.begin()),
                        std::make_move_iterator(params.end()));
    params.clear();
  }
};

class Cond : public Node {
private:
  std::unique_ptr<ExprAST> expr;

public:
  explicit Cond(std::unique_ptr<ExprAST> expr = nullptr)
      : expr(std::move(expr)) {}
};

class FuncFParam : public Node {
private:
  VarType type;
  std::string name;
  std::vector<std::unique_ptr<ExprAST>> dimensions;

public:
  explicit FuncFParam(VarType type, std::string name)
      : type(type), name(std::move(name)), dimensions() {}

  void AddDimension(int x);

  void AddDimension(std::unique_ptr<ExprAST> expr);
};

class BlockAST : public StmtAST {
private:
  std::vector<std::unique_ptr<Node>> nodes;

public:
  void AppendNodes(std::vector<std::unique_ptr<Node>> appendedNodes);
};

class FuncDefAST : public Node {
private:
  VarType return_type;
  std::string func_name;
  std::vector<std::unique_ptr<FuncFParam>> params;
  std::unique_ptr<BlockAST> block;

public:
  explicit FuncDefAST(VarType return_type, std::string func_name,
                      std::vector<std::unique_ptr<FuncFParam>> params,
                      std::unique_ptr<BlockAST> block)
      : return_type(return_type),
        func_name(std::move(func_name)),
        params(std::move(params)),
        block(std::move(block)) {}

  explicit FuncDefAST(VarType return_type, std::string func_name,
                      std::unique_ptr<BlockAST> block)
      : return_type(return_type),
        func_name(std::move(func_name)),
        params(),
        block(std::move(block)) {}

  void assignParams(std::vector<std::unique_ptr<FuncFParam>> params) {
    this->params.assign(std::make_move_iterator(params.begin()),
                        std::make_move_iterator(params.end()));
    params.clear();
  }
};

class AssignStmtAST : public StmtAST {
private:
  std::unique_ptr<LVal> lval;
  std::unique_ptr<ExprAST> rhs;

public:
  explicit AssignStmtAST(std::unique_ptr<LVal> lval,
                         std::unique_ptr<ExprAST> rhs)
      : lval(std::move(lval)), rhs(std::move(rhs)) {}
};

class EvalStmtAST : public StmtAST {
private:
  std::unique_ptr<ExprAST> expr;

public:
  explicit EvalStmtAST(std::unique_ptr<ExprAST> expr = nullptr)
      : expr(std::move(expr)) {}
};

class IfStmtAST : public StmtAST {
private:
  std::unique_ptr<Cond> cond;
  std::unique_ptr<StmtAST> then_stmt;
  std::unique_ptr<StmtAST> else_stmt;

public:
  explicit IfStmtAST(std::unique_ptr<Cond> cond,
                     std::unique_ptr<StmtAST> then_stmt,
                     std::unique_ptr<StmtAST> else_stmt = nullptr)
      : cond(std::move(cond)),
        then_stmt(std::move(then_stmt)),
        else_stmt(std::move(else_stmt)) {}
};

class ReturnStmtAST : public StmtAST {
private:
  std::unique_ptr<ExprAST> ret;

public:
  explicit ReturnStmtAST(std::unique_ptr<ExprAST> ret = nullptr)
      : ret(std::move(ret)) {}
};

class WhileStmtAST : public StmtAST {
private:
  std::unique_ptr<Cond> cond;
  std::unique_ptr<StmtAST> stmt;

public:
  explicit WhileStmtAST(std::unique_ptr<Cond> cond,
                        std::unique_ptr<StmtAST> stmt)
      : cond(std::move(cond)), stmt(std::move(stmt)) {}
};

class BreakStmtAST : public StmtAST {};

class ContinueStmtAST : public StmtAST {};

class CompUnit : public Node {
private:
  std::vector<std::unique_ptr<Node>> nodes;

public:
  void AppendDecls(std::vector<std::unique_ptr<DeclAST>> decls);

  void AppendFuncDef(std::unique_ptr<FuncDefAST> funcDef);
};

#endif  // BDDD_AST_H