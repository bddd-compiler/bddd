#include "asm/asm-fixed.h"

void storeRegisters(std::shared_ptr<ASM_Function> function, std::shared_ptr<Operand> reg) {
  function->appendPush(reg);
  function->appendPop(reg);
}

void generateLiteralPool(std::shared_ptr<ASM_Module> module) {
  int pool_number = 0;
  for (auto& func : module->m_funcs) {
    int pool_pos = 0;
    int inst_pos = 0;
    for (auto b = func->m_blocks.rbegin(); b != func->m_blocks.rend(); b++) {
      for (auto i = (*b)->m_insts.rbegin(); i != (*b)->m_insts.rend(); i++) {
        inst_pos++;
        auto inst = std::dynamic_pointer_cast<LDRInst>(*i);
        // LDR Rd, =label, need pool
        if (inst && inst->m_type == LDRInst::Type::LABEL) {
          if (inst_pos - pool_pos > POOL_RANGE) {
            auto next = std::next(i.base());
            (*b)->m_insts.insert(next, std::make_shared<POOLInst>(pool_number));
            pool_number++;
            pool_pos = inst_pos;
          }
        }
      }
    }
  }
} 