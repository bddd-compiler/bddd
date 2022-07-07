#ifndef BDDD_IR_H
#define BDDD_IR_H

class Value {};

class Instruction : public Value {};

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

#endif  // BDDD_IR_H
