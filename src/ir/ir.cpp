#include "ir/ir.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

// value and use

// warning: instances of Use should be initialized only via std::make_unique

void Use::InitUser(std::shared_ptr<Value> user) {
  if (m_value) {
    m_user = std::move(user);
    m_value->AddUse(m_user);
  }
}

void Use::UseValue(std::shared_ptr<Value> value) {
  assert(value != nullptr);

  // 别别别，我杀我自己是吧
  // if (m_value) {
  //   m_value->KillUse(m_user);
  //   m_value = nullptr;
  // }
  // m_value = value;
  // m_value->AddUse(m_user);

  m_value = value;
}

void Use::RemoveFromUseList() {
  auto it = std::find_if(m_value->m_use_list.begin(), m_value->m_use_list.end(),
                         [=](const auto& ptr) { return *ptr == *this; });
  assert(it != m_value->m_use_list.end());
  m_value->m_use_list.erase(it);
}

void Value::AddUse(const std::shared_ptr<Value>& user) {
  auto this_ptr = shared_from_this();
  assert(this_ptr != user);
  m_use_list.push_back(std::make_shared<Use>(this_ptr, user));
}

void Value::KillUse(const std::shared_ptr<Value>& user) {
  for (auto it = m_use_list.begin(); it != m_use_list.end(); ++it) {
    if ((*it)->m_user == user) {
      m_use_list.erase(it);
      return;
    }
  }
  assert(false);  // not found! what happen?
}

void Value::ReplaceUseBy(const std::shared_ptr<Value>& new_val) {
  for (auto& use : m_use_list) {
    auto this_ptr = shared_from_this();
    assert(use->m_value == this_ptr);
    // if (use->m_user != this_ptr) use->UseValue(new_val);
    use->UseValue(new_val);
  }
}

// module, function, basic block

void Module::AppendFunctionDecl(std::shared_ptr<Function> function_decl) {
  m_function_decl_list.push_back(function_decl);
}
void Module::AppendFunction(std::shared_ptr<Function> function) {
  m_function_list.push_back(function);
  m_current_func = m_function_list.back();
}
void Module::AppendGlobalVariable(
    std::shared_ptr<GlobalVariable> global_variable) {
  m_global_variable_list.push_back(global_variable);
}
void Module::AppendBasicBlock(std::shared_ptr<BasicBlock> bb) {
  m_current_func->AppendBasicBlock(std::move(bb));
}
void Module::Check() {
  for (auto& function : m_function_list) {
    for (auto& basic_block : function->m_bb_list) {
      assert(!basic_block->m_instr_list.empty());

      for (auto it = basic_block->m_instr_list.begin();
           it != basic_block->m_instr_list.end(); ++it) {
        if ((*it)->IsTerminator()) {
          // all instructions after instr should be discarded
          basic_block->m_instr_list.erase(std::next(it),
                                          basic_block->m_instr_list.end());
        }
      }
      assert(basic_block->LastInstruction()->IsTerminator());
    }
  }
}
void Function::AppendBasicBlock(std::shared_ptr<BasicBlock> bb) {
  m_bb_list.push_back(std::move(bb));
  m_current_bb = m_bb_list.back();
}
void BasicBlock::PushBackInstruction(std::shared_ptr<Instruction> instr) {
  instr->m_bb = shared_from_base<BasicBlock>();
  m_instr_list.push_back(std::move(instr));
}
void BasicBlock::PushFrontInstruction(std::shared_ptr<Instruction> instr) {
  instr->m_bb = shared_from_base<BasicBlock>();
  m_instr_list.push_front(std::move(instr));
}
void BasicBlock::InsertFrontInstruction(
    const std::shared_ptr<Instruction>& elem,
    std::shared_ptr<Instruction> instr) {
  const auto it = std::find(m_instr_list.begin(), m_instr_list.end(), elem);
  assert(it != m_instr_list.end());
  instr->m_bb = shared_from_base<BasicBlock>();
  m_instr_list.insert(it, std::move(instr));
}
void BasicBlock::InsertBackInstruction(const std::shared_ptr<Instruction>& elem,
                                       std::shared_ptr<Instruction> instr) {
  auto it = std::find(m_instr_list.begin(), m_instr_list.end(), elem);
  assert(it != m_instr_list.end());
  ++it;
  assert(it != m_instr_list.end());
  instr->m_bb = shared_from_base<BasicBlock>();
  m_instr_list.insert(it, std::move(instr));
}
void BasicBlock::RemoveInstruction(const std::shared_ptr<Instruction>& elem) {
  auto it = std::find_if(m_instr_list.begin(), m_instr_list.end(),
                         [=](auto x) { return x.get() == elem.get(); });
  if (it != m_instr_list.end()) {
    m_instr_list.erase(it);
  }
}
std::vector<std::shared_ptr<BasicBlock>> BasicBlock::Successors() {
  if (m_instr_list.empty()) return {};
  auto last_instr = LastInstruction();
  switch (last_instr->m_op) {
    case IROp::BRANCH: {
      auto br_instr = last_instr->shared_from_base<BranchInstruction>();
      return {br_instr->m_true_block, br_instr->m_false_block};
    }
    case IROp::JUMP: {
      auto jump_instr = last_instr->shared_from_base<JumpInstruction>();
      return {jump_instr->m_target_block};
    }
    default:
      return {};
  }
}

bool Instruction::HasSideEffect() {
  switch (m_op) {
    case IROp::CALL:
      return shared_from_base<CallInstruction>()->m_function->HasSideEffect();
    case IROp::BRANCH:
    case IROp::JUMP:
    case IROp::RETURN:
    case IROp::STORE:
      return true;
    default:
      return false;
  }
}