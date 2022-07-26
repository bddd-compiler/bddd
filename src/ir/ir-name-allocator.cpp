#include "ir/ir-name-allocator.h"

#include <cassert>
#include <iomanip>
#include <iostream>

#include "ir/ir.h"

// applicable for g_value_allocator
std::string IRNameAllocator::GetValueName(const std::shared_ptr<Value>& val) {
  assert(val != nullptr);

  // check if the value is constant
  if (auto constant = std::dynamic_pointer_cast<Constant>(val)) {
    switch (constant->m_type.m_base_type) {
      case BasicType::INT:
      case BasicType::CHAR:
        return std::to_string(constant->m_int_val);
      case BasicType::FLOAT: {
        std::stringstream ss;
        std::string ret;
        union MyUnion {
          double m_double_val;
          uint64_t m_int_val;
        } test{};
        test.m_double_val = constant->m_float_val;
        ss << "0x" << std::hex << std::setfill('0') << std::setw(16)
           << test.m_int_val;
        ss >> ret;
        return ret;
      }
      case BasicType::BOOL:
        return (constant->m_int_val ? "true" : "false");
      default:
        assert(false);
    }
  }

  // check if val is global variable
  if (auto global_var = std::dynamic_pointer_cast<GlobalVariable>(val)) {
    return "@" + global_var->m_name;
  }

  auto it = m_name_of_values.find(val);
  if (it != m_name_of_values.end()) {
    return "%v" + std::to_string(it->second);
  } else {
    m_name_of_values[val] = m_cnt++;
    return "%v" + std::to_string(m_name_of_values[val]);
  }
}

// only applicable for g_label_allocator
std::string IRNameAllocator::GetLabelName(const std::shared_ptr<Value>& val) {
  auto it = m_name_of_values.find(val);
  if (it != m_name_of_values.end()) {
    return "%L" + std::to_string(it->second);
  } else {
    m_name_of_values[val] = m_cnt++;
    return "%L" + std::to_string(m_name_of_values[val]);
  }
}

void IRNameAllocator::Clear() {
  m_name_of_values.clear();
  m_cnt = 0;
}
void IRNameAllocator::Insert(std::shared_ptr<Value> val) {
  m_name_of_values[val] = m_cnt++;
}
