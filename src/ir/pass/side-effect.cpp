//
// Created by garen on 7/22/22.
//

#include "ir/ir-pass-manager.h"

std::unordered_map<std::shared_ptr<Function>,
                   std::vector<std::shared_ptr<Function>>>
    called_graph;  // reversed graph of the original call graph

void dfs(std::shared_ptr<Function> callee) {
  if (callee->m_visited) return;
  callee->m_visited = true;
  for (auto &caller : called_graph[callee]) {
    if (callee->HasSideEffect()) {
      caller->m_side_effect = true;
    }
    dfs(caller);
  }
}

bool HasSideEffect(std::shared_ptr<Instruction> instr) {
  switch (instr->m_op) {
    case IROp::LOAD: {
      auto load_instr = std::dynamic_pointer_cast<LoadInstruction>(instr);
      assert(load_instr != nullptr);
      if (auto global_var = std::dynamic_pointer_cast<GlobalVariable>(
              load_instr->m_addr->m_value)) {
        return true;  // load to a global variable
      }
      if (auto alloca_var = std::dynamic_pointer_cast<AllocaInstruction>(
              load_instr->m_addr->m_value)) {
        if (alloca_var->m_is_arg) {
          return true;  // is an array from outer function
        }
      }
      return false;
    }
    case IROp::STORE: {
      auto store_instr = std::dynamic_pointer_cast<StoreInstruction>(instr);
      assert(store_instr != nullptr);
      if (auto gep_instr = std::dynamic_pointer_cast<GetElementPtrInstruction>(
              store_instr->m_addr->m_value)) {
        if (auto alloca_instr = std::dynamic_pointer_cast<AllocaInstruction>(
                gep_instr->m_addr->m_value)) {
          // example: a[0] = 3, where a is the local array in stack
          return false;  // no side effect when writing local variable
        }
      }
      return true;
    }
    case IROp::GET_ELEMENT_PTR: {
      auto gep_instr
          = std::dynamic_pointer_cast<GetElementPtrInstruction>(instr);
      assert(gep_instr != nullptr);

      if (auto global_var = std::dynamic_pointer_cast<GlobalVariable>(
              gep_instr->m_addr->m_value)) {
        return true;  // load to a global variable
      }
      if (auto alloca_var = std::dynamic_pointer_cast<AllocaInstruction>(
              gep_instr->m_addr->m_value)) {
        if (alloca_var->m_is_arg) {
          return true;  // is an array from outer function
        }
      }
      return false;
    }
    default:
      return false;
  }
}

void ComputeSideEffect(std::unique_ptr<Module> &module) {
  called_graph.clear();
  for (auto &func : module->m_function_list) {
    func->m_visited = false;
    func->m_side_effect = false;  // 无罪推定是吧
  }
  for (auto &func : module->m_function_list) {
    for (auto &bb : func->m_bb_list) {
      for (auto &instr : bb->m_instr_list) {
        if (HasSideEffect(instr)) {
          func->m_side_effect = true;
        }
        if (auto call_instr
            = std::dynamic_pointer_cast<CallInstruction>(instr)) {
          auto called_function = call_instr->m_function;
          called_graph[called_function].push_back(func);
        }
      }
    }
  }

  for (auto &func : module->m_function_list) {
    dfs(func);
  }
}

void IRPassManager::SideEffectPass() { ComputeSideEffect(m_builder->m_module); }