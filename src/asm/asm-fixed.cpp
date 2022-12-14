#include "asm/asm-fixed.h"

void storeRegisters(std::shared_ptr<ASM_Function> function,
                    std::shared_ptr<Operand> reg) {
  function->appendPush(reg);
  function->appendPop(reg);
}

void fixedParamsOffs(std::shared_ptr<ASM_Module> module) {
  for (auto& func : module->m_funcs) {
    int stack_size = func->getStackSize();
    if (stack_size & 7) {
      stack_size &= ~(unsigned int)7;
      stack_size += 8;
    }
    if (func->getPushSize() % 2) {
      stack_size += 4;
    }
    stack_size += func->getPushSize() * 4;
    auto block = func->m_blocks.front();
    for (auto& inst : func->m_params_set_list) {
      if (inst->m_is_deleted) continue;
      auto iter = block->m_insts.begin();
      auto ldr = std::dynamic_pointer_cast<LDRInst>(inst);
      if (!ldr) continue;
      int offs = ldr->m_offs->m_int_val + stack_size;
      if (Operand::addrOffsCheck(offs, ldr->m_dest->m_is_float))
        ldr->m_offs->m_int_val = offs;
      else {
        if (!ldr->m_dest->m_is_float) {
          auto mov
              = std::make_shared<MOVInst>(Operand::getRReg(RReg::R12), offs);
          block->m_insts.insert(func->m_params_pos_map[inst], mov);
          ldr->m_offs = Operand::getRReg(RReg::R12);
        } else {
          if (Operand::immCheck(offs)) {
            auto add = std::make_shared<ASInst>(
                InstOp::ADD, Operand::getRReg(RReg::R12),
                Operand::getRReg(RReg::SP), std::make_shared<Operand>(offs));
            block->m_insts.insert(func->m_params_pos_map[inst], add);
          } else {
            auto mov
                = std::make_shared<MOVInst>(Operand::getRReg(RReg::R12), offs);
            auto add = std::make_shared<ASInst>(
                InstOp::ADD, Operand::getRReg(RReg::R12),
                Operand::getRReg(RReg::SP), Operand::getRReg(RReg::R12));
            block->m_insts.insert(func->m_params_pos_map[inst], mov);
            block->m_insts.insert(func->m_params_pos_map[inst], add);
          }
          ldr->m_src = Operand::getRReg(RReg::R12);
          ldr->m_offs = std::make_shared<Operand>(0);
        }
      }
    }
  }
}

void generateLiteralPool(std::shared_ptr<ASM_Module> module) {
  int pool_number = 0;
  for (auto& func : module->m_funcs) {
    int pool_pos = 0;
    int inst_pos = 0;
    for (auto b = func->m_blocks.rbegin(); b != func->m_blocks.rend(); b++) {
      for (auto i = (*b)->m_insts.rbegin(); i != (*b)->m_insts.rend(); i++) {
        if ((*i)->m_is_deleted) continue;
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