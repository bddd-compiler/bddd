#include "ir/ir.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

#include "ast/ast.h"

// use, user, value

void Use::SetUser(std::shared_ptr<Value> user) {
  m_user = std::move(user);
  m_value->AppendUse(*this);
}

// IRBuilder

std::shared_ptr<Value> IRBuilder::GetIntConstant(int int_val) {
  auto it = m_module->m_const_ints.find(int_val);
  if (it != m_module->m_const_ints.end()) {
    return it->second;
  } else {
    auto constant = std::make_shared<Constant>(int_val);
    m_module->m_const_ints[int_val] = constant;
    return constant;
  }
}
std::shared_ptr<Value> IRBuilder::GetFloatConstant(float float_val) {
  auto it = m_module->m_const_floats.find(float_val);
  if (it != m_module->m_const_floats.end()) {
    return it->second;
  } else {
    auto constant = std::make_shared<Constant>(float_val);
    m_module->m_const_floats[float_val] = constant;
    return constant;
  }
}
std::shared_ptr<IntGlobalVariable> IRBuilder::CreateIntGlobalVariable(
    std::shared_ptr<DeclAST> decl) {
  // TODO(garen): fill init_vals
  std::vector<int> init_vals;
  std::unordered_map<std::shared_ptr<ExprAST>, int> cache;

  for (const auto& expr : decl->m_flatten_vals) {
    if (expr == nullptr) {
      init_vals.push_back(0);
      continue;
    }
    auto it = cache.find(expr);
    if (it != cache.end()) {
      init_vals.push_back(it->second);
    } else {
      auto val = expr->CodeGen(shared_from_this());
    }
  }

  auto global_variable = std::make_shared<IntGlobalVariable>(std::move(decl));
  m_module->AppendGlobalVariable(global_variable);
  return global_variable;
}
std::shared_ptr<FloatGlobalVariable> IRBuilder::CreateFloatGlobalVariable(
    std::shared_ptr<DeclAST> decl) {
  // TODO(garen): fill init_vals
  std::vector<float> init_vals;

  auto global_variable = std::make_shared<FloatGlobalVariable>(std::move(decl));
  m_module->AppendGlobalVariable(global_variable);
  return global_variable;
}
std::shared_ptr<GlobalVariable> IRBuilder::CreateGlobalVariable(
    std::shared_ptr<DeclAST> decl) {
  if (decl->GetVarType() == VarType::INT)
    return CreateIntGlobalVariable(std::move(decl));
  else if (decl->GetVarType() == VarType::FLOAT)
    return CreateFloatGlobalVariable(std::move(decl));
  else
    assert(false);  // unreachable
}
std::shared_ptr<Instruction> IRBuilder::CreateReturnInstruction(
    std::shared_ptr<Value> ret) {
  auto instr = std::make_shared<ReturnInstruction>(std::move(ret));
  instr->m_ret.SetUser(instr);
  m_module->GetCurrentBB()->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<BasicBlock> IRBuilder::CreateBasicBlock(
    std::string block_name) {
  auto bb = std::make_shared<BasicBlock>(std::move(block_name));
  AppendBasicBlock(bb);
  return bb;
}
std::shared_ptr<Instruction> IRBuilder::CreateBinaryInstruction(
    IROp op, const std::shared_ptr<Value>& lhs,
    const std::shared_ptr<Value>& rhs) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<BinaryInstruction>(op, lhs, rhs, bb);
  instr->m_lhs_val_use.SetUser(instr);
  instr->m_rhs_val_use.SetUser(instr);

  if (instr->m_lhs_val_use.m_value->m_type == ValueType::BOOL
      && instr->m_rhs_val_use.m_value->m_type == ValueType::INT) {
    auto new_lhs = CreateZExtInstruction(instr->m_lhs_val_use.m_value,
                                         instr->m_rhs_val_use.m_value->m_type);
    instr->m_lhs_val_use.m_value->RemoveUse(instr->m_lhs_val_use);
    instr->m_lhs_val_use = Use(new_lhs);
    instr->m_lhs_val_use.SetUser(instr);

  } else if (instr->m_lhs_val_use.m_value->m_type == ValueType::INT
             && instr->m_rhs_val_use.m_value->m_type == ValueType::BOOL) {
    auto new_rhs = CreateZExtInstruction(instr->m_rhs_val_use.m_value,
                                         instr->m_lhs_val_use.m_value->m_type);
    instr->m_rhs_val_use.m_value->RemoveUse(instr->m_rhs_val_use);
    instr->m_rhs_val_use = Use(new_rhs);
    instr->m_rhs_val_use.SetUser(instr);
  } else {
    assert(instr->m_lhs_val_use.m_value->m_type == ValueType::INT
           || instr->m_lhs_val_use.m_value->m_type == ValueType::FLOAT);
    assert(instr->m_lhs_val_use.m_value->m_type
           == instr->m_rhs_val_use.m_value->m_type);
  }
  bb->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<CallInstruction> IRBuilder::CreateCallInstruction(
    std::shared_ptr<FuncDefAST> func_def) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<CallInstruction>(func_def->ReturnType(),
                                                 func_def->FuncName(), bb);
  bb->PushBackInstruction(instr);
  return instr;
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
  if (cond_val->m_type != ValueType::BOOL) {
    assert(cond_val->m_type == ValueType::INT);
    auto new_cond_val
        = CreateBinaryInstruction(IROp::NE, cond_val, GetIntConstant(0));
    auto instr = std::make_shared<BranchInstruction>(
        std::move(new_cond_val), std::move(true_block), std::move(false_block),
        bb);
    instr->m_cond.SetUser(instr);
    bb->PushBackInstruction(instr);
    return instr;
  } else {
    auto instr = std::make_shared<BranchInstruction>(
        std::move(cond_val), std::move(true_block), std::move(false_block), bb);
    instr->m_cond.SetUser(instr);
    bb->PushBackInstruction(instr);
    return instr;
  }
}
std::shared_ptr<Instruction> IRBuilder::CreateStoreInstruction(
    std::shared_ptr<Value> addr, std::shared_ptr<Value> val) {
  auto bb = m_module->GetCurrentBB();
  auto instr
      = std::make_shared<StoreInstruction>(std::move(addr), std::move(val), bb);
  instr->m_addr.SetUser(instr);
  instr->m_val.SetUser(instr);
  bb->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<Instruction> IRBuilder::CreateGetElementPtrInstruction(
    std::shared_ptr<Value> addr, std::vector<std::shared_ptr<Value>> dimensions,
    const std::vector<int>& products) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<GetElementPtrInstruction>(
      std::move(addr), std::move(dimensions), products, bb);
  bb->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<Instruction> IRBuilder::CreateLoadInstruction(
    std::shared_ptr<Value> addr) {
  auto instr = std::make_shared<LoadInstruction>(std::move(addr));
  instr->m_addr.SetUser(instr);
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
    std::shared_ptr<DeclAST> decl, std::shared_ptr<Value> init_val) {
  auto bb = m_module->GetCurrentBB();

  if (decl->IsArray()) {
    auto size = decl->m_products[0];
    auto instr = std::make_shared<AllocaInstruction>(size, init_val, bb);
    bb->PushBackInstruction(instr);

    return instr;
  } else {
    auto instr
        = std::make_shared<AllocaInstruction>(decl->GetVarType(), init_val, bb);
    bb->PushBackInstruction(instr);
    return instr;
  }
}

std::shared_ptr<Value> IRBuilder::CreateZExtInstruction(
    std::shared_ptr<Value> from, ValueType type_to) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<ZExtInstruction>(std::move(from), type_to, bb);
  bb->PushBackInstruction(instr);
  return instr;
}

// module, function, basic block

void Module::AppendFunctionDecl(std::shared_ptr<Function> function_decl) {
  m_function_decl_list.push_back(function_decl);
}
void Module::AppendFunction(std::shared_ptr<Function> function) {
  m_function_list.push_back(function);
  m_current_func = m_function_list.back();
}
void Module::AppendGlobalVariable(
    std::shared_ptr<GlobalVariable> global_variable) {
  m_global_variable_list.push_back(global_variable);
}
void Module::AppendBasicBlock(std::shared_ptr<BasicBlock> bb) {
  m_current_func->AppendBasicBlock(std::move(bb));
}
void Function::AppendBasicBlock(std::shared_ptr<BasicBlock> bb) {
  m_bb_list.push_back(std::move(bb));
  m_current_bb = m_bb_list.back();
}
void IRBuilder::AppendBasicBlock(std::shared_ptr<BasicBlock> bb) {
  m_module->AppendBasicBlock(std::move(bb));
}
void BasicBlock::PushBackInstruction(std::shared_ptr<Instruction> instr) {
  instr->m_bb = shared_from_base<BasicBlock>();
  m_instr_list.push_back(std::move(instr));
}
void BasicBlock::PushFrontInstruction(std::shared_ptr<Instruction> instr) {
  instr->m_bb = shared_from_base<BasicBlock>();
  m_instr_list.push_front(std::move(instr));
}
void BasicBlock::InsertFrontInstruction(
    const std::shared_ptr<Instruction>& elem,
    std::shared_ptr<Instruction> instr) {
  const auto it = std::find(m_instr_list.begin(), m_instr_list.end(), elem);
  assert(it != m_instr_list.end());
  instr->m_bb = shared_from_base<BasicBlock>();
  m_instr_list.insert(it, std::move(instr));
}
void BasicBlock::InsertBackInstruction(const std::shared_ptr<Instruction>& elem,
                                       std::shared_ptr<Instruction> instr) {
  auto it = std::find(m_instr_list.begin(), m_instr_list.end(), elem);
  assert(it != m_instr_list.end());
  ++it;
  assert(it != m_instr_list.end());
  instr->m_bb = shared_from_base<BasicBlock>();
  m_instr_list.insert(it, std::move(instr));
}
// export LLVM IR

std::string VarTypeToString(VarType var_type) {
  switch (var_type) {
    case VarType::INT:
      return "i32";
    case VarType::FLOAT:
      return "f32";
    case VarType::VOID:
      return "void";
    default:
      assert(false);  // unreachable
  }
}

std::string ValueTypeToString(ValueType value_type) {
  switch (value_type) {
    case ValueType::INT:
      return "i32";
    case ValueType::FLOAT:
      return "f32";
    case ValueType::INT_PTR:
      return "i32*";
    case ValueType::FLOAT_PTR:
      return "f32*";
    case ValueType::VOID:
      return "void";
    case ValueType::BOOL:
      return "i1";
    case ValueType::LABEL:
      return "label";
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
      return "???";
  }
}

static std::unordered_map<std::shared_ptr<Value>, std::string> g_name_of_value;
static int g_virtual_reg_cnt = 0;
static int g_label_cnt = 0;

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

  auto it = g_name_of_value.find(val);
  if (it == g_name_of_value.end()) {
    if (val->m_type == ValueType::LABEL)
      g_name_of_value[val] = "L" + std::to_string(g_label_cnt++);
    else
      g_name_of_value[val] = std::to_string(g_virtual_reg_cnt++);
  }
  return "%" + g_name_of_value[val];
}

// return in string
// example: 1 or L1
std::string GetValueNumber(const std::shared_ptr<Value>& val) {
  assert(val != nullptr);

  // check if the value is constant
  if (auto constant = std::dynamic_pointer_cast<Constant>(val)) {
    assert(false);  // constant does not have number in virtual registers
  }

  auto it = g_name_of_value.find(val);
  if (it == g_name_of_value.end()) {
    if (val->m_type == ValueType::LABEL)
      g_name_of_value[val] = "L" + std::to_string(g_label_cnt++);
    else
      g_name_of_value[val] = std::to_string(g_virtual_reg_cnt++);
  }
  return g_name_of_value[val];
}

// static std::unordered_map<std::shared_ptr<Value>, std::shared_ptr<Value>>
//     g_addr_in_stack;
// std::shared_ptr<Value> GetStackAddr(std::shared_ptr<Value> addr) {
//   auto it = g_addr_in_stack.find(addr);
//   assert(it != g_addr_in_stack.end());
//   return it->second;
// }

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
  for (auto& arg : m_args) {
    arg->ExportIR(ofs, depth);
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
}
void BasicBlock::ExportIR(std::ofstream& ofs, int depth) {
  ofs << GetValueNumber(shared_from_this()) << ":" << std::endl;
  for (auto& instr : m_instr_list) {
    instr->ExportIR(ofs, depth);
    ofs << std::endl;
  }
}
void IntGlobalVariable::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << "(TODO) IntGlobalVariable" << std::endl;
}
void FloatGlobalVariable::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << "(TODO) FloatGlobalVariable" << std::endl;
}
void BinaryInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // assert(m_lhs_val_use.m_value->m_type == ValueType::INT
  //        || m_lhs_val_use.m_value->m_type == ValueType::FLOAT);
  // assert(m_lhs_val_use.m_value->m_type == m_rhs_val_use.m_value->m_type);

  ofs << std::string(depth * 2, ' ');
  ofs << GetValueName(shared_from_this()) << " = ";
  ofs << BinaryOpToString(m_op) << " "
      << ValueTypeToString(m_lhs_val_use.m_value->m_type) << " ";
  ofs << GetValueName(m_lhs_val_use.m_value) << ", "
      << GetValueName(m_rhs_val_use.m_value) << std::endl;
}
void CallInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  if (m_type != ValueType::VOID) {
    ofs << GetValueName(shared_from_this()) << " = ";
  }
  ofs << "call " << ValueTypeToString(m_type) << " @" << m_func_name << "(";
  // params
  bool first = true;
  for (auto& param : m_params) {
    if (first)
      first = false;
    else
      ofs << ", ";
    ofs << ValueTypeToString(param.m_value->m_type) << " "
        << GetValueName(param.m_value);
  }
  ofs << ")" << std::endl;
}
void BranchInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "br ";
  ofs << ValueTypeToString(m_cond.m_value->m_type) << " "
      << GetValueName(m_cond.m_value) << ", ";
  ofs << ValueTypeToString(m_true_block->m_type) << " "
      << GetValueName(m_true_block) << ", ";
  ofs << ValueTypeToString(m_false_block->m_type) << " "
      << GetValueName(m_false_block) << std::endl;
}
void JumpInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "br ";
  ofs << ValueTypeToString(m_target_block->m_type) << " "
      << GetValueName(m_target_block) << std::endl;
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
    ofs << "ret " << ret_val->GetTypeString() << GetValueName(ret_val)
        << std::endl;
  }
}
void GetElementPtrInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << "(TODO) GetElementPtrInstruction" << std::endl;
}
void LoadInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << std::string(depth * 2, ' ');
  ofs << GetValueName(shared_from_this()) << " = ";
  // auto stack_addr = GetStackAddr(m_addr.m_value);
  auto stack_addr = m_addr.m_value;
  ofs << "load " << ValueTypeToString(m_type) << ", "
      << ValueTypeToString(stack_addr->m_type) << " "
      << GetValueName(stack_addr);
  ofs << std::endl;
}
void StoreInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  assert(m_val.m_value != nullptr);
  assert(m_addr.m_value != nullptr);

  ofs << std::string(depth * 2, ' ');
  ofs << "store " << ValueTypeToString(m_val.m_value->m_type) << " "
      << GetValueName(m_val.m_value);
  // m_val.m_value->ExportIR(ofs, depth);
  ofs << ", " << ValueTypeToString(m_addr.m_value->GetType()) << " "
      << GetValueName(m_addr.m_value);
  ofs << std::endl;
}
void AllocaInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  if (m_size == 0) {
    ofs << GetValueName(shared_from_this()) << " = alloca ";
    if (m_type == ValueType::INT_PTR)
      ofs << "i32";
    else if (m_type == ValueType::FLOAT_PTR)
      ofs << "f32";
    ofs << std::endl;

    // if (m_init_val) {
    //   g_addr_in_stack[m_init_val] = shared_from_this();
    // }
  } else
    assert(false);  // TODO(garen):

  // assign init val
  // if (m_init_val) {
  //   ofs << std::string(depth * 2, ' ');
  //   ofs << GetValueName(m_init_val) << " = ";
  //   m_init_val->ExportIR(ofs, depth);
  //   ofs << std::endl;
  // }
}
void PhiInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << "(TODO): PhiInstruction" << std::endl;
}
// NOT a complete instruction, cannot be invoked independently
void Constant::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  if (m_is_float) {
    ofs << "f32 " << m_float_val;
  } else {
    ofs << "i32 " << m_int_val;
  }
}
void GlobalVariable::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << "(TODO) GlobalVariable" << std::endl;
}
void FunctionArg::ExportIR(std::ofstream& ofs, int depth) {
  ofs << ValueTypeToString(m_type);
}
void ZExtInstruction::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << GetValueName(shared_from_this()) << " = zext ";
  ofs << ValueTypeToString(m_val.m_value->m_type) << " "
      << GetValueName(m_val.m_value) << " to "
      << ValueTypeToString(m_target_type) << std::endl;
}
