#ifndef BDDD_IR_H
#define BDDD_IR_H

#include <list>
#include <memory>

#include "ast/ast.h"

enum class IROp {
  ADD,
  SUB,
  MUL,
  SDIV,
  SREM,
  ICMP,
  AND,
  OR,
  CALL,
  ALLOCA,
  LOAD,
  STORE,
  GET_ELEMENT_PTR,
  PHI,
};

class Use;

class Value {
public:
  std::list<Use *> use_list;
};

class User : public Value {
protected:
  std::list<Use *> operand_list;
  int num_operands;

public:
};

class Use {
public:
  Value *value;
  User *user;
};

class BasicBlock {
private:
  std::string name;

public:
  explicit BasicBlock(std::string name) : name(std::move(name)) {}
};

class Function {
private:
  std::string func_name;

public:
  [[nodiscard]] std::string funcName() const { return func_name; }
};

class Constant : public User {};

class GlobalValue : public Constant {};

class Instruction : public User {};

class BinaryInstruction : public Instruction {};

class PhiInstruction : public Instruction {};

class CallInstruction : public Instruction {};

class BranchInstruction : public Instruction {};

class JumpInstruction : public Instruction {};

class ReturnInstruction : public Instruction {};

class AccessInstruction : public Instruction {};

class GetElementPtrInstruction : public AccessInstruction {};

class LoadInstruction : public AccessInstruction {};

class StoreInstruction : public AccessInstruction {};

class AllocaInstruction : public Instruction {};

class Module {
public:
  std::list<std::shared_ptr<Function>> function_list;
  std::list<std::shared_ptr<GlobalValue>> global_value_list;
};

class IRBuilder {
public:
  Module module;
  std::shared_ptr<BasicBlock> if_then, if_else, if_finally;
  std::shared_ptr<BasicBlock> while_preheader, while_do, while_finally;

public:
  explicit IRBuilder()
      : module(),
        if_then(nullptr),
        if_else(nullptr),
        if_finally(nullptr),
        while_preheader(nullptr),
        while_do(nullptr),
        while_finally(nullptr) {}

  std::unique_ptr<BasicBlock> CreateBasicBlock(std::string block_name);

  std::unique_ptr<Function> CreateFunction(const std::string &name,
                                           VarType return_type);

  // std::unique_ptr<PhiInstruction> CreatePhiInstruction();

  std::unique_ptr<Value> CreateBinaryInstruction(IROp op,
                                                 std::shared_ptr<Value> lhs,
                                                 std::shared_ptr<Value> rhs);

  std::unique_ptr<Value> CreateCallInstruction(
      VarType return_type, std::string func_name,
      std::vector<std::shared_ptr<ExprAST>> params);

  std::unique_ptr<Value> CreateBranchInstruction(
      std::shared_ptr<Value> cond_val, std::shared_ptr<BasicBlock> true_block,
      std::shared_ptr<BasicBlock> false_block);

  std::unique_ptr<Value> CreateJumpInstruction(
      std::shared_ptr<BasicBlock> block);

  std::unique_ptr<Value> CreateReturnInstruction(std::shared_ptr<Value> ret
                                                 = nullptr);
  // std::unique_ptr<Value> CreateGetElementPtrInstruction();
  // std::unique_ptr<Value> CreateLoadInstruction();
  std::unique_ptr<Value> CreateStoreInstruction(std::shared_ptr<Value> addr,
                                                std::shared_ptr<Value> val);
  // std::unique_ptr<Value> CreateAllocaInstruction();

  std::unique_ptr<Value> CreateGlobalValue(std::shared_ptr<DeclAST> decl);
  std::unique_ptr<Value> CreateConstant(int int_val);
  std::unique_ptr<Value> CreateConstant(float float_val);

  void AppendBlock(std::shared_ptr<BasicBlock> block);
};

#endif  // BDDD_IR_H
