#ifndef BDDD_IR_H
#define BDDD_IR_H

#include <algorithm>
#include <iostream>
#include <list>
#include <memory>
#include <set>
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

  XOR,  // currently just used in i1 (boolean)

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

  [[nodiscard]] bool Equals(BasicType base_type, bool is_ptr = false) const {
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

// a Use instance is only owned by its value, storing in its m_use_list
// must be created using make_unique
class Use {
public:
  std::weak_ptr<Value> m_value;  // the value
  std::weak_ptr<Value> m_user;   // who use the value

  explicit Use() : m_value(), m_user() {}
  explicit Use(const std::shared_ptr<Value> &value,
               const std::shared_ptr<Value> &user)
      : m_value(value), m_user(user) {}

  Use(const Use &) = delete;

  // modify the value
  void UseValue(const std::shared_ptr<Value> &value);

  [[nodiscard]] std::shared_ptr<Value> getValue() const {
    return m_value.lock();
  }

  [[nodiscard]] std::shared_ptr<Value> getUser() const { return m_user.lock(); }

  // void RemoveFromUseList();
};

class BasicBlock;

class Value : public std::enable_shared_from_this<Value> {
public:
  // IMPORTANT NOTES ABOUT m_use_list:
  // 1. m_use_list record who use this value
  // (i.e. use->m_value must be this value)
  // 2. but a value may be replaced by another one
  // (in which situation we will move the Use instance to the another value)
  // 3. m_use_list should own all uses
  // (so that every modification of a Use instance can make difference for all
  // of its related instructions)
  std::list<std::unique_ptr<Use>> m_use_list;
  ValueType m_type;

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

  Use *AddUse(const std::shared_ptr<Value> &user);

  // options:
  //
  std::unique_ptr<Use> KillUse(const std::shared_ptr<Value> &user,
                               bool mov = false);

  // kill all uses that use me
  void KillAllUses();

  void ReplaceUseBy(const std::shared_ptr<Value> &new_val);

  // virtual void AllocateName(std::shared_ptr<IRNameAllocator> allocator) = 0;
  virtual void ExportIR(std::ofstream &ofs, int depth) = 0;

  // ~Value() { std::cerr << "[debug] destructor of Value" << std::endl; }
};

class Constant : public Value {
public:
  int m_int_val;
  float m_float_val;
  // constants may belong to any bb, which is useful in mem2reg

  // non-float
  explicit Constant(int int_val, BasicType basic_type)
      : Value(basic_type, true), m_int_val(int_val), m_float_val(0.0) {
    assert(basic_type == BasicType::INT || basic_type == BasicType::BOOL
           || basic_type == BasicType::CHAR);
    assert(m_bb == nullptr);
  }

  explicit Constant(float float_val)
      : Value(BasicType::FLOAT, true), m_int_val(0), m_float_val(float_val) {
    assert(m_bb == nullptr);
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;

  EvalValue Evaluate() {
    switch (m_type.m_base_type) {
      case BasicType::INT:
      case BasicType::BOOL:
      case BasicType::CHAR:
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
  virtual std::vector<Use *> Operands() = 0;

  // remove all my uses (kill all uses of other's m_use_list that user is me)
  virtual void KillAllMyUses() = 0;
};

// SPECIAL
class BinaryInstruction : public Instruction {
public:
  Use *m_lhs_val_use;
  Use *m_rhs_val_use;

  explicit BinaryInstruction(IROp op, std::shared_ptr<BasicBlock> bb)
      : Instruction(op, std::move(bb)),
        m_lhs_val_use(nullptr),
        m_rhs_val_use(nullptr) {
    // assert(m_lhs_val_use.m_value->m_type == m_rhs_val_use.m_value->m_type);
    // std::shared_ptr<BinaryInstruction> temp(this);  // to allow
    // shared_from_this

    // m_lhs_val_use = lhs_val->AddUse(temp);
    // m_rhs_val_use = rhs_val->AddUse(temp);
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

  bool IsICmp() {
    switch (m_op) {
      case IROp::I_SGE:
      case IROp::I_SGT:
      case IROp::I_SLE:
      case IROp::I_SLT:
      case IROp::I_EQ:
      case IROp::I_NE:
        return true;
      default:
        return false;
    }
  }

  bool IsFCmp() {
    switch (m_op) {
      case IROp::F_EQ:
      case IROp::F_NE:
      case IROp::F_GT:
      case IROp::F_GE:
      case IROp::F_LT:
      case IROp::F_LE:
        return true;
      default:
        return false;
    }
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  bool IsTerminator() override { return false; }

  std::vector<Use *> Operands() override {
    return {m_lhs_val_use, m_rhs_val_use};
  }

  void KillAllMyUses() override {
    m_lhs_val_use->m_value.lock()->KillUse(shared_from_this());
    m_lhs_val_use = nullptr;
    m_rhs_val_use->m_value.lock()->KillUse(shared_from_this());
    m_rhs_val_use = nullptr;
  }
};

class FNegInstruction : public Instruction {
public:
  Use *m_lhs_val_use;

  explicit FNegInstruction(std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::F_NEG, std::move(bb)), m_lhs_val_use(nullptr) {
    m_type.Set(BasicType::FLOAT);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  bool IsTerminator() override { return false; }

  std::vector<Use *> Operands() override { return {m_lhs_val_use}; }

  void KillAllMyUses() override {
    m_lhs_val_use->m_value.lock()->KillUse(shared_from_this());
    m_lhs_val_use = nullptr;
  }
};

class Function;

class CallInstruction : public Instruction {
public:
  std::string m_func_name;
  std::vector<Use *> m_params;

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
  std::vector<Use *> Operands() override { return m_params; }
  void KillAllMyUses() override {
    for (auto param : m_params) {
      param->m_value.lock()->KillUse(shared_from_this());
    }
    m_params.clear();
  }

  // should be called if params are needed
  void SetParams(std::vector<Use *> params) { m_params = std::move(params); }
};

class BranchInstruction : public Instruction {
public:
  Use *m_cond;
  std::shared_ptr<BasicBlock> m_true_block, m_false_block;

  explicit BranchInstruction(std::shared_ptr<BasicBlock> true_block,
                             std::shared_ptr<BasicBlock> false_block,
                             std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::BRANCH, std::move(bb)),
        m_cond(nullptr),
        m_true_block(std::move(true_block)),
        m_false_block(std::move(false_block)) {}

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return true; }
  std::vector<Use *> Operands() override { return {m_cond}; }
  void KillAllMyUses() override {
    m_cond->m_value.lock()->KillUse(shared_from_this());
    m_cond = nullptr;
  }
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
  std::vector<Use *> Operands() override { return {}; }
  void KillAllMyUses() override {}
};

class ReturnInstruction : public Instruction {
public:
  Use *m_ret;  // WARNING: nullable

  explicit ReturnInstruction(std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::RETURN, std::move(bb)), m_ret(nullptr) {}

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return true; }
  std::vector<Use *> Operands() override {
    if (m_ret)
      return {m_ret};
    else
      return {};
  }
  void KillAllMyUses() override {
    if (m_ret) {
      m_ret->m_value.lock()->KillUse(shared_from_this());
      m_ret = nullptr;
    }
  }

  bool IsConstant() const {
    return std::dynamic_pointer_cast<Constant>(m_ret->m_value.lock())
           != nullptr;
  }
};

class GetElementPtrInstruction : public Instruction {
public:
  Use *m_addr;
  std::vector<Use *> m_indices;

  explicit GetElementPtrInstruction(std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::GET_ELEMENT_PTR, std::move(bb)),
        m_addr(nullptr),
        m_indices() {}

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
  std::vector<Use *> Operands() override {
    std::vector<Use *> ret;
    ret.push_back(m_addr);
    ret.insert(ret.end(), m_indices.begin(), m_indices.end());
    return std::move(ret);
  }
  void KillAllMyUses() override {
    m_addr->m_value.lock()->KillUse(shared_from_this());
    m_addr = nullptr;
    for (auto index : m_indices) {
      index->m_value.lock()->KillUse(shared_from_this());
    }
    m_indices.clear();
  }
};

// must set m_addr after constructor is called
class LoadInstruction : public Instruction {
public:
  Use *m_addr;

  explicit LoadInstruction(std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::LOAD, std::move(bb)), m_addr(nullptr) {}

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
  std::vector<Use *> Operands() override { return {m_addr}; }
  void KillAllMyUses() override {
    m_addr->m_value.lock()->KillUse(shared_from_this());
    m_addr = nullptr;
  }
};

// must set m_addr and m_val after constructor is called
class StoreInstruction : public Instruction {
public:
  Use *m_addr;
  Use *m_val;

  explicit StoreInstruction(std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::STORE, std::move(bb)),
        m_addr(nullptr),
        m_val(nullptr) {}

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }

  std::vector<Use *> Operands() override { return {m_addr, m_val}; }
  void KillAllMyUses() override {
    m_addr->m_value.lock()->KillUse(shared_from_this());
    m_addr = nullptr;
    m_val->m_value.lock()->KillUse(shared_from_this());
    m_val = nullptr;
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
  std::vector<Use *> Operands() override { return {}; }
  void KillAllMyUses() override {}
};

class PhiInstruction : public Instruction {
public:
  std::unordered_map<std::shared_ptr<BasicBlock>, Use *> m_contents;

  explicit PhiInstruction(ValueType type, std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::PHI, std::move(bb)) {
    m_type = std::move(type);
  }

  // return nullptr if nothing found
  std::shared_ptr<Value> GetValue(std::shared_ptr<BasicBlock> bb) {
    auto it = std::find_if(
        m_contents.begin(), m_contents.end(),
        [=](const auto &x) { return x.first.get() == bb.get(); });
    if (it == m_contents.end())
      return nullptr;
    else
      return it->second->m_value.lock();
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

  void ReplacePhiOperand(std::shared_ptr<BasicBlock> old_block,
                         std::shared_ptr<BasicBlock> new_block) {
    auto it = m_contents.find(old_block);
    if (it != m_contents.end()) {
      AddPhiOperand(new_block, it->second->m_value.lock());
      m_contents.erase(it);
    }
  }

  bool IsValid();

  // void RemoveByValue(std::shared_ptr<Value> val) {
  //   for (auto it = m_contents.begin(); it != m_contents.end(); ++it) {
  //     if (it->second->m_value == val) {
  //       m_contents.erase(it);
  //       return;
  //     }
  //   }
  // }
  void RemoveByBasicBlock(std::shared_ptr<BasicBlock> bb) {
    for (auto it = m_contents.begin(); it != m_contents.end(); ++it) {
      if (it->first == bb) {
        m_contents.erase(it);
        return;
      }
    }
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
  std::vector<Use *> Operands() override {
    std::vector<Use *> ret;
    for (auto &content : m_contents) {
      ret.push_back(content.second);
    }
    return std::move(ret);
  }
  void KillAllMyUses() override {
    for (auto &content : m_contents) {
      content.second->m_value.lock()->KillUse(shared_from_this());
    }
    m_contents.clear();
  }
};

// fat pointer to thin pointer
// i32* to i8*, float* to i8*
class BitCastInstruction : public Instruction {
public:
  Use *m_val;  // observing pointer (naive way until std::observer_ptr)
  BasicType m_target_type;  // with *
  explicit BitCastInstruction(BasicType target_type,
                              std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::BITCAST, std::move(bb)),
        m_val(nullptr),
        m_target_type(target_type) {
    m_type.Set(target_type, true);
  }

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;
  bool IsTerminator() override { return false; }
  std::vector<Use *> Operands() override { return {m_val}; }
  void KillAllMyUses() override {
    m_val->m_value.lock()->KillUse(shared_from_this());
    m_val = nullptr;
  }
};

// i1 to i32
class ZExtInstruction : public Instruction {
public:
  Use *m_val;

  explicit ZExtInstruction(std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::ZEXT, std::move(bb)), m_val(nullptr) {
    m_type.Set(BasicType::INT);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  bool IsTerminator() override { return false; }
  std::vector<Use *> Operands() override { return {m_val}; }
  void KillAllMyUses() override {
    m_val->m_value.lock()->KillUse(shared_from_this());
    m_val = nullptr;
  }
};

// i32 to float
class SIToFPInstruction : public Instruction {
public:
  Use *m_val;
  explicit SIToFPInstruction(std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::SITOFP, std::move(bb)), m_val(nullptr) {
    m_type.Set(BasicType::FLOAT);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  bool IsTerminator() override { return false; }
  std::vector<Use *> Operands() override { return {m_val}; }
  void KillAllMyUses() override {
    m_val->m_value.lock()->KillUse(shared_from_this());
    m_val = nullptr;
  }
};

// float to i32
class FPToSIInstruction : public Instruction {
public:
  Use *m_val;
  explicit FPToSIInstruction(std::shared_ptr<BasicBlock> bb)
      : Instruction(IROp::FPTOSI, std::move(bb)), m_val(nullptr) {
    m_type.Set(BasicType::INT);
  }

  void ExportIR(std::ofstream &ofs, int depth) override;
  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  bool IsTerminator() override { return false; }
  std::vector<Use *> Operands() override { return {m_val}; }
  void KillAllMyUses() override {
    m_val->m_value.lock()->KillUse(shared_from_this());
    m_val = nullptr;
  }
};

class Loop {
public:
  std::shared_ptr<BasicBlock> m_preheader;
  std::shared_ptr<BasicBlock> m_header;
  std::set<std::shared_ptr<BasicBlock>> m_bbs;
  std::set<std::shared_ptr<Loop>> m_sub_loops;
  std::shared_ptr<Loop> m_fa_loop;
  int m_loop_depth;

  explicit Loop(std::shared_ptr<BasicBlock> header)
      : m_header(std::move(header)), m_loop_depth(-1) {}
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
  int m_dom_depth;   // depth in dominance tree
  int m_dfs_depth;   // computed in rpo dfs
  int m_loop_depth;  // depth in loop (see ComputeLoopInfo)

  bool m_visited;  // first used in mem2reg
  std::unordered_set<std::shared_ptr<BasicBlock>> m_predecessors;

  std::set<std::shared_ptr<Loop>> m_loops;

  // methods

  explicit BasicBlock(std::string name)
      : Value(BasicType::LABEL),
        m_name(std::move(name)),
        m_id(0),
        m_dom_depth(-1),
        m_dfs_depth(-1),
        m_loop_depth(-1),
        m_visited(false) {}

  void PushBackInstruction(std::shared_ptr<Instruction> instr);
  void PushFrontInstruction(std::shared_ptr<Instruction> instr);

  void InsertFrontInstruction(const std::shared_ptr<Instruction> &elem,
                              std::shared_ptr<Instruction> instr);
  void InsertBackInstruction(const std::shared_ptr<Instruction> &elem,
                             std::shared_ptr<Instruction> instr);
  void RemoveInstruction(const std::shared_ptr<Instruction> &elem);
  void RemoveInstruction(std::list<std::shared_ptr<Instruction>>::iterator it);

  std::shared_ptr<Instruction> LastInstruction() {
    assert(!m_instr_list.empty());
    return m_instr_list.back();
  }

  std::unordered_set<std::shared_ptr<BasicBlock>> Predecessors();
  std::unordered_set<std::shared_ptr<BasicBlock>> Successors();

  void RemovePredecessor(std::shared_ptr<BasicBlock> bb);

  void ReplacePredecessorBy(std::shared_ptr<BasicBlock> old_block,
                            std::shared_ptr<BasicBlock> new_block);
  void ReplaceSuccessorBy(std::shared_ptr<BasicBlock> old_block,
                          std::shared_ptr<BasicBlock> new_block);

  bool Dominate(std::shared_ptr<BasicBlock> bb);
  bool IsDominatedBy(std::shared_ptr<BasicBlock> bb);

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;

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

  std::list<std::shared_ptr<BasicBlock>> m_rpo_bb_list;

  std::set<std::shared_ptr<Loop>> m_loops;

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

  bool HasSideEffect() const { return m_side_effect; }

  void AppendBasicBlock(std::shared_ptr<BasicBlock> bb);

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator) override;
  void ExportIR(std::ofstream &ofs, int depth) override;

  friend class Module;
};

class Module {
private:
  std::shared_ptr<Function> m_current_func;

public:
  std::list<std::shared_ptr<Function>> m_function_list;
  std::list<std::shared_ptr<Function>> m_function_decl_list;
  std::list<std::shared_ptr<GlobalVariable>> m_global_variable_list;

  std::unordered_map<int, std::shared_ptr<Constant>> m_const_ints;
  std::unordered_map<float, std::shared_ptr<Constant>> m_const_floats;
  std::unordered_map<bool, std::shared_ptr<Constant>> m_const_bools;
  std::unordered_map<char, std::shared_ptr<Constant>> m_const_chars;

  std::shared_ptr<BasicBlock> GetCurrentBB() {
    return m_current_func->GetCurrentBB();
  }
  std::shared_ptr<Function> GetCurrentFunc() { return m_current_func; }

  void AppendFunctionDecl(std::shared_ptr<Function> function_decl);
  void AppendFunction(std::shared_ptr<Function> function);
  void AppendGlobalVariable(std::shared_ptr<GlobalVariable> global_variable);
  void AppendBasicBlock(std::shared_ptr<BasicBlock> bb);

  void RemoveInstrsAfterTerminator();
  void ExportIR(std::ofstream &ofs, int depth);

  // void AllocateName(std::shared_ptr<IRNameAllocator> allocator);
};

#endif  // BDDD_IR_H
