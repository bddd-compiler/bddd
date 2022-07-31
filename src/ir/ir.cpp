#include "ir/ir.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

// value and use

// warning: instances of Use should be initialized only via std::make_unique

// void Use::InitUser(std::shared_ptr<Value> user) {
//   if (m_value) {
//     m_user = std::move(user);
//     m_value->AddUse(m_user);
//   }
// }
//
// void Use::UseValue(const std::shared_ptr<Value>& value) {
//   assert(value != nullptr);
//   m_value = value;
// }

// m_use_list can contain multiple uses with the same user
// value will always be the same
// a value can use itself (e.g. phi node)
Use* Value::AddUse(const std::shared_ptr<Value>& user) {
  auto this_ptr = shared_from_this();
  // maybe phi instruction can use itself
  // assert(this_ptr != user);
  m_use_list.push_back(std::make_unique<Use>(this_ptr, user));
  return m_use_list.back().get();
}

std::unique_ptr<Use> Value::KillUse(Use* use, bool mov) {
  auto it = std::find_if(m_use_list.begin(), m_use_list.end(),
                         [&use](const auto& x) { return x.get() == use; });
  assert(it != m_use_list.end());
  // ATTENTION: no need to inform the user to remove the use
  // std::cerr << "[debug] killed" << std::endl;
  if (!mov) {
    m_use_list.erase(it);
    return nullptr;
  } else {
    auto ptr = std::move(*it);
    m_use_list.erase(it);
    return std::move(ptr);
  }
}
void Value::KillAllUses() {
  for (auto it = m_use_list.begin(); it != m_use_list.end();) {
    auto use = (*it).get();
    ++it;
    KillUse(use);
  }
}

// move from val's m_use_list to new_val's m_use_list
void Value::ReplaceUseBy(const std::shared_ptr<Value>& new_val) {
  for (auto it = m_use_list.begin(); it != m_use_list.end();) {
    auto use = (*it).get();
    ++it;
    assert(use->getValue() == shared_from_this());
    // SOMETHING NONTRIVIAL HERE: we just
    // 1. move the value's ownership out here
    auto use_ptr = KillUse(use, true);
    assert(use_ptr.get() == use);
    // 2. change its value to the new_val
    use_ptr->m_value = new_val;
    // 3. add to new_val's m_use_list
    new_val->m_use_list.push_back(std::move(use_ptr));
    // in this way, it is no need for this replacement action to inform users
  }
  assert(m_use_list.empty());
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
void Module::RemoveInstrsAfterTerminator() {
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
std::list<std::shared_ptr<Instruction>> BasicBlock::GetInstList() {
  return m_instr_list;
}
void BasicBlock::RemoveInstruction(const std::shared_ptr<Instruction>& elem) {
  auto it = std::find(m_instr_list.begin(), m_instr_list.end(), elem);
  assert(it != m_instr_list.end());
  (*it)->KillAllMyUses();
  (*it)->KillAllUses();
  m_instr_list.erase(it);
}
void BasicBlock::RemoveInstruction(
    std::list<std::shared_ptr<Instruction>>::iterator it) {
  assert(it != m_instr_list.end());
  (*it)->KillAllMyUses();
  (*it)->KillAllUses();
  m_instr_list.erase(it);
}
std::unordered_set<std::shared_ptr<BasicBlock>> BasicBlock::Predecessors() {
  return m_predecessors;
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
bool BasicBlock::Dominate(std::shared_ptr<BasicBlock> bb) {
  auto it = std::find_if(m_dominators.begin(), m_dominators.end(),
                         [=](const auto& x) { return x.get() == bb.get(); });
  return it != m_dominators.end();
}

bool BasicBlock::IsDominatedBy(std::shared_ptr<BasicBlock> bb) {
  auto it = std::find_if(m_dominated.begin(), m_dominated.end(),
                         [=](const auto& x) { return x.get() == bb.get(); });
  return it != m_dominated.end();
}
void BasicBlock::RemovePredecessor(std::shared_ptr<BasicBlock> bb) {
  for (auto& instr : m_instr_list) {
    if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
      phi->RemoveByBasicBlock(bb);
    } else {
      break;
    }
  }
  m_predecessors.erase(bb);
}
void BasicBlock::ReplacePredecessorBy(std::shared_ptr<BasicBlock> old_block,
                                      std::shared_ptr<BasicBlock> new_block) {
  // only phi instructions matter the predecessor
  for (auto instr : m_instr_list) {
    if (auto phi_instr = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
      phi_instr->ReplacePhiOperand(old_block, new_block);
    } else {
      break;
    }
  }
  auto it = m_predecessors.find(old_block);
  if (it != m_predecessors.end()) {
    m_predecessors.erase(it);
    m_predecessors.insert(new_block);
  }
}

void BasicBlock::ReplaceSuccessorBy(std::shared_ptr<BasicBlock> old_block,
                                    std::shared_ptr<BasicBlock> new_block) {
  auto last_instr = LastInstruction();
  if (auto jump_instr
      = std::dynamic_pointer_cast<JumpInstruction>(last_instr)) {
    // before: bb -> old_block
    // after: bb -> new_block
    assert(jump_instr->m_target_block == old_block);
    jump_instr->m_target_block = new_block;
    old_block->m_predecessors.erase(jump_instr->m_bb);
    new_block->m_predecessors.insert(jump_instr->m_bb);
  } else if (auto br_instr
             = std::dynamic_pointer_cast<BranchInstruction>(last_instr)) {
    assert(br_instr->m_true_block == old_block
           || br_instr->m_false_block == old_block);
    if (br_instr->m_true_block == old_block) {
      br_instr->m_true_block = new_block;
    }
    if (br_instr->m_false_block == old_block) {
      br_instr->m_false_block = new_block;
    }
    if (br_instr->m_true_block == br_instr->m_false_block) {
      // replace branch by jump
      auto new_jump_instr = std::make_shared<JumpInstruction>(
          br_instr->m_true_block, br_instr->m_bb);
      m_instr_list.pop_back();
      m_instr_list.push_back(new_jump_instr);
    }
    old_block->m_predecessors.erase(jump_instr->m_bb);
    new_block->m_predecessors.insert(jump_instr->m_bb);
  } else {
    assert(false);  // cannot be return instruction
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

std::ostream& operator<<(std::ostream& out, BasicType base_type) {
  switch (base_type) {
    case BasicType::INT:
      return out << "i32";
    case BasicType::FLOAT:
      return out << "float";
    case BasicType::BOOL:
      return out << "i1";
    case BasicType::CHAR:
      return out << "i8";
    case BasicType::LABEL:
      return out << "label";
    case BasicType::VOID:
      return out << "void";
    default:
      assert(false);  // impossible
  }
}
std::ostream& operator<<(std::ostream& out, ValueType value_type) {
  auto base_type = value_type.m_base_type;
  if (base_type == BasicType::VOID)
    return out << "void";
  else if (base_type == BasicType::LABEL)
    return out << "label";
  else if (base_type == BasicType::BOOL)
    return out << "i1";

  if (!value_type.m_dimensions.empty()) {
    for (auto dimension : value_type.m_dimensions) {
      out << '[' << dimension << " x ";
    }
    if (base_type == BasicType::INT)
      out << "i32";
    else if (base_type == BasicType::FLOAT)
      out << "float";
    else if (base_type == BasicType::CHAR)
      out << "i8";
    else
      assert(false);  // ???
    out << std::string(value_type.m_dimensions.size(), ']');
  } else {
    if (base_type == BasicType::INT)
      out << "i32";
    else if (base_type == BasicType::FLOAT)
      out << "float";
    else if (base_type == BasicType::CHAR)
      out << "i8";
    else
      assert(false);  // ???
  }
  bool first = true;
  for (int i = 0; i < value_type.m_num_star; ++i) {
    if (first)
      first = false;
    else
      out << " ";
    out << "*";
  }
  return out;
}

bool PhiInstruction::IsValid() {
  for (auto pred : m_bb->Predecessors()) {
    if (m_contents.find(pred) == m_contents.end()) {
      return false;
    }
  }
  return true;
}
