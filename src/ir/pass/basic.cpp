//
// Created by garen on 7/26/22.
//

#include <stack>

#include "ir/ir-pass-manager.h"

void UpdatePredecessors(std::shared_ptr<Function> func) {
  for (auto &bb : func->m_bb_list) {
    bb->m_predecessors.clear();
  }
  for (auto &bb : func->m_bb_list) {
    for (const std::shared_ptr<BasicBlock> &v_block : bb->Successors()) {
      v_block->m_predecessors.insert(bb);
    }
  }
}

// predecessors info is unavailable
bool RemoveUnusedBasicBlocks(std::shared_ptr<Function> func) {
  int cnt = 0;
  std::vector<std::shared_ptr<BasicBlock>> unused_bbs;
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
        it = func->m_bb_list.erase(it);
        // FORGET removing ALL USES from the use_list of their VALUES???????
        unused_bbs.push_back(bb);
        // for (auto it = bb->m_instr_list.rbegin(); it !=
        // bb->m_instr_list.rend();
        //      ++it) {
        //   // if forward, the second instruction may have a dangling pointer
        //   (*it)->KillAllMyUses();
        //   (*it)->KillAllUses();
        // }
      } else {
        ++it;
      }
    }
    if (!changed) break;
  }
  if (cnt > 1) {
    if (!unused_bbs.empty()) {
      std::sort(unused_bbs.begin(), unused_bbs.end(),
                [](const auto &a, const auto &b) {
                  return a->m_dom_depth > b->m_dom_depth;
                });
    }
    for (auto &unused_bb : unused_bbs) {
      std::reverse(unused_bb->m_instr_list.begin(),
                   unused_bb->m_instr_list.end());
      for (auto it = unused_bb->m_instr_list.begin();
           it != unused_bb->m_instr_list.end();) {
        auto instr = *it;
        ++it;
        unused_bb->RemoveInstruction(instr);
      }
      assert(unused_bb->m_instr_list.empty());
    }
    std::cerr << "[debug] "
              << "removed " << unused_bbs.size() << " unused blocks"
              << std::endl;
    unused_bbs.clear();
    return true;
  }
  return false;
}

// empty phi instructions may appear after GVN
// phi instructions with all undef is not "empty", but should be empty
bool RemoveTrivialPhis(std::shared_ptr<Function> func) {
  // should not be a loop
  int cnt = 0;
  while (true) {
    ++cnt;
    bool changed = false;
    for (auto &bb : func->m_bb_list) {
      std::vector<std::shared_ptr<PhiInstruction>> phis;
      for (auto &instr : bb->m_instr_list) {
        if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
          phis.push_back(phi);  // insert phis
        } else {
          break;
        }
      }
      // remove meaningless phis
      for (auto &phi : phis) {
        // check if all uses are the same
        std::shared_ptr<Value> val = nullptr;
        bool flag = true;
        for (auto [from_bb, use] : phi->m_contents) {
          if (use == nullptr) {
            flag = false;  // temp solution: if undef appears, just keep it
            break;
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
        if (val == nullptr) {
          // all undef
          assert(phi->m_use_list.empty());
          for (auto [incoming_bb, use] : phi->m_contents) {
            assert(use == nullptr);
          }
          phi->m_contents.clear();
        }
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
    return true;
  }
  return false;
}

bool ReplaceTrivialBranchByJump(std::shared_ptr<Function> func) {
  int cnt = 0;
  while (true) {
    ++cnt;
    bool changed = false;
    for (auto &bb : func->m_bb_list) {
      // since empty bb never appear
      auto last_instr = bb->LastInstruction();
      if (auto br_instr
          = std::dynamic_pointer_cast<BranchInstruction>(last_instr)) {
        auto cond = br_instr->m_cond->getValue();
        bool deterministic = false;
        bool flag;  // undetermined yet
        if (cond->m_type.IsConst()) {
          deterministic = true;
          auto c = std::dynamic_pointer_cast<Constant>(cond);
          flag = c->Evaluate().IsNotZero();
        } else if (auto binary
                   = std::dynamic_pointer_cast<BinaryInstruction>(cond)) {
          if (binary->m_lhs_val_use->getValue()
              == binary->m_rhs_val_use->getValue()) {
            switch (binary->m_op) {
              case IROp::I_SGE:
              case IROp::I_SLE:
              case IROp::I_EQ:
              case IROp::F_EQ:
              case IROp::F_GE:
              case IROp::F_LE:
                deterministic = true;
                flag = true;
                break;
              case IROp::I_SGT:
              case IROp::I_SLT:
              case IROp::I_NE:
              case IROp::F_NE:
              case IROp::F_GT:
              case IROp::F_LT:
              case IROp::XOR:
                deterministic = true;
                flag = false;
                break;
              default:
                break;
            }
          }
        }
        if (deterministic) {
          std::shared_ptr<BasicBlock> target_block = nullptr;
          if (flag) {
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
          bb->RemoveInstruction(br_instr);
          // bb->m_instr_list.pop_back();
          bb->PushBackInstruction(jump_instr);
        }
      }
    }
    if (!changed) break;
  }
  if (cnt > 1) {
    std::cerr << "[debug] "
              << "replace trivial branch by jump x" << cnt - 1 << std::endl;
    return true;
  }
  return false;
}

// remove trivial bb with only jump instruction
bool RemoveTrivialBasicBlocks(std::shared_ptr<Function> func) {
  int cnt = 0;
  while (true) {
    ++cnt;
    bool changed = false;
    for (auto it = func->m_bb_list.begin(); it != func->m_bb_list.end();) {
      auto nxt = std::next(it);
      // manually ++it
      std::shared_ptr<BasicBlock> bb = *it;
      // BUG: mysterious std::list::size() unmatch???????
      if (std::next(bb->m_instr_list.begin()) == bb->m_instr_list.end()) {
        auto terminator = bb->LastInstruction();  // must be a terminator
        if (auto jump_instr
            = std::dynamic_pointer_cast<JumpInstruction>(terminator)) {
          auto target_block = jump_instr->m_target_block;
          auto predecessors = bb->Predecessors();

          // check if it can be removed
          bool flag = true;
          if (predecessors.empty()) {
            auto successors = bb->Successors();
            if (successors.size() != 1 || successors.front() != bb) {
              flag = false;
            }
          }
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
            if (!flag) break;
          }
          if (flag) {
            // can be removed
            changed = true;
            if (predecessors.empty()) {
              // should not be any unused blocks
              assert(it == func->m_bb_list.begin());
              for (auto &s : bb->Successors()) {
                s->RemovePredecessor(bb);
              }
            } else {
              for (auto &pred : predecessors) {
                // try removing bb, construct pred_block -> target_block
                // ??????????????????????????????bb???predecessors???successors
                pred->ReplaceSuccessorBy(bb, target_block);
              }
              target_block->ReplacePredecessorsBy(bb, predecessors);
            }
            bb->RemoveInstruction(terminator);
            nxt = func->m_bb_list.erase(it);
          }
        }
      }
      it = nxt;
    }
    if (!changed) break;
  }
  if (cnt > 1) {
    std::cerr << "[debug] "
              << "remove trivial basic block x" << cnt - 1 << std::endl;
    return true;
  }
  return false;
}

void RemoveUnusedFunctions(std::unique_ptr<Module> &module) {
  for (auto &func : module->m_function_list) {
    func->m_visited = false;
  }
  std::stack<std::shared_ptr<Function>> stack;
  auto it = std::find_if(module->m_function_list.begin(),
                         module->m_function_list.end(),
                         [](const auto &x) { return x->FuncName() == "main"; });
  assert(it != module->m_function_list.end());
  stack.push(*it);
  while (!stack.empty()) {
    std::shared_ptr<Function> f = stack.top();
    stack.pop();
    if (f->m_visited) continue;
    f->m_visited = true;
    for (auto &bb : f->m_bb_list) {
      for (auto &instr : bb->m_instr_list) {
        if (auto call = std::dynamic_pointer_cast<CallInstruction>(instr)) {
          auto called_func = call->m_function;
          if (!called_func->IsBuiltIn() && !called_func->m_visited) {
            stack.push(called_func);
          }
        }
      }
    }
  }
  for (auto it = module->m_function_list.begin();
       it != module->m_function_list.end();) {
    // manually ++it
    auto func = *it;
    if (!func->m_visited) {
      std::cerr << "[debug] removing function " << func->FuncName() << "..."
                << std::endl;
      it = module->m_function_list.erase(it);
    } else {
      ++it;
    }
  }
}

void BasicOptimization(std::shared_ptr<Function> func) {
  while (true) {
    bool changed = false;
    changed |= ReplaceTrivialBranchByJump(func);
    changed |= DeadCodeElimination(func);
    changed |= RemoveUnusedBasicBlocks(func);
    changed |= RemoveTrivialPhis(func);
    changed |= RemoveTrivialBasicBlocks(func);
    if (!changed) break;
  }
}