#ifndef BDDD_ASM_H
#define BDDD_ASM_H

#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <unordered_set>
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
  VADD,
  VSUB,
  VMLA,
  VMLS,
  VMUL,
  VDIV,
  VCMP,
  VLDR,
  VSTR,
};

enum class OperandType { REG, VREG, IMM };

enum class CondType { EQ, NE, LT, LE, GT, GE, NONE };

enum class RIType { REG, IMM };

class ASM_Instruction;

class Operand : public std::enable_shared_from_this<Operand> {
public:
  static int vreg_cnt;
  static std::unordered_map<std::shared_ptr<Operand>, std::string> vreg_map;

  std::shared_ptr<ASM_Instruction> m_inst;
  OperandType m_op_type;
  std::string m_name;
  RReg m_rreg;
  SReg m_sreg;
  bool m_is_rreg;
  int m_immval;

  Operand(OperandType t, bool r = true) : m_op_type(t), m_is_rreg(r) {}

  Operand(int val, bool r = true)
      : m_op_type(OperandType::IMM), m_immval(val), m_is_rreg(r) {}

  std::string getName();

  std::string getRegName();

  std::string getVRegName();

  static bool immCheck(int imm);
};

class ASM_Instruction;
class ASM_BasicBlock;
class ASM_Function;

class ASM_Module {
public:
  std::list<std::shared_ptr<ASM_Function>> m_funcs;
  std::shared_ptr<Module> m_ir_module;

  void exportGlobalVar(std::ofstream& ofs);

  void exportASM(std::ofstream& ofs);
};

class PInst;

class ASM_Function {
public:
  std::string m_name;
  std::shared_ptr<Function> m_ir_func;
  std::list<std::shared_ptr<ASM_BasicBlock>> m_blocks;
  std::shared_ptr<ASM_BasicBlock> m_rblock;
  std::list<std::shared_ptr<Operand>> m_params;
  std::unique_ptr<PInst> m_push, m_pop;
  unsigned int m_stack_size;

  ASM_Function(std::shared_ptr<Function> ir_func)
      : m_ir_func(ir_func),
        m_name(ir_func->FuncName()),
        m_rblock(std::make_shared<ASM_BasicBlock>()),
        m_stack_size(0) {}

  unsigned int getStackSize();

  void allocateStack(unsigned int size);

  void exportASM(std::ofstream& ofs);
};

class ASM_BasicBlock {
public:
  static int block_cnt;
  std::string m_label;
  std::list<std::shared_ptr<ASM_Instruction>> m_insts;

  std::unordered_set<std::shared_ptr<Operand>> m_def;
  std::unordered_set<std::shared_ptr<Operand>> m_use;

  ASM_BasicBlock() : m_label(".L" + std::to_string(block_cnt++)) {}

  void exportASM(std::ofstream& ofs);

  void insert(std::shared_ptr<ASM_Instruction> inst);
};

class Shift {
public:
  enum class ShiftType { LSL, LSR, ASR, ROR, RRX } s_type;
  int s_val;

  Shift(ShiftType t, int v);
};

class ASM_Instruction {
public:
  InstOp m_op;
  CondType m_cond;

  std::shared_ptr<ASM_BasicBlock> m_block;
  std::unordered_set<std::shared_ptr<Operand>> m_def;
  std::unordered_set<std::shared_ptr<Operand>> m_use;

  std::string getOpName();

  std::string getCondName();

  void exportInstHead(std::ofstream& ofs);

  virtual void exportASM(std::ofstream& ofs) = 0;
};

class LDRInst : public ASM_Instruction {
public:
  enum class Type { LABEL, REG } m_type;
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_src;
  std::shared_ptr<Operand> m_offs;
  std::string m_label;
  std::unique_ptr<Shift> m_shift;

  LDRInst(std::shared_ptr<Operand> dest, std::string label);

  LDRInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src,
          std::shared_ptr<Operand> offs);

  void exportASM(std::ofstream& ofs) override;
};

class STRInst : public ASM_Instruction {
public:
  std::shared_ptr<Operand> m_src;
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_offs;
  std::unique_ptr<Shift> m_shift;

  STRInst(std::shared_ptr<Operand> src, std::shared_ptr<Operand> dest,
          std::shared_ptr<Operand> offs);

  void exportASM(std::ofstream& ofs) override;
};

// TODO(Huang): class ADRInst
class ADRInst;

class MOVInst : public ASM_Instruction {
public:
  RIType m_type;
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_src;

  MOVInst(std::shared_ptr<Operand> dest, int imm);

  MOVInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src);

  void exportASM(std::ofstream& ofs) override;
};

// TODO(Huang): class VMOVInst

class PInst : public ASM_Instruction {
public:
  std::vector<std::shared_ptr<Operand>> m_regs;

  PInst(InstOp op, std::vector<std::shared_ptr<Operand>> regs);

  void exportASM(std::ofstream& ofs) override;
};

class BInst : public ASM_Instruction {
public:
  std::shared_ptr<ASM_BasicBlock> m_target;

  BInst(std::shared_ptr<ASM_BasicBlock> block);

  void exportASM(std::ofstream& ofs) override;
};

// CALL uses BL inst
class CALLInst : public ASM_Instruction {
public:
  VarType m_type;

  std::string m_label;

  int m_params;

  // TODO(Huang): CALLInst implement
  CALLInst(VarType t, std::string l, int n);

  void exportASM(std::ofstream& ofs) override;
};

class ShiftInst : public ASM_Instruction {
public:
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_src;
  std::shared_ptr<Operand> m_sval;

  ShiftInst(InstOp op, std::shared_ptr<Operand> dest,
            std::shared_ptr<Operand> src, std::shared_ptr<Operand> sval);

  void exportASM(std::ofstream& ofs) override;
};

// ADD SUB RSB
class ASInst : public ASM_Instruction {
public:
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_operand1;
  std::shared_ptr<Operand> m_operand2;
  std::unique_ptr<Shift> m_shift;

  ASInst(InstOp op, std::shared_ptr<Operand> dest,
         std::shared_ptr<Operand> operand1, std::shared_ptr<Operand> operand2);

  void exportASM(std::ofstream& ofs) override;
};

// MUL MLA MLS
class MULInst : public ASM_Instruction {
public:
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_operand1;
  std::shared_ptr<Operand> m_operand2;
  std::shared_ptr<Operand> m_append;

  MULInst(InstOp op, std::shared_ptr<Operand> dest,
          std::shared_ptr<Operand> operand1, std::shared_ptr<Operand> operand2,
          std::shared_ptr<Operand> append = nullptr);

  void exportASM(std::ofstream& ofs) override;
};

class SDIVInst : public ASM_Instruction {
public:
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_devidend;
  std::shared_ptr<Operand> m_devisor;

  SDIVInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> devidend,
           std::shared_ptr<Operand> devisor);

  void exportASM(std::ofstream& ofs) override;
};

class BITInst : public ASM_Instruction {
public:
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_operand1;
  std::shared_ptr<Operand> m_operand2;
  std::unique_ptr<Shift> m_shift;

  BITInst(InstOp op, std::shared_ptr<Operand> dest,
          std::shared_ptr<Operand> operand1, std::shared_ptr<Operand> operand2);

  // MVN
  BITInst(InstOp op, std::shared_ptr<Operand> dest,
          std::shared_ptr<Operand> operand1);

  void exportASM(std::ofstream& ofs) override;
};

// CMP TST
class CTInst : public ASM_Instruction {
public:
  std::shared_ptr<Operand> m_operand1;
  std::shared_ptr<Operand> m_operand2;
  std::unique_ptr<Shift> m_shift;

  CTInst(InstOp op, std::shared_ptr<Operand> operand1,
         std::shared_ptr<Operand> operand2);

  void exportASM(std::ofstream& ofs) override;
};

#endif  // BDDD_ASM_H