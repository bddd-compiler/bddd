#include "asm/asm-builder.h"
#include "ir/ir.h"

std::shared_ptr<Operand> GenerateConstant(
    std::shared_ptr<Constant> value, std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Operand> ret = builder->getOperand(value);
  if (ret == nullptr) {
    if (value->m_is_float) {
      // TODO(Huang): Generate float constant
    } else {
      auto mov = builder->appendMOV(
          std::make_shared<Operand>(OperandType::VREG), value->m_int_val);
      ret = mov->m_dest;
    }
  }
  return ret;
}

std::shared_ptr<Operand> GenerateAddInstruction(
    std::shared_ptr<BinaryInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Operand> operand1, operand2;
  std::shared_ptr<Value> val1 = inst->m_lhs_val_use.m_value;
  std::shared_ptr<Value> val2 = inst->m_rhs_val_use.m_value;
  bool is_const1 = (std::dynamic_pointer_cast<Constant>(val1) != nullptr);
  bool is_const2 = (std::dynamic_pointer_cast<Constant>(val2) != nullptr);

  operand1 = is_const1 ? std::make_shared<Operand>(
                 std::dynamic_pointer_cast<Constant>(val1)->m_int_val)
                       : builder->getOperand(val1);
  operand2 = is_const2 ? std::make_shared<Operand>(
                 std::dynamic_pointer_cast<Constant>(val2)->m_int_val)
                       : builder->getOperand(val2);

  auto res = builder
                 ->appendAS(InstOp::ADD,
                            std::make_shared<Operand>(OperandType::VREG),
                            operand1, operand2)
                 ->m_dest;
  builder->m_value_map.insert(std::make_pair(inst, res));
  return res;
}

std::shared_ptr<Operand> GenerateSubInstruction(
    std::shared_ptr<BinaryInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Operand> operand1, operand2;
  std::shared_ptr<Value> val1 = inst->m_lhs_val_use.m_value;
  std::shared_ptr<Value> val2 = inst->m_rhs_val_use.m_value;
  bool is_const1 = (std::dynamic_pointer_cast<Constant>(val1) != nullptr);
  bool is_const2 = (std::dynamic_pointer_cast<Constant>(val2) != nullptr);

  InstOp op;
  if (is_const1) {
    op = InstOp::RSB;
    operand2 = std::make_shared<Operand>(
        std::dynamic_pointer_cast<Constant>(val1)->m_int_val);
    operand1 = builder->getOperand(val2);
  } else {
    op = InstOp::SUB;
    operand1 = builder->getOperand(val1);
    operand2 = is_const2 ? std::make_shared<Operand>(
                   std::dynamic_pointer_cast<Constant>(val2)->m_int_val)
                         : builder->getOperand(val2);
  }

  auto res = builder
                 ->appendAS(op, std::make_shared<Operand>(OperandType::VREG),
                            operand1, operand2)
                 ->m_dest;
  builder->m_value_map.insert(std::make_pair(inst, res));
  return res;
}

std::shared_ptr<Operand> GenerateMulInstruction(
    std::shared_ptr<BinaryInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Operand> operand1, operand2;
  std::shared_ptr<Value> val1 = inst->m_lhs_val_use.m_value;
  std::shared_ptr<Value> val2 = inst->m_rhs_val_use.m_value;
  operand1 = builder->getOperand(val1);
  operand2 = builder->getOperand(val2);

  auto res = builder
                 ->appendMUL(InstOp::MUL,
                             std::make_shared<Operand>(OperandType::VREG),
                             operand1, operand2)
                 ->m_dest;
  builder->m_value_map.insert(std::make_pair(inst, res));
  return res;
}

std::shared_ptr<Operand> GenerateDivInstruction(
    std::shared_ptr<BinaryInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Operand> operand1, operand2;
  std::shared_ptr<Value> val1 = inst->m_lhs_val_use.m_value;
  std::shared_ptr<Value> val2 = inst->m_rhs_val_use.m_value;
  operand1 = builder->getOperand(val1);
  operand2 = builder->getOperand(val2);

  auto res = builder
                 ->appendMUL(InstOp::SDIV,
                             std::make_shared<Operand>(OperandType::VREG),
                             operand1, operand2)
                 ->m_dest;
  builder->m_value_map.insert(std::make_pair(inst, res));
  return res;
}

// TODO(Huang): mod to generate

std::shared_ptr<Operand> GenerateBinaryInstruction(
    std::shared_ptr<BinaryInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  switch (inst->m_op) {
    case IROp::ADD:
      return GenerateAddInstruction(inst, builder);
    case IROp::SUB:
      return GenerateSubInstruction(inst, builder);
    case IROp::MUL:
      return GenerateMulInstruction(inst, builder);
    case IROp::SDIV:
    case IROp::SREM:
    //
    case IROp::SGEQ:
    case IROp::SGE:
    case IROp::SLEQ:
    case IROp::SLE:
    case IROp::EQ:
    case IROp::NE:
      break;
  }
  return nullptr;
}

//
std::shared_ptr<Operand> GenerateCallInstruction(
    std::shared_ptr<CallInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  int n = inst->m_params.size();

  int i = 0;
  // save params to r0, r1, r2, r3
  while (i < 4 && i < n) {
    std::shared_ptr<Value> value = inst->m_params[i].m_value;
    std::shared_ptr<Operand> reg = std::make_shared<Operand>(OperandType::REG);
    reg->m_rreg = (RReg)i;
    builder->appendMOV(reg, builder->getOperand(value));
  }
  // save params to stack
  while (i < n) {
    std::shared_ptr<Value> value = inst->m_params[i].m_value;
    std::shared_ptr<Operand> sp = std::make_shared<Operand>(OperandType::REG);
    sp->m_rreg = RReg::SP;
    int offs = -(i - 3) * 4;
    builder->appendSTR(builder->getOperand(value), sp,
                       std::make_shared<Operand>(offs));
  }

  // TODO(Huang): calculate the stack move size

  VarType return_type = (VarType)inst->m_type.m_base_type;
  builder->appendCALL(return_type, inst->m_func_name, n);
  // if the function has return value, save to r0
  std::shared_ptr<Operand> dest;
  if (return_type == VarType::INT || return_type == VarType::FLOAT) {
    dest = std::make_shared<Operand>(OperandType::VREG);
    std::shared_ptr<Operand> r0 = std::make_shared<Operand>(OperandType::REG);
    r0->m_rreg = RReg::R0;
    builder->appendMOV(dest, r0);
    builder->m_value_map.insert(std::make_pair(inst, dest));
  }

  // TODO(Huang): recover sp

  return dest;
}

std::shared_ptr<Operand> GenerateBranchInstruction(
    std::shared_ptr<BranchInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Operand> operand1, operand2;

  // TODO(Huang): cond
}

std::shared_ptr<Operand> GenerateJumpInstruction(
    std::shared_ptr<JumpInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto ret
      = builder->appendB(std::make_shared<ASM_BasicBlock>(), CondType::NONE);
  builder->m_filled_block.insert(std::make_pair(
      std::make_shared<std::shared_ptr<ASM_BasicBlock>>(ret->m_target),
      inst->m_bb));

  // TODO(Huang): deal with the cur block

  return nullptr;
}

std::shared_ptr<Operand> GenerateReturnInstruction(
    std::shared_ptr<ReturnInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto ret_val = inst->m_ret.m_value;
  if (ret_val != nullptr) {
    std::shared_ptr<Operand> r0 = std::make_shared<Operand>(OperandType::REG);
    r0->m_rreg = RReg::R0;
    builder->appendMOV(r0, builder->getOperand(ret_val));
  }
  builder->appendB(builder->m_cur_func->m_rblock, CondType::NONE);
  builder->m_cur_block = nullptr;
  return nullptr;
}



void GenerateInstruction(std::shared_ptr<Value> ir_value,
                         std::shared_ptr<ASM_Builder> builder) {
  if ((std::dynamic_pointer_cast<Constant>(ir_value)) != nullptr) {
    auto value = std::dynamic_pointer_cast<Constant>(ir_value);
    GenerateConstant(value, builder);
  } else if ((std::dynamic_pointer_cast<BinaryInstruction>(ir_value))
             != nullptr) {
    auto inst = std::dynamic_pointer_cast<BinaryInstruction>(ir_value);
    GenerateBinaryInstruction(inst, builder);
  } else if ((std::dynamic_pointer_cast<CallInstruction>(ir_value))
             != nullptr) {
    auto inst = std::dynamic_pointer_cast<CallInstruction>(ir_value);
    GenerateCallInstruction(inst, builder);
  } else if ((std::dynamic_pointer_cast<BranchInstruction>(ir_value))
             != nullptr) {
    auto inst = std::dynamic_pointer_cast<BranchInstruction>(ir_value);
    GenerateBranchInstruction(inst, builder);
  } else if ((std::dynamic_pointer_cast<JumpInstruction>(ir_value))
             != nullptr) {
    auto inst = std::dynamic_pointer_cast<JumpInstruction>(ir_value);
    GenerateJumpInstruction(inst, builder);
  } else if ((std::dynamic_pointer_cast<ReturnInstruction>(ir_value))
             != nullptr) {
    auto inst = std::dynamic_pointer_cast<ReturnInstruction>(ir_value);
    GenerateReturnInstruction(inst, builder);
  }
}

void GenerateBasicblock(std::shared_ptr<BasicBlock> ir_block,
                        std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_BasicBlock> block = std::make_shared<ASM_BasicBlock>();
  builder->appendBlock(block);
  for (auto &i : ir_block->GetInstList()) {
    GenerateInstruction(i, builder);
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

void GenerateModule(std::shared_ptr<Module> ir_module,
                    std::shared_ptr<ASM_Builder> builder) {
  builder->setIrModule(ir_module);
  for (auto &f : ir_module->m_function_list) {
    GenerateFunction(f, builder);
  }
}