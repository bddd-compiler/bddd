#include "ir/ir.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

#include "ast/ast.h"

// use, user, value

void Use::SetUser(std::shared_ptr<Value> user) {
  if (m_value) {
    m_user = std::move(user);
    m_value->AppendUse(*this);
  }
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
  std::vector<int> init_vals;

  for (const auto& expr : decl->m_flatten_vals) {
    if (expr == nullptr) {
      init_vals.push_back(0);
    } else {
      init_vals.push_back(expr->IntVal());
    }
  }

  auto global_variable = std::make_shared<IntGlobalVariable>(std::move(decl));
  m_module->AppendGlobalVariable(global_variable);
  return global_variable;
}
std::shared_ptr<FloatGlobalVariable> IRBuilder::CreateFloatGlobalVariable(
    std::shared_ptr<DeclAST> decl) {
  // TODO(garen): unimplemented yet
  std::vector<float> init_vals;
  assert(false);
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

  auto lhs_base_type = instr->m_lhs_val_use.m_value->m_type.m_base_type,
       rhs_base_type = instr->m_rhs_val_use.m_value->m_type.m_base_type;
  if (lhs_base_type == BaseType::BOOL && rhs_base_type == BaseType::INT) {
    auto new_lhs = CreateZExtInstruction(instr->m_lhs_val_use.m_value,
                                         instr->m_rhs_val_use.m_value->m_type);
    instr->m_lhs_val_use.m_value->RemoveUse(instr->m_lhs_val_use);
    instr->m_lhs_val_use = Use(new_lhs);
    instr->m_lhs_val_use.SetUser(instr);

  } else if (lhs_base_type == BaseType::INT
             && rhs_base_type == BaseType::BOOL) {
    auto new_rhs = CreateZExtInstruction(instr->m_rhs_val_use.m_value,
                                         instr->m_lhs_val_use.m_value->m_type);
    instr->m_rhs_val_use.m_value->RemoveUse(instr->m_rhs_val_use);
    instr->m_rhs_val_use = Use(new_rhs);
    instr->m_rhs_val_use.SetUser(instr);
  } else {
    assert(lhs_base_type == BaseType::INT || lhs_base_type == BaseType::FLOAT);
    assert(lhs_base_type == rhs_base_type);
  }
  bb->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<CallInstruction> IRBuilder::CreateCallInstruction(
    VarType return_type, std::string func_name,
    std::vector<std::shared_ptr<Value>> params) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<CallInstruction>(return_type,
                                                 std::move(func_name), bb);
  std::vector<Use> param_uses;
  for (auto& param : params) {
    param_uses.emplace_back(param, instr);
  }
  instr->SetParams(std::move(param_uses));
  bb->PushBackInstruction(instr);
  return nullptr;
}
std::shared_ptr<CallInstruction> IRBuilder::CreateCallInstruction(
    VarType return_type, std::string func_name) {
  auto bb = m_module->GetCurrentBB();
  std::shared_ptr<Function> function = nullptr;
  for (auto& it : m_module->m_function_decl_list) {
    if (it->FuncName() == func_name) {
      function = it;
      break;
    }
  }
  if (function == nullptr) {
    for (auto& it : m_module->m_function_list) {
      if (it->FuncName() == func_name) {
        function = it;
        break;
      }
    }
  }
  assert(function != nullptr);
  auto instr = std::make_shared<CallInstruction>(return_type,
                                                 std::move(func_name), bb);
  instr->m_function = std::move(function);
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
  if (cond_val->m_type.m_base_type != BaseType::BOOL) {
    assert(cond_val->m_type.m_base_type == BaseType::INT);
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
    std::shared_ptr<Value> addr, std::vector<std::shared_ptr<Value>> indices) {
  auto bb = m_module->GetCurrentBB();
  auto instr = std::make_shared<GetElementPtrInstruction>(
      std::move(addr), std::move(indices), bb);
  bb->PushBackInstruction(instr);
  return instr;
}
std::shared_ptr<Instruction> IRBuilder::CreateLoadInstruction(
    std::shared_ptr<Value> addr) {
  auto instr = std::make_shared<LoadInstruction>(addr);
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
    auto instr = std::make_shared<AllocaInstruction>(value_type, init_val, bb);
    bb->PushBackInstruction(instr);
    return instr;
  } else {
    ValueType value_type(decl->GetVarType(), true);
    auto instr = std::make_shared<AllocaInstruction>(std::move(value_type),
                                                     init_val, bb);
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
void Module::Check() {
  for (auto& function : m_function_list) {
    for (auto& basic_block : function->m_bb_list) {
      for (auto it = basic_block->m_instr_list.begin();
           it != basic_block->m_instr_list.end(); ++it) {
        if ((*it)->IsTerminator()) {
          // all instructions after instr should be discarded
          basic_block->m_instr_list.erase(std::next(it),
                                          basic_block->m_instr_list.end());
        }
      }
    }
  }
}
void Function::AppendBasicBlock(std::shared_ptr<BasicBlock> bb) {
  m_bb_list.push_back(std::move(bb));
  m_current_bb = m_bb_list.back();
}
std::list<std::shared_ptr<BasicBlock>> Function::GetBlockList() {
  return m_bb_list;
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
std::list<std::shared_ptr<Instruction>> BasicBlock::GetInstList() {
  return m_instr_list;
}