#include "ast/ast.h"
#include "ast/symbol-table.h"
#include "exceptions.h"

void InitValAST::TypeCheck(SymbolTable& symbolTable) {
  if (expr) {
    expr->TypeCheck(symbolTable);
    setIsConst(expr->isConst());
  } else {
    bool flag = true;
    for (auto& val : vals) {
      val->TypeCheck(symbolTable);
      if (!val->isConst() && flag) flag = false;
    }
    setIsConst(flag);
  }
}
void LValAST::TypeCheck(SymbolTable& symbolTable) {
  auto ptr = symbolTable.GetDecl(name);
  if (!ptr) throw MyException("undefined lval");
  if (ptr->dimensionsSize() < dimensions.size())
    throw MyException("array dimensions of lval more than real array");

  for (auto& dimension : dimensions) {
    dimension->TypeCheck(symbolTable);
  }
}
void ExprAST::TypeCheck(SymbolTable& symbolTable) {
  // TODO: ExprAST::TypeCheck
}
void DeclAST::TypeCheck(SymbolTable& symbolTable) {
  if (var_type != VarType::INT && var_type != VarType::FLOAT)
    throw MyException("unexpected var_type in DeclAST");

  if (init_val) init_val->TypeCheck(symbolTable);

  if (is_const && !init_val)
    throw MyException("const declaration without init val");
  if (is_const && init_val && !init_val->isConst())
    throw MyException("const declaration with non-const init val");

  // TODO: DeclAST::TypeCheck array items and multipliers

  if (!symbolTable.Insert(varname, shared_from_this()))
    throw MyException("declaration defined multiple times");
}
void FuncCallAST::TypeCheck(SymbolTable& symbolTable) {
  auto ptr = symbolTable.GetFuncDef(func_name);
  if (!ptr) throw MyException("FuncCall call on undefined function");

  if (paramsSize() != ptr->paramsSize())
    throw MyException("incorrect # of params");

  return_type = ptr->returnType();
  for (auto& param : params) {
    param->TypeCheck(symbolTable);
  }
}
void CondAST::TypeCheck(SymbolTable& symbolTable) {
  expr->TypeCheck(symbolTable);
}
void FuncFParamAST::TypeCheck(SymbolTable& symbolTable) {
  // TODO: FuncFParamAST::TypeCheck
}
void BlockAST::TypeCheck(SymbolTable& symbolTable) {
  symbolTable.InitializeScope(ScopeType::BLOCK);
  for (auto& node : nodes) {
    node->TypeCheck(symbolTable);
  }
  symbolTable.FinalizeScope();
}
void FuncDefAST::TypeCheck(SymbolTable& symbolTable) {
  symbolTable.InitializeScope(ScopeType::FUNC);
  if (return_type != VarType::VOID && return_type != VarType::INT
      && return_type != VarType::FLOAT)
    throw MyException("illegal return type of function");

  if (!block) throw MyException("no block stmt in FuncDef");

  try {
    for (auto& param : params) {
      param->TypeCheck(symbolTable);
    }
    block->TypeCheck(symbolTable);
  } catch (MyException& e) {
    symbolTable.FinalizeScope();
    throw e.copy();
  }

  symbolTable.FinalizeScope();
  if (symbolTable.GetFuncDef(func_name) == nullptr) {
    if (!symbolTable.Insert(func_name, shared_from_this()))
      throw MyException("cannot insert FuncDef after function definition");
  }
}
void AssignStmtAST::TypeCheck(SymbolTable& symbolTable) {
  lval->TypeCheck(symbolTable);
  rhs->TypeCheck(symbolTable);
}
void EvalStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!expr) throw MyException("evaluate nothing???");
  expr->TypeCheck(symbolTable);
}
void IfStmtAST::TypeCheck(SymbolTable& symbolTable) {
  cond->TypeCheck(symbolTable);

  // need to initialize a scope only when block stmt is meaningful to add a
  // scope, which action is invoked recursively in then_stmt
  then_stmt->TypeCheck(symbolTable);
  if (else_stmt) {
    else_stmt->TypeCheck(symbolTable);  // same to above
  }
}
void ReturnStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!symbolTable.existScope(ScopeType::FUNC))
    throw MyException("return stmt appears in non-func scope");
  if (ret) ret->TypeCheck(symbolTable);
}
void WhileStmtAST::TypeCheck(SymbolTable& symbolTable) {
  symbolTable.InitializeScope(ScopeType::LOOP);
  cond->TypeCheck(symbolTable);
  stmt->TypeCheck(symbolTable);
  symbolTable.FinalizeScope();
}
void BreakStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!symbolTable.existScope(ScopeType::LOOP))
    throw MyException("break stmt appears in non-loop scope");
}
void ContinueStmtAST::TypeCheck(SymbolTable& symbolTable) {
  if (!symbolTable.existScope(ScopeType::LOOP))
    throw MyException("continue stmt appears in non-loop scope");
}
void CompUnitAST::TypeCheck(SymbolTable& symbolTable) {
  for (const auto& node : nodes) {
    if (auto decl = std::dynamic_pointer_cast<DeclAST>(node)) {
      decl->setIsGlobal(true);
    } else if (auto func_def = std::dynamic_pointer_cast<FuncDefAST>(node)) {
      symbolTable.Insert(func_def->funcName(), node);
    } else {
      throw MyException("A node in CompUnit is neither DeclAST nor FuncDefAST");
    }
    node->TypeCheck(symbolTable);
  }
}
