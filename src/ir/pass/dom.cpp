#include "ir/ir-pass-manager.h"

// forward graph
std::vector<std::vector<size_t>> f_graph;
// backward graph
std::vector<std::vector<size_t>> b_graph;
// auxiliary graph for calculating idom from sdom
std::vector<std::vector<size_t>> aux_graph;

std::vector<size_t> dfn, ord, fa, idom, sdom, ufs, mn;
size_t co;

void dfs(std::shared_ptr<BasicBlock> bb, int dom_depth) {
  bb->m_dom_depth = dom_depth;
  for (auto &dom : bb->m_dominators) {
    if (dom == bb) continue;
    dfs(dom, dom_depth + 1);
  }
}

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
    bb->m_predecessors.clear();
    bbs.push_back(bb);
  }
  size_t n = function->m_bb_list.size();
  f_graph.clear();
  f_graph.reserve(n + 5);
  dfn.clear();
  dfn.reserve(n + 5);
  ord.clear();
  ord.reserve(n + 5);
  fa.clear();
  fa.reserve(n + 5);
  co = 0;

  for (auto &bb : function->m_bb_list) {
    // terminator instruction only have jump, branch, return
    size_t u = bb->m_id;
    for (const std::shared_ptr<BasicBlock> &v_block : bb->Successors()) {
      size_t v = v_block->m_id;
      f_graph[u].push_back(v);
      v_block->m_predecessors.push_back(bb);
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
    bb->m_predecessors.clear();
    bbs.push_back(bb);
  }
  size_t n = function->m_bb_list.size();
  f_graph.clear();
  f_graph.reserve(n + 5);
  b_graph.clear();
  b_graph.reserve(n + 5);
  aux_graph.clear();
  aux_graph.reserve(n + 5);
  dfn.clear();
  dfn.reserve(n + 5);
  ord.clear();
  ord.reserve(n + 5);
  fa.clear();
  fa.reserve(n + 5);
  idom.clear();
  idom.reserve(n + 5);
  sdom.clear();
  sdom.reserve(n + 5);
  ufs.clear();
  ufs.reserve(n + 5);
  mn.clear();
  mn.reserve(n + 5);
  co = 0;

  for (auto &bb : function->m_bb_list) {
    // terminator instruction only have jump, branch, return
    size_t u = bb->m_id;
    for (const std::shared_ptr<BasicBlock> &v_block : bb->Successors()) {
      size_t v = v_block->m_id;
      f_graph[u].push_back(v);
      b_graph[v].push_back(u);
      v_block->m_predecessors.push_back(bb);
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

  dfs(function->m_bb_list.front(), 0);
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
