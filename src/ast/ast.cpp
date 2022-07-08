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
  dimensions.push_back(std::make_unique<ExprAST>(x));
}

void LValAST::AddDimension(std::unique_ptr<ExprAST> expr) {
  dimensions.push_back(std::move(expr));
}

bool LValAST::isArray() { return decl->dimensionsSize() > dimensions.size(); }

void DeclAST::AddDimension(std::unique_ptr<ExprAST> expr) {
  dimensions.push_back(std::move(expr));
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
