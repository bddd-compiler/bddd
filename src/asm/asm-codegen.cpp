#include "asm/asm-builder.h"
#include "ir/ir.h"

void GenerateModule(std::shared_ptr<Module> ir_module,
                    std::shared_ptr<ASM_Builder> builder) {
  builder->setIrModule(ir_module);
  for (auto &f : ir_module->m_function_list) {
    GenerateFunction(f, builder);
  }
}

void GenerateFunction(std::shared_ptr<Function> ir_func,
                      std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_Function> func = std::make_shared<ASM_Function>(ir_func);
  builder->appendFunction(func);
  for (auto &b : ir_func->GetBlockList()) {
    GenerateBasicblock(b, builder);
  }
}

void GenerateBasicblock(std::shared_ptr<BasicBlock> ir_block, std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_BasicBlock> block = std::make_shared<ASM_BasicBlock>(ir_block);
  builder->appendBlock(block);
  for (auto &i : ir_block->GetInstList()) {

  }
}

void GenerateInstruction(std::shared_ptr<Instruction> ir_inst, std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_Instruction> inst = std::make_shared<ASM_Instruction>(ir_inst);
  
}