#include <cassert>

#include "ir/ir.h"

void IRNameAllocator::SetGlobalVarName(const std::shared_ptr<Value>& val,
                                       std::string name) {
  assert(std::find_if(m_name_of_global_vars.begin(),
                      m_name_of_global_vars.end(),
                      [=](auto x) { return x.first.get() == val.get(); })
         == m_name_of_global_vars.end());
  val->m_allocated_name = m_name_of_global_vars[val] = std::move(name);
}
std::string IRNameAllocator::GetValueName(const std::shared_ptr<Value>& val) {
  assert(val != nullptr);

  // check if the value is constant
  if (auto constant = std::dynamic_pointer_cast<Constant>(val)) {
    if (constant->m_is_float)
      return val->m_allocated_name = std::to_string(constant->m_float_val);
    else
      return val->m_allocated_name = std::to_string(constant->m_int_val);
  }

  // find in basic blocks
  if (val->m_type.m_base_type == BaseType::LABEL) {
    auto it = std::find_if(
        m_name_of_labels.begin(), m_name_of_labels.end(),
        [=](const auto& x) { return x.first.get() == val.get(); });
    if (it == m_name_of_labels.end()) {
      return val->m_allocated_name = m_name_of_labels[val]
             = "%L" + std::to_string(m_label_cnt++);
    } else {
      return val->m_allocated_name = it->second;
    }
  }

  // find in global variables
  if (!m_name_of_global_vars.empty()) {
    auto it = std::find_if(m_name_of_global_vars.begin(),
                           m_name_of_global_vars.end(),
                           [=](auto x) { return x.first.get() == val.get(); });
    if (it != m_name_of_global_vars.end()) {
      return val->m_allocated_name = it->second;
    }
  }

  // find in local variables
  if (!m_name_of_local_vars.empty()) {
    auto it
        = std::find_if(m_name_of_local_vars.begin(), m_name_of_local_vars.end(),
                       [=](auto x) { return x.first.get() == val.get(); });
    if (it != m_name_of_local_vars.end()) {
      return val->m_allocated_name = it->second;
    }
  }

  // add into local variables
  return val->m_allocated_name = m_name_of_local_vars[val]
         = "%" + std::to_string(m_local_var_cnt++);
}
void IRNameAllocator::ClearLocalVariables() {
  m_name_of_local_vars.clear();
  m_local_var_cnt = 0;
}
