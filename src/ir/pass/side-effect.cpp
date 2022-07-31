//
// Created by garen on 7/22/22.
//

#include "ir/ir-pass-manager.h"

std::unordered_map<std::shared_ptr<Function>,
                   std::vector<std::shared_ptr<Function>>>
    called_graph;  // reversed graph of the original call graph (except for
                   // recursive relationship)

void dfs(std::shared_ptr<Function> callee, int depth) {
  if (callee->m_visited) return;
  callee->m_visited = true;
  callee->m_called_depth = depth;
  for (auto &caller : called_graph[callee]) {
    if (callee->HasSideEffect()) {
      caller->m_side_effect = true;
    }
    dfs(caller, depth + 1);
  }
}

bool HasSideEffect(std::shared_ptr<Instruction> instr) {
  switch (instr->m_op) {
    case IROp::LOAD: {
      auto load_instr = std::dynamic_pointer_cast<LoadInstruction>(instr);
      if (auto global_var = std::dynamic_pointer_cast<GlobalVariable>(
              load_instr->m_addr->getValue())) {
        return true;  // load to a global variable
      }
      if (auto alloca_var = std::dynamic_pointer_cast<AllocaInstruction>(
              load_instr->m_addr->getValue())) {
        if (alloca_var->m_is_arg) {
          return true;  // is an array from outer function
        }
      }
      return false;
    }
    case IROp::STORE: {
      auto store_instr = std::dynamic_pointer_cast<StoreInstruction>(instr);
      if (auto gep_instr = std::dynamic_pointer_cast<GetElementPtrInstruction>(
              store_instr->m_addr->getValue())) {
        if (auto alloca_instr = std::dynamic_pointer_cast<AllocaInstruction>(
                gep_instr->m_addr->getValue())) {
          // example: a[0] = 3, where a is the local array in stack
          return false;  // no side effect when writing local variable
        }
      }
      return true;
    }
    case IROp::GET_ELEMENT_PTR: {
      auto gep_instr
          = std::dynamic_pointer_cast<GetElementPtrInstruction>(instr);

      if (auto global_var = std::dynamic_pointer_cast<GlobalVariable>(
              gep_instr->m_addr->getValue())) {
        return true;  // load to a global variable
      }
      if (auto alloca_var = std::dynamic_pointer_cast<AllocaInstruction>(
              gep_instr->m_addr->getValue())) {
        if (alloca_var->m_is_arg) {
          return true;  // is an array from outer function
        }
      }
      return false;
    }
    case IROp::CALL: {
      auto call_instr = std::dynamic_pointer_cast<CallInstruction>(instr);
      return call_instr->m_function->m_side_effect;
    }
    default:
      return false;
  }
}

void ComputeSideEffect(std::unique_ptr<Module> &module) {
  called_graph.clear();
  for (auto &func : module->m_function_decl_list) {
    func->m_side_effect = true;  // I/O functions have side effect
  }
  for (auto &func : module->m_function_list) {
    func->m_visited = false;
    func->m_side_effect = false;  // 无罪推定是吧
    func->m_calls.clear();
  }
  for (auto &func : module->m_function_list) {
    for (auto &bb : func->m_bb_list) {
      for (auto &instr : bb->m_instr_list) {
        // check side effect by instructions
        if (HasSideEffect(instr)) {
          func->m_side_effect = true;
        }
        if (auto call_instr
            = std::dynamic_pointer_cast<CallInstruction>(instr)) {
          auto called_function = call_instr->m_function;
          if (called_function != func) {
            called_graph[called_function].push_back(func);
            called_function->m_calls.emplace_back(call_instr, func);
          }
        }
      }
    }
  }

  for (auto &func : module->m_function_list) {
    dfs(func, 1);  // check side effect from call graph
  }
}

void IRPassManager::SideEffectPass() {
  RemoveUnusedFunctions(m_builder->m_module);
  ComputeSideEffect(m_builder->m_module);
}