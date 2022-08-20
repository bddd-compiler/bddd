#include <set>
#include <vector>

#include "ir/ir-pass-manager.h"

void dfs(std::shared_ptr<BasicBlock> bb,
         std::vector<std::shared_ptr<BasicBlock>> &po_bb_list) {
  if (bb->m_visited) return;
  bb->m_visited = true;
  for (auto &s : bb->Successors()) {
    if (!bb->m_visited) {
      dfs(s, po_bb_list);
    }
  }
  po_bb_list.push_back(bb);
}

bool DeadCodeElimination(std::shared_ptr<Function> func) {
  std::vector<std::shared_ptr<BasicBlock>> po_bb_list;
  for (auto &bb : func->m_bb_list) {
    bb->m_visited = false;
  }
  dfs(func->m_bb_list.front(), po_bb_list);

  std::set<std::shared_ptr<Instruction>> alive;
  std::vector<std::shared_ptr<Instruction>> worklist;

  int cnt = 0;
  while (true) {
    bool changed = false;
    alive.clear();
    worklist.clear();
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
        if (op == nullptr) continue;  // phi instruction is possible
        if (auto op_instr
            = std::dynamic_pointer_cast<Instruction>(op->getValue())) {
          if (alive.insert(op_instr).second) {
            worklist.push_back(op_instr);
          }
        }
      }
    }

    for (auto &bb : po_bb_list) {
      for (auto it = bb->m_instr_list.rbegin(); it != bb->m_instr_list.rend();
           ++it) {
        auto instr = *it;
        if (alive.find(instr) == alive.end()) {
          worklist.push_back(instr);
        }
      }
    }
    changed = (!worklist.empty());
    for (auto &instr : worklist) {
      // manually ++it
      auto bb = instr->m_bb;
      auto it2
          = std::find(bb->m_instr_list.begin(), bb->m_instr_list.end(), instr);
      // bb->m_instr_list.erase(it2);
      bb->RemoveInstruction(instr);
      cnt++;
    }
    std::vector<std::shared_ptr<Instruction>> instrs;
    for (auto &bb : func->m_bb_list) {
      instrs.clear();
      for (auto &instr : bb->m_instr_list) {
        instrs.push_back(instr);
      }
      for (auto &instr : instrs) {
        if (std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
          if (instr->m_use_list.empty()) {
            bb->RemoveInstruction(instr);
            changed = true;
            cnt++;
          }
        }
      }
    }
    if (!changed) break;
  }
  if (cnt) {
    std::cerr << "[debug] dce x" << cnt << std::endl;
    return true;
  }
  return false;
}
