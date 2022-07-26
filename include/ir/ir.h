#ifndef BDDD_IR_H
#define BDDD_IR_H

#include <algorithm>
#include <iostream>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "ast/ast.h"
#include "ir/ir-name-allocator.h"

enum class IROp {
  ADD,
  F_ADD,
  SUB,
  F_SUB,
  MUL,
  F_MUL,
  SDIV,
  F_DIV,
  SREM,
  F_NEG,

  // icmp operands
  I_SGE,
  I_SGT,
  I_SLE,
  I_SLT,
  I_EQ,
  I_NE,
  // fcmp operands
  F_EQ,
  F_NE,
  F_GT,
  F_GE,
  F_LT,
  F_LE,

  XOR,

  CALL,
  BRANCH,
  JUMP,
  RETURN,
  ALLOCA,
  LOAD,
  STORE,
  GET_ELEMENT_PTR,
  PHI,

  BITCAST,
  // extensions
  ZEXT,    // bool to int
  SITOFP,  // signed int to floating point
  FPTOSI,  // floating point to singed int
};

enum class BasicType {
  INT,    // i32 (array)
  FLOAT,  // float (array)
  CHAR,   // i8
  BOOL,   // i1

  LABEL,  // basic block
  VOID,   // e.g. store instruction, return instruction is void
};

class ValueType {
public:
  BasicType m_base_type;
  std::vector<int> m_dimensions;
  int m_num_star;  // have *
  bool m_is_const;

  explicit ValueType(BasicType base_type, int ptr_cnt = 0,
                     bool is_const = false)
      : m_base_type(base_type),
        m_dimensions(),
        m_num_star(ptr_cnt),
        m_is_const(is_const) {}

  explicit ValueType(BasicType base_type, std::vector<int> dimensions,
                     int ptr_cnt = 0, bool is_const = false)
      : m_base_type(base_type),
        m_dimensions(std::move(dimensions)),
        m_num_star(ptr_cnt),
        m_is_const(is_const) {}

  explicit ValueType(VarType var_type, int ptr_cnt = 0, bool is_const = false)
      : m_base_type(),
        m_dimensions(),
        m_num_star(ptr_cnt),
        m_is_const(is_const) {
    switch (var_type) {
      case VarType::INT:
        m_base_type = BasicType::INT;
        break;
      case VarType::FLOAT:
        m_base_type = BasicType::FLOAT;
        break;
      case VarType::CHAR:
        m_base_type = BasicType::CHAR;
        break;
      case VarType::VOID:
        m_base_type = BasicType::VOID;
        break;
      case VarType::BOOL:
        m_base_type = BasicType::BOOL;
        break;
      case VarType::UNKNOWN:
        assert(false);  // unreachable
    }
  }

  explicit ValueType(VarType var_type, std::vector<int> dimensions,
                     int ptr_cnt = 0, bool is_const = false)
      : ValueType(var_type, ptr_cnt, is_const) {
    m_dimensions = std::move(dimensions);
  }

  [[nodiscard]] bool IsConst() const { return m_is_const; }

  [[nodiscard]] bool IsBasicVariable() const {
    return m_dimensions.empty() && m_num_star == 0
           && (m_base_type == BasicType::INT || m_base_type == BasicType::FLOAT
               || m_base_type == BasicType::BOOL);
  }
  [[nodiscard]] bool IsBasicInt() const {
    return m_dimensions.empty() && m_num_star == 0
           && m_base_type == BasicType::INT;
  }
  [[nodiscard]] bool IsBasicFloat() const {
    return m_dimensions.empty() && m_num_star == 0
           && m_base_type == BasicType::FLOAT;
  }
  [[nodiscard]] bool IsBasicBool() const {
    return m_dimensions.empty() && m_num_star == 0
           && m_base_type == BasicType::BOOL;
  }

  [[nodiscard]] bool IsLabel() const { return m_base_type == BasicType::LABEL; }

  ValueType Dereference(int x = 1) {
    assert(m_num_star > 0);
    auto ret = *this;
    ret.m_num_star -= x;
    return ret;
  }

  ValueType Reference(int x = 1) {
    auto ret = *this;
    ret.m_num_star += x;
    return ret;
  }

  ValueType Reduce(int x) {
    assert(x <= m_dimensions.size());
    auto ret = *this;
    ret.m_dimensions.clear();
    for (int i = x; i < m_dimensions.size(); i++) {
      ret.m_dimensions.push_back(m_dimensions[i]);
    }
    return ret;
  }

  void Set(BasicType base_type, bool is_ptr = false, bool is_const = false) {
    m_base_type = base_type;
    m_num_star = is_ptr;
    m_is_const = is_const;
    m_dimensions.clear();
  }

  void Set(BasicType base_type, std::vector<int> dimensions,
           bool is_ptr = false, bool is_const = false) {
    m_base_type = base_type;
    m_dimensions = std::move(dimensions);
    m_num_star = is_ptr;
    m_is_const = is_const;
  }

  bool Equals(BasicType base_type, bool is_ptr = false) const {
    return m_base_type == base_type && m_num_star == is_ptr
           && m_dimensions.empty();
  }

  // std::string ToString() {
  // }

  bool operator==(const ValueType &rhs) const {
    // is_const does not matter since we do not care about const arguments
    // example: const value is the parameter to the non-const argument
    if (m_base_type != rhs.m_base_type || m_num_star != rhs.m_num_star)
      return false;
    if (m_dimensions.size() != rhs.m_dimensions.size()) return false;
    for (int i = 0; i < m_dimensions.size(); ++i) {
      if (m_dimensions[i] != rhs.m_dimensions[i]) return false;
    }
    return true;
  }

  bool operator!=(const ValueType &rhs) const { return !(*this == rhs); }
};

std::ostream &operator<<(std::ostream &out, BasicType base_type);
std::ostream &operator<<(std::ostream &out, ValueType value_type);

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
  // std::string m_allocated_name;

  // new things
  std::shared_ptr<BasicBlock> m_bb;  // belong to which basic block

  template <typename Derived> std::shared_ptr<Derived> shared_from_base() {
    return std::static_pointer_cast<Derived>(shared_from_this());
  }

  explicit Value(std::shared_ptr<BasicBlock> bb = nullptr)
      : m_type(BasicType::VOID), m_bb(std::move(bb)) {}

  explicit Value(BasicType base_type, bool is_const = false)
      : m_type(base_type, 0, is_const) {}
  explicit Value(ValueType value_type) : m_type(std::move(value_type)) {}
  explicit Value(bool is_float, int num_star = 0, bool is_const = false)
      : Value() {
    if (is_float) {
      m_type.Set(BasicType::FLOAT, num_star, is_const);
    } else {
      m_type.Set(BasicType::INT, num_star, is_const);
    }
  }

  ValueType GetType() const { return m_type; }

  std::shared_ptr<Use> AddUse(const std::shared_ptr<Value> &user);

  void KillUse(const std::shared_ptr<Value> &user);
  void KillAllUses();

  void ReplaceUseBy(const std::shared_ptr<Value> &new_val);

  // virtual void AllocateName(std::shared_ptr<IRNameAllocator> allocator) = 0;
  virtual void ExportIR(std::ofstream &ofs, int depth) = 0;
};

class Constant : public Value {
public:
  int m_int_val;
  float m_float_val;
  // constants may belong to any bb, which is useful in mem2reg

  // non-float
  explicit Constant(int int_val, BasicType basic_type,
                    std::shared_ptr<BasicBlock> bb)
      : Value(basic_type, true), m_int_val(int_val), m_float_val(0.0) {
    assert(basic_type == BasicType::INT || basic_type == BasicType::BOOL
           || basic_type == BasicType::CHAR);
    m_bb = std::move(bb);
    assert(m_bb != nullptr);
  }

  explicit Constant(float float_val, std::shared_ptr<BasicBlock> bb)
      : Value(BasicType::FLOAT, true), m_int_val(0), m_float_val(float_val) {
    m_bb = std::move(bb);
    assert(m_bb != nullptr);
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;

  EvalValue Evaluate() {
    switch (m_type.m_base_type) {
      case BasicType::INT:
      case BasicType::CHAR:
        return EvalValue(m_int_val);
        return EvalValue(m_int_val);
      case BasicType::FLOAT:
        return EvalValue(m_float_val);
      default:
        assert(false);  // ???
    }
  }

  // bool IsFloat() const { return m_type.Equals(BasicType::FLOAT); }
  // bool IsInt() const { return m_type.Equals(BasicType::INT); }
  // bool IsBool() const { return m_type.Equals(BasicType::BOOL); }
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

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
};

class IntGlobalVariable : public GlobalVariable {
public:
  std::vector<int> m_flatten_vals;
  std::unique_ptr<InitValAST> &m_init_val;

  explicit IntGlobalVariable(const std::shared_ptr<DeclAST> &decl)
      : GlobalVariable(decl), m_flatten_vals(), m_init_val(decl->m_init_val) {
    for (const auto &init_val : decl->m_flatten_vals) {
      if (init_val == nullptr)
        m_flatten_vals.push_back(0);
      else
        m_flatten_vals.push_back(init_val->IntVal());
    }

    std::vector<int> dimensions;
    for (auto &dimension : decl->m_dimensions) {
      dimensions.push_back(dimension->IntVal());
    }
    m_type.Set(BasicType::INT, std::move(dimensions), true);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class FloatGlobalVariable : public GlobalVariable {
public:
  std::vector<float> m_flatten_vals;
  std::unique_ptr<InitValAST> &m_init_val;

  explicit FloatGlobalVariable(const std::shared_ptr<DeclAST> &decl)
      : GlobalVariable(decl), m_flatten_vals(), m_init_val(decl->m_init_val) {
    for (const auto &init_val : decl->m_flatten_vals) {
      if (init_val == nullptr)
        m_flatten_vals.push_back(0);
      else
        m_flatten_vals.push_back(init_val->FloatVal());
    }

    std::vector<int> dimensions;
    for (auto &dimension : decl->m_dimensions) {
      dimensions.push_back(dimension->IntVal());
    }
    m_type.Set(BasicType::FLOAT, std::move(dimensions), true);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
};

class BasicBlock;
class CallInstruction;

class Instruction : public Value {
public:
  IROp m_op;
  bool m_visited;  // used in GCM

  explicit Instruction(IROp op, std::shared_ptr<BasicBlock> bb)
      : Value(std::move(bb)), m_op(op), m_visited(false) {}

  bool HasSideEffect();

  virtual bool IsTerminator() = 0;
  virtual std::vector<std::shared_ptr<Use>> Operands() = 0;
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
    m_lhs_val_use = lhs_val->AddUse(temp);
    m_rhs_val_use = rhs_val->AddUse(temp);
    switch (op) {
      case IROp::ADD:
      case IROp::SUB:
      case IROp::MUL:
      case IROp::SDIV:
      case IROp::SREM:
      case IROp::ZEXT:
      case IROp::FPTOSI:
        m_type.Set(BasicType::INT);
        break;
      case IROp::I_SGE:
      case IROp::I_SGT:
      case IROp::I_SLE:
      case IROp::I_SLT:
      case IROp::I_EQ:
      case IROp::I_NE:
      case IROp::F_EQ:
      case IROp::F_NE:
      case IROp::F_GT:
      case IROp::F_GE:
      case IROp::F_LT:
      case IROp::F_LE:
      case IROp::XOR:  // TODO(garen): temp solution
        m_type.Set(BasicType::BOOL);
        break;
      case IROp::F_ADD:
      case IROp::F_SUB:
      case IROp::F_MUL:
      case IROp::F_DIV:
      case IROp::SITOFP:
        m_type.Set(BasicType::FLOAT);
        break;
      default:
        assert(false);  // unreachable
    }
  }

  void ExportIR(std::ofstream &ofs, int depth) override;

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  bool IsTerminator() override { return false; }

  std::vector<std::shared_ptr<Use>> Operands() override {
    return {m_lhs_val_use, m_rhs_val_use};
  }
};

class FNegInstruction : public Instruction {
public:
  std::shared_ptr<Use> m_lhs_val_use;

  explicit FNegInstruction(std::shared_ptr<Value> lhs_val,
                           std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::F_NEG, std::move(bb)), m_lhs_val_use(nullptr) {
    std::shared_ptr<FNegInstruction> temp(this);  // to allow shared_from_this
    m_lhs_val_use = lhs_val->AddUse(temp);
    m_type.Set(BasicType::FLOAT);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  bool IsTerminator() override { return false; }

  std::vector<std::shared_ptr<Use>> Operands() override {
    return {m_lhs_val_use};
  }
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
        m_type.Set(BasicType::INT);
        break;
      case VarType::FLOAT:
        m_type.Set(BasicType::FLOAT);
        break;
      case VarType::VOID:
        m_type.Set(BasicType::VOID);
        break;
      default:
        assert(false);  // unreachable
    }
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;

  bool IsTerminator() override { return false; }  // seems not terminator
  std::vector<std::shared_ptr<Use>> Operands() override { return m_params; }

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
    m_cond = cond->AddUse(temp);
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return true; }
  std::vector<std::shared_ptr<Use>> Operands() override { return {m_cond}; }
};

class JumpInstruction : public Instruction {
public:
  std::shared_ptr<BasicBlock> m_target_block;

  explicit JumpInstruction(std::shared_ptr<BasicBlock> target_block,
                           std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::JUMP, std::move(bb)),
        m_target_block(std::move(target_block)) {}

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return true; }
  std::vector<std::shared_ptr<Use>> Operands() override { return {}; }
};

class ReturnInstruction : public Instruction {
public:
  std::shared_ptr<Use> m_ret;  // WARNING: nullable

  explicit ReturnInstruction(std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::RETURN, std::move(bb)), m_ret(nullptr) {
    // TODO(garen): problem here!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // std::shared_ptr<Value> temp(this);
    // if (ret != nullptr) {
    //   m_ret = ret->AddUse(this);
    // }
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return true; }
  std::vector<std::shared_ptr<Use>> Operands() override {
    if (m_ret)
      return {m_ret};
    else
      return {};
  }

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
    std::shared_ptr<Value> temp(this);
    m_addr = addr->AddUse(temp);
    for (auto index : indices) {
      m_indices.push_back(index->AddUse(temp));
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

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
  std::vector<std::shared_ptr<Use>> Operands() override {
    std::vector<std::shared_ptr<Use>> ret;
    ret.push_back(m_addr);
    ret.insert(ret.end(), m_indices.begin(), m_indices.end());
    return std::move(ret);
  }
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
    m_addr = addr->AddUse(temp);
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
  std::vector<std::shared_ptr<Use>> Operands() override { return {m_addr}; }
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
    m_addr = addr->AddUse(temp);
    m_val = val->AddUse(temp);
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }

  std::vector<std::shared_ptr<Use>> Operands() override {
    return {m_addr, m_val};
  }
};

class AllocaInstruction : public Instruction {
public:
  bool m_is_arg;
  bool m_is_const;

  // used in mem2reg
  int m_alloca_id;
  std::vector<std::shared_ptr<BasicBlock>> m_defs;

  explicit AllocaInstruction(ValueType value_type,
                             std::shared_ptr<BasicBlock> bb,
                             bool is_arg = false, bool is_const = false)
      : Instruction(IROp::ALLOCA, std::move(bb)),
        m_alloca_id(-1),
        m_is_arg(is_arg),
        m_is_const(is_const) {
    m_type = std::move(value_type);
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
  std::vector<std::shared_ptr<Use>> Operands() override { return {}; }
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
    auto it = std::find_if(
        m_contents.begin(), m_contents.end(),
        [=](const auto &x) { return x.first.get() == bb.get(); });
    if (it == m_contents.end()) {
      if (val == nullptr)
        m_contents[bb] = nullptr;
      else
        m_contents[bb] = val->AddUse(shared_from_this());
    } else {
      // na mei shi le
    }
  }

  bool IsValid();

  void Remove(std::shared_ptr<Value> val) {
    for (auto it = m_contents.begin(); it != m_contents.end(); ++it) {
      if (it->second->m_value == val) {
        m_contents.erase(it);
        return;
      }
    }
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
  std::vector<std::shared_ptr<Use>> Operands() override {
    std::vector<std::shared_ptr<Use>> ret;
    for (auto [_, use] : m_contents) {
      ret.push_back(use);
    }
    return std::move(ret);
  }
};

// fat pointer to thin pointer
// i32* to i8*, float* to i8*
class BitCastInstruction : public Instruction {
public:
  std::shared_ptr<Use> m_val;
  BasicType m_target_type;  // with *
  explicit BitCastInstruction(const std::shared_ptr<Value> &val,
                              BasicType target_type,
                              std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::BITCAST, std::move(bb)),
        m_val(),
        m_target_type(target_type) {
    m_type.Set(target_type, true);
    std::shared_ptr<Value> temp(this);
    m_val = val->AddUse(temp);
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
  std::vector<std::shared_ptr<Use>> Operands() override { return {m_val}; }
};

// i1 to i32
class ZExtInstruction : public Instruction {
public:
  std::shared_ptr<Use> m_val;

  explicit ZExtInstruction(std::shared_ptr<Value> val,
                           std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::ZEXT, std::move(bb)), m_val(nullptr) {
    m_type.Set(BasicType::INT);
    std::shared_ptr<ZExtInstruction> temp(this);
    m_val = val->AddUse(temp);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  bool IsTerminator() override { return false; }
  std::vector<std::shared_ptr<Use>> Operands() override { return {m_val}; }
};

// i32 to float
class SIToFPInstruction : public Instruction {
public:
  std::shared_ptr<Use> m_val;
  explicit SIToFPInstruction(std::shared_ptr<Value> val,
                             std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::SITOFP, std::move(bb)), m_val(nullptr) {
    m_type.Set(BasicType::FLOAT);
    std::shared_ptr<SIToFPInstruction> temp(this);
    m_val = val->AddUse(temp);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  bool IsTerminator() override { return false; }
  std::vector<std::shared_ptr<Use>> Operands() override { return {m_val}; }
};

// float to i32
class FPToSIInstruction : public Instruction {
public:
  std::shared_ptr<Use> m_val;
  explicit FPToSIInstruction(std::shared_ptr<Value> val,
                             std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::FPTOSI, std::move(bb)), m_val(nullptr) {
    m_type.Set(BasicType::INT);
    std::shared_ptr<FPToSIInstruction> temp(this);
    m_val = val->AddUse(temp);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  bool IsTerminator() override { return false; }
  std::vector<std::shared_ptr<Use>> Operands() override { return {m_val}; }
};

class BasicBlock : public Value {
private:
  std::string m_name;

public:
  std::list<std::shared_ptr<Instruction>> m_instr_list;

  // members used in ComputeDominanceRelationship
  size_t m_id;                         // for calculating dominance relationship
  std::shared_ptr<BasicBlock> m_idom;  // immediate dominator
  std::unordered_set<std::shared_ptr<BasicBlock>>
      m_dominators;  // 我支配谁（可能是自己）
  std::unordered_set<std::shared_ptr<BasicBlock>>
      m_dominated;  // 我被谁支配（可能是自己）
  std::unordered_set<std::shared_ptr<BasicBlock>> m_dominance_frontier;
  int m_dom_depth;  // depth in dominance tree

  // members used in ComputeLoopInfo
  int m_loop_depth;  // depth in loop (see ComputeLoopInfo)

  bool m_visited;  // first used in mem2reg
  std::vector<std::shared_ptr<BasicBlock>> m_predecessors;

  // m_bb = nullptr (otherwise self-loop)

  // methods

  explicit BasicBlock(std::string name)
      : Value(BasicType::LABEL),
        m_name(std::move(name)),
        m_id(0),
        m_dom_depth(-1),
        m_visited(false) {}

  void PushBackInstruction(std::shared_ptr<Instruction> instr);
  void PushFrontInstruction(std::shared_ptr<Instruction> instr);

  void InsertFrontInstruction(const std::shared_ptr<Instruction> &elem,
                              std::shared_ptr<Instruction> instr);
  void InsertBackInstruction(const std::shared_ptr<Instruction> &elem,
                             std::shared_ptr<Instruction> instr);
  void RemoveInstruction(const std::shared_ptr<Instruction> &elem);

  std::shared_ptr<Instruction> LastInstruction() {
    assert(!m_instr_list.empty());
    return m_instr_list.back();
  }

  std::vector<std::shared_ptr<BasicBlock>> Predecessors();
  std::vector<std::shared_ptr<BasicBlock>> Successors();

  bool Dominate(std::shared_ptr<BasicBlock> bb);
  bool IsDominatedBy(std::shared_ptr<BasicBlock> bb);

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
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

    switch (m_decl->GetVarType()) {
      case VarType::INT:
        m_type.Set(BasicType::INT, std::move(dimensions),
                   m_decl->DimensionsSize() >= 1);
        break;
      case VarType::FLOAT:
        m_type.Set(BasicType::FLOAT, std::move(dimensions),
                   m_decl->DimensionsSize() >= 1);
        break;
      case VarType::CHAR:
        m_type.Set(BasicType::CHAR, std::move(dimensions),
                   m_decl->DimensionsSize() >= 1);
        break;
      case VarType::BOOL:
        m_type.Set(BasicType::BOOL, std::move(dimensions),
                   m_decl->DimensionsSize() >= 1);
        break;
      default:
        assert(false);  // unreachable
    }
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
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

  bool m_visited;      // first used in ComputeSideEffect
  bool m_side_effect;  // whether the function has side effect

  explicit Function(const std::shared_ptr<FuncDefAST> &func_ast)
      : m_func_ast(func_ast),
        m_func_name(func_ast->FuncName()),
        m_bb_list(),
        m_args(),
        m_is_decl(func_ast->m_is_builtin),
        m_current_bb(nullptr),
        m_visited(false),
        m_side_effect(false) {
    for (auto &param : func_ast->m_params) {
      auto arg = std::make_shared<FunctionArg>(param, m_is_decl);
      m_args.push_back(arg);
    }
    switch (func_ast->ReturnType()) {
      case VarType::INT:
        m_type.Set(BasicType::INT);
        break;
      case VarType::FLOAT:
        m_type.Set(BasicType::FLOAT);
        break;
      case VarType::VOID:
        m_type.Set(BasicType::VOID);
        break;
      default:
        assert(false);  // unreachable
    }
  }

  [[nodiscard]] std::string FuncName() const { return m_func_name; }
  [[nodiscard]] VarType ReturnType() const { return m_func_ast->ReturnType(); }

  std::shared_ptr<BasicBlock> GetCurrentBB() { return m_current_bb; }

  bool HasSideEffect() { return m_side_effect; }

  void AppendBasicBlock(std::shared_ptr<BasicBlock> bb);

<<<<<<< HEAD
  std::list<std::shared_ptr<BasicBlock>> GetBlockList();

  void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth);
=======
  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
>>>>>>> garen_dev

  friend class Module;
};

class Module {
private:
  std::shared_ptr<Function> m_current_func;

public:
  std::list<std::shared_ptr<Function>> m_function_list;
  std::list<std::shared_ptr<Function>> m_function_decl_list;
  std::list<std::shared_ptr<GlobalVariable>> m_global_variable_list;

  std::vector<std::pair<int, std::shared_ptr<Constant>>> m_const_ints;
  std::vector<std::pair<float, std::shared_ptr<Constant>>> m_const_floats;

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

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator);
};

#endif  // BDDD_IR_H
