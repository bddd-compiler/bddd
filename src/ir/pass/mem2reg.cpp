#include "ir/pass/mem2reg.h"

#include <iostream>
#include <stack>
#include <unordered_set>

// forward graph
std::vector<std::vector<size_t>> f_graph;
// backward graph
std::vector<std::vector<size_t>> b_graph;
// auxiliary graph for calculating idom from sdom
std::vector<std::vector<size_t>> aux_graph;

std::vector<size_t> dfn, ord, fa, idom, sdom, ufs, mn;
size_t co;

size_t find(size_t u) {
  if (u == ufs[u]) return u;
  size_t ret = find(ufs[u]);
  if (dfn[sdom[mn[ufs[u]]]] < dfn[sdom[mn[u]]]) mn[u] = mn[ufs[u]];
  return ufs[u] = ret;
}

void tarjan(size_t u) {
  dfn[u] = ++co;
  ord[dfn[u]] = u;
  for (size_t v : f_graph[u]) {
    if (!dfn[v]) {
      fa[v] = u;
      tarjan(v);
    }
  }
}

std::vector<std::shared_ptr<BasicBlock>> bbs;

void RemoveUnusedBlocks(std::shared_ptr<Function> function) {
  bbs.clear();
  bbs.push_back(nullptr);
  size_t cnt = 0;
  for (auto &bb : function->m_bb_list) {
    bb->m_id = ++cnt;
    bbs.push_back(bb);
  }
  size_t n = function->m_bb_list.size();
  f_graph.clear();
  f_graph.resize(n + 5);
  dfn.clear();
  dfn.resize(n + 5);
  ord.clear();
  ord.resize(n + 5);
  fa.clear();
  fa.resize(n + 5);
  co = 0;

  for (auto &bb : function->m_bb_list) {
    // terminator instruction only have jump, branch, return
    size_t u = bb->m_id;
    for (const std::shared_ptr<BasicBlock> &v_block : bb->Successors()) {
      size_t v = v_block->m_id;
      f_graph[u].push_back(v);
    }
  }
  tarjan(1);  // start from 1

  for (auto it = function->m_bb_list.begin();
       it != function->m_bb_list.end();) {
    auto bb = *it;
    if (!dfn[bb->m_id]) {
      // not visited
      auto del = it;
      ++it;
      function->m_bb_list.erase(del);
    } else {
      ++it;
    }
  }
}

void ComputeDominanceRelationship(std::shared_ptr<Function> function) {
  RemoveUnusedBlocks(function);

  // allocate id for each basic block
  bbs.clear();
  bbs.push_back(nullptr);
  size_t cnt = 0;
  for (auto &bb : function->m_bb_list) {
    bb->m_id = ++cnt;  // 1, 2, 3, ...
    bb->m_dominators.clear();
    bb->m_dominated.clear();
    bb->m_idom = nullptr;
    bbs.push_back(bb);
  }
  size_t n = function->m_bb_list.size();
  f_graph.clear();
  f_graph.resize(n + 5);
  b_graph.clear();
  b_graph.resize(n + 5);
  aux_graph.clear();
  aux_graph.resize(n + 5);
  dfn.clear();
  dfn.resize(n + 5);
  ord.clear();
  ord.resize(n + 5);
  fa.clear();
  fa.resize(n + 5);
  idom.clear();
  idom.resize(n + 5);
  sdom.clear();
  sdom.resize(n + 5);
  ufs.clear();
  ufs.resize(n + 5);
  mn.clear();
  mn.resize(n + 5);
  co = 0;

  for (auto &bb : function->m_bb_list) {
    // terminator instruction only have jump, branch, return
    size_t u = bb->m_id;
    for (const std::shared_ptr<BasicBlock> &v_block : bb->Successors()) {
      size_t v = v_block->m_id;
      f_graph[u].push_back(v);
      b_graph[v].push_back(u);
    }
  }

  tarjan(1);  // start from 1
  for (int i = 1; i <= n; ++i) {
    sdom[i] = ufs[i] = mn[i] = i;
  }
  for (size_t i = co; i >= 2; --i) {
    size_t t = ord[i];
    for (size_t v : b_graph[t]) {
      if (!dfn[v]) continue;
      find(v);
      if (dfn[sdom[mn[v]]] < dfn[sdom[t]]) {
        sdom[t] = sdom[mn[v]];
      }
    }
    ufs[t] = fa[t];
    aux_graph[sdom[t]].push_back(t);
    t = fa[t];
    for (size_t v : aux_graph[t]) {
      find(v);
      if (t == sdom[mn[v]])
        idom[v] = t;
      else
        idom[v] = mn[v];
    }
    aux_graph[t].clear();
  }
  for (size_t i = 2; i <= co; ++i) {
    size_t t = ord[i];
    if (idom[t] ^ sdom[t]) {
      idom[t] = idom[idom[t]];
    }
  }

  for (size_t i = co; i >= 2; --i) {
    std::shared_ptr<BasicBlock> bb = bbs[ord[i]];
    bb->m_dominators.push_back(bb);
    assert(ord[i] != idom[ord[i]]);
    for (auto x : bbs[ord[i]]->m_dominators) {
      if (idom[ord[i]] != 0) {
        bbs[idom[ord[i]]]->m_dominators.push_back(x);
      }
    }
  }
  bbs[ord[1]]->m_dominators.push_back(bbs[ord[1]]);

  for (size_t i = 1; i <= co; ++i) {
    for (auto x : bbs[i]->m_dominators) {
      x->m_dominated.push_back(bbs[i]);
    }
  }

  for (size_t i = 1; i <= co; ++i) {
    if (idom[i]) {
      bbs[i]->m_idom = bbs[idom[i]];
    }
  }
}

// must be called after ComputeDominanceRelationship
void ComputeDominanceFrontier(std::shared_ptr<Function> function) {
  for (auto &a : function->m_bb_list) {
    for (auto &b : a->Successors()) {
      auto x = a;
      while (x->m_id == b->m_id
             || std::find_if(b->m_dominated.begin(), b->m_dominated.end(),
                             [=](const std::shared_ptr<BasicBlock> &rhs) {
                               return rhs.get() == x.get();
                             })
                    == b->m_dominated.end()) {
        if (x->m_id != b->m_id) {
          x->m_dominance_frontier.insert(b);
        }
        // df[x].insert(b);
        x = x->m_idom;
        if (x == nullptr) break;
      }
    }
  }
}

void updateReachingDefinition(std::shared_ptr<Value> v,
                              std::shared_ptr<Value> instr) {
  // auto v = load_instr->m_addr->m_value;
  auto r = v->m_reaching_def;
  while (!(r == nullptr || r->m_bb == nullptr
           || std::find_if(
                  r->m_bb->m_dominators.begin(), r->m_bb->m_dominators.end(),
                  [=](const auto &x) { return x.get() == instr->m_bb.get(); })
                  != r->m_bb->m_dominators.end())) {
    r = r->m_reaching_def;
  }
  v->m_reaching_def = r;
}

void Mem2Reg(std::shared_ptr<Function> function,
             std::shared_ptr<IRBuilder> builder) {
  size_t alloca_cnt = 0;
  std::vector<std::shared_ptr<AllocaInstruction>> allocas;
  for (auto &bb : function->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      instr->m_reaching_def = nullptr;
      if (auto alloca_instr
          = std::dynamic_pointer_cast<AllocaInstruction>(instr)) {
        if (alloca_instr->m_type.m_dimensions.empty()) {
          // single local variable (not an array)
          alloca_instr->m_alloca_id = ++alloca_cnt;
          // alloca_instr->m_bb = bb;
          allocas.push_back(alloca_instr);
        }
      }
    }
  }

  // std::vector<std::vector<std::shared_ptr<BasicBlock>>> defs(allocas.size());
  for (auto &bb : function->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      if (auto store_instr
          = std::dynamic_pointer_cast<StoreInstruction>(instr)) {
        if (auto alloca_instr = std::dynamic_pointer_cast<AllocaInstruction>(
                store_instr->m_addr->m_value)) {
          if (alloca_instr->m_alloca_id != 0) {
            alloca_instr->m_defs.push_back(bb);
          }
        }
      }
    }
  }

  // part1: insert phi functions
  ComputeDominanceRelationship(function);
  ComputeDominanceFrontier(function);

  std::unordered_map<std::shared_ptr<PhiInstruction>,
                     std::shared_ptr<AllocaInstruction>>
      phis;
  for (auto &alloca : allocas) {
    std::unordered_set<std::shared_ptr<BasicBlock>> f;
    std::stack<std::shared_ptr<BasicBlock>> w;
    for (auto &bb : alloca->m_defs) {
      w.push(bb);
    }
    while (!w.empty()) {
      std::shared_ptr<BasicBlock> x = w.top();  // x is basic block
      w.pop();
      for (auto &y : x->m_dominance_frontier) {  // y is basic block too
        if (std::find_if(f.begin(), f.end(),
                         [=](auto x) { return x.get() == y.get(); })
            == f.end()) {
          auto phi_instr
              = builder->CreatePhiInstruction(alloca->m_type.Dereference(), y);
          phis[phi_instr] = alloca;
          f.insert(y);
          if (std::find_if(alloca->m_defs.begin(), alloca->m_defs.end(),
                           [=](auto x) { return x.get() == y.get(); })
              == alloca->m_defs.end()) {
            w.push(y);
          }
        }
      }
    }
  }

  // part 2: rename

  for (const auto &bb : function->m_bb_list) {
    bb->m_visited = false;
  }
  std::stack<std::shared_ptr<BasicBlock>> stack;
  stack.push(*function->m_bb_list.begin());
  while (!stack.empty()) {
    std::shared_ptr<BasicBlock> bb = stack.top();
    stack.pop();
    if (bb->m_visited) continue;
    bb->m_visited = true;
    // std::cerr << "idx: " << bb->m_id << std::endl;
    // reassign loop_it before deletion
    for (auto loop_it = bb->m_instr_list.begin();
         loop_it != bb->m_instr_list.end();) {
      auto &instr = *loop_it;
      auto it = std::find_if(allocas.begin(), allocas.end(),
                             [=](auto x) { return x.get() == instr.get(); });
      if (it != allocas.end()) {
        ++loop_it;
        bb->RemoveInstruction(instr);
        // instr not in bb->m_instr_list now, so must continue immediately
        continue;
      }

      // for variables used by non-phi instructions
      if (auto load_instr = std::dynamic_pointer_cast<LoadInstruction>(instr)) {
        it = std::find_if(allocas.begin(), allocas.end(), [=](auto x) {
          return x.get() == load_instr->m_addr->m_value.get();
        });
        if (it != allocas.end()) {
          // load_instr->m_addr->m_value->m_reaching_def may not dominate the
          // current load instruction
          updateReachingDefinition(load_instr->m_addr->m_value, load_instr);

          load_instr->ReplaceUseBy(load_instr->m_addr->m_value->m_reaching_def);
          ++loop_it;
          bb->RemoveInstruction(instr);
          continue;  // similarly
        }
      }

      // for variables defined by i

      auto store_instr = std::dynamic_pointer_cast<StoreInstruction>(instr);
      if (store_instr != nullptr) {
        it = std::find_if(allocas.begin(), allocas.end(), [=](auto x) {
          return x.get() == store_instr->m_addr->m_value.get();
        });
        if (it != allocas.end()) {
          // simply update
          store_instr->m_val->m_value->m_reaching_def
              = store_instr->m_addr->m_value->m_reaching_def;
          store_instr->m_addr->m_value->m_reaching_def
              = store_instr->m_val->m_value;
          ++loop_it;
          bb->RemoveInstruction(instr);
          continue;
        }
      }

      auto phi_instr = std::dynamic_pointer_cast<PhiInstruction>(instr);
      if (phi_instr != nullptr) {
        for (auto &phi : phis) {
          if (phi.first.get() == phi_instr.get()) {
            // simply update
            auto &alloca = phi.second;
            phi_instr->m_reaching_def = alloca->m_reaching_def;
            alloca->m_reaching_def = phi_instr;
          }
        }
      }
      ++loop_it;
    }

    auto successors = bb->Successors();
    for (auto it = successors.rbegin(); it != successors.rend(); ++it) {
      stack.push(*it);
    }
    for (auto &x : successors) {
      for (auto &instr : x->m_instr_list) {
        if (auto phi_instr = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
          for (auto &phi : phis) {
            if (phi.first.get() == phi_instr.get()) {
              if (phi.second->m_reaching_def != nullptr) {
                std::shared_ptr<Value> r = phi.second->m_reaching_def;
                while (r->m_bb
                       && std::find_if(
                              bb->m_dominated.begin(), bb->m_dominated.end(),
                              [=](const auto &lhs) { return lhs == r->m_bb; })
                              == bb->m_dominated.end()) {
                  r = r->m_reaching_def;
                }
                phi.second->m_reaching_def = r;

                phi_instr->AddPhiOperand(bb, phi.second->m_reaching_def);
              }
            }
          }
        } else {
          break;  // not phi instructions after now
        }
      }
    }
  }

  // remove unused or trivial phis (replace use by its unique incoming value)
  // std::vector<
  //     std::pair<std::shared_ptr<PhiInstruction>,
  //     std::shared_ptr<BasicBlock>>> trivial_phis;
  for (auto &bb : function->m_bb_list) {
    for (auto it = bb->m_instr_list.begin(); it != bb->m_instr_list.end();) {
      auto instr = *it;
      if (auto phi_instr = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
        // std::cerr << "size: " << phi_instr->m_use_list.size()
        //           << ", # operands: " << phi_instr->m_contents.size()
        //           << std::endl;
        if (phi_instr->m_use_list.empty()) {
          ++it;
          bb->RemoveInstruction(phi_instr);
        } else {
          ++it;
        }
      } else {
        break;
      }
    }
  }

  // for (auto &[trivial_phi, bb] : trivial_phis) {
  //   bb->RemoveInstruction(trivial_phi);
  // }
}
