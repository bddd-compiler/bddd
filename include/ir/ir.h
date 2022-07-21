#ifndef BDDD_IR_H
#define BDDD_IR_H

#include <algorithm>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "ast/ast.h"
#include "ir/ir-name-allocator.h"
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

// can only be initialized using make_unique
class Use {
public:
  std::shared_ptr<Value> m_value;  // the value
  std::shared_ptr<Value> m_user;   // who use the value

  explicit Use() : m_value(nullptr), m_user(nullptr) {}
  explicit Use(std::shared_ptr<Value> value, std::shared_ptr<Value> user)
      : m_value(std::move(value)), m_user(std::move(user)) {
    // assert(value != nullptr);
    // UseValue(value);
    // if (user != nullptr) InitUser(user);
  }

  Use(const Use &) = delete;

  // IMPORTANT:
  // must be done after construction in IRBuilder::CreateXXX
  // only after this function, the whole initialization is complete
  // void InitUser(std::shared_ptr<Value> user);

  // modify the value
  void UseValue(std::shared_ptr<Value> value);

  void RemoveFromUseList();

  bool operator==(const Use &rhs) const {
    return m_value == rhs.m_value && m_user == rhs.m_user;
  }
};

class BasicBlock;

// for simplification, value and user are composite together
class Value : public std::enable_shared_from_this<Value> {
public:
  // record who use me (but maybe replaced, i.e. m_value may not be itself)
  // user should be unique in m_use_list (promise me)
  // m_use_list should logically own all uses (modification in m_use_list can
  // make differences into other related instructions)
  std::list<std::shared_ptr<Use>> m_use_list;
  ValueType m_type;
  std::string m_allocated_name;

  // new things
  std::shared_ptr<Value> m_reaching_def;
  std::shared_ptr<BasicBlock> m_bb;  // belong to which basic block

  template <typename Derived> std::shared_ptr<Derived> shared_from_base() {
    return std::static_pointer_cast<Derived>(shared_from_this());
  }

  explicit Value(std::shared_ptr<BasicBlock> bb = nullptr)
      : m_type(BaseType::VOID), m_bb(std::move(bb)) {}

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

  std::shared_ptr<Use> AddUse(const std::shared_ptr<Value> &user);

  void KillUse(const std::shared_ptr<Value> &user);

  void ReplaceUseBy(const std::shared_ptr<Value> &new_val);

  virtual void AllocateName(std::shared_ptr<IRNameAllocator> allocator) = 0;
  virtual void ExportIR(std::ofstream &ofs, int depth) = 0;
};

class Constant : public Value {
public:
  bool m_is_float;
  int m_int_val;
  float m_float_val;
  // constants do not belong to any bb

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

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
};

class GlobalVariable : public Value {
public:
  std::string m_name;  // get from user
  bool m_is_const;
  bool m_is_float;
  bool m_is_array;
  // TODO(garen): record info about m_indices of array

  // global variables do not belong to any bb

  explicit GlobalVariable(const std::shared_ptr<DeclAST> &decl)
      : Value(decl->GetVarType() == VarType::FLOAT, decl->IsArray()),
        m_name(decl->VarName()),
        m_is_const(decl->IsConst()),
        m_is_float(decl->GetVarType() == VarType::FLOAT),
        m_is_array(decl->IsArray()) {}

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
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
class CallInstruction;

class Instruction : public Value {
public:
  IROp m_op;

  explicit Instruction(IROp op, std::shared_ptr<BasicBlock> bb)
      : Value(std::move(bb)), m_op(op) {}

  bool HasSideEffect();

  virtual bool IsTerminator() = 0;
};

class BinaryInstruction : public Instruction {
public:
  std::shared_ptr<Use> m_lhs_val_use;
  std::shared_ptr<Use> m_rhs_val_use;

  explicit BinaryInstruction(IROp op, std::shared_ptr<Value> lhs_val,
                             std::shared_ptr<Value> rhs_val,
                             std::shared_ptr<BasicBlock> bb)
      : Instruction(op, std::move(bb)),
        m_lhs_val_use(nullptr),
        m_rhs_val_use(nullptr) {
    // assert(m_lhs_val_use.m_value->m_type == m_rhs_val_use.m_value->m_type);
    std::shared_ptr<BinaryInstruction> temp(this);  // to allow shared_from_this
    m_lhs_val_use = lhs_val->AddUse(shared_from_this());
    m_rhs_val_use = rhs_val->AddUse(shared_from_this());
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

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  bool IsTerminator() override { return false; }
};

class Function;

class CallInstruction : public Instruction {
public:
  std::string m_func_name;
  std::vector<std::shared_ptr<Use>> m_params;

  std::shared_ptr<Function> m_function;

  explicit CallInstruction(VarType return_type, std::string func_name,
                           std::shared_ptr<BasicBlock> bb)
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

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;

  bool IsTerminator() override { return false; }  // seems not terminator

  // should be called if params are needed
  void SetParams(std::vector<std::shared_ptr<Use>> params) {
    m_params = std::move(params);
  }
};

class BranchInstruction : public Instruction {
public:
  std::shared_ptr<Use> m_cond;
  std::shared_ptr<BasicBlock> m_true_block, m_false_block;

  explicit BranchInstruction(std::shared_ptr<Value> cond,
                             std::shared_ptr<BasicBlock> true_block,
                             std::shared_ptr<BasicBlock> false_block,
                             std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::BRANCH, std::move(bb)),
        m_cond(nullptr),
        m_true_block(std::move(true_block)),
        m_false_block(std::move(false_block)) {
    std::shared_ptr<BranchInstruction> temp(this);
    m_cond = cond->AddUse(shared_from_this());
  }

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return true; }
};

class JumpInstruction : public Instruction {
public:
  std::shared_ptr<BasicBlock> m_target_block;

  explicit JumpInstruction(std::shared_ptr<BasicBlock> target_block,
                           std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::JUMP, std::move(bb)),
        m_target_block(std::move(target_block)) {}

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return true; }
};

class ReturnInstruction : public Instruction {
public:
  std::shared_ptr<Use> m_ret;

  explicit ReturnInstruction(std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::RETURN, std::move(bb)), m_ret(nullptr) {
    // TODO(garen): problem here!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // std::shared_ptr<Value> temp(this);
    // if (ret != nullptr) {
    //   m_ret = ret->AddUse(this);
    // }
  }

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return true; }

  bool IsConstant() const {
    return std::dynamic_pointer_cast<Constant>(m_ret->m_value) != nullptr;
  }
};

class AccessInstruction : public Instruction {
public:
  explicit AccessInstruction(IROp op, std::shared_ptr<BasicBlock> bb)
      : Instruction(op, std::move(bb)) {}
};

class GetElementPtrInstruction : public AccessInstruction {
public:
  std::shared_ptr<Use> m_addr;
  std::vector<std::shared_ptr<Use>> m_indices;

  explicit GetElementPtrInstruction(std::shared_ptr<Value> addr,
                                    std::vector<std::shared_ptr<Value>> indices,
                                    std::shared_ptr<BasicBlock> bb)
      : AccessInstruction(IROp::GET_ELEMENT_PTR, std::move(bb)),
        m_addr(nullptr),
        m_indices() {
    std::shared_ptr<Value> test(this);
    m_addr = addr->AddUse(shared_from_this());
    for (auto index : indices) {
      m_indices.push_back(index->AddUse(shared_from_this()));
    }

    // determine m_type
    m_type.Set(addr->m_type.m_base_type, true);
    m_type.m_dimensions.clear();

    // TODO(garen): maybe wrong here
    for (size_t i = m_indices.size() - 1; i < addr->m_type.m_dimensions.size();
         ++i) {
      m_type.m_dimensions.push_back(addr->m_type.m_dimensions[i]);
    }
  }

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
};

class LoadInstruction : public AccessInstruction {
public:
  std::shared_ptr<Use> m_addr;
  explicit LoadInstruction(std::shared_ptr<Value> addr,
                           std::shared_ptr<BasicBlock> bb)
      : AccessInstruction(IROp::LOAD, std::move(bb)), m_addr(nullptr) {
    assert(addr != nullptr);
    assert(addr->m_type.m_num_star >= 1);
    m_type = addr->m_type.Dereference();
    std::shared_ptr<Value> temp(this);
    m_addr = addr->AddUse(shared_from_this());
  }

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
};

class StoreInstruction : public AccessInstruction {
public:
  std::shared_ptr<Use> m_addr;
  std::shared_ptr<Use> m_val;

  explicit StoreInstruction(std::shared_ptr<Value> addr,
                            std::shared_ptr<Value> val,
                            std::shared_ptr<BasicBlock> bb)
      : AccessInstruction(IROp::STORE, std::move(bb)),
        m_addr(nullptr),
        m_val(nullptr) {
    std::shared_ptr<Value> temp(this);
    m_addr = addr->AddUse(shared_from_this());
    m_val = val->AddUse(shared_from_this());
  }

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
};

class AllocaInstruction : public Instruction {
public:
  std::shared_ptr<Value> m_init_val;

  // new things
  size_t m_alloca_id;
  std::vector<std::shared_ptr<BasicBlock>> m_defs;

  explicit AllocaInstruction(ValueType value_type,
                             std::shared_ptr<BasicBlock> bb,
                             std::shared_ptr<Value> init_val = nullptr)
      : Instruction(IROp::ALLOCA, std::move(bb)),
        m_init_val(std::move(init_val)),
        m_alloca_id(0) {
    m_type = std::move(value_type);
  }

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
};

// TODO(garen): initialization from IRBuilder
class PhiInstruction : public Instruction {
public:
  std::unordered_map<std::shared_ptr<BasicBlock>, std::shared_ptr<Use>>
      m_contents;

  explicit PhiInstruction(ValueType type, std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::PHI, std::move(bb)) {
    m_type = type;
  }

  // return nullptr if nothing found
  std::shared_ptr<Value> GetValue(std::shared_ptr<BasicBlock> bb) {
    auto it = std::find_if(
        m_contents.begin(), m_contents.end(),
        [=](const auto &x) { return x.first.get() == bb.get(); });
    if (it == m_contents.end())
      return nullptr;
    else
      return it->second->m_value;
  }

  void AddPhiOperand(std::shared_ptr<BasicBlock> bb,
                     std::shared_ptr<Value> val) {
    assert(val != nullptr);
    auto it = std::find_if(
        m_contents.begin(), m_contents.end(),
        [=](const auto &x) { return x.first.get() == bb.get(); });
    if (it == m_contents.end()) {
      m_contents[bb] = val->AddUse(shared_from_this());
    } else {
      // na mei shi le
    }
  }

  void Remove(std::shared_ptr<BasicBlock> bb) { m_contents.erase(bb); }

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
};

class ZExtInstruction : public Instruction {
public:
  std::shared_ptr<Use> m_val;
  ValueType m_target_type;

  explicit ZExtInstruction(std::shared_ptr<Value> val, ValueType target_type,
                           std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::ZEXT, std::move(bb)),
        m_val(nullptr),
        m_target_type(std::move(target_type)) {
    m_type = m_target_type;
    std::shared_ptr<Value> temp(this);
    m_val = val->AddUse(shared_from_this());
  }

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
};

class BasicBlock : public Value {
private:
  std::string m_name;

public:
  std::list<std::shared_ptr<Instruction>> m_instr_list;

  // new things
  size_t m_id;                         // for calculating dominance relationship
  std::shared_ptr<BasicBlock> m_idom;  // immediate dominator
  std::vector<std::shared_ptr<BasicBlock>> m_dominators;  // 我支配谁
  std::vector<std::shared_ptr<BasicBlock>> m_dominated;   // 我被谁支配
  std::unordered_set<std::shared_ptr<BasicBlock>> m_dominance_frontier;
  bool m_visited;

  // m_bb = nullptr (otherwise self-loop)

  // methods

  explicit BasicBlock(std::string name)
      : Value(BaseType::LABEL),
        m_name(std::move(name)),
        m_id(0),
        m_visited(false) {}

  void PushBackInstruction(std::shared_ptr<Instruction> instr);
  void PushFrontInstruction(std::shared_ptr<Instruction> instr);

  void InsertFrontInstruction(const std::shared_ptr<Instruction> &elem,
                              std::shared_ptr<Instruction> instr);
  void InsertBackInstruction(const std::shared_ptr<Instruction> &elem,
                             std::shared_ptr<Instruction> instr);
  void RemoveInstruction(const std::shared_ptr<Instruction> &elem);

  std::shared_ptr<Instruction> LastInstruction() { return m_instr_list.back(); }

  std::vector<std::shared_ptr<BasicBlock>> Successors();

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;

  std::list<std::shared_ptr<Instruction>> GetInstList();

  friend class Module;
};

class FunctionArg : public Value {
public:
  std::shared_ptr<DeclAST> m_decl;
  bool m_is_decl;  // its parent function is declaration or definition

  // m_bb = nullptr

  explicit FunctionArg(std::unique_ptr<FuncFParamAST> &arg, bool is_decl)
      : m_decl(arg->m_decl), m_is_decl(is_decl) {
    std::vector<int> dimensions;
    if (!m_decl->m_dimensions.empty()) {
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
    } else {
      assert(false);  // unreachable
    }
    // if (m_decl->IsArray() && m_decl->IsParam()) {
    //   m_type = m_type.Reference();
    // }
  }

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
};

// function declaration and function definition
class Function : public Value {
private:
  bool m_is_decl;
  std::string m_func_name;
  std::shared_ptr<FuncDefAST> m_func_ast;

  std::shared_ptr<BasicBlock> m_current_bb;

public:
  std::vector<std::shared_ptr<FunctionArg>> m_args;
  std::list<std::shared_ptr<BasicBlock>> m_bb_list;

  explicit Function(const std::shared_ptr<FuncDefAST> &func_ast)
      : m_func_ast(func_ast),
        m_func_name(func_ast->FuncName()),
        m_bb_list(),
        m_args(),
        m_is_decl(func_ast->m_is_builtin),
        m_current_bb(nullptr) {
    for (auto &param : func_ast->m_params) {
      auto arg = std::make_shared<FunctionArg>(param, m_is_decl);
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

  bool HasSideEffect() {
    // TODO(garen): unimplemented
    assert(false);
  }

  void AppendBasicBlock(std::shared_ptr<BasicBlock> bb);

  std::list<std::shared_ptr<BasicBlock>> GetBlockList();

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
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

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator);
};

#endif  // BDDD_IR_H
