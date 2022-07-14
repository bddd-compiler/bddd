#ifndef BDDD_IR_H
#define BDDD_IR_H

#include <list>
#include <memory>
#include <unordered_map>
#include <utility>

#include "ast/ast.h"

enum class IROp {
  ADD,
  SUB,
  MUL,
  SDIV,
  SREM,
  SGEQ,
  SGE,
  SLEQ,
  SLE,
  EQ,
  NE,
  CALL,
  BRANCH,
  JUMP,
  RETURN,
  ALLOCA,
  LOAD,
  STORE,
  GET_ELEMENT_PTR,
  PHI,
};

enum class ValueType {
  INT,
  FLOAT,
  INT_PTR,
  FLOAT_PTR,
  VOID,
};

class Value;

class Use {
public:
  std::shared_ptr<Value> m_value;
  std::shared_ptr<Value> m_user;

  explicit Use(std::shared_ptr<Value> value);

  void SetUser(std::shared_ptr<Value> user);
};

// for simplification, value and user are composite together
class Value : public std::enable_shared_from_this<Value> {
protected:
  std::list<Use> m_use_list;      // users that use this value (def-use)
  std::list<Use> m_operand_list;  // values that this user uses (use-def)
  template <typename Derived> std::shared_ptr<Derived> shared_from_base() {
    return std::static_pointer_cast<Derived>(shared_from_this());
  }

public:
  ValueType m_type;

  explicit Value() : m_type(ValueType::VOID) {}
  explicit Value(ValueType type) : m_type(type) {}
  explicit Value(bool is_float, bool is_array) {
    if (is_float) {
      if (is_array)
        m_type = ValueType::FLOAT_PTR;
      else
        m_type = ValueType::FLOAT;
    } else {
      if (is_array)
        m_type = ValueType::INT_PTR;
      else
        m_type = ValueType::INT;
    }
  }

  ValueType GetType() const { return m_type; }

  std::string GetTypeString() const {
    switch (m_type) {
      case ValueType::INT:
        return "i32 ";
      case ValueType::FLOAT:
        return "f32 ";
      case ValueType::INT_PTR:
        return "i32* ";
      case ValueType::FLOAT_PTR:
        return "f32* ";
      case ValueType::VOID:
        return "void ";
      default:
        return "???";
    }
  }

  void AppendUse(Use use);
  void AppendOperand(const Use &use);

  virtual void ExportIR(std::ofstream &ofs, int depth) = 0;
};

class Constant : public Value {
public:
  bool m_is_float;
  int m_int_val;
  float m_float_val;

  explicit Constant(int int_val)
      : Value(ValueType::INT),
        m_is_float(false),
        m_int_val(int_val),
        m_float_val(0.0) {}

  explicit Constant(float float_val)
      : Value(ValueType::FLOAT),
        m_is_float(true),
        m_int_val(0),
        m_float_val(float_val) {}

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class GlobalVariable : public Value {
public:
  std::string m_varname;
  bool m_is_const;
  bool m_is_float;
  bool m_is_array;
  // TODO(garen): record info about m_dimensions of array

  explicit GlobalVariable(const std::shared_ptr<DeclAST> &decl)
      : Value(decl->GetVarType() == VarType::FLOAT, decl->IsArray()),
        m_varname(decl->VarName()),
        m_is_const(decl->IsConst()),
        m_is_float(decl->GetVarType() == VarType::FLOAT),
        m_is_array(decl->IsArray()) {}

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class IntGlobalVariable : public GlobalVariable {
public:
  std::vector<int> m_init_vals;

  explicit IntGlobalVariable(const std::shared_ptr<DeclAST> &decl)
      : GlobalVariable(decl), m_init_vals() {
    for (const auto &init_val : decl->m_flatten_vals) {
      if (init_val == nullptr) m_init_vals.push_back(0);
      m_init_vals.push_back(init_val->IntVal());
    }
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class FloatGlobalVariable : public GlobalVariable {
public:
  std::vector<float> m_init_vals;

  explicit FloatGlobalVariable(const std::shared_ptr<DeclAST> &decl)
      : GlobalVariable(decl), m_init_vals() {
    for (const auto &init_val : decl->m_flatten_vals) {
      if (init_val == nullptr) m_init_vals.push_back(0);
      m_init_vals.push_back(init_val->FloatVal());
    }
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
};
class BasicBlock;

class Instruction : public Value {
public:
  IROp m_op;
  std::shared_ptr<BasicBlock> m_bb;  // belong to which basic block

  explicit Instruction(IROp op, std::shared_ptr<BasicBlock> bb = nullptr)
      : Value(), m_op(op), m_bb(std::move(bb)) {}
};

class BinaryInstruction : public Instruction {
public:
  Use m_lhs_val_use;
  Use m_rhs_val_use;

  explicit BinaryInstruction(IROp op, std::shared_ptr<Value> lhs_val,
                             std::shared_ptr<Value> rhs_val,
                             std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(op, std::move(bb)),
        m_lhs_val_use(std::move(lhs_val)),
        m_rhs_val_use(std::move(rhs_val)) {
    assert(m_lhs_val_use.m_value->m_type == m_rhs_val_use.m_value->m_type);
    m_type = m_lhs_val_use.m_value->m_type;
    AppendOperand(m_lhs_val_use);
    AppendOperand(m_rhs_val_use);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class Function;

class CallInstruction : public Instruction {
public:
  VarType m_return_type;
  std::string m_func_name;
  std::vector<Use> m_params;

  std::shared_ptr<Function> m_function;

  explicit CallInstruction(VarType return_type, std::string func_name,
                           std::vector<Use> m_params,
                           std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(IROp::CALL, std::move(bb)),
        m_return_type(return_type),
        m_func_name(std::move(func_name)),
        m_params(std::move(m_params)) {}

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class BranchInstruction : public Instruction {
public:
  Use m_cond;
  std::shared_ptr<BasicBlock> m_true_block, m_false_block;

  explicit BranchInstruction(std::shared_ptr<Value> cond,
                             std::shared_ptr<BasicBlock> true_block,
                             std::shared_ptr<BasicBlock> false_block,
                             std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(IROp::BRANCH, std::move(bb)),
        m_cond(std::move(cond)),
        m_true_block(std::move(true_block)),
        m_false_block(std::move(false_block)) {}

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class JumpInstruction : public Instruction {
public:
  std::shared_ptr<BasicBlock> m_target_block;

  explicit JumpInstruction(std::shared_ptr<BasicBlock> target_block,
                           std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(IROp::JUMP, std::move(bb)),
        m_target_block(std::move(target_block)) {}

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class ReturnInstruction : public Instruction {
public:
  Use m_ret;

  explicit ReturnInstruction(std::shared_ptr<Value> ret = nullptr,
                             std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(IROp::RETURN, std::move(bb)), m_ret(std::move(ret)) {
    if (m_ret.m_value != nullptr) AppendOperand(m_ret);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;

  bool IsConstant() {
    return std::dynamic_pointer_cast<Constant>(m_ret.m_value) != nullptr;
  }
};

class AccessInstruction : public Instruction {
public:
  explicit AccessInstruction(IROp op, std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(op, std::move(bb)) {}
};

class GetElementPtrInstruction : public AccessInstruction {
public:
  std::shared_ptr<Value> m_addr;
  std::vector<std::shared_ptr<Value>> m_dimensions;
  std::vector<int> m_products;

  explicit GetElementPtrInstruction(
      std::shared_ptr<Value> addr,
      std::vector<std::shared_ptr<Value>> dimensions, std::vector<int> products,
      std::shared_ptr<BasicBlock> bb = nullptr)
      : AccessInstruction(IROp::GET_ELEMENT_PTR, std::move(bb)),
        m_addr(std::move(addr)),
        m_dimensions(std::move(dimensions)),
        m_products(std::move(products)) {}

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class LoadInstruction : public AccessInstruction {
public:
  Use m_addr;
  explicit LoadInstruction(std::shared_ptr<Value> addr,
                           std::shared_ptr<BasicBlock> bb = nullptr)
      : AccessInstruction(IROp::LOAD, std::move(bb)), m_addr(std::move(addr)) {
    switch (m_addr.m_value->m_type) {
      case ValueType::INT_PTR:
        m_type = ValueType::INT;
        break;
      case ValueType::FLOAT_PTR:
        m_type = ValueType::FLOAT;
        break;
      default:
        assert(false);  // unreachable
    }
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class StoreInstruction : public AccessInstruction {
public:
  Use m_addr, m_val;

  explicit StoreInstruction(std::shared_ptr<Value> addr,
                            std::shared_ptr<Value> val,
                            std::shared_ptr<BasicBlock> bb = nullptr)
      : AccessInstruction(IROp::STORE, std::move(bb)),
        m_addr(std::move(addr)),
        m_val(std::move(val)) {}

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class AllocaInstruction : public Instruction {
public:
  int m_size;
  std::shared_ptr<Value> m_init_val;

  explicit AllocaInstruction(int size,
                             std::shared_ptr<Value> init_val = nullptr,
                             std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(IROp::ALLOCA, std::move(bb)),
        m_size(size),
        m_init_val(init_val) {}

  explicit AllocaInstruction(VarType type,
                             std::shared_ptr<Value> init_val = nullptr,
                             std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(IROp::ALLOCA, std::move(bb)), m_init_val(init_val) {
    if (type == VarType::INT)
      m_type = ValueType::INT_PTR;
    else if (type == VarType::FLOAT)
      m_type = ValueType::FLOAT_PTR;
    else
      assert(false);  // unknown
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class PhiInstruction : public Instruction {
public:
  void ExportIR(std::ofstream &ofs, int depth) override;
};

class BasicBlock : public Value {
private:
  std::string m_name;

  std::list<std::shared_ptr<Instruction>> m_instr_list;

public:
  explicit BasicBlock(std::string name) : Value(), m_name(std::move(name)) {}

  void PushBackInstruction(std::shared_ptr<Instruction> instr);
  void PushFrontInstruction(std::shared_ptr<Instruction> instr);

  void InsertFrontInstruction(const std::shared_ptr<Instruction> &elem,
                              std::shared_ptr<Instruction> instr);
  void InsertBackInstruction(const std::shared_ptr<Instruction> &elem,
                             std::shared_ptr<Instruction> instr);

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class FunctionArg : public Value {
public:
  std::shared_ptr<DeclAST> m_decl;
  explicit FunctionArg(std::unique_ptr<FuncFParamAST> &arg)
      : m_decl(arg->m_decl) {}

  void ExportIR(std::ofstream &ofs, int depth) override;
};

// function declaration
// class FuncDecl {};

// class definition
class Function : public Value {
private:
  std::string m_func_name;
  std::shared_ptr<FuncDefAST> m_func_ast;
  std::vector<std::shared_ptr<FunctionArg>> m_args;

  std::list<std::shared_ptr<BasicBlock>> m_bb_list;
  std::shared_ptr<BasicBlock> m_current_bb;

public:
  explicit Function(const std::shared_ptr<FuncDefAST> &func_ast)
      : m_func_ast(func_ast),
        m_func_name(func_ast->FuncName()),
        m_bb_list(),
        m_args(),
        m_current_bb(nullptr) {
    for (auto &arg : func_ast->m_params) {
      m_args.push_back(std::make_unique<FunctionArg>(arg));
    }
  }

  [[nodiscard]] std::string FuncName() const { return m_func_name; }
  [[nodiscard]] VarType ReturnType() const { return m_func_ast->ReturnType(); }

  std::shared_ptr<BasicBlock> GetCurrentBB() { return m_current_bb; }

  void AppendBasicBlock(std::shared_ptr<BasicBlock> bb);

  void ExportIR(std::ofstream &ofs, int depth);
};

class Module {
private:
  std::shared_ptr<Function> m_current_func;

public:
  std::list<std::shared_ptr<Function>> m_function_list;
  // std::list<std::shared_ptr<FuncDecl>> m_func_decl_list;
  std::list<std::shared_ptr<GlobalVariable>> m_global_variable_list;

  std::map<int, std::shared_ptr<Constant>> m_const_ints;
  std::map<float, std::shared_ptr<Constant>> m_const_floats;

  std::shared_ptr<BasicBlock> GetCurrentBB() {
    return m_current_func->GetCurrentBB();
  }
  std::shared_ptr<Function> GetCurrentFunc() { return m_current_func; }

  void AppendFunction(std::shared_ptr<Function> function);
  void AppendGlobalVariable(std::shared_ptr<GlobalVariable> global_variable);
  void AppendBasicBlock(std::shared_ptr<BasicBlock> bb);

  void ExportIR(std::ofstream &ofs, int depth);
};

class IRBuilder : public std::enable_shared_from_this<IRBuilder> {
public:
  std::unique_ptr<Module> m_module;
  std::shared_ptr<BasicBlock> m_if_true, m_if_false, m_if_exit;
  std::shared_ptr<BasicBlock> m_while_entry, m_while_header, m_while_exit;

public:
  explicit IRBuilder(std::unique_ptr<Module> module)
      : m_module(std::move(module)),
        m_if_true(nullptr),
        m_if_false(nullptr),
        m_if_exit(nullptr),
        m_while_entry(nullptr),
        m_while_header(nullptr),
        m_while_exit(nullptr) {}

  std::shared_ptr<BasicBlock> CreateBasicBlock(std::string block_name);

  void AppendBasicBlock(std::shared_ptr<BasicBlock> bb);

  std::shared_ptr<Function> CreateFunction(std::shared_ptr<FuncDefAST> func);

  // std::unique_ptr<PhiInstruction> CreatePhiInstruction();

  std::shared_ptr<Instruction> CreateBinaryInstruction(
      IROp op, const std::shared_ptr<Value> &lhs,
      const std::shared_ptr<Value> &rhs);

  std::shared_ptr<Instruction> CreateCallInstruction(
      std::shared_ptr<FuncCallAST> func_call);

  std::shared_ptr<Instruction> CreateBranchInstruction(
      std::shared_ptr<Value> cond_val, std::shared_ptr<BasicBlock> true_block,
      std::shared_ptr<BasicBlock> false_block);

  std::shared_ptr<Instruction> CreateJumpInstruction(
      std::shared_ptr<BasicBlock> block);

  std::shared_ptr<Instruction> CreateReturnInstruction(
      std::shared_ptr<Value> ret = nullptr);

  std::shared_ptr<Instruction> CreateGetElementPtrInstruction(
      std::shared_ptr<Value> addr,
      std::vector<std::shared_ptr<Value>> dimensions,
      const std::vector<int> &products);

  std::shared_ptr<Instruction> CreateLoadInstruction(
      std::shared_ptr<Value> addr);

  std::shared_ptr<Instruction> CreateStoreInstruction(
      std::shared_ptr<Value> addr, std::shared_ptr<Value> val);

  std::shared_ptr<Instruction> CreateAllocaInstruction(
      std::shared_ptr<DeclAST> decl, std::shared_ptr<Value> init_val = nullptr);

  std::shared_ptr<GlobalVariable> CreateGlobalVariable(
      std::shared_ptr<DeclAST> decl);
  std::shared_ptr<IntGlobalVariable> CreateIntGlobalVariable(
      std::shared_ptr<DeclAST> decl);
  std::shared_ptr<FloatGlobalVariable> CreateFloatGlobalVariable(
      std::shared_ptr<DeclAST> decl);

  std::shared_ptr<Value> GetIntConstant(int int_val);
  std::shared_ptr<Value> GetFloatConstant(float float_val);
};

#endif  // BDDD_IR_H
