//
// Created by garen on 8/17/22.
//

// 1. a preheader
// 2. a single back-edge (a single latch)
// 3. no exit block for the loop has a predecessor outside the loop

#include "ir/ir-pass-manager.h"
#include "ir/loop.h"

void ReplaceTargetBlock(std::shared_ptr<JumpInstruction> jmp_instr,
                        std::shared_ptr<BasicBlock> old_block,
                        std::shared_ptr<BasicBlock> new_block) {
  assert(jmp_instr->m_target_block == old_block);
  jmp_instr->m_target_block = new_block;
}

void ReplaceTargetBlock(std::shared_ptr<BranchInstruction> br_instr,
                        std::shared_ptr<BasicBlock> old_block,
                        std::shared_ptr<BasicBlock> new_block) {
  if (br_instr->m_true_block == old_block) {
    br_instr->m_true_block = new_block;
  } else if (br_instr->m_false_block == old_block) {
    br_instr->m_false_block = new_block;
  } else {
    assert(false);
  }
}

bool SimplifyLoop(std::shared_ptr<Loop> loop, std::shared_ptr<Function> func) {
  std::vector<std::shared_ptr<Loop>> worklist;
  worklist.push_back(loop);
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
      std::set<std::shared_ptr<BasicBlock>> outside_preds;
      for (auto &bb : l->m_bbs) {
        if (bb == l->m_header) continue;
        for (auto &pred : bb->Predecessors()) {
          if (l->m_bbs.find(pred) == l->m_bbs.end()) {
            outside_preds.insert(pred);
          }
        }
      }
      for (auto &pred : outside_preds) {
        std::cerr << "[debug] out-of-loop predecessor?????" << std::endl;
        auto terminator = pred->LastInstruction();
        // return false;
        assert(false);
      }

      for (auto &exiting_bb : l->m_exiting_bbs) {
        if (auto br = std::dynamic_pointer_cast<BranchInstruction>(
                exiting_bb->LastInstruction())) {
          assert(br->m_cond != nullptr && br->m_cond->getValue() != nullptr);
        }
      }
      assert(!l->m_preheaders.empty());
      if (l->m_preheaders.size() > 1) {
        // insert a new preheader
        auto new_preheader = std::make_shared<BasicBlock>("new_preheader");
        auto it = std::find(func->m_bb_list.begin(), func->m_bb_list.end(),
                            l->m_header);
        assert(it != func->m_bb_list.end());
        func->m_bb_list.insert(it, new_preheader);
        for (auto now = l->m_fa_loop; now; now = now->m_fa_loop) {
          now->m_bbs.insert(new_preheader);
        }
        auto jmp
            = std::make_shared<JumpInstruction>(l->m_header, new_preheader);
        bool flag = false;
        for (auto &preheader : l->m_preheaders) {
          new_preheader->AddPredecessor(preheader);
          auto term = preheader->LastInstruction();
          if (auto jmp_instr
              = std::dynamic_pointer_cast<JumpInstruction>(term)) {
            if (jmp_instr->m_target_block == l->m_header) {
              ReplaceTargetBlock(jmp_instr, l->m_header, new_preheader);
              l->m_header->ReplacePredecessorsBy(preheader, {new_preheader});
              new_preheader->AddPredecessor(preheader);
              flag = true;
            }
          } else if (auto br_instr
                     = std::dynamic_pointer_cast<BranchInstruction>(term)) {
            if (br_instr->m_true_block == l->m_header
                || br_instr->m_false_block == l->m_header) {
              ReplaceTargetBlock(br_instr, l->m_header, new_preheader);
              l->m_header->ReplacePredecessorsBy(preheader, {new_preheader});
              new_preheader->AddPredecessor(preheader);
              flag = true;
            }
          } else {
            assert(false);
          }
        }
        if (!flag) {
          for (auto &instr : l->m_header->m_instr_list) {
            if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
              phi->AddPhiOperand(new_preheader, nullptr);
            } else {
              break;
            }
          }
        }
        // l->m_header->ReplacePredecessorsBy(l->m_preheaders, new_preheader);
        l->m_preheaders.clear();
        l->m_preheaders.insert(new_preheader);
        // 最后再来
        new_preheader->PushBackInstruction(jmp);
        l->m_header->AddPredecessor(new_preheader);
        changed = true;
      }

      assert(!l->m_exit_bbs.empty());
      // assert(l->m_exit_bbs.size() == 1);
      // 多个exit blocks是很正常的
      std::unordered_set<std::shared_ptr<BasicBlock>> bad_exit_bbs;
      for (auto &exit_bb : l->m_exit_bbs) {
        bool need = false;
        // exit_bb cannot have any out-of-loop predecessors
        for (auto &p : exit_bb->Predecessors()) {
          if (l->m_bbs.find(p) == l->m_bbs.end()) {
            bad_exit_bbs.insert(exit_bb);
          }
        }
      }
      if (!bad_exit_bbs.empty()) {
        l->m_exit_bbs.clear();
        for (auto &exit_bb : bad_exit_bbs) {
          // insert a new exit block
          auto new_exit_bb = std::make_shared<BasicBlock>("new_exit_bb");
          auto it = std::find(func->m_bb_list.begin(), func->m_bb_list.end(),
                              exit_bb);
          assert(it != func->m_bb_list.end());
          func->m_bb_list.insert(it, new_exit_bb);
          for (auto now = l->m_fa_loop; now; now = now->m_fa_loop) {
            now->m_bbs.insert(new_exit_bb);
          }
          auto jmp = std::make_shared<JumpInstruction>(exit_bb, new_exit_bb);
          std::unordered_set<std::shared_ptr<BasicBlock>> preds_in_bb;
          for (auto &pred : exit_bb->Predecessors()) {
            if (l->m_bbs.find(pred) != l->m_bbs.end()) {
              preds_in_bb.insert(pred);
            }
          }
          for (auto &exiting_bb : preds_in_bb) {
            auto term = exiting_bb->LastInstruction();
            if (auto jmp_instr
                = std::dynamic_pointer_cast<JumpInstruction>(term)) {
              ReplaceTargetBlock(jmp_instr, exit_bb, new_exit_bb);
              exit_bb->ReplacePredecessorsBy(exiting_bb, {new_exit_bb});
              new_exit_bb->AddPredecessor(exiting_bb);
            } else if (auto br_instr
                       = std::dynamic_pointer_cast<BranchInstruction>(term)) {
              ReplaceTargetBlock(br_instr, exit_bb, new_exit_bb);
              exit_bb->ReplacePredecessorsBy(exiting_bb, {new_exit_bb});
              new_exit_bb->AddPredecessor(exiting_bb);
            } else {
              assert(false);
            }
          }
          // exit_bb->ReplacePredecessorsBy(l->m_exiting_bbs, new_exit_bb);

          // for (auto &preheader : l->m_preheaders) {
          //   auto term = preheader->LastInstruction();
          //   if (auto jmp_instr
          //       = std::dynamic_pointer_cast<JumpInstruction>(term)) {
          //     if (jmp_instr->m_target_block == exit_bb) {
          //       ReplaceTargetBlock(jmp_instr, exit_bb, new_exit_bb);
          //       exit_bb->ReplacePredecessorsBy(preheader, {new_exit_bb});
          //       new_exit_bb->AddPredecessor(preheader);
          //     }
          //   } else if (auto br_instr
          //              = std::dynamic_pointer_cast<BranchInstruction>(term))
          //              {
          //     if (br_instr->m_true_block == exit_bb
          //         || br_instr->m_false_block == exit_bb) {
          //       ReplaceTargetBlock(br_instr, exit_bb, new_exit_bb);
          //       exit_bb->ReplacePredecessorsBy(preheader, {new_exit_bb});
          //       new_exit_bb->AddPredecessor(preheader);
          //     }
          //   } else {
          //     assert(false);
          //   }
          // }
          l->m_exit_bbs.insert(new_exit_bb);
          // 最后再来
          new_exit_bb->PushBackInstruction(jmp);
          exit_bb->AddPredecessor(new_exit_bb);
          changed = true;
        }
      }
      // check
      for (auto &exit_bb : l->m_exit_bbs) {
        for (auto &p : exit_bb->Predecessors()) {
          assert(l->m_bbs.find(p) != l->m_bbs.end());
        }
      }

      if (l->m_latches.size() != 1) {
        // insert a new latch
        auto new_latch = std::make_shared<BasicBlock>("new_latch");
        std::vector<std::shared_ptr<PhiInstruction>> old_phis, new_phis;
        for (auto &instr : l->m_header->m_instr_list) {
          if (auto old_phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
            old_phis.push_back(old_phi);
            auto new_phi
                = std::make_shared<PhiInstruction>(old_phi->m_type, new_latch);
            new_phis.push_back(new_phi);
            new_latch->PushBackInstruction(new_phi);
            for (auto &[incoming_bb, use] : old_phi->m_contents) {
              if (l->m_latches.find(incoming_bb) != l->m_latches.end()) {
                new_phi->AddPhiOperand(incoming_bb,
                                       use ? use->getValue() : nullptr);
              }
            }
            old_phi->AddPhiOperand(new_latch, new_phi);
          } else {
            break;
          }
        }
        for (auto &old_latch : l->m_latches) {
          l->m_header->RemovePredecessor(old_latch);
          new_latch->AddPredecessor(old_latch);
          auto terminator = old_latch->LastInstruction();
          if (auto jmp
              = std::dynamic_pointer_cast<JumpInstruction>(terminator)) {
            assert(jmp->m_target_block == l->m_header);
            jmp->m_target_block = new_latch;
          } else if (auto br = std::dynamic_pointer_cast<BranchInstruction>(
                         terminator)) {
            if (br->m_true_block == l->m_header) {
              br->m_true_block = new_latch;
            } else if (br->m_false_block == l->m_header) {
              br->m_false_block = new_latch;
            } else {
              assert(false);  // impossible
            }
          }
        }
        l->m_latches.clear();
        l->m_latches.insert(new_latch);
        auto jmp = std::make_shared<JumpInstruction>(l->m_header, new_latch);
        auto it = std::find(func->m_bb_list.begin(), func->m_bb_list.end(),
                            l->m_header);
        assert(it != func->m_bb_list.end());
        ++it;
        func->m_bb_list.insert(it, new_latch);
        for (auto now = l->m_fa_loop; now; now = now->m_fa_loop) {
          now->m_bbs.insert(new_latch);
        }
        l->m_bbs.insert(new_latch);
        // 最后再来
        new_latch->PushBackInstruction(jmp);
        l->m_header->AddPredecessor(new_latch);
        changed = true;
      }

      // continue or break
      if (!changed) break;
    }
    // if (l->m_header->Predecessors().size() != 2) return false;
    // assert(l->m_header->Predecessors().size() == 2);
  }
  return true;
}

void IRPassManager::LoopSimplifyPass() {
  for (auto &func : m_builder->m_module->m_function_list) {
    RemoveTrivialBasicBlocks(func);
  }
  for (auto &func : m_builder->m_module->m_function_list) {
    ComputeLoopRelationship(func);
    for (auto &loop : func->m_loops) {
      bool flag = SimplifyLoop(loop, func);
      if (!flag) break;
    }
  }
}