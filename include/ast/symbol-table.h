#ifndef BDDD_SYMBOL_TABLE_H
#define BDDD_SYMBOL_TABLE_H

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

class AST;
class DeclAST;
class FuncDefAST;

class SymbolTable {
private:
  std::stack<std::unordered_map<std::string, std::shared_ptr<AST>>> tables;

public:
  bool InsertDecl(std::string name, std::shared_ptr<DeclAST> decl);

  bool InsertFuncDef(std::string name, std::shared_ptr<FuncDefAST> funcDef);

  std::shared_ptr<DeclAST> GetDecl(std::string name);

  std::shared_ptr<FuncDefAST> GetFuncDef(std::string name);

  void InitializeScope();

  bool FinalizeScope();

  [[nodiscard]] size_t size() const;
};

#endif  // BDDD_SYMBOL_TABLE_H
