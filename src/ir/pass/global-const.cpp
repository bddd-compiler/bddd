//
// Created by garen on 8/5/22.
//

#include "ir/ir-pass-manager.h"

bool CanBeConstSingle(std::shared_ptr<Value> var) {
  for (auto &use : var->m_use_list) {
    auto user = use->getUser();
    if (auto store = std::dynamic_pointer_cast<StoreInstruction>(user)) {
      return false;
    }
  }
  return true;
}

bool CanBeConstArray(std::shared_ptr<Value> var) {
  for (auto &use : var->m_use_list) {
    auto user = use->getUser();
    if (auto gep = std::dynamic_pointer_cast<GetElementPtrInstruction>(user)) {
      bool temp = CanBeConstArray(gep);
      if (!temp) return false;
    } else if (auto call = std::dynamic_pointer_cast<CallInstruction>(user)) {
      if (call->HasSideEffect() && call->m_func_name != "putint"
          && call->m_func_name != "putch" && call->m_func_name != "putfloat"
          && call->m_func_name != "putarray" && call->m_func_name != "putfarray"
          && call->m_func_name != "putf") {
        return false;
      }
    } else if (auto store = std::dynamic_pointer_cast<StoreInstruction>(user)) {
      return false;
    } else if (auto load = std::dynamic_pointer_cast<LoadInstruction>(user)) {
      continue;
    } else {
      assert(false);
    }
  }
  return true;
}

void IRPassManager::EliminateGlobalConstArrayAccess() {
  SideEffectPass();
  std::vector<std::shared_ptr<GlobalVariable>> const_gvs;
  std::vector<std::shared_ptr<GlobalVariable>> const_singles;
  for (auto &var : m_builder->m_module->m_global_variable_list) {
    if (var->m_is_array) {
      // check if it can be a const array
      if (!var->m_is_const) {
        bool flag = CanBeConstArray(var);
        if (flag) {
          var->m_is_const = true;
        }
      }
      if (var->m_is_const) {
        const_gvs.push_back(var);
      }
    } else {
      if (!var->m_is_const) {
        bool flag = CanBeConstSingle(var);
        if (flag) {
          var->m_is_const = true;
        }
      }
      if (var->m_is_const) {
        const_singles.push_back(var);
      }
    }
  }

  for (auto &var : const_singles) {
    std::cerr << "[debug] marking global const single" << std::endl;
    for (auto it = var->m_use_list.begin(); it != var->m_use_list.end(); ++it) {
      auto user = it->get()->getUser();
      auto val = var->GetFlattenVal(0);
      if (auto load = std::dynamic_pointer_cast<LoadInstruction>(user)) {
        assert(val.IsConstInt() || val.IsConstFloat());
        std::shared_ptr<Value> new_val;
        if (val.IsConstInt()) {
          new_val = m_builder->GetIntConstant(val.IntVal());
        } else {
          new_val = m_builder->GetFloatConstant(val.FloatVal());
        }
        load->ReplaceUseBy(new_val);
      }
    }
  }

  for (auto &var : const_gvs) {
    std::cerr << "[debug] marking global const array" << std::endl;
    for (auto it = var->m_use_list.begin(); it != var->m_use_list.end(); ++it) {
      auto user = it->get()->getUser();
      if (auto gep
          = std::dynamic_pointer_cast<GetElementPtrInstruction>(user)) {
        std::vector<int> indices;
        for (auto use : gep->m_indices) {
          if (auto c = std::dynamic_pointer_cast<Constant>(use->getValue())) {
            auto res = c->Evaluate();
            if (res.IsConstInt()) {
              indices.push_back(res.IntVal());
            }
          }
        }
        if (indices[0] == 0) {
          if (indices.size() == var->m_type.m_dimensions.size() + 1) {
            int offset = 0;
            int prod = 1;
            for (int i = var->m_type.m_dimensions.size() - 1; i >= 0; --i) {
              offset += indices[i + 1] * prod;
              prod *= var->m_type.m_dimensions[i];
            }
            auto val = var->GetFlattenVal(offset);
            for (auto &use : gep->m_use_list) {
              if (auto load = std::dynamic_pointer_cast<LoadInstruction>(
                      use->getUser())) {
                // replace this load instruction by val
                assert(val.IsConstInt() || val.IsConstFloat());
                std::shared_ptr<Value> new_val;
                if (val.IsConstInt()) {
                  new_val = m_builder->GetIntConstant(val.IntVal());
                } else {
                  new_val = m_builder->GetFloatConstant(val.FloatVal());
                }
                load->ReplaceUseBy(new_val);
              }
            }
          }
        }
      }
    }
  }

  for (auto &func : m_builder->m_module->m_function_list) {
    DeadCodeElimination(func);
  }
}