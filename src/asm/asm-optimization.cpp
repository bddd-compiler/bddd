#include "asm/asm-optimization.h"

void reduceRedundantMOV(std::shared_ptr<ASM_Module> module) {
  for (auto& func : module->m_funcs) {
    for (auto& block : func->m_blocks) {
      auto iter = block->m_insts.begin();
      while (iter != block->m_insts.end()) {
        auto inst = std::dynamic_pointer_cast<MOVInst>(*iter);
        if (!inst || inst->m_src->m_op_type != OperandType::REG
            || inst->m_dest->m_is_rreg ^ inst->m_src->m_is_rreg) {
          iter++;
          continue;
        }
        if (inst->m_dest->m_rreg == inst->m_src->m_rreg) {
          iter = block->m_insts.erase(iter);
        } else {
          iter++;
        }
      }
    }
  }
}

void optimize(std::shared_ptr<ASM_Module> module) {
  reduceRedundantMOV(module);
}