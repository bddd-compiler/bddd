//
// Created by garen on 7/21/22.
//
#include "ir/ir-pass-manager.h"
#include "ir/ir.h"

void FunctionInlining(std::shared_ptr<Function> function,
                      std::shared_ptr<IRBuilder> builder) {
  // first identify whether a function can be inlined
  auto first_bb = function->m_bb_list.front();
  if (!first_bb->m_predecessors.empty()) return;  // cannot inline
  int cnt = 0;
  for (auto &bb : function->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      ++cnt;
      if (auto call_instr = std::dynamic_pointer_cast<CallInstruction>(instr)) {
        if (call_instr->m_function == function)
          return;  // recursive function cannot inline here
      }
    }
  }
  if (cnt >= 42) return;  // inlined function cannot have too many instructions
  // TODO
}

void IRPassManager::FunctionInliningPass() {
  for (auto func : m_builder->m_module->m_function_list) {
    FunctionInlining(func, m_builder);
  }
}
