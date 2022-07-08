#include <cassert>

#include "ast/ast.h"
#include "ast/symbol-table.h"
#include "exceptions.h"

void InitValAST::TypeCheck(SymbolTable& symbolTable) {
  if (expr) {
    expr->TypeCheck(symbolTable);
    setIsConst(expr->isConst());
  } else {
    bool flag = true;
    for (auto& val : vals) {
      val->TypeCheck(symbolTable);
      if (!val->isConst() && flag) flag = false;
    }
    setIsConst(flag);
  }
}
void LValAST::TypeCheck(SymbolTable& symbolTable) {
  auto ptr = symbolTable.GetDecl(name);
  if (!ptr) throw MyException("undefined lval");
  if (ptr->dimensionsSize() < dimensions.size())
    throw MyException("array dimensions of lval more than real array");

  for (auto& dimension : dimensions) {
    dimension->TypeCheck(symbolTable);
  }
}
std::variant<int, float> LValAST::Evaluate() {
  assert(!isArray());
  if (dimensions.empty()) {
    return decl->Evaluate(0);
  }

  int offset = 0;
  std::vector<int> products(decl->dimensionsSize());

  // calculate array products
  int tot = 1;
  for (auto i = decl->dimensions.size() - 1; i >= 0; --i) {
    // happen after DeclAST::TypeCheck
    auto [type, res] = decl->dimensions[i]->Evaluate();
    assert(type == ExprAST::EvalType::INT);
    tot *= std::get<int>(res);
    products[i] = tot;
  }
  // TODO(garen): potentially buggy code
  for (auto i = 0; i < dimensions.size() - 1; i++) {
    auto [type, res] = dimensions[i]->Evaluate();
    assert(type == ExprAST::EvalType::INT);
    offset += std::get<int>(res) * products[i + 1];
  }
  auto [type, res] = dimensions[dimensions.size() - 1]->Evaluate();
  assert(type == ExprAST::EvalType::INT);
  offset += std::get<int>(res);

  return decl->Evaluate(offset);
}
void ExprAST::TypeCheck(SymbolTable& symbolTable) {
  auto [var_type, res] = Evaluate();
  // INT, FLOAT, BOOL, VAR, anything except ERROR, are legal results
  if (var_type == EvalType::ERROR) throw MyException("unexpected evaluation");
}

std::pair<ExprAST::EvalType, std::variant<int, float>> ExprAST::Evaluate() {
  std::pair<EvalType, std::variant<int, float>> lhs_res, rhs_res;
  switch (op) {
    case Op::POSITIVE:
      return lhs->Evaluate();
      break;
    case Op::NEGATIVE:
      lhs_res = lhs->Evaluate();
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
      lhs_res = lhs->Evaluate();
      rhs_res = rhs->Evaluate();
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
      lhs_res = lhs->Evaluate();
      rhs_res = rhs->Evaluate();
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
      lhs_res = lhs->Evaluate();
      rhs_res = rhs->Evaluate();
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
      lhs_res = lhs->Evaluate();
      rhs_res = rhs->Evaluate();
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
      lhs_res = lhs->Evaluate();
      rhs_res = rhs->Evaluate();
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
      lhs_res = lhs->Evaluate();
      rhs_res = rhs->Evaluate();
      if (lhs_res.first == EvalType::BOOL && rhs_res.first == EvalType::BOOL) {
        return std::make_pair(EvalType::BOOL, 0);
      } else {
        return std::make_pair(EvalType::ERROR, 0);
      }
      break;
    case Op::NOT:
      lhs_res = lhs->Evaluate();
      if (lhs_res.first == EvalType::BOOL)
        return std::make_pair(EvalType::BOOL, 0);
      else
        return std::make_pair(EvalType::ERROR, 0);
      break;
    case Op::CONST_INT:
      return std::make_pair(EvalType::INT, int_val);
      break;
    case Op::CONST_FLOAT:
      return std::make_pair(EvalType::FLOAT, float_val);
      break;
    case Op::LVAL:
      if (lval->decl->isConst()) {
        auto var_type = lval->decl->varType();
        if (var_type == VarType::INT) {
          return std::make_pair(EvalType::INT, std::get<int>(lval->Evaluate()));
        } else if (var_type == VarType::FLOAT) {
          return std::make_pair(EvalType::FLOAT,
                                std::get<float>(lval->Evaluate()));
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
  if (var_type != VarType::INT && var_type != VarType::FLOAT)
    throw MyException("unexpected var_type in DeclAST");

  if (init_val) init_val->TypeCheck(symbolTable);

  if (is_const && !init_val)
    throw MyException("const declaration without init val");
  if (is_const && init_val && !init_val->isConst())
    throw MyException("const declaration with non-const init val");

  if (dimensions.empty()) {  // single
    if (init_val && init_val->expr) {
      flatten_vals.push_back(init_val->expr);
      assert(flatten_vals.size() == 1);
    } else {
      // TODO(garen): insert a random value
      flatten_vals.push_back(nullptr);
    }
  } else {  // array
    int tot = 1;
    std::vector<int> products(dimensions.size());
    for (auto i = dimensions.size() - 1; i >= 0; --i) {
      dimensions[i]->TypeCheck(symbolTable);
      auto [type, res] = dimensions[i]->Evaluate();
      assert(type == ExprAST::EvalType::INT);
      tot *= std::get<int>(res);
      products[i] = tot;
    }
    flatten_vals.resize(tot);
    if (init_val) {
      fillFlattenVals(0, 0, products);
    } else {
      // temporarily all set to nullptr
      // TODO(garen): insert *tot* random values
      for (auto i = 0; i < tot; ++i) {
        flatten_vals.push_back(nullptr);
      }
    }
  }

  if (!symbolTable.Insert(varname, shared_from_this()))
    throw MyException("declaration defined multiple times");
}
// TODO(garen): potentially buggy code
void DeclAST::fillFlattenVals(int n, int offset,
                              const std::vector<int>& sizes) {
  auto temp = offset / sizes[n];
  auto l = temp * sizes[n];
  auto size = (n == sizes.size() - 1 ? 1 : sizes[n + 1]);
  auto r = l + size;
  for (auto& val : init_val->vals) {
    if (val->expr) {  // single
      flatten_vals[offset++] = val->expr;
    } else {  // array
      // temporarily all set to nullptr
      // TODO: assign real values
      while (offset < r) {
        flatten_vals[offset++] = nullptr;
      }
    }
  }
}
std::variant<int, float> DeclAST::Evaluate(int n) {
  assert(n < flatten_vals.size());
  auto [type, ret] = flatten_vals[n]->Evaluate();
  assert(type == ExprAST::EvalType::INT || type == ExprAST::EvalType::FLOAT);
  return ret;
}

void FuncCallAST::TypeCheck(SymbolTable& symbolTable) {
  auto ptr = symbolTable.GetFuncDef(func_name);
  if (!ptr) throw MyException("FuncCall call on undefined function");

  if (paramsSize() != ptr->paramsSize())
    throw MyException("incorrect # of params");

  return_type = ptr->returnType();
  for (auto& param : params) {
    param->TypeCheck(symbolTable);
  }
}
void CondAST::TypeCheck(SymbolTable& symbolTable) {
  expr->TypeCheck(symbolTable);
}
void FuncFParamAST::TypeCheck(SymbolTable& symbolTable) {
  // TODO(garen): FuncFParamAST::TypeCheck
}
void BlockAST::TypeCheck(SymbolTable& symbolTable) {
  symbolTable.InitializeScope(ScopeType::BLOCK);
  for (auto& node : nodes) {
    node->TypeCheck(symbolTable);
  }
  symbolTable.FinalizeScope();
}
void FuncDefAST::TypeCheck(SymbolTable& symbolTable) {
  symbolTable.InitializeScope(ScopeType::FUNC);
  if (return_type != VarType::VOID && return_type != VarType::INT
      && return_type != VarType::FLOAT)
    throw MyException("illegal return type of function");

  if (!block) throw MyException("no block stmt in FuncDef");

  try {
    for (auto& param : params) {
      param->TypeCheck(symbolTable);
    }
    block->TypeCheck(symbolTable);
  } catch (MyException& e) {
    symbolTable.FinalizeScope();
    throw e.copy();
  }

  symbolTable.FinalizeScope();
  if (symbolTable.GetFuncDef(func_name) == nullptr) {
    if (!symbolTable.Insert(func_name, shared_from_this()))
      throw MyException("cannot insert FuncDef after function definition");
  }
}
void AssignStmtAST::TypeCheck(SymbolTable& symbolTable) {
  lval->TypeCheck(symbolTable);
  rhs->TypeCheck(symbolTable);
}
void EvalStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!expr) throw MyException("evaluate nothing???");
  expr->TypeCheck(symbolTable);
}
void IfStmtAST::TypeCheck(SymbolTable& symbolTable) {
  cond->TypeCheck(symbolTable);

  // need to initialize a scope only when block stmt is meaningful to add a
  // scope, which action is invoked recursively in then_stmt
  then_stmt->TypeCheck(symbolTable);
  if (else_stmt) {
    else_stmt->TypeCheck(symbolTable);  // same to above
  }
}
void ReturnStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!symbolTable.existScope(ScopeType::FUNC))
    throw MyException("return stmt appears in non-func scope");
  if (ret) ret->TypeCheck(symbolTable);
}
void WhileStmtAST::TypeCheck(SymbolTable& symbolTable) {
  symbolTable.InitializeScope(ScopeType::LOOP);
  cond->TypeCheck(symbolTable);
  stmt->TypeCheck(symbolTable);
  symbolTable.FinalizeScope();
}
void BreakStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!symbolTable.existScope(ScopeType::LOOP))
    throw MyException("break stmt appears in non-loop scope");
}
void ContinueStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!symbolTable.existScope(ScopeType::LOOP))
    throw MyException("continue stmt appears in non-loop scope");
}
void CompUnitAST::TypeCheck(SymbolTable& symbolTable) {
  for (const auto& node : nodes) {
    if (auto decl = std::dynamic_pointer_cast<DeclAST>(node)) {
      decl->setIsGlobal(true);
    } else if (auto func_def = std::dynamic_pointer_cast<FuncDefAST>(node)) {
      symbolTable.Insert(func_def->funcName(), node);
    } else {
      throw MyException("A node in CompUnit is neither DeclAST nor FuncDefAST");
    }
    node->TypeCheck(symbolTable);
  }
}
