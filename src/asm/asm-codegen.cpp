#include "asm/asm-builder.h"
#include "ir/ir.h"

std::shared_ptr<Operand> ASM_Builder::GenerateConstant(
    std::shared_ptr<Constant> value, bool genimm, bool checkimm) {
  std::shared_ptr<Operand> ret;
  if (value->m_type.IsBasicFloat()) {
    // TODO(Huang): float
  } else {
    int imm = value->m_int_val;
    if (genimm && (!checkimm || Operand::immCheck(imm))) {
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

  auto ret = builder->getOperand(inst);
  builder->appendAS(InstOp::ADD, ret, operand1, operand2);
  return ret;
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

  auto ret = builder->getOperand(inst);
  builder->appendAS(op, ret, operand1, operand2);
  return ret;
}

std::shared_ptr<Operand> GenerateMulInstruction(
    std::shared_ptr<BinaryInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto operand1 = builder->getOperand(inst->m_lhs_val_use->m_value);
  auto operand2 = builder->getOperand(inst->m_rhs_val_use->m_value);

  auto ret = builder->getOperand(inst);
  builder->appendMUL(InstOp::MUL, ret, operand1, operand2);
  return ret;
}

std::shared_ptr<Operand> GenerateDivInstruction(
    std::shared_ptr<BinaryInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto operand1 = builder->getOperand(inst->m_lhs_val_use->m_value);
  auto operand2 = builder->getOperand(inst->m_rhs_val_use->m_value);

  auto ret = builder->getOperand(inst);
  builder->appendMUL(InstOp::SDIV, ret, operand1, operand2);
  return ret;
}

std::shared_ptr<Operand> GenerateModInstruction(
    std::shared_ptr<BinaryInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto devidend = builder->getOperand(inst->m_lhs_val_use->m_value);
  auto devisor = builder->getOperand(inst->m_rhs_val_use->m_value);

  auto div_ret = std::make_shared<Operand>(OperandType::VREG);
  auto ret = builder->getOperand(inst);
  builder->appendSDIV(div_ret, devidend, devisor);
  builder->appendMUL(InstOp::MLS, ret, div_ret, devisor, devidend);
  return ret;
}

std::shared_ptr<Operand> GenerateCMPInstruction(
    std::shared_ptr<BinaryInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  // TODO(Huang): swap op1 and op2 if op1 is constant, then change the cond
  auto operand1 = builder->getOperand(inst->m_lhs_val_use->m_value);
  auto operand2 = builder->getOperand(inst->m_rhs_val_use->m_value, true);
  builder->appendCT(InstOp::CMP, operand1, operand2);
  return nullptr;
}

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
    case IROp::I_SGE:
    case IROp::I_SGT:
    case IROp::I_SLE:
    case IROp::I_SLT:
    case IROp::I_EQ:
    case IROp::I_NE:
      return GenerateCMPInstruction(inst, builder);
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
    std::shared_ptr<Operand> reg = Operand::getRReg((RReg)i);
    builder->appendMOV(reg, builder->getOperand(value, true, false));
    i++;
  }
  // save params to stack
  while (i < n) {
    std::shared_ptr<Value> value = inst->m_params[i]->m_value;
    int sp_offs = (i - 4) * 4;
    std::shared_ptr<Operand> offs;
    if (0 <= sp_offs && sp_offs < 4096) {
      offs = std::make_shared<Operand>(sp_offs);
    } else {
      offs = std::make_shared<Operand>(OperandType::VREG);
      builder->appendMOV(offs, sp_offs);
    }
    builder->appendSTR(builder->getOperand(value), Operand::getRReg(RReg::SP),
                       offs);
    i++;
  }

  VarType return_type = (VarType)inst->m_type.m_base_type;
  builder->appendCALL(return_type, inst->m_func_name, n);
  // if the function has return value, save to r0
  std::shared_ptr<Operand> ret;
  if (return_type == VarType::INT || return_type == VarType::FLOAT) {
    ret = builder->getOperand(inst);
    builder->appendMOV(ret, Operand::getRReg(RReg::R0));
  }

  // reclaim sp
  if (stack_move_size) {
    builder->reclaimSP();
  }
  return ret;
}

std::shared_ptr<Operand> GenerateBranchInstruction(
    std::shared_ptr<BranchInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_BasicBlock> true_block, false_block;
  std::shared_ptr<BinaryInstruction> inst_cond;
  true_block = builder->getBlock(inst->m_true_block);
  false_block = builder->getBlock(inst->m_false_block);
  inst_cond
      = std::dynamic_pointer_cast<BinaryInstruction>(inst->m_cond->m_value);
  CondType cond = GetCondFromIR(inst_cond->m_op);
  assert(cond != CondType::NONE);
  builder->m_cur_block->m_branch_pos = --builder->m_cur_block->m_insts.end();
  builder->appendB(true_block, cond);
  builder->appendB(false_block, CondType::NONE);
  return nullptr;
}

std::shared_ptr<Operand> GenerateJumpInstruction(
    std::shared_ptr<JumpInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_BasicBlock> target_block
      = builder->getBlock(inst->m_target_block);
  builder->appendB(target_block, CondType::NONE);
  builder->m_cur_block->m_branch_pos = --builder->m_cur_block->m_insts.end();
  return nullptr;
}

std::shared_ptr<Operand> GenerateReturnInstruction(
    std::shared_ptr<ReturnInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Value> ret_val;
  if (inst->m_ret && inst->m_ret->m_value) {
    ret_val = inst->m_ret->m_value;
    builder->appendMOV(Operand::getRReg(RReg::R0),
                       builder->getOperand(ret_val, true, false));
  }
  auto b = builder->appendB(builder->m_cur_func->m_rblock, CondType::NONE);
  if (ret_val) b->addUse(Operand::getRReg(RReg::R0));
  return nullptr;
}

std::shared_ptr<Operand> getAddr(std::shared_ptr<Value> value,
                                 std::shared_ptr<ASM_Builder> builder) {
  auto ret = builder->getOperand(value);
  if (auto var = std::dynamic_pointer_cast<GlobalVariable>(value)) {
    builder->appendLDR(ret, var->m_name);
  }
  return ret;
}

std::shared_ptr<Operand> GenerateGepInstruction(
    std::shared_ptr<GetElementPtrInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto base_addr = inst->m_addr->m_value;
  auto dimensions = base_addr->m_type.m_dimensions;
  auto indices = inst->m_indices;
  int const_offs = 0;
  int attribute = 1;
  std::shared_ptr<Operand> offs_op;
  bool first = true;

  // maybe redundant??
  if (indices.size() == 1) {
    if (auto val = std::dynamic_pointer_cast<Constant>(indices[0]->m_value)) {
      assert(val->m_type.IsBasicInt());
      const_offs = val->m_int_val * 4;
    } else {
      offs_op = builder->getOperand(indices[0]->m_value);
    }
  }
  //
  for (int i = dimensions.size() - 1; i >= 0; i--) {
    if (i + 1 >= indices.size()) {
      attribute *= dimensions[i];
      continue;
    }
    if (auto val
        = std::dynamic_pointer_cast<Constant>(indices[i + 1]->m_value)) {
      assert(val->m_type.IsBasicInt());
      const_offs += val->m_int_val * attribute * 4;
    } else {
      if (first) {
        first = false;
        offs_op = std::make_shared<Operand>(OperandType::VREG);
        auto mov = builder->appendMOV(
            std::make_shared<Operand>(OperandType::VREG), attribute);
        builder->appendMUL(InstOp::MUL, offs_op,
                           builder->getOperand(indices[i + 1]->m_value),
                           mov->m_dest);
      } else {
        auto mov = builder->appendMOV(
            std::make_shared<Operand>(OperandType::VREG), attribute);
        builder->appendMUL(InstOp::MLA, offs_op,
                           builder->getOperand(indices[i + 1]->m_value),
                           mov->m_dest, offs_op);
      }
    }
    attribute *= dimensions[i];
  }

  // const part offset obtained in ir phase
  std::shared_ptr<Operand> const_offs_op;
  if (const_offs) {
    if (Operand::immCheck(const_offs)) {
      const_offs_op = std::make_shared<Operand>(const_offs);
    } else {
      const_offs_op = std::make_shared<Operand>(OperandType::VREG);
      builder->appendMOV(const_offs_op, const_offs);
    }
  }

  if (!offs_op) {
    offs_op = const_offs_op;
  } else {
    builder->appendShift(InstOp::LSL, offs_op, offs_op,
                         std::make_shared<Operand>(2));
    if (const_offs) {
      builder->appendAS(InstOp::ADD, offs_op, offs_op, const_offs_op);
    }
  }

  // store the absolute address in register, then return it
  // actually, it would be better to return an offset form, like [Rn, Rm]
  // so that we can reduce the add instruction
  // TODO(Huang): optimize to offset form
  if (offs_op) {
    auto ret = builder->getOperand(inst);
    builder->appendAS(InstOp::ADD, ret, getAddr(base_addr, builder), offs_op);
    return ret;
  }

  // if wrong, try to append a mov inst
  return getAddr(base_addr, builder);
}

std::shared_ptr<Operand> GenerateLoadInstruction(
    std::shared_ptr<LoadInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto src_value = inst->m_addr->m_value;
  auto ret = builder->getOperand(inst);
  builder->appendLDR(ret, getAddr(src_value, builder),
                     std::make_shared<Operand>(0));
  return ret;
}

std::shared_ptr<Operand> GenerateStoreInstruction(
    std::shared_ptr<StoreInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto addr = inst->m_addr->m_value;
  auto val = inst->m_val->m_value;
  auto src = builder->getOperand(val);
  builder->appendSTR(src, getAddr(addr, builder), std::make_shared<Operand>(0));
  return nullptr;
}

std::shared_ptr<Operand> GenerateAllocaInstruction(
    std::shared_ptr<AllocaInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  int alloc_size = 4;
  for (int i : inst->m_type.m_dimensions) {
    alloc_size *= i;
  }

  int sp_offs = builder->m_cur_func->getStackSize();
  builder->m_cur_func->allocateStack(alloc_size);
  std::shared_ptr<Operand> offs;
  if (Operand::immCheck(sp_offs)) {
    offs = std::make_shared<Operand>(sp_offs);
  } else {
    offs = std::make_shared<Operand>(OperandType::VREG);
    builder->appendMOV(std::make_shared<Operand>(OperandType::VREG), sp_offs);
  }
  auto ret = builder->getOperand(inst);
  builder->appendAS(InstOp::ADD, ret, Operand::getRReg(RReg::SP), offs);
  return ret;
}

std::shared_ptr<Operand> GeneratePhiInstruction(
    std::shared_ptr<PhiInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  // we need to store the temp result to make sure all PHI instruction execute
  // in parallel
  auto tmp = std::make_shared<Operand>(OperandType::VREG);
  auto ret = builder->getOperand(inst);
  builder->appendMOV(ret, tmp);
  for (auto &[ir_block, value] : inst->m_contents) {
    std::shared_ptr<ASM_BasicBlock> block = builder->getBlock(ir_block);
    assert(block);
    if (!value) continue;
    auto src = builder->getOperand(value->m_value, true, false);
    block->appendFilledMOV(std::make_shared<MOVInst>(tmp, src));
  }
  return ret;
}

void GenerateInstruction(std::shared_ptr<Value> ir_value,
                         std::shared_ptr<ASM_Builder> builder) {
  if (auto value = std::dynamic_pointer_cast<Constant>(ir_value)) {
    builder->GenerateConstant(value, true, true);
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
  std::shared_ptr<ASM_BasicBlock> block = builder->getBlock(ir_block);
  builder->appendBlock(block);
  builder->m_block_map.insert(std::make_pair(ir_block, block));
  for (auto &i : ir_block->GetInstList()) {
    GenerateInstruction(i, builder);
  }
}

void GenerateFunction(std::shared_ptr<Function> ir_func,
                      std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_Function> func = std::make_shared<ASM_Function>(ir_func);
  builder->appendFunction(func);
  for (auto &b : ir_func->m_bb_list) {
    GenerateBasicblock(b, builder);
  }

#ifdef SP_FOR_PARAM
  builder->fixedStackParams();
#endif

  builder->m_cur_func->m_blocks.push_back(builder->m_cur_func->m_rblock);
  // insert MOV instruction(from PHI)
  for (auto &b : func->m_blocks) {
    b->fillMOV();
  }

  // insert mov instruction in the entry block for loading params
  auto first_block = func->m_blocks.front();
  auto iter = first_block->m_insts.begin();
  for (auto &mov : func->m_params_set_list) {
    first_block->m_insts.insert(iter, mov);
    mov->m_block = first_block;
  }
}

void GenerateModule(std::shared_ptr<Module> ir_module,
                    std::shared_ptr<ASM_Builder> builder) {
  builder->setIrModule(ir_module);
  for (auto &f : ir_module->m_function_list) {
    GenerateFunction(f, builder);
  }
}