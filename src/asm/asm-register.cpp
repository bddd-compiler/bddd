#include "asm/asm-register.h"

#include "asm/asm-fixed.h"

// #define REG_ALLOC_DEBUG

RegisterAllocator::RegisterAllocator(std::shared_ptr<ASM_Module> module,
                                     RegType type) {
  m_module = module;
  m_reg_type = type;
}

void RegisterAllocator::Allocate() {
  assert(m_module);
  initialColors();
  for (auto& func : m_module->m_funcs) {
    setCurFunction(func);
    init();
    getInitial();
    AllocateCurFunc();
    if (m_reg_type == RegType::R) {
      for (auto& node : coloredNodes) {
        RReg rreg = color_r[node];
        if (node->m_op_type == OperandType::VREG) {
          node->m_op_type = OperandType::REG;
          node->m_is_float = false;
          node->m_rreg = rreg;
        }
        if (4 <= (int)rreg && (int)rreg <= 11) {
          storeRegisters(func, Operand::getRReg(rreg));
        }
      }
    } else {
      for (auto& node : coloredNodes) {
        SReg sreg = color_s[node];
        if (node->m_op_type == OperandType::VREG) {
          node->m_op_type = OperandType::REG;
          node->m_is_float = true;
          node->m_sreg = sreg;
        }
      }
    }
    initial.clear();
  }
}

void RegisterAllocator::setCurFunction(std::shared_ptr<ASM_Function> func) {
  m_cur_func = func;
}

void RegisterAllocator::init() {
  precolored.clear();
  initial.clear();
  simplifyWorklist.clear();
  freezeWorklist.clear();
  spillWorklist.clear();
  spilledNodes.clear();
  coalescedNodes.clear();
  coloredNodes.clear();
  selectStack.clear();
  coalescedRecord.clear();
  coalescedMoves.clear();
  constrainedMoves.clear();
  frozenMoves.clear();
  worklistMoves.clear();
  activeMoves.clear();
  adjSet.clear();
  adjList.clear();
  degree.clear();
  moveList.clear();
  alias.clear();
  color_r.clear();
  color_s.clear();
  isSelectSpill = false;
}

void RegisterAllocator::initialColors() {
  if (m_reg_type == RegType::R) {
    rreg_avaliable = {RReg::R0,  RReg::R1,  RReg::R2,  RReg::R3, RReg::R4,
                      RReg::R5,  RReg::R6,  RReg::R7,  RReg::R8, RReg::R9,
                      RReg::R10, RReg::R11, RReg::R12, RReg::LR};
    K = rreg_avaliable.size();
  } else {
    for (int i = 1; i < 32; i++) {
      sreg_avaliable.insert((SReg)i);
    }
    K = sreg_avaliable.size();
  }
}

void RegisterAllocator::getInitial() {
  bool flag = m_reg_type == RegType::R;
  for (auto& b : m_cur_func->m_blocks) {
    for (auto& i : b->m_insts) {
      if (i->m_is_deleted) continue;
      std::unordered_set<OpPtr> defs, uses;
      if (flag) {
        defs = i->m_def;
        uses = i->m_use;
      } else {
        defs = i->m_f_def;
        uses = i->m_f_use;
      }
      for (auto& op : defs) {
        if (op->m_op_type == OperandType::VREG) initial.insert(op);
      }
      for (auto& op : uses) {
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

void RegisterAllocator::updateDepth(std::shared_ptr<ASM_BasicBlock> block,
                                    OpPtr node) {
  assert(block);
  if (m_depth_map[node] < block->m_loop_depth) {
    m_depth_map[node] = block->m_loop_depth;
  }
}

void RegisterAllocator::AllocateCurFunc() {
#ifdef REG_ALLOC_DEBUG
  debug("AllocateCurFunc");
#endif
  LivenessAnalysis();
  Build();
  MkWorklist();
  isSelectSpill = false;
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
#ifdef REG_ALLOC_DEBUG
  debug("AddEdge");
#endif
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
#ifdef REG_ALLOC_DEBUG
  debug("Build");
#endif
  for (auto& b : m_cur_func->m_blocks) {
    std::unordered_set<OpPtr> live = b->m_liveout;
    std::unordered_map<OpPtr, int> lifespan_map;
    int cnt = 0;
    for (auto iter = b->m_insts.rbegin(); iter != b->m_insts.rend(); iter++) {
      std::shared_ptr<ASM_Instruction>& inst = *iter;
      if (inst->m_is_deleted) continue;
      cnt++;
      std::unordered_set<OpPtr> defs, uses;
      if (m_reg_type == RegType::R) {
        defs = inst->m_def;
        uses = inst->m_use;
      } else {
        defs = inst->m_f_def;
        uses = inst->m_f_use;
      }
      if (auto I = std::dynamic_pointer_cast<MOVInst>(inst)) {
        if (I->m_type != MOVType::IMM && I->m_dest->getRegType() == m_reg_type
            && I->m_src->getRegType() == m_reg_type) {
          for (auto& use : uses) {
            live.erase(use);
          }
          for (auto& n : defs) {
            moveList[n].insert(I);
            updateDepth(I->m_block, n);
          }
          for (auto& n : uses) {
            if (n->m_op_type == OperandType::IMM) {
              continue;
            }
            moveList[n].insert(I);
            updateDepth(I->m_block, n);
          }
          worklistMoves.insert(I);
        }
      }
      for (auto& def : defs) {
        live.insert(def);
      }
      for (auto& d : defs) {
        for (auto& l : live) {
          AddEdge(l, d);
          updateDepth(inst->m_block, l);
          updateDepth(inst->m_block, d);
        }
      }
      // live := use(I) ∪ (live\def(I))
      for (auto& def : defs) {
        live.erase(def);
        if (lifespan_map.find(def) != lifespan_map.end()) {
          def->lifespan = cnt - lifespan_map[def];
        }
      }
      for (auto& use : uses) {
        if (use->m_op_type == OperandType::IMM) {
          continue;
        }
        use->lifespan = b->m_insts.size() - cnt;
        auto ret = live.insert(use);
        if (ret.second) {
          lifespan_map[use] = cnt;
        }
      }
    }
  }

  // print
  // std::cout << "adjSet: ";
  // for (auto& o : adjSet) std::cout << "(" << o.first->getName() << "," <<
  // o.second->getName() << ") "; std::cout << std::endl;
}

std::unordered_set<OpPtr> RegisterAllocator::Adjacent(OpPtr n) {
#ifdef REG_ALLOC_DEBUG
  debug("Adjacent");
#endif
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
#ifdef REG_ALLOC_DEBUG
  debug("MkWorklist");
#endif
  for (auto& n : initial) {
    if (degree[n] >= K) {
      spillWorklist.insert(n);
    } else if (MoveRelated(n)) {
      freezeWorklist.insert(n);
    } else {
      simplifyWorklist.insert(n);
    }
  }
  initial.clear();
}

void RegisterAllocator::Simplify() {
#ifdef REG_ALLOC_DEBUG
  debug("Simplify");
#endif
  assert(!simplifyWorklist.empty());
  OpPtr n = *simplifyWorklist.begin();
  simplifyWorklist.erase(simplifyWorklist.begin());
  selectStack.push_back(n);
  for (auto& m : Adjacent(n)) {
    DecrementDegree(m);
  }
}

void RegisterAllocator::DecrementDegree(OpPtr m) {
#ifdef REG_ALLOC_DEBUG
  debug("DecrementDegree");
#endif
  int d = degree[m];
  degree[m] = d - 1;
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
#ifdef REG_ALLOC_DEBUG
  debug("EnableMoves");
#endif
  for (auto& n : nodes)
    for (auto& m : NodeMoves(n))
      if (activeMoves.find(m) != activeMoves.end()) {
        activeMoves.erase(m);
        worklistMoves.insert(m);
      }
}

void RegisterAllocator::Coalesce() {
#ifdef REG_ALLOC_DEBUG
  debug("Coalesce");
#endif
  assert(!worklistMoves.empty());
  auto m = *worklistMoves.begin();
  auto x = GetAlias(m->m_src);
  auto y = GetAlias(m->m_dest);
  if (x->m_op_type == OperandType::REG) {
    std::swap(x, y);
  }
  auto& u = x;
  auto& v = y;
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
#ifdef REG_ALLOC_DEBUG
  debug("AddWorkList");
#endif
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
#ifdef REG_ALLOC_DEBUG
  debug("Combine");
#endif
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
#ifdef REG_ALLOC_DEBUG
  debug("Freeze");
#endif
  assert(!freezeWorklist.empty());
  auto u = *freezeWorklist.begin();
  freezeWorklist.erase(freezeWorklist.begin());
  simplifyWorklist.insert(u);
  FreezeMoves(u);
}

void RegisterAllocator::FreezeMoves(OpPtr u) {
#ifdef REG_ALLOC_DEBUG
  debug("FreezeMoves");
#endif
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
#ifdef REG_ALLOC_DEBUG
  debug("SelectSpill");
#endif
  // TODO(Huang): use a better heuristic algorithm
  // Note: avoid choosing nodes that are the tiny live ranges
  // resulting from the fetches of previously spilled registers

  // if (!isSelectSpill) {
  //   coalescedRecord.insert(coalescedNodes.begin(), coalescedNodes.end());
  //   isSelectSpill = true;
  // }
  OpPtr m = *std::max_element(
      spillWorklist.cbegin(), spillWorklist.cend(), [this](OpPtr a, OpPtr b) {
        // return a->lifespan < b->lifespan;
        if (a->lifespan && b->lifespan) {
          // still compare degree if lifespan is equal!
          if (a->lifespan != b->lifespan) return a->lifespan < b->lifespan;
        } else if (a->lifespan)
          return true;
        else if (b->lifespan)
          return false;
        assert(m_depth_map.find(a) != m_depth_map.cend()
               && m_depth_map.find(b) != m_depth_map.cend());
        return degree[a] / pow(2, m_depth_map[a])
               < degree[b] / pow(2, m_depth_map[b]);
      });

  spillWorklist.erase(m);
  simplifyWorklist.insert(m);
  FreezeMoves(m);
}

void RegisterAllocator::AssignColors() {
#ifdef REG_ALLOC_DEBUG
  debug("AssignColors");
#endif
  if (m_reg_type == RegType::R)
    AssignColorsR();
  else if (m_reg_type == RegType::S)
    AssignColorsS();
}

void RegisterAllocator::AssignColorsR() {
  while (!selectStack.empty()) {
    OpPtr n = selectStack.back();
    selectStack.pop_back();
    std::set<RReg> okColors;
    for (auto r : rreg_avaliable) {
      okColors.insert(r);
    }
    for (auto& w : adjList[n]) {
      OpPtr a = GetAlias(w);
      if (a->m_op_type == OperandType::REG) {
        okColors.erase(a->m_rreg);
      } else if (coloredNodes.find(a) != coloredNodes.end()) {
        okColors.erase(color_r[GetAlias(w)]);
      }
    }
    if (okColors.empty()) {
      spilledNodes.insert(n);
    } else {
      coloredNodes.insert(n);
      RReg rreg = *okColors.begin();
      color_r[n] = rreg;
    }
  }
  for (auto& n : coalescedNodes) {
    color_r[n] = color_r[GetAlias(n)];
    coloredNodes.insert(n);
  }
  coalescedNodes.clear();
}

void RegisterAllocator::AssignColorsS() {
  while (!selectStack.empty()) {
    OpPtr n = selectStack.back();
    selectStack.pop_back();
    std::set<SReg> okColors;
    for (auto s : sreg_avaliable) {
      okColors.insert(s);
    }
    for (auto& w : adjList[n]) {
      OpPtr a = GetAlias(w);
      if (a->m_op_type == OperandType::REG) {
        okColors.erase(a->m_sreg);
      } else if (coloredNodes.find(a) != coloredNodes.end()) {
        okColors.erase(color_s[GetAlias(w)]);
      }
    }
    if (okColors.empty()) {
      spilledNodes.insert(n);
    } else {
      coloredNodes.insert(n);
      SReg sreg = *okColors.begin();
      color_s[n] = sreg;
    }
  }
  for (auto& n : coalescedNodes) {
    color_s[n] = color_s[GetAlias(n)];
    coloredNodes.insert(n);
  }
  coalescedNodes.clear();
}

void RegisterAllocator::RewriteProgram() {
#ifdef REG_ALLOC_DEBUG
  debug("RewriteProgram");
#endif
  // std::cout << "rewrite program" << std::endl;
  // for (auto& n : spilledNodes) std::cout << n->getName() << ", ";
  // std::cout << std::endl;
  // std::ofstream ofs("temp.s");
  // m_module->exportASM(ofs);
  // ofs.close();
  // std::cin.get();

  // Allocate memory locations for each v ∈ spilledNodes,
  // Create a new temporary vi for each definition and each use,
  // In the program (instructions), insert a store after each
  // definition of a vi, a fetch before each use of a vi.
  // Put all the vi into a set newTemps.

  std::unordered_set<OpPtr> newTemps;
  for (auto& v : spilledNodes) {
    if (v->lifespan <= 1 && !v->rejected) {
      v->rejected = true;
      newTemps.insert(v);
      continue;
    }

    int sp_offs = m_cur_func->getStackSize();
    m_cur_func->allocateStack(4);
    for (auto& b : m_cur_func->m_blocks) {
      for (auto iter = b->m_insts.begin(); iter != b->m_insts.end(); iter++) {
        auto& i = *iter;
        if (i->m_is_deleted) {
          continue;
        }
        int fixed_offs = sp_offs + i->m_params_offset;
        std::unordered_set<OpPtr> defs, uses;
        if (m_reg_type == RegType::R) {
          defs = i->m_def;
          uses = i->m_use;
        } else {
          defs = i->m_f_def;
          uses = i->m_f_use;
        }
        if (uses.find(v) != uses.end()) {
          OpPtr newOp;
          if (i->m_is_mov) {
            newOp = std::dynamic_pointer_cast<MOVInst>(i)->m_dest;
            i->m_is_deleted = true;
          } else {
            // replace use
            newOp = std::make_shared<Operand>(OperandType::VREG, v->m_is_float);
            i->replaceUse(newOp, v);
          }

          // insert a load instruction before use of newOp
          OpPtr offs;
          std::shared_ptr<MOVInst> mov = nullptr;
          if (Operand::addrOffsCheck(fixed_offs, newOp->m_is_float)) {
            offs = std::make_shared<Operand>(fixed_offs);
          } else {
            offs = std::make_shared<Operand>(OperandType::VREG);
            mov = std::make_shared<MOVInst>(offs, fixed_offs);
            mov->m_params_offset = i->m_params_offset;
            newTemps.insert(offs);
            updateDepth(b, offs);
          }
          auto ldr = std::make_shared<LDRInst>(
              newOp, Operand::getRReg(RReg::SP), offs);
          ldr->m_params_offset = i->m_params_offset;
          if (mov) {
            b->insertSpillLDR(iter, ldr, mov);
          } else {
            b->insertSpillLDR(iter, ldr);
          }
          if (newOp->m_op_type == OperandType::VREG) {
            newTemps.insert(newOp);
            updateDepth(b, newOp);
          }
        }
        if (defs.find(v) != defs.end()) {
          OpPtr newOp;
          auto next = std::next(iter);
          if (i->m_is_mov) {
            newOp = std::dynamic_pointer_cast<MOVInst>(i)->m_src;
            i->m_is_deleted = true;
          } else {
            // replace def
            newOp = std::make_shared<Operand>(OperandType::VREG, v->m_is_float);
            i->replaceDef(newOp, v);
          }

          // insert a store instruction after defination of newOp
          OpPtr offs;
          std::shared_ptr<MOVInst> mov = nullptr;
          if (Operand::addrOffsCheck(fixed_offs, newOp->m_is_float)) {
            offs = std::make_shared<Operand>(fixed_offs);
          } else {
            offs = std::make_shared<Operand>(OperandType::VREG);
            mov = std::make_shared<MOVInst>(offs, fixed_offs);
            mov->m_params_offset = i->m_params_offset;
            newTemps.insert(offs);
            updateDepth(b, offs);
          }
          auto str = std::make_shared<STRInst>(
              newOp, Operand::getRReg(RReg::SP), offs);
          str->m_params_offset = i->m_params_offset;
          if (mov) {
            b->insertSpillSTR(iter, str, mov);
          } else {
            b->insertSpillSTR(iter, str);
          }
          if (newOp->m_op_type == OperandType::VREG) {
            newTemps.insert(newOp);
            updateDepth(b, newOp);
          }
          while (next != b->m_insts.end()) {
            auto inst = *next;
            if (!inst->m_is_deleted) {
              if (m_reg_type == RegType::R) {
                if (inst->m_def.find(v) == inst->m_def.end()
                    && inst->m_use.find(v) != inst->m_use.end()) {
                  inst->replaceUse(newOp, v);
                } else
                  break;
              } else {
                if (inst->m_f_def.find(v) == inst->m_f_def.end()
                    && inst->m_f_use.find(v) != inst->m_f_use.end()) {
                  inst->replaceUse(newOp, v);
                } else
                  break;
              }
            }
            next++;
            iter++;
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
  // for (auto& n : coalescedRecord) initial.erase(n);
  coloredNodes.clear();
  coalescedNodes.clear();
  // std::swap(coloredNodes, coalescedRecord);
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
  std::cout << "\ncoloredNodes: ";
  for (auto& n : coloredNodes) std::cout << n->getName() << ", ";
  std::cout << "\nselectStack: ";
  for (auto& n : selectStack) std::cout << n->getName() << ", ";
  std::cout << std::endl << std::endl;
}