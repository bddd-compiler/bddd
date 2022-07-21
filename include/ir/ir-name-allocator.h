#ifndef BDDD_REGISTER_ALLOCATOR_H
#define BDDD_REGISTER_ALLOCATOR_H

#include <memory>
#include <unordered_map>
class Value;

class IRNameAllocator {
private:
  std::unordered_map<std::shared_ptr<Value>, std::string> m_name_of_global_vars;
  std::unordered_map<std::shared_ptr<Value>, std::string> m_name_of_labels;
  std::unordered_map<std::shared_ptr<Value>, std::string> m_name_of_local_vars;

  int m_local_var_cnt;
  int m_label_cnt;

public:
  explicit IRNameAllocator()
      : m_name_of_global_vars(),
        m_name_of_labels(),
        m_name_of_local_vars(),
        m_local_var_cnt(0),
        m_label_cnt(1) {}

  void SetGlobalVarName(const std::shared_ptr<Value>& val, std::string name);

  // return in string
  // example: %1, %L1
  std::string GetValueName(const std::shared_ptr<Value>& val);

  void ClearLocalVariables();
};

#endif  // BDDD_REGISTER_ALLOCATOR_H
