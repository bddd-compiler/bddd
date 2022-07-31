#ifndef BDDD_REGISTER_ALLOCATOR_H
#define BDDD_REGISTER_ALLOCATOR_H

#include <memory>
#include <unordered_map>
class Value;

class IRNameAllocator {
private:
  std::unordered_map<std::shared_ptr<Value>, size_t> m_name_of_values;

  size_t m_cnt;

public:
  explicit IRNameAllocator() : m_name_of_values(), m_cnt(0) {}

  std::string GetValueName(const std::shared_ptr<Value>& val);
  std::string GetLabelName(const std::shared_ptr<Value>& val);
  void Insert(std::shared_ptr<Value> val);

  void Clear();
};

#endif  // BDDD_REGISTER_ALLOCATOR_H
