#ifndef BDDD_BUILDER_H
#define BDDD_BUILDER_H

#include "ir/ir.h"

class IRBuilder : public std::enable_shared_from_this<IRBuilder> {
public:
  std::unique_ptr<Module> m_module;
  std::shared_ptr<BasicBlock> m_if_true, m_if_false, m_if_exit;
  std::shared_ptr<BasicBlock> m_while_entry, m_while_exit;

public:
  explicit IRBuilder(std::unique_ptr<Module> module)
      : m_module(std::move(module)),
        m_if_true(nullptr),
        m_if_false(nullptr),
        m_if_exit(nullptr),
        m_while_entry(nullptr),
        m_while_exit(nullptr) {}

  std::shared_ptr<BasicBlock> CreateBasicBlock(std::string block_name);

  void AppendBasicBlock(std::shared_ptr<BasicBlock> bb);

  std::shared_ptr<Function> CreateFunction(std::shared_ptr<FuncDefAST> func);

  std::shared_ptr<Instruction> CreateBinaryInstruction(
      IROp op, const std::shared_ptr<Value> &lhs,
      const std::shared_ptr<Value> &rhs);

  std::shared_ptr<Instruction> CreateFNegInstruction(
      const std::shared_ptr<Value> &lhs);

  // more specific version
  // example: call memset when initialize a local array
  std::shared_ptr<CallInstruction> CreateCallInstruction(
      VarType return_type, std::string func_name,
      std::vector<std::shared_ptr<Value>> params_values,
      std::shared_ptr<Function> function = nullptr);

  void CreateMemsetInstruction(std::shared_ptr<Value> first_elem, int size);

  // get information from AST
  std::shared_ptr<CallInstruction> CreateCallInstruction(
      VarType return_type, std::string func_name,
      const std::vector<std::shared_ptr<ExprAST>> &params);

  std::shared_ptr<Instruction> CreateBranchInstruction(
      std::shared_ptr<Value> cond_val, std::shared_ptr<BasicBlock> true_block,
      std::shared_ptr<BasicBlock> false_block);

  std::shared_ptr<Instruction> CreateJumpInstruction(
      std::shared_ptr<BasicBlock> block);

  std::shared_ptr<Instruction> CreateReturnInstruction(
      VarType expected_type, std::shared_ptr<Value> ret = nullptr);

  std::shared_ptr<Instruction> CreateGetElementPtrInstruction(
      std::shared_ptr<Value> addr, std::vector<std::shared_ptr<Value>> indices);

  std::shared_ptr<Instruction> CreateLoadInstruction(
      std::shared_ptr<Value> addr);

  std::shared_ptr<Instruction> CreateStoreInstruction(
      std::shared_ptr<Value> addr, std::shared_ptr<Value> val);

  std::shared_ptr<Instruction> CreateAllocaInstruction(
      std::shared_ptr<DeclAST> decl, bool is_arg = false,
      bool is_const = false);

  std::shared_ptr<GlobalVariable> CreateGlobalVariable(
      const std::shared_ptr<DeclAST> &decl);
  std::shared_ptr<IntGlobalVariable> CreateIntGlobalVariable(
      std::shared_ptr<DeclAST> decl);
  std::shared_ptr<FloatGlobalVariable> CreateFloatGlobalVariable(
      std::shared_ptr<DeclAST> decl);

  std::shared_ptr<Value> CreateBitCastInstruction(std::shared_ptr<Value> from,
                                                  BasicType target_type);
  std::shared_ptr<Value> CreateZExtInstruction(std::shared_ptr<Value> from);
  std::shared_ptr<Value> CreateSIToFPInstruction(std::shared_ptr<Value> from);
  std::shared_ptr<Value> CreateFPToSIInstruction(std::shared_ptr<Value> from);

  std::shared_ptr<Value> GetIntConstant(int int_val);
  std::shared_ptr<Value> GetFloatConstant(float float_val);
  std::shared_ptr<Value> GetBoolConstant(bool bool_val);
  std::shared_ptr<Value> GetCharConstant(char char_val);

  std::shared_ptr<Value> GetConstant(IROp op, EvalValue lhs, EvalValue rhs);

  // create an empty phi instruction and insert at the preheader of bb
  std::shared_ptr<PhiInstruction> CreatePhiInstruction(
      ValueType type, std::shared_ptr<BasicBlock> bb);
};

#endif  // BDDD_BUILDER_H
