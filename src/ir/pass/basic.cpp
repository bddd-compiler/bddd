//
// Created by garen on 7/26/22.
//

#include <stack>

#include "ir/ir-pass-manager.h"

void RemoveUnvisitedBasicBlocks(std::shared_ptr<Function> func) {
  int cnt = 0;
  while (true) {
    ++cnt;
    bool changed = false;
    std::stack<std::shared_ptr<BasicBlock>> stack;
    for (auto &bb : func->m_bb_list) {
      bb->m_visited = false;
    }
    stack.push(func->m_bb_list.front());
    while (!stack.empty()) {
      std::shared_ptr<BasicBlock> bb = stack.top();
      stack.pop();
      if (bb->m_visited) continue;
      bb->m_visited = true;
      for (auto &s : bb->Successors()) {
        if (!s->m_visited) {
          stack.push(s);
        }
      }
    }
    for (auto it = func->m_bb_list.begin(); it != func->m_bb_list.end();) {
      // manually ++it
      auto bb = *it;
      if (!bb->m_visited) {
        changed = true;
        // remove bb
        for (auto &s : bb->Successors()) {
          s->RemovePredecessor(bb);
        }
        auto del = it;
        ++it;
        func->m_bb_list.erase(del);
        // FORGET removing ALL USES from the use_list of their VALUES???????

        for (auto &instr : bb->m_instr_list) {
          instr->KillAllUses();
          instr->KillAllMyUses();
        }
      } else {
        ++it;
      }
    }
    if (!changed) break;
  }
  if (cnt > 1) {
    std::cerr << "[debug] "
              << "remove unused block x" << cnt - 1 << std::endl;
  }
}

// empty phi instructions may appear after GVN
void RemoveTrivialPhis(std::shared_ptr<Function> func) {
  // should not be a loop
  int cnt = 0;
  while (true) {
    ++cnt;
    bool changed = false;
    for (auto &bb : func->m_bb_list) {
      std::vector<std::shared_ptr<PhiInstruction>> phis;
      for (auto &instr : bb->m_instr_list) {
        if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
          phis.push_back(phi);  // insert empty phi
        } else {
          break;
        }
      }
      // remove meaningless phis
      for (auto &phi : phis) {
        // check if all uses are the same
        std::shared_ptr<Value> val = nullptr;  // initially undef
        bool flag = true;
        for (auto [from_bb, use] : phi->m_contents) {
          if (use == nullptr) {
            continue;  // undef
          }
          if (val == nullptr) {
            if (use->getValue() != phi) val = use->getValue();
          } else {
            if (use->getValue() != phi && use->getValue() != val) {
              // not correct
              flag = false;
              break;
            }
          }
        }
        if (!flag) {
          // with at least two non-trivial different phi operands, cannot
          // be removed, look for another phi instruction
          continue;
        }
        changed = true;
        if (!phi->m_contents.empty()) {
          assert(val != nullptr);  // phi operands cannot be all undef
          // otherwise, can be replaced by its one-and-only val
          phi->ReplaceUseBy(val);
          phi->m_bb->RemoveInstruction(phi);
        } else {
          // empty phi instruction, just remove it
          phi->m_bb->RemoveInstruction(phi);
        }
      }
    }
    if (!changed) break;
  }
  if (cnt > 1) {
    std::cerr << "[debug] "
              << "remove trivial phis x" << cnt - 1 << std::endl;
  }
}

void ReplaceTrivialBranchByJump(std::shared_ptr<Function> func) {
  int cnt = 0;
  while (true) {
    ++cnt;
    bool changed = false;
    for (auto &bb : func->m_bb_list) {
      // since empty bb never appear
      auto last_instr = bb->LastInstruction();
      if (auto br_instr
          = std::dynamic_pointer_cast<BranchInstruction>(last_instr)) {
        if (br_instr->m_cond->getValue()->m_type.IsConst()) {
          auto c = std::dynamic_pointer_cast<Constant>(
              br_instr->m_cond->getValue());
          std::shared_ptr<BasicBlock> target_block = nullptr;
          if (c->Evaluate().IsNotZero()) {
            // go to true_block
            // delete bb -> false_block
            br_instr->m_false_block->RemovePredecessor(bb);
            target_block = br_instr->m_true_block;
          } else {
            // go to false_block
            // delete bb -> true_block
            br_instr->m_true_block->RemovePredecessor(bb);
            target_block = br_instr->m_false_block;
          }
          assert(target_block != nullptr);
          changed = true;
          auto jump_instr = std::make_shared<JumpInstruction>(target_block, bb);
          bb->m_instr_list.pop_back();
          bb->PushBackInstruction(jump_instr);
        }
      }
    }
    if (!changed) break;
  }
  if (cnt > 1) {
    std::cerr << "[debug] "
              << "replace trivial branch by jump x" << cnt - 1 << std::endl;
  }
}

// remove trivial bb with only jump instruction
void RemoveTrivialBasicBlock(std::shared_ptr<Function> func) {
  int cnt = 0;
  while (true) {
    ++cnt;
    bool changed = false;
    for (auto it = func->m_bb_list.begin(); it != func->m_bb_list.end();) {
      auto nxt = std::next(it);
      // manually ++it
      auto bb = *it;
      if (bb->m_instr_list.size() == 1) {
        auto instr = bb->m_instr_list.front();  // must be a terminator
        if (auto jump_instr
            = std::dynamic_pointer_cast<JumpInstruction>(instr)) {
          auto target_block = jump_instr->m_target_block;
          auto predecessors = bb->Predecessors();

          // check if it can be removed
          bool flag = true;
          for (auto &pred : predecessors) {
            for (auto &instr2 : target_block->m_instr_list) {
              if (auto phi
                  = std::dynamic_pointer_cast<PhiInstruction>(instr2)) {
                auto pred_val = phi->GetValue(pred);
                auto bb_val = phi->GetValue(bb);
                if (pred_val != nullptr && bb_val != nullptr
                    && pred_val != bb_val) {
                  flag = false;
                  break;
                }
              } else {
                break;
              }
            }
            if (!flag) {
              break;
            }
          }
          if (flag) {
            // can be removed
            changed = true;
            for (auto &pred : predecessors) {
              // try removing bb, construct pred_block -> target_block
              // 记得一定要更新这两个bb的predecessors和successors
              pred->ReplaceSuccessorBy(bb, target_block);
              target_block->ReplacePredecessorBy(bb, pred);
            }
            bb->RemoveInstruction(instr);
            continue;
          }
        }
      }
      it = nxt;
    }
    if (!changed) break;
  }
  assert(cnt == 1);
  if (cnt > 1) {
    std::cerr << "[debug] "
              << "remove trivial basic block x" << cnt - 1 << std::endl;
  }
}
