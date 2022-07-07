#ifndef BDDD_IR_H
#define BDDD_IR_H

#include <list>
#include <memory>

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

class Module {};

class BasicBlock {};

class Function {};

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

class IRBuilder {
public:
  std::unique_ptr<BasicBlock> CreateBasicBlock();

  std::unique_ptr<Function> CreateFunction();

  std::unique_ptr<PhiInstruction> CreatePhiInstruction();

  std::unique_ptr<Value> CreateBinaryInstruction();
  std::unique_ptr<Value> CreateCallInstruction();
  std::unique_ptr<Value> CreateBranchInstruction();
  std::unique_ptr<Value> CreateJumpInstruction();
  std::unique_ptr<Value> CreateReturnInstruction();
  std::unique_ptr<Value> CreateGetElementPtrInstruction();
  std::unique_ptr<Value> CreateLoadInstruction();
  std::unique_ptr<Value> CreateStoreInstruction();

  void AppendBlock(std::unique_ptr<BasicBlock> basicBlock);
};

#endif  // BDDD_IR_H
