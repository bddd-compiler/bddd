#include "asm/asm-register.h"
#include "asm/asm.h"

void ASM_BasicBlock::appendSuccessor(std::shared_ptr<ASM_BasicBlock> succ) {
  m_successors.push_back(succ);
}

void ASM_BasicBlock::appendPredecessor(std::shared_ptr<ASM_BasicBlock> pred) {
  m_predecessors.push_back(pred);
}

std::vector<std::shared_ptr<ASM_BasicBlock>> ASM_BasicBlock::getSuccessors() {
  return m_successors;
}

std::vector<std::shared_ptr<ASM_BasicBlock>> ASM_BasicBlock::getPredecessors() {
  return m_predecessors;
}

void RegisterAllocator::LivenessAnalysis() {
  // calculate def and use set of all blocks
  for (auto& b : m_cur_func->m_blocks) {
    b->m_def.clear();
    b->m_use.clear();
    for (auto& i : b->m_insts) {
      for (auto& def : i->m_def) {
        if (b->m_def.find(def) == b->m_def.end()) {
          b->m_def.insert(def);
        }
      }
      for (auto& use : i->m_use) {
        if (b->m_use.find(use) == b->m_use.end()) {
          b->m_use.insert(use);
        }
      }
    }
    b->m_livein.clear();
    b->m_liveout.clear();
  }

  // calculate liveIn and liveOut of all blocks
  // loop until nothing was added
  bool end;
  do {
    end = true;
    for (auto iter = m_cur_func->m_blocks.rbegin();
         iter != m_cur_func->m_blocks.rend(); iter++) {
      auto b = *iter;
      // record in'[b] and out'[b]
      auto temp_in = b->m_livein;
      auto temp_out = b->m_liveout;

      // in[b] = use[b] union (out[b] - def[b])
      b->m_livein = b->m_use;
      for (auto& out : b->m_liveout) {
        if (b->m_def.find(out) == b->m_def.end()
            && b->m_livein.find(out) == b->m_livein.end()) {
          b->m_livein.insert(out);
        }
      }

      // out[b] = union(in[s]), s in succ[b]
      for (auto& successor : b->getSuccessors()) {
        for (auto& in : successor->m_livein) {
          if (b->m_liveout.find(in) == b->m_liveout.end()) {
            b->m_liveout.insert(in);
          }
        }
      }

      // check if loop is end
      if (b->m_livein != temp_in || b->m_liveout != temp_out) end = false;
    }
  } while (!end);

  // print
  // for (auto& b : m_cur_func->m_blocks) {
  //   std::cout << b->m_label << ":" << std::endl;
  //   std::cout << "liveUse: ";
  //   for (auto& use : b->m_use) std::cout << use->getName() << " ";
  //   std::cout << std::endl;
  //   std::cout << "liveDef: ";
  //   for (auto& def : b->m_def) std::cout << def->getName() << " ";
  //   std::cout << std::endl;
  //   std::cout << "liveIn:  ";
  //   for (auto& in : b->m_livein) std::cout << in->getName() << " ";
  //   std::cout << std::endl;
  //   std::cout << "liveOut: ";
  //   for (auto& out : b->m_liveout) std::cout << out->getName() << " ";
  //   std::cout << std::endl;
  // }
}

void ASM_Instruction::addDef(std::shared_ptr<Operand> def) {
  m_def.insert(def);
}

void ASM_Instruction::addUse(std::shared_ptr<Operand> use) {
  if (use->m_op_type == OperandType::IMM) return;
  m_use.insert(use);
}

void LDRInst::replaceDef(std::shared_ptr<Operand> newOp,
                         std::shared_ptr<Operand> oldOp) {
  assert(m_dest == oldOp);
  m_dest = newOp;
}

void LDRInst::replaceUse(std::shared_ptr<Operand> newOp,
                         std::shared_ptr<Operand> oldOp) {
  assert(m_type == Type::REG);
  assert(m_src == oldOp || m_offs == oldOp);
  if (m_src == oldOp) m_src = newOp;
  if (m_offs == oldOp) m_offs = newOp;
}

void STRInst::replaceDef(std::shared_ptr<Operand> newOp,
                         std::shared_ptr<Operand> oldOp) {
  assert(false);
  return;
}

void STRInst::replaceUse(std::shared_ptr<Operand> newOp,
                         std::shared_ptr<Operand> oldOp) {
  assert(m_src == oldOp || m_dest == oldOp || m_offs == oldOp);
  if (m_src == oldOp) m_src = newOp;
  if (m_dest == oldOp) m_dest = newOp;
  if (m_offs == oldOp) m_offs = newOp;
}

void MOVInst::replaceDef(std::shared_ptr<Operand> newOp,
                         std::shared_ptr<Operand> oldOp) {
  assert(m_dest == oldOp);
  m_dest = newOp;
}

void MOVInst::replaceUse(std::shared_ptr<Operand> newOp,
                         std::shared_ptr<Operand> oldOp) {
  assert(m_type == RIType::REG);
  assert(m_dest == oldOp || m_src == oldOp);
  if (m_dest == oldOp) m_dest = newOp;
  if (m_src == oldOp) m_src = newOp;
}

void PInst::replaceDef(std::shared_ptr<Operand> newOp,
                       std::shared_ptr<Operand> oldOp) {
  assert(false);
  return;
}

void PInst::replaceUse(std::shared_ptr<Operand> newOp,
                       std::shared_ptr<Operand> oldOp) {
  assert(false);
  return;
}

void BInst::replaceDef(std::shared_ptr<Operand> newOp,
                       std::shared_ptr<Operand> oldOp) {
  assert(false);
  return;
}

void BInst::replaceUse(std::shared_ptr<Operand> newOp,
                       std::shared_ptr<Operand> oldOp) {
  assert(false);
  return;
}

void CALLInst::replaceDef(std::shared_ptr<Operand> newOp,
                          std::shared_ptr<Operand> oldOp) {
  assert(false);
  return;
}

void CALLInst::replaceUse(std::shared_ptr<Operand> newOp,
                          std::shared_ptr<Operand> oldOp) {
  assert(false);
  return;
}

void ShiftInst::replaceDef(std::shared_ptr<Operand> newOp,
                           std::shared_ptr<Operand> oldOp) {
  assert(m_dest == oldOp);
  m_dest = newOp;
}

void ShiftInst::replaceUse(std::shared_ptr<Operand> newOp,
                           std::shared_ptr<Operand> oldOp) {
  assert(m_src == oldOp || m_sval == oldOp);
  if (m_src == oldOp) m_src = newOp;
  if (m_sval == oldOp) m_sval = newOp;
}

void ASInst::replaceDef(std::shared_ptr<Operand> newOp,
                        std::shared_ptr<Operand> oldOp) {
  assert(m_dest == oldOp);
  m_dest = newOp;
}

void ASInst::replaceUse(std::shared_ptr<Operand> newOp,
                        std::shared_ptr<Operand> oldOp) {
  assert(m_operand1 == oldOp || m_operand2 == oldOp);
  if (m_operand1 == oldOp) m_operand1 = newOp;
  if (m_operand2 == oldOp) m_operand2 = newOp;
}

void MULInst::replaceDef(std::shared_ptr<Operand> newOp,
                         std::shared_ptr<Operand> oldOp) {
  assert(m_dest == oldOp);
  m_dest = newOp;
}

void MULInst::replaceUse(std::shared_ptr<Operand> newOp,
                         std::shared_ptr<Operand> oldOp) {
  assert(m_operand1 == oldOp || m_operand2 == oldOp
         || m_append && m_append == oldOp);
  if (m_operand1 == oldOp) m_operand1 = newOp;
  if (m_operand2 == oldOp) m_operand2 = newOp;
  if (m_append && m_append == oldOp) m_append = newOp;
}

void SDIVInst::replaceDef(std::shared_ptr<Operand> newOp,
                        std::shared_ptr<Operand> oldOp) {
  assert(m_dest == oldOp);
  m_dest = newOp;
}

void SDIVInst::replaceUse(std::shared_ptr<Operand> newOp,
                        std::shared_ptr<Operand> oldOp) {
  assert(m_devidend == oldOp || m_devisor == oldOp);
  if (m_devidend == oldOp) m_devidend = newOp;
  if (m_devisor == oldOp) m_devisor = newOp;
}

void BITInst::replaceDef(std::shared_ptr<Operand> newOp,
                        std::shared_ptr<Operand> oldOp) {
  assert(m_dest == oldOp);
  m_dest = newOp;
}

void BITInst::replaceUse(std::shared_ptr<Operand> newOp,
                        std::shared_ptr<Operand> oldOp) {
  assert(m_operand1 == oldOp || m_operand2 == oldOp);
  if (m_operand1 == oldOp) m_operand1 = newOp;
  if (m_operand2 == oldOp) m_operand2 = newOp;
}

void CTInst::replaceDef(std::shared_ptr<Operand> newOp,
                        std::shared_ptr<Operand> oldOp) {
  assert(false);
  return;
}

void CTInst::replaceUse(std::shared_ptr<Operand> newOp,
                        std::shared_ptr<Operand> oldOp) {
  assert(m_operand1 == oldOp || m_operand2 == oldOp);
  if (m_operand1 == oldOp) m_operand1 = newOp;
  if (m_operand2 == oldOp) m_operand2 = newOp;
}