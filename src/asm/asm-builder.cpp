#include "asm/asm-builder.h"

ASM_Builder::ASM_Builder(std::shared_ptr<ASM_Module> m) : m_module(m) {}

void ASM_Builder::init() {
  m_value_map.clear();
  m_block_map.clear();
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
  std::shared_ptr<Value> value;

  // set params in r0 ~ r3
  // TODO(Huang): store r0 if there is a call(BL) in this function
  std::shared_ptr<Operand> reg;
  while (i < 4 && i < n) {
    value = m_cur_func->m_ir_func->m_args[i];
    reg = Operand::getRReg((RReg)i);
    m_value_map.insert(std::make_pair(value, reg));
    i++;
  }

  // set params in stack
#ifndef SP_FOR_PARAM
  // use r11 as the frame pointer register
  while (i < n) {
    value = m_cur_func->m_ir_func->m_args[i];
    std::shared_ptr<Operand> offs;
    int fp_offs = (i - 4) * 4;
    if (0 <= fp_offs && fp_offs < 4096) {
      offs = std::make_shared<Operand>(fp_offs);
    } else {
      auto mov = std::make_shared<MOVInst>(
          std::make_shared<Operand>(OperandType::VREG), fp_offs);
      offs = mov->m_dest;
      m_cur_func->m_params_set_list.push_back(mov);
    }
    auto ldr = std::make_shared<LDRInst>(getOperand(value),
                                         Operand::getRReg(RReg::R11), offs);
    m_cur_func->m_params_set_list.push_back(ldr);
    i++;
  }
  if (n > 4) {
    m_cur_func->m_push->m_regs.insert(Operand::getRReg(RReg::R11));
    m_cur_func->m_pop->m_regs.insert(Operand::getRReg(RReg::R11));
  }
#else
  while (i < n) {
    value = m_cur_func->m_ir_func->m_args[i];
    auto ret = std::make_shared<Operand>(OperandType::VREG);
    m_value_map.insert(std::make_pair(value, ret));
    i++;
  }
#endif
}

#ifdef SP_FOR_PARAM
void ASM_Builder::fixedStackParams() {
  // fixed offset of params in stack
  int n = m_cur_func->m_ir_func->m_args.size();
  std::shared_ptr<Value> value;
  std::shared_ptr<Operand> sp = Operand::getRReg(RReg::SP);
  for (int i = 4; i < n; i++) {
    value = m_cur_func->m_ir_func->m_args[i];

    // maybe wrong if push/pop is changed
    int sp_offs = (i - 3 + m_cur_func->m_push->m_regs.size()) * 4;
    std::shared_ptr<Operand> offs;
    if (0 <= sp_offs && sp_offs < 4096) {
      offs = std::make_shared<Operand>(sp_offs);
    } else {
      auto mov = std::make_shared<MOVInst>(
          std::make_shared<Operand>(OperandType::VREG), sp_offs);
      offs = mov->m_dest;
      m_cur_func->m_params_set_list.push_back(mov);
    }
    auto ldr = std::make_shared<LDRInst>(getOperand(value), sp, offs);
    m_cur_func->m_params_set_list.push_back(ldr);
  }
}
#endif

void ASM_Builder::appendBlock(std::shared_ptr<ASM_BasicBlock> block) {
  m_cur_func->m_blocks.push_back(block);
  setCurBlock(block);
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
    offs
        = appendMOV(std::make_shared<Operand>(OperandType::VREG), size)->m_dest;
  }
  auto sp = Operand::getRReg(RReg::SP);
  appendAS(InstOp::SUB, sp, sp, offs);
  m_cur_func->m_sp_alloc_size.push(offs);
}

void ASM_Builder::reclaimSP() {
  auto sp = Operand::getRReg(RReg::SP);
  appendAS(InstOp::ADD, sp, sp, m_cur_func->m_sp_alloc_size.top());
  m_cur_func->m_sp_alloc_size.pop();
}

std::shared_ptr<Operand> ASM_Builder::getOperand(
    std::shared_ptr<Value> value, bool genimm,
    std::shared_ptr<ASM_BasicBlock> block) {
  if (auto val = std::dynamic_pointer_cast<Constant>(value)) {
    return GenerateConstant(val, genimm, block);
  }
  auto ret = m_value_map.find(value) != m_value_map.end() ? m_value_map[value]
                                                          : nullptr;
  if (ret) {
    return ret;
  }
  return createOperand(value);
}

std::shared_ptr<Operand> ASM_Builder::createOperand(
    std::shared_ptr<Value> value) {
  auto ret = std::make_shared<Operand>(OperandType::VREG);
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
std::shared_ptr<MOVInst> ASM_Builder::appendMOV(std::shared_ptr<Operand> dest,
                                                int imm) {
  auto mov = std::make_shared<MOVInst>(dest, imm);
  m_cur_block->insert(mov);
  return mov;
}

std::shared_ptr<MOVInst> ASM_Builder::appendMOV(std::shared_ptr<Operand> dest,
                                                std::shared_ptr<Operand> src) {
  auto mov = std::make_shared<MOVInst>(dest, src);
  m_cur_block->insert(mov);
  return mov;
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
  return ct;
}