#include "asm/asm-optimization.h"

void reduceRedundantMOV(std::shared_ptr<ASM_Module> module) {
  for (auto& func : module->m_funcs) {
    for (auto& block : func->m_blocks) {
      auto iter = block->m_insts.begin();
      while (iter != block->m_insts.end()) {
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

void reduceAdjacentJump(std::shared_ptr<ASM_Module> module) {
  for (auto& func : module->m_funcs) {
    for (auto b_iter = func->m_blocks.begin(); b_iter != func->m_blocks.end();
         b_iter++) {
      // skip block without a jump instruction
      auto block = *b_iter;
      if (block->m_branch_pos == block->m_insts.end()) {
        continue;
      }
      if (std::dynamic_pointer_cast<BInst>(*(block->m_branch_pos))) {
        auto inst = std::dynamic_pointer_cast<BInst>(*(block->m_branch_pos));
        if (inst->m_target == *std::next(b_iter)) {
          block->m_insts.erase(block->m_branch_pos);
        }
      }   
      else {
        auto iter1 = std::next(block->m_branch_pos);
        auto iter2 = std::next(std::next(block->m_branch_pos));
        auto inst1 = std::dynamic_pointer_cast<BInst>(*iter1);
        auto inst2 = std::dynamic_pointer_cast<BInst>(*iter2);
        if (inst2->m_target == *std::next(b_iter)) {
          block->m_insts.erase(iter2);
        }
        else if (inst1->m_target == *std::next(b_iter)) {
          inst2->m_cond = inst1->getOppositeCond();
          block->m_insts.erase(iter1);
        }
      }
    }
  }
}

void optimize(std::shared_ptr<ASM_Module> module) {
  reduceRedundantMOV(module);
  reduceAdjacentJump(module);
}