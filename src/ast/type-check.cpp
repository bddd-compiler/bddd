#include <cassert>

#include "ast/ast.h"
#include "ast/symbol-table.h"
#include "exceptions.h"

void InitValAST::TypeCheck(SymbolTable& symbol_table) {
  if (m_expr) {
    m_expr->TypeCheck(symbol_table);
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
void LValAST::TypeCheck(SymbolTable& symbolTable) {
  auto ptr = symbolTable.GetDecl(m_name);
  if (!ptr) throw MyException("undefined lval");
  if (ptr->DimensionsSize() < m_dimensions.size())
    throw MyException("array dimensions of lval more than real array");

  for (auto& dimension : m_dimensions) {
    dimension->TypeCheck(symbolTable);
  }
}
std::variant<int, float> LValAST::Evaluate(SymbolTable& symbol_table) {
  assert(!IsArray());
  assert(m_decl != nullptr);
  if (m_dimensions.empty()) {
    return m_decl->Evaluate(symbol_table, 0);
  }

  int offset = 0;
  std::vector<int> products(m_decl->DimensionsSize());

  // calculate array products
  int tot = 1;
  for (auto i = m_decl->m_dimensions.size() - 1; i >= 0; --i) {
    // happen after DeclAST::TypeCheck
    auto [type, res] = m_decl->m_dimensions[i]->Evaluate(symbol_table);
    assert(type == ExprAST::EvalType::INT);
    tot *= std::get<int>(res);
    products[i] = tot;
  }
  // TODO(garen): potentially buggy code
  size_t i = 0;
  for (i = 0; i < m_dimensions.size() - 1; i++) {
    auto [type, res] = m_dimensions[i]->Evaluate(symbol_table);
    assert(type == ExprAST::EvalType::INT);
    offset += std::get<int>(res) * products[i + 1];
  }
  auto [type, res] = m_dimensions[i]->Evaluate(symbol_table);
  assert(type == ExprAST::EvalType::INT);
  offset += std::get<int>(res);

  return m_decl->Evaluate(symbol_table, offset);
}
void ExprAST::TypeCheck(SymbolTable& symbol_table) {
  auto [var_type, res] = Evaluate(symbol_table);
  // INT, FLOAT, BOOL, VAR, anything except ERROR, are legal results
  if (var_type == EvalType::ERROR) throw MyException("unexpected evaluation");
}

std::pair<ExprAST::EvalType, std::variant<int, float>> ExprAST::Evaluate(
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
      else if (lhs_res.first == EvalType::VAR)
        return std::make_pair(EvalType::VAR, 0);
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
        } else if (rhs_res.first == EvalType::VAR) {
          return std::make_pair(EvalType::VAR, 0);
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
        } else if (rhs_res.first == EvalType::VAR) {
          return std::make_pair(EvalType::VAR, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR) {
        return std::make_pair(EvalType::VAR, 0);
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
        } else if (rhs_res.first == EvalType::VAR) {
          return std::make_pair(EvalType::VAR, 0);
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
        } else if (rhs_res.first == EvalType::VAR) {
          return std::make_pair(EvalType::VAR, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR) {
        return std::make_pair(EvalType::VAR, 0);
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
        } else if (rhs_res.first == EvalType::VAR) {
          return std::make_pair(EvalType::VAR, 0);
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
        } else if (rhs_res.first == EvalType::VAR) {
          return std::make_pair(EvalType::VAR, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR) {
        return std::make_pair(EvalType::VAR, 0);
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
          if (divisor == 0) return std::make_pair(EvalType::ERROR, 0);
          return std::make_pair(EvalType::INT, dividend / divisor);
        } else if (rhs_res.first == EvalType::FLOAT) {
          auto dividend = std::get<int>(lhs_res.second);
          auto divisor = std::get<float>(rhs_res.second);
          if (divisor == 0) return std::make_pair(EvalType::ERROR, 0);
          return std::make_pair(EvalType::FLOAT,
                                static_cast<float>(dividend) / divisor);
        } else if (rhs_res.first == EvalType::VAR) {
          return std::make_pair(EvalType::VAR, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::FLOAT) {
        if (rhs_res.first == EvalType::INT) {
          auto dividend = std::get<float>(lhs_res.second);
          auto divisor = std::get<int>(rhs_res.second);
          if (divisor == 0) return std::make_pair(EvalType::ERROR, 0);
          return std::make_pair(EvalType::FLOAT,
                                dividend / static_cast<float>(divisor));
        } else if (rhs_res.first == EvalType::FLOAT) {
          auto dividend = std::get<float>(lhs_res.second);
          auto divisor = std::get<float>(rhs_res.second);
          if (divisor == 0) return std::make_pair(EvalType::ERROR, 0);
          return std::make_pair(EvalType::FLOAT, dividend / divisor);
        } else if (rhs_res.first == EvalType::VAR) {
          return std::make_pair(EvalType::VAR, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR) {
        return std::make_pair(EvalType::VAR, 0);
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::MOD:
      if (lhs_res.first == EvalType::INT) {
        if (rhs_res.first == EvalType::INT) {
          auto dividend = std::get<int>(lhs_res.second);
          auto divisor = std::get<int>(rhs_res.second);
          if (divisor == 0) return std::make_pair(EvalType::ERROR, 0);
          return std::make_pair(EvalType::INT, dividend % divisor);
        } else if (rhs_res.first == EvalType::VAR) {
          return std::make_pair(EvalType::VAR, 0);
        } else {
          return std::make_pair(EvalType::ERROR, 0);
        }
      } else if (lhs_res.first == EvalType::VAR) {
        if (rhs_res.first == EvalType::INT || rhs_res.first == EvalType::VAR) {
          return std::make_pair(EvalType::VAR, 0);
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
      if ((lhs_res.first == EvalType::INT || lhs_res.first == EvalType::FLOAT
           || lhs_res.first == EvalType::VAR)
          && (rhs_res.first == EvalType::INT || rhs_res.first == EvalType::FLOAT
              || rhs_res.first == EvalType::VAR)) {
        return std::make_pair(EvalType::BOOL, 0);
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::AND:
    case Op::OR:
      lhs_res = m_lhs->Evaluate(symbol_table);
      rhs_res = m_rhs->Evaluate(symbol_table);
      if (lhs_res.first == EvalType::BOOL && rhs_res.first == EvalType::BOOL) {
        return std::make_pair(EvalType::BOOL, 0);
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::NOT:
      lhs_res = m_lhs->Evaluate(symbol_table);
      if (lhs_res.first == EvalType::BOOL)
        return std::make_pair(EvalType::BOOL, 0);
      else
        return std::make_pair(EvalType::ERROR, 0);
      break;
    case Op::CONST_INT:
      return std::make_pair(EvalType::INT, m_int_val);
      break;
    case Op::CONST_FLOAT:
      return std::make_pair(EvalType::FLOAT, m_float_val);
      break;
    case Op::LVAL:
      assert(m_lval->m_decl == nullptr);
      // assign value to lval->decl
      m_lval->m_decl = symbol_table.GetDecl(m_lval->Name());

      assert(m_lval->m_decl != nullptr);
      if (m_lval->m_decl->IsConst()) {
        auto var_type = m_lval->m_decl->GetVarType();
        if (var_type == VarType::INT) {
          return std::make_pair(EvalType::INT,
                                std::get<int>(m_lval->Evaluate(symbol_table)));
        } else if (var_type == VarType::FLOAT) {
          return std::make_pair(
              EvalType::FLOAT, std::get<float>(m_lval->Evaluate(symbol_table)));
        }
      } else {
        return std::make_pair(EvalType::VAR, 0);
      }
      break;
    case Op::FUNC_CALL:
      return std::make_pair(EvalType::VAR, 0);
      break;
    default:
      assert(false);  // unreachable
      break;
  }
  return std::make_pair(EvalType::ERROR, 0);
}
void DeclAST::TypeCheck(SymbolTable& symbolTable) {
  if (m_var_type != VarType::INT && m_var_type != VarType::FLOAT)
    throw MyException("unexpected var_type in DeclAST");

  if (m_init_val) m_init_val->TypeCheck(symbolTable);

  if (m_is_const && !m_init_val)
    throw MyException("const declaration without init val");
  if (m_is_const && m_init_val && !m_init_val->IsConst())
    throw MyException("const declaration with non-const init val");

  if (m_dimensions.empty()) {  // single
    if (m_init_val && m_init_val->m_expr) {
      m_flatten_vals.push_back(m_init_val->m_expr);
      assert(m_flatten_vals.size() == 1);
    } else {
      // TODO(garen): insert a random value
      m_flatten_vals.push_back(nullptr);
    }
  } else {  // array
    int tot = 1;
    std::vector<int> products(m_dimensions.size());
    for (auto i = m_dimensions.size() - 1; i >= 0; --i) {
      m_dimensions[i]->TypeCheck(symbolTable);
      auto [type, res] = m_dimensions[i]->Evaluate(symbolTable);
      assert(type == ExprAST::EvalType::INT);
      tot *= std::get<int>(res);
      products[i] = tot;
    }
    m_flatten_vals.resize(tot);
    if (m_init_val) {
      FillFlattenVals(0, 0, products);
    } else {
      // temporarily all set to nullptr
      // TODO(garen): insert *tot* random values
      for (auto i = 0; i < tot; ++i) {
        m_flatten_vals.push_back(nullptr);
      }
    }
  }

  if (!symbolTable.Insert(m_varname, shared_from_this()))
    throw MyException("declaration defined multiple times");
}
// TODO(garen): potentially buggy code
void DeclAST::FillFlattenVals(int n, int offset,
                              const std::vector<int>& sizes) {
  auto temp = offset / sizes[n];
  auto l = temp * sizes[n];
  auto size = (n == sizes.size() - 1 ? 1 : sizes[n + 1]);
  auto r = l + size;
  for (auto& val : m_init_val->m_vals) {
    if (val->m_expr) {  // single
      m_flatten_vals[offset++] = val->m_expr;
    } else {  // array
      // temporarily all set to nullptr
      // TODO: assign real values
      while (offset < r) {
        m_flatten_vals[offset++] = nullptr;
      }
    }
  }
}
std::variant<int, float> DeclAST::Evaluate(SymbolTable& symbol_table, int n) {
  assert(n < m_flatten_vals.size());
  auto [type, ret] = m_flatten_vals[n]->Evaluate(symbol_table);
  assert(type == ExprAST::EvalType::INT || type == ExprAST::EvalType::FLOAT);
  return ret;
}

void FuncCallAST::TypeCheck(SymbolTable& symbol_table) {
  auto ptr = symbol_table.GetFuncDef(m_func_name);
  if (!ptr) throw MyException("FuncCall call on undefined function");

  if (ParamsSize() != ptr->ParamsSize())
    throw MyException("incorrect # of params");

  m_return_type = ptr->ReturnType();
  for (auto& param : m_params) {
    param->TypeCheck(symbol_table);
  }
}
void CondAST::TypeCheck(SymbolTable& symbol_table) {
  m_expr->TypeCheck(symbol_table);
}
void FuncFParamAST::TypeCheck(SymbolTable& symbolTable) {
  // TODO(garen): FuncFParamAST::TypeCheck
}
void BlockAST::TypeCheck(SymbolTable& symbol_table) {
  symbol_table.InitializeScope(ScopeType::BLOCK);
  for (auto& node : m_nodes) {
    node->TypeCheck(symbol_table);
  }
  symbol_table.FinalizeScope();
}
void FuncDefAST::TypeCheck(SymbolTable& symbolTable) {
  symbolTable.InitializeScope(ScopeType::FUNC);
  if (m_return_type != VarType::VOID && m_return_type != VarType::INT
      && m_return_type != VarType::FLOAT)
    throw MyException("illegal return type of function");

  if (!m_block) throw MyException("no block m_stmt in FuncDef");

  try {
    for (auto& param : m_params) {
      param->TypeCheck(symbolTable);
    }
    m_block->TypeCheck(symbolTable);
  } catch (MyException& e) {
    symbolTable.FinalizeScope();
    throw e.copy();
  }

  symbolTable.FinalizeScope();
  if (symbolTable.GetFuncDef(m_func_name) == nullptr) {
    if (!symbolTable.Insert(m_func_name, shared_from_this()))
      throw MyException("cannot insert FuncDef after function definition");
  }
}
void AssignStmtAST::TypeCheck(SymbolTable& symbolTable) {
  m_lval->TypeCheck(symbolTable);
  m_rhs->TypeCheck(symbolTable);
}
void EvalStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!m_expr) throw MyException("evaluate nothing???");
  m_expr->TypeCheck(symbolTable);
}
void IfStmtAST::TypeCheck(SymbolTable& symbolTable) {
  m_cond->TypeCheck(symbolTable);

  // need to initialize a scope only when block m_stmt is meaningful to add a
  // scope, which action is invoked recursively in m_then
  m_then->TypeCheck(symbolTable);
  if (m_else) {
    m_else->TypeCheck(symbolTable);  // same to above
  }
}
void ReturnStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!symbolTable.existScope(ScopeType::FUNC))
    throw MyException("return m_stmt appears in non-func scope");
  if (m_ret) m_ret->TypeCheck(symbolTable);
}
void WhileStmtAST::TypeCheck(SymbolTable& symbolTable) {
  symbolTable.InitializeScope(ScopeType::LOOP);
  m_cond->TypeCheck(symbolTable);
  m_stmt->TypeCheck(symbolTable);
  symbolTable.FinalizeScope();
}
void BreakStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!symbolTable.existScope(ScopeType::LOOP))
    throw MyException("break m_stmt appears in non-loop scope");
}
void ContinueStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!symbolTable.existScope(ScopeType::LOOP))
    throw MyException("continue m_stmt appears in non-loop scope");
}
void CompUnitAST::TypeCheck(SymbolTable& symbolTable) {
  for (const auto& node : m_nodes) {
    if (auto decl = std::dynamic_pointer_cast<DeclAST>(node)) {
      decl->SetIsGlobal(true);
    } else if (auto func_def = std::dynamic_pointer_cast<FuncDefAST>(node)) {
      symbolTable.Insert(func_def->FuncName(), node);
    } else {
      throw MyException("A node in CompUnit is neither DeclAST nor FuncDefAST");
    }
    node->TypeCheck(symbolTable);
  }
}
