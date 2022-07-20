#include "ir/ir-name-allocator.h"
#include "ir/ir.h"

// export LLVM IR
std::string VarTypeToString(VarType var_type) {
  switch (var_type) {
    case VarType::INT:
      return "i32";
    case VarType::FLOAT:
      return "float";
    case VarType::VOID:
      return "void";
    default:
      assert(false);  // unreachable
  }
}

std::string BinaryOpToString(IROp op) {
  switch (op) {
    case IROp::ADD:
      return "add";
    case IROp::SUB:
      return "sub";
    case IROp::MUL:
      return "mul";
    case IROp::SDIV:
      return "sdiv";
    case IROp::SREM:
      return "srem";
    case IROp::SGEQ:
      return "icmp sge";
    case IROp::SGE:
      return "icmp sgt";
    case IROp::SLEQ:
      return "icmp sle";
    case IROp::SLE:
      return "icmp slt";
    case IROp::EQ:
      return "icmp eq";
    case IROp::NE:
      return "icmp ne";
    default:
      assert(false);  // unreachable
  }
}

void PrintGlobalArrayValues(std::ofstream& ofs, int n, int& offset,
                            const std::vector<int>& sizes,
                            const std::shared_ptr<IntGlobalVariable>& gv) {
  ofs << gv->m_type.Dereference().Reduce(n).ToString() << " ";
  if (n == gv->m_type.m_dimensions.size()) {
    ofs << gv->m_init_vals[offset++];
  } else {
    ofs << "[";
    bool first = true;
    for (int i = 0; i < gv->m_type.m_dimensions[n]; ++i) {
      if (first)
        first = false;
      else
        ofs << ", ";
      PrintGlobalArrayValues(ofs, n + 1, offset, sizes, gv);
    }
    ofs << "]";
  }
}
// allocate name
// export ir

void Module::AllocateName(std::shared_ptr<IRNameAllocator> allocator) {
  for (auto& global_variable : m_global_variable_list) {
    global_variable->AllocateName(allocator);
  }
  for (auto& func_decl : m_function_decl_list) {
    func_decl->AllocateName(allocator);
  }
  for (auto& func_def : m_function_list) {
    func_def->AllocateName(allocator);
  }
}
void Module::ExportIR(std::ofstream& ofs, int depth) {
  for (auto& global_variable : m_global_variable_list) {
    global_variable->ExportIR(ofs, depth);
  }
  for (auto& func_decl : m_function_decl_list) {
    func_decl->ExportIR(ofs, depth);
  }
  for (auto& func_def : m_function_list) {
    func_def->ExportIR(ofs, depth);
  }
}
void Function::AllocateName(std::shared_ptr<IRNameAllocator> allocator) {
  if (m_is_decl) return;  // no need to allocate names
  for (auto& arg : m_args) {
    arg->AllocateName(allocator);
  }
  for (auto& bb : m_bb_list) {
    bb->AllocateName(allocator);
  }
  allocator->ClearLocalVariables();
}
void Function::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  if (m_is_decl) {
    ofs << "declare ";
  } else {
    ofs << "define dso_local ";
  }
  ofs << VarTypeToString(ReturnType());
  ofs << " @" << FuncName() << "(";
  bool first = true;
  for (auto& arg : m_args) {
    if (first)
      first = false;
    else
      ofs << ", ";
    arg->ExportIR(ofs, depth);
  }
  ofs << ")";
  if (!m_is_decl) {
    ofs << " {" << std::endl;
    // print function body
    for (auto& bb : m_bb_list) {
      bb->ExportIR(ofs, depth + 1);
    }
    ofs << "}" << std::endl;
  } else {
    ofs << std::endl;
  }
}
void BasicBlock::AllocateName(std::shared_ptr<IRNameAllocator> allocator) {
  allocator->GetValueName(shared_from_this());
  for (auto& instr : m_instr_list) {
    instr->AllocateName(allocator);
  }
}
void BasicBlock::ExportIR(std::ofstream& ofs, int depth) {
  ofs << m_allocated_name.substr(1) << ": ";
  ofs << "; " << m_name << std::endl;
  for (auto& instr : m_instr_list) {
    instr->ExportIR(ofs, depth);
    ofs << std::endl;
  }
}
void GlobalVariable::AllocateName(std::shared_ptr<IRNameAllocator> allocator) {
  allocator->SetGlobalVarName(shared_from_this(), "@" + m_name);
}
void IntGlobalVariable::ExportIR(std::ofstream& ofs, int depth) {
  // depth is useless here
  if (m_is_const) return;
  // g_allocator.SetGlobalVarName(shared_from_this(), "@" + m_allocated_name);
  if (m_init_vals.size() == 1) {
    ofs << m_allocated_name << " = dso_local global i32 " << m_init_vals[0]
        << std::endl;
  } else {
    ofs << m_allocated_name << " = dso_local global ";
    int offset = 0;
    int tot = 1;
    std::vector<int> sizes(m_type.m_dimensions.size());
    for (int i = sizes.size() - 1; i >= 0; --i) {
      tot *= m_type.m_dimensions[i];
      sizes[i] = tot;
    }
    PrintGlobalArrayValues(ofs, 0, offset, sizes,
                           shared_from_base<IntGlobalVariable>());
    ofs << std::endl;
  }
}
void FloatGlobalVariable::ExportIR(std::ofstream& ofs, int depth) {
  assert(false);  // TODO
  if (m_is_const) return;
  // depth is useless here
  assert(m_init_vals.size() == 1);
  // g_allocator.SetGlobalVarName(shared_from_this(), "@" + m_allocated_name);
  ofs << m_allocated_name << " = dso_local global float " << m_init_vals[0]
      << std::endl;
}
void BinaryInstruction::AllocateName(
    std::shared_ptr<IRNameAllocator> allocator) {
  allocator->GetValueName(shared_from_this());
  allocator->GetValueName(m_lhs_val_use->m_value);
  allocator->GetValueName(m_rhs_val_use->m_value);
}
void BinaryInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // assert(m_lhs_val_use.m_value->m_type == ValueType::INT
  //        || m_lhs_val_use.m_value->m_type == ValueType::FLOAT);
  // assert(m_lhs_val_use.m_value->m_type == m_rhs_val_use.m_value->m_type);

  ofs << std::string(depth * 2, ' ');
  ofs << m_allocated_name << " = ";
  ofs << BinaryOpToString(m_op) << " "
      << m_lhs_val_use->m_value->m_type.ToString() << " ";
  ofs << m_lhs_val_use->m_value->m_allocated_name << ", "
      << m_rhs_val_use->m_value->m_allocated_name << std::endl;
}
void CallInstruction::AllocateName(std::shared_ptr<IRNameAllocator> allocator) {
  if (m_type.m_base_type != BaseType::VOID) {
    allocator->GetValueName(shared_from_this());
  }

  for (auto& param : m_params) {
    // since it may be constant, otherwise constant is empty
    allocator->GetValueName(param->m_value);
  }
}
void CallInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  if (m_type.m_base_type != BaseType::VOID) {
    ofs << m_allocated_name << " = ";
  }
  ofs << "call " << m_type.ToString() << " @" << m_func_name << "(";
  // params
  bool first = true;
  for (auto& param : m_params) {
    if (first)
      first = false;
    else
      ofs << ", ";

    ofs << param->m_value->m_type.ToString() << " "
        << param->m_value->m_allocated_name;
  }
  ofs << ")" << std::endl;
}
void BranchInstruction::AllocateName(
    std::shared_ptr<IRNameAllocator> allocator) {
  allocator->GetValueName(m_cond->m_value);
  // allocator->GetValueName(m_true_block);
  // allocator->GetValueName(m_false_block);
}
void BranchInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "br ";
  ofs << m_cond->m_value->m_type.ToString() << " "
      << m_cond->m_value->m_allocated_name << ", ";
  ofs << m_true_block->m_type.ToString() << " "
      << m_true_block->m_allocated_name << ", ";
  ofs << m_false_block->m_type.ToString() << " "
      << m_false_block->m_allocated_name << std::endl;
}
void JumpInstruction::AllocateName(std::shared_ptr<IRNameAllocator> allocator) {
  // allocator->GetValueName(m_target_block);
}
void JumpInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "br ";
  ofs << m_target_block->m_type.ToString() << " "
      << m_target_block->m_allocated_name << std::endl;
}
void ReturnInstruction::AllocateName(
    std::shared_ptr<IRNameAllocator> allocator) {
  if (m_ret != nullptr && m_ret->m_value != nullptr) {
    allocator->GetValueName(m_ret->m_value);
  }
}
void ReturnInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  if (m_ret == nullptr || m_ret->m_value == nullptr) {
    ofs << "ret void" << std::endl;
  } else if (IsConstant()) {
    ofs << "ret " << m_ret->m_value->m_type.ToString() << " ";
    m_ret->m_value->ExportIR(ofs, depth);
    ofs << std::endl;
  } else {
    // ret_val->ExportIR(ofs, depth);
    ofs << "ret " << m_ret->m_value->m_type.ToString() << " "
        << m_ret->m_value->m_allocated_name << std::endl;
  }
}
void GetElementPtrInstruction::AllocateName(
    std::shared_ptr<IRNameAllocator> allocator) {
  allocator->GetValueName(shared_from_this());
  allocator->GetValueName(m_addr->m_value);
  for (const auto& index : m_indices) {
    allocator->GetValueName(index->m_value);
  }
}
void GetElementPtrInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << m_allocated_name << " = ";
  ofs << "getelementptr " << m_addr->m_value->m_type.Dereference().ToString()
      << ", ";
  ofs << m_addr->m_value->m_type.ToString() << " "
      << m_addr->m_value->m_allocated_name << ", ";
  bool first = true;
  for (const auto& index : m_indices) {
    if (first)
      first = false;
    else
      ofs << ", ";
    ofs << index->m_value->m_type.ToString() << " "
        << index->m_value->m_allocated_name;
  }
  ofs << std::endl;
}
void LoadInstruction::AllocateName(std::shared_ptr<IRNameAllocator> allocator) {
  allocator->GetValueName(shared_from_this());
  allocator->GetValueName(m_addr->m_value);
}
void LoadInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << m_allocated_name << " = ";
  ofs << "load " << m_type.ToString() << ", "
      << m_addr->m_value->m_type.ToString() << " "
      << m_addr->m_value->m_allocated_name;
  ofs << std::endl;
}
void StoreInstruction::AllocateName(
    std::shared_ptr<IRNameAllocator> allocator) {
  allocator->GetValueName(m_val->m_value);
  allocator->GetValueName(m_addr->m_value);
}
void StoreInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  assert(m_val->m_value != nullptr);
  assert(m_addr->m_value != nullptr);

  ofs << std::string(depth * 2, ' ');
  ofs << "store " << m_val->m_value->m_type.ToString() << " "
      << m_val->m_value->m_allocated_name;
  // m_val.m_value->ExportIR(ofs, depth);
  ofs << ", " << m_addr->m_value->GetType().ToString() << " "
      << m_addr->m_value->m_allocated_name;
  ofs << std::endl;
}
void AllocaInstruction::AllocateName(
    std::shared_ptr<IRNameAllocator> allocator) {
  allocator->GetValueName(shared_from_this());
}
void AllocaInstruction::ExportIR(std::ofstream& ofs, int depth) {
  assert(m_type.m_num_star > 0);
  ofs << std::string(depth * 2, ' ');

  ofs << m_allocated_name << " = alloca " << m_type.Dereference().ToString()
      << std::endl;

  // init vals are not initialized here
}
void PhiInstruction::AllocateName(std::shared_ptr<IRNameAllocator> allocator) {
  allocator->GetValueName(shared_from_this());
  // only itself, no need to generate its incoming values (some still not
  // appeared yet)
}
void PhiInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << std::string(depth * 2, ' ');
  ofs << m_allocated_name << " = phi ";
  ofs << "i32 ";  // TODO(garen): float
  bool first = true;
  for (auto& [bb, incoming_val] : m_contents) {
    if (first)
      first = false;
    else
      ofs << ", ";
    ofs << "[ ";
    if (incoming_val->m_value->m_allocated_name.empty()) {
      incoming_val->m_value->ExportIR(ofs, depth);  // name not generated
    } else {
      ofs << incoming_val->m_value->m_allocated_name;
    }
    ofs << ", " << bb->m_allocated_name << " ]";
  }
  ofs << std::endl;
}
void Constant::AllocateName(std::shared_ptr<IRNameAllocator> allocator) {
  // do nothing here
}
// NOT a complete instruction, cannot be invoked independently
void Constant::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  if (m_is_float) {
    ofs << m_float_val;
  } else {
    ofs << m_int_val;
  }
}
void ZExtInstruction::AllocateName(std::shared_ptr<IRNameAllocator> allocator) {
  allocator->GetValueName(shared_from_this());
  allocator->GetValueName(m_val->m_value);
}
void ZExtInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << m_allocated_name << " = zext ";
  ofs << m_val->m_value->m_type.ToString() << " "
      << m_val->m_value->m_allocated_name << " to " << m_target_type.ToString()
      << std::endl;
}

void FunctionArg::AllocateName(std::shared_ptr<IRNameAllocator> allocator) {
  if (!m_is_decl) {
    allocator->GetValueName(shared_from_this());
  }
}
void FunctionArg::ExportIR(std::ofstream& ofs, int depth) {
  // depth is useless here
  // only Function can know whether declaration or definition
  ofs << m_type.ToString();
  if (!m_is_decl) {
    ofs << " " << m_allocated_name;
  }
}
