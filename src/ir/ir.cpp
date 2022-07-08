#include "ir/ir.h"

#include <memory>
#include <vector>

#include "ast/ast.h"

static std::vector<std::unique_ptr<FuncDefAST>> builtin_funcs;

void InitBuiltinFunctions() {
  builtin_funcs.clear();

  builtin_funcs.push_back(
      std::make_unique<FuncDefAST>(VarType::INT, "getint", nullptr));
  builtin_funcs.push_back(
      std::make_unique<FuncDefAST>(VarType::INT, "getch", nullptr));
  builtin_funcs.push_back(
      std::make_unique<FuncDefAST>(VarType::FLOAT, "getfloat", nullptr));

  std::vector<std::unique_ptr<FuncFParamAST>> temp1;
  temp1.push_back(std::make_unique<FuncFParamAST>(VarType::INT, "", nullptr));
  builtin_funcs.push_back(std::make_unique<FuncDefAST>(
      VarType::INT, "getarray", std::move(temp1), nullptr));

  std::vector<std::unique_ptr<FuncFParamAST>> temp2;
  temp2.push_back(std::make_unique<FuncFParamAST>(VarType::FLOAT, "", nullptr));
  builtin_funcs.push_back(std::make_unique<FuncDefAST>(
      VarType::INT, "getfarray", std::move(temp2), nullptr));

  std::vector<std::unique_ptr<FuncFParamAST>> temp3;
  temp3.push_back(std::make_unique<FuncFParamAST>(VarType::INT, ""));
  builtin_funcs.push_back(std::make_unique<FuncDefAST>(
      VarType::VOID, "putint", std::move(temp3), nullptr));

  std::vector<std::unique_ptr<FuncFParamAST>> temp4;
  temp4.push_back(std::make_unique<FuncFParamAST>(VarType::INT, ""));
  builtin_funcs.push_back(std::make_unique<FuncDefAST>(
      VarType::VOID, "putch", std::move(temp4), nullptr));

  std::vector<std::unique_ptr<FuncFParamAST>> temp5;
  temp5.push_back(std::make_unique<FuncFParamAST>(VarType::FLOAT, ""));
  builtin_funcs.push_back(std::make_unique<FuncDefAST>(
      VarType::VOID, "putfloat", std::move(temp5), nullptr));

  std::vector<std::unique_ptr<FuncFParamAST>> temp6;
  temp6.push_back(std::make_unique<FuncFParamAST>(VarType::INT, ""));
  temp6.push_back(std::make_unique<FuncFParamAST>(VarType::INT, "", nullptr));
  builtin_funcs.push_back(std::make_unique<FuncDefAST>(
      VarType::VOID, "putarray", std::move(temp6), nullptr));

  std::vector<std::unique_ptr<FuncFParamAST>> temp7;
  temp7.push_back(std::make_unique<FuncFParamAST>(VarType::INT, ""));
  temp7.push_back(std::make_unique<FuncFParamAST>(VarType::FLOAT, "", nullptr));
  builtin_funcs.push_back(std::make_unique<FuncDefAST>(
      VarType::VOID, "putfarray", std::move(temp7), nullptr));
}

void convertSSA() {
  InitBuiltinFunctions();
  IRBuilder builder;
}
void IRBuilder::AppendBlock(std::shared_ptr<BasicBlock> block) {}
std::unique_ptr<Value> IRBuilder::CreateConstant(int int_val) {
  // TODO(garen): nothing created here
  return nullptr;
}
std::unique_ptr<Value> IRBuilder::CreateConstant(float float_val) {
  // TODO(garen): nothing created here
  return nullptr;
}
std::unique_ptr<Value> IRBuilder::CreateGlobalValue(
    std::shared_ptr<DeclAST> decl) {
  // TODO(garen): nothing created here
  return nullptr;
}
std::unique_ptr<Value> IRBuilder::CreateReturnInstruction(
    std::shared_ptr<Value> ret) {
  // TODO(garen): nothing created here
  return nullptr;
}
std::unique_ptr<BasicBlock> IRBuilder::CreateBasicBlock(
    std::string block_name) {
  // TODO(garen): nothing created here
  return nullptr;
}
std::unique_ptr<Function> IRBuilder::CreateFunction(const std::string& name,
                                                    VarType return_type) {
  // TODO(garen): nothing created here
  return nullptr;
}
std::unique_ptr<Value> IRBuilder::CreateBinaryInstruction(
    IROp op, std::shared_ptr<Value> lhs, std::shared_ptr<Value> rhs) {
  // TODO(garen): nothing created here
  return nullptr;
}
std::unique_ptr<Value> IRBuilder::CreateCallInstruction(
    VarType return_type, std::string func_name,
    std::vector<std::shared_ptr<ExprAST>> params) {
  // TODO(garen): nothing created here
  return nullptr;
}
std::unique_ptr<Value> IRBuilder::CreateJumpInstruction(
    std::shared_ptr<BasicBlock> block) {
  // TODO(garen): nothing created here
  return nullptr;
}
std::unique_ptr<Value> IRBuilder::CreateBranchInstruction(
    std::shared_ptr<Value> cond_val, std::shared_ptr<BasicBlock> true_block,
    std::shared_ptr<BasicBlock> false_block) {
  // TODO(garen): nothing created here
  return nullptr;
}
std::unique_ptr<Value> IRBuilder::CreateStoreInstruction(
    std::shared_ptr<Value> addr, std::shared_ptr<Value> val) {
  // TODO(garen): nothing created here
  return nullptr;
}
