#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

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
    switch (constant->m_type.m_base_type) {
      case BasicType::INT:
      case BasicType::CHAR:
        return val->m_allocated_name = std::to_string(constant->m_int_val);
      case BasicType::FLOAT: {
        std::stringstream ss;
        union MyUnion {
          double m_double_val;
          uint64_t m_int_val;
        } test;
        test.m_double_val = constant->m_float_val;
        ss << "0x" << std::hex << std::setfill('0') << std::setw(16)
           << test.m_int_val;
        ss >> val->m_allocated_name;
        return val->m_allocated_name;
      }
      case BasicType::BOOL:
        return val->m_allocated_name = (constant->m_int_val ? "true" : "false");
      default:
        assert(false);
    }
  }

  // find in basic blocks
  if (val->m_type.IsLabel()) {
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
