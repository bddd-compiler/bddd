#include "ast/ast.h"

#include <cassert>

#include "exceptions.h"

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
  m_dimensions.push_back(std::make_unique<ExprAST>(x));
}

void LValAST::AddDimension(std::unique_ptr<ExprAST> expr) {
  m_dimensions.push_back(std::move(expr));
}

bool LValAST::IsSingle() {
  return m_decl->DimensionsSize() == m_dimensions.size();
}

bool LValAST::IsArray() {
  return m_decl->DimensionsSize() > m_dimensions.size();
}

void DeclAST::AddDimension(int x) {
  m_dimensions.push_back(std::make_unique<ExprAST>(x));
}
void DeclAST::AddDimension(std::unique_ptr<ExprAST> expr) {
  m_dimensions.push_back(std::move(expr));
}
void FuncFParamAST::AddDimension(int x) { m_decl->AddDimension(x); }
void FuncFParamAST::AddDimension(std::unique_ptr<ExprAST> expr) {
  m_decl->AddDimension(std::move(expr));
}
void BlockAST::AppendNodes(std::vector<std::unique_ptr<AST>> nodes) {
  m_nodes.insert(m_nodes.end(), std::make_move_iterator(nodes.begin()),
                 std::make_move_iterator(nodes.end()));
  nodes.clear();
}
void FuncDefAST::AssignParams(
    std::vector<std::unique_ptr<FuncFParamAST>> params) {
  m_params.assign(std::make_move_iterator(params.begin()),
                  std::make_move_iterator(params.end()));
  params.clear();
}
void FuncCallAST::AssignParams(std::vector<std::unique_ptr<ExprAST>> params) {
  m_params.assign(std::make_move_iterator(params.begin()),
                  std::make_move_iterator(params.end()));
  params.clear();
}
void CompUnitAST::AppendDecls(std::vector<std::unique_ptr<DeclAST>> decls) {
  m_nodes.insert(m_nodes.end(), std::make_move_iterator(decls.begin()),
                 std::make_move_iterator(decls.end()));
  decls.clear();
}
void CompUnitAST::AppendFuncDef(std::unique_ptr<FuncDefAST> funcDef) {
  m_nodes.push_back(std::move(funcDef));
}

std::vector<std::shared_ptr<FuncDefAST>> g_builtin_funcs;  // global

void InitBuiltinFunctions() {
  if (!g_builtin_funcs.empty()) return;

  g_builtin_funcs.push_back(
      std::make_shared<FuncDefAST>(VarType::INT, "getint", nullptr, true));
  g_builtin_funcs.push_back(
      std::make_shared<FuncDefAST>(VarType::INT, "getch", nullptr, true));
  g_builtin_funcs.push_back(
      std::make_shared<FuncDefAST>(VarType::FLOAT, "getfloat", nullptr, true));

  std::vector<std::unique_ptr<FuncFParamAST>> temp1;
  temp1.push_back(std::make_unique<FuncFParamAST>(VarType::INT, "", nullptr));
  g_builtin_funcs.push_back(std::make_shared<FuncDefAST>(
      VarType::INT, "getarray", std::move(temp1), nullptr, true));

  std::vector<std::unique_ptr<FuncFParamAST>> temp2;
  temp2.push_back(std::make_unique<FuncFParamAST>(VarType::FLOAT, "", nullptr));
  g_builtin_funcs.push_back(std::make_shared<FuncDefAST>(
      VarType::INT, "getfarray", std::move(temp2), nullptr, true));

  std::vector<std::unique_ptr<FuncFParamAST>> temp3;
  temp3.push_back(std::make_unique<FuncFParamAST>(VarType::INT, ""));
  g_builtin_funcs.push_back(std::make_shared<FuncDefAST>(
      VarType::VOID, "putint", std::move(temp3), nullptr, true));

  std::vector<std::unique_ptr<FuncFParamAST>> temp4;
  temp4.push_back(std::make_unique<FuncFParamAST>(VarType::INT, ""));
  g_builtin_funcs.push_back(std::make_shared<FuncDefAST>(
      VarType::VOID, "putch", std::move(temp4), nullptr, true));

  std::vector<std::unique_ptr<FuncFParamAST>> temp5;
  temp5.push_back(std::make_unique<FuncFParamAST>(VarType::FLOAT, ""));
  g_builtin_funcs.push_back(std::make_shared<FuncDefAST>(
      VarType::VOID, "putfloat", std::move(temp5), nullptr, true));

  std::vector<std::unique_ptr<FuncFParamAST>> temp6;
  temp6.push_back(std::make_unique<FuncFParamAST>(VarType::INT, ""));
  temp6.push_back(std::make_unique<FuncFParamAST>(VarType::INT, "", nullptr));
  g_builtin_funcs.push_back(std::make_shared<FuncDefAST>(
      VarType::VOID, "putarray", std::move(temp6), nullptr, true));

  std::vector<std::unique_ptr<FuncFParamAST>> temp7;
  temp7.push_back(std::make_unique<FuncFParamAST>(VarType::INT, ""));
  temp7.push_back(std::make_unique<FuncFParamAST>(VarType::FLOAT, "", nullptr));
  g_builtin_funcs.push_back(std::make_shared<FuncDefAST>(
      VarType::VOID, "putfarray", std::move(temp7), nullptr, true));

  assert(g_builtin_funcs.size() == 10);
}
