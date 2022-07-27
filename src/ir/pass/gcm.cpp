#include <unordered_map>

#include "ir/ir-pass-manager.h"

// definition of natural loop:
// loop body is a strongly connected component of bbs with unique header
// (entry) header is the only way to enter the natural loop
class NaturalLoop : public std::enable_shared_from_this<NaturalLoop> {
public:
  std::vector<std::shared_ptr<NaturalLoop>> m_sub_loops;
  std::vector<std::shared_ptr<BasicBlock>> m_bbs;
  std::shared_ptr<NaturalLoop> m_fa_loop;

  explicit NaturalLoop(std::shared_ptr<BasicBlock> header)
      : m_sub_loops(), m_bbs{std::move(header)}, m_fa_loop(nullptr) {}

  std::shared_ptr<BasicBlock> Header() { return m_bbs.front(); }

  int LoopDepth() {
    int cnt = 0;
    auto loop = shared_from_this();
    while (loop != nullptr) {
      loop = loop->m_fa_loop;
      ++cnt;
    }
    return cnt;
  }
};

std::vector<std::shared_ptr<BasicBlock>> w;
std::unordered_map<std::shared_ptr<BasicBlock>, std::shared_ptr<NaturalLoop>>
    deepest_loop;
std::vector<std::shared_ptr<NaturalLoop>> top_loops;

int GetLoopDepth(std::shared_ptr<BasicBlock> bb) {
  auto it
      = std::find_if(deepest_loop.begin(), deepest_loop.end(),
                     [=](const auto &x) { return x.first.get() == bb.get(); });
  if (it == deepest_loop.end())
    return 0;  // not in loop
  else
    return it->second->LoopDepth();
}

void CollectLoopInfo(std::shared_ptr<BasicBlock> header) {
  for (auto bb : header->m_dominators) {
    CollectLoopInfo(bb);
  }
  // assert(w.empty());
  for (auto pred : header->Predecessors()) {
    if (header->Dominate(pred)) {
      // back edge
      w.push_back(pred);
    }
  }
  if (w.empty()) return;
  auto loop = std::make_shared<NaturalLoop>(header);
  while (!w.empty()) {
    auto pred = w.back();
    w.pop_back();
    auto it = std::find_if(
        deepest_loop.begin(), deepest_loop.end(),
        [=](const auto &x) { return x.first.get() == pred.get(); });
    if (it == deepest_loop.end()) {
      // should insert
      deepest_loop[pred] = loop;
      if (pred != header) {
        w.insert(w.end(), pred->m_predecessors.begin(),
                 pred->m_predecessors.end());
      }
    } else {
      auto sub_loop = it->second;
      while (sub_loop->m_fa_loop) sub_loop = sub_loop->m_fa_loop;
      if (sub_loop != loop) {
        sub_loop->m_fa_loop = loop;
        for (auto &p : sub_loop->Header()->Predecessors()) {
          auto it2 = std::find_if(
              deepest_loop.begin(), deepest_loop.end(),
              [=](const auto &x) { return x.first.get() == pred.get(); });
          if (it2 == deepest_loop.end() || it->second != sub_loop) {
            w.push_back(p);
          }
        }
      }
    }
  }
}

void PopulateLoopInfo(std::shared_ptr<BasicBlock> bb) {
  if (bb->m_visited) return;
  bb->m_visited = true;
  for (auto &s : bb->Successors()) {
    PopulateLoopInfo(s);
  }
  auto it
      = std::find_if(deepest_loop.begin(), deepest_loop.end(),
                     [=](const auto &x) { return x.first.get() == bb.get(); });
  auto sub_loop = (it == deepest_loop.end()) ? nullptr : it->second;
  if (sub_loop != nullptr && sub_loop->Header() == bb) {
    if (sub_loop->m_fa_loop) {
      sub_loop->m_fa_loop->m_sub_loops.push_back(sub_loop);
    } else {
      top_loops.push_back(sub_loop);
    }
    // reverse the order here
    std::reverse(std::next(sub_loop->m_bbs.begin()), sub_loop->m_bbs.end());
    std::reverse(sub_loop->m_sub_loops.begin(), sub_loop->m_sub_loops.end());
    sub_loop = sub_loop->m_fa_loop;
  }
  while (sub_loop) {
    sub_loop->m_bbs.push_back(bb);
    sub_loop = sub_loop->m_fa_loop;
  }
}

// prerequisite: computed dominance relationship
void ComputeLoopRelationship(std::shared_ptr<Function> function) {
  w.clear();
  deepest_loop.clear();
  top_loops.clear();
  CollectLoopInfo(function->m_bb_list.front());
  for (auto &bb : function->m_bb_list) {
    bb->m_visited = false;
  }
  PopulateLoopInfo(function->m_bb_list.front());
  for (auto &bb : function->m_bb_list) {
    bb->m_loop_depth = GetLoopDepth(bb);
  }
}

void MoveInstruction(std::shared_ptr<Instruction> instr,
                     std::shared_ptr<BasicBlock> bb) {
  assert(bb != nullptr);
  instr->m_bb->m_instr_list.remove(instr);
  instr->m_bb = bb;
  instr->m_bb->m_instr_list.push_back(instr);
}

void ScheduleEarly(std::shared_ptr<Instruction> instr,
                   std::shared_ptr<Function> function) {
  if (instr->m_visited) return;
  instr->m_visited = true;
  auto entry = function->m_bb_list.front();

  if (auto binary_instr = std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
    MoveInstruction(binary_instr, entry);

    if (auto lhs_instr = std::dynamic_pointer_cast<Instruction>(
            binary_instr->m_lhs_val_use->m_value)) {
      ScheduleEarly(lhs_instr, function);
      if (binary_instr->m_bb->m_dom_depth < lhs_instr->m_bb->m_dom_depth) {
        MoveInstruction(binary_instr, lhs_instr->m_bb);
      }
    }
    if (auto rhs_instr = std::dynamic_pointer_cast<Instruction>(
            binary_instr->m_rhs_val_use->m_value)) {
      ScheduleEarly(rhs_instr, function);
      if (binary_instr->m_bb->m_dom_depth < rhs_instr->m_bb->m_dom_depth) {
        MoveInstruction(binary_instr, rhs_instr->m_bb);
      }
    }
  }

  if (auto gep_instr
      = std::dynamic_pointer_cast<GetElementPtrInstruction>(instr)) {
    MoveInstruction(gep_instr, entry);

    if (auto addr_instr
        = std::dynamic_pointer_cast<Instruction>(gep_instr->m_addr->m_value)) {
      ScheduleEarly(addr_instr, function);
      if (gep_instr->m_bb->m_dom_depth < addr_instr->m_bb->m_dom_depth) {
        MoveInstruction(gep_instr, addr_instr->m_bb);
      }
    }
    for (auto &index : gep_instr->m_indices) {
      if (auto index_instr
          = std::dynamic_pointer_cast<Instruction>(index->m_value)) {
        ScheduleEarly(index_instr, function);
        if (gep_instr->m_bb->m_dom_depth < index_instr->m_bb->m_dom_depth) {
          MoveInstruction(gep_instr, index_instr->m_bb);
        }
      }
    }
  }

  if (auto load_instr = std::dynamic_pointer_cast<LoadInstruction>(instr)) {
    MoveInstruction(load_instr, entry);

    if (auto addr_instr
        = std::dynamic_pointer_cast<Instruction>(load_instr->m_addr->m_value)) {
      ScheduleEarly(addr_instr, function);
      if (load_instr->m_bb->m_dom_depth < addr_instr->m_bb->m_dom_depth) {
        MoveInstruction(load_instr, addr_instr->m_bb);
      }
    }
  }

  // TODO(garen): pure function call
}

std::shared_ptr<BasicBlock> FindLCA(std::shared_ptr<BasicBlock> a,
                                    std::shared_ptr<BasicBlock> b) {
  if (a == nullptr) return b;
  while (a->m_dom_depth < b->m_dom_depth) {
    a = a->m_idom;
  }
  while (b->m_dom_depth < a->m_dom_depth) {
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

  // TODO(garen): eliminate chain calculation

  for (auto &_y : instr->m_use_list) {
    if (auto y = std::dynamic_pointer_cast<Instruction>(_y->m_user)) {
      ScheduleLate(y);
      auto use = y->m_bb;
      if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(y)) {
        auto it = std::find_if(
            phi->m_contents.begin(), phi->m_contents.end(),
            [=](const auto &p) { return p.second.get() == _y.get(); });
        if (it != nullptr) {
          use = it->first;
        }
      }
      lca = FindLCA(lca, use);
    }
  }
  // TODO(garen): use the latest and earliest blocks to pick final position

  auto best = lca;
  while (lca != instr->m_bb) {
    if (lca->m_loop_depth < best->m_loop_depth) {
      best = lca;
    }
    lca = lca->m_idom;
  }
  // instr->m_bb = best;
  if (instr->m_bb != best) {
    MoveInstruction(instr, best);
  }
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
    ScheduleEarly(instr, function);
  }

  for (auto &instr : instrs) {
    instr->m_visited = false;
  }
  for (auto &instr : instrs) {
    ScheduleLate(instr);
  }

  // for (auto &bb : function->bList) {
  //   MoveIcmpBack(bb);
  //   MoveTerminalInstBack(bb);
  //   MovePhiFront(bb);
  //   MescheduleInstOrder(bb);
  //   MoveIcmpBack(bb);
  //   MoveTerminalInstBack(bb);
  //   MovePhiFront(bb);
  // }
}

void IRPassManager::GCMPass() {
  for (auto &func : m_builder->m_module->m_function_list) {
    if (func->m_bb_list.empty()) continue;
    RunGCM(func, m_builder);
  }
}