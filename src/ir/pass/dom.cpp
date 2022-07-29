#include "ir/ir-pass-manager.h"

// forward graph
std::vector<std::vector<size_t>> f_graph;
// backward graph
std::vector<std::vector<size_t>> b_graph;
// auxiliary graph for calculating idom from sdom
std::vector<std::vector<size_t>> aux_graph;
// dominance tree
std::vector<std::vector<size_t>> dom_tree;

std::vector<size_t> dfn, ord, fa, idom, sdom, ufs, mn;
size_t co;

std::vector<std::shared_ptr<BasicBlock>> bbs;

void dfs(std::shared_ptr<BasicBlock> bb, int dom_depth) {
  // std::cerr << "[debug] " << bb->m_id << std::endl;
  bb->m_dom_depth = dom_depth;
  bb->m_dominators.insert(bb);
  bb->m_dominated.insert(bb);
  for (auto i : dom_tree[bb->m_id]) {
    dfs(bbs[i], dom_depth + 1);

    for (auto &x : bbs[i]->m_dominators) {
      bb->m_dominators.insert(x);
      x->m_dominated.insert(bb);
    }
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

void ComputeDominanceRelationship(std::shared_ptr<Function> function) {
  RemoveUnvisitedBasicBlocks(function);

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
  f_graph.resize(n + 5);
  b_graph.clear();
  b_graph.resize(n + 5);
  aux_graph.clear();
  aux_graph.resize(n + 5);
  dom_tree.clear();
  dom_tree.resize(n + 5);
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
      v_block->m_predecessors.insert(bb);
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
  for (size_t i = 2; i <= co; ++i) {
    size_t t = ord[i];
    dom_tree[idom[t]].push_back(t);
    assert(idom[i] != 0);
    bbs[i]->m_idom = bbs[idom[i]];
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
