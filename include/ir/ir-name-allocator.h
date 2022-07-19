#ifndef BDDD_REGISTER_ALLOCATOR_H
#define BDDD_REGISTER_ALLOCATOR_H

#include "ir/ir.h"

class IRRegisterAllocator {
private:
  std::unordered_map<std::shared_ptr<Value>, std::string> m_name_of_global_vars;
  std::unordered_map<std::shared_ptr<Value>, std::string> m_name_of_labels;
  std::unordered_map<std::shared_ptr<Value>, std::string> m_name_of_local_vars;

  int m_local_var_cnt;
  int m_label_cnt;

public:
  explicit IRRegisterAllocator()
      : m_name_of_global_vars(),
        m_name_of_labels(),
        m_name_of_local_vars(),
        m_local_var_cnt(0),
        m_label_cnt(0) {}

  void SetGlobalVarName(const std::shared_ptr<Value>& val, std::string name) {
    assert(m_name_of_global_vars.find(val) == m_name_of_global_vars.end());
    m_name_of_global_vars[val] = std::move(name);
  }

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

    if (val->m_type.m_base_type == BaseType::LABEL) {
      auto it = m_name_of_labels.find(val);
      if (it == m_name_of_labels.end())
        m_name_of_labels[val] = "%L" + std::to_string(m_label_cnt++);
      return m_name_of_labels[val];
    }
    // try finding in global variables
    auto it = m_name_of_global_vars.find(val);
    if (it != m_name_of_global_vars.end()) {
      return it->second;
    }

    auto it2 = m_name_of_local_vars.find(val);
    if (it2 == m_name_of_local_vars.end())
      m_name_of_local_vars[val] = "%" + std::to_string(m_local_var_cnt++);

    return m_name_of_local_vars[val];
  }

  void ClearLocalVariables() {
    m_name_of_local_vars.clear();
    m_local_var_cnt = 0;
  }
};

#endif  // BDDD_REGISTER_ALLOCATOR_H
