#include "asm/asm.h"

int Operand::vreg_cnt = 0;
int ASM_BasicBlock::block_cnt = 0;
std::unordered_map<std::shared_ptr<Operand>, std::string> Operand::vreg_map;

void ASM_BasicBlock::insert(std::shared_ptr<ASM_Instruction> inst) {
  m_insts.push_back(inst);
}

void ASM_BasicBlock::insertPhiMOV(std::shared_ptr<ASM_Instruction> mov) {
  m_insts.insert(m_branch_pos, mov);
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
      return getRegName();
    case OperandType::VREG:
      return getVRegName();
  }
  return "";  // unreachable, just write to avoid warning
}

std::string Operand::getRegName() {
  // rreg
  if (m_is_rreg) {
    switch (m_rreg) {
      case RReg::SP:
        return "SP";
      case RReg::LR:
        return "LR";
      case RReg::PC:
        return "PC";
      default:
        return "R" + std::to_string((int)m_rreg);
    }
  }
  // sreg
  return "S" + std::to_string((int)m_sreg);
}

std::string Operand::getVRegName() {
  std::shared_ptr<Operand> _this = shared_from_this();
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
      return "VMOV";
    case InstOp::VADD:
      return "VADD";
    case InstOp::VSUB:
      return "VSUB";
    case InstOp::VMLA:
      return "VMLA";
    case InstOp::VMLS:
      return "VMLS";
    case InstOp::VMUL:
      return "VMUL";
    case InstOp::VDIV:
      return "VDIV";
    case InstOp::VCMP:
      return "VCMP";
    case InstOp::VLDR:
      return "VLDR";
    case InstOp::VSTR:
      return "VSTR";
  }
  return "";
}

std::string ASM_Instruction::getOpSuffixName() {
  switch (m_op) {
    case InstOp::VMOV:
    case InstOp::VADD:
    case InstOp::VSUB:
    case InstOp::VMLA:
    case InstOp::VMLS:
    case InstOp::VMUL:
    case InstOp::VDIV:
    case InstOp::VCMP:
      return ".F32";
    case InstOp::VLDR:
    case InstOp::VSTR:
      return ".32";
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
  m_op = InstOp::LDR;
  m_cond = CondType::NONE;
  m_type = Type::LABEL;
  m_dest = dest;
  m_label = label;
}

LDRInst::LDRInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src,
                 std::shared_ptr<Operand> offs) {
  m_op = InstOp::LDR;
  m_cond = CondType::NONE;
  m_type = Type::REG;
  m_dest = dest;
  m_src = src;
  m_offs = offs;
}

STRInst::STRInst(std::shared_ptr<Operand> src, std::shared_ptr<Operand> dest,
                 std::shared_ptr<Operand> offs) {
  m_op = InstOp::STR;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_src = src;
  m_offs = offs;
}

MOVInst::MOVInst(std::shared_ptr<Operand> dest, int imm) {
  m_op = InstOp::MOV;
  m_cond = CondType::NONE;
  m_type = RIType::IMM;
  m_dest = dest;
  m_src = std::make_shared<Operand>(imm);
}

MOVInst::MOVInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src) {
  m_op = InstOp::MOV;
  m_cond = CondType::NONE;
  m_type = RIType::REG;
  m_dest = dest;
  m_src = src;
}

PInst::PInst(InstOp op) {
  m_op = op;
  m_cond = CondType::NONE;
}

BInst::BInst(std::shared_ptr<ASM_BasicBlock> block) {
  m_op = InstOp::B;
  m_cond = CondType::NONE;
  m_target = block;
}

CALLInst::CALLInst(VarType type, std::string label, int n) {
  m_op = InstOp::BL;
  m_cond = CondType::NONE;
  m_type = type;
  m_label = label;
  m_params = n;
}

ShiftInst::ShiftInst(InstOp op, std::shared_ptr<Operand> dest,
                     std::shared_ptr<Operand> src,
                     std::shared_ptr<Operand> sval) {
  m_op = op;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_src = src;
  m_sval = sval;
}

ASInst::ASInst(InstOp op, std::shared_ptr<Operand> dest,
               std::shared_ptr<Operand> operand1,
               std::shared_ptr<Operand> operand2) {
  m_op = op;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_operand1 = operand1;
  m_operand2 = operand2;
}

MULInst::MULInst(InstOp op, std::shared_ptr<Operand> dest,
                 std::shared_ptr<Operand> operand1,
                 std::shared_ptr<Operand> operand2,
                 std::shared_ptr<Operand> append) {
  m_op = op;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_operand1 = operand1;
  m_operand2 = operand2;
  m_append = append;
}

SDIVInst::SDIVInst(std::shared_ptr<Operand> dest,
                   std::shared_ptr<Operand> devidend,
                   std::shared_ptr<Operand> devisor) {
  m_op = InstOp::SDIV;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_devidend = devidend;
  m_devisor = devisor;
}

BITInst::BITInst(InstOp op, std::shared_ptr<Operand> dest,
                 std::shared_ptr<Operand> operand1,
                 std::shared_ptr<Operand> operand2) {
  m_op = op;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_operand1 = operand1;
  m_operand2 = operand2;
}

BITInst::BITInst(InstOp op, std::shared_ptr<Operand> dest,
                 std::shared_ptr<Operand> operand1) {
  m_op = op;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_operand1 = operand1;
}

CTInst::CTInst(InstOp op, std::shared_ptr<Operand> operand1,
               std::shared_ptr<Operand> operand2) {
  m_op = op;
  m_cond = CondType::NONE;
  m_operand1 = operand1;
  m_operand2 = operand2;
}

unsigned int ASM_Function::getStackSize() { return m_local_alloc; }

void ASM_Function::allocateStack(unsigned int size) { m_local_alloc += size; }

void ASM_Function::appendPush(std::shared_ptr<Operand> reg) {
  m_push->m_regs.push_back(reg);
}

void ASM_Function::appendPop(std::shared_ptr<Operand> reg) {
  m_pop->m_regs.push_back(reg);
}