#include "asm/asm.h"

CondType GetCondFromIR(IROp op) {
  switch (op) {
    case IROp::I_SGE:
    case IROp::F_GE:
      return CondType::GE;
    case IROp::I_SGT:
    case IROp::F_GT:
      return CondType::GT;
    case IROp::I_SLE:
    case IROp::F_LE:
      return CondType::LE;
    case IROp::I_SLT:
    case IROp::F_LT:
      return CondType::LT;
    case IROp::I_EQ:
    case IROp::F_EQ:
      return CondType::EQ;
    case IROp::I_NE:
    case IROp::F_NE:
      return CondType::NE;
    default:
      break;
  }
  return CondType::NONE;
}

int ASM_BasicBlock::block_id = 0;

int Operand::vrreg_cnt = 0;
int Operand::vsreg_cnt = 0;
std::unordered_map<std::shared_ptr<Operand>, std::string> Operand::vreg_map;
std::unordered_map<RReg, std::shared_ptr<Operand>> Operand::rreg_map;
std::unordered_map<SReg, std::shared_ptr<Operand>> Operand::sreg_map;

int ASM_Function::getPushSize() {
  return m_push->m_regs.size() + m_f_push->m_regs.size();
}

int ASM_Function::getStackSize() { return m_local_alloc; }

void ASM_Function::allocateStack(int size) { m_local_alloc += size; }

void ASM_Function::appendPush(std::shared_ptr<Operand> reg) {
  if (reg->m_is_float)
    m_f_push->m_regs.insert(reg);
  else
    m_push->m_regs.insert(reg);
}

void ASM_Function::appendPop(std::shared_ptr<Operand> reg) {
  if (reg->m_is_float)
    m_f_pop->m_regs.insert(reg);
  else
    m_pop->m_regs.insert(reg);
}

void ASM_BasicBlock::insert(std::shared_ptr<ASM_Instruction> inst) {
  m_insts.push_back(inst);
  inst->m_block = shared_from_this();
}

void ASM_BasicBlock::insertSpillLDR(
    std::list<std::shared_ptr<ASM_Instruction>>::iterator iter,
    std::shared_ptr<ASM_Instruction> ldr, std::shared_ptr<ASM_Instruction> add,
    std::shared_ptr<ASM_Instruction> mov) {
  if (mov) {
    m_insts.insert(iter, mov);
    mov->m_block = shared_from_this();
  }
  if (add) {
    m_insts.insert(iter, add);
    add->m_block = shared_from_this();
  }
  m_insts.insert(iter, ldr);
  ldr->m_block = shared_from_this();
}

void ASM_BasicBlock::insertSpillSTR(
    std::list<std::shared_ptr<ASM_Instruction>>::iterator iter,
    std::shared_ptr<ASM_Instruction> str, std::shared_ptr<ASM_Instruction> add,
    std::shared_ptr<ASM_Instruction> mov) {
  auto next = std::next(iter);
  if (mov) {
    m_insts.insert(next, mov);
    mov->m_block = shared_from_this();
  }
  if (add) {
    m_insts.insert(next, add);
    add->m_block = shared_from_this();
  }
  m_insts.insert(next, str);
  str->m_block = shared_from_this();
}

void ASM_BasicBlock::insertPhiMOV(std::shared_ptr<ASM_Instruction> mov) {
  assert(m_branch_pos != m_insts.end());
  m_insts.insert(m_branch_pos, mov);
  mov->m_block = shared_from_this();
}

void ASM_BasicBlock::appendFilledMOV(std::shared_ptr<ASM_Instruction> mov) {
  m_mov_filled_list.push_back(mov);
}

void ASM_BasicBlock::fillMOV() {
  for (auto& mov : m_mov_filled_list) {
    insertPhiMOV(mov);
  }
}

#include "asm/asm.h"

// for MOV, ADD, SUB, RSB
/*
  int type immediate check
  arm instruction use a 12-bit immediate
  4 bits rotation, 8 bits value
  check whether the imm is valid
*/
bool Operand::immCheck(int imm) {
  if ((imm & ~0xff) == 0 || (imm & ~0xc000003f) == 0 || (imm & ~0xf000000f) == 0
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

// for VMOV
/*
  float type immediate check
  arm floating-poing process instruction use a a 8-bit imm to represent IEEE-754
  32-bit value from 8-bit: abcd efgh -> 32-bit: aBbbbbbc defgh000 00000000
  00000000 (B = not(b)) so the valid constant for imm is:

  imm = (-1)^S * 2^exp * mantissa
        S = a,
        exp = Bcd - 3(equals Bbbbbbcd - 127),
        mantissa = 1.efgh

  so imm = (+/-)(bin)(1.0000 ~ 1.1111) * 2^(-3 ~ 4)
  or imm = (+/-)(int)(16 ~ 31) * 2^(-7 ~ 0)
*/
bool Operand::immCheck(float imm) {
  // fast check the lower bound and upper bound
  if (imm < 0.125 || imm > 31) return false;
  int i = 0;
  while (i < 8) {
    if (imm >= 16 && imm <= 31) {
      return std::ceil(imm) == std::floor(imm);
    }
    imm *= 2;
  }
  return false;
}

// check if the offset value is valid
bool Operand::addrOffsCheck(int offs, bool is_float) {
  if (is_float) {
    // VLDR and VSTR
    // Values are multiples of 4 in the range 0-1020.(+/-)
    return -1020 <= offs && offs <= 1020 && offs % 4 == 0;
  } else {
    // LDR and STR
    // Any value in the range 0-4095 is permitted.(+/-)
    return -4095 <= offs && offs <= 4095;
  }
}

std::string Operand::getName() {
  switch (m_op_type) {
    case OperandType::IMM:
      if (m_is_float) return ("#" + std::to_string(m_float_val));
      return ("#" + std::to_string(m_int_val));
    case OperandType::REG:
      return getRegName();
    case OperandType::VREG:
      return getVRegName();
    case OperandType::SPECIAL_REG:
      return m_special_reg;
  }
  return "";  // unreachable, just write to avoid warning
}

std::string Operand::getRegName() {
  // rreg
  if (!m_is_float) {
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
  std::string name = m_is_float ? "VS" + std::to_string(vsreg_cnt++)
                                : "VR" + std::to_string(vrreg_cnt++);
  vreg_map.insert(std::make_pair(_this, name));
  return name;
}

RegType Operand::getRegType() {
  if (m_is_float)
    return RegType::S;
  else
    return RegType::R;
}

std::shared_ptr<Operand> Operand::getRReg(RReg r) {
  if (Operand::rreg_map.find(r) != Operand::rreg_map.end()) {
    return Operand::rreg_map[r];
  }
  auto ret = std::make_shared<Operand>(OperandType::REG);
  ret->m_rreg = r;
  Operand::rreg_map[r] = ret;
  return ret;
}

std::shared_ptr<Operand> Operand::getSReg(SReg s) {
  if (Operand::sreg_map.find(s) != Operand::sreg_map.end()) {
    return Operand::sreg_map[s];
  }
  auto ret = std::make_shared<Operand>(OperandType::REG);
  ret->m_sreg = s;
  Operand::sreg_map[s] = ret;
  ret->m_is_float = true;
  return ret;
}

Shift::Shift(ShiftType t, int v) : s_type(t), s_val(v) {
  if (t != ShiftType::RRX) {
    int upbound = 31;
    if (t == ShiftType::ASR || t == ShiftType::LSR) upbound++;
    assert(1 <= v && v <= upbound);
  }
}

std::string Shift::getShiftName() {
  switch (s_type) {
    case ShiftType::ASR:
      return "ASR";
    case ShiftType::LSL:
      return "LSL";
    case ShiftType::LSR:
      return "LSR";
    case ShiftType::ROR:
      return "ROR";
    case ShiftType::RRX:
      return "RRX";
  }
  assert(false);
  return "";
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
    case InstOp::MSR:
      return "MSR";
    case InstOp::MRS:
      return "MRS";
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
    case InstOp::VNEG:
      return "VNEG";
    case InstOp::VCMP:
      return "VCMP";
    case InstOp::VMSR:
      return "VMSR";
    case InstOp::VMRS:
      return "VMRS";
    case InstOp::VLDR:
      return "VLDR";
    case InstOp::VSTR:
      return "VSTR";
    case InstOp::VPUSH:
      return "VPUSH";
    case InstOp::VPOP:
      return "VPOP";
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
    case InstOp::VNEG:
    case InstOp::VCMP:
      return ".F32";
    case InstOp::VLDR:
    case InstOp::VSTR:
      return ".32";
    default:
      break;
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
    default:
      break;
  }
  return "";
}

CondType ASM_Instruction::getOppositeCond(CondType cond) {
  switch (cond) {
    case CondType::EQ:
      return CondType::NE;
    case CondType::NE:
      return CondType::EQ;
    case CondType::LT:
      return CondType::GE;
    case CondType::LE:
      return CondType::GT;
    case CondType::GT:
      return CondType::LE;
    case CondType::GE:
      return CondType::LT;
    case CondType::NONE:
      return CondType::NONE;
  }
  return CondType::NONE;
}

LDRInst::LDRInst(std::shared_ptr<Operand> dest, std::string label) {
  if (dest->m_is_float)
    m_op = InstOp::VLDR;
  else
    m_op = InstOp::LDR;
  m_cond = CondType::NONE;
  m_type = Type::LABEL;
  m_dest = dest;
  m_label = label;

  addDef(dest);
}

LDRInst::LDRInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src,
                 std::shared_ptr<Operand> offs) {
  if (dest->m_is_float)
    m_op = InstOp::VLDR;
  else
    m_op = InstOp::LDR;
  m_cond = CondType::NONE;
  m_type = Type::REG;
  m_dest = dest;
  m_src = src;
  m_offs = offs;

  addDef(dest);
  addUse(src);
  addUse(offs);
}

STRInst::STRInst(std::shared_ptr<Operand> src, std::shared_ptr<Operand> dest,
                 std::shared_ptr<Operand> offs) {
  if (src->m_is_float)
    m_op = InstOp::VSTR;
  else
    m_op = InstOp::STR;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_src = src;
  m_offs = offs;

  addUse(src);
  addUse(dest);
  addUse(offs);
}

MOVInst::MOVInst(std::shared_ptr<Operand> dest, int imm) {
  m_op = InstOp::MOV;
  m_cond = CondType::NONE;
  m_type = MOVType::IMM;
  m_dest = dest;
  m_src = std::make_shared<Operand>(imm);

  addDef(dest);
}

MOVInst::MOVInst(std::shared_ptr<Operand> dest, float imm) {
  m_op = InstOp::VMOV;
  m_cond = CondType::NONE;
  m_type = MOVType::IMM;
  m_dest = dest;
  m_src = std::make_shared<Operand>(imm);

  addDef(dest);
}

MOVInst::MOVInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src) {
  if (dest->m_is_float || src->m_is_float)
    m_op = InstOp::VMOV;
  else
    m_op = InstOp::MOV;
  m_cond = CondType::NONE;
  m_type = src->m_op_type == OperandType::IMM ? MOVType::IMM : MOVType::REG;
  m_dest = dest;
  m_src = src;

  if (m_type == MOVType::REG) m_is_mov = true;

  addDef(dest);
  addUse(src);
}

MRSInst::MRSInst(std::string reg, std::shared_ptr<Operand> src) {
  if (reg == "APSR") {
    m_op = InstOp::MSR;
    m_dest = std::make_shared<Operand>("APSR_NZCVQG");
  } else if (reg == "FPSCR") {
    m_op = InstOp::VMSR;
    m_dest = std::make_shared<Operand>("FPSCR");
  } else
    assert(false);

  m_cond = CondType::NONE;
  m_src = src;

  addUse(m_src);
}

MRSInst::MRSInst(std::shared_ptr<Operand> dest, std::string reg) {
  if (reg == "APSR") {
    m_op = InstOp::MRS;
    m_src = std::make_shared<Operand>("APSR");
  } else if (reg == "FPSCR") {
    m_op = InstOp::VMRS;
    m_src = std::make_shared<Operand>("FPSCR");
  } else
    assert(false);

  m_cond = CondType::NONE;
  m_dest = dest;

  addDef(m_dest);
}

PInst::PInst(InstOp op) {
  m_op = op;
  m_cond = CondType::NONE;
  if (op == InstOp::PUSH)
    m_regs.insert(Operand::getRReg(RReg::LR));
  else if (op == InstOp::POP)
    m_regs.insert(Operand::getRReg(RReg::PC));
  else if (op != InstOp::VPUSH && op != InstOp::VPOP)
    assert(false);
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

  // BL instruction will change LR and R12
  addDef(Operand::getRReg(RReg::R0));
  addDef(Operand::getRReg(RReg::R1));
  addDef(Operand::getRReg(RReg::R2));
  addDef(Operand::getRReg(RReg::R3));
  addDef(Operand::getRReg(RReg::R12));
  addDef(Operand::getRReg(RReg::LR));

  for (int i = 0; i < 16; i++) {
    addDef(Operand::getSReg((SReg)i));
  }

  if (label == "putfloat") {
    addUse(Operand::getSReg(SReg::S0));
    return;
  }

  int i = 0;
  while (i < 4 && i < n) {
    addUse(Operand::getRReg((RReg)i));
    i++;
  }
}

ShiftInst::ShiftInst(InstOp op, std::shared_ptr<Operand> dest,
                     std::shared_ptr<Operand> src,
                     std::shared_ptr<Operand> sval) {
  m_op = op;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_src = src;
  m_sval = sval;

  addDef(dest);
  addUse(src);
  addUse(sval);
}

ASInst::ASInst(InstOp op, std::shared_ptr<Operand> dest,
               std::shared_ptr<Operand> operand1,
               std::shared_ptr<Operand> operand2) {
  m_op = op;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_operand1 = operand1;
  m_operand2 = operand2;

  addDef(dest);
  addUse(operand1);
  addUse(operand2);
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

  addDef(dest);
  addUse(operand1);
  addUse(operand2);
  if (m_append) addUse(append);
}

SDIVInst::SDIVInst(std::shared_ptr<Operand> dest,
                   std::shared_ptr<Operand> devidend,
                   std::shared_ptr<Operand> devisor) {
  m_op = InstOp::SDIV;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_devidend = devidend;
  m_devisor = devisor;

  addDef(dest);
  addUse(devidend);
  addUse(devisor);
}

BITInst::BITInst(InstOp op, std::shared_ptr<Operand> dest,
                 std::shared_ptr<Operand> operand1,
                 std::shared_ptr<Operand> operand2) {
  m_op = op;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_operand1 = operand1;
  m_operand2 = operand2;

  addDef(dest);
  addUse(operand1);
  addUse(operand2);
}

BITInst::BITInst(InstOp op, std::shared_ptr<Operand> dest,
                 std::shared_ptr<Operand> operand1) {
  m_op = op;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_operand1 = operand1;

  addDef(dest);
  addUse(operand1);
}

CTInst::CTInst(InstOp op, std::shared_ptr<Operand> operand1,
               std::shared_ptr<Operand> operand2) {
  m_op = op;
  m_cond = CondType::NONE;
  m_operand1 = operand1;
  m_operand2 = operand2;

  addUse(operand1);
  addUse(operand2);
}

VNEGInst::VNEGInst(std::shared_ptr<Operand> dest,
                   std::shared_ptr<Operand> operand) {
  m_op = InstOp::VNEG;
  m_cond = CondType::NONE;
  m_dest = dest;
  m_operand = operand;

  addDef(dest);
  addUse(operand);
}