#include "ir/ir.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

#include "ast/ast.h"

// use, user, value

Use::Use(std::shared_ptr<Value> value)
    : m_value(std::move(value)), m_user(nullptr) {
  if (m_value != nullptr) m_value->AppendUse(*this);
}
void Use::SetUser(std::shared_ptr<Value> user) { m_user = std::move(user); }

void Value::AppendUse(Use use) { m_use_list.push_back(use); }

void Value::AppendOperand(const Use& use) { m_operand_list.push_back(use); }

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
  assert(instr->m_lhs_val_use.m_value->m_type == ValueType::INT
         || instr->m_lhs_val_use.m_value->m_type == ValueType::FLOAT);
  assert(instr->m_rhs_val_use.m_value->m_type == ValueType::INT
         || instr->m_rhs_val_use.m_value->m_type == ValueType::FLOAT);

  bb->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<Instruction> IRBuilder::CreateCallInstruction(
    std::shared_ptr<FuncCallAST> func_call) {
  std::vector<Use> param_uses;
  for (const auto& param : func_call->m_params) {
    auto val = param->CodeGen(shared_from_this());
  }

  auto instr = std::make_shared<CallInstruction>(
      func_call->ReturnType(), func_call->FuncName(), std::move(param_uses));

  return nullptr;
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
  auto instr = std::make_shared<BranchInstruction>(
      std::move(cond_val), std::move(true_block), std::move(false_block), bb);
  instr->m_cond.SetUser(instr);
  bb->PushBackInstruction(instr);
  return instr;
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

// module, function, basic block

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
    default:
      assert(false);  // unreachable
  }
}

std::string BinaryOpToString(IROp op) {
  switch (op) {
    case IROp::ADD:
      return "add i32";
    case IROp::SUB:
      return "sub i32";
    case IROp::MUL:
      return "mul i32";
    case IROp::SDIV:
      return "sdiv i32";
    case IROp::SREM:
      return "srem?";
    case IROp::SGEQ:
      return "icmp geq?";
    case IROp::SGE:
      return "icmp ge?";
    case IROp::SLEQ:
      return "icmp leq?";
    case IROp::SLE:
      return "icmp le?";
    case IROp::EQ:
      return "icmp eq?";
    case IROp::NE:
      return "icmp neq?";
    default:
      return "???";
  }
}

static std::unordered_map<std::shared_ptr<Value>, std::string> g_name_of_value;
static int g_name_of_value_cnt = 1;  // TODO(garen): special feature of LLVM
std::string GetValueName(const std::shared_ptr<Value>& val) {
  // check if the value is constant
  if (auto constant = std::dynamic_pointer_cast<Constant>(val)) {
    if (constant->m_is_float)
      return std::to_string(constant->m_float_val);
    else
      return std::to_string(constant->m_int_val);
  }

  auto it = g_name_of_value.find(val);
  if (it == g_name_of_value.end()) {
    g_name_of_value[val] = "%" + std::to_string(g_name_of_value_cnt++);
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
  // for (auto &func_decl: m_func_decl_list) {
  //
  // }
  for (auto& func_def : m_function_list) {
    func_def->ExportIR(ofs, depth);
  }
}
void Function::ExportIR(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ');
  ofs << "define dso_local " << VarTypeToString(ReturnType());
  ofs << " @" << FuncName() << "(";
  for (auto& arg : m_args) {
    arg->ExportIR(ofs, depth);
  }
  ofs << ") {" << std::endl;
  // TODO(garen): print function body
  for (auto& bb : m_bb_list) {
    bb->ExportIR(ofs, depth + 1);
  }
  ofs << "}" << std::endl;
}
void BasicBlock::ExportIR(std::ofstream& ofs, int depth) {
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
  // TODO(garen):
  assert(m_lhs_val_use.m_value->m_type == ValueType::INT
         || m_lhs_val_use.m_value->m_type == ValueType::FLOAT);
  assert(m_rhs_val_use.m_value->m_type == ValueType::INT
         || m_rhs_val_use.m_value->m_type == ValueType::FLOAT);

  ofs << std::string(depth * 2, ' ');
  ofs << GetValueName(shared_from_this()) << " = ";
  ofs << BinaryOpToString(m_op) << " ";
  ofs << GetValueName(m_lhs_val_use.m_value) << ", "
      << GetValueName(m_rhs_val_use.m_value) << std::endl;
}
void CallInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << "(TODO) CallInstruction" << std::endl;
}
void BranchInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << "(TODO) BranchInstruction" << std::endl;
}
void JumpInstruction::ExportIR(std::ofstream& ofs, int depth) {
  // TODO(garen):
  ofs << "(TODO) JumpInstruction" << std::endl;
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
  // TODO(garen): depth is useless here
}
