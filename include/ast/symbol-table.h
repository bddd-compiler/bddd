#ifndef BDDD_SYMBOL_TABLE_H
#define BDDD_SYMBOL_TABLE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast/ast.h"

enum class ScopeType {
  GLOBAL,
  FUNC,
  LOOP,
  BLOCK,
};

class SymbolTable {
private:
  std::vector<std::unordered_map<std::string, std::shared_ptr<AST>>> tables;
  std::vector<ScopeType> scopes;

public:
  explicit SymbolTable();

  bool Insert(const std::string& name, const std::shared_ptr<AST>& ast);

  std::shared_ptr<DeclAST> GetDecl(const std::string& name);

  std::shared_ptr<FuncDefAST> GetFuncDef(const std::string& name);

  void InitializeScope(ScopeType scope);

  bool FinalizeScope();

  [[nodiscard]] size_t size() const;

  // ScopeType currentScope();

  bool existScope(ScopeType scope);
};

#endif  // BDDD_SYMBOL_TABLE_H
