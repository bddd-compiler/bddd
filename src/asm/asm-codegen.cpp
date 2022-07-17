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

void GenerateBasicblock(std::shared_ptr<BasicBlock> ir_block,
                        std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_BasicBlock> block
      = std::make_shared<ASM_BasicBlock>(ir_block);
  builder->appendBlock(block);
  for (auto &i : ir_block->GetInstList()) {
    GenerateInstruction(i, builder);
  }
}

void GenerateInstruction(std::shared_ptr<Value> ir_value,
                         std::shared_ptr<ASM_Builder> builder) {
  if ((std::dynamic_pointer_cast<Constant>(ir_value)) != nullptr) {
    auto value = std::dynamic_pointer_cast<Constant>(ir_value);
    GenerateConstant(value, builder);
    // TODO(Huang): Generate Constant
  }
  else if ((std::dynamic_pointer_cast<Constant>(ir_value)) != nullptr) {
    // TODO(Huang): Generate Constant
  }
  else if ((std::dynamic_pointer_cast<Constant>(ir_value)) != nullptr) {
    // TODO(Huang): Generate Constant
  }
  else if ((std::dynamic_pointer_cast<Constant>(ir_value)) != nullptr) {
    // TODO(Huang): Generate Constant
  }
  else if ((std::dynamic_pointer_cast<Constant>(ir_value)) != nullptr) {
    // TODO(Huang): Generate Constant
  }
  else if ((std::dynamic_pointer_cast<Constant>(ir_value)) != nullptr) {
    // TODO(Huang): Generate Constant
  }
  else if ((std::dynamic_pointer_cast<Constant>(ir_value)) != nullptr) {
    // TODO(Huang): Generate Constant
  }
  else if ((std::dynamic_pointer_cast<Constant>(ir_value)) != nullptr) {
    // TODO(Huang): Generate Constant
  }
}

std::shared_ptr<Operand> GenerateConstant(std::shared_ptr<Constant> value, std::shared_ptr<ASM_Builder> builder) {
    std::shared_ptr<Operand> operand = builder->getOperand(value);
    if (operand == nullptr) {
        
    }
}