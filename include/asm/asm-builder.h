#ifndef BDDD_ASM_BUILDER_H
#define BDDD_ASM_BUILDER_H

#include <unordered_map>

#include "asm/asm.h"

class ASM_Builder {
public:
  std::shared_ptr<ASM_Module> m_module;
  std::shared_ptr<ASM_Function> m_cur_func;
  std::shared_ptr<ASM_BasicBlock> m_cur_block;

  std::unordered_map<std::shared_ptr<Value>, std::shared_ptr<Operand>>
      m_value_map;
  std::unordered_map<std::shared_ptr<BasicBlock>,
                     std::shared_ptr<ASM_BasicBlock>>
      m_block_map;

  ASM_Builder(std::shared_ptr<ASM_Module> m);

  void setIrModule(std::shared_ptr<Module> ir_module);

  void appendFunction(std::shared_ptr<ASM_Function> func);

  void setCurFunction(std::shared_ptr<ASM_Function> func);

  void appendBlock(std::shared_ptr<ASM_BasicBlock> block);

  void setCurBlock(std::shared_ptr<ASM_BasicBlock> block);

  std::shared_ptr<Operand> getOperand(std::shared_ptr<Value> value);

  std::shared_ptr<Operand> createOperand(std::shared_ptr<Value> value);

  // appendLDR
  std::shared_ptr<LDRInst> appendLDR(std::shared_ptr<Operand> dest,
                                     std::string label);

  std::shared_ptr<LDRInst> appendLDR(std::shared_ptr<Operand> dest,
                                     std::shared_ptr<Operand> src, int imm);

  std::shared_ptr<LDRInst> appendLDR(std::shared_ptr<Operand> dest,
                                     std::shared_ptr<Operand> src,
                                     std::shared_ptr<Operand> offs);

  // appendSTR
  std::shared_ptr<STRInst> appendSTR(std::shared_ptr<Operand> src,
                                     std::shared_ptr<Operand> dest, int imm);

  std::shared_ptr<STRInst> appendSTR(std::shared_ptr<Operand> src,
                                     std::shared_ptr<Operand> dest,
                                     std::shared_ptr<Operand> offs);

  // appendMOV
  std::shared_ptr<MOVInst> appendMOV(std::shared_ptr<Operand> dest, int imm);

  std::shared_ptr<MOVInst> appendMOV(std::shared_ptr<Operand> dest,
                                     std::shared_ptr<Operand> src);


  void appendASInst(std::shared_ptr<Instruction> ir_inst);
};

#endif  // BDDD_ASM_BUILDER_H
