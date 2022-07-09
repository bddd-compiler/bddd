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
  virtual void TypeCheck(SymbolTable &symbol_table) = 0;
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
  bool m_is_const;  // true => expr is const or all sub-init-vals are const
  std::shared_ptr<ExprAST> m_expr;

public:
  [[nodiscard]] bool IsConst() const { return m_is_const; }
  void SetIsConst(bool is_const) { m_is_const = is_const; }

public:
  std::vector<std::unique_ptr<InitValAST>> m_vals;

  explicit InitValAST() : m_expr(nullptr), m_vals(), m_is_const(false) {}

  explicit InitValAST(std::unique_ptr<ExprAST> expr)
      : m_expr(std::move(expr)), m_vals(), m_is_const(false) {}

  explicit InitValAST(std::unique_ptr<InitValAST> val)
      : m_expr(nullptr), m_vals(), m_is_const(false) {
    m_vals.push_back(std::move(val));
  }

  void AppendVal(std::unique_ptr<InitValAST> val) {
    m_vals.push_back(std::move(val));
  }

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  void FillVals(int n, int &offset, const std::vector<int> &sizes,
                std::vector<std::shared_ptr<ExprAST>> &vals);

  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;

  friend class DeclAST;
};

/**
 * @name cannot be empty
 * @dimensions optional (nullptr might appear in dimensions)
 * @decl available after typechecking
 */
class LValAST : public AST {
private:
  std::string m_name;
  std::vector<std::unique_ptr<ExprAST>> m_dimensions;

public:
  std::shared_ptr<DeclAST> m_decl;
  std::string Name() const { return m_name; }

  explicit LValAST(std::string name)
      : m_name(std::move(name)), m_dimensions(), m_decl(nullptr) {}

  // methods used in AST construction
  void AddDimension(int x);

  void AddDimension(std::unique_ptr<ExprAST> expr);

  void Debug(std::ofstream &ofs, int depth) override;

  bool IsArray();

  // methods used in typechecking

  void TypeCheck(SymbolTable &symbol_table) override;

  // called only when is_const is true and not an array
  std::variant<int, float> Evaluate(SymbolTable &symbol_table);

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
  Op m_op;
  std::unique_ptr<ExprAST> m_lhs;
  std::unique_ptr<ExprAST> m_rhs;
  std::unique_ptr<FuncCallAST> m_func_call;
  int m_int_val;
  float m_float_val;
  std::unique_ptr<LValAST> m_lval;
  bool m_is_const;  // true => can get value from int_val or float_val

public:
  [[nodiscard]] bool IsConst() const { return m_is_const; }
  int IntVal() const { return m_int_val; }
  float FloatVal() const { return m_float_val; }
  void SetIsConst(bool is_const) { m_is_const = is_const; }
  Op GetOp() const { return m_op; }

public:
  enum class EvalType {
    INT,  // BOOL is included, non-zero is true, zero is false
    FLOAT,
    // BOOL,
    VAR_INT,
    VAR_FLOAT,
    ARR,  // array cannot join with any computation
    ERROR,
  };

public:
  explicit ExprAST(Op op, std::unique_ptr<ExprAST> lhs,
                   std::unique_ptr<ExprAST> rhs = nullptr)
      : m_op(op),
        m_lhs(std::move(lhs)),
        m_rhs(std::move(rhs)),
        m_func_call(nullptr),
        m_int_val(0),
        m_float_val(0.0),
        m_lval(nullptr),
        m_is_const(false) {}

  explicit ExprAST(std::unique_ptr<FuncCallAST> func_call)
      : m_op(Op::FUNC_CALL),
        m_lhs(nullptr),
        m_rhs(nullptr),
        m_func_call(std::move(func_call)),
        m_int_val(0),
        m_float_val(0.0),
        m_lval(nullptr),
        m_is_const(false) {}

  explicit ExprAST(int val)
      : m_op(Op::CONST_INT),
        m_lhs(nullptr),
        m_rhs(nullptr),
        m_func_call(nullptr),
        m_int_val(val),
        m_float_val(0.0),
        m_lval(nullptr),
        m_is_const(true) {}

  explicit ExprAST(float val)
      : m_op(Op::CONST_FLOAT),
        m_lhs(nullptr),
        m_rhs(nullptr),
        m_func_call(nullptr),
        m_int_val(0),
        m_float_val(val),
        m_lval(nullptr),
        m_is_const(true) {}

  explicit ExprAST(std::unique_ptr<LValAST> lval)
      : m_op(Op::LVAL),
        m_lhs(nullptr),
        m_rhs(nullptr),
        m_func_call(nullptr),
        m_int_val(0),
        m_float_val(0.0),
        m_lval(std::move(lval)),
        m_is_const(false) {}

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;

  std::pair<EvalType, std::variant<int, float>> Evaluate(
      SymbolTable &symbol_table);

  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
  std::shared_ptr<Value> CodeGenAnd(IRBuilder &builder);
  std::shared_ptr<Value> CodeGenOr(IRBuilder &builder);
};

/**
 * @is_const indicates whether it is a const declaration
 * @is_global used in typechecking
 * @is_param not declaration of variable but an argument of function definition
 * @var_type only can be INT or FLOAT
 */
class DeclAST : public AST {
private:
  bool m_is_const;   // true => init_val is also const
  bool m_is_global;  // false at default
  VarType m_var_type;
  std::string m_varname;
  bool m_is_param;  // false at default

public:
  std::vector<std::unique_ptr<ExprAST>> m_dimensions;
  std::unique_ptr<InitValAST> m_init_val;
  std::vector<std::shared_ptr<ExprAST>> m_flatten_vals;

  void SetIsConst(bool is_const) { m_is_const = is_const; }
  void SetIsGlobal(bool is_global) { m_is_global = is_global; }
  void SetIsParam(bool is_param) { m_is_param = is_param; }
  void SetVarType(VarType var_type) { m_var_type = var_type; }
  void SetInitVal(std::unique_ptr<InitValAST> init_val) {
    m_init_val = std::move(init_val);
  }

  size_t DimensionsSize() const { return m_dimensions.size(); }
  bool IsGlobal() const { return m_is_global; }
  bool IsConst() const { return m_is_const; }
  bool IsParam() const { return m_is_param; }
  VarType GetVarType() const { return m_var_type; }
  std::string VarName() const { return m_varname; }

  explicit DeclAST(std::string varname,
                   std::unique_ptr<InitValAST> init_val = nullptr)
      : m_is_const(false),
        m_is_global(false),
        m_is_param(false),
        m_var_type(VarType::UNKNOWN),
        m_varname(std::move(varname)),
        m_init_val(std::move(init_val)),
        m_flatten_vals() {}

  void AddDimension(int x);
  void AddDimension(std::unique_ptr<ExprAST> expr);

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;

  // called only when is_const = true and flatten_vals is constructed
  std::variant<int, float> Evaluate(SymbolTable &symbol_table, int n);

  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

/**
 * @func_name the name of called function
 * @params the parameters of function call
 * @return_type the return type of function call, initially unknown
 */
class FuncCallAST : public AST {
private:
  std::string m_func_name;
  std::vector<std::unique_ptr<ExprAST>> m_params;
  VarType m_return_type;  // initially UNKNOWN, available after typechecking

public:
  [[nodiscard]] size_t ParamsSize() const { return m_params.size(); }

public:
  VarType GetReturnType() const { return m_return_type; }
  explicit FuncCallAST(std::string func_name)
      : m_func_name(std::move(func_name)),
        m_params(),
        m_return_type(VarType::UNKNOWN) {}

  void AssignParams(std::vector<std::unique_ptr<ExprAST>> params);

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class CondAST : public AST {
private:
  std::unique_ptr<ExprAST> m_expr;

public:
  explicit CondAST(std::unique_ptr<ExprAST> expr) : m_expr(std::move(expr)) {}

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

/**
 * @attention
 * FuncFParam is a special wrapper of DeclAST, in which m_is_const = false,
 * m_is_global = false, m_init_val = nullptr, and m_flatten_vals is meaningless
 *
 * We only use m_name, m_var_type and m_dimensions
 */
class FuncFParamAST : public AST {
private:
  std::shared_ptr<DeclAST> decl;

public:
  explicit FuncFParamAST(VarType type, std::string name)
      : decl(std::make_shared<DeclAST>(std::move(name))) {
    decl->SetVarType(type);
    decl->SetIsParam(true);
  }
  explicit FuncFParamAST(VarType type, std::string name,
                         std::unique_ptr<ExprAST> dimension)
      : decl(std::make_shared<DeclAST>(std::move(name))) {
    decl->SetVarType(type);
    decl->SetIsParam(true);
    decl->m_dimensions.push_back(std::move(dimension));
  }

  void AddDimension(int x);

  void AddDimension(std::unique_ptr<ExprAST> expr);

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class BlockAST : public StmtAST {
private:
  std::vector<std::shared_ptr<AST>> m_nodes;

public:
  void AppendNodes(std::vector<std::unique_ptr<AST>> nodes);

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class FuncDefAST : public AST {
private:
  VarType m_return_type;
  std::string m_func_name;
  std::vector<std::unique_ptr<FuncFParamAST>> m_params;
  std::unique_ptr<BlockAST> m_block;

public:
  size_t ParamsSize() const { return m_params.size(); }
  VarType ReturnType() const { return m_return_type; }
  std::string FuncName() const { return m_func_name; }

public:
  explicit FuncDefAST(VarType return_type, std::string func_name,
                      std::vector<std::unique_ptr<FuncFParamAST>> params,
                      std::unique_ptr<BlockAST> block)
      : m_return_type(return_type),
        m_func_name(std::move(func_name)),
        m_params(std::move(params)),
        m_block(std::move(block)) {}

  explicit FuncDefAST(VarType return_type, std::string func_name,
                      std::unique_ptr<BlockAST> block)
      : m_return_type(return_type),
        m_func_name(std::move(func_name)),
        m_params(),
        m_block(std::move(block)) {}

  void AssignParams(std::vector<std::unique_ptr<FuncFParamAST>> params);

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class AssignStmtAST : public StmtAST {
private:
  std::unique_ptr<LValAST> m_lval;
  std::unique_ptr<ExprAST> m_rhs;

public:
  explicit AssignStmtAST(std::unique_ptr<LValAST> lval,
                         std::unique_ptr<ExprAST> rhs)
      : m_lval(std::move(lval)), m_rhs(std::move(rhs)) {}

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class EvalStmtAST : public StmtAST {
private:
  std::unique_ptr<ExprAST> m_expr;

public:
  explicit EvalStmtAST(std::unique_ptr<ExprAST> expr)
      : m_expr(std::move(expr)) {}

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class IfStmtAST : public StmtAST {
private:
  std::unique_ptr<CondAST> m_cond;
  std::unique_ptr<StmtAST> m_then;
  std::unique_ptr<StmtAST> m_else;

public:
  explicit IfStmtAST(std::unique_ptr<CondAST> cond,
                     std::unique_ptr<StmtAST> then_stmt,
                     std::unique_ptr<StmtAST> else_stmt = nullptr)
      : m_cond(std::move(cond)),
        m_then(std::move(then_stmt)),
        m_else(std::move(else_stmt)) {}

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class ReturnStmtAST : public StmtAST {
private:
  std::unique_ptr<ExprAST> m_ret;

public:
  explicit ReturnStmtAST(std::unique_ptr<ExprAST> ret = nullptr)
      : m_ret(std::move(ret)) {}

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class WhileStmtAST : public StmtAST {
private:
  std::unique_ptr<CondAST> m_cond;
  std::unique_ptr<StmtAST> m_stmt;

public:
  explicit WhileStmtAST(std::unique_ptr<CondAST> cond,
                        std::unique_ptr<StmtAST> stmt)
      : m_cond(std::move(cond)), m_stmt(std::move(stmt)) {}

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class BreakStmtAST : public StmtAST {
public:
  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class ContinueStmtAST : public StmtAST {
public:
  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

class CompUnitAST : public AST {
private:
  std::vector<std::shared_ptr<AST>> m_nodes;

public:
  void AppendDecls(std::vector<std::unique_ptr<DeclAST>> decls);

  void AppendFuncDef(std::unique_ptr<FuncDefAST> funcDef);

  void Debug(std::ofstream &ofs, int depth) override;

  void TypeCheck(SymbolTable &symbol_table) override;
  std::shared_ptr<Value> CodeGen(IRBuilder &builder) override;
};

extern std::vector<std::shared_ptr<FuncDefAST>> g_builtin_funcs;  // global

void InitBuiltinFunctions();

#endif  // BDDD_AST_H