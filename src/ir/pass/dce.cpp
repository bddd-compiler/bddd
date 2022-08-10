#include <set>
#include <vector>

#include "ir/ir-pass-manager.h"

void DeadCodeElimination(std::shared_ptr<Function> func) {
  std::set<std::shared_ptr<Instruction>> alive;
  std::vector<std::shared_ptr<Instruction>> worklist;
  // Collect the set of "root" instructions that are known live.
  for (auto &bb : func->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      if (instr->m_op == IROp::JUMP || instr->m_op == IROp::BRANCH
          || instr->HasSideEffect()) {
        alive.insert(instr);
        worklist.push_back(instr);
      }
    }
  }

  while (!worklist.empty()) {
    auto instr = worklist.back();
    worklist.pop_back();
    for (auto op : instr->Operands()) {
      if (auto op_instr
          = std::dynamic_pointer_cast<Instruction>(op->getValue())) {
        if (alive.insert(op_instr).second) {
          worklist.push_back(op_instr);
        }
      }
    }
  }

  for (auto &bb : func->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      if (alive.find(instr) == alive.end()) {
        worklist.push_back(instr);
        // instr->KillAllMyUses();
      }
    }
  }
  for (auto it = worklist.rbegin(); it != worklist.rend(); ++it) {
    // manually ++it
    auto instr = *it;
    auto bb = instr->m_bb;
    auto it2
        = std::find(bb->m_instr_list.begin(), bb->m_instr_list.end(), instr);
    bb->m_instr_list.erase(it2);
    // bb->RemoveInstruction(instr);
    std::cerr << "[debug] dce" << std::endl;
  }
}
