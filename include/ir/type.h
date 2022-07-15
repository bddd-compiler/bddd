#ifndef BDDD_TYPE_H
#define BDDD_TYPE_H

#include <cassert>
#include <string>
#include <vector>

enum class BaseType {
  INT,    // i32 (array)
  FLOAT,  // float (array)
  BOOL,   // i1

  LABEL,
  VOID,
};

class ValueType {
public:
  BaseType m_base_type;
  std::vector<int> m_dimensions;
  int m_num_star;  // have *

  explicit ValueType(BaseType base_type, bool is_ptr = false)
      : m_base_type(base_type), m_dimensions(), m_num_star(is_ptr) {}

  explicit ValueType(BaseType base_type, std::vector<int> dimensions,
                     bool is_ptr = false)
      : m_base_type(base_type),
        m_dimensions(std::move(dimensions)),
        m_num_star(is_ptr) {}

  explicit ValueType(VarType var_type, bool is_ptr = false)
      : m_base_type(), m_dimensions(), m_num_star(is_ptr) {
    switch (var_type) {
      case VarType::INT:
        m_base_type = BaseType::INT;
        break;
      case VarType::FLOAT:
        m_base_type = BaseType::FLOAT;
        break;
      case VarType::VOID:
        m_base_type = BaseType::VOID;
        break;
      case VarType::UNKNOWN:
        assert(false);  // unreachable
    }
  }

  explicit ValueType(VarType var_type, std::vector<int> dimensions,
                     bool is_ptr = false)
      : ValueType(var_type, is_ptr) {
    m_dimensions = std::move(dimensions);
  }

  ValueType Dereference(int x = 1) {
    assert(m_num_star > 0);
    auto ret = *this;
    ret.m_num_star -= x;
    return ret;
  }

  ValueType Reference(int x = 1) {
    auto ret = *this;
    ret.m_num_star += x;
    return ret;
  }

  ValueType Reduce(int x) {
    assert(x <= m_dimensions.size());
    auto ret = *this;
    ret.m_dimensions.clear();
    for (int i = x; i < m_dimensions.size(); i++) {
      ret.m_dimensions.push_back(m_dimensions[i]);
    }
    return ret;
  }

  void Set(BaseType base_type, bool is_ptr = false) {
    m_base_type = base_type;
    m_num_star = is_ptr;
  }

  void Set(BaseType base_type, std::vector<int> dimensions,
           bool is_ptr = false) {
    m_base_type = base_type;
    m_dimensions = std::move(dimensions);
    m_num_star = is_ptr;
  }

  std::string ToString() {
    if (m_base_type == BaseType::VOID)
      return "void";
    else if (m_base_type == BaseType::LABEL)
      return "label";
    else if (m_base_type == BaseType::BOOL)
      return "i1";

    std::string ret;  // initially empty
    if (!m_dimensions.empty()) {
      for (auto dimension : m_dimensions) {
        ret = ret + '[' + std::to_string(dimension) + " x ";
      }
      if (m_base_type == BaseType::INT)
        ret += "i32";
      else if (m_base_type == BaseType::FLOAT)
        ret += "float";
      ret += std::string(m_dimensions.size(), ']');
    } else {
      if (m_base_type == BaseType::INT)
        ret += "i32";
      else if (m_base_type == BaseType::FLOAT)
        ret += "float";
    }
    assert(!ret.empty());
    bool first = true;
    for (int i = 0; i < m_num_star; ++i) {
      if (first)
        first = false;
      else
        ret += " ";
      ret += "*";
    }
    return std::move(ret);
  }

  bool operator==(const ValueType &rhs) const {
    if (m_base_type != rhs.m_base_type || m_num_star != rhs.m_num_star)
      return false;
    if (m_dimensions.size() != rhs.m_dimensions.size()) return false;
    for (int i = 0; i < m_dimensions.size(); ++i) {
      if (m_dimensions[i] != rhs.m_dimensions[i]) return false;
    }
    return true;
  }
};

#endif  // BDDD_TYPE_H
