#include "asm/asm-builder.h"
#include "ir/ir.h"

std::shared_ptr<Operand> ASM_Builder::GenerateConstant(
    std::shared_ptr<Constant> value, bool genimm) {
  std::shared_ptr<Operand> ret;
  if (value->m_is_float) {
    // TODO(Huang): float
  } else {
    int imm = value->m_int_val;
    if (genimm && Operand::immCheck(imm)) {
      return std::make_shared<Operand>(imm);
    }
    ret = appendMOV(std::make_shared<Operand>(OperandType::VREG), imm)->m_dest;
  }
  return ret;
}

std::shared_ptr<Operand> GenerateAddInstruction(
    std::shared_ptr<BinaryInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Operand> operand1, operand2;
  std::shared_ptr<Value> val1 = inst->m_lhs_val_use->m_value;
  std::shared_ptr<Value> val2 = inst->m_rhs_val_use->m_value;

  if (std::dynamic_pointer_cast<Constant>(val1)) {
    std::swap(val1, val2);
  }
  operand1 = builder->getOperand(val1);
  operand2 = builder->getOperand(val2, true);

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
  std::shared_ptr<Value> val1 = inst->m_lhs_val_use->m_value;
  std::shared_ptr<Value> val2 = inst->m_rhs_val_use->m_value;
  bool is_const1 = (std::dynamic_pointer_cast<Constant>(val1) != nullptr);
  bool is_const2 = (std::dynamic_pointer_cast<Constant>(val2) != nullptr);

  InstOp op;
  if (is_const1) {
    op = InstOp::RSB;
    std::swap(val1, val2);
  } else {
    op = InstOp::SUB;
  }
  operand1 = builder->getOperand(val1);
  operand2 = builder->getOperand(val2, true);

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
  auto operand1 = builder->getOperand(inst->m_lhs_val_use->m_value);
  auto operand2 = builder->getOperand(inst->m_rhs_val_use->m_value);
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
  auto operand1 = builder->getOperand(inst->m_lhs_val_use->m_value);
  auto operand2 = builder->getOperand(inst->m_rhs_val_use->m_value);
  auto res = builder
                 ->appendMUL(InstOp::SDIV,
                             std::make_shared<Operand>(OperandType::VREG),
                             operand1, operand2)
                 ->m_dest;
  builder->m_value_map.insert(std::make_pair(inst, res));
  return res;
}

std::shared_ptr<Operand> GenerateModInstruction(
    std::shared_ptr<BinaryInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto devidend = builder->getOperand(inst->m_lhs_val_use->m_value);
  auto devisor = builder->getOperand(inst->m_rhs_val_use->m_value);
  auto div_ret = builder
                     ->appendSDIV(std::make_shared<Operand>(OperandType::VREG),
                                  devidend, devisor)
                     ->m_dest;
  auto ret = builder
                 ->appendMUL(InstOp::MLS,
                             std::make_shared<Operand>(OperandType::VREG),
                             div_ret, devisor, devidend)
                 ->m_dest;
  builder->m_value_map.insert(std::make_pair(inst, ret));
  return ret;
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
      return GenerateDivInstruction(inst, builder);
    case IROp::SREM:
      return GenerateModInstruction(inst, builder);
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
  // calculate the stack move size
  unsigned int stack_move_size = std::max(n - 4, 0) * 4;
  if (stack_move_size) {
    builder->allocSP(stack_move_size);
  }

  int i = 0;
  // save params to r0 ~ r3
  while (i < 4 && i < n) {
    std::shared_ptr<Value> value = inst->m_params[i]->m_value;
    std::shared_ptr<Operand> reg = std::make_shared<Operand>(OperandType::REG);
    reg->m_rreg = (RReg)i;
    builder->appendMOV(reg, builder->getOperand(value, true));
    i++;
  }
  // save params to stack
  std::shared_ptr<Operand> sp = std::make_shared<Operand>(OperandType::REG);
  sp->m_rreg = RReg::SP;
  while (i < n) {
    std::shared_ptr<Value> value = inst->m_params[i]->m_value;
    int sp_offs = (i - 4) * 4;
    std::shared_ptr<Operand> offs;
    if (Operand::immCheck(sp_offs)) {
      offs = std::make_shared<Operand>(sp_offs);
    } else {
      offs = builder
                 ->appendMOV(std::make_shared<Operand>(OperandType::VREG),
                             sp_offs)
                 ->m_dest;
    }
    builder->appendSTR(builder->getOperand(value), sp, offs);
    i++;
  }

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

  // reclaim sp
  if (stack_move_size) {
    builder->reclaimSP();
  }
  return dest;
}

std::shared_ptr<Operand> GenerateBranchInstruction(
    std::shared_ptr<BranchInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Operand> operand1, operand2;

  // TODO(Huang): cond
  return nullptr;
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
  auto ret_val = inst->m_ret->m_value;
  if (ret_val != nullptr) {
    std::shared_ptr<Operand> r0 = std::make_shared<Operand>(OperandType::REG);
    r0->m_rreg = RReg::R0;
    builder->appendMOV(r0, builder->getOperand(ret_val, true));
  }
  builder->appendB(builder->m_cur_func->m_rblock, CondType::NONE);
  builder->m_cur_block = nullptr;
  return nullptr;
}

std::shared_ptr<Operand> getAddr(std::shared_ptr<Value> value,
                                 std::shared_ptr<ASM_Builder> builder) {
  if (auto var = std::dynamic_pointer_cast<GlobalVariable>(value)) {
    return builder
        ->appendLDR(std::make_shared<Operand>(OperandType::VREG), var->m_name)
        ->m_dest;
  }
  return builder->getOperand(value);
}

std::shared_ptr<Operand> GenerateGepInstruction(
    std::shared_ptr<GetElementPtrInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto base_addr = inst->m_addr->m_value;
  auto dimensions = base_addr->m_type.m_dimensions;
  auto indices = inst->m_indices;
  int offs = 0;
  int attribute = 1;
  for (int i = dimensions.size() - 1; i >= 0; i--) {
    offs += std::dynamic_pointer_cast<Constant>(indices[i + 1]->m_value)
                ->m_int_val
            * attribute;
    attribute *= dimensions[i];
  }
  offs *= 4;

  std::shared_ptr<Operand> offsOp;
  if (Operand::immCheck(offs)) {
    offsOp = std::make_shared<Operand>(offs);
  } else {
    offsOp
        = builder->appendMOV(std::make_shared<Operand>(OperandType::VREG), offs)
              ->m_dest;
  }

  // store the absolute address in register, then return it
  // actually, it would be better to return an offset form, like [Rn, Rm]
  // so that we can reduce the add instruction
  // TODO(Huang): optimize to offset form
  auto ret = builder
                 ->appendAS(InstOp::ADD,
                            std::make_shared<Operand>(OperandType::VREG),
                            getAddr(base_addr, builder), offsOp)
                 ->m_dest;
  builder->m_value_map.insert(std::make_pair(inst, ret));
  return ret;
}

std::shared_ptr<Operand> GenerateLoadInstruction(
    std::shared_ptr<LoadInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto src = inst->m_addr->m_value;
  auto dest
      = builder
            ->appendLDR(std::make_shared<Operand>(OperandType::VREG),
                        getAddr(src, builder), std::make_shared<Operand>(0))
            ->m_dest;
  builder->m_value_map.insert(std::make_pair(inst, dest));
  return dest;
}

std::shared_ptr<Operand> GenerateStoreInstruction(
    std::shared_ptr<StoreInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto addr = inst->m_addr->m_value;
  auto val = inst->m_val->m_value;
  auto src = builder->getOperand(val);
  auto dest = builder
                  ->appendSTR(src, getAddr(addr, builder),
                              std::make_shared<Operand>(0))
                  ->m_dest;
  return nullptr;
}

std::shared_ptr<Operand> GenerateAllocaInstruction(
    std::shared_ptr<AllocaInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  // TODO(Huang): alloc_size = 4 is temporary
  unsigned int alloc_size = 4;

  unsigned int sp_offs = builder->m_cur_func->getStackSize();
  builder->m_cur_func->allocateStack(alloc_size);
  std::shared_ptr<Operand> offs;
  if (Operand::immCheck(sp_offs)) {
    offs = std::make_shared<Operand>(sp_offs);
  } else {
    offs
        = builder
              ->appendMOV(std::make_shared<Operand>(OperandType::VREG), sp_offs)
              ->m_dest;
  }
  std::shared_ptr<Operand> sp = std::make_shared<Operand>(OperandType::REG);
  sp->m_rreg = RReg::SP;
  auto ret
      = builder
            ->appendAS(InstOp::ADD,
                       std::make_shared<Operand>(OperandType::VREG), sp, offs)
            ->m_dest;
  builder->m_value_map.insert(std::make_pair(inst, ret));
  return ret;
}

std::shared_ptr<Operand> GeneratePhiInstruction(
    std::shared_ptr<PhiInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto ret = std::make_shared<Operand>(OperandType::VREG);
  for (auto &[ir_block, value] : inst->m_contents) {
    std::shared_ptr<ASM_BasicBlock> block = builder->getBlock(ir_block);
    std::shared_ptr<Operand> src = builder->getOperand(value->m_value);
    auto mov = std::make_shared<MOVInst>(ret, src);
    assert(block && block->m_branch_pos != block->m_insts.end());
    block->insertPhiMOV(mov);
  }
  builder->m_value_map.insert(std::make_pair(inst, ret));
  return ret;
}

void GenerateInstruction(std::shared_ptr<Value> ir_value,
                         std::shared_ptr<ASM_Builder> builder) {
  if (auto value = std::dynamic_pointer_cast<Constant>(ir_value)) {
    builder->GenerateConstant(value, true);
  } else if (auto inst
             = std::dynamic_pointer_cast<BinaryInstruction>(ir_value)) {
    GenerateBinaryInstruction(inst, builder);
  } else if (auto inst = std::dynamic_pointer_cast<CallInstruction>(ir_value)) {
    GenerateCallInstruction(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<BranchInstruction>(ir_value)) {
    GenerateBranchInstruction(inst, builder);
  } else if (auto inst = std::dynamic_pointer_cast<JumpInstruction>(ir_value)) {
    GenerateJumpInstruction(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<ReturnInstruction>(ir_value)) {
    GenerateReturnInstruction(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<GetElementPtrInstruction>(ir_value)) {
    GenerateGepInstruction(inst, builder);
  } else if (auto inst = std::dynamic_pointer_cast<LoadInstruction>(ir_value)) {
    GenerateLoadInstruction(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<StoreInstruction>(ir_value)) {
    GenerateStoreInstruction(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<AllocaInstruction>(ir_value)) {
    GenerateAllocaInstruction(inst, builder);
  } else if (auto inst = std::dynamic_pointer_cast<PhiInstruction>(ir_value)) {
    GeneratePhiInstruction(inst, builder);
  }
}

void GenerateBasicblock(std::shared_ptr<BasicBlock> ir_block,
                        std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_BasicBlock> block = std::make_shared<ASM_BasicBlock>();
  builder->appendBlock(block);
  for (auto &i : ir_block->GetInstList()) {
    builder->m_block_map.insert(std::make_pair(ir_block, block));
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

#ifdef SP_FOR_PARAM
  builder->fixedStackParams();
#endif

  builder->m_cur_func->m_blocks.push_back(builder->m_cur_func->m_rblock);
}

void GenerateModule(std::shared_ptr<Module> ir_module,
                    std::shared_ptr<ASM_Builder> builder) {
  builder->setIrModule(ir_module);
  for (auto &f : ir_module->m_function_list) {
    GenerateFunction(f, builder);
  }
}