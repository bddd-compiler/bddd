#ifndef BDDD_ASM_BUILDER_H
#define BDDD_ASM_BUILDER_H

#include "asm/asm.h"

class ASM_Builder {
public:
  std::shared_ptr<ASM_Module> m_module;
  std::shared_ptr<ASM_Function> m_cur_func;
  std::shared_ptr<ASM_BasicBlock> m_cur_block;

  ASM_Builder(std::shared_ptr<ASM_Module> m);

  void setIrModule(std::shared_ptr<Module> ir_module);

  void appendFunction(std::shared_ptr<ASM_Function> func);

  void setCurFunction(std::shared_ptr<ASM_Function> func);

  void appendBlock(std::shared_ptr<ASM_BasicBlock> block);

  void setCurBlock(std::shared_ptr<ASM_BasicBlock> block);
};

#endif  // BDDD_ASM_BUILDER_H
