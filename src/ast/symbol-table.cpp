#include "ast/symbol-table.h"

#include <cassert>

SymbolTable::SymbolTable() : tables(), scopes() {
  tables.emplace_back();
  scopes.push_back(ScopeType::GLOBAL);
}

SymbolTable::SymbolTable(const std::vector<std::shared_ptr<FuncDefAST>>& funcs)
    : tables(), scopes() {
  tables.emplace_back();
  scopes.push_back(ScopeType::GLOBAL);

  for (auto& func : funcs) {
    // anything wrong should not happen here
    Insert(func->FuncName(), func);
  }
}

bool SymbolTable::Insert(const std::string& name,
                         const std::shared_ptr<AST>& ast) {
  auto& table = tables.back();
  if (table.find(name) != table.end()) return false;
  table[name] = ast;
  return true;
}

void SymbolTable::InitializeScope(ScopeType scope) {
  tables.emplace_back();
  scopes.push_back(scope);
}
bool SymbolTable::FinalizeScope() {
  if (size() <= 1) return false;
  tables.pop_back();
  return true;
}
size_t SymbolTable::size() const { return tables.size(); }
std::shared_ptr<DeclAST> SymbolTable::GetDecl(const std::string& name) {
  for (auto it = tables.rbegin(); it != tables.rend(); ++it) {
    if (it->find(name) != it->end()) {
      if (auto ptr = std::dynamic_pointer_cast<DeclAST>((*it)[name]))
        return ptr;
      else
        return nullptr;
    }
  }
  return nullptr;
}
std::shared_ptr<FuncDefAST> SymbolTable::GetFuncDef(const std::string& name) {
  for (auto it = tables.rbegin(); it != tables.rend(); ++it) {
    if (it->find(name) != it->end()) {
      if (auto ptr = std::dynamic_pointer_cast<FuncDefAST>((*it)[name]))
        return ptr;
      else
        return nullptr;
    }
  }
  return nullptr;
}
// ScopeType SymbolTable::currentScope() { return scopes.back(); }
bool SymbolTable::existScope(ScopeType scope) {
  for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
    if (*it == scope) return true;
  }
  return false;
}
