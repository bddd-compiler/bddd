#include <iomanip>

#include "ir/ir-name-allocator.h"
#include "ir/ir.h"

IRNameAllocator g_reg_allocator, g_label_allocator;

// export LLVM IR
std::string VarTypeToString(VarType var_type) {
  switch (var_type) {
    case VarType::INT:
      return "i32";
    case VarType::FLOAT:
      return "float";
    case VarType::VOID:
      return "void";
    case VarType::CHAR:
      return "i8";
    case VarType::BOOL:
      return "i1";
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
    case IROp::I_SGE:
      return "icmp sge";
    case IROp::I_SGT:
      return "icmp sgt";
    case IROp::I_SLE:
      return "icmp sle";
    case IROp::I_SLT:
      return "icmp slt";
    case IROp::I_EQ:
      return "icmp eq";
    case IROp::I_NE:
      return "icmp ne";
      // default:
      //   assert(false);  // unreachable
    case IROp::F_ADD:
      return "fadd";
    case IROp::F_SUB:
      return "fsub";
    case IROp::F_MUL:
      return "fmul";
    case IROp::F_DIV:
      return "fdiv";
    case IROp::F_EQ:
      return "fcmp oeq";
    case IROp::F_NE:
      return "fcmp one";
    case IROp::F_GT:
      return "fcmp ogt";
    case IROp::F_GE:
      return "fcmp oge";
    case IROp::F_LT:
      return "fcmp olt";
    case IROp::F_LE:
      return "fcmp ole";
    case IROp::XOR:
      return "xor";
    default:
      assert(false);  // not binary operator
  }
}

void PrintGlobalIntArrayValues(std::ofstream& ofs, int n, int& offset,
                               std::unique_ptr<InitValAST>& init_val,
                               const std::vector<int>& sizes,
                               const std::shared_ptr<IntGlobalVariable>& gv) {
  ofs << gv->m_type.Dereference().Reduce(n) << " ";
  if (init_val == nullptr || init_val->AllZero()) {
    if (n == gv->m_type.m_dimensions.size()) {
      ofs << gv->m_flatten_vals[offset++];
    } else {
      ofs << "zeroinitializer";
      int tot = 1;
      for (int i = n; i < gv->m_type.m_dimensions.size(); ++i) {
        tot *= gv->m_type.m_dimensions[i];
      }
      offset += tot;
    }
  } else {
    if (n == gv->m_type.m_dimensions.size()) {
      ofs << gv->m_flatten_vals[offset++];
    } else {
      ofs << "[";
      assert(init_val->m_vals.size() <= gv->m_type.m_dimensions[n]);
      bool first = true;
      for (int i = 0; i < gv->m_type.m_dimensions[n]; ++i) {
        if (first)
          first = false;
        else
          ofs << ", ";
        if (i < init_val->m_vals.size()) {
          // defined in init_val
          PrintGlobalIntArrayValues(ofs, n + 1, offset, init_val->m_vals[i],
                                    sizes, gv);
        } else {
          // default zero
          if (n + 1 == gv->m_type.m_dimensions.size()) {
            ofs << "i32 0";
            ++offset;
          } else {
            ofs << gv->m_type.Dereference().Reduce(n + 1) << " ";
            ofs << "zeroinitializer";
            int tot = 1;
            for (int i = n + 1; i < gv->m_type.m_dimensions.size(); ++i) {
              tot *= gv->m_type.m_dimensions[i];
            }
            offset += tot;
          }
        }
      }
      ofs << "]";
    }
  }
}
void PrintGlobalFloatArrayValues(
    std::ofstream& ofs, int n, int& offset,
    std::unique_ptr<InitValAST>& init_val, const std::vector<int>& sizes,
    const std::shared_ptr<FloatGlobalVariable>& gv) {
  ofs << gv->m_type.Dereference().Reduce(n) << " ";
  if (init_val == nullptr || init_val->AllZero()) {
    if (n == gv->m_type.m_dimensions.size()) {
      ofs << gv->m_flatten_vals[offset++];
    } else {
      ofs << "zeroinitializer";
      int tot = 1;
      for (int i = n; i < gv->m_type.m_dimensions.size(); ++i) {
        tot *= gv->m_type.m_dimensions[i];
      }
      offset += tot;
    }
  } else {
    if (n == gv->m_type.m_dimensions.size()) {
      ofs << std::hexfloat << gv->m_flatten_vals[offset++];
    } else {
      ofs << "[";
      assert(init_val->m_vals.size() <= gv->m_type.m_dimensions[n]);
      bool first = true;
      for (int i = 0; i < gv->m_type.m_dimensions[n]; ++i) {
        if (first)
          first = false;
        else
          ofs << ", ";
        if (i < init_val->m_vals.size()) {
          // defined in init_val
          PrintGlobalFloatArrayValues(ofs, n + 1, offset, init_val->m_vals[i],
                                      sizes, gv);
        } else {
          // default zero
          if (n + 1 == gv->m_type.m_dimensions.size()) {
            ofs << "float 0";
            ++offset;
          } else {
            ofs << gv->m_type.Dereference().Reduce(n + 1) << " ";
            ofs << "zeroinitializer";
            int tot = 1;
            for (int i = n + 1; i < gv->m_type.m_dimensions.size(); ++i) {
              tot *= gv->m_type.m_dimensions[i];
            }
            offset += tot;
          }
        }
      }
      ofs << "]";
    }
  }
}
// export ir

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
  g_label_allocator.Clear();
  g_reg_allocator.Clear();

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
    // first allocate names for basic blocks
    for (auto& bb : m_bb_list) {
      g_label_allocator.Insert(bb);
    }
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
void BasicBlock::ExportIR(std::ofstream& ofs, int depth) {
  ofs << g_label_allocator.GetLabelName(shared_from_this()).substr(1) << ": ";
  ofs << "; (" << m_name << ") ";
  ofs << "preds: [";
  bool first = true;
  for (auto pred : Predecessors()) {
    if (first)
      first = false;
    else
      ofs << ", ";
    ofs << g_label_allocator.GetLabelName(pred);
  }
  ofs << "]";
  ofs << std::endl;
  for (auto& instr : m_instr_list) {
    instr->ExportIR(ofs, depth);
    ofs << std::endl;
  }
}
void IntGlobalVariable::ExportIR(std::ofstream& ofs, int depth) {
  // depth is useless here
  if (m_is_const && m_flatten_vals.size() == 1) return;
  // g_allocator.SetGlobalVarName(shared_from_this(), "@" + m_allocated_name);
  ofs << "@" << m_name << " = dso_local "
      << (m_is_const ? "constant " : "global ");
  if (m_flatten_vals.size() == 1) {
    ofs << "i32 " << m_flatten_vals[0] << std::endl;
  } else {
    int offset = 0;
    int tot = 1;
    std::vector<int> sizes(m_type.m_dimensions.size());
    for (int i = sizes.size() - 1; i >= 0; --i) {
      tot *= m_type.m_dimensions[i];
      sizes[i] = tot;
    }
    PrintGlobalIntArrayValues(ofs, 0, offset, m_init_val, sizes,
                              shared_from_base<IntGlobalVariable>());
    ofs << std::endl;
  }
}
void FloatGlobalVariable::ExportIR(std::ofstream& ofs, int depth) {
  // depth is useless here
  if (m_is_const && m_flatten_vals.size() == 1) return;
  ofs << "@" << m_name << " = dso_local "
      << (m_is_const ? "constant " : "global ");
  if (m_flatten_vals.size() == 1) {
    ofs << "float " << m_flatten_vals[0] << std::endl;
  } else {
    int offset = 0;
    int tot = 1;
    std::vector<int> sizes(m_type.m_dimensions.size());
    for (int i = sizes.size() - 1; i >= 0; --i) {
      tot *= m_type.m_dimensions[i];
      sizes[i] = tot;
    }
    PrintGlobalFloatArrayValues(ofs, 0, offset, m_init_val, sizes,
                                shared_from_base<FloatGlobalVariable>());
    ofs << std::endl;
  }
}
void BinaryInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // assert(m_lhs_val_use.m_value->m_type == ValueType::CONST_INT
  //        || m_lhs_val_use.m_value->m_type == ValueType::CONST_FLOAT);
  // assert(m_lhs_val_use.m_value->m_type == m_rhs_val_use.m_value->m_type);

  ofs << std::string(depth * 2, ' ');
  ofs << g_reg_allocator.GetValueName(shared_from_this()) << " = ";
  ofs << BinaryOpToString(m_op) << " " << m_lhs_val_use->m_value->m_type << " ";
  ofs << g_reg_allocator.GetValueName(m_lhs_val_use->m_value) << ", "
      << g_reg_allocator.GetValueName(m_rhs_val_use->m_value) << std::endl;
}
void FNegInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << g_reg_allocator.GetValueName(shared_from_this()) << " = ";
  ofs << "fneg"
      << " " << m_lhs_val_use->m_value->m_type << " "
      << g_reg_allocator.GetValueName(m_lhs_val_use->m_value) << std::endl;
}
void CallInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  if (m_type.m_base_type != BasicType::VOID) {
    ofs << g_reg_allocator.GetValueName(shared_from_this()) << " = ";
  }
  ofs << "call " << m_type << " @" << m_func_name << "(";
  // params
  bool first = true;
  for (auto& param : m_params) {
    if (first)
      first = false;
    else
      ofs << ", ";

    ofs << param->m_value->m_type << " "
        << g_reg_allocator.GetValueName(param->m_value);
  }
  ofs << ")" << std::endl;
}
void BranchInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "br ";
  ofs << m_cond->m_value->m_type << " "
      << g_reg_allocator.GetValueName(m_cond->m_value) << ", ";
  ofs << m_true_block->m_type << " "
      << g_label_allocator.GetLabelName(m_true_block) << ", ";
  ofs << m_false_block->m_type << " "
      << g_label_allocator.GetLabelName(m_false_block) << std::endl;
}
void JumpInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "br ";
  ofs << m_target_block->m_type << " "
      << g_label_allocator.GetLabelName(m_target_block) << std::endl;
}
void ReturnInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  if (m_ret == nullptr || m_ret->m_value == nullptr) {
    ofs << "ret void" << std::endl;
  } else if (IsConstant()) {
    ofs << "ret " << m_ret->m_value->m_type << " ";
    m_ret->m_value->ExportIR(ofs, depth);
    ofs << std::endl;
  } else {
    // ret_val->ExportIR(ofs, depth);
    ofs << "ret " << m_ret->m_value->m_type << " "
        << g_reg_allocator.GetValueName(m_ret->m_value) << std::endl;
  }
}
void GetElementPtrInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << g_reg_allocator.GetValueName(shared_from_this()) << " = ";
  ofs << "getelementptr " << m_addr->m_value->m_type.Dereference() << ", ";
  ofs << m_addr->m_value->m_type << " "
      << g_reg_allocator.GetValueName(m_addr->m_value) << ", ";
  bool first = true;
  for (const auto& index : m_indices) {
    if (first)
      first = false;
    else
      ofs << ", ";
    ofs << index->m_value->m_type << " "
        << g_reg_allocator.GetValueName(index->m_value);
  }
  ofs << std::endl;
}
void LoadInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << g_reg_allocator.GetValueName(shared_from_this()) << " = ";
  ofs << "load " << m_type << ", " << m_addr->m_value->m_type << " "
      << g_reg_allocator.GetValueName(m_addr->m_value);
  ofs << std::endl;
}
void StoreInstruction::ExportIR(std::ofstream& ofs, int depth) {
  assert(m_val->m_value != nullptr);
  assert(m_addr->m_value != nullptr);

  ofs << std::string(depth * 2, ' ');
  ofs << "store " << m_val->m_value->m_type << " "
      << g_reg_allocator.GetValueName(m_val->m_value);
  // m_val.m_value->ExportIR(ofs, depth);
  ofs << ", " << m_addr->m_value->GetType() << " "
      << g_reg_allocator.GetValueName(m_addr->m_value);
  ofs << std::endl;
}
void AllocaInstruction::ExportIR(std::ofstream& ofs, int depth) {
  assert(m_type.m_num_star > 0);
  ofs << std::string(depth * 2, ' ');

  ofs << g_reg_allocator.GetValueName(shared_from_this()) << " = alloca "
      << m_type.Dereference() << std::endl;

  // init vals are not initialized here
}
void PhiInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << g_reg_allocator.GetValueName(shared_from_this()) << " = phi ";
  ofs << m_type << " ";  // i32 or float
  bool first = true;
  for (auto& [bb, incoming_val] : m_contents) {
    if (first)
      first = false;
    else
      ofs << ", ";
    ofs << "[ ";
    if (incoming_val == nullptr) {
      ofs << "undef";
    } else {
      ofs << g_reg_allocator.GetValueName(incoming_val->m_value);
    }
    ofs << ", " << g_label_allocator.GetLabelName(bb) << " ]";
  }
  ofs << std::endl;
}
// NOT a complete instruction, cannot be invoked independently
void Constant::ExportIR(std::ofstream& ofs, int depth) {
  switch (m_type.m_base_type) {
    case BasicType::FLOAT:
      union MyUnion {
        double m_double_val;
        uint64_t m_int_val;
      } test;
      test.m_double_val = m_float_val;
      ofs << "0x" << std::hex << std::setfill('0') << std::setw(16)
          << test.m_int_val;
      break;
    case BasicType::INT:
    case BasicType::CHAR:
      ofs << m_int_val;
      break;
    case BasicType::BOOL:
      ofs << (m_int_val ? "true" : "false");
      break;
    default:
      assert(false);
  }
}
void BitCastInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << g_reg_allocator.GetValueName(shared_from_this()) << " = bitcast ";
  ofs << m_val->m_value->m_type << " "
      << g_reg_allocator.GetValueName(m_val->m_value) << " to " << m_target_type
      << "*" << std::endl;
}
void ZExtInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << g_reg_allocator.GetValueName(shared_from_this()) << " = zext ";
  ofs << m_val->m_value->m_type << " "
      << g_reg_allocator.GetValueName(m_val->m_value) << " to i32" << std::endl;
}
void SIToFPInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << g_reg_allocator.GetValueName(shared_from_this()) << " = sitofp ";
  ofs << m_val->m_value->m_type << " "
      << g_reg_allocator.GetValueName(m_val->m_value) << " to float"
      << std::endl;
}
void FPToSIInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << g_reg_allocator.GetValueName(shared_from_this()) << " = fptosi ";
  ofs << m_val->m_value->m_type << " "
      << g_reg_allocator.GetValueName(m_val->m_value) << " to i32" << std::endl;
}
// not complete
void FunctionArg::ExportIR(std::ofstream& ofs, int depth) {
  // depth is useless here
  ofs << m_type;
  if (!m_is_decl) {
    ofs << " " << g_reg_allocator.GetValueName(shared_from_this());
  }
}
