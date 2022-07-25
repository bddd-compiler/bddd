//
// Created by garen on 7/24/22.
//

#ifndef BDDD_TYPE_H
#define BDDD_TYPE_H

#include <cassert>

#include "exceptions.h"

enum class EvalType {
  CONST_INT,  // BOOL is included, non-zero is true, zero is false
  CONST_FLOAT,
  VAR_INT,
  VAR_FLOAT,
  ARR,  // array cannot join with any computation
  VOID,
  ERROR,
};

class EvalValue {
private:
  int m_int_val;
  float m_float_val;

public:
  EvalType m_eval_type;

  explicit EvalValue()
      : m_eval_type(EvalType::ERROR), m_int_val(0), m_float_val(0.0) {}
  explicit EvalValue(EvalType eval_type)
      : m_eval_type(eval_type), m_int_val(0), m_float_val(0.0) {}
  explicit EvalValue(int int_val)
      : m_eval_type(EvalType::CONST_INT),
        m_int_val(int_val),
        m_float_val(0.0) {}
  explicit EvalValue(float float_val)
      : m_eval_type(EvalType::CONST_FLOAT),
        m_int_val(0),
        m_float_val(float_val) {}

  bool operator==(const EvalValue &rhs) const {
    if (m_eval_type != rhs.m_eval_type) return false;

    if (m_eval_type == EvalType::CONST_FLOAT)
      return m_float_val == rhs.m_float_val;
    else if (m_eval_type == EvalType::CONST_INT)
      return m_int_val == rhs.m_int_val;
    else
      return false;  // if both non-const, cannot be equal
  }

  [[nodiscard]] EvalValue ToFloat() const {
    assert(m_eval_type == EvalType::CONST_INT);
    return EvalValue(static_cast<float>(m_int_val));
  }

  [[nodiscard]] EvalValue ToInt() const {
    assert(m_eval_type == EvalType::CONST_FLOAT);
    return EvalValue(static_cast<int>(m_float_val));
  }

  EvalValue operator+(const EvalValue &_rhs) const {
    if (IsInt() && _rhs.IsInt()) {
      // both int, result is int too
      return EvalValue(m_int_val + _rhs.m_int_val);
    }
    // otherwise, change to float first
    auto lhs = IsFloat() ? *this : this->ToFloat();
    auto rhs = _rhs.IsFloat() ? _rhs : _rhs.ToFloat();
    return EvalValue(lhs.FloatVal() + rhs.FloatVal());
  }

  EvalValue operator-(const EvalValue &_rhs) const {
    if (IsInt() && _rhs.IsInt()) {
      // both int, result is int too
      return EvalValue(m_int_val - _rhs.m_int_val);
    }
    // otherwise, change to float first
    auto lhs = IsFloat() ? *this : this->ToFloat();
    auto rhs = _rhs.IsFloat() ? _rhs : _rhs.ToFloat();
    return EvalValue(lhs.FloatVal() - rhs.FloatVal());
  }

  EvalValue operator*(const EvalValue &_rhs) const {
    if (IsInt() && _rhs.IsInt()) {
      // both int, result is int too
      return EvalValue(m_int_val * _rhs.m_int_val);
    }
    // otherwise, change to float first
    auto lhs = IsFloat() ? *this : this->ToFloat();
    auto rhs = _rhs.IsFloat() ? _rhs : _rhs.ToFloat();
    return EvalValue(lhs.FloatVal() * rhs.FloatVal());
  }

  EvalValue operator/(const EvalValue &_rhs) const {
    if (IsInt() && _rhs.IsInt()) {
      // both int, result is int too
      if (_rhs.IntVal() == 0) throw MyException("div by zero");
      return EvalValue(IntVal() / _rhs.IntVal());
    }
    // otherwise, change to float first
    auto lhs = IsFloat() ? *this : this->ToFloat();
    auto rhs = _rhs.IsFloat() ? _rhs : _rhs.ToFloat();
    if (_rhs.FloatVal() == 0) throw MyException("div by zero");
    return EvalValue(lhs.FloatVal() / rhs.FloatVal());
  }

  EvalValue operator%(const EvalValue &_rhs) const {
    if (!IsInt() && !_rhs.IsInt()) {
      // both int, result is int too
      if (_rhs.IntVal() == 0) throw MyException("mod by zero");
      return EvalValue(IntVal() % _rhs.IntVal());
    }
    // otherwise, throw exception
    throw MyException("operator % only supports integers");
  }

  [[nodiscard]] bool Equals(int int_val,
                            bool allow_implicit_conversion = false) const {
    bool ret = (m_eval_type == EvalType::CONST_INT && m_int_val == int_val);
    if (allow_implicit_conversion)
      ret = ret
            || (m_eval_type == EvalType::CONST_FLOAT
                && m_float_val == static_cast<float>(int_val));
    return ret;
  }

  [[nodiscard]] bool Equals(float float_val,
                            bool allow_implicit_conversion = false) const {
    bool ret
        = (m_eval_type == EvalType::CONST_FLOAT && m_float_val == float_val);
    if (allow_implicit_conversion)
      ret = ret
            || (m_eval_type == EvalType::CONST_FLOAT
                && m_int_val == static_cast<int>(float_val));
    return ret;
  }

  [[nodiscard]] bool IsInt() const {
    return m_eval_type == EvalType::CONST_INT
           || m_eval_type == EvalType::VAR_INT;
  }
  [[nodiscard]] bool IsFloat() const {
    return m_eval_type == EvalType::CONST_FLOAT
           || m_eval_type == EvalType::VAR_FLOAT;
  }
  [[nodiscard]] bool IsConstInt() const {
    return m_eval_type == EvalType::CONST_INT;
  }
  [[nodiscard]] bool IsConstFloat() const {
    return m_eval_type == EvalType::CONST_FLOAT;
  }
  [[nodiscard]] bool IsConst() const {
    return m_eval_type == EvalType::CONST_INT
           || m_eval_type == EvalType::CONST_FLOAT;
  }

  [[nodiscard]] bool IsSingle() const {
    return m_eval_type == EvalType::CONST_INT
           || m_eval_type == EvalType::CONST_FLOAT
           || m_eval_type == EvalType::VAR_INT
           || m_eval_type == EvalType::VAR_FLOAT;
  }

  [[nodiscard]] int IntVal() const {
    assert(m_eval_type == EvalType::CONST_INT);
    return m_int_val;
  }
  [[nodiscard]] float FloatVal() const {
    assert(m_eval_type == EvalType::CONST_FLOAT);
    return m_float_val;
  }
};

#endif  // BDDD_TYPE_H
