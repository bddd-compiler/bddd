#include "ir/ir-pass-manager.h"

void dfs(std::shared_ptr<Instruction> instr) {
  if (instr->m_visited) return;
  instr->m_visited = true;
  for (auto &operand : instr->Operands()) {
    if (auto operand_instr
        = std::dynamic_pointer_cast<Instruction>(operand->getValue())) {
      dfs(operand_instr);
    }
  }
}

void DeadCodeElimination(std::shared_ptr<Function> function) {
  for (auto &bb : function->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      bb->m_visited = false;
    }
  }

  for (auto &bb : function->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      if (instr->HasSideEffect()) {
        dfs(instr);
      }
    }
  }

  for (auto &bb : function->m_bb_list) {
    for (auto it = bb->m_instr_list.begin(); it != bb->m_instr_list.end();) {
      auto instr = *it;
      if (!instr->m_visited) {
        auto del = it;
        ++it;
        for (auto op : instr->Operands()) {
          op->getValue()->KillUse(op);
        }
        bb->RemoveInstruction(del);
        std::cerr << "dce" << std::endl;
      } else {
        ++it;
      }
    }
  }
  for (auto &bb : function->m_bb_list) {
    for (auto it = bb->m_instr_list.begin(); it != bb->m_instr_list.end();) {
      // manually ++it
      auto instr = *it;
      if (!instr->m_visited) {
        auto del = it;
        ++it;
        bb->RemoveInstruction(del);
        std::cerr << "dce" << std::endl;
      } else {
        ++it;
      }
    }
  }
}