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

class ValueNameAllocator {
private:
  std::unordered_map<std::shared_ptr<Value>, std::string> m_name_of_global_vars;
  std::unordered_map<std::shared_ptr<Value>, std::string> m_name_of_labels;
  std::unordered_map<std::shared_ptr<Value>, std::string> m_name_of_local_vars;

  int m_local_var_cnt;
  int m_label_cnt;

public:
  explicit ValueNameAllocator()
      : m_name_of_global_vars(),
        m_name_of_labels(),
        m_name_of_local_vars(),
        m_local_var_cnt(0),
        m_label_cnt(0) {}

  void SetGlobalVarName(const std::shared_ptr<Value>& val, std::string name) {
    assert(m_name_of_global_vars.find(val) == m_name_of_global_vars.end());
    m_name_of_global_vars[val] = std::move(name);
  }

  // return in string
  // example: %1, %L1
  std::string GetValueName(const std::shared_ptr<Value>& val) {
    assert(val != nullptr);

    // check if the value is constant
    if (auto constant = std::dynamic_pointer_cast<Constant>(val)) {
      if (constant->m_is_float)
        return std::to_string(constant->m_float_val);
      else
        return std::to_string(constant->m_int_val);
    }

    if (val->m_type.m_base_type == BaseType::LABEL) {
      auto it = m_name_of_labels.find(val);
      if (it == m_name_of_labels.end())
        m_name_of_labels[val] = "%L" + std::to_string(m_label_cnt++);
      return m_name_of_labels[val];
    }
    // try finding in global variables
    auto it = m_name_of_global_vars.find(val);
    if (it != m_name_of_global_vars.end()) {
      return it->second;
    }

    auto it2 = m_name_of_local_vars.find(val);
    if (it2 == m_name_of_local_vars.end())
      m_name_of_local_vars[val] = "%" + std::to_string(m_local_var_cnt++);

    return m_name_of_local_vars[val];
  }

  void ClearLocalVariables() {
    m_name_of_local_vars.clear();
    m_local_var_cnt = 0;
  }
};

ValueNameAllocator g_allocator;

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
    // ofs << arg->ToString();
    arg->ExportIR(ofs, depth);
    if (!m_is_decl) {
      ofs << " " << g_allocator.GetValueName(arg);
    }
  }
  ofs << ")";
  if (!m_is_decl) {
    ofs << " {" << std::endl;
    // TODO(garen): print function body
    for (auto& bb : m_bb_list) {
      bb->ExportIR(ofs, depth + 1);
    }
    ofs << "}" << std::endl;
  } else {
    ofs << std::endl;
  }
  g_allocator.ClearLocalVariables();
}
void BasicBlock::ExportIR(std::ofstream& ofs, int depth) {
  ofs << g_allocator.GetValueName(shared_from_this()).substr(1) << ":"
      << std::endl;
  for (auto& instr : m_instr_list) {
    instr->ExportIR(ofs, depth);
    ofs << std::endl;
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
void IntGlobalVariable::ExportIR(std::ofstream& ofs, int depth) {
  // depth is useless here
  if (m_is_const) return;
  g_allocator.SetGlobalVarName(shared_from_this(), "@" + m_varname);
  if (m_init_vals.size() == 1) {
    ofs << "@" << m_varname << " = dso_local global i32 " << m_init_vals[0]
        << std::endl;
  } else {
    ofs << "@" << m_varname << " = dso_local global ";
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
  if (m_is_const) return;
  // depth is useless here
  assert(m_init_vals.size() == 1);
  g_allocator.SetGlobalVarName(shared_from_this(), "@" + m_varname);
  ofs << g_allocator.GetValueName(shared_from_this())
      << " = dso_local global float " << m_init_vals[0] << std::endl;
}
void BinaryInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // assert(m_lhs_val_use.m_value->m_type == ValueType::INT
  //        || m_lhs_val_use.m_value->m_type == ValueType::FLOAT);
  // assert(m_lhs_val_use.m_value->m_type == m_rhs_val_use.m_value->m_type);

  ofs << std::string(depth * 2, ' ');
  ofs << g_allocator.GetValueName(shared_from_this()) << " = ";
  ofs << BinaryOpToString(m_op) << " "
      << m_lhs_val_use.m_value->m_type.ToString() << " ";
  ofs << g_allocator.GetValueName(m_lhs_val_use.m_value) << ", "
      << g_allocator.GetValueName(m_rhs_val_use.m_value) << std::endl;
}
void CallInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  if (m_type.m_base_type != BaseType::VOID) {
    ofs << g_allocator.GetValueName(shared_from_this()) << " = ";
  }
  ofs << "call " << m_type.ToString() << " @" << m_func_name << "(";
  // params
  bool first = true;
  for (auto& param : m_params) {
    if (first)
      first = false;
    else
      ofs << ", ";

    ofs << param.m_value->m_type.ToString() << " "
        << g_allocator.GetValueName(param.m_value);
  }
  ofs << ")" << std::endl;
}
void BranchInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "br ";
  ofs << m_cond.m_value->m_type.ToString() << " "
      << g_allocator.GetValueName(m_cond.m_value) << ", ";
  ofs << m_true_block->m_type.ToString() << " "
      << g_allocator.GetValueName(m_true_block) << ", ";
  ofs << m_false_block->m_type.ToString() << " "
      << g_allocator.GetValueName(m_false_block) << std::endl;
}
void JumpInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "br ";
  ofs << m_target_block->m_type.ToString() << " "
      << g_allocator.GetValueName(m_target_block) << std::endl;
}
void ReturnInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  auto ret_val = m_ret.m_value;
  if (ret_val == nullptr) {
    ofs << "ret void" << std::endl;
  } else if (IsConstant()) {
    ofs << "ret ";
    ret_val->ExportIR(ofs, depth);
    ofs << std::endl;
  } else {
    // ret_val->ExportIR(ofs, depth);
    ofs << "ret " << ret_val->GetTypeString()
        << g_allocator.GetValueName(ret_val) << std::endl;
  }
}
void GetElementPtrInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << g_allocator.GetValueName(shared_from_this()) << " = ";
  ofs << "getelementptr " << m_addr->m_type.Dereference().ToString() << ", ";
  ofs << m_addr->m_type.ToString() << " " << g_allocator.GetValueName(m_addr)
      << ", ";
  bool first = true;
  for (const auto& val : m_indices) {
    if (first)
      first = false;
    else
      ofs << ", ";
    ofs << val->m_type.ToString() << " " << g_allocator.GetValueName(val);
  }
  ofs << std::endl;
}
void LoadInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << std::string(depth * 2, ' ');
  ofs << g_allocator.GetValueName(shared_from_this()) << " = ";
  // auto stack_addr = GetStackAddr(m_addr.m_value);
  auto stack_addr = m_addr.m_value;
  ofs << "load " << m_type.ToString() << ", " << stack_addr->m_type.ToString()
      << " " << g_allocator.GetValueName(stack_addr);
  ofs << std::endl;
}
void StoreInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  assert(m_val.m_value != nullptr);
  assert(m_addr.m_value != nullptr);

  ofs << std::string(depth * 2, ' ');
  ofs << "store " << m_val.m_value->m_type.ToString() << " "
      << g_allocator.GetValueName(m_val.m_value);
  // m_val.m_value->ExportIR(ofs, depth);
  ofs << ", " << m_addr.m_value->GetType().ToString() << " "
      << g_allocator.GetValueName(m_addr.m_value);
  ofs << std::endl;
}
void AllocaInstruction::ExportIR(std::ofstream& ofs, int depth) {
  assert(m_type.m_num_star > 0);
  ofs << std::string(depth * 2, ' ');

  ofs << g_allocator.GetValueName(shared_from_this()) << " = alloca "
      << m_type.Dereference().ToString() << std::endl;

  // TODO(garen): assign init vals
}
void PhiInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << "(TODO): PhiInstruction" << std::endl;
}
// NOT a complete instruction, cannot be invoked independently
void Constant::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  if (m_is_float) {
    ofs << "float " << m_float_val;
  } else {
    ofs << "i32 " << m_int_val;
  }
}
// void FunctionArg::ExportIR(std::ofstream& ofs, int depth) {
//   ofs << m_type.ToString();
// }
void ZExtInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << g_allocator.GetValueName(shared_from_this()) << " = zext ";
  ofs << m_val.m_value->m_type.ToString() << " "
      << g_allocator.GetValueName(m_val.m_value) << " to "
      << m_target_type.ToString() << std::endl;
}
void FunctionArg::ExportIR(std::ofstream& ofs, int depth) {
  // depth is useless here
  ofs << m_type.ToString();
}
