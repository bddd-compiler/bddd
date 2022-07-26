#include <cassert>
#include <iostream>

#include "ast/ast.h"
#include "ast/symbol-table.h"
#include "ast/type.h"
#include "exceptions.h"

void InitValAST::TypeCheck(SymbolTable& symbol_table) {
  if (m_expr) {
    m_expr->TypeCheck(symbol_table);  // m_expr->m_is_const is updated here
    SetIsConst(m_expr->IsConst());
    auto eval_value = m_expr->Evaluate(symbol_table);
    if (eval_value.IsConstInt()) {
      SetAllZero(eval_value.IntVal() == 0);
    } else if (eval_value.IsConstFloat()) {
      SetAllZero(eval_value.FloatVal() == 0);
    }
  } else {
    bool is_const = true;
    bool all_zero = true;
    for (auto& val : m_vals) {
      val->TypeCheck(symbol_table);
      if (is_const && !val->IsConst()) is_const = false;
      if (all_zero && !val->AllZero()) all_zero = false;
    }
    SetIsConst(is_const);
    SetAllZero(all_zero);
  }
}
void InitValAST::FillVals(int n, int& offset, const std::vector<int>& sizes,
                          std::vector<std::shared_ptr<ExprAST>>& vals) {
  auto temp = offset / sizes[n];
  auto l = temp * sizes[n];
  auto r = l + sizes[n];
  auto size = (n == sizes.size() - 1 ? 1 : sizes[n + 1]);

  // assign values
  // TODO(garen): WARNING!!! nullptr represents zero, dangerous behaviour
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
  if (m_decl->DimensionsSize() < m_indices.size())
    throw MyException("array m_indices of lval more than real array");

  for (auto& index : m_indices) {
    index->TypeCheck(symbol_table);
  }
}
EvalValue LValAST::Evaluate(SymbolTable& symbol_table) {
  assert(!IsArray());
  assert(m_decl != nullptr);
  if (m_indices.empty()) {
    return m_decl->GetFlattenVal(symbol_table, 0);
  }

  int offset = 0;

  assert(m_decl->m_products.size() == m_decl->m_dimensions.size());
  size_t i;
  bool flag = true;
  for (i = 0; i < m_indices.size() - 1; i++) {
    auto eval_value = m_indices[i]->Evaluate(symbol_table);
    assert(eval_value.IsInt());
    if (!eval_value.IsConstInt()) {
      flag = false;
    } else {
      offset += eval_value.IntVal() * m_decl->m_products[i + 1];
    }
  }
  auto eval_value = m_indices[i]->Evaluate(symbol_table);
  assert(eval_value.IsInt());
  if (!eval_value.IsConstInt()) {
    flag = false;
  } else {
    offset += eval_value.IntVal();
  }
  if (flag)
    return m_decl->GetFlattenVal(symbol_table, offset);
  else {
    switch (m_decl->GetVarType()) {
      case VarType::INT:
        return EvalValue(EvalType::VAR_INT);
      case VarType::FLOAT:
        return EvalValue(EvalType::VAR_FLOAT);
      default:
        assert(false);
    }
  }
}
void ExprAST::TypeCheck(SymbolTable& symbol_table) {
  auto eval_value = Evaluate(symbol_table);
  // CONST_INT, CONST_FLOAT, VAR_INT, VAR_FLOAT, anything except ERROR, are
  // legal results
  switch (eval_value.m_eval_type) {
    case EvalType::CONST_INT:
    case EvalType::CONST_FLOAT:
      SetIsConst(true);
      break;
    case EvalType::VAR_INT:
    case EvalType::VAR_FLOAT:
      SetIsConst(false);
      break;
    case EvalType::ERROR:
      throw MyException("unexpected evaluation");
    default:
      break;  // do nothing
  }
}

// if a is int variable, if (+--!!!a) {} is legal expression
// there is no distinction between int and bool
// array may be evaluated, which cannot join in any computation
EvalValue ExprAST::Evaluate(SymbolTable& symbol_table) {
  auto ret = EvaluateInner(symbol_table);
  // memorize the answer
  if (ret.IsConstInt()) {
    m_op = Op::CONST_INT;
    m_int_val = ret.IntVal();
  } else if (ret.IsConstFloat()) {
    m_op = Op::CONST_FLOAT;
    m_float_val = ret.FloatVal();
  }
  return ret;
}

EvalValue ExprAST::EvaluateInner(SymbolTable& symbol_table) {
  EvalValue lhs_res, rhs_res;
  switch (m_op) {
    case Op::POSITIVE:
      return m_lhs->Evaluate(symbol_table);
    case Op::NEGATIVE:
      lhs_res = m_lhs->Evaluate(symbol_table);
      switch (lhs_res.m_eval_type) {
        case EvalType::CONST_INT:
          return EvalValue(-lhs_res.IntVal());
        case EvalType::CONST_FLOAT:
          return EvalValue(-lhs_res.FloatVal());
        case EvalType::VAR_INT:
          return EvalValue(EvalType::VAR_INT);
        case EvalType::VAR_FLOAT:
          return EvalValue(EvalType::VAR_FLOAT);
        default:
          return EvalValue(EvalType::ERROR);
      }
    case Op::PLUS:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      switch (lhs_res.m_eval_type) {
        case EvalType::CONST_INT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
              return EvalValue(lhs_res.IntVal() + rhs_res.IntVal());
            case EvalType::CONST_FLOAT:
              return EvalValue(lhs_res.IntVal() + rhs_res.FloatVal());
            case EvalType::VAR_INT:
              return EvalValue(EvalType::VAR_INT);
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::CONST_FLOAT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
              // rhs_res int->float
              m_rhs->SetFloatVal(rhs_res.IntVal());
              return EvalValue(lhs_res.FloatVal() + rhs_res.IntVal());
            case EvalType::CONST_FLOAT:
              return EvalValue(lhs_res.FloatVal() + rhs_res.FloatVal());
            case EvalType::VAR_INT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::VAR_INT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
            case EvalType::VAR_INT:
              return EvalValue(EvalType::VAR_INT);
            case EvalType::CONST_FLOAT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::VAR_FLOAT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
            case EvalType::VAR_INT:
            case EvalType::CONST_FLOAT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        default:
          return EvalValue(EvalType::ERROR);
      }
    case Op::MINUS:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      switch (lhs_res.m_eval_type) {
        case EvalType::CONST_INT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
              return EvalValue(lhs_res.IntVal() - rhs_res.IntVal());
            case EvalType::CONST_FLOAT:
              m_lhs->SetFloatVal(lhs_res.IntVal());
              return EvalValue(lhs_res.IntVal() - rhs_res.FloatVal());
            case EvalType::VAR_INT:
              return EvalValue(EvalType::VAR_INT);
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::CONST_FLOAT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
              m_rhs->SetFloatVal(rhs_res.IntVal());
              return EvalValue(lhs_res.FloatVal() - rhs_res.IntVal());
            case EvalType::CONST_FLOAT:
              return EvalValue(lhs_res.FloatVal() - rhs_res.FloatVal());
            case EvalType::VAR_INT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::VAR_INT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
            case EvalType::VAR_INT:
              return EvalValue(EvalType::VAR_INT);
            case EvalType::CONST_FLOAT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::VAR_FLOAT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
            case EvalType::VAR_INT:
            case EvalType::CONST_FLOAT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        default:
          return EvalValue(EvalType::ERROR);
      }
    case Op::MULTI:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      switch (lhs_res.m_eval_type) {
        case EvalType::CONST_INT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
              return EvalValue(lhs_res.IntVal() * rhs_res.IntVal());
            case EvalType::CONST_FLOAT:
              m_lhs->SetFloatVal(lhs_res.IntVal());
              return EvalValue(lhs_res.IntVal() * rhs_res.FloatVal());
            case EvalType::VAR_INT:
              return EvalValue(EvalType::VAR_INT);
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::CONST_FLOAT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
              m_rhs->SetFloatVal(rhs_res.IntVal());
              return EvalValue(lhs_res.FloatVal() * rhs_res.IntVal());
            case EvalType::CONST_FLOAT:
              return EvalValue(lhs_res.FloatVal() * rhs_res.FloatVal());
            case EvalType::VAR_INT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::VAR_INT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
            case EvalType::VAR_INT:
              return EvalValue(EvalType::VAR_INT);
            case EvalType::CONST_FLOAT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::VAR_FLOAT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
            case EvalType::VAR_INT:
            case EvalType::CONST_FLOAT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        default:
          return EvalValue(EvalType::ERROR);
      }
    case Op::DIV:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      switch (lhs_res.m_eval_type) {
        case EvalType::CONST_INT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
              if (rhs_res.IntVal() == 0) return EvalValue(EvalType::ERROR);
              return EvalValue(lhs_res.IntVal() / rhs_res.IntVal());
            case EvalType::CONST_FLOAT:
              if (rhs_res.FloatVal() == 0) return EvalValue(EvalType::ERROR);
              m_lhs->SetFloatVal(lhs_res.IntVal());
              return EvalValue(lhs_res.IntVal() / rhs_res.FloatVal());
            case EvalType::VAR_INT:
              return EvalValue(EvalType::VAR_INT);
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::CONST_FLOAT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
              if (rhs_res.IntVal() == 0) return EvalValue(EvalType::ERROR);
              m_rhs->SetFloatVal(rhs_res.IntVal());
              return EvalValue(lhs_res.FloatVal() / rhs_res.IntVal());
            case EvalType::CONST_FLOAT:
              if (rhs_res.FloatVal() == 0) return EvalValue(EvalType::ERROR);
              return EvalValue(lhs_res.FloatVal() / rhs_res.FloatVal());
            case EvalType::VAR_INT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::VAR_INT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
            case EvalType::VAR_INT:
              return EvalValue(EvalType::VAR_INT);
            case EvalType::CONST_FLOAT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::VAR_FLOAT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
            case EvalType::VAR_INT:
            case EvalType::CONST_FLOAT:
            case EvalType::VAR_FLOAT:
              return EvalValue(EvalType::VAR_FLOAT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        default:
          return EvalValue(EvalType::ERROR);
      }
    case Op::MOD:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      switch (lhs_res.m_eval_type) {
        case EvalType::CONST_INT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
              if (rhs_res.IntVal() == 0) return EvalValue(EvalType::ERROR);
              return EvalValue(lhs_res.IntVal() % rhs_res.IntVal());
            case EvalType::VAR_INT:
              return EvalValue(EvalType::VAR_INT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        case EvalType::VAR_INT:
          switch (rhs_res.m_eval_type) {
            case EvalType::CONST_INT:
            case EvalType::VAR_INT:
              return EvalValue(EvalType::VAR_INT);
            default:
              return EvalValue(EvalType::ERROR);
          }
        default:
          return EvalValue(EvalType::ERROR);
      }
    case Op::LE:
    case Op::LEQ:
    case Op::GE:
    case Op::GEQ:
    case Op::EQ:
    case Op::NEQ:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      if (lhs_res.IsConst() && rhs_res.IsConst()) {
        // lhs and rhs are constant, then the result is also constant
        float lhs_num, rhs_num;
        if (lhs_res.IsConstInt())
          lhs_num = static_cast<float>(lhs_res.IntVal());
        else
          lhs_num = lhs_res.FloatVal();
        if (rhs_res.IsConstInt())
          rhs_num = static_cast<float>(rhs_res.IntVal());
        else
          rhs_num = rhs_res.FloatVal();

        switch (m_op) {
          case Op::LE:
            return EvalValue(lhs_num < rhs_num);
          case Op::LEQ:
            return EvalValue(lhs_num <= rhs_num);
          case Op::GE:
            return EvalValue(lhs_num > rhs_num);
          case Op::GEQ:
            return EvalValue(lhs_num >= rhs_num);
          case Op::EQ:
            return EvalValue(lhs_num == rhs_num);
          case Op::NEQ:
            return EvalValue(lhs_num != rhs_num);
          default:
            assert(false);  // impossible
        }
      } else if (lhs_res.IsSingle() && rhs_res.IsSingle()) {
        // either one of them is not constant, then may be 1 or 0
        if (!lhs_res.IsInt() || !rhs_res.IsInt()) {
          if (lhs_res.IsConstInt()) m_lhs->SetFloatVal(lhs_res.IntVal());
          if (rhs_res.IsConstInt()) m_rhs->SetFloatVal(rhs_res.IntVal());
        }
        return EvalValue(EvalType::VAR_INT);
      } else {
        return EvalValue(EvalType::ERROR);
      }
    case Op::AND:
    case Op::OR:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      if (lhs_res.IsConst() && rhs_res.IsConst()) {
        // either 1 or 0
        float lhs_num, rhs_num;
        if (lhs_res.IsConstInt())
          lhs_num = static_cast<float>(lhs_res.IntVal());
        else
          lhs_num = lhs_res.FloatVal();
        if (rhs_res.IsConstInt())
          rhs_num = static_cast<float>(rhs_res.IntVal());
        else
          rhs_num = rhs_res.FloatVal();

        if (m_op == Op::AND)
          return EvalValue(lhs_num && rhs_num);
        else
          return EvalValue(lhs_num || rhs_num);
      } else if (lhs_res.IsSingle() && rhs_res.IsSingle()) {
        return EvalValue(EvalType::VAR_INT);
      } else {
        return EvalValue(EvalType::ERROR);
      }
    case Op::NOT:
      lhs_res = m_lhs->Evaluate(symbol_table);
      switch (lhs_res.m_eval_type) {
        case EvalType::CONST_INT:
          return EvalValue(!lhs_res.IntVal());
        case EvalType::CONST_FLOAT:
          return EvalValue(!lhs_res.FloatVal());
        case EvalType::VAR_INT:
        case EvalType::VAR_FLOAT:
          return EvalValue(EvalType::VAR_INT);
        default:
          return EvalValue(EvalType::ERROR);
      }
    case Op::CONST_INT:
      return EvalValue(m_int_val);
    case Op::CONST_FLOAT:
      return EvalValue(m_float_val);
    case Op::LVAL:
      m_lval->TypeCheck(symbol_table);
      // if (m_lval->m_decl == nullptr) {
      //   // assign value to lval->decl
      //   m_lval->m_decl = symbol_table.GetDecl(m_lval->Name());
      // }

      assert(m_lval->m_decl != nullptr);

      // can we handle lval array?
      // yes, just give a new enum variant ARR
      if (m_lval->IsArray()) return EvalValue(EvalType::ARR);

      // must be a single value
      if (m_lval->m_decl->IsConst()) {
        // although decl is const, indices may be variable, which leads to a
        // non-const result
        switch (m_lval->m_decl->GetVarType()) {
          case VarType::INT:
            // return EvalValue(m_lval->Evaluate(symbol_table).IntVal());
          case VarType::FLOAT:
            // return EvalValue(m_lval->Evaluate(symbol_table).FloatVal());
            return m_lval->Evaluate(symbol_table);
          default:
            return EvalValue(EvalType::ERROR);
        }
      } else {
        switch (m_lval->m_decl->GetVarType()) {
          case VarType::INT:
            return EvalValue(EvalType::VAR_INT);
          case VarType::FLOAT:
            return EvalValue(EvalType::VAR_FLOAT);
          default:
            return EvalValue(EvalType::ERROR);
        }
      }
    case Op::FUNC_CALL:
      m_func_call->TypeCheck(symbol_table);

      switch (m_func_call->ReturnType()) {
        case VarType::INT:
          return EvalValue(EvalType::VAR_INT);
        case VarType::FLOAT:
          return EvalValue(EvalType::VAR_FLOAT);
        case VarType::VOID:
          return EvalValue(EvalType::VOID);
        default:
          return EvalValue(EvalType::ERROR);
      }
    default:
      assert(false);  // unreachable
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
      if (m_dimensions[0] != nullptr) {
        throw MyException("the first dimension of param array is not empty?!");
      }
      m_products.clear();
      m_products.push_back(-1);
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

  assert(m_flatten_vals.empty());
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
      auto eval_value = m_dimensions[i]->Evaluate(symbol_table);
      assert(eval_value.IsConstInt());
      tot *= eval_value.IntVal();
      m_products[i] = tot;
    }
    assert(tot > 0);
    m_flatten_vals.resize(tot);
    if (m_init_val) {
      int offset = 0;
      m_init_val->FillVals(0, offset, m_products, m_flatten_vals);
    } else {
      // temporarily all set to nullptr
      // TODO(garen): insert *tot* random values
    }
  }

  if (!symbol_table.Insert(m_varname, shared_from_this()))
    throw MyException("declaration defined multiple times");
}

EvalValue DeclAST::GetFlattenVal(SymbolTable& symbol_table, int offset) {
  assert(offset < m_flatten_vals.size());
  auto eval_value = m_flatten_vals[offset]->Evaluate(symbol_table);
  assert(eval_value.IsConst());
  return eval_value;
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

  m_expected_type = symbol_table.GetCurrentFunc()->ReturnType();
  if (m_expected_type == VarType::VOID && m_ret != nullptr)
    throw MyException("void function returns non-void value??");
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
