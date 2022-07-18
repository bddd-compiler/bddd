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

std::string Operand::getName() {
  switch (m_op_type) {
    case OperandType::IMM:
      return ("#" + std::to_string(m_immval));
    case OperandType::REG:
      return m_is_rreg ? getRegName(m_rreg) : getRegName(m_sreg);
    case OperandType::VREG:
      return getVRegName();
  }
  return "";  // unreachable, just write to avoid warning
}

std::string Operand::getRegName(RReg reg) {
  std::string name;
  int i = (int)reg;
  switch (i) {
    case 13:
      name = "SP";
    case 14:
      name = "LR";
    case 15:
      name = "PC";
    default:
      name = "R" + std::to_string(i);
  }
  return name;
}

std::string Operand::getRegName(SReg reg) {
  std::string name;
  int i = (int)reg;
  name = "S" + std::to_string(i);
  return name;
}

std::string Operand::getVRegName() {
  std::shared_ptr<Operand> _this(this);
  auto it = vreg_map.find(_this);
  if (it != vreg_map.end()) {
    return it->second;
  }
  std::string name = "VR" + std::to_string(vreg_cnt++);
  vreg_map.insert(std::make_pair(_this, name));
  return name;
}

Shift::Shift(ShiftType t, int v) : s_type(t), s_val(v) {
  if (v < 0 || v > 32) {
    std::cout << "invalid shift value!" << std::endl;
  }
}

std::string ASM_Instruction::getOpName() {
  switch (m_op) {
    case InstOp::LDR:
      return "LDR";
    case InstOp::STR:
      return "STR";
    case InstOp::ADR:
      return "ADR";
    case InstOp::MOV:
      return "MOV";
    case InstOp::PUSH:
      return "PUSH";
    case InstOp::POP:
      return "POP";
    case InstOp::B:
      return "B";
    case InstOp::BL:
      return "BL";
    case InstOp::LSL:
      return "LSL";
    case InstOp::LSR:
      return "LSR";
    case InstOp::ASR:
      return "ASR";
    case InstOp::ROR:
      return "ROR";
    case InstOp::RRX:
      return "RRX";
    case InstOp::ADD:
      return "ADD";
    case InstOp::SUB:
      return "SUB";
    case InstOp::RSB:
      return "RSB";
    case InstOp::MLA:
      return "MLA";
    case InstOp::MLS:
      return "MLS";
    case InstOp::MUL:
      return "MUL";
    case InstOp::SDIV:
      return "SDIV";
    case InstOp::AND:
      return "AND";
    case InstOp::ORR:
      return "ORR";
    case InstOp::MVN:
      return "MVN";
    case InstOp::EOR:
      return "EOR";
    case InstOp::BIC:
      return "BIC";
    case InstOp::CMP:
      return "CMP";
    case InstOp::TST:
      return "TST";
    case InstOp::VMOV:
      return "VMOV.F32";
    case InstOp::VADD:
      return "VADD.F32";
    case InstOp::VSUB:
      return "VSUB.F32";
    case InstOp::VMLA:
      return "VMLA.F32";
    case InstOp::VMLS:
      return "VMLS.F32";
    case InstOp::VMUL:
      return "VMUL.F32";
    case InstOp::VDIV:
      return "VDIV.F32";
    case InstOp::VCMP:
      return "VCMP.F32";
    case InstOp::VLDR:
      return "VLDR.32";
    case InstOp::VSTR:
      return "VSTR.32";
  }
  return "";
}

std::string ASM_Instruction::getCondName() {
  switch (m_cond) {
    case CondType::EQ:
      return "EQ";
    case CondType::NE:
      return "NE";
    case CondType::LT:
      return "LT";
    case CondType::LE:
      return "LE";
    case CondType::GT:
      return "GT";
    case CondType::GE:
      return "GE";
  }
  return "";
}

LDRInst::LDRInst(std::shared_ptr<Operand> dest, std::string label) {
  m_type = Type::LABEL;
  m_dest = dest;
  m_label = label;
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

CALLInst::CALLInst(VarType type, std::string label, int n) {
  m_op = InstOp::BL;
  m_type = type;
  m_label = label;
  m_params = n;
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
                 std::shared_ptr<Operand> append) {
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