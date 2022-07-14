#include "asm/asm.h"

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

LDRInst::LDRInst(std::unique_ptr<Operand> dest, std::string label) {
  m_type = Type::LABEL;
  m_dest = std::move(dest);
  m_src = std::make_unique<Operand>(label);
}

LDRInst::LDRInst(std::unique_ptr<Operand> dest, std::unique_ptr<Operand> src,
                 int imm) {
  m_type = Type::IMM;
  m_dest = std::move(dest);
  m_src = std::move(src);
  m_offs = std::make_unique<Operand>(imm);
}

LDRInst::LDRInst(std::unique_ptr<Operand> dest, std::unique_ptr<Operand> src,
                 std::unique_ptr<Operand> offs) {
  m_type = Type::REG;
  m_dest = std::move(dest);
  m_src = std::move(src);
  m_offs = std::move(offs);
}

STRInst::STRInst(std::unique_ptr<Operand> src, std::unique_ptr<Operand> dest,
                 int imm) {
  m_type = RIType::IMM;
  m_src = std::move(src);
  m_dest = std::move(dest);
  m_offs = std::make_unique<Operand>(imm);
}

STRInst::STRInst(std::unique_ptr<Operand> src, std::unique_ptr<Operand> dest,
                 std::unique_ptr<Operand> offs) {
  m_type = RIType::REG;
  m_dest = std::move(dest);
  m_src = std::move(src);
  m_offs = std::move(offs);
}

MOVInst::MOVInst(std::unique_ptr<Operand> dest, int imm) {
  m_type = RIType::IMM;
  m_dest = std::move(dest);
  m_src = std::make_unique<Operand>(imm);
}

MOVInst::MOVInst(std::unique_ptr<Operand> dest, std::unique_ptr<Operand> src) {
  m_type = RIType::REG;
  m_dest = std::move(dest);
  m_src = std::move(src);
}

PInst::PInst(InstOp op, std::vector<std::unique_ptr<Operand>> regs) {
  m_op = op;
  for (int i = 0; i < regs.size(); i++) {
    m_regs.push_back(move(regs[i]));
  }
}

BInst::BInst(InstOp op, std::unique_ptr<Operand> label) {
  m_op = op;
  m_label = std::move(label);
}

ShiftInst::ShiftInst(InstOp op, std::unique_ptr<Operand> dest,
                     std::unique_ptr<Operand> src, int imm) {
  m_op = op;
  m_type = RIType::IMM;
  m_dest = std::move(dest);
  m_src = std::move(src);
  m_sval = std::make_unique<Operand>(imm);
}

ShiftInst::ShiftInst(InstOp op, std::unique_ptr<Operand> dest,
                     std::unique_ptr<Operand> src,
                     std::unique_ptr<Operand> sval) {
  m_op = op;
  m_type = RIType::REG;
  m_dest = std::move(dest);
  m_src = std::move(src);
  m_sval = std::move(sval);
}

ASInst::ASInst(InstOp op, std::unique_ptr<Operand> dest,
               std::unique_ptr<Operand> operand1, int imm) {
  m_op = op;
  m_type = RIType::IMM;
  m_dest = std::move(dest);
  m_operand1 = std::move(operand1);
  m_operand2 = std::make_unique<Operand>(imm);
}

ASInst::ASInst(InstOp op, std::unique_ptr<Operand> dest,
               std::unique_ptr<Operand> operand1,
               std::unique_ptr<Operand> operand2) {
  m_op = op;
  m_type = RIType::REG;
  m_dest = std::move(dest);
  m_operand1 = std::move(operand1);
  m_operand2 = std::move(operand2);
}

MULInst::MULInst(InstOp op, std::unique_ptr<Operand> dest,
                 std::unique_ptr<Operand> operand1,
                 std::unique_ptr<Operand> operand2,
                 std::unique_ptr<Operand> append = nullptr) {
  m_op = op;
  m_dest = std::move(dest);
  m_operand1 = std::move(operand1);
  m_operand2 = std::move(operand2);
  m_append = std::move(append);
}

SDIVInst::SDIVInst(std::unique_ptr<Operand> dest,
                   std::unique_ptr<Operand> devidend,
                   std::unique_ptr<Operand> devisor) {
  m_dest = std::move(dest);
  m_devidend = std::move(devidend);
  m_devisor = std::move(devisor);
}

BITInst::BITInst(InstOp op, std::unique_ptr<Operand> dest,
                 std::unique_ptr<Operand> operand1, int imm) {
  m_op = op;
  m_type = RIType::IMM;
  m_dest = std::move(dest);
  m_operand1 = std::move(operand1);
  m_operand2 = std::make_unique<Operand>(imm);
}

BITInst::BITInst(InstOp op, std::unique_ptr<Operand> dest,
                 std::unique_ptr<Operand> operand1,
                 std::unique_ptr<Operand> operand2) {
  m_op = op;
  m_type = RIType::REG;
  m_dest = std::move(dest);
  m_operand1 = std::move(operand1);
  m_operand2 = std::move(operand2);
}

BITInst::BITInst(InstOp op, std::unique_ptr<Operand> dest, int imm) {
  m_op = op;
  m_type = RIType::IMM;
  m_dest = std::move(dest);
  m_operand2 = std::make_unique<Operand>(imm);
}

BITInst::BITInst(InstOp op, std::unique_ptr<Operand> dest,
                 std::unique_ptr<Operand> operand1) {
  m_op = op;
  m_type = RIType::REG;
  m_dest = std::move(dest);
  m_operand1 = std::move(operand1);
}

CTInst::CTInst(InstOp op, std::unique_ptr<Operand> operand1, int imm) {
  m_op = op;
  m_type = RIType::IMM;
  m_operand1 = std::move(operand1);
  m_operand2 = std::make_unique<Operand>(imm);
}

CTInst::CTInst(InstOp op, std::unique_ptr<Operand> operand1,
               std::unique_ptr<Operand> operand2) {
  m_op = op;
  m_type = RIType::REG;
  m_operand1 = std::move(operand1);
  m_operand2 = std::move(operand2);
}