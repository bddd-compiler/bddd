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

void combineInstruction(std::shared_ptr<ASM_Module> module) {
  for (auto& func : module->m_funcs) {
    for (auto& block : func->m_blocks) {
      std::unordered_map<std::shared_ptr<Operand>,
                         std::shared_ptr<ASM_Instruction>>
          inst_map;
      for (auto& inst : block->m_insts) {
        if (inst->m_is_deleted) continue;
        if (inst->m_op == InstOp::MUL) {
          auto i = std::dynamic_pointer_cast<MULInst>(inst);
          if (i->m_dest->m_op_type == OperandType::VREG)
            inst_map[i->m_dest] = inst;
        }
        if (auto i = std::dynamic_pointer_cast<ShiftInst>(inst))
          if (i->m_dest->m_op_type == OperandType::VREG)
            inst_map[i->m_dest] = inst;
        if (inst->m_op == InstOp::ADD) {
          auto i = std::dynamic_pointer_cast<ASInst>(inst);
          if (i->m_operand2->m_op_type == OperandType::VREG && !i->m_shift) {
            std::shared_ptr<ASM_Instruction> def_inst;
            if (inst_map.find(i->m_operand1) != inst_map.end())
              def_inst = inst_map[i->m_operand1];
            else if (inst_map.find(i->m_operand2) != inst_map.end())
              def_inst = inst_map[i->m_operand2];
            std::shared_ptr<ASM_Instruction> ret_inst;
            if (auto mul = std::dynamic_pointer_cast<MULInst>(def_inst)) {
              inst = combineMULToADD(mul, i);
              inst->m_block = block;
              std::cout << "combine mul to add" << std::endl;
            }
            if (auto shift = std::dynamic_pointer_cast<ShiftInst>(def_inst)) {
              inst = combineShiftToADD(shift, i);
              inst->m_block = block;
              std::cout << "combine shift to add" << std::endl;
            }
          }
        }
      }
    }
  }
}

std::shared_ptr<ASM_Instruction> combineMULToADD(std::shared_ptr<MULInst> mul,
                                                 std::shared_ptr<ASInst> as) {
  auto ret = as->m_dest;
  auto op1 = as->m_operand1;
  if (op1 == mul->m_dest) {
    op1 = as->m_operand2;
  }
  return std::make_shared<MULInst>(InstOp::MLA, ret, mul->m_operand1,
                                   mul->m_operand2, op1);
}

std::shared_ptr<ASM_Instruction> combineShiftToADD(
    std::shared_ptr<ShiftInst> shift_inst, std::shared_ptr<ASInst> as) {
  auto ret = as->m_dest;
  auto op1 = as->m_operand1;
  if (op1 == shift_inst->m_dest) {
    op1 = as->m_operand2;
  }
  auto ret_inst
      = std::make_shared<ASInst>(InstOp::ADD, ret, op1, shift_inst->m_src);
  ret_inst->m_shift = std::make_unique<Shift>(shift_inst->m_op,
                                              shift_inst->m_sval->m_int_val);
  return ret_inst;
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