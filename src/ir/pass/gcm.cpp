#include <unordered_map>

#include "ir/ir-pass-manager.h"

// bool IsPinned(std::shared_ptr<Instruction> instr) {
//   if (auto binary_instr =
//   std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
//     return binary_instr->IsICmp();
//   }
//   if (auto call_instr = std::dynamic_pointer_cast<CallInstruction>(instr)) {
//     return call_instr->HasSideEffect();
//   }
//   if (instr->m_op == IROp::GET_ELEMENT_PTR) return true;
//   return false;
// }

void MoveInstruction(std::shared_ptr<Instruction> instr,
                     std::shared_ptr<BasicBlock> bb) {
  assert(bb != nullptr);
  if (instr->m_bb != bb) {
    // no need to remove all uses
    instr->m_bb->m_instr_list.remove(instr);
    instr->m_bb = bb;
    instr->m_bb->m_instr_list.push_back(instr);
  }
}

void ScheduleEarly(std::shared_ptr<Instruction> instr,
                   std::shared_ptr<BasicBlock> root) {
  if (instr->m_visited) return;
  instr->m_visited = true;

  if (std::dynamic_pointer_cast<BinaryInstruction>(instr)
      || std::dynamic_pointer_cast<GetElementPtrInstruction>(instr)
      || std::dynamic_pointer_cast<LoadInstruction>(instr)) {
    MoveInstruction(instr, root);
    for (auto op : instr->Operands()) {
      if (auto input = std::dynamic_pointer_cast<Instruction>(op->getValue())) {
        ScheduleEarly(input, root);
        if (instr->m_bb->m_dom_depth < input->m_bb->m_dom_depth) {
          MoveInstruction(instr, input->m_bb);
        }
      }
    }
  }

  // TODO(garen): pure function call
}

std::shared_ptr<BasicBlock> FindLCA(std::shared_ptr<BasicBlock> a,
                                    std::shared_ptr<BasicBlock> b) {
  if (a == nullptr) return b;
  // emm Click那篇论文贴LCA还能贴错
  while (b->m_dom_depth < a->m_dom_depth) {
    a = a->m_idom;
  }
  while (a->m_dom_depth < b->m_dom_depth) {
    b = b->m_idom;
  }
  while (a != b) {
    a = a->m_idom;
    b = b->m_idom;
  }
  return a;
}

void ScheduleLate(std::shared_ptr<Instruction> instr) {
  if (instr->m_visited) return;
  instr->m_visited = true;

  if (std::dynamic_pointer_cast<BinaryInstruction>(instr)
      || std::dynamic_pointer_cast<GetElementPtrInstruction>(instr)
      || std::dynamic_pointer_cast<LoadInstruction>(instr)) {
    std::shared_ptr<BasicBlock> lca = nullptr;
    for (auto &y_use : instr->m_use_list) {
      if (auto y = std::dynamic_pointer_cast<Instruction>(
              std::shared_ptr(y_use->m_user))) {
        ScheduleLate(y);
        auto use = y->m_bb;
        if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(y)) {
          auto it = std::find_if(
              phi->m_contents.begin(), phi->m_contents.end(),
              [&y_use](const auto &p) { return p.second == y_use.get(); });
          assert(it != phi->m_contents.end());
          use = it->first;
        }
        if (lca)
          lca = FindLCA(lca, use);
        else
          lca = use;
      }
    }
    // use the latest and earliest blocks to pick final position
    assert(lca != nullptr);
    auto best = lca;
    while (lca != instr->m_bb) {
      if (lca->m_loop_depth < best->m_loop_depth) {
        best = lca;
      }
      lca = lca->m_idom;
    }
    MoveInstruction(instr, best);
    bool flag = false;
    // just place it before its first use
    for (auto it = best->m_instr_list.begin(); it != best->m_instr_list.end();
         ++it) {
      auto instr2 = *it;
      if (instr2->m_op != IROp::PHI) {
        for (auto &use : instr->m_use_list) {
          if (std::shared_ptr(use->m_user) == instr2) {
            // no need to remove all uses
            best->m_instr_list.remove(instr);
            best->m_instr_list.insert(it, instr);
            flag = true;
            break;
          }
        }
      }
      if (flag) break;
    }
  }
}

void MovePhiFirst(std::shared_ptr<BasicBlock> bb) {
  std::vector<std::shared_ptr<PhiInstruction>> phis;
  bool in_first = true;
  for (auto &instr : bb->m_instr_list) {
    if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
      if (!in_first) phis.push_back(phi);
    } else {
      in_first = false;
    }
    // cannot "else break" now
  }
  for (auto &phi : phis) {
    // no need to remove all uses
    bb->m_instr_list.remove(phi);
    bb->m_instr_list.push_front(phi);
  }
}

void MoveTerminatorLast(std::shared_ptr<BasicBlock> bb) {
  std::shared_ptr<Instruction> terminator = nullptr;
  for (auto &instr : bb->m_instr_list) {
    if (instr->IsTerminator()) {
      if (terminator)
        assert(false);
      else
        terminator = instr;
    }
  }
  assert(terminator != nullptr);
  // no need to remove all uses
  bb->m_instr_list.remove(terminator);
  bb->m_instr_list.push_back(terminator);
}

void RunGCM(std::shared_ptr<Function> function,
            std::shared_ptr<IRBuilder> builder) {
  std::vector<std::shared_ptr<Instruction>> instrs;
  for (auto &bb : function->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      instrs.push_back(instr);
    }
  }

  for (auto &instr : instrs) {
    instr->m_visited = false;
  }
  for (auto &instr : instrs) {
    ScheduleEarly(instr, function->m_bb_list.front());
  }

  for (auto &instr : instrs) {
    instr->m_visited = false;
  }
  for (auto &instr : instrs) {
    ScheduleLate(instr);
  }

  for (auto &bb : function->m_bb_list) {
    MovePhiFirst(bb);
    MoveTerminatorLast(bb);
  }
}

void IRPassManager::GCMPass() {
  for (auto &func : m_builder->m_module->m_function_list) {
    ComputeLoopRelationship(func);
    // DeadCodeElimination(func);
    ComputeDominanceRelationship(func);

    assert(!func->m_bb_list.empty());
    RunGCM(func, m_builder);
  }
}