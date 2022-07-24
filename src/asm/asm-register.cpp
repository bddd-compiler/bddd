#include "asm/asm-register.h"

RegisterAllocator::RegisterAllocator(std::shared_ptr<ASM_Module> module) {
  m_module = module;
}

void RegisterAllocator::Allocate() {
  assert(m_module);
  for (auto& func : m_module->m_funcs) {
    setCurFunction(func);
    getPrecoloredAndInitial();
    AllocateCurFunc();
    for (auto& node : coloredNodes) {
      int c = color[node];
      if (node->m_op_type == OperandType::VREG) {
        node->m_op_type = OperandType::REG;
        node->m_is_rreg = true;
        node->m_rreg = (RReg)c;
      }
    }
    initial.clear();
  }
}

void RegisterAllocator::setCurFunction(std::shared_ptr<ASM_Function> func) {
  m_cur_func = func;
}

void RegisterAllocator::LivenessAnalysis() {
  // calculate def and use set of all blocks
  for (auto& b : m_cur_func->m_blocks) {
    b->m_def.clear();
    b->m_use.clear();
    for (auto& i : b->m_insts) {
      for (auto& def : i->m_def) {
        if (b->m_def.find(def) == b->m_def.end()) {
          b->m_def.insert(def);
        }
      }
      for (auto& use : i->m_use) {
        if (b->m_use.find(use) == b->m_use.end()) {
          b->m_use.insert(use);
        }
      }
    }
    b->m_livein.clear();
    b->m_liveout.clear();
  }

  // calculate liveIn and liveOut of all blocks
  // loop until nothing was added
  bool end;
  do {
    end = true;
    for (auto iter = m_cur_func->m_blocks.rbegin();
         iter != m_cur_func->m_blocks.rend(); iter++) {
      auto b = *iter;
      // record in'[b] and out'[b]
      auto temp_in = b->m_livein;
      auto temp_out = b->m_liveout;

      // in[b] = use[b] union (out[b] - def[b])
      b->m_livein = b->m_use;
      for (auto& out : b->m_liveout) {
        if (b->m_def.find(out) == b->m_def.end()
            && b->m_livein.find(out) == b->m_livein.end()) {
          b->m_livein.insert(out);
        }
      }

      // out[b] = union(in[s]), s in succ[b]
      for (auto& successor : b->getSuccessors()) {
        for (auto& in : successor->m_livein) {
          if (b->m_liveout.find(in) == b->m_liveout.end()) {
            b->m_liveout.insert(in);
          }
        }
      }

      // check if loop is end
      if (b->m_livein != temp_in || b->m_liveout != temp_out) end = false;
    }
  } while (!end);

  // print
//   for (auto& b : m_cur_func->m_blocks) {
//     std::cout << b->m_label << ":" << std::endl;
//     std::cout << "liveIn:  ";
//     for (auto& in : b->m_livein) std::cout << in->getName() << " ";
//     std::cout << std::endl;
//     std::cout << "liveOut: ";
//     for (auto& out : b->m_liveout) std::cout << out->getName() << " ";
//     std::cout << std::endl;
//   }
}

void RegisterAllocator::getPrecoloredAndInitial() {
  for (auto& b : m_cur_func->m_blocks) {
    for (auto& i : b->m_insts) {
      for (auto& op : i->m_def) {
        if (op->m_op_type == OperandType::VREG) initial.insert(op);
      }
      for (auto& op : i->m_use) {
        if (op->m_op_type == OperandType::VREG) initial.insert(op);
      }
    }
    for (auto& [i, o] : Operand::rreg_map) {
      precolored.insert(o);
    }
  }
}

void RegisterAllocator::AllocateCurFunc() {
  LivenessAnalysis();
  Build();
  MkWorklist();
  do {
    if (!simplifyWorklist.empty()) {
      Simplify();
    } else if (!worklistMoves.empty()) {
      Coalesce();
    } else if (!freezeWorklist.empty()) {
      Freeze();
    } else if (!spillWorklist.empty()) {
      SelectSpill();
    }
  } while (!simplifyWorklist.empty() || !worklistMoves.empty()
           || !freezeWorklist.empty() || !spillWorklist.empty());
  AssignColors();
  if (!spilledNodes.empty()) {
    RewriteProgram();
    AllocateCurFunc();
  }
}

void RegisterAllocator::AddEdge(OpPtr u, OpPtr v) {
  auto e = std::make_pair(u, v);
  if (adjSet.find(e) == adjSet.end() && u != v) {
    adjSet.insert(e);
    adjSet.insert(std::make_pair(v, u));
    if (precolored.find(u) == precolored.end()) {
      adjList[u].insert(v);
      degree[u]++;
    }
    if (precolored.find(v) == precolored.end()) {
      adjList[v].insert(u);
      degree[v]++;
    }
  }
}

void RegisterAllocator::Build() {
  for (auto& b : m_cur_func->m_blocks) {
    std::unordered_set<OpPtr> live = b->m_liveout;
    for (auto iter = b->m_insts.rbegin(); iter != b->m_insts.rend(); iter++) {
      std::shared_ptr<ASM_Instruction> inst = *iter;
      if (auto I = std::dynamic_pointer_cast<MOVInst>(inst)) {
        for (auto& use : I->m_use) {
          live.erase(use);
        }
        for (auto& n : I->m_def) {
          moveList[n].insert(I);
        }
        for (auto& n : I->m_use) {
          moveList[n].insert(I);
        }
        worklistMoves.insert(I);
      }
      for (auto& def : inst->m_def) {
        live.insert(def);
      }
      for (auto& d : inst->m_def) {
        for (auto& l : live) {
          AddEdge(l, d);
        }
      }
      // live := use(I) ∪ (live\def(I))
      for (auto& def : inst->m_def) {
        live.erase(def);
      }
      for (auto& use : inst->m_use) {
        live.insert(use);
      }
    }
  }
}

std::unordered_set<OpPtr> RegisterAllocator::Adjacent(OpPtr n) {
  if (adjList.find(n) == adjList.end()) {
    return {};
  }
  auto ret = adjList[n];
  for (auto& op : selectStack) {
    ret.erase(op);
  }
  for (auto& op : coalescedNodes) {
    ret.erase(op);
  }
  return ret;
}

std::unordered_set<std::shared_ptr<MOVInst>> RegisterAllocator::NodeMoves(
    OpPtr n) {
  std::unordered_set<std::shared_ptr<MOVInst>> ret;
  if (moveList.find(n) == moveList.end()) {
    return {};
  }
  for (auto& m : moveList[n]) {
    if (activeMoves.find(m) != activeMoves.end()
        || worklistMoves.find(m) != worklistMoves.end()) {
      ret.insert(m);
    }
  }
  return ret;
}

bool RegisterAllocator::MoveRelated(OpPtr n) { return !NodeMoves(n).empty(); }

void RegisterAllocator::MkWorklist() {
  for (auto& n : initial) {
    if (degree[n] >= K) {
      spillWorklist.insert(n);
    } else if (MoveRelated(n)) {
      freezeWorklist.insert(n);
    } else {
      simplifyWorklist.insert(n);
    }
    initial.erase(n);
  }
}

void RegisterAllocator::Simplify() {
  assert(!simplifyWorklist.empty());
  OpPtr n = *simplifyWorklist.begin();
  simplifyWorklist.erase(n);
  selectStack.push_back(n);
  for (auto& m : Adjacent(n)) {
    DecrementDegree(m);
  }
}

void RegisterAllocator::DecrementDegree(OpPtr m) {
  int d = degree[m]--;
  if (d == K) {
    auto adj = Adjacent(m);
    adj.insert(m);
    EnableMoves(adj);
    spillWorklist.erase(m);
    if (MoveRelated(m)) {
      freezeWorklist.insert(m);
    } else {
      simplifyWorklist.insert(m);
    }
  }
}

void RegisterAllocator::EnableMoves(std::unordered_set<OpPtr> nodes) {
  for (auto& n : nodes)
    for (auto& m : NodeMoves(n))
      if (activeMoves.find(m) != activeMoves.end()) {
        activeMoves.erase(m);
        worklistMoves.insert(m);
      }
}

void RegisterAllocator::Coalesce() {
  assert(!worklistMoves.empty());
  auto m = *worklistMoves.begin();
  auto x = GetAlias(m->m_src);
  auto y = GetAlias(m->m_dest);
  OpPtr u, v;
  if (precolored.find(y) != precolored.end()) {
    u = y;
    v = x;
  } else {
    u = x;
    v = y;
  }
  worklistMoves.erase(m);
  if (u == v) {
    coalescedMoves.insert(m);
    AddWorkList(u);
  } else if (precolored.find(v) != precolored.end()
             || adjSet.find(std::make_pair(u, v)) != adjSet.end()) {
    constrainedMoves.insert(m);
    AddWorkList(u);
    AddWorkList(v);
  } else if (precolored.find(u) != precolored.end() && AllOK(u, v)
             || precolored.find(u) == precolored.end()
                    && ConservativeAdj(u, v)) {
    coalescedMoves.insert(m);
    Combine(u, v);
    AddWorkList(u);
  } else {
    activeMoves.insert(m);
  }
}

void RegisterAllocator::AddWorkList(OpPtr u) {
  if (precolored.find(u) == precolored.end() && !MoveRelated(u)
      && degree[u] < K) {
    freezeWorklist.erase(u);
    simplifyWorklist.insert(u);
  }
}

bool RegisterAllocator::OK(OpPtr t, OpPtr r) {
  return degree[t] < K || precolored.find(t) != precolored.end()
         || adjSet.find(std::make_pair(t, r)) != adjSet.end();
}

bool RegisterAllocator::Conservative(std::unordered_set<OpPtr> nodes) {
  int k = 0;
  for (auto& n : nodes)
    if (degree[n] >= K) k++;
  return k < K;
}

OpPtr RegisterAllocator::GetAlias(OpPtr n) {
  return coalescedNodes.find(n) != coalescedNodes.end() ? GetAlias(alias[n])
                                                        : n;
}

void RegisterAllocator::Combine(OpPtr u, OpPtr v) {
  if (freezeWorklist.find(v) != freezeWorklist.end()) {
    freezeWorklist.erase(v);
  } else {
    spillWorklist.erase(v);
  }
  coalescedNodes.insert(v);
  alias[v] = u;
  for (auto& n : moveList[v]) {
    moveList[u].insert(n);
  }

  for (auto& t : Adjacent(v)) {
    AddEdge(t, u);
    DecrementDegree(t);
  }
  if (degree[u] >= K && freezeWorklist.find(u) != freezeWorklist.end()) {
    freezeWorklist.erase(u);
    spillWorklist.insert(u);
  }
}

void RegisterAllocator::Freeze() {
  assert(!freezeWorklist.empty());
  auto& u = *freezeWorklist.begin();
  freezeWorklist.erase(u);
  simplifyWorklist.insert(u);
  FreezeMoves(u);
}

void RegisterAllocator::FreezeMoves(OpPtr u) {
  for (auto& m : NodeMoves(u)) {
    if (activeMoves.find(m) != activeMoves.end()) {
      activeMoves.erase(m);
    } else {
      worklistMoves.erase(m);
    }
    frozenMoves.insert(m);
    auto& v = m->m_dest == u ? m->m_src : m->m_dest;
    if (NodeMoves(v).empty() && degree[v] < K) {
      freezeWorklist.erase(v);
      simplifyWorklist.insert(v);
    }
  }
}

void RegisterAllocator::SelectSpill() {
  // TODO(Huang): use a better heuristic algorithm
  // Note: avoid choosing nodes that are the tiny live ranges
  // resulting from the fetches of previously spilled registers

  // now we just select the first
  auto& m = *spillWorklist.begin();
  spillWorklist.erase(m);
  simplifyWorklist.insert(m);
  FreezeMoves(m);
}

void RegisterAllocator::AssignColors() {
  while (!selectStack.empty()) {
    OpPtr n = selectStack.back();
    selectStack.pop_back();
    std::unordered_set<int> okColors;
    for (int i = 0; i < K; i++) {
      okColors.insert(i);
    }
    for (auto& w : adjList[n]) {
      OpPtr a = GetAlias(w);
      if (coloredNodes.find(a) != coloredNodes.end()
          || precolored.find(a) != precolored.end()) {
        okColors.erase(color[a]);
      }
    }
    if (okColors.empty()) {
      spilledNodes.insert(n);
    } else {
      coloredNodes.insert(n);
      int c = *okColors.begin();
      color[n] = c;
    }
  }
  for (auto& n : coalescedNodes) {
    color[n] = color[GetAlias(n)];
  }
}

void RegisterAllocator::RewriteProgram() {
  // TODO(Huang):
  // Allocate memory locations for each v ∈ spilledNodes,
  // Create a new temporary vi for each definition and each use,
  // In the program (instructions), insert a store after each
  // definition of a vi, a fetch before each use of a vi.
  // Put all the vi into a set newTemps.

  std::cout << "Haven't allocate memory for spill nodes" << std::endl;
  assert(false);
}

bool RegisterAllocator::AllOK(OpPtr u, OpPtr v) {
  for (auto& t : Adjacent(v)) {
    if (!OK(t, u)) return false;
  }
  return true;
}

bool RegisterAllocator::ConservativeAdj(OpPtr u, OpPtr v) {
  std::unordered_set<OpPtr> nodes;
  for (auto& op : Adjacent(u)) {
    nodes.insert(op);
  }
  for (auto& op : Adjacent(v)) {
    nodes.insert(op);
  }
  return Conservative(nodes);
}