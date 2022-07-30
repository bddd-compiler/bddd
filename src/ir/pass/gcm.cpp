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

void ScheduleLate(std::shared_ptr<Instruction> instr) {
  if (instr->m_visited) return;
  instr->m_visited = true;

  std::shared_ptr<BasicBlock> lca = nullptr;
  for (auto &y_use : instr->m_use_list) {
    if (auto y = std::dynamic_pointer_cast<Instruction>(y_use->getUser())) {
      ScheduleLate(y);
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
  while (lca != instr->m_bb) {
    if (lca->m_loop_depth < best->m_loop_depth) {
      best = lca;
    }
    lca = lca->m_idom;
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
    ScheduleLate(instr);
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
    std::cerr << "[debug]" << bb->m_id << std::endl;

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
    // 2. getelementptr instructions
    // need to place at least in front of the first load/store
    std::vector<std::shared_ptr<GetElementPtrInstruction>> gep_order;
    std::map<std::shared_ptr<GetElementPtrInstruction>, int> gep_mp;  // degree
    for (auto it = unpinned.begin(); it != unpinned.end();) {
      auto instr = *it;
      if (auto gep_instr
          = std::dynamic_pointer_cast<GetElementPtrInstruction>(instr)) {
        if (gep_mp.find(gep_instr) == gep_mp.end()) {
          gep_mp[gep_instr] = 0;
        }
        auto addr = gep_instr->m_addr->getValue();
        // if addr is alloca or global variable, it does not matter
        // if addr is another gep, then they have order
        if (auto gep_addr
            = std::dynamic_pointer_cast<GetElementPtrInstruction>(addr)) {
          if (gep_addr->m_bb == bb) {
            assert(!gep_addr->m_placed);
            ++gep_mp[gep_instr];
          }
        }
        it = unpinned.erase(it);
      } else {
        ++it;
      }
    }
    std::queue<std::shared_ptr<GetElementPtrInstruction>> gep_queue;
    for (auto &[gep, cnt] : gep_mp) {
      if (cnt == 0) {
        gep_queue.push(gep);
      }
    }
    while (!gep_queue.empty()) {
      std::shared_ptr<GetElementPtrInstruction> instr = gep_queue.front();
      gep_queue.pop();
      gep_order.push_back(instr);
      auto addr = instr->m_addr->getValue();
      if (auto gep_addr
          = std::dynamic_pointer_cast<GetElementPtrInstruction>(addr)) {
        if (gep_addr->m_bb == bb) {
          --gep_mp[gep_addr];
          if (gep_mp[gep_addr] == 0) {
            gep_queue.push(gep_addr);
          }
        }
      }
    }
    assert(gep_order.size() == gep_mp.size());
    for (auto it = gep_order.rbegin(); it != gep_order.rend(); ++it) {
      auto instr = *it;
      // move to the proper position
      // check all operands
      long maxd = -1;
      std::shared_ptr<Instruction> target = nullptr;
      for (auto op : instr->Operands()) {
        auto val = op->getValue();
        auto val_instr = std::dynamic_pointer_cast<Instruction>(val);
        if (val_instr != nullptr && val_instr->m_bb == bb) {
          auto it2 = std::find(bb->m_instr_list.begin(), bb->m_instr_list.end(),
                               val_instr);
          assert(it2 != bb->m_instr_list.end());
          auto distance = std::distance(bb->m_instr_list.begin(), it2);
          if (maxd < distance) {
            maxd = distance;
            target = val_instr;
          }
        }
      }
      if (maxd == -1) {
        MoveInstructionFront(instr, instr->m_bb);
      } else {
        assert(maxd < bb->m_instr_list.size());
        assert(target != nullptr);
        MoveInstructionBehind(instr, target);
      }
      instr->m_placed = true;
      // std::remove(unpinned.begin(), unpinned.end(), instr);
    }

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
    for (auto &instr : unpinned) {
      assert(!instr->m_placed);
    }
    // the order of pinned instruction cannot change
    for (auto &instr : pinned) {
      assert(instr->m_placed);  // instr->m_placed = true;
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
            MoveInstructionBefore(sub_instr, instr);
            sub_instr->m_placed = true;
            w.push(sub_instr);
          }
        }
      }
    }
    // check if all instructions are placed (important later on!)
    // unfortunately not
    // these values are the computations that are hoisted from below to above
    // using topological sort to determine the order
    // it seems we should deal with two kinds of instruction
    // 1. binary instructions (icmp/fcmp instructions are mostly close to br)
    std::vector<std::shared_ptr<BinaryInstruction>> order;
    std::map<std::shared_ptr<BinaryInstruction>, int> mmp;
    for (auto &instr : unpinned) {
      if (instr->m_placed) continue;
      if (auto binary_instr
          = std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
        if (mmp.find(binary_instr) == mmp.end()) {
          mmp[binary_instr] = 0;
        }
        auto lhs = binary_instr->m_lhs_val_use->getValue();
        if (auto lhs_instr
            = std::dynamic_pointer_cast<BinaryInstruction>(lhs)) {
          if (lhs_instr->m_bb == bb) {
            assert(!lhs_instr->m_placed);
            ++mmp[lhs_instr];
          }
        }
        auto rhs = binary_instr->m_rhs_val_use->getValue();
        if (auto rhs_instr
            = std::dynamic_pointer_cast<BinaryInstruction>(rhs)) {
          if (rhs_instr->m_bb == bb) {
            assert(!rhs_instr->m_placed);
            ++mmp[rhs_instr];
          }
        }
      }
    }
    std::queue<std::shared_ptr<BinaryInstruction>> q;
    for (auto &[comp, cnt] : mmp) {
      if (cnt == 0) {
        q.push(comp);
      }
    }
    while (!q.empty()) {
      std::shared_ptr<BinaryInstruction> instr = q.front();
      q.pop();
      order.push_back(instr);
      auto lhs = instr->m_lhs_val_use->getValue();
      if (auto lhs_instr = std::dynamic_pointer_cast<BinaryInstruction>(lhs)) {
        if (lhs_instr->m_bb == bb) {
          --mmp[lhs_instr];
          if (mmp[lhs_instr] == 0) {
            q.push(lhs_instr);
          }
        }
      }
      auto rhs = instr->m_rhs_val_use->getValue();
      if (auto rhs_instr = std::dynamic_pointer_cast<BinaryInstruction>(rhs)) {
        if (rhs_instr->m_bb == bb) {
          --mmp[rhs_instr];
          if (mmp[rhs_instr] == 0) {
            q.push(rhs_instr);
          }
        }
      }
    }
    assert(order.size() == mmp.size());
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
      auto instr = *it;
      MoveInstructionBefore(instr, bb->LastInstruction());
      instr->m_placed = true;
      // std::remove(unpinned.begin(), unpinned.end(), instr);
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
  }
}