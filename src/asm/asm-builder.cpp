#include "asm/asm-builder.h"

ASM_Builder::ASM_Builder(std::shared_ptr<ASM_Module> m) : m_module(m) {}

void ASM_Builder::init() {
  m_value_map.clear();
  m_block_map.clear();
  m_filled_block.clear();
}

void ASM_Builder::setIrModule(std::shared_ptr<Module> ir_module) {
  m_module->m_ir_module = ir_module;
}

void ASM_Builder::appendFunction(std::shared_ptr<ASM_Function> func) {
  m_module->m_funcs.push_back(func);
  setCurFunction(func);

  // TODO(Huang): initialize
  init();
}

void ASM_Builder::setCurFunction(std::shared_ptr<ASM_Function> func) {
  m_cur_func = func;
}

void ASM_Builder::appendBlock(std::shared_ptr<ASM_BasicBlock> block) {
  m_cur_func->m_blocks.push_back(block);
  setCurBlock(block);
}

void ASM_Builder::setCurBlock(std::shared_ptr<ASM_BasicBlock> block) {
  m_cur_block = block;
}

std::shared_ptr<Operand> ASM_Builder::getOperand(std::shared_ptr<Value> value) {
  auto ret = m_value_map.find(value) != m_value_map.end() ? m_value_map[value]
                                                          : nullptr;
  if (ret) {
    return ret;
  }
  if ((std::dynamic_pointer_cast<Constant>(value) != nullptr)) {
    auto val = std::dynamic_pointer_cast<Constant>(value);
    ret = appendMOV(std::make_shared<Operand>(OperandType::VREG),
                    val->m_int_val)
              ->m_dest;
  }
  return ret;
}

std::shared_ptr<Operand> ASM_Builder::createOperand(
    std::shared_ptr<Value> value) {
  return nullptr;
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