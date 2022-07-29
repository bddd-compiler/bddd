//
// Created by garen on 7/21/22.
//

#include <memory>
#include <utility>
#include <vector>

#include "ir/ir-pass-manager.h"

// check if a BINARY operator is commutative
bool IsCommutative(IROp op) {
  switch (op) {
    case IROp::ADD:
    case IROp::MUL:
    case IROp::F_ADD:
    case IROp::F_MUL:
      return true;
    default:
      return false;
  }
}

std::vector<std::pair<std::shared_ptr<Value>, std::shared_ptr<Value>>> g_vns;
std::unordered_map<std::shared_ptr<Value>, size_t> g_idx;

std::shared_ptr<Value> GetValue(std::shared_ptr<Value> val,
                                std::shared_ptr<IRBuilder> builder);

size_t GetValueNumber(std::shared_ptr<Value> val,
                      std::shared_ptr<IRBuilder> builder) {
  auto temp = GetValue(std::move(val), std::move(builder));
  return g_idx[temp];
}

std::shared_ptr<Value> GetValueForGEPInstr(
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
          if (GetValueNumber(old_gep_instr->m_indices[i]->m_value, builder)
              != GetValueNumber(instr->m_indices[i]->m_value, builder)) {
            flag = false;
            break;
          }
        }
        if (flag) {
          return GetValue(new_val, builder);
        }
      }
    }
  }
  return instr;
}

std::shared_ptr<Value> GetValueForCallInstr(
    std::shared_ptr<CallInstruction> instr,
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
          if (GetValueNumber(instr->m_params[i]->m_value, builder)
              != GetValueNumber(old_call_instr->m_params[i]->m_value,
                                builder)) {
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

std::shared_ptr<Value> GetValueForBinaryInstr(
    std::shared_ptr<BinaryInstruction> instr,
    std::shared_ptr<IRBuilder> builder) {
  // if both operands are const, the result is const too
  if (instr->m_lhs_val_use->m_value->m_type.IsConst()
      && instr->m_rhs_val_use->m_value->m_type.IsConst()) {
    auto lhs
        = std::dynamic_pointer_cast<Constant>(instr->m_lhs_val_use->m_value);
    auto rhs
        = std::dynamic_pointer_cast<Constant>(instr->m_rhs_val_use->m_value);
    return builder->GetConstant(instr->m_op, lhs->Evaluate(), rhs->Evaluate());
  }

  if (instr->m_op == IROp::ADD || instr->m_op == IROp::F_ADD) {
    if (instr->m_lhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_lhs_val_use->m_value);
      assert(c != nullptr);
      auto res = c->Evaluate();
      // 0 + a = a
      if (instr->m_op == IROp::ADD && res.Equals(0)) {
        assert(instr->m_rhs_val_use->m_value->m_type.IsBasicInt());
        return instr->m_rhs_val_use->m_value;
      } else if (instr->m_op == IROp::F_ADD && res.Equals(0.0f)) {
        assert(instr->m_rhs_val_use->m_value->m_type.IsBasicFloat());
        return instr->m_rhs_val_use->m_value;
      }
    }
    if (instr->m_rhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_rhs_val_use->m_value);
      assert(c != nullptr);
      auto res = c->Evaluate();
      // a + 0 = a
      if (instr->m_op == IROp::ADD && res.Equals(0)) {
        assert(instr->m_lhs_val_use->m_value->m_type.IsBasicInt());
        return instr->m_lhs_val_use->m_value;
      } else if (instr->m_op == IROp::F_ADD && res.Equals(0.0f)) {
        assert(instr->m_lhs_val_use->m_value->m_type.IsBasicFloat());
        return instr->m_lhs_val_use->m_value;
      }
    }
  }

  if (instr->m_op == IROp::SUB || instr->m_op == IROp::F_SUB) {
    if (instr->m_rhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_rhs_val_use->m_value);
      assert(c != nullptr);
      auto res = c->Evaluate();
      // a - 0 = a
      if (instr->m_op == IROp::SUB && res.Equals(0)) {
        assert(instr->m_lhs_val_use->m_value->m_type.IsBasicInt());
        return instr->m_lhs_val_use->m_value;
      } else if (instr->m_op == IROp::F_SUB && res.Equals(0.0f)) {
        assert(instr->m_lhs_val_use->m_value->m_type.IsBasicFloat());
        return instr->m_lhs_val_use->m_value;
      }
    }
  }

  if (instr->m_op == IROp::MUL || instr->m_op == IROp::F_MUL) {
    if (instr->m_rhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_rhs_val_use->m_value);
      assert(c != nullptr);
      auto res = c->Evaluate();
      if (instr->m_op == IROp::MUL) {
        assert(instr->m_lhs_val_use->m_value->m_type.IsBasicInt());
        if (res.Equals(0)) {
          // a * 0 = 0
          return builder->GetIntConstant(0);
        } else if (res.Equals(1)) {
          // a * 1 = a
          return instr->m_lhs_val_use->m_value;
        } else if (res.Equals(-1)) {
          // a * -1 = 0 - a
          instr->m_op = IROp::SUB;
          instr->m_rhs_val_use->UseValue(instr->m_lhs_val_use->m_value);
          instr->m_lhs_val_use->UseValue(builder->GetIntConstant(0));
          return instr;
        }
      } else {
        assert(instr->m_lhs_val_use->m_value->m_type.IsBasicFloat());
        if (res.Equals(0.0f)) {
          return builder->GetFloatConstant(0.0);
        } else if (res.Equals(1.0f)) {
          return instr->m_lhs_val_use->m_value;
        } else if (res.Equals(-1.0f)) {
          instr->m_op = IROp::F_SUB;
          instr->m_rhs_val_use->UseValue(instr->m_lhs_val_use->m_value);
          instr->m_lhs_val_use->UseValue(builder->GetFloatConstant(0.0f));
          return instr;
        }
      }
    }

    if (instr->m_lhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_lhs_val_use->m_value);
      assert(c != nullptr);
      auto res = c->Evaluate();
      if (instr->m_op == IROp::MUL) {
        assert(instr->m_rhs_val_use->m_value->m_type.IsBasicInt());
        if (res.Equals(0)) {
          // 0 * a = 0
          return builder->GetIntConstant(0);
        } else if (res.Equals(1)) {
          // 1 * a = a
          return instr->m_rhs_val_use->m_value;
        } else if (res.Equals(-1)) {
          // -1 * a = 0 - a
          instr->m_op = IROp::SUB;
          instr->m_lhs_val_use->UseValue(builder->GetIntConstant(0));
          return instr;
        }
      } else {
        assert(instr->m_rhs_val_use->m_value->m_type.IsBasicFloat());
        if (res.Equals(0.0f)) {
          return builder->GetFloatConstant(0.0);
        } else if (res.Equals(1.0f)) {
          return instr->m_rhs_val_use->m_value;
        } else if (res.Equals(-1.0f)) {
          instr->m_op = IROp::F_SUB;
          instr->m_lhs_val_use->UseValue(builder->GetFloatConstant(0.0f));
          return instr;
        }
      }
    }
  }

  if (instr->m_op == IROp::SDIV || instr->m_op == IROp::F_DIV) {
    if (instr->m_lhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_lhs_val_use->m_value);
      assert(c != nullptr);
      auto res = c->Evaluate();
      // 0 / a = 0
      if (instr->m_op == IROp::SDIV && res.Equals(0)) {
        return builder->GetIntConstant(0);
      } else if (instr->m_op == IROp::F_DIV && res.Equals(0.0f)) {
        return builder->GetFloatConstant(0.0);
      }
    }
    if (instr->m_rhs_val_use->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(instr->m_rhs_val_use->m_value);
      assert(c != nullptr);
      auto res = c->Evaluate();
      if (instr->m_op == IROp::SDIV) {
        assert(instr->m_lhs_val_use->m_value->m_type.IsBasicInt());
        if (res.Equals(0)) {
          // a / 0 raise exception
          assert(false);  // ??????????
        } else if (res.Equals(1)) {
          // a / 1 = a
          return instr->m_lhs_val_use->m_value;
        } else if (res.Equals(-1)) {
          // a / -1 = -a = 0 - a
          instr->m_op = IROp::SUB;
          instr->m_rhs_val_use->UseValue(instr->m_lhs_val_use->m_value);
          instr->m_lhs_val_use->UseValue(builder->GetIntConstant(0));
          return instr;
        }
      } else {
        assert(instr->m_lhs_val_use->m_value->m_type.IsBasicFloat());
        if (res.Equals(0.0f)) {
          assert(false);
        } else if (res.Equals(1.0f)) {
          return instr->m_lhs_val_use->m_value;
        } else if (res.Equals(-1.0f)) {
          instr->m_op = IROp::F_SUB;
          instr->m_rhs_val_use->UseValue(instr->m_lhs_val_use->m_value);
          instr->m_lhs_val_use->UseValue(builder->GetFloatConstant(0.0f));
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

  if (GetValueNumber(instr->m_lhs_val_use->m_value, builder)
      == GetValueNumber(instr->m_rhs_val_use->m_value, builder)) {
    switch (instr->m_op) {
      case IROp::I_SGE:
      case IROp::I_SLE:
      case IROp::I_EQ:
      case IROp::F_GE:
      case IROp::F_LE:
      case IROp::F_EQ:
        return builder->GetBoolConstant(true);
      case IROp::I_SGT:
      case IROp::I_SLT:
      case IROp::I_NE:
      case IROp::F_GT:
      case IROp::F_LT:
      case IROp::F_NE:
        return builder->GetBoolConstant(false);
      default:
        break;
    }
  }

  if (instr->IsICmp() || instr->IsFCmp()) return instr;

  // find previous computed values from cloud
  int i = 0;
  for (auto [old_instr, new_val] : g_vns) {
    if (auto old_binary_instr
        = std::dynamic_pointer_cast<BinaryInstruction>(old_instr)) {
      if (old_binary_instr != instr && old_binary_instr->m_op == instr->m_op) {
        // check if they are the same
        auto old_lhs_vn
            = GetValueNumber(old_binary_instr->m_lhs_val_use->m_value, builder);
        auto new_lhs_vn
            = GetValueNumber(instr->m_lhs_val_use->m_value, builder);
        auto old_rhs_vn
            = GetValueNumber(old_binary_instr->m_rhs_val_use->m_value, builder);
        auto new_rhs_vn
            = GetValueNumber(instr->m_rhs_val_use->m_value, builder);
        if (old_lhs_vn == new_lhs_vn && old_rhs_vn == new_rhs_vn) {
          // identical, return the result in vn table
          return new_val;
        } else if (IsCommutative(instr->m_op) && old_lhs_vn == new_rhs_vn
                   && old_rhs_vn == new_lhs_vn) {
          // commutative law
          return new_val;
        }
      }
    }
    ++i;
  }
  return instr;
}

std::shared_ptr<Value> GetValue(std::shared_ptr<Value> val,
                                std::shared_ptr<IRBuilder> builder) {
  auto it = g_idx.find(val);
  if (it != g_idx.end()) {
    auto [old_val, idx] = *it;
    if (g_vns[idx].first != g_vns[idx].second) {
      return g_vns[idx].second = GetValue(g_vns[idx].second, builder);
    } else {
      return g_vns[idx].first;
    }
  }
  g_vns.emplace_back(val, val);
  size_t idx = g_vns.size() - 1;
  g_idx[val] = idx;

  if (auto binary_instr = std::dynamic_pointer_cast<BinaryInstruction>(val)) {
    g_vns[idx].second = GetValueForBinaryInstr(binary_instr, builder);
  } else if (auto call_instr
             = std::dynamic_pointer_cast<CallInstruction>(val)) {
    g_vns[idx].second = GetValueForCallInstr(call_instr, builder);
  } else if (auto gep_instr
             = std::dynamic_pointer_cast<GetElementPtrInstruction>(val)) {
    g_vns[idx].second = GetValueForGEPInstr(gep_instr, builder);
  } else if (auto zext_instr
             = std::dynamic_pointer_cast<ZExtInstruction>(val)) {
    // i1 to i32
    if (zext_instr->m_val->m_value->m_type.IsConst()) {
      auto c = std::dynamic_pointer_cast<Constant>(zext_instr->m_val->m_value);
      g_vns[idx].second = builder->GetIntConstant(c->Evaluate().IntVal());
    }
  } else if (auto sitofp_instr
             = std::dynamic_pointer_cast<SIToFPInstruction>(val)) {
    // i32 to float
    if (sitofp_instr->m_val->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(sitofp_instr->m_val->m_value);
      g_vns[idx].second = builder->GetFloatConstant(c->Evaluate().IntVal());
    }
  } else if (auto fptosi_instr
             = std::dynamic_pointer_cast<FPToSIInstruction>(val)) {
    // float to i32
    if (fptosi_instr->m_val->m_value->m_type.IsConst()) {
      auto c
          = std::dynamic_pointer_cast<Constant>(fptosi_instr->m_val->m_value);
      g_vns[idx].second = builder->GetIntConstant(c->Evaluate().FloatVal());
    }
  }
  return g_vns[idx].second;
}

void RunGVN(std::shared_ptr<Function> function,
            std::shared_ptr<IRBuilder> builder) {
  for (auto &bb : function->m_bb_list) {
    // manually ++it
    for (auto it = bb->m_instr_list.begin(); it != bb->m_instr_list.end();) {
      auto instr = *it;
      auto instr_val = GetValue(instr, builder);
      // std::cerr << "[debug] idx: " << g_idx[instr_val] << std::endl;

      if (instr_val != instr) {
        instr->ReplaceUseBy(instr_val);
        size_t idx = g_idx[instr_val];
        g_idx.erase(instr_val);
        std::swap(g_vns[idx], g_vns.back());
        g_vns.pop_back();
        auto del = it;
        ++it;
        bb->RemoveInstruction(del);
      } else {
        ++it;
      }
    }
  }
}

void IRPassManager::GVNPass() {
  SideEffectPass();
  for (auto &func : m_builder->m_module->m_function_list) {
    if (func->m_bb_list.empty()) continue;
    RunGVN(func, m_builder);
    // RemoveTrivialPhis(func);
    ReplaceTrivialBranchByJump(func);
    RemoveUnvisitedBasicBlocks(func);
  }
}