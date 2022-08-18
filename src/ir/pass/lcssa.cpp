//
// Created by garen on 8/17/22.
//

#include "ir/ir-pass-manager.h"
#include "ir/loop.h"

// TODO: unfinished

void FormLCSSAInner(std::shared_ptr<Loop> loop) {
  if (loop->m_visited) return;
  loop->m_visited = true;
  bool changed = false;
  assert(!loop->m_exit_bbs.empty());

  // TODO: blocks dominating exits
  std::vector<std::shared_ptr<BasicBlock>> blocks_dominating_exits;

  std::vector<std::shared_ptr<Instruction>> worklist;
  for (auto &bb : blocks_dominating_exits) {
    if (bb->m_deepest_loop != loop) continue;
    for (auto &instr : bb->m_instr_list) {
      if (instr->m_use_list.empty())
        continue;
      else if (instr->m_use_list.size() == 1) {
        auto user = instr->m_use_list.front()->getUser();
        if (user->m_bb == bb
            && !std::dynamic_pointer_cast<PhiInstruction>(user))
          continue;
      }
      worklist.push_back(instr);
    }
  }

  // TODO: form LCSSA for instructions
  std::vector<Use *> rewriting_uses;
  std::map<std::shared_ptr<Loop>,
           std::unordered_set<std::shared_ptr<BasicBlock>>>
      loop_exit_blocks;
  while (!worklist.empty()) {
    // IMPORTANT: guarantee to me that you would never change the address of use
    // otherwise it will all be fxxked up
    auto instr = worklist.back();
    worklist.pop_back();
    auto l = instr->m_bb->m_deepest_loop;
    if (loop_exit_blocks.find(l) == loop_exit_blocks.end()) {
      loop_exit_blocks[l] = l->m_exit_bbs;
    }
    auto &exit_bbs = loop_exit_blocks[l];
    if (exit_bbs.empty()) continue;
    rewriting_uses.clear();
    for (auto &use : instr->m_use_list) {
      auto user = use->getUser();
      auto user_bb = user->m_bb;
      if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(user)) {
        user_bb = phi->GetBasicBlock(use.get());
      }
      if (instr->m_bb != user_bb && l->m_bbs.find(user_bb) == l->m_bbs.end()) {
        rewriting_uses.push_back(use.get());
      }
    }
    if (rewriting_uses.empty()) continue;
    // we are changing

    assert(instr->m_op != IROp::STORE && instr->m_op != IROp::BRANCH
           && instr->m_op == IROp::JUMP && instr->m_op == IROp::RETURN);
    std::vector<std::shared_ptr<PhiInstruction>> new_phis;
    for (auto &exit_bb : exit_bbs) {
      if (!instr->m_bb->Dominate(exit_bb)) continue;
      auto phi = std::make_shared<PhiInstruction>(instr->m_type, exit_bb);
      phi->m_is_lcssa = true;
      exit_bb->m_instr_list.insert(exit_bb->m_instr_list.begin(), phi);

      for (auto &pred : exit_bb->Predecessors()) {
        phi->AddPhiOperand(pred, instr);
        if (l->m_bbs.find(pred) == l->m_bbs.end()) {
          rewriting_uses.push_back(phi->m_contents[pred]);
        }
      }
      new_phis.push_back(phi);

      auto other_l = exit_bb->m_deepest_loop;
      assert(other_l != nullptr);
      bool flag = true;
      while (other_l != nullptr) {
        if (other_l == l) {
          flag = false;
          break;
        }
        other_l = other_l->m_fa_loop;
      }
      assert(flag);
    }
    for (auto use : rewriting_uses) {
      auto user = use->getUser();
      auto user_bb = user->m_bb;
      if (auto phi_user = std::dynamic_pointer_cast<PhiInstruction>(user)) {
        user_bb = phi_user->GetBasicBlock(use);
      }
      if (auto used_lcssa_phi = std::dynamic_pointer_cast<PhiInstruction>(
              user_bb->m_instr_list.front())) {
        assert(used_lcssa_phi->m_is_lcssa);
        if (exit_bbs.find(user_bb) != exit_bbs.end()) {
          // change value of use
          auto use_ptr = use->getValue()->KillUse(use, true);
          assert(use_ptr.get() == use);
          use_ptr->m_value = used_lcssa_phi;
          used_lcssa_phi->m_use_list.push_back(std::move(use_ptr));
        }
      } else if (new_phis.size() == 1) {
        // change value of use
        auto use_ptr = use->getValue()->KillUse(use, true);
        assert(use_ptr.get() == use);
        use_ptr->m_value = new_phis.front();
        used_lcssa_phi->m_use_list.push_back(std::move(use_ptr));
      } else {
        // ???
      }
    }
  }

  // TODO: check if the loop is LCSSA form
}

void FormLCSSARecursively(std::shared_ptr<Loop> loop) {
  for (auto &sub_loop : loop->m_sub_loops) {
    FormLCSSARecursively(sub_loop);
  }
  FormLCSSAInner(loop);
}

void FormLCSSAPass(std::shared_ptr<Function> func) {
  std::vector<std::shared_ptr<Loop>> worklist;
  for (auto &loop : func->m_loops) {
    worklist.push_back(loop);
  }
  std::unordered_set<std::shared_ptr<Loop>> test;
  while (!worklist.empty()) {
    auto loop = worklist.back();
    worklist.pop_back();
    loop->m_visited = false;
    test.insert(loop);
    for (auto &sub_loop : loop->m_sub_loops) {
      worklist.push_back(sub_loop);
    }
  }
  assert(test.size() == func->m_loops.size());
  for (auto &loop : func->m_loops) {
    FormLCSSARecursively(loop);
  }
}
