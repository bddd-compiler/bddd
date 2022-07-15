#ifndef BDDD_IR_H
#define BDDD_IR_H

#include <algorithm>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>

#include "ast/ast.h"
#include "ir/type.h"

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
  ZEXT,
};

class Value;

class Use {
public:
  std::shared_ptr<Value> m_value;
  std::shared_ptr<Value> m_user;

  explicit Use(std::shared_ptr<Value> value,
               std::shared_ptr<Value> user = nullptr)
      : m_value(std::move(value)), m_user(std::move(user)) {}

  // IMPORTANT:
  // must be done after construction in IRBuilder::CreateXXX
  // only after this function, the whole initialization is complete
  void SetUser(std::shared_ptr<Value> user);
};

// for simplification, value and user are composite together
class Value : public std::enable_shared_from_this<Value> {
protected:
  std::list<Use> m_use_list;  // users that use this value (def-use)
  template <typename Derived> std::shared_ptr<Derived> shared_from_base() {
    return std::static_pointer_cast<Derived>(shared_from_this());
  }

public:
  ValueType m_type;

  explicit Value() : m_type(BaseType::VOID) {}
  explicit Value(BaseType base_type) : m_type(base_type) {}
  explicit Value(ValueType value_type) : m_type(std::move(value_type)) {}
  explicit Value(bool is_float, bool is_ptr = false) : Value() {
    if (is_float) {
      m_type.Set(BaseType::FLOAT, is_ptr);
    } else {
      m_type.Set(BaseType::INT, is_ptr);
    }
  }

  ValueType GetType() const { return m_type; }

  std::string GetTypeString() { return m_type.ToString(); }

  void AppendUse(const Use &use) { m_use_list.push_back(use); }
  void RemoveUse(const Use &use) {
    for (auto it = m_use_list.begin(); it != m_use_list.end(); ++it) {
      if (it->m_value == use.m_value && it->m_user == use.m_user) {
        m_use_list.erase(it);
        return;
      }
    }
    assert(false);  // not found! what happen?
  }
  virtual void ExportIR(std::ofstream &ofs, int depth) = 0;
};

class Constant : public Value {
public:
  bool m_is_float;
  int m_int_val;
  float m_float_val;

  explicit Constant(int int_val)
      : Value(BaseType::INT),
        m_is_float(false),
        m_int_val(int_val),
        m_float_val(0.0) {}

  explicit Constant(float float_val)
      : Value(BaseType::FLOAT),
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
  // TODO(garen): record info about m_indices of array

  explicit GlobalVariable(const std::shared_ptr<DeclAST> &decl)
      : Value(decl->GetVarType() == VarType::FLOAT, decl->IsArray()),
        m_varname(decl->VarName()),
        m_is_const(decl->IsConst()),
        m_is_float(decl->GetVarType() == VarType::FLOAT),
        m_is_array(decl->IsArray()) {}
};

class IntGlobalVariable : public GlobalVariable {
public:
  std::vector<int> m_init_vals;

  explicit IntGlobalVariable(const std::shared_ptr<DeclAST> &decl)
      : GlobalVariable(decl), m_init_vals() {
    for (const auto &init_val : decl->m_flatten_vals) {
      if (init_val == nullptr)
        m_init_vals.push_back(0);
      else
        m_init_vals.push_back(init_val->IntVal());
    }

    std::vector<int> dimensions;
    for (auto &dimension : decl->m_dimensions) {
      dimensions.push_back(dimension->IntVal());
    }
    m_type.Set(BaseType::INT, std::move(dimensions), true);
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
    assert(!decl->IsArray());
    m_type.Set(BaseType::FLOAT, true);
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

  virtual bool IsTerminator() = 0;
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
    // assert(m_lhs_val_use.m_value->m_type == m_rhs_val_use.m_value->m_type);
    switch (op) {
      case IROp::ADD:
      case IROp::SUB:
      case IROp::MUL:
      case IROp::SDIV:
      case IROp::SREM:
        m_type.Set(BaseType::INT);
        break;
      case IROp::SGEQ:
      case IROp::SGE:
      case IROp::SLEQ:
      case IROp::SLE:
      case IROp::EQ:
      case IROp::NE:
        m_type.Set(BaseType::BOOL);
        break;
      case IROp::ZEXT:
        // no need to set?
        break;
      case IROp::CALL:
      case IROp::BRANCH:
      case IROp::JUMP:
      case IROp::RETURN:
      case IROp::ALLOCA:
      case IROp::LOAD:
      case IROp::STORE:
      case IROp::GET_ELEMENT_PTR:
      case IROp::PHI:
        assert(false);  // unreachable
        break;
    }
  }

  void ExportIR(std::ofstream &ofs, int depth) override;

  bool IsTerminator() override { return false; }
};

class Function;

class CallInstruction : public Instruction {
public:
  std::string m_func_name;
  std::vector<Use> m_params;

  std::shared_ptr<Function> m_function;

  explicit CallInstruction(VarType return_type, std::string func_name,
                           std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(IROp::CALL, std::move(bb)),
        m_func_name(std::move(func_name)),
        m_function(nullptr),
        m_params() {
    switch (return_type) {
      case VarType::INT:
        m_type.Set(BaseType::INT);
        break;
      case VarType::FLOAT:
        m_type.Set(BaseType::FLOAT);
        break;
      case VarType::VOID:
        m_type.Set(BaseType::VOID);
        break;
      default:
        assert(false);  // unreachable
    }
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }  // seems not terminator

  // should be called if params are needed
  void SetParams(std::vector<Use> params) { m_params = std::move(params); }
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
  bool IsTerminator() override { return true; }
};

class JumpInstruction : public Instruction {
public:
  std::shared_ptr<BasicBlock> m_target_block;

  explicit JumpInstruction(std::shared_ptr<BasicBlock> target_block,
                           std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(IROp::JUMP, std::move(bb)),
        m_target_block(std::move(target_block)) {}

  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return true; }
};

class ReturnInstruction : public Instruction {
public:
  Use m_ret;

  explicit ReturnInstruction(std::shared_ptr<Value> ret = nullptr,
                             std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(IROp::RETURN, std::move(bb)), m_ret(std::move(ret)) {}

  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return true; }

  bool IsConstant() const {
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
  std::vector<std::shared_ptr<Value>> m_indices;

  explicit GetElementPtrInstruction(std::shared_ptr<Value> addr,
                                    std::vector<std::shared_ptr<Value>> indices,
                                    std::shared_ptr<BasicBlock> bb = nullptr)
      : AccessInstruction(IROp::GET_ELEMENT_PTR, std::move(bb)),
        m_addr(std::move(addr)),
        m_indices(std::move(indices)) {
    // determine m_type
    m_type.Set(m_addr->m_type.m_base_type, true);
    m_type.m_dimensions.clear();

    // maybe wrong here
    for (size_t i = m_indices.size() - 1;
         i < m_addr->m_type.m_dimensions.size(); ++i) {
      m_type.m_dimensions.push_back(m_addr->m_type.m_dimensions[i]);
    }
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
};

class LoadInstruction : public AccessInstruction {
public:
  Use m_addr;
  explicit LoadInstruction(std::shared_ptr<Value> addr,
                           std::shared_ptr<BasicBlock> bb = nullptr)
      : AccessInstruction(IROp::LOAD, std::move(bb)), m_addr(std::move(addr)) {
    assert(m_addr.m_value->m_type.m_num_star >= 1);
    m_type = m_addr.m_value->m_type.Dereference();
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
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
  bool IsTerminator() override { return false; }
};

class AllocaInstruction : public Instruction {
public:
  std::shared_ptr<Value> m_init_val;

  explicit AllocaInstruction(ValueType value_type,
                             std::shared_ptr<Value> init_val = nullptr,
                             std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(IROp::ALLOCA, std::move(bb)),
        m_init_val(std::move(init_val)) {
    m_type = std::move(value_type);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
};

class PhiInstruction : public Instruction {
public:
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
};

class ZExtInstruction : public Instruction {
public:
  Use m_val;
  ValueType m_target_type;

  explicit ZExtInstruction(std::shared_ptr<Value> val, ValueType target_type,
                           std::shared_ptr<BasicBlock> bb = nullptr)
      : Instruction(IROp::ZEXT, std::move(bb)),
        m_val(std::move(val)),
        m_target_type(std::move(target_type)) {
    m_type = m_target_type;
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
};

class BasicBlock : public Value {
private:
  std::string m_name;

  std::list<std::shared_ptr<Instruction>> m_instr_list;

public:
  explicit BasicBlock(std::string name)
      : Value(BaseType::LABEL), m_name(std::move(name)) {}

  void PushBackInstruction(std::shared_ptr<Instruction> instr);
  void PushFrontInstruction(std::shared_ptr<Instruction> instr);

  void InsertFrontInstruction(const std::shared_ptr<Instruction> &elem,
                              std::shared_ptr<Instruction> instr);
  void InsertBackInstruction(const std::shared_ptr<Instruction> &elem,
                             std::shared_ptr<Instruction> instr);

  std::shared_ptr<Instruction> LastInstruction() { return m_instr_list.back(); }

  void ExportIR(std::ofstream &ofs, int depth) override;

  friend class Module;
};

class FunctionArg : public Value {
public:
  std::shared_ptr<DeclAST> m_decl;

  explicit FunctionArg(std::unique_ptr<FuncFParamAST> &arg)
      : m_decl(arg->m_decl) {
    std::vector<int> dimensions;
    if (m_decl->m_dimensions.size() >= 1) {
      assert(m_decl->m_dimensions[0] == nullptr);
    }
    for (int i = 1; i < m_decl->m_dimensions.size(); ++i) {
      dimensions.push_back(m_decl->m_dimensions[i]->IntVal());
    }
    if (m_decl->GetVarType() == VarType::INT) {
      m_type.Set(BaseType::INT, std::move(dimensions),
                 m_decl->DimensionsSize() >= 1);
    } else if (m_decl->GetVarType() == VarType::FLOAT) {
      m_type.Set(BaseType::FLOAT, std::move(dimensions),
                 m_decl->DimensionsSize() >= 1);
    } else
      assert(false);  // unreachable
    // if (m_decl->IsArray() && m_decl->IsParam()) {
    //   m_type = m_type.Reference();
    // }
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
};

// function declaration and function definition
class Function : public Value {
private:
  bool m_is_decl;
  std::string m_func_name;
  std::shared_ptr<FuncDefAST> m_func_ast;

  std::list<std::shared_ptr<BasicBlock>> m_bb_list;
  std::shared_ptr<BasicBlock> m_current_bb;

public:
  std::vector<std::shared_ptr<FunctionArg>> m_args;

  explicit Function(const std::shared_ptr<FuncDefAST> &func_ast)
      : m_func_ast(func_ast),
        m_func_name(func_ast->FuncName()),
        m_bb_list(),
        m_args(),
        m_is_decl(func_ast->m_is_builtin),
        m_current_bb(nullptr) {
    for (auto &param : func_ast->m_params) {
      auto arg = std::make_shared<FunctionArg>(param);
      m_args.push_back(arg);
    }
    switch (func_ast->ReturnType()) {
      case VarType::INT:
        m_type.Set(BaseType::INT);
        break;
      case VarType::FLOAT:
        m_type.Set(BaseType::FLOAT);
        break;
      case VarType::VOID:
        m_type.Set(BaseType::VOID);
        break;
      default:
        assert(false);  // unreachable
    }
  }

  [[nodiscard]] std::string FuncName() const { return m_func_name; }
  [[nodiscard]] VarType ReturnType() const { return m_func_ast->ReturnType(); }

  std::shared_ptr<BasicBlock> GetCurrentBB() { return m_current_bb; }

  void AppendBasicBlock(std::shared_ptr<BasicBlock> bb);

  void ExportIR(std::ofstream &ofs, int depth);

  friend class Module;
};

class Module {
private:
  std::shared_ptr<Function> m_current_func;

public:
  std::list<std::shared_ptr<Function>> m_function_list;
  std::list<std::shared_ptr<Function>> m_function_decl_list;
  std::list<std::shared_ptr<GlobalVariable>> m_global_variable_list;

  std::map<int, std::shared_ptr<Constant>> m_const_ints;
  std::map<float, std::shared_ptr<Constant>> m_const_floats;

  std::shared_ptr<BasicBlock> GetCurrentBB() {
    return m_current_func->GetCurrentBB();
  }
  std::shared_ptr<Function> GetCurrentFunc() { return m_current_func; }

  void AppendFunctionDecl(std::shared_ptr<Function> function_decl);
  void AppendFunction(std::shared_ptr<Function> function);
  void AppendGlobalVariable(std::shared_ptr<GlobalVariable> global_variable);
  void AppendBasicBlock(std::shared_ptr<BasicBlock> bb);

  void Check();
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

  std::shared_ptr<CallInstruction> CreateCallInstruction(VarType return_type,
                                                         std::string func_name);

  std::shared_ptr<CallInstruction> CreateCallInstruction(
      VarType return_type, std::string func_name,
      std::vector<std::shared_ptr<Value>> params);

  std::shared_ptr<Instruction> CreateBranchInstruction(
      std::shared_ptr<Value> cond_val, std::shared_ptr<BasicBlock> true_block,
      std::shared_ptr<BasicBlock> false_block);

  std::shared_ptr<Instruction> CreateJumpInstruction(
      std::shared_ptr<BasicBlock> block);

  std::shared_ptr<Instruction> CreateReturnInstruction(
      std::shared_ptr<Value> ret = nullptr);

  std::shared_ptr<Instruction> CreateGetElementPtrInstruction(
      std::shared_ptr<Value> addr, std::vector<std::shared_ptr<Value>> indices);

  std::shared_ptr<Instruction> CreateLoadInstruction(
      std::shared_ptr<Value> addr);

  std::shared_ptr<Instruction> CreateStoreInstruction(
      std::shared_ptr<Value> addr, std::shared_ptr<Value> val);

  std::shared_ptr<Instruction> CreateAllocaInstruction(
      std::shared_ptr<DeclAST> decl, std::shared_ptr<Value> init_val = nullptr);

  std::shared_ptr<GlobalVariable> CreateGlobalVariable(
      const std::shared_ptr<DeclAST> &decl);
  std::shared_ptr<IntGlobalVariable> CreateIntGlobalVariable(
      std::shared_ptr<DeclAST> decl);
  std::shared_ptr<FloatGlobalVariable> CreateFloatGlobalVariable(
      std::shared_ptr<DeclAST> decl);

  std::shared_ptr<Value> CreateZExtInstruction(std::shared_ptr<Value> from,
                                               ValueType type_to);

  std::shared_ptr<Value> GetIntConstant(int int_val);
  std::shared_ptr<Value> GetFloatConstant(float float_val);
};

#endif  // BDDD_IR_H
