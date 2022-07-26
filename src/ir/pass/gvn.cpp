//
// Created by garen on 7/21/22.
//

#include <memory>
#include <vector>

#include "ir/ir-pass-manager.h"

// check if a BINARY operator is commutative
bool IsCommutative(IROp op) {
  switch (op) {
    case IROp::ADD:
    case IROp::MUL:
      return true;
    default:
      return false;
  }
}

bool IsComparison(IROp op) {
  switch (op) {
    case IROp::I_SGE:
    case IROp::I_SGT:
    case IROp::I_SLE:
    case IROp::I_SLT:
    case IROp::I_EQ:
    case IROp::I_NE:
      return true;
    default:
      return false;
  }
}

std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> g_vns;
std::unordered_map<std::shared_ptr<Value>, size_t> g_idx;

std::shared_ptr<Value> GetVN(std::shared_ptr<Value> val,
                             std::shared_ptr<IRBuilder> builder);

std::shared_ptr<Value> FindForGEPInstr(
    std::shared_ptr<GetElementPtrInstruction> instr,
    std::shared_ptr<IRBuilder> builder) {
  for (auto [old_instr, new_val] : g_vns) {
    if (auto old_gep_instr
        = std::dynamic_pointer_cast<GetElementPtrInstruction>(old_instr)) {
      if (old_gep_instr != instr
          && old_gep_instr->m_indices.size() == instr->m_indices.size()
          && old_gep_instr->m_addr == instr->m_addr) {
        bool flag = true;
        auto sz = instr->m_indices.size();
        for (int i = 0; i < sz; ++i) {
          if (GetVN(old_gep_instr->m_indices[i]->m_value, builder)
              != GetVN(instr->m_indices[i]->m_value, builder)) {
            flag = false;
            break;
          }
        }
        if (flag) {
          return GetVN(new_val, builder);
        }
      }
    }
  }
  return instr;
}

std::shared_ptr<Value> FindForCallInstr(std::shared_ptr<CallInstruction> instr,
                                        std::shared_ptr<IRBuilder> builder) {
  if (instr->HasSideEffect()) return instr;  // cannot be replaced
  // TODO(garen): load from address?

  for (auto [old_instr, new_val] : g_vns) {
    if (auto old_call_instr
        = std::dynamic_pointer_cast<CallInstruction>(old_instr)) {
      if (old_call_instr != instr
          && old_call_instr->m_function == instr->m_function
          && old_call_instr->m_func_name == instr->m_func_name
          && old_call_instr->m_params.size() == instr->m_params.size()) {
        bool flag = true;
        auto sz = instr->m_params.size();
        for (int i = 0; i < sz; ++i) {
          if (GetVN(instr->m_params[i]->m_value, builder)
              != GetVN(old_call_instr->m_params[i]->m_value, builder)) {
            flag = false;
            break;
          }
        }
        if (flag) {
          return new_val;
        }
      }
    }
  }
  return instr;  // fallback, nothing found
}

std::shared_ptr<Value> FindForBinaryInstr(
    std::shared_ptr<BinaryInstruction> instr,
    std::shared_ptr<IRBuilder> builder) {
  // if both operands are const, the result is const too
  if (instr->m_lhs_val_use->m_value->m_type.IsConst()
      && instr->m_rhs_val_use->m_value->m_type.IsConst()) {
    return builder->GetConstant(instr->m_op, instr->m_lhs_val_use->m_value,
                                instr->m_rhs_val_use->m_value);
  }

  if (instr->m_op == IROp::ADD) {
    if (instr->m_lhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_lhs_val_use->m_value);
      if (c != nullptr && c->Evaluate().Equals(0)) {
        // 0 + a = a
        return instr->m_rhs_val_use->m_value;
      }
    }
    if (instr->m_rhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_rhs_val_use->m_value);
      if (c != nullptr && c->Evaluate().Equals(0)) {
        // a + 0 = a
        return instr->m_lhs_val_use->m_value;
      }
    }
  }

  if (instr->m_op == IROp::SUB) {
    if (instr->m_rhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_rhs_val_use->m_value);
      if (c != nullptr && c->Evaluate().Equals(0)) {
        // a - 0 = a
        return instr->m_lhs_val_use->m_value;
      }
    }
  }

  if (instr->m_op == IROp::MUL) {
    if (instr->m_rhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_rhs_val_use->m_value);
      if (c != nullptr) {
        if (c->Evaluate().Equals(0)) {
          // a * 0 = 0
          return builder->GetIntConstant(0);
        } else if (c->Evaluate().Equals(1)) {
          // a * 1 = a
          return instr->m_lhs_val_use->m_value;
        } else if (c->Evaluate().Equals(-1)) {
          // a * -1 => 0 - a
          instr->m_op = IROp::SUB;
          instr->m_rhs_val_use->UseValue(instr->m_lhs_val_use->m_value);
          instr->m_lhs_val_use->UseValue(builder->GetIntConstant(0));
          return instr;
        }
      }
    }
    if (instr->m_lhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_lhs_val_use->m_value);
      if (c != nullptr) {
        if (c->Evaluate().Equals(0)) {
          // 0 * a = 0
          return builder->GetIntConstant(0);
        } else if (c->Evaluate().Equals(1)) {
          // 1 * a = a
          return instr->m_rhs_val_use->m_value;
        } else if (c->Evaluate().Equals(-1)) {
          // -1 * a = 0 - a
          instr->m_op = IROp::SUB;
          instr->m_lhs_val_use->UseValue(builder->GetIntConstant(0));
          return instr;
        }
      }
    }
  }

  if (instr->m_op == IROp::SDIV) {
    if (instr->m_lhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_lhs_val_use->m_value);
      if (c != nullptr && c->Evaluate().Equals(0)) {
        // 0 / a = 0
        return builder->GetIntConstant(0);
      }
    }
    if (instr->m_rhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_rhs_val_use->m_value);
      if (c != nullptr) {
        if (c->Evaluate().Equals(0)) {
          // a / 0 raise exception
          assert(false);  // ??????????
        } else if (c->Evaluate().Equals(1)) {
          // a / 1 = a
          return instr->m_lhs_val_use->m_value;
        } else if (c->Evaluate().Equals(-1)) {
          // a / -1 = -a = 0 - a
          instr->m_op = IROp::SUB;
          instr->m_rhs_val_use->UseValue(instr->m_lhs_val_use->m_value);
          instr->m_lhs_val_use->UseValue(builder->GetIntConstant(0));
          return instr;
        }
      }
    }
  }

  if (instr->m_op == IROp::SREM) {
    if (instr->m_lhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_lhs_val_use->m_value);
      if (c != nullptr && c->Evaluate().Equals(0)) {
        // 0 % a = 0
        return builder->GetIntConstant(0);
      }
    }
    if (instr->m_rhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_rhs_val_use->m_value);
      if (c != nullptr) {
        if (c->Evaluate().Equals(0)) {
          // a % 0 raise exception
          assert(false);  // ??????????
        } else if (c->Evaluate().Equals(1) || c->Evaluate().Equals(-1)) {
          // a % 1 = 0
          // a % -1 = 0
          return builder->GetIntConstant(0);
        }
      }
    }
  }

  if (GetVN(instr->m_lhs_val_use->m_value, builder)
      == GetVN(instr->m_rhs_val_use->m_value, builder)) {
    switch (instr->m_op) {
      case IROp::I_SGE:
      case IROp::I_SLE:
      case IROp::I_EQ:
        return builder->GetIntConstant(1);
      case IROp::I_SGT:
      case IROp::I_SLT:
      case IROp::I_NE:
        return builder->GetIntConstant(0);
      default:
        assert(false);  // unreachable
    }
  }

  if (IsComparison(instr->m_op)) return instr;

  // find previous computed values from cloud
  for (auto [old_instr, new_val] : g_vns) {
    if (auto old_binary_instr
        = std::dynamic_pointer_cast<BinaryInstruction>(old_instr)) {
      if (old_binary_instr != instr && old_binary_instr->m_op == instr->m_op) {
        // check if they are the same
        if (GetVN(old_binary_instr->m_lhs_val_use->m_value, builder)
                == GetVN(instr->m_lhs_val_use->m_value, builder)
            && GetVN(old_binary_instr->m_rhs_val_use->m_value, builder)
                   == GetVN(instr->m_rhs_val_use->m_value, builder)) {
          // identical, return the result in vn table
          return new_val;
        } else if (IsCommutative(instr->m_op)
                   && GetVN(old_binary_instr->m_lhs_val_use->m_value, builder)
                          == GetVN(instr->m_rhs_val_use->m_value, builder)
                   && GetVN(old_binary_instr->m_rhs_val_use->m_value, builder)
                          == GetVN(instr->m_lhs_val_use->m_value, builder)) {
          // commutative law
          return new_val;
        }
      }
    }
  }
  return instr;
}

std::shared_ptr<Value> GetVN(std::shared_ptr<Value> val,
                             std::shared_ptr<IRBuilder> builder) {
  auto it = std::find_if(g_idx.begin(), g_idx.end(),
                         [=](const auto &x) { return x.first == val; });
  if (it != g_idx.end()) {
    auto [old_val, idx] = *it;
    if (g_vns[idx].first != g_vns[idx].second) {
      return g_vns[idx].second = GetVN(g_vns[idx].second, builder);
    } else {
      return g_vns[idx].first;
    }
  }
  g_vns.emplace_back(val, val);
  g_idx[val] = g_vns.size() - 1;  // last one

  if (auto binary_instr = std::dynamic_pointer_cast<BinaryInstruction>(val)) {
    g_vns.back().second = FindForBinaryInstr(binary_instr, builder);
  } else if (auto call_instr
             = std::dynamic_pointer_cast<CallInstruction>(val)) {
    g_vns.back().second = FindForCallInstr(call_instr, builder);
  } else if (auto gep_instr
             = std::dynamic_pointer_cast<GetElementPtrInstruction>(val)) {
    g_vns.back().second = FindForGEPInstr(gep_instr, builder);
  }
  return g_vns.back().second;
}

void RunGVN(std::shared_ptr<Function> function,
            std::shared_ptr<IRBuilder> builder) {
  for (auto &bb : function->m_bb_list) {
    // manually ++it
    for (auto it = bb->m_instr_list.begin(); it != bb->m_instr_list.end();) {
      auto instr = *it;
      auto instr_vn = GetVN(instr, builder);
      if (instr_vn != instr) {
        instr->ReplaceUseBy(instr_vn);
        auto del = it;
        ++it;
        bb->m_instr_list.erase(del);
      } else {
        ++it;
      }
    }
  }
}

void IRPassManager::GVNPass() {
  for (auto &func : m_builder->m_module->m_function_list) {
    if (func->m_bb_list.empty()) continue;
    RunGVN(func, m_builder);
  }
}