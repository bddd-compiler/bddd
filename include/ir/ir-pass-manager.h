#ifndef BDDD_IR_PASS_MANAGER_H
#define BDDD_IR_PASS_MANAGER_H

#include "ir/builder.h"
#include "ir/pass/mem2reg.h"

class IRPassManager {
public:
  std::shared_ptr<IRBuilder> m_builder;

  explicit IRPassManager(std::shared_ptr<IRBuilder> builder)
      : m_builder(std::move(builder)) {}

  void Mem2RegPass() {
    for (auto func : m_builder->m_module->m_function_list) {
      Mem2Reg(func, m_builder);
    }
  }
};

#endif  // BDDD_IR_PASS_MANAGER_H
