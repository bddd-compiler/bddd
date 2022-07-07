#include "ast/ast.h"

#include <iostream>

// static std::map<Op, std::string> op_map {
//     {Op::POSITIVE, "+"},
//     {Op::NEGATIVE, "-"},
//     {Op::PLUS, "+"},
//     {Op::MINUS, "-"},
//     {Op::MULTI, "*"},
//     {Op::DIV, "/"},
//     {Op::MOD, "%"},
//     {Op::LE, "<"},
//     {Op::LEQ, "<="},
//     {Op::GE, ">"},
//     {Op::GEQ, ">="},
//     {Op::EQ, "=="},
//     {Op::NEQ, "!="},
//     {Op::AND, "&&"},
//     {Op::OR, "||"},
//     {Op::NOT, "!"}
// };

void LValAST::AddDimension(int x) {
  dimensions.push_back(std::make_unique<ExprAST>(x));
}

void LValAST::AddDimension(std::unique_ptr<ExprAST> expr) {
  dimensions.push_back(std::move(expr));
}
void DeclAST::AddDimension(std::unique_ptr<ExprAST> expr) {
  dimensions.push_back(std::move(expr));
}
void DeclAST::setIsConst(bool isConst) { is_const = isConst; }
void DeclAST::setVarType(VarType varType) { var_type = varType; }
void DeclAST::setInitVal(std::unique_ptr<InitValAST> initVal) {
  init_val = std::move(initVal);
}
void FuncFParamAST::AddDimension(int x) {
  dimensions.push_back(std::make_unique<ExprAST>(x));
}
void FuncFParamAST::AddDimension(std::unique_ptr<ExprAST> expr) {
  dimensions.push_back(std::move(expr));
}
void BlockAST::AppendNodes(std::vector<std::unique_ptr<AST>> appendedNodes) {
  nodes.insert(nodes.end(), std::make_move_iterator(appendedNodes.begin()),
               std::make_move_iterator(appendedNodes.end()));
  appendedNodes.clear();
}

void CompUnitAST::AppendDecls(std::vector<std::unique_ptr<DeclAST>> decls) {
  nodes.insert(nodes.end(), std::make_move_iterator(decls.begin()),
               std::make_move_iterator(decls.end()));
  decls.clear();
}
void CompUnitAST::AppendFuncDef(std::unique_ptr<FuncDefAST> funcDef) {
  nodes.push_back(std::move(funcDef));
}
// debug
void InitValAST::Debug(std::ofstream& ofs, int depth) {
  if (expr) {
    ofs << std::string(depth * 2, ' ') << "InitValAST (single):" << std::endl;
    expr->Debug(ofs, depth + 1);
  } else {
    ofs << std::string(depth * 2, ' ') << "InitValAST (array): {" << std::endl;
    for (auto& val : vals) {
      val->Debug(ofs, depth + 1);
    }
    ofs << std::string(depth * 2, ' ') << "}" << std::endl;
  }
}
void LValAST::Debug(std::ofstream& ofs, int depth) {
  if (dimensions.empty()) {
    ofs << std::string(depth * 2, ' ') << "LValAST (single): " << name
        << std::endl;
  } else {
    ofs << std::string(depth * 2, ' ') << "LValAST (array): " << name
        << std::endl;
    for (auto& dimension : dimensions) {
      dimension->Debug(ofs, depth + 1);
    }
  }
}
void ExprAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "ExprAST ";
  switch (op) {
    case Op::POSITIVE:
      ofs << "positive" << std::endl;
      lhs->Debug(ofs, depth + 1);
      break;
    case Op::NEGATIVE:
      ofs << "negative" << std::endl;
      lhs->Debug(ofs, depth + 1);
      break;
    case Op::PLUS:
      ofs << "plus" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::MINUS:
      ofs << "minus" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::MULTI:
      ofs << "multi" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::DIV:
      ofs << "div" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::MOD:
      ofs << "mod" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::LE:
      ofs << "le" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::LEQ:
      ofs << "leq" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::GE:
      ofs << "ge" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::GEQ:
      ofs << "geq" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::EQ:
      ofs << "eq" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::NEQ:
      ofs << "neq" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::AND:
      ofs << "and" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::OR:
      ofs << "or" << std::endl;
      lhs->Debug(ofs, depth + 1);
      rhs->Debug(ofs, depth + 1);
      break;
    case Op::NOT:
      ofs << "not" << std::endl;
      lhs->Debug(ofs, depth + 1);
      break;
    case Op::CONST_INT:
      ofs << "const int: " << int_val << std::endl;
      break;
    case Op::CONST_FLOAT:
      ofs << "const float: " << float_val << std::endl;
      break;
    case Op::LVAL:
      ofs << "lval: " << std::endl;
      lval->Debug(ofs, depth + 1);
      break;
    case Op::FUNC_CALL:
      ofs << "func call: " << std::endl;
      func_call->Debug(ofs, depth + 1);
      break;
    default:
      std::cerr << "unimplemented" << std::endl;
      exit(-1);
  }
}
void DeclAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "DeclAST";
  if (is_const) {
    ofs << " (const)";
  } else {
    ofs << " (var)";
  }
  switch (var_type) {
    case VarType::INT:
      ofs << " (int) " << varname << ":" << std::endl;
      init_val->Debug(ofs, depth + 1);
      break;
    case VarType::FLOAT:
      ofs << " (float) " << varname << ":" << std::endl;
      init_val->Debug(ofs, depth + 1);
      break;
    default:
      std::cerr << "unexpected var_type in DeclAST" << std::endl;
      exit(-1);
  }
}
void FuncCallAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "FuncCallAST " << func_name << " ("
      << std::endl;
  for (auto& param : params) {
    param->Debug(ofs, depth + 1);
  }
  ofs << ")" << std::endl;
}
void CondAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "CondAST" << std::endl;
  expr->Debug(ofs, depth + 1);
}
void FuncFParamAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "FuncFParamAST";
  switch (type) {
    case VarType::INT:
      ofs << " (int)";
      break;
    case VarType::FLOAT:
      ofs << " (float)";
      break;
    default:
      std::cerr << "unexpected var_type in FuncFParamAST" << std::endl;
      exit(-1);
  }
  ofs << " " << name;
  if (!dimensions.empty()) {
    ofs << " (array):" << std::endl;
    for (auto& dimension : dimensions) {
      dimension->Debug(ofs, depth + 1);
    }
  } else {
    ofs << " (single)" << std::endl;
  }
}
void BlockAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "BlockAST:" << std::endl;
  for (auto& node : nodes) {
    node->Debug(ofs, depth + 1);
  }
}
void FuncDefAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "FuncDefAST";
  switch (return_type) {
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
  ofs << " " << func_name << ":" << std::endl;
  ofs << std::string(depth * 2, ' ') << "params:" << std::endl;
  for (auto& param : params) {
    param->Debug(ofs, depth + 1);
  }
  ofs << std::string(depth * 2, ' ') << "block:" << std::endl;
  block->Debug(ofs, depth + 1);
}
void AssignStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "AssignStmtAST" << std::endl;
  ofs << std::string(depth * 2, ' ') << "lval:" << std::endl;
  lval->Debug(ofs, depth + 1);
  ofs << std::string(depth * 2, ' ') << "rhs:" << std::endl;
  rhs->Debug(ofs, depth + 1);
}
void EvalStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "EvalStmtAST:" << std::endl;
  expr->Debug(ofs, depth + 1);
}
void IfStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "IfStmtAST:" << std::endl;
  ofs << std::string(depth * 2, ' ') << "cond:" << std::endl;
  cond->Debug(ofs, depth + 1);
  ofs << std::string(depth * 2, ' ') << "then:" << std::endl;
  then_stmt->Debug(ofs, depth + 1);
  if (else_stmt) {
    ofs << std::string(depth * 2, ' ') << "else:" << std::endl;
    else_stmt->Debug(ofs, depth + 1);
  }
}
void ReturnStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "ReturnStmtAST:" << std::endl;
  ret->Debug(ofs, depth + 1);
}
void WhileStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "WhileStmtAST:" << std::endl;
  ofs << std::string(depth * 2, ' ') << "cond:" << std::endl;
  cond->Debug(ofs, depth + 1);
  ofs << std::string(depth * 2, ' ') << "stmt:" << std::endl;
  stmt->Debug(ofs, depth + 1);
}
void BreakStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "BreakStmtAST" << std::endl;
}
void ContinueStmtAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "ContinueStmtAST" << std::endl;
}
void CompUnitAST::Debug(std::ofstream& ofs, int depth) {
  ofs << std::string(depth * 2, ' ') << "CompUnitAST:" << std::endl;
  for (auto& node : nodes) {
    node->Debug(ofs, depth + 1);
  }
}
