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
  }

  // print
  // std::cout << "precolored: ";
  // for (auto& o : precolored) std::cout << o->getName() << " ";
  // std::cout << std::endl;
  // std::cout << "initial: ";
  // for (auto& o : initial) std::cout << o->getName() << " ";
  // std::cout << std::endl;
}

void RegisterAllocator::AllocateCurFunc() {
  // debug("AllocateCurFunc");
  LivenessAnalysis();
  Build();
  MkWorklist();
  do {
    if (!simplifyWorklist.empty()) {
      // std::cout << "Simplify" << std::endl;
      Simplify();
    } else if (!worklistMoves.empty()) {
      // std::cout << "Coalesce" << std::endl;
      Coalesce();
    } else if (!freezeWorklist.empty()) {
      // std::cout << "Freeze" << std::endl;
      Freeze();
    } else if (!spillWorklist.empty()) {
      // std::cout << "SelectSpill" << std::endl;
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
  // debug("AddEdge");
  auto e = std::make_pair(u, v);
  if (adjSet.find(e) == adjSet.end() && u != v) {
    adjSet.insert(e);
    adjSet.insert(std::make_pair(v, u));
    if (u->m_op_type != OperandType::REG) {
      adjList[u].insert(v);
      degree[u]++;
    }
    if (v->m_op_type != OperandType::REG) {
      adjList[v].insert(u);
      degree[v]++;
    }
  }
}

void RegisterAllocator::Build() {
  // debug("Build");
  for (auto& b : m_cur_func->m_blocks) {
    std::unordered_set<OpPtr>& live = b->m_liveout;
    for (auto iter = b->m_insts.rbegin(); iter != b->m_insts.rend(); iter++) {
      std::shared_ptr<ASM_Instruction>& inst = *iter;
      if (auto I = std::dynamic_pointer_cast<MOVInst>(inst)) {
        if (I->m_type != MOVInst::RIType::IMM) {
          for (auto& use : I->m_use) {
            live.erase(use);
          }
          for (auto& n : I->m_def) {
            moveList[n].insert(I);
          }
          for (auto& n : I->m_use) {
            if (n->m_op_type == OperandType::IMM) {
              continue;
            }
            moveList[n].insert(I);
          }
          worklistMoves.insert(I);
        }
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
        if (use->m_op_type == OperandType::IMM) {
          continue;
        }
        live.insert(use);
      }
    }
  }

  // print
  // std::cout << "adjSet: ";
  // for (auto& o : adjSet) std::cout << "(" << o.first->getName() << "," <<
  // o.second->getName() << ") "; std::cout << std::endl;
}

std::unordered_set<OpPtr> RegisterAllocator::Adjacent(OpPtr n) {
  // debug("Adjacent");
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
  // debug("NodeMoves");
  if (moveList.find(n) == moveList.end()) {
    return {};
  }
  std::unordered_set<std::shared_ptr<MOVInst>> temp, ret;
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
  // debug("MkWorklist");
  for (auto& n : initial) {
    if (degree[n] >= K) {
      // std::cout << "spill: " << n->getName() << std::endl;
      spillWorklist.insert(n);
    } else if (MoveRelated(n)) {
      // std::cout << "freeze: " << n->getName() << std::endl;
      freezeWorklist.insert(n);
    } else {
      // std::cout << "simplify: " << n->getName() << std::endl;
      simplifyWorklist.insert(n);
    }
  }
  initial.clear();
}

void RegisterAllocator::Simplify() {
  // debug("Simplify");
  assert(!simplifyWorklist.empty());
  OpPtr n = *simplifyWorklist.begin();
  simplifyWorklist.erase(simplifyWorklist.begin());
  selectStack.push_back(n);
  for (auto& m : Adjacent(n)) {
    DecrementDegree(m);
  }
}

void RegisterAllocator::DecrementDegree(OpPtr m) {
  // debug("DecrementDegree");
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

void RegisterAllocator::EnableMoves(std::unordered_set<OpPtr>& nodes) {
  // debug("EnableMoves");
  for (auto& n : nodes)
    for (auto& m : NodeMoves(n))
      if (activeMoves.find(m) != activeMoves.end()) {
        activeMoves.erase(m);
        worklistMoves.insert(m);
      }
}

void RegisterAllocator::Coalesce() {
  // debug("Coalesce");
  assert(!worklistMoves.empty());
  auto m = *worklistMoves.begin();
  auto x = GetAlias(m->m_src);
  auto y = GetAlias(m->m_dest);
  OpPtr u, v;
  if (x->m_op_type == OperandType::REG) {
    u = y;
    v = x;
  } else {
    u = x;
    v = y;
  }
  worklistMoves.erase(worklistMoves.begin());
  if (u == v) {
    coalescedMoves.insert(m);
    AddWorkList(u);
  } else if (v->m_op_type == OperandType::REG
             || adjSet.find(std::make_pair(u, v)) != adjSet.end()) {
    constrainedMoves.insert(m);
    AddWorkList(u);
    AddWorkList(v);
  } else if (u->m_op_type == OperandType::REG && AllOK(u, v)
             || u->m_op_type != OperandType::REG && ConservativeAdj(u, v)) {
    coalescedMoves.insert(m);
    Combine(u, v);
    AddWorkList(u);
  } else {
    activeMoves.insert(m);
  }
}

void RegisterAllocator::AddWorkList(OpPtr u) {
  // debug("AddWorkList");
  if (u->m_op_type != OperandType::REG && !MoveRelated(u) && degree[u] < K) {
    freezeWorklist.erase(u);
    simplifyWorklist.insert(u);
  }
}

bool RegisterAllocator::OK(OpPtr t, OpPtr r) {
  return degree[t] < K || t->m_op_type == OperandType::REG
         || adjSet.find(std::make_pair(t, r)) != adjSet.end();
}

bool RegisterAllocator::Conservative(std::unordered_set<OpPtr>& nodes) {
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
  // debug("Combine");
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
  // debug("Freeze");
  assert(!freezeWorklist.empty());
  auto u = *freezeWorklist.begin();
  freezeWorklist.erase(freezeWorklist.begin());
  simplifyWorklist.insert(u);
  FreezeMoves(u);
}

void RegisterAllocator::FreezeMoves(OpPtr u) {
  // debug("FreezeMoves");
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
  // debug("SelectSpill");
  // TODO(Huang): use a better heuristic algorithm
  // Note: avoid choosing nodes that are the tiny live ranges
  // resulting from the fetches of previously spilled registers

  // now we just select the first
  
  spillWorklist.erase(m);
  simplifyWorklist.insert(m);
  FreezeMoves(m);
}

void RegisterAllocator::AssignColors() {
  // debug("AssignColors");
  while (!selectStack.empty()) {
    OpPtr n = selectStack.back();
    selectStack.pop_back();
    std::set<int> okColors;
    for (int i = 0; i < K; i++) {
      okColors.insert(i);
    }
    for (auto& w : adjList[n]) {
      OpPtr a = GetAlias(w);
      if (coloredNodes.find(a) != coloredNodes.end()
          || a->m_op_type == OperandType::REG) {
        okColors.erase(color[GetAlias(w)]);
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
  // debug("RewriteProgram");
  // TODO(Huang):
  // Allocate memory locations for each v ∈ spilledNodes,
  // Create a new temporary vi for each definition and each use,
  // In the program (instructions), insert a store after each
  // definition of a vi, a fetch before each use of a vi.
  // Put all the vi into a set newTemps.

  // std::cout << "Haven't allocate memory for spill nodes" << std::endl;
  // assert(false);
  std::unordered_set<OpPtr> newTemps;
  for (auto& v : spilledNodes) {
    int sp_offs = m_cur_func->getStackSize();
    m_cur_func->allocateStack(4);
    for (auto& b : m_cur_func->m_blocks) {
      for (auto iter = b->m_insts.begin(); iter != b->m_insts.end(); iter++) {
        auto& i = *iter;
        if (i->m_use.find(v) != i->m_use.end()) {
          // replace use

          OpPtr newOp = std::make_shared<Operand>(OperandType::VREG);
          i->replaceUse(newOp, v);
          i->m_use.erase(v);
          i->addUse(newOp);
          // insert a load instruction before use of newOp
          OpPtr offs;
          std::shared_ptr<MOVInst> mov = nullptr;
          if (0 <= sp_offs && sp_offs < 4096) {
            offs = std::make_shared<Operand>(sp_offs);
          } else {
            offs = std::make_shared<Operand>(OperandType::VREG);
            mov = std::make_shared<MOVInst>(offs, sp_offs);
            newTemps.insert(offs);
          }
          auto ldr = std::make_shared<LDRInst>(
              newOp, Operand::getRReg(RReg::SP), offs);
          if (mov) {
            b->insertSpillLDR(iter, ldr, mov);
          } else {
            b->insertSpillLDR(iter, ldr);
          }
          newTemps.insert(newOp);
        }
        if (i->m_def.find(v) != i->m_def.end()) {
          // replace def

          OpPtr newOp = std::make_shared<Operand>(OperandType::VREG);
          i->replaceDef(newOp, v);
          i->m_def.erase(v);
          i->addDef(newOp);
          // insert a store instruction after defination of newOp
          OpPtr offs;
          std::shared_ptr<MOVInst> mov = nullptr;
          if (0 <= sp_offs && sp_offs < 4096) {
            offs = std::make_shared<Operand>(sp_offs);
          } else {
            offs = std::make_shared<Operand>(OperandType::VREG);
            mov = std::make_shared<MOVInst>(offs, sp_offs);
            newTemps.insert(offs);
          }
          auto str = std::make_shared<STRInst>(
              newOp, Operand::getRReg(RReg::SP), offs);
          if (mov) {
            b->insertSpillSTR(iter, str, mov);
          } else {
            b->insertSpillSTR(iter, str);
          };
          newTemps.insert(newOp);
          auto next = std::next(iter);
          while (next != b->m_insts.end()
                 && (*next)->m_def.find(v) == (*next)->m_def.end()) {
            if ((*next)->m_use.find(v) != (*next)->m_use.end()) {
              (*next)->replaceUse(newOp, v);
              (*next)->m_use.erase(v);
              (*next)->addUse(newOp);
              next++;
              iter++;
            } else {
              break;
            }
          }
        }
      }
    }
  }

  spilledNodes.clear();
  initial.clear();
  for (auto& n : coloredNodes) initial.insert(n);
  for (auto& n : coalescedNodes) initial.insert(n);
  for (auto& n : newTemps) initial.insert(n);
  coloredNodes.clear();
  coalescedNodes.clear();
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

void RegisterAllocator::debug(std::string func) {
  std::cout << func << std::endl;
  std::cout << "precolored: ";
  for (auto& n : precolored) std::cout << n->getName() << ", ";
  std::cout << "\ninitial: ";
  for (auto& n : initial) std::cout << n->getName() << ", ";
  std::cout << "\nsimplifyWorklist: ";
  for (auto& n : simplifyWorklist) std::cout << n->getName() << ", ";
  std::cout << "\nfreezeWorklist: ";
  for (auto& n : freezeWorklist) std::cout << n->getName() << ", ";
  std::cout << "\nspillWorklist: ";
  for (auto& n : spillWorklist) std::cout << n->getName() << ", ";
  std::cout << "\nspilledNodes: ";
  for (auto& n : spilledNodes) std::cout << n->getName() << ", ";
  std::cout << "\ncoalescedNodes: ";
  for (auto& n : coalescedNodes) std::cout << n->getName() << ", ";
  std::cout << "\noloredNodes: ";
  for (auto& n : coloredNodes) std::cout << n->getName() << ", ";
  std::cout << "\nselectStack: ";
  for (auto& n : selectStack) std::cout << n->getName() << ", ";
  std::cout << std::endl << std::endl;
}