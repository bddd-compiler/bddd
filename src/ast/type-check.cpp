#include <cassert>

#include "ast/ast.h"
#include "ast/symbol-table.h"
#include "exceptions.h"

void InitValAST::TypeCheck(SymbolTable& symbol_table) {
  if (m_expr) {
    m_expr->TypeCheck(symbol_table);  // m_expr->m_is_const is updated here
    SetIsConst(m_expr->IsConst());
  } else {
    bool flag = true;
    for (auto& val : m_vals) {
      val->TypeCheck(symbol_table);
      if (!val->IsConst() && flag) flag = false;
    }
    SetIsConst(flag);
  }
}
void InitValAST::FillVals(int n, int& offset, const std::vector<int>& sizes,
                          std::vector<std::shared_ptr<ExprAST>>& vals) {
  auto temp = offset / sizes[n];
  auto l = temp * sizes[n];
  auto r = l + sizes[n];
  auto size = (n == sizes.size() - 1 ? 1 : sizes[n + 1]);

  // assign values
  // TODO(garen): WARNING!!! nullptr represents zero, danger behaviour
  for (auto& val : m_vals) {
    if (val->m_expr) {  // single
      vals[offset++] = val->m_expr;
    } else {  // array

      while (offset % size) vals[offset++] = nullptr;  // align
      val->FillVals(n + 1, offset, sizes, vals);
    }
  }
  while (offset < r) vals[offset++] = nullptr;  // align
}
void LValAST::TypeCheck(SymbolTable& symbol_table) {
  auto ptr = symbol_table.GetDecl(m_name);
  if (!ptr) throw MyException("undefined lval");
  m_decl = ptr;
  if (m_decl->DimensionsSize() < m_dimensions.size())
    throw MyException("array m_dimensions of lval more than real array");

  for (auto& dimension : m_dimensions) {
    dimension->TypeCheck(symbol_table);
  }
}
std::variant<int, float> LValAST::Evaluate(SymbolTable& symbol_table) {
  assert(!IsArray());
  assert(m_decl != nullptr);
  if (m_dimensions.empty()) {
    return m_decl->GetFlattenVal(symbol_table, 0);
  }

  int offset = 0;

  assert(m_decl->m_products.size() == m_decl->m_dimensions.size());
  size_t i;
  for (i = 0; i < m_dimensions.size() - 1; i++) {
    auto [type, res] = m_dimensions[i]->Evaluate(symbol_table);
    assert(type == ExprAST::EvalType::INT);
    offset += std::get<int>(res) * m_decl->m_products[i + 1];
  }
  auto [type, res] = m_dimensions[i]->Evaluate(symbol_table);
  assert(type == ExprAST::EvalType::INT || type == ExprAST::EvalType::VAR_INT);
  offset += std::get<int>(res);

  return m_decl->GetFlattenVal(symbol_table, offset);
}
void ExprAST::TypeCheck(SymbolTable& symbol_table) {
  auto [var_type, res] = Evaluate(symbol_table);
  // INT, FLOAT, VAR_INT, VAR_FLOAT, anything except ERROR, are legal results
  switch (var_type) {
    case EvalType::INT:
    case EvalType::FLOAT:
      SetIsConst(true);
      break;
    case EvalType::VAR_INT:
    case EvalType::VAR_FLOAT:
      SetIsConst(false);
      break;
    case EvalType::ARR:
    case EvalType::VOID:
      // do nothing
      break;
    case EvalType::ERROR:
      throw MyException("unexpected evaluation");
      break;
    default:
      break;
  }
}

// if a is int variable, if (+--!!!a) {} is legal expression
// there is no distinction between int and bool
// array may be evaluated, which cannot join in any computation
std::pair<ExprAST::EvalType, std::variant<int, float>> ExprAST::Evaluate(
    SymbolTable& symbol_table) {
  auto ret = EvaluateInner(symbol_table);
  // memorize the answer
  if (ret.first == EvalType::INT) {
    m_op = Op::CONST_INT;
    m_int_val = std::get<int>(ret.second);
  } else if (ret.first == EvalType::FLOAT) {
    m_op = Op::CONST_FLOAT;
    m_float_val = std::get<float>(ret.second);
  }
  return ret;
}

std::pair<ExprAST::EvalType, std::variant<int, float>> ExprAST::EvaluateInner(
    SymbolTable& symbol_table) {
  std::pair<EvalType, std::variant<int, float>> lhs_res, rhs_res;
  switch (m_op) {
    case Op::POSITIVE:
      return m_lhs->Evaluate(symbol_table);
      break;
    case Op::NEGATIVE:
      lhs_res = m_lhs->Evaluate(symbol_table);
      if (lhs_res.first == EvalType::INT)
        return std::make_pair(lhs_res.first, -std::get<int>(lhs_res.second));
      else if (lhs_res.first == EvalType::FLOAT)
        return std::make_pair(lhs_res.first, -std::get<float>(lhs_res.second));
      else if (lhs_res.first == EvalType::VAR_INT)
        return std::make_pair(EvalType::VAR_INT, 0);
      else if (lhs_res.first == EvalType::VAR_FLOAT)
        return std::make_pair(EvalType::VAR_FLOAT, 0);
      else
        return std::make_pair(EvalType::ERROR, 0);
      break;
    case Op::PLUS:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      if (lhs_res.first == EvalType::INT) {
        if (rhs_res.first == EvalType::INT) {
          return std::make_pair(
              EvalType::INT,
              std::get<int>(lhs_res.second) + std::get<int>(rhs_res.second));
        } else if (rhs_res.first == EvalType::FLOAT) {
          return std::make_pair(
              EvalType::FLOAT, static_cast<float>(std::get<int>(lhs_res.second))
                                   + std::get<float>(rhs_res.second));
        } else if (rhs_res.first == EvalType::VAR_INT) {
          return std::make_pair(EvalType::VAR_INT, 0);
        } else if (rhs_res.first == EvalType::VAR_FLOAT) {
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::FLOAT) {
        if (rhs_res.first == EvalType::INT) {
          return std::make_pair(
              EvalType::FLOAT,
              std::get<float>(lhs_res.second)
                  + static_cast<float>(std::get<int>(rhs_res.second)));
        } else if (rhs_res.first == EvalType::FLOAT) {
          return std::make_pair(EvalType::FLOAT,
                                std::get<float>(lhs_res.second)
                                    + std::get<float>(rhs_res.second));
        } else if (rhs_res.first == EvalType::VAR_INT) {
          return std::make_pair(EvalType::VAR_INT, 0);
        } else if (rhs_res.first == EvalType::VAR_FLOAT) {
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR_INT) {
        if (rhs_res.first == EvalType::INT
            || rhs_res.first == EvalType::VAR_INT)
          return std::make_pair(EvalType::VAR_INT, 0);
        else if (rhs_res.first == EvalType::FLOAT
                 || rhs_res.first == EvalType::VAR_FLOAT)
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR_FLOAT) {
        if (rhs_res.first == EvalType::INT || rhs_res.first == EvalType::FLOAT
            || rhs_res.first == EvalType::VAR_INT
            || rhs_res.first == EvalType::VAR_FLOAT)
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        else
          return std::make_pair(EvalType::ERROR, 0);
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::MINUS:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      if (lhs_res.first == EvalType::INT) {
        if (rhs_res.first == EvalType::INT) {
          return std::make_pair(
              EvalType::INT,
              std::get<int>(lhs_res.second) - std::get<int>(rhs_res.second));
        } else if (rhs_res.first == EvalType::FLOAT) {
          return std::make_pair(
              EvalType::FLOAT, static_cast<float>(std::get<int>(lhs_res.second))
                                   - std::get<float>(rhs_res.second));
        } else if (rhs_res.first == EvalType::VAR_INT) {
          return std::make_pair(EvalType::VAR_INT, 0);
        } else if (rhs_res.first == EvalType::VAR_FLOAT) {
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::FLOAT) {
        if (rhs_res.first == EvalType::INT) {
          return std::make_pair(
              EvalType::FLOAT,
              std::get<float>(lhs_res.second)
                  - static_cast<float>(std::get<int>(rhs_res.second)));
        } else if (rhs_res.first == EvalType::FLOAT) {
          return std::make_pair(EvalType::FLOAT,
                                std::get<float>(lhs_res.second)
                                    - std::get<float>(rhs_res.second));
        } else if (rhs_res.first == EvalType::VAR_INT
                   || rhs_res.first == EvalType::VAR_FLOAT) {
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR_INT) {
        if (rhs_res.first == EvalType::INT
            || rhs_res.first == EvalType::VAR_INT)
          return std::make_pair(EvalType::VAR_INT, 0);
        else if (rhs_res.first == EvalType::FLOAT
                 || rhs_res.first == EvalType::VAR_FLOAT)
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR_FLOAT) {
        if (rhs_res.first == EvalType::INT || rhs_res.first == EvalType::FLOAT
            || rhs_res.first == EvalType::VAR_INT
            || rhs_res.first == EvalType::VAR_FLOAT)
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        else
          return std::make_pair(EvalType::ERROR, 0);
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::MULTI:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      if (lhs_res.first == EvalType::INT) {
        if (rhs_res.first == EvalType::INT) {
          return std::make_pair(
              EvalType::INT,
              std::get<int>(lhs_res.second) * std::get<int>(rhs_res.second));
        } else if (rhs_res.first == EvalType::FLOAT) {
          return std::make_pair(
              EvalType::FLOAT, static_cast<float>(std::get<int>(lhs_res.second))
                                   * std::get<float>(rhs_res.second));
        } else if (rhs_res.first == EvalType::VAR_INT) {
          return std::make_pair(EvalType::VAR_INT, 0);
        } else if (rhs_res.first == EvalType::VAR_FLOAT) {
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::FLOAT) {
        if (rhs_res.first == EvalType::INT) {
          return std::make_pair(
              EvalType::FLOAT,
              std::get<float>(lhs_res.second)
                  * static_cast<float>(std::get<int>(rhs_res.second)));
        } else if (rhs_res.first == EvalType::FLOAT) {
          return std::make_pair(EvalType::FLOAT,
                                std::get<float>(lhs_res.second)
                                    * std::get<float>(rhs_res.second));
        } else if (rhs_res.first == EvalType::VAR_INT
                   || rhs_res.first == EvalType::VAR_FLOAT) {
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR_INT) {
        if (rhs_res.first == EvalType::INT
            || rhs_res.first == EvalType::VAR_INT)
          return std::make_pair(EvalType::VAR_INT, 0);
        else if (rhs_res.first == EvalType::FLOAT
                 || rhs_res.first == EvalType::VAR_FLOAT)
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR_FLOAT) {
        return std::make_pair(EvalType::VAR_FLOAT, 0);
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::DIV:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      if (lhs_res.first == EvalType::INT) {
        if (rhs_res.first == EvalType::INT) {
          auto dividend = std::get<int>(lhs_res.second);
          auto divisor = std::get<int>(rhs_res.second);
          if (divisor == 0) {
            return std::make_pair(EvalType::ERROR, 0);
          }
          return std::make_pair(EvalType::INT, dividend / divisor);
        } else if (rhs_res.first == EvalType::FLOAT) {
          auto dividend = std::get<int>(lhs_res.second);
          auto divisor = std::get<float>(rhs_res.second);
          if (divisor == 0) {
            return std::make_pair(EvalType::ERROR, 0);
          }
          return std::make_pair(EvalType::FLOAT,
                                static_cast<float>(dividend) / divisor);
        } else if (rhs_res.first == EvalType::VAR_INT) {
          // divided-by-zero may happen in runtime
          return std::make_pair(EvalType::VAR_INT, 0);
        } else if (rhs_res.first == EvalType::VAR_FLOAT) {
          // divided-by-zero may happen in runtime
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::FLOAT) {
        if (rhs_res.first == EvalType::INT) {
          auto dividend = std::get<float>(lhs_res.second);
          auto divisor = std::get<int>(rhs_res.second);
          if (divisor == 0) {
            return std::make_pair(EvalType::ERROR, 0);
          }
          return std::make_pair(EvalType::FLOAT,
                                dividend / static_cast<float>(divisor));
        } else if (rhs_res.first == EvalType::FLOAT) {
          auto dividend = std::get<float>(lhs_res.second);
          auto divisor = std::get<float>(rhs_res.second);
          if (divisor == 0) {
            return std::make_pair(EvalType::ERROR, 0);
          }
          return std::make_pair(EvalType::FLOAT, dividend / divisor);
        } else if (rhs_res.first == EvalType::VAR_INT
                   || rhs_res.first == EvalType::VAR_FLOAT) {
          // divided-by-zero may happen in runtime
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR_INT) {
        if (rhs_res.first == EvalType::INT
            || rhs_res.first == EvalType::VAR_INT)
          // divided-by-zero may happen in runtime
          return std::make_pair(EvalType::VAR_INT, 0);
        else if (rhs_res.first == EvalType::FLOAT
                 || rhs_res.first == EvalType::VAR_FLOAT)
          // divided-by-zero may happen in runtime
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR_FLOAT) {
        return std::make_pair(EvalType::VAR_FLOAT, 0);
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::MOD:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      if (lhs_res.first == EvalType::INT) {
        if (rhs_res.first == EvalType::INT) {
          auto dividend = std::get<int>(lhs_res.second);
          auto divisor = std::get<int>(rhs_res.second);
          if (divisor == 0) {
            return std::make_pair(EvalType::ERROR, 0);
          }
          return std::make_pair(EvalType::INT, dividend % divisor);
        } else if (rhs_res.first == EvalType::VAR_INT) {
          // divided-by-zero may happen in runtime
          return std::make_pair(EvalType::VAR_INT, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR_INT) {
        if (rhs_res.first == EvalType::INT
            || rhs_res.first == EvalType::VAR_INT) {
          return std::make_pair(EvalType::VAR_INT, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::LE:
    case Op::LEQ:
    case Op::GE:
    case Op::GEQ:
    case Op::EQ:
    case Op::NEQ:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      if ((lhs_res.first == EvalType::INT || lhs_res.first == EvalType::FLOAT)
          && (rhs_res.first == EvalType::INT
              || rhs_res.first == EvalType::FLOAT)) {
        // lhs and rhs are constant, then the result is also constant
        float lhs_num, rhs_num;
        if (lhs_res.first == EvalType::INT)
          lhs_num = static_cast<float>(std::get<int>(lhs_res.second));
        else
          lhs_num = std::get<float>(lhs_res.second);
        if (rhs_res.first == EvalType::INT)
          rhs_num = static_cast<float>(std::get<int>(rhs_res.second));
        else
          rhs_num = std::get<float>(rhs_res.second);

        if (m_op == Op::LE)
          return std::make_pair(EvalType::INT, lhs_num < rhs_num ? 1 : 0);
        else if (m_op == Op::LEQ)
          return std::make_pair(EvalType::INT, lhs_num <= rhs_num ? 1 : 0);
        else if (m_op == Op::GE)
          return std::make_pair(EvalType::INT, lhs_num > rhs_num ? 1 : 0);
        else if (m_op == Op::GEQ)
          return std::make_pair(EvalType::INT, lhs_num >= rhs_num ? 1 : 0);
        else if (m_op == Op::EQ)
          return std::make_pair(EvalType::INT, lhs_num == rhs_num ? 1 : 0);
        else if (m_op == Op::NEQ)
          return std::make_pair(EvalType::INT, lhs_num != rhs_num ? 1 : 0);
        else
          assert(false);  // unreachable

      } else if ((lhs_res.first == EvalType::INT
                  || lhs_res.first == EvalType::FLOAT
                  || lhs_res.first == EvalType::VAR_INT
                  || lhs_res.first == EvalType::VAR_FLOAT)
                 && ((rhs_res.first == EvalType::INT
                      || rhs_res.first == EvalType::FLOAT
                      || rhs_res.first == EvalType::VAR_INT
                      || rhs_res.first == EvalType::VAR_FLOAT))) {
        // either one of them is not constant, then may be 1 or 0
        return std::make_pair(EvalType::VAR_INT, 0);
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::AND:
    case Op::OR:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      if ((lhs_res.first == EvalType::INT || lhs_res.first == EvalType::VAR_INT
           || lhs_res.first == EvalType::FLOAT
           || lhs_res.first == EvalType::VAR_FLOAT)
          && (rhs_res.first == EvalType::INT
              || rhs_res.first == EvalType::VAR_INT
              || rhs_res.first == EvalType::FLOAT
              || rhs_res.first == EvalType::VAR_FLOAT)) {
        return std::make_pair(EvalType::VAR_INT, 0);
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::NOT:
      lhs_res = m_lhs->Evaluate(symbol_table);
      if (lhs_res.first == EvalType::INT) {
        // TODO(garen): naive patch here, just a temp solution
        if (std::get<int>(lhs_res.second) != 0)
          return std::make_pair(EvalType::INT, 0);
        else
          return std::make_pair(EvalType::INT, 1);
      } else if (lhs_res.first == EvalType::FLOAT) {
        if (std::get<float>(lhs_res.second) != 0)
          return std::make_pair(EvalType::INT, 0);
        else
          return std::make_pair(EvalType::INT, 1);
      } else if (lhs_res.first == EvalType::VAR_INT
                 || lhs_res.first == EvalType::VAR_FLOAT) {
        return std::make_pair(EvalType::VAR_INT, 0);
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::CONST_INT:
      return std::make_pair(EvalType::INT, m_int_val);
      break;
    case Op::CONST_FLOAT:
      return std::make_pair(EvalType::FLOAT, m_float_val);
      break;
    case Op::LVAL:
      m_lval->TypeCheck(symbol_table);
      // if (m_lval->m_decl == nullptr) {
      //   // assign value to lval->decl
      //   m_lval->m_decl = symbol_table.GetDecl(m_lval->Name());
      // }

      assert(m_lval->m_decl != nullptr);

      // can we handle lval array?
      // yes, just give a new enum variant ARR
      if (m_lval->IsArray()) return std::make_pair(EvalType::ARR, 0);

      if (m_lval->m_decl->IsConst()) {
        auto var_type = m_lval->m_decl->GetVarType();
        if (var_type == VarType::INT) {
          return std::make_pair(EvalType::INT,
                                std::get<int>(m_lval->Evaluate(symbol_table)));
        } else if (var_type == VarType::FLOAT) {
          return std::make_pair(
              EvalType::FLOAT, std::get<float>(m_lval->Evaluate(symbol_table)));
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else {
        auto type = m_lval->m_decl->GetVarType();
        if (type == VarType::INT)
          return std::make_pair(EvalType::VAR_INT, 0);
        else if (type == VarType::FLOAT)
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        else
          return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::FUNC_CALL:
      m_func_call->TypeCheck(symbol_table);

      switch (m_func_call->ReturnType()) {
        case VarType::INT:
          return std::make_pair(EvalType::VAR_INT, 0);
        case VarType::FLOAT:
          return std::make_pair(EvalType::VAR_FLOAT, 0);
        case VarType::VOID:
          // TODO(garen): temp solution with serious problems
          // void function call should return nothing, rather than an integer
          return std::make_pair(EvalType::VOID, 0);
        default:
          return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    default:
      assert(false);  // unreachable
      break;
  }
}
void DeclAST::TypeCheck(SymbolTable& symbol_table) {
  if (m_var_type != VarType::INT && m_var_type != VarType::FLOAT)
    throw MyException("unexpected var_type in DeclAST");

  if (m_is_param) {
    if (!m_dimensions.empty()) {
      for (int i = m_dimensions.size() - 1; i > 0; --i) {
        m_dimensions[i]->TypeCheck(symbol_table);
      }
      if (!m_dimensions[0]->IsConst() || m_dimensions[0]->IntVal() != -1) {
        throw MyException("the first dimension of param array is not empty?!");
      }
    }
    return;  // param finished here
  }
  // not a param of function
  if (m_init_val) {
    m_init_val->TypeCheck(symbol_table);
    if (m_init_val->m_expr) {
      // single init-val
      auto op = m_init_val->m_expr->GetOp();
      if (m_var_type == VarType::INT) {
        if (op == Op::CONST_FLOAT) {
          auto temp = static_cast<int>(m_init_val->m_expr->FloatVal());
          m_init_val->m_expr = std::make_shared<ExprAST>(temp);
        }
      } else if (m_var_type == VarType::FLOAT) {
        if (op == Op::CONST_INT) {
          auto temp = static_cast<float>(m_init_val->m_expr->IntVal());
          m_init_val->m_expr = std::make_shared<ExprAST>(temp);
        }
      }
    }
  }

  if (m_is_const && !m_init_val)
    throw MyException("const declaration without init val");
  if (m_is_const && m_init_val && !m_init_val->IsConst())
    throw MyException("const declaration with non-const init val");

  if (m_dimensions.empty()) {  // single
    if (m_init_val && m_init_val->m_expr) {
      m_flatten_vals.push_back(m_init_val->m_expr);
      assert(m_flatten_vals.size() == 1);
    } else {
      // TODO(garen): undefined value, try inserting a random value
      m_flatten_vals.push_back(nullptr);
    }
  } else {  // array
    int tot = 1;
    m_products.resize(m_dimensions.size());
    for (int i = m_dimensions.size() - 1; i >= 0; --i) {
      m_dimensions[i]->TypeCheck(symbol_table);
      auto [type, res] = m_dimensions[i]->Evaluate(symbol_table);
      assert(type == ExprAST::EvalType::INT);
      tot *= std::get<int>(res);
      m_products[i] = tot;
    }
    m_flatten_vals.resize(tot);
    if (m_init_val) {
      int offset = 0;
      m_init_val->FillVals(0, offset, m_products, m_flatten_vals);
    } else {
      // temporarily all set to nullptr
      // TODO(garen): insert *tot* random values
      for (auto i = 0; i < tot; ++i) {
        m_flatten_vals.push_back(nullptr);
      }
    }
  }

  if (!symbol_table.Insert(m_varname, shared_from_this()))
    throw MyException("declaration defined multiple times");
}

std::variant<int, float> DeclAST::GetFlattenVal(SymbolTable& symbol_table,
                                                int offset) {
  assert(offset < m_flatten_vals.size());
  auto [type, ret] = m_flatten_vals[offset]->Evaluate(symbol_table);
  assert(type == ExprAST::EvalType::INT || type == ExprAST::EvalType::FLOAT);
  return ret;
}

void FuncCallAST::TypeCheck(SymbolTable& symbol_table) {
  auto ptr = symbol_table.GetFuncDef(m_func_name);
  if (!ptr) throw MyException("FuncCall call on undefined function");

  if (FuncName() != "putf" && ParamsSize() != ptr->ParamsSize())
    throw MyException("incorrect # of params");

  m_func_def = ptr;
  m_return_type = ptr->ReturnType();
  for (auto& param : m_params) {
    param->TypeCheck(symbol_table);
  }
}
void CondAST::TypeCheck(SymbolTable& symbol_table) {
  m_expr->TypeCheck(symbol_table);
}
void FuncFParamAST::TypeCheck(SymbolTable& symbol_table) {
  // TODO(garen): FuncFParamAST::TypeCheck
  // allow m_name shadowing
  m_decl->TypeCheck(symbol_table);
  if (!symbol_table.Insert(m_decl->VarName(), m_decl))
    throw MyException("redefined variable conflict with function argument");
}
void BlockAST::TypeCheck(SymbolTable& symbol_table) {
  symbol_table.InitializeScope(ScopeType::BLOCK);
  for (auto& node : m_nodes) {
    node->TypeCheck(symbol_table);
  }
  symbol_table.FinalizeScope();
}
void FuncDefAST::TypeCheck(SymbolTable& symbol_table) {
  symbol_table.InitializeScope(ScopeType::FUNC);
  if (m_return_type != VarType::VOID && m_return_type != VarType::INT
      && m_return_type != VarType::FLOAT)
    throw MyException("illegal return type of function");

  if (!m_block) throw MyException("no block m_stmt in FuncDef");

  try {
    for (auto& param : m_params) {
      param->TypeCheck(symbol_table);  // also add param to symbol table
    }
    m_block->TypeCheck(symbol_table);
  } catch (MyException& e) {
    symbol_table.FinalizeScope();
    throw e.copy();
  }

  symbol_table.FinalizeScope();
  if (symbol_table.GetFuncDef(m_func_name) == nullptr) {
    if (!symbol_table.Insert(m_func_name, shared_from_this()))
      throw MyException("cannot insert FuncDef after function definition");
  }
}
void AssignStmtAST::TypeCheck(SymbolTable& symbol_table) {
  m_lval->TypeCheck(symbol_table);
  if (m_lval->m_decl->IsConst())
    throw MyException("lval must be a non-const variable");
  m_rhs->TypeCheck(symbol_table);
}
void EvalStmtAST::TypeCheck(SymbolTable& symbol_table) {
  if (!m_expr) throw MyException("evaluate nothing???");
  m_expr->TypeCheck(symbol_table);
}
void IfStmtAST::TypeCheck(SymbolTable& symbol_table) {
  m_cond->TypeCheck(symbol_table);

  // need to initialize a scope only when block m_stmt is meaningful to add a
  // scope, which action is invoked recursively in m_then
  m_then->TypeCheck(symbol_table);
  if (m_else) {
    m_else->TypeCheck(symbol_table);  // same to above
  }
}
void ReturnStmtAST::TypeCheck(SymbolTable& symbol_table) {
  if (!symbol_table.existScope(ScopeType::FUNC))
    throw MyException("return m_stmt appears in non-func scope");
  if (m_ret) m_ret->TypeCheck(symbol_table);
}
void WhileStmtAST::TypeCheck(SymbolTable& symbol_table) {
  symbol_table.InitializeScope(ScopeType::LOOP);
  m_cond->TypeCheck(symbol_table);
  m_stmt->TypeCheck(symbol_table);
  symbol_table.FinalizeScope();
}
void BreakStmtAST::TypeCheck(SymbolTable& symbol_table) {
  if (!symbol_table.existScope(ScopeType::LOOP))
    throw MyException("break m_stmt appears in non-loop scope");
}
void ContinueStmtAST::TypeCheck(SymbolTable& symbol_table) {
  if (!symbol_table.existScope(ScopeType::LOOP))
    throw MyException("continue m_stmt appears in non-loop scope");
}
void CompUnitAST::TypeCheck(SymbolTable& symbol_table) {
  for (const auto& node : m_nodes) {
    if (auto decl = std::dynamic_pointer_cast<DeclAST>(node)) {
      decl->SetIsGlobal(true);
    } else if (auto func_def = std::dynamic_pointer_cast<FuncDefAST>(node)) {
      symbol_table.Insert(func_def->FuncName(), node);
    } else {
      throw MyException("A node in CompUnit is neither DeclAST nor FuncDefAST");
    }
    node->TypeCheck(symbol_table);
  }
}
