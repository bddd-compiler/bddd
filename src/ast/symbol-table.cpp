#include "ast/symbol-table.h"

bool SymbolTable::InsertDecl(std::string name, std::shared_ptr<DeclAST> decl) {
  // TODO
  return false;
}

bool SymbolTable::InsertFuncDef(std::string name, std::shared_ptr<FuncDefAST> funcDef) {
  // TODO
  return false;
}

void SymbolTable::InitializeScope() { tables.emplace(); }
bool SymbolTable::FinalizeScope() {
  if (size() <= 1) return false;
  tables.pop();
  return true;
}
size_t SymbolTable::size() const { return tables.size(); }
