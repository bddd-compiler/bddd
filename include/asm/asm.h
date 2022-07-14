#ifndef BDDD_ASM_H
#define BDDD_ASM_H

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "ir/ir.h"

enum class RReg {
  R0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R10,
  R11,
  R12,
  SP,
  LR,
  PC
};

enum class SReg {
  S1,
  S2,
  S3,
  S4,
  S5,
  S6,
  S7,
  S8,
  S9,
  S10,
  S11,
  S12,
  S13,
  S14,
  S15,
  S16,
  S17,
  S18,
  S19,
  S20,
  S21,
  S22,
  S23,
  S24,
  S25,
  S26,
  S27,
  S28,
  S29,
  S30,
  S31
};

enum class InstOp {
  // data instructions
  LDR,
  STR,
  ADR,
  MOV,
  PUSH,
  POP,
  // branch instructions
  B,
  BL,
  // shift instructions
  LSL,
  LSR,
  ASR,
  ROR,
  RRX,
  // arithmetic instructions
  ADD,
  SUB,
  RSB,
  MLA,
  MLS,
  MUL,
  SDIV,
  // bitwise instructions
  AND,
  ORR,
  MVN,
  EOR,
  BIC,
  // compare instructions
  CMP,
  TST,
  // floating-poing data-processing instructions
  VMOV,
  VDUP,
  VADD,
  VSUB,
  VMLA,
  VMLS,
  VMUL,
  VDIV,
  VCMP,
  // advanced SIMD load/store instructions
  VLDR,
  VLD1,
  VSTR,
  VST1
};

enum class OperandType { REG, VREG, IMM, LABEL };

enum class CondType { EQ, NE, LT, LE, GT, GE, NONE };

enum class RIType { REG, IMM };

class Operand {
public:
  static int m_vcnt;

  OperandType m_op_type;
  std::string m_name;
  RReg m_rreg;
  SReg m_sreg;
  int m_immval;

  Operand(OperandType t) : m_op_type(t) {}

  Operand(std::string label) : m_op_type(OperandType::LABEL), m_name(label) {}

  Operand(int val) : m_op_type(OperandType::IMM), m_immval(val) {}
};

class ASM_Instruction;
class ASM_BasicBlock;
class ASM_Function;

class ASM_Module {
public:
  std::list<std::shared_ptr<ASM_Function>> m_funcs;
  std::shared_ptr<Module> m_ir_module;
};

class ASM_Function {
public:
  std::string m_name;
  std::shared_ptr<Function> m_ir_func;
  std::list<std::shared_ptr<ASM_BasicBlock>> m_blocks;
};

class ASM_BasicBlock {
public:
  std::string m_label;
  std::shared_ptr<BasicBlock> m_ir_block;
  std::list<std::shared_ptr<ASM_Instruction>> m_insts;
};

class ASM_Instruction {
public:
  InstOp m_op;
  CondType m_cond;
};

class Shift {
public:
  enum class ShiftType { LSL, LSR, ASR, ROR, RRX } s_type;
  int s_val;

  Shift(ShiftType t, int v);
};

class LDRInst : public ASM_Instruction {
public:
  enum class Type { LABEL, REG, IMM } m_type;
  std::unique_ptr<Operand> m_dest;
  std::unique_ptr<Operand> m_src;
  std::unique_ptr<Operand> m_offs;
  std::unique_ptr<Shift> m_shift;

  LDRInst(std::unique_ptr<Operand> dest, std::string label);

  LDRInst(std::unique_ptr<Operand> dest, std::unique_ptr<Operand> src, int imm);

  LDRInst(std::unique_ptr<Operand> dest, std::unique_ptr<Operand> src,
          std::unique_ptr<Operand> offs);
};

class STRInst : public ASM_Instruction {
public:
  RIType m_type;
  std::unique_ptr<Operand> m_src;
  std::unique_ptr<Operand> m_dest;
  std::unique_ptr<Operand> m_offs;
  std::unique_ptr<Shift> m_shift;

  STRInst(std::unique_ptr<Operand> src, std::unique_ptr<Operand> dest, int imm);

  STRInst(std::unique_ptr<Operand> src, std::unique_ptr<Operand> dest,
          std::unique_ptr<Operand> offs);
};

// TODO(Huang): class ADRInst
class ADRInst;

class MOVInst : public ASM_Instruction {
public:
  RIType m_type;
  std::unique_ptr<Operand> m_dest;
  std::unique_ptr<Operand> m_src;

  MOVInst(std::unique_ptr<Operand> dest, int imm);

  MOVInst(std::unique_ptr<Operand> dest, std::unique_ptr<Operand> src);
};

// TODO(Huang): class VMOVInst

class PInst : public ASM_Instruction {
public:
  std::vector<std::unique_ptr<Operand>> m_regs;

  PInst(InstOp op, std::vector<std::unique_ptr<Operand>> regs);
};

class BInst : public ASM_Instruction {
public:
  std::unique_ptr<Operand> m_label;

  BInst(InstOp op, std::unique_ptr<Operand> label);
};

class ShiftInst : public ASM_Instruction {
public:
  RIType m_type;
  std::unique_ptr<Operand> m_dest;
  std::unique_ptr<Operand> m_src;
  std::unique_ptr<Operand> m_sval;

  ShiftInst(InstOp op, std::unique_ptr<Operand> dest,
            std::unique_ptr<Operand> src, int imm);

  ShiftInst(InstOp op, std::unique_ptr<Operand> dest,
            std::unique_ptr<Operand> src, std::unique_ptr<Operand> sval);
};

// ADD SUB RSB
class ASInst : public ASM_Instruction {
public:
  RIType m_type;
  std::unique_ptr<Operand> m_dest;
  std::unique_ptr<Operand> m_operand1;
  std::unique_ptr<Operand> m_operand2;
  std::unique_ptr<Shift> m_shift;

  ASInst(InstOp op, std::unique_ptr<Operand> dest,
         std::unique_ptr<Operand> operand1, int imm);

  ASInst(InstOp op, std::unique_ptr<Operand> dest,
         std::unique_ptr<Operand> operand1, std::unique_ptr<Operand> operand2);
};

// MUL MLA MLS
class MULInst : public ASM_Instruction {
public:
  std::unique_ptr<Operand> m_dest;
  std::unique_ptr<Operand> m_operand1;
  std::unique_ptr<Operand> m_operand2;
  std::unique_ptr<Operand> m_append;

  MULInst(InstOp op, std::unique_ptr<Operand> dest,
          std::unique_ptr<Operand> operand1, std::unique_ptr<Operand> operand2,
          std::unique_ptr<Operand> append = nullptr);
};

class SDIVInst : public ASM_Instruction {
public:
  std::unique_ptr<Operand> m_dest;
  std::unique_ptr<Operand> m_devidend;
  std::unique_ptr<Operand> m_devisor;

  SDIVInst(std::unique_ptr<Operand> dest, std::unique_ptr<Operand> devidend,
           std::unique_ptr<Operand> devisor);
};

class BITInst : public ASM_Instruction {
public:
  RIType m_type;
  std::unique_ptr<Operand> m_dest;
  std::unique_ptr<Operand> m_operand1;
  std::unique_ptr<Operand> m_operand2;
  std::unique_ptr<Shift> m_shift;

  BITInst(InstOp op, std::unique_ptr<Operand> dest,
          std::unique_ptr<Operand> operand1, int imm);

  BITInst(InstOp op, std::unique_ptr<Operand> dest,
          std::unique_ptr<Operand> operand1, std::unique_ptr<Operand> operand2);

  // MVN
  BITInst(InstOp op, std::unique_ptr<Operand> dest, int imm);

  BITInst(InstOp op, std::unique_ptr<Operand> dest,
          std::unique_ptr<Operand> operand1);
};

// CMP TST
class CTInst : public ASM_Instruction {
public:
  RIType m_type;
  std::unique_ptr<Operand> m_operand1;
  std::unique_ptr<Operand> m_operand2;
  std::unique_ptr<Shift> m_shift;

  CTInst(InstOp op, std::unique_ptr<Operand> operand1, int imm);

  CTInst(InstOp op, std::unique_ptr<Operand> operand1,
         std::unique_ptr<Operand> operand2);
};

#endif  // BDDD_ASM_H