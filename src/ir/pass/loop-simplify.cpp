//
// Created by garen on 8/17/22.
//

// 1. a preheader
// 2. a single back-edge (a single latch)
// 3. no exit block for the loop has a predecessor outside the loop

#include "ir/ir-pass-manager.h"
#include "ir/loop.h"

void UpdateLoopDetails(std::shared_ptr<Loop> loop) {
  loop->m_preheaders.clear();
  loop->m_latches.clear();
  for (auto &pred : loop->m_header->Predecessors()) {
    if (loop->m_bbs.find(pred) == loop->m_bbs.end()) {
      loop->m_preheaders.insert(pred);
    } else {
      loop->m_latches.insert(pred);
    }
  }
  // assert(!loop->m_preheaders.empty());
  // assert(!loop->m_latches.empty());
}
void UpdateLoopDetails(std::shared_ptr<BasicBlock> bb) {
  for (auto &loop : bb->m_loops) {
    UpdateLoopDetails(loop);
  }
}

std::shared_ptr<BasicBlock> AddMiddleBasicBlock(
    std::shared_ptr<Loop> loop, std::shared_ptr<Function> func,
    std::string bb_name, std::list<std::shared_ptr<BasicBlock>>::iterator it,
    std::unordered_set<std::shared_ptr<BasicBlock>> old_preds,
    std::shared_ptr<BasicBlock> succ, bool inside_bb) {
  auto middle_bb = std::make_shared<BasicBlock>(bb_name);
  func->m_bb_list.insert(it, middle_bb);
  if (inside_bb) loop->m_bbs.insert(middle_bb);
  for (auto now = loop->m_fa_loop; now; now = now->m_fa_loop) {
    now->m_bbs.insert(middle_bb);
  }
  auto jmp = std::make_shared<JumpInstruction>(succ, middle_bb);
  middle_bb->PushBackInstruction(jmp);
  for (auto &old_pred : old_preds) {
    auto term = old_pred->LastInstruction();
    // old_pred -> succ
    // old_pred -> middle_bb -> succ
    if (auto jmp_instr = std::dynamic_pointer_cast<JumpInstruction>(term)) {
      assert(jmp_instr->m_target_block == succ);
      jmp_instr->m_target_block = middle_bb;
      middle_bb->AddPredecessor(old_pred);
    } else if (auto br_instr
               = std::dynamic_pointer_cast<BranchInstruction>(term)) {
      if (br_instr->m_true_block == succ) {
        br_instr->m_true_block = middle_bb;
        middle_bb->AddPredecessor(old_pred);
      } else if (br_instr->m_false_block == succ) {
        br_instr->m_false_block = middle_bb;
        middle_bb->AddPredecessor(old_pred);
      } else {
        assert(false);
      }
    } else {
      assert(false);
    }
  }
  succ->ReplacePredecessorsBy(old_preds, middle_bb);
  for (auto &old_pred : old_preds) {
    UpdateLoopDetails(old_pred);
  }
  UpdateLoopDetails(succ);
  return middle_bb;
}

bool SimplifyLoop(std::shared_ptr<Loop> lp, std::shared_ptr<Function> func) {
  std::vector<std::shared_ptr<Loop>> worklist;
  worklist.push_back(lp);
  for (int i = 0; i < worklist.size(); ++i) {
    auto l = worklist[i];
    worklist.insert(worklist.end(), l->m_sub_loops.begin(),
                    l->m_sub_loops.end());
  }
  while (!worklist.empty()) {
    auto l = worklist.back();
    worklist.pop_back();
    //
    while (true) {
      bool changed = false;

      // std::set<std::shared_ptr<BasicBlock>> outside_preds;
      // for (auto &bb : l->m_bbs) {
      //   if (bb == l->m_header) continue;
      //   for (auto &pred : bb->Predecessors()) {
      //     if (l->m_bbs.find(pred) == l->m_bbs.end()) {
      //       outside_preds.insert(pred);
      //       assert(false);
      //     }
      //   }
      // }
      // for (auto &pred : outside_preds) {
      //   std::cerr << "[debug] out-of-loop predecessor?????" << std::endl;
      //   // auto terminator = pred->LastInstruction();
      //   // return false;
      //   assert(false);
      // }

      for (auto &exiting_bb : l->m_exiting_bbs) {
        if (auto br = std::dynamic_pointer_cast<BranchInstruction>(
                exiting_bb->LastInstruction())) {
          assert(br->m_cond != nullptr && br->m_cond->getValue() != nullptr);
        }
      }
      assert(!l->m_preheaders.empty());
      if (l->m_preheaders.size() > 1) {
        // insert a new preheader
        // old_preheader ->
        // old_preheader -> new_preheader -> header
        // old_preheader ->

        auto it = std::find(func->m_bb_list.begin(), func->m_bb_list.end(),
                            l->m_header);
        auto new_preheader = AddMiddleBasicBlock(
            l, func, "new_preheader", it, l->m_preheaders, l->m_header, false);
        l->m_preheaders.clear();
        l->m_preheaders.insert(new_preheader);
        std::cerr << "[debug] add a new preheader" << std::endl;
        changed = true;
      }

      if (l->m_latches.size() != 1) {
        // insert a new latch
        // old_latch ->
        // old_latch -> new_latch -> header
        // old_latch ->

        auto it = std::find(func->m_bb_list.begin(), func->m_bb_list.end(),
                            l->m_header);
        assert(it != func->m_bb_list.end());
        ++it;
        auto new_latch = AddMiddleBasicBlock(l, func, "new_latch", it,
                                             l->m_latches, l->m_header, true);

        l->m_latches.clear();
        l->m_latches.insert(new_latch);
        std::cerr << "[debug] add a new latch" << std::endl;
        changed = true;
      }

      assert(!l->m_exit_bbs.empty());
      // assert(l->m_exit_bbs.size() == 1);
      // 多个exit blocks是很正常的
      std::unordered_set<std::shared_ptr<BasicBlock>> bad_exit_bbs;
      for (auto &exit_bb : l->m_exit_bbs) {
        // exit_bb cannot have any out-of-loop predecessors
        for (auto &p : exit_bb->Predecessors()) {
          if (l->m_bbs.find(p) == l->m_bbs.end()) {
            bad_exit_bbs.insert(exit_bb);
          }
        }
      }
      if (!bad_exit_bbs.empty()) {
        // exiting_bb     -> exit_bb
        // exiting_bb     -> exit_bb
        // out_of_loop_bb -> exit_bb
        // out_of_loop_bb -> exit_bb
        // target:
        // exiting_bb -> good_exit_bb -> exit_bb
        //             out_of_loop_bb -> exit_bb
        l->m_exit_bbs.clear();
        for (auto &exit_bb : bad_exit_bbs) {
          // insert a new exit block
          std::unordered_set<std::shared_ptr<BasicBlock>> preds_in_bb;

          auto it = std::find(func->m_bb_list.begin(), func->m_bb_list.end(),
                              exit_bb);
          for (auto &pred : exit_bb->Predecessors()) {
            if (l->m_bbs.find(pred) != l->m_bbs.end()) {
              preds_in_bb.insert(pred);
            }
          }
          auto new_exit_bb = AddMiddleBasicBlock(l, func, "new_exit_bb", it,
                                                 preds_in_bb, exit_bb, false);
          l->m_exit_bbs.insert(new_exit_bb);
          std::cerr << "[debug] add a new exit bb" << std::endl;
          changed = true;
        }
      }
      // check
      for (auto &exit_bb : l->m_exit_bbs) {
        for (auto &p : exit_bb->Predecessors()) {
          assert(l->m_bbs.find(p) != l->m_bbs.end());
        }
      }

      if (!changed) break;
    }
    // if (l->m_header->Predecessors().size() != 2) return false;
    // assert(l->m_header->Predecessors().size() == 2);
  }
  return true;
}

bool CreateUniqueLatch(std::shared_ptr<Loop> lp,
                       std::shared_ptr<Function> func) {
  std::vector<std::shared_ptr<Loop>> worklist;
  worklist.push_back(lp);
  for (int i = 0; i < worklist.size(); ++i) {
    auto l = worklist[i];
    worklist.insert(worklist.end(), l->m_sub_loops.begin(),
                    l->m_sub_loops.end());
  }
  while (!worklist.empty()) {
    auto l = worklist.back();
    worklist.pop_back();
    //
    if (l->m_latches.size() > 1) {
      // insert a new latch
      // old_latch ->
      // old_latch -> new_latch -> header
      // old_latch ->

      auto it = std::find(func->m_bb_list.begin(), func->m_bb_list.end(),
                          l->m_header);
      assert(it != func->m_bb_list.end());
      ++it;
      auto new_latch = AddMiddleBasicBlock(l, func, "new_latch", it,
                                           l->m_latches, l->m_header, true);

      l->m_latches.clear();
      l->m_latches.insert(new_latch);
      std::cerr << "[debug] add a new latch" << std::endl;
      assert(l->m_latches.size() == 1);
    }

    if (l->m_preheaders.size() > 1) {
      // insert a new preheader
      // old_preheader ->
      // old_preheader -> new_preheader -> header
      // old_preheader ->

      auto it = std::find(func->m_bb_list.begin(), func->m_bb_list.end(),
                          l->m_header);
      auto new_preheader = AddMiddleBasicBlock(
          l, func, "new_preheader", it, l->m_preheaders, l->m_header, false);
      l->m_preheaders.clear();
      l->m_preheaders.insert(new_preheader);
      std::cerr << "[debug] add a new preheader" << std::endl;
      assert(l->m_preheaders.size() == 1);
    }
  }
  return true;
}

bool SimplifyLoopRecursively(std::shared_ptr<Loop> loop,
                             std::shared_ptr<Function> func) {
  // depth first
  for (auto &sub_loop : loop->m_sub_loops) {
    SimplifyLoopRecursively(sub_loop, func);
  }
  // SimplifyLoop(loop, func);
  CreateUniqueLatch(loop, func);
  return false;
}

void IRPassManager::LoopSimplifyPass() {
  for (auto &func : m_builder->m_module->m_function_list) {
    RemoveTrivialBasicBlocks(func);
    ComputeLoopRelationship(func);
    for (auto &loop : func->m_top_loops) {
      SimplifyLoopRecursively(loop, func);
    }
  }
  // for (auto &func : m_builder->m_module->m_function_list) {
  //   RemoveTrivialBasicBlocks(func);
  // }
  // for (auto &func : m_builder->m_module->m_function_list) {
  //   ComputeLoopRelationship(func);
  //   for (auto &loop : func->m_loops) {
  //     bool flag = SimplifyLoopRecursively(loop, func);
  //     if (!flag) break;
  //   }
  // }
}