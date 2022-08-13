#include "ir/builder.h"

#include <iostream>

void IRBuilder::AppendBasicBlock(std::shared_ptr<BasicBlock> bb) {
  m_module->AppendBasicBlock(std::move(bb));
}
std::shared_ptr<Value> IRBuilder::GetIntConstant(int int_val) {
  auto it = m_module->m_const_ints.find(int_val);
  if (it != m_module->m_const_ints.end()) {
    return it->second;
  } else {
    auto c = std::make_shared<Constant>(int_val, BasicType::INT);
    m_module->m_const_ints[int_val] = c;
    return c;
  }
}
std::shared_ptr<Value> IRBuilder::GetFloatConstant(float float_val) {
  auto it = m_module->m_const_floats.find(float_val);
  if (it != m_module->m_const_floats.end()) {
    return it->second;
  } else {
    auto c = std::make_shared<Constant>(float_val);
    m_module->m_const_floats[float_val] = c;
    return c;
  }
}
std::shared_ptr<Value> IRBuilder::GetBoolConstant(bool bool_val) {
  auto it = m_module->m_const_bools.find(bool_val);
  if (it != m_module->m_const_bools.end()) {
    return it->second;
  } else {
    auto c = std::make_shared<Constant>(bool_val, BasicType::BOOL);
    m_module->m_const_bools[bool_val] = c;
    return c;
  }
}
std::shared_ptr<Value> IRBuilder::GetCharConstant(char char_val) {
  auto it = m_module->m_const_chars.find(char_val);
  if (it != m_module->m_const_chars.end()) {
    return it->second;
  } else {
    auto c = std::make_shared<Constant>(char_val, BasicType::CHAR);
    m_module->m_const_chars[char_val] = c;
    return c;
  }
}
// std::shared_ptr<Value> IRBuilder::GetIntConstant(
//     int int_val, std::unique_ptr<Module>& module) {
//   auto it = module->m_const_ints.find(int_val);
//   if (it != module->m_const_ints.end()) {
//     return it->second;
//   } else {
//     auto c = std::make_shared<Constant>(int_val, BasicType::INT);
//     module->m_const_ints[int_val] = c;
//     return c;
//   }
// }
// std::shared_ptr<Value> IRBuilder::GetFloatConstant(
//     float float_val, std::unique_ptr<Module>& module) {
//   auto it = module->m_const_floats.find(float_val);
//   if (it != module->m_const_floats.end()) {
//     return it->second;
//   } else {
//     auto c = std::make_shared<Constant>(float_val);
//     module->m_const_floats[float_val] = c;
//     return c;
//   }
// }
// std::shared_ptr<Value> IRBuilder::GetBoolConstant(
//     bool bool_val, std::unique_ptr<Module>& module) {
//   auto it = module->m_const_bools.find(bool_val);
//   if (it != module->m_const_bools.end()) {
//     return it->second;
//   } else {
//     auto c = std::make_shared<Constant>(bool_val, BasicType::BOOL);
//     module->m_const_floats[bool_val] = c;
//     return c;
//   }
// }
// std::shared_ptr<Value> IRBuilder::GetCharConstant(
//     char char_val, std::unique_ptr<Module>& module) {
//   auto it = module->m_const_chars.find(char_val);
//   if (it != module->m_const_chars.end()) {
//     return it->second;
//   } else {
//     auto c = std::make_shared<Constant>(char_val, BasicType::CHAR);
//     module->m_const_floats[char_val] = c;
//     return c;
//   }
// }

// only support binary operators
// lhs and rhs can be allowed to be int or float
std::shared_ptr<Value> IRBuilder::GetConstant(IROp op, EvalValue lhs,
                                              EvalValue rhs) {
  switch (op) {
    case IROp::ADD:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetIntConstant((lhs + rhs).IntVal());
    case IROp::F_ADD:
      assert(lhs.IsConstFloat() && rhs.IsConstFloat());
      return GetFloatConstant((lhs + rhs).FloatVal());
    case IROp::SUB:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetIntConstant((lhs - rhs).IntVal());
    case IROp::F_SUB:
      assert(lhs.IsConstFloat() && rhs.IsConstFloat());
      return GetFloatConstant((lhs - rhs).FloatVal());
    case IROp::MUL:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetIntConstant((lhs * rhs).IntVal());
    case IROp::F_MUL:
      assert(lhs.IsConstFloat() && rhs.IsConstFloat());
      return GetFloatConstant((lhs * rhs).FloatVal());
    case IROp::SDIV:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetIntConstant((lhs / rhs).IntVal());
    case IROp::F_DIV:
      assert(lhs.IsConstFloat() && rhs.IsConstFloat());
      return GetFloatConstant((lhs / rhs).FloatVal());
    case IROp::SREM:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetIntConstant((lhs % rhs).IntVal());
    case IROp::F_NEG:
      assert(lhs.IsConstFloat());
      return GetFloatConstant(-lhs.FloatVal());
      break;
    case IROp::I_SGE:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetBoolConstant(lhs.IntVal() >= rhs.IntVal());
    case IROp::I_SGT:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetBoolConstant(lhs.IntVal() > rhs.IntVal());
    case IROp::I_SLE:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetBoolConstant(lhs.IntVal() <= rhs.IntVal());
    case IROp::I_SLT:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetBoolConstant(lhs.IntVal() < rhs.IntVal());
    case IROp::I_EQ:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetBoolConstant(lhs.IntVal() == rhs.IntVal());
    case IROp::I_NE:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetBoolConstant(lhs.IntVal() != rhs.IntVal());
    case IROp::F_EQ:
      assert(lhs.IsConstFloat() && rhs.IsConstFloat());
      return GetBoolConstant(lhs.FloatVal() == rhs.FloatVal());
    case IROp::F_NE:
      assert(lhs.IsConstFloat() && rhs.IsConstFloat());
      return GetBoolConstant(lhs.FloatVal() != rhs.FloatVal());
    case IROp::F_GT:
      assert(lhs.IsConstFloat() && rhs.IsConstFloat());
      return GetBoolConstant(lhs.FloatVal() > rhs.FloatVal());
    case IROp::F_GE:
      assert(lhs.IsConstFloat() && rhs.IsConstFloat());
      return GetBoolConstant(lhs.FloatVal() >= rhs.FloatVal());
    case IROp::F_LT:
      assert(lhs.IsConstFloat() && rhs.IsConstFloat());
      return GetBoolConstant(lhs.FloatVal() < rhs.FloatVal());
    case IROp::F_LE:
      assert(lhs.IsConstFloat() && rhs.IsConstFloat());
      return GetBoolConstant(lhs.FloatVal() <= rhs.FloatVal());
    case IROp::XOR:
      assert(lhs.IsConstInt() && rhs.IsConstInt());
      return GetBoolConstant(lhs.IntVal() ^ rhs.IntVal());  // temp sol
    default:
      assert(false);  // uncovered
  }
}

std::shared_ptr<IntGlobalVariable> IRBuilder::CreateIntGlobalVariable(
    std::shared_ptr<DeclAST> decl) {
  std::vector<int> flatten_vals;

  for (const auto& expr : decl->m_flatten_vals) {
    if (expr == nullptr) {
      flatten_vals.push_back(0);
    } else {
      flatten_vals.push_back(expr->IntVal());
    }
  }

  auto global_variable = std::make_shared<IntGlobalVariable>(std::move(decl));
  m_module->AppendGlobalVariable(global_variable);
  return global_variable;
}
std::shared_ptr<FloatGlobalVariable> IRBuilder::CreateFloatGlobalVariable(
    std::shared_ptr<DeclAST> decl) {
  std::vector<float> flatten_vals;

  for (const auto& expr : decl->m_flatten_vals) {
    if (expr == nullptr) {
      flatten_vals.push_back(0.0);
    } else {
      flatten_vals.push_back(expr->FloatVal());
    }
  }
  auto global_variable = std::make_shared<FloatGlobalVariable>(std::move(decl));
  m_module->AppendGlobalVariable(global_variable);
  return global_variable;
}
std::shared_ptr<GlobalVariable> IRBuilder::CreateGlobalVariable(
    const std::shared_ptr<DeclAST>& decl) {
  if (decl->GetVarType() == VarType::INT) {
    auto addr = CreateIntGlobalVariable(decl);
    decl->m_addr = addr;
    return addr;
  } else if (decl->GetVarType() == VarType::FLOAT) {
    auto addr = CreateFloatGlobalVariable(decl);
    decl->m_addr = addr;
    return addr;
  } else
    assert(false);  // unreachable
}
std::shared_ptr<Instruction> IRBuilder::CreateReturnInstruction(
    VarType expected_type, std::shared_ptr<Value> ret) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<ReturnInstruction>(bb);
  if (ret != nullptr) {
    assert(ret->m_type.IsBasicInt() || ret->m_type.IsBasicFloat());
    if (expected_type == VarType::INT && ret->m_type.IsBasicFloat()) {
      // TODO(garen): can be optimized
      ret = CreateFPToSIInstruction(ret);
    } else if (expected_type == VarType::FLOAT && ret->m_type.IsBasicInt()) {
      // TODO(garen): can be optimized
      ret = CreateSIToFPInstruction(ret);
    }
    instr->m_ret = ret->AddUse(instr);
  } else {
    assert(expected_type == VarType::VOID);
  }
  // instr->m_ret->InitUser(instr);
  bb->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<BasicBlock> IRBuilder::CreateBasicBlock(
    std::string block_name) {
  auto bb = std::make_shared<BasicBlock>(std::move(block_name));
  AppendBasicBlock(bb);
  return bb;
}
std::shared_ptr<Instruction> IRBuilder::CreateFNegInstruction(
    const std::shared_ptr<Value>& lhs) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<FNegInstruction>(bb);
  instr->m_lhs_val_use = lhs->AddUse(instr);
  bb->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<Instruction> IRBuilder::CreateBinaryInstruction(
    IROp op, const std::shared_ptr<Value>& lhs,
    const std::shared_ptr<Value>& rhs) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<BinaryInstruction>(op, bb);
  instr->m_lhs_val_use = lhs->AddUse(instr);
  instr->m_rhs_val_use = rhs->AddUse(instr);

  auto lhs_type = instr->m_lhs_val_use->getValue()->m_type,
       rhs_type = instr->m_rhs_val_use->getValue()->m_type;
  // consider these situations:
  // i32 and float
  // i32 and i1
  // float and i1
  if (lhs_type.IsBasicBool() && rhs_type.IsBasicInt()) {
    // i1 vs i32
    // zext for lhs
    auto new_lhs = CreateZExtInstruction(instr->m_lhs_val_use->getValue());
    instr->m_lhs_val_use->getValue()->KillUse(instr->m_lhs_val_use);
    instr->m_lhs_val_use = new_lhs->AddUse(instr);
  } else if (lhs_type.IsBasicInt() && rhs_type.IsBasicBool()) {
    // i32 vs i1
    // zext for rhs
    auto new_rhs = CreateZExtInstruction(instr->m_rhs_val_use->getValue());
    instr->m_rhs_val_use->getValue()->KillUse(instr->m_rhs_val_use);
    instr->m_rhs_val_use = new_rhs->AddUse(instr);
  } else if (lhs_type.IsBasicInt() && rhs_type.IsBasicFloat()) {
    // i32 vs float
    // sitofp for lhs
    auto new_lhs = CreateSIToFPInstruction(instr->m_lhs_val_use->getValue());
    instr->m_lhs_val_use->getValue()->KillUse(instr->m_lhs_val_use);
    instr->m_lhs_val_use = new_lhs->AddUse(instr);
  } else if (lhs_type.IsBasicFloat() && rhs_type.IsBasicInt()) {
    // float vs i32
    // sitofp for rhs
    auto new_rhs = CreateSIToFPInstruction(instr->m_rhs_val_use->getValue());
    instr->m_rhs_val_use->getValue()->KillUse(instr->m_rhs_val_use);
    instr->m_rhs_val_use = new_rhs->AddUse(instr);
  } else if (lhs_type.IsBasicFloat() && rhs_type.IsBasicBool()) {
    // float vs i1
    // i1 -> i32 -> float
    // zext and sitofp for rhs

    auto zext = CreateZExtInstruction(instr->m_rhs_val_use->getValue());
    auto sitofp = CreateSIToFPInstruction(zext);
    instr->m_rhs_val_use->getValue()->KillUse(instr->m_rhs_val_use);
    instr->m_rhs_val_use = sitofp->AddUse(instr);
  } else if (lhs_type.IsBasicBool() && rhs_type.IsBasicFloat()) {
    // i1 vs float
    // i1 -> i32 -> float
    // zext and sitofp for lhs

    auto zext = CreateZExtInstruction(instr->m_lhs_val_use->getValue());
    auto sitofp = CreateSIToFPInstruction(zext);
    instr->m_lhs_val_use->getValue()->KillUse(instr->m_lhs_val_use);
    instr->m_lhs_val_use = sitofp->AddUse(instr);
  } else if (lhs_type != rhs_type) {
    assert(false);  // not considered yet
  }
  bb->PushBackInstruction(instr);
  return instr;
}

std::shared_ptr<CallInstruction> IRBuilder::CreateCallInstruction(
    VarType return_type, std::string func_name,
    std::vector<std::shared_ptr<Value>> params_values,
    std::shared_ptr<Function> function) {
  if (function == nullptr) {
    // find function
    auto it = std::find_if(m_module->m_function_decl_list.begin(),
                           m_module->m_function_decl_list.end(),
                           [=](const std::shared_ptr<Function>& ptr) -> bool {
                             return ptr->FuncName() == func_name;
                           });
    if (it == m_module->m_function_decl_list.end()) {
      it = std::find_if(m_module->m_function_list.begin(),
                        m_module->m_function_list.end(),
                        [=](const std::shared_ptr<Function>& ptr) -> bool {
                          return ptr->FuncName() == func_name;
                        });
      assert(it != m_module->m_function_list.end());
    }
    function = *it;
  }
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<CallInstruction>(return_type,
                                                 std::move(func_name), bb);
  std::vector<Use*> params_uses;
  for (auto& val : params_values) {
    auto use = val->AddUse(instr);
    params_uses.push_back(use);
  }
  instr->m_function = function;
  // for (auto& param_use : params_uses) {
  //   param_use->InitUser(instr);
  // }
  instr->SetParams(std::move(params_uses));
  bb->PushBackInstruction(instr);
  return instr;
}

std::shared_ptr<CallInstruction> IRBuilder::CreateCallInstruction(
    VarType return_type, std::string func_name,
    const std::vector<std::shared_ptr<ExprAST>>& params) {
  auto it = std::find_if(m_module->m_function_decl_list.begin(),
                         m_module->m_function_decl_list.end(),
                         [=](const std::shared_ptr<Function>& ptr) -> bool {
                           return ptr->FuncName() == func_name;
                         });
  if (it == m_module->m_function_decl_list.end()) {
    it = std::find_if(m_module->m_function_list.begin(),
                      m_module->m_function_list.end(),
                      [=](const std::shared_ptr<Function>& ptr) -> bool {
                        return ptr->FuncName() == func_name;
                      });
    assert(it != m_module->m_function_list.end());
  }
  auto function = *it;
  assert(params.size() == function->m_args.size());

  std::vector<std::shared_ptr<Value>> params_values;

  for (int i = 0; i < params.size(); ++i) {
    auto val = params[i]->CodeGen(shared_from_this());

    // fat pointer to thin pointer
    if (val->m_type.m_dimensions.size()
            > function->m_args[i]->m_type.m_dimensions.size()
        && val->m_type.m_num_star == function->m_args[i]->m_type.m_num_star) {
      assert(val->m_type.m_dimensions.size()
             == function->m_args[i]->m_type.m_dimensions.size() + 1);
      std::vector<std::shared_ptr<Value>> gep_params(
          val->m_type.m_dimensions.size() + 1
              - function->m_args[i]->m_type.m_dimensions.size(),
          GetIntConstant(0));
      val = CreateGetElementPtrInstruction(val, std::move(gep_params));
    }
    // recursively pass an array as parameter
    if (val->m_type.m_dimensions == function->m_args[i]->m_type.m_dimensions
        && val->m_type.m_num_star > function->m_args[i]->m_type.m_num_star) {
      assert(val->m_type.m_num_star
             == function->m_args[i]->m_type.m_num_star + 1);
      val = CreateLoadInstruction(val);
    }

    if (val->m_type.IsBasicInt()
        && function->m_args[i]->m_type.IsBasicFloat()) {
      // sitofp
      val = CreateSIToFPInstruction(val);
    } else if (val->m_type.IsBasicFloat()
               && function->m_args[i]->m_type.IsBasicInt()) {
      val = CreateFPToSIInstruction(val);
    }

    assert(val->m_type == function->m_args[i]->m_type);

    params_values.push_back(val);
  }
  return CreateCallInstruction(return_type, std::move(func_name),
                               std::move(params_values), function);
}
std::shared_ptr<Instruction> IRBuilder::CreateJumpInstruction(
    std::shared_ptr<BasicBlock> block) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<JumpInstruction>(block, bb);
  bb->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<Instruction> IRBuilder::CreateBranchInstruction(
    std::shared_ptr<Value> cond_val, std::shared_ptr<BasicBlock> true_block,
    std::shared_ptr<BasicBlock> false_block) {
  auto bb = m_module->GetCurrentBB();
  if (!cond_val->m_type.IsBasicBool()) {
    if (cond_val->m_type.IsBasicInt()) {
      auto new_cond_val
          = CreateBinaryInstruction(IROp::I_NE, cond_val, GetIntConstant(0));
      auto instr = std::make_shared<BranchInstruction>(
          std::move(true_block), std::move(false_block), bb);
      instr->m_cond = new_cond_val->AddUse(instr);
      bb->PushBackInstruction(instr);
      return instr;
    } else if (cond_val->m_type.IsBasicFloat()) {
      auto new_cond_val = CreateBinaryInstruction(IROp::F_NE, cond_val,
                                                  GetFloatConstant(0.0));
      auto instr = std::make_shared<BranchInstruction>(
          std::move(true_block), std::move(false_block), bb);
      instr->m_cond = new_cond_val->AddUse(instr);
      bb->PushBackInstruction(instr);
      return instr;
    } else {
      assert(false);  // ???
    }
  } else {
    auto instr = std::make_shared<BranchInstruction>(
        std::move(true_block), std::move(false_block), bb);
    instr->m_cond = cond_val->AddUse(instr);
    bb->PushBackInstruction(instr);
    return instr;
  }
}
// SPECIAL
std::shared_ptr<Instruction> IRBuilder::CreateStoreInstruction(
    std::shared_ptr<Value> addr, std::shared_ptr<Value> val) {
  // maybe store int to float
  if (addr->m_type.Equals(BasicType::FLOAT, true)
      && val->m_type.Equals(BasicType::INT)) {
    if (auto con = std::dynamic_pointer_cast<Constant>(val)) {
      val = GetFloatConstant(con->m_int_val);
    } else {
      val = CreateSIToFPInstruction(val);
    }
  } else if (addr->m_type.Equals(BasicType::INT, true)
             && val->m_type.Equals(BasicType::FLOAT)) {
    if (auto con = std::dynamic_pointer_cast<Constant>(val)) {
      val = GetIntConstant(con->m_float_val);
    } else {
      val = CreateFPToSIInstruction(val);
    }
  }
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<StoreInstruction>(bb);
  instr->m_addr = addr->AddUse(instr);
  instr->m_val = val->AddUse(instr);
  bb->PushBackInstruction(instr);
  return instr;
}
// SPECIAL
std::shared_ptr<Instruction> IRBuilder::CreateGetElementPtrInstruction(
    std::shared_ptr<Value> addr, std::vector<std::shared_ptr<Value>> indices) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<GetElementPtrInstruction>(bb);
  instr->m_addr = addr->AddUse(instr);
  for (auto& index : indices) {
    instr->m_indices.push_back(index->AddUse(instr));
  }

  instr->m_type.Set(addr->m_type.m_base_type, true);
  instr->m_type.m_dimensions.clear();
  // TODO(garen): maybe wrong here
  for (size_t i = instr->m_indices.size() - 1;
       i < addr->m_type.m_dimensions.size(); ++i) {
    instr->m_type.m_dimensions.push_back(addr->m_type.m_dimensions[i]);
  }
  bb->PushBackInstruction(instr);
  return instr;
}
// SPECIAL
std::shared_ptr<Instruction> IRBuilder::CreateLoadInstruction(
    std::shared_ptr<Value> addr) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<LoadInstruction>(bb);
  instr->m_addr = addr->AddUse(instr);
  instr->m_type = addr->m_type.Dereference();
  m_module->GetCurrentBB()->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<Function> IRBuilder::CreateFunction(
    std::shared_ptr<FuncDefAST> func_ast) {
  auto func = std::make_shared<Function>(func_ast);
  m_module->AppendFunction(func);
  return func;
}
std::shared_ptr<Instruction> IRBuilder::CreateAllocaInstruction(
    std::shared_ptr<DeclAST> decl, bool is_arg, bool is_const) {
  auto bb = m_module->GetCurrentBB();

  if (decl->IsArray()) {
    std::vector<int> dimensions;
    int ptr_cnt = 1;
    if (decl->m_dimensions[0] == nullptr) {
      ++ptr_cnt;
    } else
      dimensions.push_back(decl->m_dimensions[0]->IntVal());

    for (int i = 1; i < decl->m_dimensions.size(); ++i) {
      dimensions.push_back(decl->m_dimensions[i]->IntVal());
    }
    auto value_type
        = ValueType(decl->GetVarType(), std::move(dimensions), ptr_cnt);
    // if (decl->IsParam()) {
    //   value_type = value_type.Reduce(1).Reference(1);
    // }
    auto instr
        = std::make_shared<AllocaInstruction>(value_type, bb, is_arg, is_const);
    bb->PushBackInstruction(instr);
    return instr;
  } else {
    ValueType value_type(decl->GetVarType(), 1);
    auto instr
        = std::make_shared<AllocaInstruction>(value_type, bb, is_arg, is_const);
    bb->PushBackInstruction(instr);
    return instr;
  }
}

std::shared_ptr<Value> IRBuilder::CreateBitCastInstruction(
    std::shared_ptr<Value> from, BasicType target_type) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<BitCastInstruction>(target_type, bb);
  instr->m_val = from->AddUse(instr);
  bb->PushBackInstruction(instr);
  return instr;
}

std::shared_ptr<Value> IRBuilder::CreateZExtInstruction(
    std::shared_ptr<Value> from) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<ZExtInstruction>(bb);
  instr->m_val = from->AddUse(instr);
  bb->PushBackInstruction(instr);
  return instr;
}

std::shared_ptr<Value> IRBuilder::CreateSIToFPInstruction(
    std::shared_ptr<Value> from) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<SIToFPInstruction>(bb);
  instr->m_val = from->AddUse(instr);
  bb->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<Value> IRBuilder::CreateFPToSIInstruction(
    std::shared_ptr<Value> from) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<FPToSIInstruction>(bb);
  instr->m_val = from->AddUse(instr);
  bb->PushBackInstruction(instr);
  return instr;
}

// automatically insert phi instruction to the front of bb list
std::shared_ptr<PhiInstruction> IRBuilder::CreatePhiInstruction(
    ValueType type, std::shared_ptr<BasicBlock> bb) {
  auto instr = std::make_shared<PhiInstruction>(type, bb);
  // Phi instruction does not initialize users?
  // of course, cuz users will be added when adding operands in m_contents
  bb->PushFrontInstruction(instr);
  return instr;
}

void IRBuilder::CreateMemsetInstruction(std::shared_ptr<Value> first_elem,
                                        int size) {
  auto first = CreateBitCastInstruction(first_elem, BasicType::CHAR);
  std::vector<std::shared_ptr<Value>> params
      = {first, GetCharConstant(0), GetIntConstant(4 * size),
         GetBoolConstant(false)};
  CreateCallInstruction(VarType::VOID, "llvm.memset.p0i8.i32",
                        std::move(params));
}
