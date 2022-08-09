//
// Created by garen on 8/1/22.
//

#include "ir/ir-pass-manager.h"

// for every instruction from the worklist, check to see if it has any uses that
// are outside the current loop. If so, insert LCSSA phi nodes and rewrite the
// uses.
// bool FormLCSSAForInstructions(
//     std::vector<std::shared_ptr<Instruction>> &worklist) {
//   //
//   std::vector<Use *> rewriting_uses;
//   while (!worklist.empty()) {
//     rewriting_uses.clear();
//     std::shared_ptr<Instruction> instr = worklist.back();
//     worklist.pop_back();
//     auto bb = instr->m_bb;
//     auto loop = bb->m_deepest_loop;
//     auto exit_bbs = loop->m_exit_bbs;
//     if (exit_bbs.empty()) continue;
//     for (auto &use : instr->m_use_list) {
//       auto user_instr =
//       std::dynamic_pointer_cast<Instruction>(use->getUser()); auto user_bb =
//       user_instr->m_bb;
//     }
//   }
// }