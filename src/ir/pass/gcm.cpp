#include <queue>
#include <stack>
#include <unordered_map>

#include "ir/ir-pass-manager.h"

// cannot move outside the original basic block
bool IsPinned(std::shared_ptr<Instruction> instr) {
  if (auto binary_instr = std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
    return binary_instr->IsICmp() || binary_instr->IsFCmp();
  }
  if (auto call_instr = std::dynamic_pointer_cast<CallInstruction>(instr)) {
    return call_instr->HasSideEffect();
  }
  if (instr->m_op == IROp::GET_ELEMENT_PTR) return false;
  return true;
}

void MoveInstructionBack(std::shared_ptr<Instruction> instr,
                         std::shared_ptr<BasicBlock> bb, bool check = true) {
  assert(bb != nullptr);
  if (check) {
    if (IsPinned(instr)) return;  // ignore it
  }
  // no need to remove all uses
  instr->m_bb->m_instr_list.remove(instr);
  instr->m_bb = bb;
  instr->m_bb->m_instr_list.push_back(instr);
  assert(instr->m_bb->m_instr_list.back() == instr);
}

void MoveInstructionFront(std::shared_ptr<Instruction> instr,
                          std::shared_ptr<BasicBlock> bb, bool check = true) {
  assert(bb != nullptr);
  if (check) {
    if (IsPinned(instr)) return;  // ignore it
  }
  // no need to remove all uses
  instr->m_bb->m_instr_list.remove(instr);
  instr->m_bb = bb;
  instr->m_bb->m_instr_list.push_front(instr);
  assert(bb->m_instr_list.front() == instr);
}

void MoveInstructionBefore(std::shared_ptr<Instruction> instr,
                           std::shared_ptr<Instruction> standard) {
  assert(instr->m_bb == standard->m_bb);
  auto bb = instr->m_bb;
  bb->m_instr_list.remove(instr);
  auto it
      = std::find(bb->m_instr_list.begin(), bb->m_instr_list.end(), standard);
  assert(it != bb->m_instr_list.end());
  bb->m_instr_list.insert(it, instr);
}

void MoveInstructionBehind(std::shared_ptr<Instruction> instr,
                           std::shared_ptr<Instruction> standard) {
  assert(instr->m_bb == standard->m_bb);
  auto bb = instr->m_bb;
  bb->m_instr_list.remove(instr);
  auto it
      = std::find(bb->m_instr_list.begin(), bb->m_instr_list.end(), standard);
  assert(it != bb->m_instr_list.end());
  ++it;
  bb->m_instr_list.insert(it, instr);
}

void ScheduleEarly(std::shared_ptr<Instruction> instr,
                   std::shared_ptr<BasicBlock> root) {
  if (instr->m_visited) return;
  instr->m_visited = true;

  auto original_bb = instr->m_bb;
  if (instr->m_op == IROp::GET_ELEMENT_PTR) {
    MoveInstructionBack(instr, root);
    for (auto op : instr->Operands()) {
      if (auto input = std::dynamic_pointer_cast<Instruction>(op->getValue())) {
        ScheduleEarly(input, root);
        if (instr->m_bb->m_dom_depth < input->m_bb->m_dom_depth) {
          MoveInstructionBack(instr, input->m_bb);
        }
      }
    }
  }
  if (auto call_instr = std::dynamic_pointer_cast<CallInstruction>(instr)) {
    MoveInstructionBack(instr, root);
    for (auto op : instr->Operands()) {
      if (auto input = std::dynamic_pointer_cast<Instruction>(op->getValue())) {
        ScheduleEarly(input, root);
        if (instr->m_bb->m_dom_depth < input->m_bb->m_dom_depth) {
          MoveInstructionBack(instr, input->m_bb);
        }
      }
    }
    if (call_instr->HasSideEffect()) {
      MoveInstructionBack(instr, original_bb);
    }
  }
  if (auto binary_instr = std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
    MoveInstructionBack(instr, root);
    for (auto op : instr->Operands()) {
      if (auto input = std::dynamic_pointer_cast<Instruction>(op->getValue())) {
        ScheduleEarly(input, root);
        if (instr->m_bb->m_dom_depth < input->m_bb->m_dom_depth) {
          MoveInstructionBack(instr, input->m_bb);
        }
      }
    }
    if (binary_instr->IsICmp() || binary_instr->IsFCmp()) {
      MoveInstructionBack(instr, original_bb);
    }
  }
  if (auto load_instr = std::dynamic_pointer_cast<LoadInstruction>(instr)) {
    MoveInstructionBack(instr, root);
    for (auto op : instr->Operands()) {
      if (auto input = std::dynamic_pointer_cast<Instruction>(op->getValue())) {
        ScheduleEarly(input, root);
      }
    }
    MoveInstructionBack(instr, original_bb);
  }
  if (auto store_instr = std::dynamic_pointer_cast<StoreInstruction>(instr)) {
    MoveInstructionBack(instr, root);
    for (auto op : instr->Operands()) {
      if (auto input = std::dynamic_pointer_cast<Instruction>(op->getValue())) {
        ScheduleEarly(input, root);
      }
    }
    MoveInstructionBack(instr, original_bb);
  }
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

void ScheduleLate(std::shared_ptr<Instruction> instr,
                  std::shared_ptr<IRBuilder> builder) {
  if (instr->m_visited) return;
  instr->m_visited = true;

  // if (auto binary_instr =
  // std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
  //   std::vector<Use *> uses;
  //   for (auto &use : instr->m_use_list) {
  //     uses.push_back(use.get());
  //   }
  //   for (auto &use : uses) {
  //     if (auto user_instr
  //         = std::dynamic_pointer_cast<BinaryInstruction>(use->getUser())) {
  //       CombineInstruction(binary_instr, user_instr, builder);
  //     }
  //   }
  // }

  std::shared_ptr<BasicBlock> lca = nullptr;
  for (auto &y_use : instr->m_use_list) {
    if (auto y = std::dynamic_pointer_cast<Instruction>(y_use->getUser())) {
      ScheduleLate(y, builder);
      if (IsPinned(instr)) continue;
      auto use = y->m_bb;
      if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(y)) {
        auto it = std::find_if(
            phi->m_contents.begin(), phi->m_contents.end(),
            [&y_use](const auto &p) { return p.second == y_use.get(); });
        assert(it != phi->m_contents.end());
        use = it->first;
      }
      lca = FindLCA(lca, use);
    }
  }
  if (IsPinned(instr)) {
    return;
  }
  // use the latest and earliest blocks to pick final position
  if (lca == nullptr) {
    // there are some situations that lca will be nullptr
    // and the order does not matter much, the correctness does not change
    // known situations:
    // 1. expr stmt
    return;
  }
  assert(lca != nullptr);
  auto best = lca;
  while (lca != nullptr && lca != instr->m_bb) {
    if (lca->m_loop_depth < best->m_loop_depth) {
      best = lca;
    }
    lca = lca->m_idom;
  }
  if (lca == nullptr) {
    std::cerr << "[debug] strange: lca == nullptr" << std::endl;
    return;
  }
  MoveInstructionBack(instr, best);
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

// cmp + (xor) + br
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

  if (auto br = std::dynamic_pointer_cast<BranchInstruction>(terminator)) {
    std::stack<std::shared_ptr<Instruction>> instrs;
    instrs.push(br);
    std::shared_ptr<Value> now = br->m_cond->getValue();
    while (now) {
      if (auto cmp = std::dynamic_pointer_cast<BinaryInstruction>(now)) {
        if (cmp->m_op == IROp::XOR) {
          auto rhs = std::dynamic_pointer_cast<Constant>(
              cmp->m_rhs_val_use->getValue());
          assert(rhs->Evaluate().IntVal() == 1);
          instrs.push(cmp);
          now = cmp->m_lhs_val_use->getValue();
        } else if (cmp->IsICmp() || cmp->IsFCmp()) {
          instrs.push(cmp);
          break;
        } else {
          break;
        }
      } else {
        break;
      }
    }
    while (!instrs.empty()) {
      std::shared_ptr<Instruction> instr = instrs.top();
      instrs.pop();
      bb->m_instr_list.remove(instr);
      bb->m_instr_list.push_back(instr);
    }
  } else {
    // no need to remove all uses
    bb->m_instr_list.remove(terminator);
    bb->m_instr_list.push_back(terminator);
  }
}

void RunGCM(std::shared_ptr<Function> function,
            std::shared_ptr<IRBuilder> builder) {
  std::vector<std::shared_ptr<Instruction>> instrs;
  for (auto &bb : function->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      instr->m_placed = false;
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
    ScheduleLate(instr, builder);
  }

  for (auto &bb : function->m_bb_list) {
    MovePhiFirst(bb);
    MoveTerminatorLast(bb);
  }

  // 自己创造一种算法
  // dfs遍历支配树，记录当前以及之前bb已经定义了的变量（包含phi）
  // 对当前指令，判断所有operand是否已经出现
  // 如果已经出现，保持不动
  // 如果没有出现，那么这个operand应该在这条指令后面，要把它移到至少当前指令的前面
  // 这个移到当前指令的前面是递归的，记得给每个指令加vis判重
  // 重点是注意出现的load store的相对位置千万不能移动
  // gep是可以移动的，但是load store千万不能移动
  // gep也可能有依赖
  // alloca默认放最前面

  for (auto &bb : function->m_bb_list) {
    bb->m_visited = false;
  }
  std::stack<std::shared_ptr<BasicBlock>> stack;
  stack.push(function->m_bb_list.front());
  while (!stack.empty()) {
    auto bb = stack.top();
    stack.pop();
    if (bb->m_visited) continue;
    bb->m_visited = true;
    // std::cerr << "[debug] bb.id: " << bb->m_id << std::endl;

    // determine "pinned" instructions and "unpinned" instructions
    // here the concept is inside a basic block
    // "pinned" instructions cannot change their relative relationship
    // while "unpinned" ones can move as long as SSA properties hold
    std::vector<std::shared_ptr<Instruction>> unpinned, pinned;
    for (auto &instr : bb->m_instr_list) {
      if (!IsPinned(instr)) {
        instr->m_placed = false;
        unpinned.push_back(instr);
      } else {
        instr->m_placed = true;
        pinned.push_back(instr);
      }
    }

    // AllocaInstructions are unpinned, and should move to front
    for (auto it = unpinned.begin(); it != unpinned.end();) {
      auto instr = *it;
      if (instr->m_op == IROp::ALLOCA) {
        MoveInstructionFront(instr, bb, false);
        instr->m_placed = true;
        // if (!def.insert(instr).second) assert(false);  // GG
        it = unpinned.erase(it);
      } else {
        ++it;
      }
    }
    // PhiInstructions are unpinned, and should move before allocas
    for (auto it = unpinned.begin(); it != unpinned.end();) {
      auto instr = *it;
      if (instr->m_op == IROp::PHI) {
        MoveInstructionFront(instr, bb, false);
        instr->m_placed = true;
        // if (!def.insert(instr).second) assert(false);  // GG
        it = unpinned.erase(it);
      } else {
        ++it;
      }
    }
    // all remaining unpinned instructions are not been placed now
    for (auto &instr : unpinned) {
      assert(!instr->m_placed);
      assert(instr->m_bb == bb);
    }
    // the order of pinned instruction cannot change
    for (auto &instr : pinned) {
      assert(instr->m_placed);  // instr->m_placed = true;
      assert(instr->m_bb == bb);
    }

    // sort unpinned instructions
    // use topological sort to determine the order
    // now unpinned instructions include
    // 1. computational binary instructions
    // 2. getelementptr instructions
    // 3. function calls without side effect
    std::map<std::shared_ptr<Instruction>, int> mmp;  // in-degree
    for (auto &instr : unpinned) {
      assert(!instr->m_placed);
      if (auto binary_instr
          = std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
        if (mmp.find(binary_instr) == mmp.end()) mmp[binary_instr] = 0;

        auto lhs = binary_instr->m_lhs_val_use->getValue();
        if (auto lhs_instr
            = std::dynamic_pointer_cast<BinaryInstruction>(lhs)) {
          if (std::find(unpinned.begin(), unpinned.end(), lhs_instr)
              != unpinned.end()) {
            assert(!lhs_instr->m_placed);
            ++mmp[lhs_instr];
          }
        } else if (auto lhs_call
                   = std::dynamic_pointer_cast<CallInstruction>(lhs)) {
          if (std::find(unpinned.begin(), unpinned.end(), lhs_call)
              != unpinned.end()) {
            assert(!lhs_call->m_placed);
            ++mmp[lhs_call];
          }
        }
        auto rhs = binary_instr->m_rhs_val_use->getValue();
        if (auto rhs_instr
            = std::dynamic_pointer_cast<BinaryInstruction>(rhs)) {
          if (std::find(unpinned.begin(), unpinned.end(), rhs_instr)
              != unpinned.end()) {
            assert(!rhs_instr->m_placed);
            ++mmp[rhs_instr];
          }
        } else if (auto rhs_call
                   = std::dynamic_pointer_cast<CallInstruction>(rhs)) {
          if (std::find(unpinned.begin(), unpinned.end(), rhs_call)
              != unpinned.end()) {
            assert(!rhs_call->m_placed);
            ++mmp[rhs_call];
          }
        }
      } else if (auto gep_instr
                 = std::dynamic_pointer_cast<GetElementPtrInstruction>(instr)) {
        if (mmp.find(gep_instr) == mmp.end()) mmp[gep_instr] = 0;

        auto addr = gep_instr->m_addr->getValue();
        // if addr is gep instruction, this gep instruction should be placed
        // before this gep instruction
        if (auto gep_addr
            = std::dynamic_pointer_cast<GetElementPtrInstruction>(addr)) {
          if (std::find(unpinned.begin(), unpinned.end(), gep_addr)
              != unpinned.end()) {
            assert(!gep_addr->m_placed);
            ++mmp[gep_addr];
          }
        }
        for (auto index : gep_instr->m_indices) {
          auto val = index->getValue();
          if (auto binary_index
              = std::dynamic_pointer_cast<BinaryInstruction>(val)) {
            if (std::find(unpinned.begin(), unpinned.end(), binary_index)
                != unpinned.end()) {
              assert(!binary_index->m_placed);
              ++mmp[binary_index];
            }
          } else if (auto call_index
                     = std::dynamic_pointer_cast<CallInstruction>(val)) {
            if (std::find(unpinned.begin(), unpinned.end(), call_index)
                != unpinned.end()) {
              assert(!call_index->m_placed);
              ++mmp[call_index];
            }
          }
        }
      } else if (auto call_instr
                 = std::dynamic_pointer_cast<CallInstruction>(instr)) {
        if (mmp.find(call_instr) == mmp.end()) mmp[call_instr] = 0;

        for (auto param : call_instr->m_params) {
          auto val = param->getValue();
          if (auto binary_param
              = std::dynamic_pointer_cast<BinaryInstruction>(val)) {
            if (std::find(unpinned.begin(), unpinned.end(), binary_param)
                != unpinned.end()) {
              assert(!binary_param->m_placed);
              ++mmp[binary_param];
            }
          } else if (auto gep_param
                     = std::dynamic_pointer_cast<GetElementPtrInstruction>(
                         val)) {
            if (std::find(unpinned.begin(), unpinned.end(), gep_param)
                != unpinned.end()) {
              assert(!gep_param->m_placed);
              ++mmp[gep_param];
            }
          } else if (auto call_param
                     = std::dynamic_pointer_cast<CallInstruction>(val)) {
            if (std::find(unpinned.begin(), unpinned.end(), call_param)
                != unpinned.end()) {
              assert(!call_param->m_placed);
              ++mmp[call_param];
            }
          }
        }
      }
    }

    std::queue<std::shared_ptr<Instruction>> q;
    for (auto &[instr, cnt] : mmp) {
      if (cnt == 0) {
        q.push(instr);
      }
    }
    std::vector<std::shared_ptr<Instruction>> order;
    while (!q.empty()) {
      std::shared_ptr<Instruction> instr = q.front();
      q.pop();
      order.push_back(instr);
      if (auto binary_instr
          = std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
        auto lhs = binary_instr->m_lhs_val_use->getValue();
        if (auto lhs_instr
            = std::dynamic_pointer_cast<BinaryInstruction>(lhs)) {
          if (std::find(unpinned.begin(), unpinned.end(), lhs_instr)
              != unpinned.end()) {
            if (--mmp[lhs_instr] == 0) {
              q.push(lhs_instr);
            }
          }
        } else if (auto lhs_call
                   = std::dynamic_pointer_cast<CallInstruction>(lhs)) {
          if (std::find(unpinned.begin(), unpinned.end(), lhs_call)
              != unpinned.end()) {
            if (--mmp[lhs_call] == 0) {
              q.push(lhs_call);
            }
          }
        }

        auto rhs = binary_instr->m_rhs_val_use->getValue();
        if (auto rhs_instr
            = std::dynamic_pointer_cast<BinaryInstruction>(rhs)) {
          if (std::find(unpinned.begin(), unpinned.end(), rhs_instr)
              != unpinned.end()) {
            if (--mmp[rhs_instr] == 0) {
              q.push(rhs_instr);
            }
          }
        } else if (auto rhs_call
                   = std::dynamic_pointer_cast<CallInstruction>(rhs)) {
          if (std::find(unpinned.begin(), unpinned.end(), rhs_call)
              != unpinned.end()) {
            if (--mmp[rhs_call] == 0) {
              q.push(rhs_call);
            }
          }
        }
      } else if (auto gep_instr
                 = std::dynamic_pointer_cast<GetElementPtrInstruction>(instr)) {
        auto addr = gep_instr->m_addr->getValue();
        if (auto gep_addr
            = std::dynamic_pointer_cast<GetElementPtrInstruction>(addr)) {
          if (std::find(unpinned.begin(), unpinned.end(), gep_addr)
              != unpinned.end()) {
            if (--mmp[gep_addr] == 0) {
              q.push(gep_addr);
            }
          }
        }
        for (auto index : gep_instr->m_indices) {
          auto val = index->getValue();
          if (auto binary_index
              = std::dynamic_pointer_cast<BinaryInstruction>(val)) {
            if (std::find(unpinned.begin(), unpinned.end(), binary_index)
                != unpinned.end()) {
              if (--mmp[binary_index] == 0) {
                q.push(binary_index);
              }
            }
          } else if (auto call_index
                     = std::dynamic_pointer_cast<CallInstruction>(val)) {
            if (std::find(unpinned.begin(), unpinned.end(), call_index)
                != unpinned.end()) {
              if (--mmp[call_index] == 0) {
                q.push(call_index);
              }
            }
          }
        }
      } else if (auto call_instr
                 = std::dynamic_pointer_cast<CallInstruction>(instr)) {
        for (auto param : call_instr->m_params) {
          auto val = param->getValue();
          if (auto binary_param
              = std::dynamic_pointer_cast<BinaryInstruction>(val)) {
            if (std::find(unpinned.begin(), unpinned.end(), binary_param)
                != unpinned.end()) {
              if (--mmp[binary_param] == 0) {
                q.push(binary_param);
              }
            }
          } else if (auto gep_param
                     = std::dynamic_pointer_cast<GetElementPtrInstruction>(
                         val)) {
            if (std::find(unpinned.begin(), unpinned.end(), gep_param)
                != unpinned.end()) {
              if (--mmp[gep_param] == 0) {
                q.push(gep_param);
              }
            }
          } else if (auto call_param
                     = std::dynamic_pointer_cast<CallInstruction>(val)) {
            if (std::find(unpinned.begin(), unpinned.end(), call_param)
                != unpinned.end()) {
              if (--mmp[call_param] == 0) {
                q.push(call_param);
              }
            }
          }
        }
      }
    }
    assert(order.size() == mmp.size());
    // std::reverse(order.begin(), order.end());
    if (!order.empty()) {
      // std::cerr << "[debug] size: " << order.size() << std::endl;
    }

    // for every pinned instruction, make sure all uses of operands appear
    // at least before it

    std::stack<std::shared_ptr<Instruction>> w;  // pinned instructions
    for (auto it = pinned.rbegin(); it != pinned.rend(); ++it) {
      if ((*it)->m_op != IROp::PHI) w.push(*it);
    }
    while (!w.empty()) {
      std::shared_ptr<Instruction> instr = w.top();
      w.pop();
      assert(instr->m_placed);
      for (auto operand : instr->Operands()) {
        if (auto sub_instr
            = std::dynamic_pointer_cast<Instruction>(operand->getValue())) {
          if (!sub_instr->m_placed) {
            // auto it = std::find(unpinned.begin(), unpinned.end(), sub_instr);
            // assert(it != unpinned.end());
            // MoveInstructionBefore(sub_instr, instr);
            // sub_instr->m_placed = true;
            // w.push(sub_instr);
            // unpinned.erase(it);

            auto temp = std::find(order.begin(), order.end(), sub_instr);
            if (temp == order.end())
              continue;  // not computational binary, not gep
            bool flag = true;
            for (auto it = temp; it != order.end(); ++it) {
              auto temp_instr = *it;
              if (!temp_instr->m_placed) {
                flag = false;  // not the time
                break;
              }
            }
            if (!flag) continue;
            auto begin = std::find(order.rbegin(), order.rend(), sub_instr);
            assert(begin != order.rend());
            for (auto it = begin; it != order.rend(); ++it) {
              auto temp_instr = *it;
              if (!temp_instr->m_placed) {
                MoveInstructionBefore(temp_instr, instr);
                temp_instr->m_placed = true;
              }
            }
          }
        }
      }
    }

    for (auto &instr : order) {
      if (!instr->m_placed) {
        // place it right before its first use by others
        for (auto use_instr : bb->m_instr_list) {
          if (use_instr == instr || use_instr->m_op == IROp::PHI) continue;
          auto it2
              = std::find_if(instr->m_use_list.begin(), instr->m_use_list.end(),
                             [&use_instr](std::unique_ptr<Use> &use) {
                               return use->getUser() == use_instr;
                             });
          if (it2 != instr->m_use_list.end()) {
            MoveInstructionBefore(instr, use_instr);
            break;
          }
        }
        instr->m_placed = true;
      }
    }

    // check if all instructions are placed (important for later use!)
    for (auto &instr : bb->m_instr_list) {
      assert(instr->m_placed);
    }
    if (auto br
        = std::dynamic_pointer_cast<BranchInstruction>(bb->LastInstruction())) {
      if (bb->m_instr_list.size() >= 2
          && !br->m_cond->getValue()->m_type.IsConst()) {
        auto last_two_instr = *std::next(bb->m_instr_list.rbegin());
        assert(last_two_instr == br->m_cond->getValue());
      }
    }

    // expand worklist
    auto succ = bb->Successors();
    for (auto it = succ.rbegin(); it != succ.rend(); ++it) {
      auto s = *it;
      if (!s->m_visited) {
        stack.push(s);
      }
    }
  }
}

void IRPassManager::GCMPass() {
  SideEffectPass();
  for (auto &func : m_builder->m_module->m_function_list) {
    ComputeLoopRelationship(func);
    // DeadCodeElimination(func);
    ComputeDominanceRelationship(func);

    assert(!func->m_bb_list.empty());
    RunGCM(func, m_builder);
    ReplaceTrivialBranchByJump(func);
    RemoveUnusedBasicBlocks(func);
    RemoveTrivialPhis(func);
    RemoveTrivialBasicBlocks(func);
  }
  // RemoveUnusedFunctions(m_builder->m_module);
}