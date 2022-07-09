#ifndef BDDD_AST_H
#define BDDD_AST_H

#include <fstream>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

class Value;

class IRBuilder;

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

class SymbolTable;

class AST : public std::enable_shared_from_this<AST> {
public:
  virtual ~AST() = default;
  virtual void Debug(std::ofstream &ofs, int depth) = 0;
  virtual void TypeCheck(SymbolTable &symbolTable) = 0;
  virtual std::shared_ptr<Value> CodeGen(IRBuilder &builder) = 0;
};

class StmtAST : public AST {};

class DeclAST;

class ExprAST;

/**
 * @expr not null when single expression, this time vals is null
 * @vals not null when array of items, this time expr is null
 * @is_const available in typechecking
 */
class InitValAST : public AST {
private:
  bool is_const;  // true => expr is const or all sub-init-vals are const

public:
  [[nodiscard]] bool isConst() const { return is_const; }
  void setIsConst(bool isConst) { is_const = isConst; }

public:
  std::shared_ptr<ExprAST> expr;
  std::vector<std::unique_ptr<InitValAST>> vals;

  explicit InitValAST() : expr(nullptr), vals(), is_const(false) {}

  explicit InitValAST(std::unique_ptr<ExprAST> expr)
      : expr(std::move(expr)), vals(), is_const(false) {}

  explicit InitValAST(std::unique_ptr<InitValAST> val)
      : expr(nullptr), vals(), is_const(false) {
    vals.push_back(std::move(val));
  }

  void AppendVal(std::unique_ptr<InitValAST> val) {
    vals.push_back(std::move(val));
  }

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;

  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

/**
 * @name cannot be empty
 * @dimensions optional (nullptr might appear in dimensions)
 * @decl available after typechecking
 */
class LValAST : public AST {
private:
  std::string name;
  std::vector<std::unique_ptr<ExprAST>> dimensions;

public:
  std::string getName() const { return name; }
  std::shared_ptr<DeclAST> decl;
  explicit LValAST(std::string name)
      : name(std::move(name)), dimensions(), decl(nullptr) {}

  // methods used in AST construction
  void AddDimension(int x);

  void AddDimension(std::unique_ptr<ExprAST> expr);

  void Debug(std::ofstream &ofs, int depth) override;

  bool isArray();

  // methods used in typechecking

  void TypeCheck(SymbolTable &symbolTable) override;

  // called only when is_const is true and not an array
  std::variant<int, float> Evaluate(SymbolTable &SymbolTable);

  // methods used in codegen
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;

  std::shared_ptr<Value> CodeGenGEP(IRBuilder &builder);
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
 * @is_const available when typechecking
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
  bool is_const;  // true => can get value from int_val or float_val

public:
  [[nodiscard]] bool isConst() const { return is_const; }
  int intVal() const { return int_val; }
  float floatVal() const { return float_val; }
  void setIsConst(bool isConst) { is_const = isConst; }

public:
  enum class EvalType {
    INT,
    FLOAT,
    BOOL,
    VAR,
    ERROR,
  };

public:
  explicit ExprAST(Op op, std::unique_ptr<ExprAST> lhs,
                   std::unique_ptr<ExprAST> rhs = nullptr)
      : op(op),
        lhs(std::move(lhs)),
        rhs(std::move(rhs)),
        func_call(nullptr),
        int_val(0),
        float_val(0.0),
        lval(nullptr),
        is_const(false) {}

  explicit ExprAST(std::unique_ptr<FuncCallAST> func_call)
      : op(Op::FUNC_CALL),
        lhs(nullptr),
        rhs(nullptr),
        func_call(std::move(func_call)),
        int_val(0),
        float_val(0.0),
        lval(nullptr),
        is_const(false) {}

  explicit ExprAST(int val)
      : op(Op::CONST_INT),
        lhs(nullptr),
        rhs(nullptr),
        func_call(nullptr),
        int_val(val),
        float_val(0.0),
        lval(nullptr),
        is_const(true) {}

  explicit ExprAST(float val)
      : op(Op::CONST_FLOAT),
        lhs(nullptr),
        rhs(nullptr),
        func_call(nullptr),
        int_val(0),
        float_val(val),
        lval(nullptr),
        is_const(true) {}

  explicit ExprAST(std::unique_ptr<LValAST> lval)
      : op(Op::LVAL),
        lhs(nullptr),
        rhs(nullptr),
        func_call(nullptr),
        int_val(0),
        float_val(0.0),
        lval(std::move(lval)),
        is_const(false) {}

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;

  std::pair<EvalType, std::variant<int, float>> Evaluate(
      SymbolTable &symbolTable);

  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
  std::shared_ptr<Value> CodeGenAnd(IRBuilder &builder);
  std::shared_ptr<Value> CodeGenOr(IRBuilder &builder);
};

/**
 * @is_const indicates whether it is a const declaration
 * @is_global used in typechecking
 * @var_type only can be INT or FLOAT
 */
class DeclAST : public AST {
private:
  bool is_const;  // true => init_val is also const
  bool is_global;
  VarType var_type;

  std::string varname;

  void fillFlattenVals(int n, int offset, const std::vector<int> &sizes);

public:
  std::vector<std::unique_ptr<ExprAST>> dimensions;
  std::unique_ptr<InitValAST> init_val;
  std::vector<std::shared_ptr<ExprAST>> flatten_vals;

  void setIsConst(bool isConst) { is_const = isConst; }
  void setIsGlobal(bool isGlobal) { is_global = isGlobal; }
  void setVarType(VarType varType) { var_type = varType; }
  void setInitVal(std::unique_ptr<InitValAST> initVal) {
    init_val = std::move(initVal);
  }

  size_t dimensionsSize() const { return dimensions.size(); }
  bool isGlobal() const { return is_global; }
  bool isConst() const { return is_const; }
  VarType varType() const { return var_type; }

  explicit DeclAST(std::string varname,
                   std::unique_ptr<InitValAST> initval = nullptr)
      : is_const(false),
        is_global(false),
        var_type(VarType::UNKNOWN),
        varname(std::move(varname)),
        init_val(std::move(initval)),
        flatten_vals() {}

  void AddDimension(std::unique_ptr<ExprAST> expr);

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;

  // called only when is_const = true and flatten_vals is constructed
  std::variant<int, float> Evaluate(SymbolTable &symbolTable, int n);

  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

/**
 * @func_name the name of called function
 * @params the parameters of function call
 * @return_type the return type of function call, initially unknown
 */
class FuncCallAST : public AST {
private:
  std::string func_name;
  std::vector<std::unique_ptr<ExprAST>> params;
  VarType return_type;

public:
  [[nodiscard]] size_t paramsSize() const { return params.size(); }

public:
  explicit FuncCallAST(std::string func_name)
      : func_name(std::move(func_name)),
        params(),
        return_type(VarType::UNKNOWN) {}

  void assignParams(std::vector<std::unique_ptr<ExprAST>> _params) {
    params.assign(std::make_move_iterator(_params.begin()),
                  std::make_move_iterator(_params.end()));
    _params.clear();
  }

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class CondAST : public AST {
private:
  std::unique_ptr<ExprAST> expr;

public:
  explicit CondAST(std::unique_ptr<ExprAST> expr) : expr(std::move(expr)) {}

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

/**
 * @dimensions not null when param is array, the first element should be -1
 */
class FuncFParamAST : public AST {
private:
  VarType type;
  std::string name;
  std::vector<std::unique_ptr<ExprAST>> dimensions;

public:
  explicit FuncFParamAST(VarType type, std::string name)
      : type(type), name(std::move(name)), dimensions() {}

  explicit FuncFParamAST(VarType type, std::string name,
                         std::unique_ptr<ExprAST> dimension)
      : type(type), name(std::move(name)), dimensions() {
    dimensions.push_back(std::move(dimension));
  }

  void AddDimension();

  void AddDimension(int x);

  void AddDimension(std::unique_ptr<ExprAST> expr);

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class BlockAST : public StmtAST {
private:
  std::vector<std::shared_ptr<AST>> nodes;

public:
  void AppendNodes(std::vector<std::unique_ptr<AST>> appendedNodes);

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class FuncDefAST : public AST {
private:
  VarType return_type;
  std::string func_name;
  std::vector<std::unique_ptr<FuncFParamAST>> params;
  std::unique_ptr<BlockAST> block;

public:
  size_t paramsSize() const { return params.size(); }
  VarType returnType() const { return return_type; }
  std::string funcName() const { return func_name; }

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

  void assignParams(std::vector<std::unique_ptr<FuncFParamAST>> _params) {
    params.assign(std::make_move_iterator(_params.begin()),
                  std::make_move_iterator(_params.end()));
    _params.clear();
  }

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
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

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class EvalStmtAST : public StmtAST {
private:
  std::unique_ptr<ExprAST> expr;

public:
  explicit EvalStmtAST(std::unique_ptr<ExprAST> expr) : expr(std::move(expr)) {}

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
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

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class ReturnStmtAST : public StmtAST {
private:
  std::unique_ptr<ExprAST> ret;

public:
  explicit ReturnStmtAST(std::unique_ptr<ExprAST> ret = nullptr)
      : ret(std::move(ret)) {}

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
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

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class BreakStmtAST : public StmtAST {
public:
  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class ContinueStmtAST : public StmtAST {
public:
  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class CompUnitAST : public AST {
private:
  std::vector<std::shared_ptr<AST>> nodes;

public:
  void AppendDecls(std::vector<std::unique_ptr<DeclAST>> decls);

  void AppendFuncDef(std::unique_ptr<FuncDefAST> funcDef);

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbolTable) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

#endif  // BDDD_AST_H