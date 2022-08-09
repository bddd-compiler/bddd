//
// Created by garen on 7/27/22.
//

#include <stack>

#include "ir/ir-pass-manager.h"
#include "ir/loop.h"

std::vector<std::pair<std::shared_ptr<BasicBlock>, std::shared_ptr<BasicBlock>>>
    g_back_edges;

void GetBackEdges(std::shared_ptr<Function> func) {
  g_back_edges.clear();
  for (auto &bb : func->m_bb_list) {
    for (auto &s : bb->Successors()) {
      // bb -> s
      // tail is dominated by head
      if (s->Dominate(bb)) {
        g_back_edges.emplace_back(bb, s);
      }
    }
  }
}

void DetectNaturalLoops(std::shared_ptr<Function> func) {
  // reducible CFG: one back edge corresponds to a natural loop
  for (auto [tail, head] : g_back_edges) {
    // std::cerr << "back edge: " << tail->m_id << "->" << head->m_id <<
    // std::endl;
    auto loop = std::make_shared<Loop>(head);
    auto preds = head->Predecessors();
    int temp = 0;
    for (auto &pred : preds) {
      if (pred->m_id < head->m_id) {
        loop->m_preheader = pred;
        ++temp;
      } else {
        loop->m_ends.push_back(pred);
        --temp;
      }
    }
    assert(temp <= 0);

    // populate nodes using predecessors
    std::stack<std::shared_ptr<BasicBlock>> stack;
    stack.push(tail);
    while (!stack.empty()) {
      std::shared_ptr<BasicBlock> x = stack.top();
      stack.pop();
      loop->m_bbs.insert(x);
      // std::cerr << x->m_id << " ";
      if (x == head) continue;
      for (auto &p : x->Predecessors()) {
        if (std::find(loop->m_bbs.begin(), loop->m_bbs.end(), p)
            == loop->m_bbs.end()) {
          stack.push(p);
        }
      }
    }
    assert(loop->m_bbs.find(head) != loop->m_bbs.end());
    // std::cerr << std::endl;

    for (auto &bb : loop->m_bbs) {
      for (auto &e_loop : bb->m_loops) {
        if (std::includes(e_loop->m_bbs.begin(), e_loop->m_bbs.end(),
                          loop->m_bbs.begin(), loop->m_bbs.end())) {
          e_loop->m_sub_loops.insert(loop);
        } else if (std::includes(loop->m_bbs.begin(), loop->m_bbs.end(),
                                 e_loop->m_bbs.begin(), e_loop->m_bbs.end())) {
          loop->m_sub_loops.insert(e_loop);
        } else if (e_loop->m_header == head) {
          e_loop->m_bbs.insert(loop->m_bbs.begin(), loop->m_bbs.end());
          for (auto &loop_bb : loop->m_bbs) {
            loop_bb->m_loops.insert(e_loop);
          }
          func->m_loops.erase(loop);
          loop = nullptr;
          break;
        }
      }
      if (loop == nullptr) break;
    }
    if (loop == nullptr) continue;

    for (auto &bb : loop->m_bbs) {
      bb->m_loops.insert(loop);
    }
    func->m_loops.insert(loop);
  }
  for (auto &loop : func->m_loops) {
    loop->m_loop_depth = 1;
    if (loop->m_sub_loops.empty()) {
      func->m_deepest_loops.push_back(loop);
      for (auto &bb : loop->m_bbs) {
        bb->m_deepest_loop = loop;
      }
    }
  }
  for (auto &loop : func->m_loops) {
    for (auto &inner : loop->m_sub_loops) {
      // for every outer loop, depth++
      // finally the answer is the loop depth
      ++inner->m_loop_depth;
    }
  }
  for (auto &loop : func->m_loops) {
    for (auto &bb : loop->m_bbs) {
      bb->m_loop_depth = std::max(bb->m_loop_depth, loop->m_loop_depth);
    }
    for (auto &inner : loop->m_sub_loops) {
      if (inner->m_loop_depth == loop->m_loop_depth + 1) {
        inner->m_fa_loop = loop;
      }
    }
  }
  for (auto &loop : func->m_loops) {
    for (auto it = loop->m_sub_loops.begin(); it != loop->m_sub_loops.end();) {
      // manually ++it
      if ((*it)->m_loop_depth != loop->m_loop_depth + 1) {
        it = loop->m_sub_loops.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void ComputeLoopRelationship(std::shared_ptr<Function> func) {
  func->m_deepest_loops.clear();
  for (auto &bb : func->m_bb_list) {
    bb->m_loops.clear();
    bb->m_loop_depth = 0;
  }
  func->m_loops.clear();
  ComputeDominanceRelationship(func);
  GetBackEdges(func);
  DetectNaturalLoops(func);

  for (auto &loop : func->m_loops) {
    std::cerr << "preheader: " << loop->m_preheader->m_id << std::endl;
    std::cerr << "header: " << loop->m_header->m_id << std::endl;
    std::cerr << "end: ";
    for (auto &end : loop->m_ends) {
      std::cerr << end->m_id << " ";
    }
    std::cerr << std::endl;
    std::cerr << "bbs: ";
    for (auto &bb : loop->m_bbs) {
      std::cerr << bb->m_id << " ";
    }
    std::cerr << std::endl;
    std::cerr << "depth: " << loop->m_loop_depth << std::endl;
    std::cerr << std::endl;
  }
}