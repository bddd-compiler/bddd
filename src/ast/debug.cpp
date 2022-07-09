#include <iostream>

#include "ast/ast.h"

void InitValAST::Debug(std::ofstream& ofs, int depth) {
  if (m_expr) {
    ofs << std::string(depth * 2, ' ') << "InitValAST (single):" << std::endl;
    m_expr->Debug(ofs, depth + 1);
  } else {
    ofs << std::string(depth * 2, ' ') << "InitValAST (array): {" << std::endl;
    for (auto& val : m_vals) {
      val->Debug(ofs, depth + 1);
    }
    ofs << std::string(depth * 2, ' ') << "}" << std::endl;
  }
}
void LValAST::Debug(std::ofstream& ofs, int depth) {
  if (m_dimensions.empty()) {
    ofs << std::string(depth * 2, ' ') << "LValAST (single): " << m_name
        << std::endl;
  } else {
    ofs << std::string(depth * 2, ' ') << "LValAST (array): " << m_name
        << std::endl;
    for (auto& dimension : m_dimensions) {
      dimension->Debug(ofs, depth + 1);
    }
  }
}
void ExprAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "ExprAST ";
  switch (m_op) {
    case Op::POSITIVE:
      ofs << "positive" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      break;
    case Op::NEGATIVE:
      ofs << "negative" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      break;
    case Op::PLUS:
      ofs << "plus" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::MINUS:
      ofs << "minus" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::MULTI:
      ofs << "multi" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::DIV:
      ofs << "div" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::MOD:
      ofs << "mod" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::LE:
      ofs << "le" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::LEQ:
      ofs << "leq" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::GE:
      ofs << "ge" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::GEQ:
      ofs << "geq" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::EQ:
      ofs << "eq" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::NEQ:
      ofs << "neq" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::AND:
      ofs << "and" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::OR:
      ofs << "or" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      m_rhs->Debug(ofs, depth + 1);
      break;
    case Op::NOT:
      ofs << "not" << std::endl;
      m_lhs->Debug(ofs, depth + 1);
      break;
    case Op::CONST_INT:
      ofs << "const int: " << m_int_val << std::endl;
      break;
    case Op::CONST_FLOAT:
      ofs << "const float: " << m_float_val << std::endl;
      break;
    case Op::LVAL:
      ofs << "lval: " << std::endl;
      m_lval->Debug(ofs, depth + 1);
      break;
    case Op::FUNC_CALL:
      ofs << "func call: " << std::endl;
      m_func_call->Debug(ofs, depth + 1);
      break;
    default:
      std::cerr << "unimplemented" << std::endl;
      exit(-1);
  }
}
void DeclAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "DeclAST";
  if (m_is_const) {
    ofs << " (const)";
  } else {
    ofs << " (var)";
  }
  switch (m_var_type) {
    case VarType::INT:
      ofs << " (int)";
      break;
    case VarType::FLOAT:
      ofs << " (float)";
      break;
    default:
      std::cerr << "unexpected var_type in DeclAST" << std::endl;
      exit(-1);
  }
  ofs << " " << m_varname;
  if (!m_dimensions.empty()) {
    ofs << " (array):" << std::endl;
    for (auto& dimension : m_dimensions) {
      dimension->Debug(ofs, depth + 1);
    }
  } else {
    ofs << " (single):" << std::endl;
  }
  if (m_init_val) {
    ofs << std::string(depth * 2, ' ') << "with init_val:" << std::endl;
    m_init_val->Debug(ofs, depth + 1);
  }
}
void FuncCallAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "FuncCallAST " << m_func_name << " ("
      << std::endl;
  for (auto& param : m_params) {
    param->Debug(ofs, depth + 1);
  }
  ofs << ")" << std::endl;
}
void CondAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "CondAST" << std::endl;
  m_expr->Debug(ofs, depth + 1);
}
void FuncFParamAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "FuncFParamAST (special DeclAST):";
  decl->Debug(ofs, depth);
}
void BlockAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "BlockAST:" << std::endl;
  for (auto& node : m_nodes) {
    node->Debug(ofs, depth + 1);
  }
}
void FuncDefAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "FuncDefAST";
  switch (m_return_type) {
    case VarType::INT:
      ofs << " (int)";
      break;
    case VarType::FLOAT:
      ofs << " (float)";
      break;
    case VarType::VOID:
      ofs << " (void)";
      break;
    default:
      std::cerr << "unexpected var_type in FuncDefAST" << std::endl;
      exit(-1);
  }
  ofs << " " << m_func_name << ":" << std::endl;
  ofs << std::string(depth * 2, ' ') << "params:" << std::endl;
  for (auto& param : m_params) {
    param->Debug(ofs, depth + 1);
  }
  ofs << std::string(depth * 2, ' ') << "block:" << std::endl;
  m_block->Debug(ofs, depth + 1);
}
void AssignStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "AssignStmtAST" << std::endl;
  ofs << std::string(depth * 2, ' ') << "lval:" << std::endl;
  m_lval->Debug(ofs, depth + 1);
  ofs << std::string(depth * 2, ' ') << "rhs:" << std::endl;
  m_rhs->Debug(ofs, depth + 1);
}
void EvalStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "EvalStmtAST:" << std::endl;
  m_expr->Debug(ofs, depth + 1);
}
void IfStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "IfStmtAST:" << std::endl;
  ofs << std::string(depth * 2, ' ') << "m_cond:" << std::endl;
  m_cond->Debug(ofs, depth + 1);
  ofs << std::string(depth * 2, ' ') << "then:" << std::endl;
  m_then->Debug(ofs, depth + 1);
  if (m_else) {
    ofs << std::string(depth * 2, ' ') << "else:" << std::endl;
    m_else->Debug(ofs, depth + 1);
  }
}
void ReturnStmtAST::Debug(std::ofstream& ofs, int depth) {
  if (m_ret) {
    ofs << std::string(depth * 2, ' ') << "ReturnStmtAST:" << std::endl;
    m_ret->Debug(ofs, depth + 1);
  } else {
    ofs << std::string(depth * 2, ' ') << "ReturnStmtAST: null" << std::endl;
  }
}
void WhileStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "WhileStmtAST:" << std::endl;
  ofs << std::string(depth * 2, ' ') << "m_cond:" << std::endl;
  m_cond->Debug(ofs, depth + 1);
  ofs << std::string(depth * 2, ' ') << "m_stmt:" << std::endl;
  m_stmt->Debug(ofs, depth + 1);
}
void BreakStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "BreakStmtAST" << std::endl;
}
void ContinueStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "ContinueStmtAST" << std::endl;
}
void CompUnitAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "CompUnitAST:" << std::endl;
  for (auto& node : m_nodes) {
    node->Debug(ofs, depth + 1);
  }
}
