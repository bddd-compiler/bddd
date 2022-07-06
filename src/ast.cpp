#include "ast.h"

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

void LVal::addDimension(int x) { dimensions.push_back(std::make_unique<ExprAST>(x)); }

void LVal::addDimension(std::unique_ptr<ExprAST> expr) { dimensions.push_back(std::move(expr)); }
void DeclAST::addDimension(std::unique_ptr<ExprAST> expr) { dimensions.push_back(std::move(expr)); }
void FuncFParam::addDimension(int x) { dimensions.push_back(std::make_unique<ExprAST>(x)); }
void FuncFParam::addDimension(std::unique_ptr<ExprAST> expr) { dimensions.push_back(std::move(expr)); }
void BlockAST::appendNodes(std::vector<std::unique_ptr<Node>> appended_nodes) {
  nodes.insert(nodes.end(), std::make_move_iterator(appended_nodes.begin()),
               std::make_move_iterator(appended_nodes.end()));
  appended_nodes.clear();
}

void CompUnit::appendDecls(std::vector<std::unique_ptr<DeclAST>> decls) {
  nodes.insert(nodes.end(), std::make_move_iterator(decls.begin()), std::make_move_iterator(decls.end()));
  decls.clear();
}
void CompUnit::appendFuncDef(std::unique_ptr<FuncDefAST> funcDef) { nodes.push_back(std::move(funcDef)); }
