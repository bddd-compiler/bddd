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

void ComputeDominanceRelationship(std::shared_ptr<Function> function) {
  // allocate id for each basic block

  bbs.push_back(nullptr);
  size_t cnt = 0;
  for (auto &bb : function->m_bb_list) {
    bb->m_id = ++cnt;  // 1, 2, 3, ...
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
    auto bb = bbs[ord[i]];
    bb->m_dominators.push_back(bb);
    for (const auto &x : bbs[ord[i]]->m_dominators) {
      bbs[idom[ord[i]]]->m_dominators.push_back(x);
    }
  }
  bbs[ord[1]]->m_dominators.push_back(bbs[ord[1]]);
}

// must be called after ComputeDominanceRelationship
void ComputeDominanceFrontier(std::shared_ptr<Function> function) {
  for (auto &a : function->m_bb_list) {
    for (auto &b : a->Successors()) {
      auto x = a;
      if (x == b
          || std::find_if(b->m_dominators.begin(), b->m_dominators.end(),
                          [=](const std::shared_ptr<BasicBlock> &rhs) {
                            return rhs.get() == x.get();
                          })
                 == b->m_dominators.end()) {
        x->m_dominance_frontier.insert(b);
        // df[x].insert(b);
        x->m_idom = x;
      }
    }
  }
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

  std::unordered_set<std::shared_ptr<BasicBlock>> f;
  std::stack<std::shared_ptr<BasicBlock>> w;
  std::unordered_map<std::shared_ptr<PhiInstruction>, std::shared_ptr<Value>>
      phis;
  for (auto &alloca : allocas) {
    for (auto &bb : alloca->m_defs) {
      w.push(bb);
    }
    while (!w.empty()) {
      auto x = w.top();  // x is basic block
      w.pop();
      for (auto &y : x->m_dominance_frontier) {  // y is basic block too
        if (std::find_if(f.begin(), f.end(),
                         [=](auto x) { return x.get() == y.get(); })
            == f.end()) {
          auto phi_instr = builder->CreatePhiInstruction(y);
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

  for (int i = 1; i <= co; ++i) {
    std::shared_ptr<BasicBlock> &bb = bbs[ord[i]];
    // reassign _it before deletion
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

      auto load_instr = std::dynamic_pointer_cast<LoadInstruction>(instr);
      if (load_instr != nullptr) {
        it = std::find_if(allocas.begin(), allocas.end(), [=](auto x) {
          return x.get() == load_instr->m_addr->m_value.get();
        });
        if (it != allocas.end()) {
          load_instr->ReplaceUseBy(load_instr->m_addr->m_value->m_reaching_def);
          ++loop_it;
          bb->RemoveInstruction(instr);
          continue;  // similarly
        }
      }

      auto store_instr = std::dynamic_pointer_cast<StoreInstruction>(instr);
      if (store_instr != nullptr) {
        it = std::find_if(allocas.begin(), allocas.end(), [=](auto x) {
          return x.get() == store_instr->m_addr->m_value.get();
        });
        if (it != allocas.end()) {
          store_instr->m_addr->m_value->m_reaching_def
              = store_instr->m_val->m_value;
          ++loop_it;
          bb->RemoveInstruction(instr);
          continue;
        }
      }

      auto phi_instr = std::dynamic_pointer_cast<PhiInstruction>(instr);
      if (phi_instr != nullptr) {
        auto it2 = std::find_if(phis.begin(), phis.end(), [=](auto x) {
          return x.first.get() == phi_instr.get();
        });

        if (it2 != phis.end()) {
          auto temp = it2->second;
          temp->m_reaching_def = phi_instr;
        }
      }
      ++loop_it;
    }

    for (auto &x : bb->Successors()) {
      for (auto &instr : x->m_instr_list) {
        if (auto phi_instr = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
          auto phi_it = std::find_if(phis.begin(), phis.end(), [=](auto lhs) {
            return lhs.first.get() == phi_instr.get();
          });
          if (phi_it != phis.end()) {
            // bb -> x
            // phi_it->second is its corresponding alloca
            phi_instr->m_contents[bb].UseValue(phi_it->second->m_reaching_def);
          }
        } else {
          break;  // not phi instructions after now
        }
      }
    }
  }
  std::cerr << "123" << std::endl;

  // for (int i = 1; i <= co; ++i) {
  //   auto &bb = bbs[ord[i]];
  //   for (auto &instr : bb->m_instr_list) {
  //     if (auto alloca_instr
  //         = std::dynamic_pointer_cast<AllocaInstruction>(instr)) {
  //       if (alloca_instr->m_alloca_id != 0) {
  //         bb->RemoveInstruction(instr);
  //       }
  //     } else if (auto load_instr
  //                = std::dynamic_pointer_cast<LoadInstruction>(instr)) {
  //       if (auto alloca_instr = std::dynamic_pointer_cast<AllocaInstruction>(
  //               load_instr->m_addr->m_value)) {
  //         if (alloca_instr->m_alloca_id != 0) {
  //           load_instr->ReplaceUseBy(load_instr->m_reaching_def);
  //           bb->RemoveInstruction(instr);
  //         }
  //       }
  //     } else if (auto store_instr
  //                = std::dynamic_pointer_cast<StoreInstruction>(instr)) {
  //       if (auto alloca_instr = std::dynamic_pointer_cast<AllocaInstruction>(
  //               load_instr->m_addr->m_value)) {
  //         if (alloca_instr->m_alloca_id != 0) {
  //           store_instr->m_reaching_def = store_instr->m_val->m_value;
  //           bb->RemoveInstruction(instr);
  //         }
  //       }
  //     } else if (auto phi_instr
  //                = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
  //       if (auto it2 = std::find_if(
  //               phis.begin(), phis.end(),
  //               [=](auto x) { return x.first.get() == phi_instr.get(); });
  //           it2 != phis.end()) {
  //         auto temp = (*it2).second;
  //         temp->m_reaching_def = phi_instr;
  //       }
  //     }
  //   }
  //
  //   for (auto &x : bb->Successors()) {
  //     for (auto &instr : x->m_instr_list) {
  //       if (auto phi_instr =
  //       std::dynamic_pointer_cast<PhiInstruction>(instr)) {
  //         if (auto it2 = std::find_if(
  //                 phis.begin(), phis.end(),
  //                 [=](auto x) { return x.first.get() == phi_instr.get(); });
  //             it2 != phis.end()) {
  //           auto it3 = std::find_if(
  //               phi_instr->m_contents.begin(), phi_instr->m_contents.end(),
  //               [=](const auto &x) { return x.first.get() == bb.get(); });
  //           it3->second.UseValue(it2->second->m_reaching_def);
  //         }
  //       } else {
  //         break;
  //       }
  //     }
  //   }
  // }
}
