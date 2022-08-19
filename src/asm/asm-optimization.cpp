#include "asm/asm-optimization.h"

void eliminateRedundantMOV(std::shared_ptr<ASM_Module> module) {
  for (auto& func : module->m_funcs) {
    for (auto& block : func->m_blocks) {
      auto iter = block->m_insts.begin();
      while (iter != block->m_insts.end()) {
        if ((*iter)->m_is_deleted) {
          iter++;
          continue;
        }
        auto inst = std::dynamic_pointer_cast<MOVInst>(*iter);
        if (!inst || inst->m_src->m_op_type != OperandType::REG
            || inst->m_dest->getRegType() != inst->m_src->getRegType()) {
          iter++;
          continue;
        }
        if (inst->m_dest->getRegType() == RegType::R
            && inst->m_dest->m_rreg == inst->m_src->m_rreg) {
          iter = block->m_insts.erase(iter);
        } else if (inst->m_dest->getRegType() == RegType::S
                   && inst->m_dest->m_sreg == inst->m_src->m_sreg) {
          iter = block->m_insts.erase(iter);
        } else {
          iter++;
        }
      }
    }
  }
}

void eliminateRedundantJump(std::shared_ptr<ASM_Module> module) {
  for (auto& func : module->m_funcs) {
    for (auto b_iter = func->m_blocks.begin(); b_iter != func->m_blocks.end();
         b_iter++) {
      // skip block without a jump instruction
      auto block = *b_iter;
      if (block->m_branch_pos == block->m_insts.end()) {
        continue;
      }
      auto iter = block->m_branch_pos;
      auto inst = std::dynamic_pointer_cast<BInst>(*iter);
      if (inst->m_target == *std::next(b_iter)) {
        CondType cond = ASM_Instruction::getOppositeCond(inst->m_cond);
        iter = block->m_insts.erase(iter);
        block->m_branch_pos = iter;
        if (iter != block->m_insts.end()) {
          (*iter)->m_cond = cond;
        }
      } else {
        iter = std::next(iter);
        if (iter != block->m_insts.end()) {
          inst = std::dynamic_pointer_cast<BInst>(*iter);
          if (inst->m_target == *std::next(b_iter)) {
            iter = block->m_insts.erase(iter);
            block->m_branch_pos = iter;
          }
        }
      }
    }
  }
  for (auto& func : module->m_funcs) {
    bool flag = true;
    while (flag) {
      flag = false;
      for (auto& block : func->m_blocks) {
        auto iter = block->m_branch_pos;
        if (iter == block->m_insts.end()) {
          continue;
        }
        auto inst = std::dynamic_pointer_cast<BInst>(*iter);
        if (switchTargetBlock(block, inst)) flag = true;
        if (++iter != block->m_insts.end()) {
          auto next = std::dynamic_pointer_cast<BInst>(*iter);
          if (switchTargetBlock(block, next)) flag = true;
        }
      }
    }
  }
}

bool switchTargetBlock(std::shared_ptr<ASM_BasicBlock> block,
                       std::shared_ptr<BInst> inst) {
  if (!inst) return false;
  std::shared_ptr<ASM_BasicBlock> old_target, new_target;
  old_target = inst->m_target;
  if (!old_target->m_insts.empty()) {
    auto first_inst
        = std::dynamic_pointer_cast<BInst>(old_target->m_insts.front());
    if (first_inst && first_inst->m_cond == CondType::NONE) {
      new_target = first_inst->m_target;
    }
  }
  if (!new_target) return false;
  inst->m_target = new_target;
  block->removeSuccessor(old_target);
  old_target->removePredecessor(block);
  block->appendSuccessor(new_target);
  new_target->appendPredecessor(block);
  return true;
}

void removeUnreachableBlock(std::shared_ptr<ASM_Module> module) {
  for (auto& func : module->m_funcs) {
    bool flag = true;
    while (flag) {
      flag = false;
      auto iter = func->m_blocks.begin();
      while (iter != func->m_blocks.end()) {
        auto block = *iter;
        if (block == func->m_blocks.front() || block == func->m_blocks.back()) {
          iter++;
          continue;
        }
        if (block->m_predecessors.empty()) {
          for (auto& succ : block->m_successors) {
            succ->removePredecessor(block);
          }
          iter = func->m_blocks.erase(iter);
          flag = true;
          continue;
        }
        iter++;
      }
    }
  }
}

// void combineInstruction(std::shared_ptr<ASM_Module> module) {
//   for (auto& func : module->m_funcs) {
//     for (auto& block : func->m_blocks) {
//       std::unordered_map<std::shared_ptr<Operand>,
//                          std::shared_ptr<ASM_Instruction>>
//           inst_map;
//       for (auto& inst : block->m_insts) {
//         if (inst->m_is_deleted) continue;
//         if (inst->)
//         if (auto i = std::dynamic_pointer_cast<MULInst>(inst))
//           inst_map[i->m_dest] = inst;
//         if (auto i = std::dynamic_pointer_cast<ShiftInst>(inst))
//           inst_map[i->m_dest] = inst;
//         if (auto i = std::dynamic_pointer_cast<ASInst>(inst)) {
//           if (i->m_operand2->m_op_type == OperandType::VREG) {
//             if (inst_map.find(i->m_operand2) != inst_map.end()) {
//               auto def_inst = inst_map[i->m_operand2];
//               std::shared_ptr<ASM_Instruction> ret_inst;
//               if (auto mul = std::dynamic_pointer_cast<MULInst>(def_inst)) 
//                 ret_inst = combineMULToADD(mul, i);
//             }
//           }
//         }
//       }
//     }
//   }
// }

std::shared_ptr<ASM_Instruction> combineMULToADD(std::shared_ptr<MULInst> mul,
                                                 std::shared_ptr<ASInst> as) {
  std::shared_ptr<ASM_Instruction> ret;
  if (mul->m_op != InstOp::MUL) {
    return nullptr;
  }
}

void eliminateDeadInstruction(std::shared_ptr<ASM_Module> module) {
  int cnt = 0;
  for (auto& func : module->m_funcs) {
    func->LivenessAnalysis(RegType::R);
    for (auto& block : func->m_blocks) {
      auto live = block->m_liveout;
      for (auto iter = block->m_insts.rbegin(); iter != block->m_insts.rend();
           iter++) {
        auto inst = *iter;
        if (inst->m_is_deleted) continue;
        if (inst->m_op == InstOp::BL) {
          for (auto& use : inst->m_use) {
            live.insert(use);
          }
          continue;
        }
        bool is_used = inst->m_def.empty();
        for (auto& def : inst->m_def) {
          if (live.find(def) != live.end()
              || def->m_op_type != OperandType::VREG) {
            is_used = true;
          }
          live.erase(def);
        }
        if (!is_used) {
          inst->m_is_deleted = true;
          cnt++;
          continue;
        }
        for (auto& use : inst->m_use) {
          live.insert(use);
        }
      }
    }
  }
  std::cerr << "[debug] remove dead instruction x" << cnt << std::endl;
}

void optimizeTemp(std::shared_ptr<ASM_Module> module, bool optimization) {
  eliminateRedundantJump(module);
  removeUnreachableBlock(module);

  if (optimization) {
    // combineInstruction(module);
    eliminateDeadInstruction(module);
  }
}

void optimize(std::shared_ptr<ASM_Module> module) {
  eliminateRedundantMOV(module);
}