#include "ast/ast.h"

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

void LVal::AddDimension(int x) {
  dimensions.push_back(std::make_unique<ExprAST>(x));
}

void LVal::AddDimension(std::unique_ptr<ExprAST> expr) {
  dimensions.push_back(std::move(expr));
}
void DeclAST::AddDimension(std::unique_ptr<ExprAST> expr) {
  dimensions.push_back(std::move(expr));
}
void DeclAST::setIsConst(bool isConst) { is_const = isConst; }
void DeclAST::setVarType(VarType varType) { var_type = varType; }
void DeclAST::setInitVal(std::unique_ptr<InitVal> initVal) {
  init_val = std::move(initVal);
}
void FuncFParam::AddDimension(int x) {
  dimensions.push_back(std::make_unique<ExprAST>(x));
}
void FuncFParam::AddDimension(std::unique_ptr<ExprAST> expr) {
  dimensions.push_back(std::move(expr));
}
void BlockAST::AppendNodes(std::vector<std::unique_ptr<Node>> appendedNodes) {
  nodes.insert(nodes.end(), std::make_move_iterator(appendedNodes.begin()),
               std::make_move_iterator(appendedNodes.end()));
  appendedNodes.clear();
}

void CompUnit::AppendDecls(std::vector<std::unique_ptr<DeclAST>> decls) {
  nodes.insert(nodes.end(), std::make_move_iterator(decls.begin()),
               std::make_move_iterator(decls.end()));
  decls.clear();
}
void CompUnit::AppendFuncDef(std::unique_ptr<FuncDefAST> funcDef) {
  nodes.push_back(std::move(funcDef));
}
