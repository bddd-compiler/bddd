//
// Created by garen on 7/26/22.
//

#include "ir/ir-pass-manager.h"

void dfs(std::shared_ptr<BasicBlock> bb, int depth,
         std::shared_ptr<Function> func) {
  if (bb->m_visited) return;
  bb->m_visited = true;
  bb->m_dfs_depth = depth;
  for (auto s : bb->Successors()) {
    if (!s->m_visited) {
      dfs(s, depth + 1, func);
    }
  }
  func->m_rpo_bb_list.push_front(bb);  // just push_back -> push_front
}

void RPOPass(std::unique_ptr<Module>& module) {
  for (auto func : module->m_function_list) {
    func->m_rpo_bb_list.clear();
    for (auto bb : func->m_bb_list) {
      bb->m_visited = false;
    }
    dfs(func->m_bb_list.front(), 1, func);
  }
}