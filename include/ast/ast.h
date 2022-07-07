#ifndef BDDD_AST_H
#define BDDD_AST_H

#include <fstream>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

/* used as var_type indicator of ExprAST instance */
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
  FUNC_CALL,
};

enum class VarType {
  INT,
  FLOAT,
  VOID,
  UNKNOWN,
};

class AST {
public:
  virtual ~AST() = default;
  virtual void Debug(std::ofstream &ofs, int depth) = 0;
  // virtual void TypeCheck() = 0;
};

class StmtAST : public AST {};

class DeclAST;

class ExprAST;

/**
 * expr != nullptr, vals == {}: single expression
 * expr == nullptr, vals != {}: array of items
 */
class InitValAST : public AST {
private:
  std::unique_ptr<ExprAST> expr;
  std::vector<std::unique_ptr<InitValAST>> vals;

public:
  explicit InitValAST() : expr(nullptr), vals() {}

  explicit InitValAST(std::unique_ptr<ExprAST> expr)
      : expr(std::move(expr)), vals() {}

  explicit InitValAST(std::unique_ptr<InitValAST> val) : expr(nullptr), vals() {
    vals.push_back(std::move(val));
  }

  void AppendVal(std::unique_ptr<InitValAST> val) {
    vals.push_back(std::move(val));
  }

  void Debug(std::ofstream &ofs, int depth) override;
};

/**
 * name cannot be empty
 * optional: dimensions
 * -1 might appear in dimensions
 */
class LValAST : public AST {
private:
  // std::unique_ptr<DeclAST> decl;
  std::string name;
  std::vector<std::unique_ptr<ExprAST>> dimensions;

public:
  explicit LValAST(std::string name) : name(std::move(name)), dimensions() {}

  // explicit LValAST(std::unique_ptr<DeclAST> decl)
  //     : decl(std::move(decl)), name(), dimensions() {}

  void AddDimension(int x);

  void AddDimension(std::unique_ptr<ExprAST> expr);

  void Debug(std::ofstream &ofs, int depth) override;
};

class FuncCallAST;

/**
 * @op indicates the type of expression
 * @lhs meaningful when op is unary or binary operator
 * @rhs meaningful only if op is binary operator
 * @int_val meaningful only if op == CONST_INT
 * @float_val meaningful only if op == CONST_FLOAT
 * @func_call meaningful only if op == FUNC_CALL
 * @lval meaningful only if op == LVAL
 */
class ExprAST : public AST {
private:
  Op op;
  std::unique_ptr<ExprAST> lhs;
  std::unique_ptr<ExprAST> rhs;
  std::unique_ptr<FuncCallAST> func_call;
  int int_val;
  float float_val;
  std::unique_ptr<LValAST> lval;

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

  explicit ExprAST(std::unique_ptr<FuncCallAST> func_call)
      : op(Op::FUNC_CALL),
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

  explicit ExprAST(std::unique_ptr<LValAST> lval)
      : op(Op::LVAL),
        lhs(nullptr),
        rhs(nullptr),
        func_call(nullptr),
        int_val(0),
        float_val(0.0),
        lval(std::move(lval)) {}

  void Debug(std::ofstream &ofs, int depth) override;
};

/**
 * @is_const indicates whether it is a const declaration
 * @var_type only can be INT or FLOAT
 */
class DeclAST : public AST {
private:
  bool is_const;
  VarType var_type;
  std::unique_ptr<InitValAST> init_val;

public:
  void setIsConst(bool isConst);
  void setVarType(VarType varType);
  void setInitVal(std::unique_ptr<InitValAST> initVal);

private:
  std::string varname;
  std::vector<std::unique_ptr<ExprAST>> dimensions;

public:
  explicit DeclAST(std::string varname,
                   std::unique_ptr<InitValAST> initval = nullptr)
      : is_const(false),
        var_type(VarType::UNKNOWN),
        varname(std::move(varname)),
        init_val(std::move(initval)) {}

  void AddDimension(std::unique_ptr<ExprAST> expr);

  void Debug(std::ofstream &ofs, int depth) override;
};

class FuncCallAST : public AST {
private:
  std::string func_name;
  std::vector<std::unique_ptr<ExprAST>> params;

public:
  explicit FuncCallAST(std::string func_name)
      : func_name(std::move(func_name)), params() {}

  void assignParams(std::vector<std::unique_ptr<ExprAST>> _params) {
    params.assign(std::make_move_iterator(_params.begin()),
                  std::make_move_iterator(_params.end()));
    _params.clear();
  }

  void Debug(std::ofstream &ofs, int depth) override;
};

class CondAST : public AST {
private:
  std::unique_ptr<ExprAST> expr;

public:
  explicit CondAST(std::unique_ptr<ExprAST> expr) : expr(std::move(expr)) {}

  void Debug(std::ofstream &ofs, int depth) override;
};

class FuncFParamAST : public AST {
private:
  VarType type;
  std::string name;
  std::vector<std::unique_ptr<ExprAST>> dimensions;

public:
  explicit FuncFParamAST(VarType type, std::string name)
      : type(type), name(std::move(name)), dimensions() {}

  void AddDimension(int x);

  void AddDimension(std::unique_ptr<ExprAST> expr);

  void Debug(std::ofstream &ofs, int depth) override;
};

class BlockAST : public StmtAST {
private:
  std::vector<std::unique_ptr<AST>> nodes;

public:
  void AppendNodes(std::vector<std::unique_ptr<AST>> appendedNodes);

  void Debug(std::ofstream &ofs, int depth) override;
};

class FuncDefAST : public AST {
private:
  VarType return_type;
  std::string func_name;
  std::vector<std::unique_ptr<FuncFParamAST>> params;
  std::unique_ptr<BlockAST> block;

public:
  explicit FuncDefAST(VarType return_type, std::string func_name,
                      std::vector<std::unique_ptr<FuncFParamAST>> params,
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

  void assignParams(std::vector<std::unique_ptr<FuncFParamAST>> params) {
    this->params.assign(std::make_move_iterator(params.begin()),
                        std::make_move_iterator(params.end()));
    params.clear();
  }

  void Debug(std::ofstream &ofs, int depth) override;
};

class AssignStmtAST : public StmtAST {
private:
  std::unique_ptr<LValAST> lval;
  std::unique_ptr<ExprAST> rhs;

public:
  explicit AssignStmtAST(std::unique_ptr<LValAST> lval,
                         std::unique_ptr<ExprAST> rhs)
      : lval(std::move(lval)), rhs(std::move(rhs)) {}

  void Debug(std::ofstream &ofs, int depth) override;
};

class EvalStmtAST : public StmtAST {
private:
  std::unique_ptr<ExprAST> expr;

public:
  explicit EvalStmtAST(std::unique_ptr<ExprAST> expr = nullptr)
      : expr(std::move(expr)) {}

  void Debug(std::ofstream &ofs, int depth) override;
};

class IfStmtAST : public StmtAST {
private:
  std::unique_ptr<CondAST> cond;
  std::unique_ptr<StmtAST> then_stmt;
  std::unique_ptr<StmtAST> else_stmt;

public:
  explicit IfStmtAST(std::unique_ptr<CondAST> cond,
                     std::unique_ptr<StmtAST> then_stmt,
                     std::unique_ptr<StmtAST> else_stmt = nullptr)
      : cond(std::move(cond)),
        then_stmt(std::move(then_stmt)),
        else_stmt(std::move(else_stmt)) {}

  void Debug(std::ofstream &ofs, int depth) override;
};

class ReturnStmtAST : public StmtAST {
private:
  std::unique_ptr<ExprAST> ret;

public:
  explicit ReturnStmtAST(std::unique_ptr<ExprAST> ret = nullptr)
      : ret(std::move(ret)) {}

  void Debug(std::ofstream &ofs, int depth) override;
};

class WhileStmtAST : public StmtAST {
private:
  std::unique_ptr<CondAST> cond;
  std::unique_ptr<StmtAST> stmt;

public:
  explicit WhileStmtAST(std::unique_ptr<CondAST> cond,
                        std::unique_ptr<StmtAST> stmt)
      : cond(std::move(cond)), stmt(std::move(stmt)) {}

  void Debug(std::ofstream &ofs, int depth) override;
};

class BreakStmtAST : public StmtAST {
public:
  void Debug(std::ofstream &ofs, int depth) override;
};

class ContinueStmtAST : public StmtAST {
public:
  void Debug(std::ofstream &ofs, int depth) override;
};

class CompUnitAST : public AST {
private:
  std::vector<std::unique_ptr<AST>> nodes;

public:
  void AppendDecls(std::vector<std::unique_ptr<DeclAST>> decls);

  void AppendFuncDef(std::unique_ptr<FuncDefAST> funcDef);

  void Debug(std::ofstream &ofs, int depth) override;
};

#endif  // BDDD_AST_H