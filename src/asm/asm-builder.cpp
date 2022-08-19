#include "asm/asm-builder.h"

ASM_Builder::ASM_Builder(std::shared_ptr<ASM_Module> m) : m_module(m) {}

void ASM_Builder::init() {
  m_value_map.clear();
  m_block_map.clear();
  m_load_map.clear();
}

void ASM_Builder::setIrModule(std::shared_ptr<Module> ir_module) {
  m_module->m_ir_module = ir_module;
}

void ASM_Builder::appendFunction(std::shared_ptr<ASM_Function> func) {
  m_module->m_funcs.push_back(func);
  setCurFunction(func);
  init();
  setParams();
}

void ASM_Builder::setCurFunction(std::shared_ptr<ASM_Function> func) {
  m_cur_func = func;
}

void ASM_Builder::setParams() {
  int i = 0;
  int n = m_cur_func->m_ir_func->m_args.size();
  m_cur_func->m_params_size = n;

  // set params in r0 ~ r3
  while (i < 4 && i < n) {
    auto value = m_cur_func->m_ir_func->m_args[i];
    bool is_float = value->m_type.IsBasicFloat();
    auto ret = std::make_shared<Operand>(OperandType::VREG, is_float);
    auto mov = std::make_shared<MOVInst>(ret, Operand::getRReg((RReg)i));
    mov->addDef(Operand::getRReg(RReg::R12));
    m_cur_func->m_params_set_list.push_back(mov);
    m_value_map.insert(std::make_pair(value, ret));
    i++;
  }

  // set params in stack
  while (i < n) {
    auto value = m_cur_func->m_ir_func->m_args[i];
    bool is_float = value->m_type.IsBasicFloat();
    int fp_offs = (i - 4) * 4;
    auto ret = std::make_shared<Operand>(OperandType::VREG, is_float);
    auto ldr = std::make_shared<LDRInst>(ret, Operand::getRReg(RReg::SP),
                                         std::make_shared<Operand>(fp_offs));
    ldr->addDef(Operand::getRReg(RReg::R12));
    m_cur_func->m_params_set_list.push_back(ldr);
    m_cur_func->m_stack_params_offs[ret] = fp_offs;
    m_value_map.insert(std::make_pair(value, ret));
    i++;
  }
}

void ASM_Builder::appendBlock(std::shared_ptr<ASM_BasicBlock> block) {
  m_cur_func->m_blocks.push_back(block);
  setCurBlock(block);
  m_load_map.clear();
}

void ASM_Builder::setCurBlock(std::shared_ptr<ASM_BasicBlock> block) {
  m_cur_block = block;
}

void ASM_Builder::allocSP(int size) {
  if (size & 7) {
    size &= ~(unsigned int)7;
    size += 8;
  }
  std::shared_ptr<Operand> offs;
  if (Operand::immCheck(size)) {
    offs = std::make_shared<Operand>(size);
  } else {
    offs = std::make_shared<Operand>(OperandType::VREG);
    appendMOV(offs, size);
  }
  auto sp = Operand::getRReg(RReg::SP);
  appendAS(InstOp::SUB, sp, sp, offs);
  m_cur_func->m_sp_alloc_size.push(size);
}

void ASM_Builder::reclaimSP() {
  int size = m_cur_func->m_sp_alloc_size.top();
  m_cur_func->m_sp_alloc_size.pop();
  std::shared_ptr<Operand> offs;
  if (Operand::immCheck(size)) {
    offs = std::make_shared<Operand>(size);
  } else {
    offs = std::make_shared<Operand>(OperandType::VREG);
    appendMOV(offs, size);
  }
  auto sp = Operand::getRReg(RReg::SP);
  appendAS(InstOp::ADD, sp, sp, offs);
}

std::shared_ptr<Operand> ASM_Builder::getOperand(
    std::shared_ptr<Value> value, bool genimm, bool checkimm,
    std::shared_ptr<ASM_BasicBlock> phi_block) {
  if (m_value_map.find(value) != m_value_map.end()) {
    return m_value_map[value];
  }
  if (auto val = std::dynamic_pointer_cast<Constant>(value)) {
    return GenerateConstant(val, genimm, checkimm, phi_block);
  }
  return createOperand(value);
}

std::shared_ptr<Operand> ASM_Builder::createOperand(
    std::shared_ptr<Value> value) {
  auto is_float = value->m_type.IsBasicFloat();
  auto ret = std::make_shared<Operand>(OperandType::VREG, is_float);
  m_value_map.insert(std::make_pair(value, ret));
  return ret;
}

std::shared_ptr<ASM_BasicBlock> ASM_Builder::getBlock(
    std::shared_ptr<BasicBlock> ir_block) {
  auto ret = m_block_map.find(ir_block) != m_block_map.end()
                 ? m_block_map[ir_block]
                 : nullptr;
  if (!ret) {
    ret = std::make_shared<ASM_BasicBlock>(ir_block->m_loop_depth);
    m_block_map.insert(std::make_pair(ir_block, ret));
  }
  return ret;
}

// appendLDR
std::shared_ptr<LDRInst> ASM_Builder::appendLDR(std::shared_ptr<Operand> dest,
                                                std::string label) {
  auto ldr = std::make_shared<LDRInst>(dest, label);
  m_cur_block->insert(ldr);
  return ldr;
}

std::shared_ptr<LDRInst> ASM_Builder::appendLDR(std::shared_ptr<Operand> dest,
                                                std::shared_ptr<Operand> src,
                                                std::shared_ptr<Operand> offs) {
  auto ldr = std::make_shared<LDRInst>(dest, src, offs);
  m_cur_block->insert(ldr);
  return ldr;
}

// appendSTR

std::shared_ptr<STRInst> ASM_Builder::appendSTR(std::shared_ptr<Operand> src,
                                                std::shared_ptr<Operand> dest,
                                                std::shared_ptr<Operand> offs) {
  auto str = std::make_shared<STRInst>(src, dest, offs);
  m_cur_block->insert(str);
  return str;
}

// appendMOV
std::shared_ptr<MOVInst> ASM_Builder::appendMOV(
    std::shared_ptr<Operand> dest, int imm,
    std::shared_ptr<ASM_BasicBlock> phi_block) {
  auto mov = std::make_shared<MOVInst>(dest, imm);
  if (phi_block)
    phi_block->insertPhiMOV(mov);
  else
  m_cur_block->insert(mov);
  return mov;
}

std::shared_ptr<MOVInst> ASM_Builder::appendMOV(
    std::shared_ptr<Operand> dest, float imm,
    std::shared_ptr<ASM_BasicBlock> phi_block) {
  std::shared_ptr<MOVInst> mov;
  if (Operand::immCheck(imm)) {
    mov = std::make_shared<MOVInst>(dest, imm);
    if (phi_block)
      phi_block->insertPhiMOV(mov);
    else
    m_cur_block->insert(mov);
  } else {
    int temp = *(int*)&imm;
    auto mov_temp = std::make_shared<MOVInst>(
        std::make_shared<Operand>(OperandType::VREG), temp);
    auto mov = std::make_shared<MOVInst>(dest, mov_temp->m_dest);
    if (phi_block) {
      phi_block->insertPhiMOV(mov_temp);
      phi_block->insertPhiMOV(mov);
    } else {
    m_cur_block->insert(mov_temp);
    m_cur_block->insert(mov);
    }
  }
  return mov;
}

std::shared_ptr<MOVInst> ASM_Builder::appendMOV(std::shared_ptr<Operand> dest,
                                                std::shared_ptr<Operand> src) {
  auto mov = std::make_shared<MOVInst>(dest, src);
  m_cur_block->insert(mov);
  return mov;
}

// appendMRS
std::shared_ptr<MRSInst> ASM_Builder::appendMRS(std::string reg,
                                                std::shared_ptr<Operand> src) {
  auto msr = std::make_shared<MRSInst>(reg, src);
  m_cur_block->insert(msr);
  return msr;
}

std::shared_ptr<MRSInst> ASM_Builder::appendMRS(std::shared_ptr<Operand> dest,
                                                std::string reg) {
  auto mrs = std::make_shared<MRSInst>(dest, reg);
  m_cur_block->insert(mrs);
  return mrs;
}

// appendB
std::shared_ptr<BInst> ASM_Builder::appendB(
    std::shared_ptr<ASM_BasicBlock> block, CondType cond) {
  auto b = std::make_shared<BInst>(block);
  b->m_cond = cond;
  m_cur_block->insert(b);
  m_cur_block->appendSuccessor(block);
  block->appendPredecessor(m_cur_block);
  return b;
}

// appendCALL
std::shared_ptr<CALLInst> ASM_Builder::appendCALL(VarType type,
                                                  std::string label, int n) {
  auto call = std::make_shared<CALLInst>(type, label, n);
  m_cur_block->insert(call);
  return call;
}

// appendShift
std::shared_ptr<ShiftInst> ASM_Builder::appendShift(
    InstOp op, std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src,
    std::shared_ptr<Operand> sval) {
  auto shift = std::make_shared<ShiftInst>(op, dest, src, sval);
  m_cur_block->insert(shift);
  return shift;
}

// appendAS
std::shared_ptr<ASInst> ASM_Builder::appendAS(
    InstOp op, std::shared_ptr<Operand> dest, std::shared_ptr<Operand> operand1,
    std::shared_ptr<Operand> operand2) {
  auto as = std::make_shared<ASInst>(op, dest, operand1, operand2);
  m_cur_block->insert(as);
  return as;
}

// appendMUL
std::shared_ptr<MULInst> ASM_Builder::appendMUL(
    InstOp op, std::shared_ptr<Operand> dest, std::shared_ptr<Operand> operand1,
    std::shared_ptr<Operand> operand2) {
  auto mul = std::make_shared<MULInst>(op, dest, operand1, operand2);
  m_cur_block->insert(mul);
  return mul;
}

std::shared_ptr<MULInst> ASM_Builder::appendMUL(
    InstOp op, std::shared_ptr<Operand> dest, std::shared_ptr<Operand> operand1,
    std::shared_ptr<Operand> operand2, std::shared_ptr<Operand> append) {
  auto mul = std::make_shared<MULInst>(op, dest, operand1, operand2, append);
  m_cur_block->insert(mul);
  return mul;
}

// appendSDIV
std::shared_ptr<SDIVInst> ASM_Builder::appendSDIV(
    std::shared_ptr<Operand> dest, std::shared_ptr<Operand> devidend,
    std::shared_ptr<Operand> devisor) {
  auto sdiv = std::make_shared<SDIVInst>(dest, devidend, devisor);
  m_cur_block->insert(sdiv);
  return sdiv;
}

// appendBIT
std::shared_ptr<BITInst> ASM_Builder::appendBIT(
    InstOp op, std::shared_ptr<Operand> dest, std::shared_ptr<Operand> operand1,
    std::shared_ptr<Operand> operand2) {
  auto bit = std::make_shared<BITInst>(op, dest, operand1, operand2);
  m_cur_block->insert(bit);
  return bit;
}

std::shared_ptr<BITInst> ASM_Builder::appendBIT(
    InstOp op, std::shared_ptr<Operand> dest,
    std::shared_ptr<Operand> operand1) {
  auto bit = std::make_shared<BITInst>(op, dest, operand1);
  m_cur_block->insert(bit);
  return bit;
}

// appendCT
std::shared_ptr<CTInst> ASM_Builder::appendCT(
    InstOp op, std::shared_ptr<Operand> operand1,
    std::shared_ptr<Operand> operand2) {
  auto ct = std::make_shared<CTInst>(op, operand1, operand2);
  m_cur_block->insert(ct);
  if (op == InstOp::VCMP) {
    auto temp_reg = std::make_shared<Operand>(OperandType::VREG);
    appendMRS(temp_reg, "FPSCR");
    m_cur_block->m_status_load_inst
        = std::make_shared<MRSInst>("APSR", temp_reg);
  }
  return ct;
}

std::shared_ptr<VNEGInst> ASM_Builder::appendVNEG(
    std::shared_ptr<Operand> dest, std::shared_ptr<Operand> operand) {
  auto vneg = std::make_shared<VNEGInst>(dest, operand);
  m_cur_block->insert(vneg);
  return vneg;
}