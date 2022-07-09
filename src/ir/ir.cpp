#include "ir/ir.h"

#include <memory>
#include <vector>

#include "ast/ast.h"

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
