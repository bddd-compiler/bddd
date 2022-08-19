#include "asm/asm-builder.h"
#include "ir/ir.h"

// #define DIV_CONST

std::shared_ptr<Operand> ASM_Builder::GenerateConstant(
    std::shared_ptr<Constant> value, bool genimm, bool checkimm,
    std::shared_ptr<ASM_BasicBlock> phi_block) {
  assert(value->m_type.IsConst());
  std::shared_ptr<Operand> ret;
  if (value->m_type.IsBasicFloat()) {
    ret = std::make_shared<Operand>(OperandType::VREG, true);
    appendMOV(ret, value->m_float_val, phi_block);
  } else {
    int imm = value->m_int_val;
    if (genimm && (!checkimm || Operand::immCheck(imm))) {
      return std::make_shared<Operand>(imm);
    }
    ret = std::make_shared<Operand>(OperandType::VREG);
    appendMOV(ret, imm, phi_block);
  }
  return ret;
}

std::shared_ptr<Operand> GenerateAdd(std::shared_ptr<BinaryInstruction> inst,
                                     std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Value> val1 = inst->m_lhs_val_use->getValue();
  std::shared_ptr<Value> val2 = inst->m_rhs_val_use->getValue();
  bool is_int = inst->m_op == IROp::ADD;
  InstOp op = is_int ? InstOp::ADD : InstOp::VADD;
  auto ret = builder->getOperand(inst);

  if (val1->m_type.IsConst()) {
    std::swap(val1, val2);
  }
  auto operand1 = builder->getOperand(val1);
  auto operand2 = builder->getOperand(val2, is_int);

  if (!is_int) ret->m_is_float = true;

  builder->appendAS(op, ret, operand1, operand2);
  return ret;
}

std::shared_ptr<Operand> GenerateSub(std::shared_ptr<BinaryInstruction> inst,
                                     std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Value> val1 = inst->m_lhs_val_use->getValue();
  std::shared_ptr<Value> val2 = inst->m_rhs_val_use->getValue();
  bool is_int = inst->m_op == IROp::SUB;
  InstOp op = is_int ? InstOp::SUB : InstOp::VSUB;
  auto ret = builder->getOperand(inst);

  if (op == InstOp::SUB && val1->m_type.IsConst()) {
    op = InstOp::RSB;
    std::swap(val1, val2);
  }
  auto operand1 = builder->getOperand(val1);
  auto operand2 = builder->getOperand(val2, is_int);

  if (!is_int) ret->m_is_float = true;

  builder->appendAS(op, ret, operand1, operand2);
  return ret;
}

std::shared_ptr<Operand> GenerateMul(std::shared_ptr<BinaryInstruction> inst,
                                     std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Value> val1 = inst->m_lhs_val_use->getValue();
  std::shared_ptr<Value> val2 = inst->m_rhs_val_use->getValue();
  bool is_const1 = val1->m_type.IsConst();
  bool is_int = inst->m_op == IROp::MUL;
  InstOp op = is_int ? InstOp::MUL : InstOp::VMUL;
  auto ret = builder->getOperand(inst);

  if (val1->m_type.IsConst()) {
    std::swap(val1, val2);
  }
  if (val2->m_type.IsConst()) {
    auto const_val = std::dynamic_pointer_cast<Constant>(val2);
    if (const_val->m_int_val == 1 || const_val->m_float_val == 1) {
      builder->appendMOV(ret, builder->getOperand(val1));
      return ret;
    } else if (val2->m_type.IsBasicInt()) {
      int temp = 2;
      for (int i = 1; i < 32; i++) {
        if (temp == const_val->m_int_val) {
          builder->appendShift(InstOp::LSL, ret, builder->getOperand(val1),
                               std::make_shared<Operand>(i));
          return ret;
        }
        temp <<= 1;
      }
    }
  }

  auto operand1 = builder->getOperand(val1);
  auto operand2 = builder->getOperand(val2);

  if (!is_int) ret->m_is_float = true;
  builder->appendMUL(op, ret, operand1, operand2);
  return ret;
}

void divConst(std::shared_ptr<Operand> ret, std::shared_ptr<Operand> operand1,
              int devisor, std::shared_ptr<ASM_Builder> builder) {
  u_int32_t d = devisor;
  u_int64_t n = (1 << 31) - ((1 << 31) % d) - 1;
  u_int64_t p = 32;
  while (((u_int64_t)1 << p) <= n * (d - ((u_int64_t)1 << p) % d)) {
    p++;
  }
  u_int64_t m = (((u_int64_t)1 << p) + (u_int64_t)d - ((u_int64_t)1 << p) % d)
                / (u_int64_t)d;
  auto magic_num = std::make_shared<Operand>(OperandType::VREG);
  builder->appendMOV(magic_num, (int)m);
  auto mul_ret = std::make_shared<Operand>(OperandType::VREG);
  if (m >= 0x80000000)
    builder->appendMUL(InstOp::SMMLA, mul_ret, magic_num, operand1, operand1);
  else
    builder->appendMUL(InstOp::SMMUL, mul_ret, magic_num, operand1);
  auto temp_reg = std::make_shared<Operand>(OperandType::VREG);
  builder->appendShift(InstOp::ASR, temp_reg, mul_ret,
                       std::make_shared<Operand>((int)(p - 32)));
  auto ret_inst = builder->appendAS(InstOp::ADD, ret, temp_reg, operand1);
  ret_inst->m_shift = std::make_unique<Shift>(Shift::ShiftType::LSR, 31);
}

std::shared_ptr<Operand> GenerateDiv(std::shared_ptr<BinaryInstruction> inst,
                                     std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Value> val1 = inst->m_lhs_val_use->getValue();
  std::shared_ptr<Value> val2 = inst->m_rhs_val_use->getValue();
  bool is_int = inst->m_op == IROp::SDIV;
  InstOp op = is_int ? InstOp::SDIV : InstOp::VDIV;
  auto ret = builder->getOperand(inst);
  auto operand1 = builder->getOperand(val1);

  if (val2->m_type.IsConst()) {
    auto const_val = std::dynamic_pointer_cast<Constant>(val2);
    if (const_val->m_int_val == 1 || const_val->m_float_val == 1) {
      builder->appendMOV(ret, operand1);
      return ret;
    } else if (is_int) {
      // check if the value of devisor is pow of 2
      int temp = 2;
      for (int i = 1; i < 32; i++) {
        if (temp == const_val->m_int_val) {
          auto ret_temp = std::make_shared<Operand>(OperandType::VREG);
          builder->appendShift(InstOp::ASR, ret_temp, operand1,
                               std::make_shared<Operand>(i - 1));
          auto add
              = builder->appendAS(InstOp::ADD, ret_temp, operand1, ret_temp);
          add->m_shift = std::make_unique<Shift>(Shift::ShiftType::LSR, 32 - i);
          builder->appendShift(InstOp::ASR, ret, ret_temp,
                               std::make_shared<Operand>(i));
          return ret;
        }
        temp <<= 1;
      }

      // div const
      // seems to be a negative optimization?
#ifdef DIV_CONST
      divConst(ret, operand1, const_val->m_int_val, builder);
      return ret;
#endif
    }
  }

  auto operand2 = builder->getOperand(val2);

  if (!is_int) ret->m_is_float = true;
  builder->appendMUL(op, ret, operand1, operand2);
  return ret;
}

std::shared_ptr<Operand> GenerateMod(std::shared_ptr<BinaryInstruction> inst,
                                     std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Value> val1 = inst->m_lhs_val_use->getValue();
  std::shared_ptr<Value> val2 = inst->m_rhs_val_use->getValue();
  auto ret = builder->getOperand(inst);
  auto devidend = builder->getOperand(val1);
  std::shared_ptr<Operand> div_ret;

  if (val2->m_type.IsConst()) {
    assert(val2->m_type.IsBasicInt());
    auto const_val = std::dynamic_pointer_cast<Constant>(val2);
    if (const_val->m_int_val == 1) {
      builder->appendMOV(ret, 0);
      return ret;
    }
    int temp = 2;
    for (int i = 1; i < 32; i++) {
      if (temp == const_val->m_int_val) {
        int and_val = temp - 1;
        std::shared_ptr<Operand> and_reg;
        if (Operand::immCheck(and_val)) {
          and_reg = std::make_shared<Operand>(and_val);
        } else {
          and_reg = std::make_shared<Operand>(OperandType::VREG);
          builder->appendMOV(and_reg, and_val);
        }
        auto and_inst = builder->appendBIT(InstOp::AND, ret, devidend, and_reg);
        and_inst->m_set_flag = true;
        auto cmp_inst = builder->appendCT(InstOp::CMP, devidend,
                                          std::make_shared<Operand>(0));
        cmp_inst->m_cond = CondType::NE;
        auto sub_inst = builder->appendAS(InstOp::SUB, ret, ret,
                                          std::make_shared<Operand>(temp));
        sub_inst->m_cond = CondType::MI;
        return ret;
      }
      temp <<= 1;
    }
#ifdef DIV_CONST
    div_ret = std::make_shared<Operand>(OperandType::VREG);
    divConst(div_ret, devidend, const_val->m_int_val, builder);
#endif
  }

  auto devisor = builder->getOperand(val2);
  if (!div_ret) {
    div_ret = std::make_shared<Operand>(OperandType::VREG);
    builder->appendSDIV(div_ret, devidend, devisor);
  }
  builder->appendMUL(InstOp::MLS, ret, div_ret, devisor, devidend);
  return ret;
}

std::shared_ptr<Operand> GenerateFNeg(std::shared_ptr<FNegInstruction> inst,
                                      std::shared_ptr<ASM_Builder> builder) {
  auto operand = builder->getOperand(inst->m_lhs_val_use->getValue());
  auto ret = builder->getOperand(inst);
  ret->m_is_float = true;
  builder->appendVNEG(ret, operand);
  return ret;
}

std::shared_ptr<Operand> GenerateCMP(std::shared_ptr<BinaryInstruction> inst,
                                     std::shared_ptr<ASM_Builder> builder) {
  // TODO(Huang): swap op1 and op2 if op1 is constant, then change the cond
  bool is_int = ((int)inst->m_op <= 15);
  auto operand1 = builder->getOperand(inst->m_lhs_val_use->getValue());
  std::shared_ptr<Operand> operand2;

  // vcmp allow op2 to be constant 0.0
  if (!is_int) {
    auto float_val
        = std::dynamic_pointer_cast<Constant>(inst->m_rhs_val_use->getValue());
    if (float_val && float_val->m_float_val == (float)0.0) {
      operand2 = std::make_shared<Operand>((float)0.0);
      builder->appendCT(InstOp::VCMP, operand1, operand2);
      return nullptr;
    }
  }

  operand2 = builder->getOperand(inst->m_rhs_val_use->getValue(), is_int);
  InstOp op = is_int ? InstOp::CMP : InstOp::VCMP;
  builder->appendCT(op, operand1, operand2);
  return nullptr;
}

std::shared_ptr<Operand> GenerateBinary(std::shared_ptr<BinaryInstruction> inst,
                                        std::shared_ptr<ASM_Builder> builder) {
  switch (inst->m_op) {
    case IROp::ADD:
    case IROp::F_ADD:
      return GenerateAdd(inst, builder);
    case IROp::SUB:
    case IROp::F_SUB:
      return GenerateSub(inst, builder);
    case IROp::MUL:
    case IROp::F_MUL:
      return GenerateMul(inst, builder);
    case IROp::SDIV:
    case IROp::F_DIV:
      return GenerateDiv(inst, builder);
    case IROp::SREM:
      return GenerateMod(inst, builder);
    case IROp::XOR:
      return nullptr;
    case IROp::I_SGE:
    case IROp::I_SGT:
    case IROp::I_SLE:
    case IROp::I_SLT:
    case IROp::I_EQ:
    case IROp::I_NE:
    case IROp::F_EQ:
    case IROp::F_NE:
    case IROp::F_GT:
    case IROp::F_GE:
    case IROp::F_LT:
    case IROp::F_LE:
      return GenerateCMP(inst, builder);
    case IROp::ZEXT:
    case IROp::FPTOSI:
    case IROp::SITOFP:
      std::cerr << "Is this really a binary instruction???" << std::endl;
    default:
      std::cout << (int)inst->m_op << std::endl;
      assert(false);
  }
  return nullptr;
}

// not support float yet
std::shared_ptr<Operand> GenerateCall(std::shared_ptr<CallInstruction> inst,
                                      std::shared_ptr<ASM_Builder> builder) {
  if (inst->m_func_name == "llvm.memset.p0i8.i32") {
    builder->appendMOV(
        Operand::getRReg(RReg::R0),
        builder->getOperand(inst->m_params[0]->getValue(), true, false));
    builder->appendMOV(
        Operand::getRReg(RReg::R1),
        builder->getOperand(inst->m_params[1]->getValue(), true, false));
    builder->appendMOV(
        Operand::getRReg(RReg::R2),
        builder->getOperand(inst->m_params[2]->getValue(), true, false));
    builder->appendCALL(VarType::VOID, "memset", 3);
    return nullptr;
  }

  if (inst->m_func_name == "putfloat") {
    builder->appendMOV(Operand::getSReg(SReg::S0),
                       builder->getOperand(inst->m_params[0]->getValue()));
    builder->appendCALL(VarType::VOID, "putfloat", 1);
    return nullptr;
  }

  int n = inst->m_params.size();
  // calculate the stack move size
  int stack_move_size = std::max(n - 4, 0) * 4;
  if (stack_move_size) {
    builder->allocSP(stack_move_size);
  }
  if (stack_move_size & 7) {
    stack_move_size &= ~(unsigned int)7;
    stack_move_size += 8;
  }

  int i = 0;
  // save params to r0 ~ r3
  while (i < 4 && i < n) {
    std::shared_ptr<Value> value = inst->m_params[i]->getValue();
    std::shared_ptr<Operand> reg = Operand::getRReg((RReg)i);
    auto mov = builder->appendMOV(reg, builder->getOperand(value, true, false));
    mov->m_params_offset = stack_move_size;
    i++;
  }

  // save params to stack
  while (i < n) {
    std::shared_ptr<Value> value = inst->m_params[i]->getValue();
    int sp_offs = (i - 4) * 4;
    std::shared_ptr<Operand> offs;
    std::shared_ptr<STRInst> str;
    if (Operand::addrOffsCheck(sp_offs, value->m_type.IsBasicFloat())) {
      offs = std::make_shared<Operand>(sp_offs);
      str = builder->appendSTR(builder->getOperand(value),
                               Operand::getRReg(RReg::SP), offs);
    } else {
      if (value->m_type.IsBasicInt()) {
        offs = std::make_shared<Operand>(OperandType::VREG);
        auto mov = builder->appendMOV(offs, sp_offs);
        mov->m_params_offset = stack_move_size;
        str = builder->appendSTR(builder->getOperand(value),
                                 Operand::getRReg(RReg::SP), offs);
      } else {
        offs = std::make_shared<Operand>(OperandType::VREG);
        if (Operand::immCheck(sp_offs)) {
          auto add
              = builder->appendAS(InstOp::ADD, offs, Operand::getRReg(RReg::SP),
                                  std::make_shared<Operand>(sp_offs));
          add->m_params_offset = stack_move_size;
        } else {
          auto mov = builder->appendMOV(offs, sp_offs);
          auto add = builder->appendAS(InstOp::ADD, offs,
                                       Operand::getRReg(RReg::SP), offs);
          mov->m_params_offset = stack_move_size;
          add->m_params_offset = stack_move_size;
        }
        str = builder->appendSTR(builder->getOperand(value), offs,
                                 std::make_shared<Operand>(0));
      }
    }
    str->m_params_offset = stack_move_size;
    i++;
  }

  VarType return_type = (VarType)inst->m_type.m_base_type;
  builder->appendCALL(return_type, inst->m_func_name, n);

  // reclaim sp
  if (stack_move_size) {
    builder->reclaimSP();
  }

  // if the function has return value, save to r0
  std::shared_ptr<Operand> ret;
  if (return_type == VarType::INT || return_type == VarType::FLOAT) {
    ret = builder->getOperand(inst);
    if (inst->m_func_name == "getfloat")
      builder->appendMOV(ret, Operand::getSReg(SReg::S0));
    else
      builder->appendMOV(ret, Operand::getRReg(RReg::R0));
  }

  return ret;
}

std::shared_ptr<Operand> GenerateBranch(std::shared_ptr<BranchInstruction> inst,
                                        std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_BasicBlock> true_block, false_block;
  std::shared_ptr<BinaryInstruction> inst_cond;
  true_block = builder->getBlock(inst->m_true_block);
  false_block = builder->getBlock(inst->m_false_block);

  inst_cond
      = std::dynamic_pointer_cast<BinaryInstruction>(inst->m_cond->getValue());

  bool is_not = false;
  while (inst_cond->m_op == IROp::XOR) {
    is_not = !is_not;
    inst_cond = std::dynamic_pointer_cast<BinaryInstruction>(
        inst_cond->m_lhs_val_use->getValue());
  }
  CondType cond = GetCondFromIR(inst_cond->m_op);
  if (is_not) {
    cond = ASM_Instruction::getOppositeCond(cond);
  }
  assert(cond != CondType::NONE);
  if (builder->m_cur_block->m_status_load_inst) {
    builder->m_cur_block->insert(builder->m_cur_block->m_status_load_inst);
  }
  builder->appendB(true_block, cond);
  builder->m_cur_block->m_branch_pos
      = std::prev(builder->m_cur_block->m_insts.end());
  builder->appendB(false_block, CondType::NONE);
  return nullptr;
}

std::shared_ptr<Operand> GenerateJump(std::shared_ptr<JumpInstruction> inst,
                                      std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_BasicBlock> target_block
      = builder->getBlock(inst->m_target_block);
  builder->appendB(target_block, CondType::NONE);
  builder->m_cur_block->m_branch_pos
      = std::prev(builder->m_cur_block->m_insts.end());
  return nullptr;
}

std::shared_ptr<Operand> GenerateReturn(std::shared_ptr<ReturnInstruction> inst,
                                        std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Value> ret_val;
  if (inst->m_ret && inst->m_ret->getValue()) {
    ret_val = inst->m_ret->getValue();
    builder->appendMOV(Operand::getRReg(RReg::R0),
                       builder->getOperand(ret_val, true, false));
  }
  auto b = builder->appendB(builder->m_cur_func->m_rblock, CondType::NONE);
  if (ret_val) b->addUse(Operand::getRReg(RReg::R0));
  return nullptr;
}

std::shared_ptr<Operand> getAddr(std::shared_ptr<Value> value,
                                 std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<Operand> ret;
  if (auto var = std::dynamic_pointer_cast<GlobalVariable>(value)) {
    ret = std::make_shared<Operand>(OperandType::VREG);
    builder->appendLDR(ret, var->m_name);
  } else {
    ret = builder->getOperand(value);
  }
  return ret;
}

std::shared_ptr<Operand> GenerateGetElementPtr(
    std::shared_ptr<GetElementPtrInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto base_addr = inst->m_addr->getValue();         // the base address
  auto dimensions = base_addr->m_type.m_dimensions;  // dimensions
  auto indices = inst->m_indices;  // indices of GetElementPtrInstruction

  int const_offs = 0;                      // store offset of constant
  std::shared_ptr<Operand> var_offs;       // store offset of variable
  std::shared_ptr<Operand> const_offs_op;  // constant offset to operand
  std::shared_ptr<Operand> var_offs_op;    // variable offset to operand
  std::shared_ptr<Operand> offs_op;  // sum of const_offs_op and var_offs_op
  int attribute = 1;
  bool first = true;

  for (int i = dimensions.size(); i >= 0; i--) {
    if (i >= indices.size()) {
      attribute *= dimensions[i - 1];
      continue;
    }
    if (auto val
        = std::dynamic_pointer_cast<Constant>(indices[i]->getValue())) {
      assert(val->m_type.IsBasicInt());
      const_offs += val->m_int_val * attribute * 4;
    } else {
      var_offs = builder->getOperand(indices[i]->getValue());
      if (first) {
        first = false;
        var_offs_op = std::make_shared<Operand>(OperandType::VREG);
        if (attribute == 1) {
          builder->appendMOV(var_offs_op, var_offs);
        } else {
          auto mov = builder->appendMOV(
              std::make_shared<Operand>(OperandType::VREG), attribute);
          builder->appendMUL(InstOp::MUL, var_offs_op, var_offs, mov->m_dest);
        }
      } else {
        auto mov = builder->appendMOV(
            std::make_shared<Operand>(OperandType::VREG), attribute);
        builder->appendMUL(InstOp::MLA, var_offs_op, var_offs, mov->m_dest,
                           var_offs_op);
      }
    }
    if (i > 0) attribute *= dimensions[i - 1];
  }

  // const part offset obtained in ir phase
  if (const_offs) {
    if (Operand::immCheck(const_offs)) {
      const_offs_op = std::make_shared<Operand>(const_offs);
    } else {
      const_offs_op = std::make_shared<Operand>(OperandType::VREG);
      builder->appendMOV(const_offs_op, const_offs);
    }
  }

  if (!var_offs_op) {
    offs_op = const_offs_op;
  } else {
    offs_op = std::make_shared<Operand>(OperandType::VREG);
    builder->appendShift(InstOp::LSL, offs_op, var_offs_op,
                         std::make_shared<Operand>(2));
    if (const_offs) {
      builder->appendAS(InstOp::ADD, offs_op, offs_op, const_offs_op);
    }
  }

  // store the absolute address in register, then return it
  // actually, it would be better to return an offset form, like [Rn, Rm]
  // so that we can reduce the add instruction
  // TODO(Huang): optimize to offset form

  auto ret = builder->getOperand(inst);
  if (offs_op)
    builder->appendAS(InstOp::ADD, ret, getAddr(base_addr, builder), offs_op);
  else
    builder->appendMOV(ret, getAddr(base_addr, builder));

  return ret;
}

std::shared_ptr<Operand> GenerateLoad(std::shared_ptr<LoadInstruction> inst,
                                      std::shared_ptr<ASM_Builder> builder) {
  auto addr = inst->m_addr->getValue();
  std::shared_ptr<Operand> ret;
  ret = builder->getOperand(inst);
  builder->appendLDR(ret, getAddr(addr, builder), std::make_shared<Operand>(0));
  return ret;
}

std::shared_ptr<Operand> GenerateStore(std::shared_ptr<StoreInstruction> inst,
                                       std::shared_ptr<ASM_Builder> builder) {
  auto addr = inst->m_addr->getValue();
  auto val = inst->m_val->getValue();
  auto src = builder->getOperand(val);
  builder->appendSTR(src, getAddr(addr, builder), std::make_shared<Operand>(0));
  return nullptr;
}

std::shared_ptr<Operand> GenerateAlloca(std::shared_ptr<AllocaInstruction> inst,
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
    builder->appendMOV(offs, sp_offs);
  }
  auto ret = builder->getOperand(inst);
  builder->appendAS(InstOp::ADD, ret, Operand::getRReg(RReg::SP), offs);
  return ret;
}

std::shared_ptr<Operand> GeneratePhi(std::shared_ptr<PhiInstruction> inst,
                                     std::shared_ptr<ASM_Builder> builder) {
  // we need to store the temp result to make sure all PHI instruction execute
  // in parallel  (XXX)
#if 1
  auto tmp = std::make_shared<Operand>(OperandType::VREG);
  tmp->m_is_float = inst->m_type.IsBasicFloat();
  auto ret = builder->getOperand(inst);
  builder->appendMOV(ret, tmp);
  for (auto &[ir_block, value] : inst->m_contents) {
    std::shared_ptr<ASM_BasicBlock> block = builder->getBlock(ir_block);
    assert(block);
    if (!value) continue;
    auto src = builder->getOperand(value->getValue(), true, false, block);
    block->appendFilledMOV(std::make_shared<MOVInst>(tmp, src));
  }
  return ret;
#else
  // actually we don't have to store the temp result because of SSA form
  auto ret = builder->getOperand(inst);
  for (auto &[ir_block, value] : inst->m_contents) {
    std::shared_ptr<ASM_BasicBlock> block = builder->getBlock(ir_block);
    assert(block);
    if (!value) continue;
    auto src = builder->getOperand(value->getValue(), true, false);
    block->appendFilledMOV(std::make_shared<MOVInst>(ret, src));
  }
  return ret;
#endif
}

std::shared_ptr<Operand> GenerateBitCast(
    std::shared_ptr<BitCastInstruction> inst,
    std::shared_ptr<ASM_Builder> builder) {
  auto src_val = inst->m_val->getValue();
  auto ret = builder->getOperand(src_val);
  builder->m_value_map.insert(std::make_pair(inst, ret));
  return ret;
}

std::shared_ptr<Operand> GenerateZExt(std::shared_ptr<ZExtInstruction> inst,
                                      std::shared_ptr<ASM_Builder> builder) {
  auto src_val = inst->m_val->getValue();
  auto inst_cond = std::dynamic_pointer_cast<BinaryInstruction>(src_val);
  bool is_not = false;
  while (inst_cond->m_op == IROp::XOR) {
    is_not = !is_not;
    inst_cond = std::dynamic_pointer_cast<BinaryInstruction>(
        inst_cond->m_lhs_val_use->getValue());
  }
  CondType cond = GetCondFromIR(inst_cond->m_op);
  if (is_not) {
    cond = ASM_Instruction::getOppositeCond(cond);
  }
  assert(cond != CondType::NONE);
  if (builder->m_cur_block->m_status_load_inst) {
    builder->m_cur_block->insert(builder->m_cur_block->m_status_load_inst);
  }
  auto ret = builder->getOperand(inst);
  auto mov_true = builder->appendMOV(ret, 1);
  auto mov_false = builder->appendMOV(ret, 0);
  mov_true->m_cond = cond;
  mov_false->m_cond = ASM_Instruction::getOppositeCond(cond);
  return ret;
}

std::shared_ptr<Operand> GenerateSIToFP(std::shared_ptr<SIToFPInstruction> inst,
                                        std::shared_ptr<ASM_Builder> builder) {
  auto src_val = inst->m_val->getValue();
  auto src = builder->getOperand(src_val);
  auto ret = builder->getOperand(inst);
  ret->m_is_float = true;

  builder->appendMOV(Operand::getRReg(RReg::R0), src);
  builder->appendCALL(VarType::FLOAT, "__aeabi_i2f", 1);
  builder->appendMOV(ret, Operand::getRReg(RReg::R0));
  return ret;
}

std::shared_ptr<Operand> GenerateFPToSI(std::shared_ptr<FPToSIInstruction> inst,
                                        std::shared_ptr<ASM_Builder> builder) {
  auto src_val = inst->m_val->getValue();
  auto src = builder->getOperand(src_val);
  auto ret = builder->getOperand(inst);

  builder->appendMOV(Operand::getRReg(RReg::R0), src);
  builder->appendCALL(VarType::FLOAT, "__aeabi_f2iz", 1);
  builder->appendMOV(ret, Operand::getRReg(RReg::R0));
  return ret;
}

void GenerateInstruction(std::shared_ptr<Value> ir_value,
                         std::shared_ptr<ASM_Builder> builder) {
  if (auto value = std::dynamic_pointer_cast<Constant>(ir_value)) {
    builder->GenerateConstant(value, true, true);
  } else if (auto inst
             = std::dynamic_pointer_cast<BinaryInstruction>(ir_value)) {
    GenerateBinary(inst, builder);
  } else if (auto inst = std::dynamic_pointer_cast<FNegInstruction>(ir_value)) {
    GenerateFNeg(inst, builder);
  } else if (auto inst = std::dynamic_pointer_cast<CallInstruction>(ir_value)) {
    GenerateCall(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<BranchInstruction>(ir_value)) {
    GenerateBranch(inst, builder);
  } else if (auto inst = std::dynamic_pointer_cast<JumpInstruction>(ir_value)) {
    GenerateJump(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<ReturnInstruction>(ir_value)) {
    GenerateReturn(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<GetElementPtrInstruction>(ir_value)) {
    GenerateGetElementPtr(inst, builder);
  } else if (auto inst = std::dynamic_pointer_cast<LoadInstruction>(ir_value)) {
    GenerateLoad(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<StoreInstruction>(ir_value)) {
    GenerateStore(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<AllocaInstruction>(ir_value)) {
    GenerateAlloca(inst, builder);
  } else if (auto inst = std::dynamic_pointer_cast<PhiInstruction>(ir_value)) {
    GeneratePhi(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<BitCastInstruction>(ir_value)) {
    GenerateBitCast(inst, builder);
  } else if (auto inst = std::dynamic_pointer_cast<ZExtInstruction>(ir_value)) {
    GenerateZExt(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<SIToFPInstruction>(ir_value)) {
    GenerateSIToFP(inst, builder);
  } else if (auto inst
             = std::dynamic_pointer_cast<FPToSIInstruction>(ir_value)) {
    GenerateFPToSI(inst, builder);
  }
}

void GenerateBasicblock(std::shared_ptr<BasicBlock> ir_block,
                        std::shared_ptr<ASM_Builder> builder) {
  std::shared_ptr<ASM_BasicBlock> block = builder->getBlock(ir_block);
  builder->appendBlock(block);
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

  builder->m_cur_func->m_blocks.push_back(builder->m_cur_func->m_rblock);
  // insert MOV instruction(from PHI)
  for (auto &b : func->m_blocks) {
    b->fillMOV();
  }

  // insert mov instruction in the entry block for loading params
  auto first_block = func->m_blocks.front();
  auto iter = first_block->m_insts.begin();
  for (auto &inst : func->m_params_set_list) {
    if (inst->m_is_deleted) continue;
    first_block->m_insts.insert(iter, inst);
    func->m_params_pos_map[inst] = std::prev(iter);
    inst->m_block = first_block;
  }
}

void GenerateModule(std::shared_ptr<Module> ir_module,
                    std::shared_ptr<ASM_Builder> builder) {
  builder->setIrModule(ir_module);
  for (auto &f : ir_module->m_function_list) {
    GenerateFunction(f, builder);
  }
}