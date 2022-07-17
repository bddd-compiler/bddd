#include "asm/asm.h"

void ASM_BasicBlock::insert(std::shared_ptr<ASM_Instruction> inst) {
  m_insts.push_back(inst);
}

// arm instruction use a 12-bit immediate
// 4 bits rotation, 8 bits value
// check whether the imm is valid
bool Operand::immCheck(int imm) {
  if ((imm & ~0xff) == 0 || (imm & ~0xc000003f) == 0 || (imm & ~0xf000000f)
      || (imm & ~0xfc000003) == 0) {
    return true;
  }
  unsigned int window = (0xff << 24);
  while (window > (unsigned int)0xff) {
    if ((imm & ~window) == 0) {
      return true;
    }
    window >>= 2;
  }
  return false;
}

Shift::Shift(ShiftType t, int v) : s_type(t), s_val(v) {
  if (v < 0 || v > 32) {
    std::cout << "invalid shift value!" << std::endl;
  }
}

LDRInst::LDRInst(std::shared_ptr<Operand> dest, std::string label) {
  m_type = Type::LABEL;
  m_dest = dest;
  m_src = std::make_shared<Operand>(label);
}

LDRInst::LDRInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src,
                 std::shared_ptr<Operand> offs) {
  m_type = Type::REG;
  m_dest = dest;
  m_src = src;
  m_offs = offs;
}

STRInst::STRInst(std::shared_ptr<Operand> src, std::shared_ptr<Operand> dest,
                 std::shared_ptr<Operand> offs) {
  m_dest = dest;
  m_src = src;
  m_offs = offs;
}

MOVInst::MOVInst(std::shared_ptr<Operand> dest, int imm) {
  m_type = RIType::IMM;
  m_dest = dest;
  m_src = std::make_shared<Operand>(imm);
}

MOVInst::MOVInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src) {
  m_type = RIType::REG;
  m_dest = dest;
  m_src = src;
}

PInst::PInst(InstOp op, std::vector<std::shared_ptr<Operand>> regs) {
  m_op = op;
  for (int i = 0; i < regs.size(); i++) {
    m_regs.push_back(move(regs[i]));
  }
}

BInst::BInst(std::shared_ptr<ASM_BasicBlock> block) {
  m_op = InstOp::B;
  m_target = block;
}

ShiftInst::ShiftInst(InstOp op, std::shared_ptr<Operand> dest,
                     std::shared_ptr<Operand> src,
                     std::shared_ptr<Operand> sval) {
  m_op = op;
  m_dest = dest;
  m_src = src;
  m_sval = sval;
}

ASInst::ASInst(InstOp op, std::shared_ptr<Operand> dest,
               std::shared_ptr<Operand> operand1,
               std::shared_ptr<Operand> operand2) {
  m_op = op;
  m_dest = dest;
  m_operand1 = operand1;
  m_operand2 = operand2;
}

MULInst::MULInst(InstOp op, std::shared_ptr<Operand> dest,
                 std::shared_ptr<Operand> operand1,
                 std::shared_ptr<Operand> operand2,
                 std::shared_ptr<Operand> append = nullptr) {
  m_op = op;
  m_dest = dest;
  m_operand1 = operand1;
  m_operand2 = operand2;
  m_append = append;
}

SDIVInst::SDIVInst(std::shared_ptr<Operand> dest,
                   std::shared_ptr<Operand> devidend,
                   std::shared_ptr<Operand> devisor) {
  m_dest = dest;
  m_devidend = devidend;
  m_devisor = devisor;
}

BITInst::BITInst(InstOp op, std::shared_ptr<Operand> dest,
                 std::shared_ptr<Operand> operand1,
                 std::shared_ptr<Operand> operand2) {
  m_op = op;
  m_dest = dest;
  m_operand1 = operand1;
  m_operand2 = operand2;
}

BITInst::BITInst(InstOp op, std::shared_ptr<Operand> dest,
                 std::shared_ptr<Operand> operand1) {
  m_op = op;
  m_dest = dest;
  m_operand1 = operand1;
}

CTInst::CTInst(InstOp op, std::shared_ptr<Operand> operand1,
               std::shared_ptr<Operand> operand2) {
  m_op = op;
  m_operand1 = operand1;
  m_operand2 = operand2;
}