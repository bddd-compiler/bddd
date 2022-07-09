#include "ast/ast.h"

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

bool LValAST::IsArray() {
  return m_decl->DimensionsSize() > m_dimensions.size();
}

void DeclAST::AddDimension(std::unique_ptr<ExprAST> expr) {
  m_dimensions.push_back(std::move(expr));
}
void FuncFParamAST::AddDimension(int x) {
  m_dimensions.push_back(std::make_unique<ExprAST>(x));
}
void FuncFParamAST::AddDimension(std::unique_ptr<ExprAST> expr) {
  m_dimensions.push_back(std::move(expr));
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
