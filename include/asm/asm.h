#ifndef BDDD_ASM_H
#define BDDD_ASM_H

#include <cmath>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>
#include <set>
#include <bitset>

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

enum class CondType { NONE, NE, LT, LE, GT, GE, EQ };

CondType GetCondFromIR(IROp op);

class ASM_Instruction;

class Operand : public std::enable_shared_from_this<Operand> {
public:
  static int vrreg_cnt;
  static int vsreg_cnt;
  static std::unordered_map<std::shared_ptr<Operand>, std::string> vreg_map;
  static std::unordered_map<RReg, std::shared_ptr<Operand>> rreg_map;

  std::shared_ptr<ASM_Instruction> m_inst;
  OperandType m_op_type;
  std::string m_name;
  RReg m_rreg;
  SReg m_sreg;
  bool m_is_rreg;
  int m_int_val;
  float m_float_val;

  //
  int lifespan = 0;
  bool rejected = false;

  Operand(OperandType t, bool r = true) : m_op_type(t), m_is_rreg(r) {}

  Operand(int val)
      : m_op_type(OperandType::IMM), m_int_val(val), m_is_rreg(true) {}

  Operand(float val)
      : m_op_type(OperandType::IMM), m_float_val(val), m_is_rreg(false) {}

  std::string getName();

  std::string getRegName();

  std::string getVRegName();

  static std::shared_ptr<Operand> getRReg(RReg r);

  static bool immCheck(int imm);

  static bool immCheck(float imm);
};

class ASM_Instruction;
class ASM_BasicBlock;
class ASM_Function;

class ASM_Module {
public:
  std::list<std::shared_ptr<ASM_Function>> m_funcs;
  std::shared_ptr<Module> m_ir_module;

  void exportGlobalVar(std::ofstream& ofs);

  template <class T>
  void exportVarBody(std::ofstream& ofs, std::shared_ptr<T> init_val);

  void exportASM(std::ofstream& ofs);
};

class PInst;

class ASM_Function {
public:
  std::string m_name;
  std::shared_ptr<Function> m_ir_func;
  std::list<std::shared_ptr<ASM_BasicBlock>> m_blocks;
  std::shared_ptr<ASM_BasicBlock> m_rblock;
  std::unique_ptr<PInst> m_push, m_pop;
  std::list<std::shared_ptr<ASM_Instruction>> m_params_set_list;
  int m_params;
  unsigned int m_local_alloc;
  std::stack<std::shared_ptr<Operand>> m_sp_alloc_size;

  ASM_Function(std::shared_ptr<Function> ir_func)
      : m_ir_func(ir_func),
        m_name(ir_func->FuncName()),
        m_rblock(std::make_shared<ASM_BasicBlock>()),
        m_local_alloc(0),
        m_push(std::make_unique<PInst>(InstOp::PUSH)),
        m_pop(std::make_unique<PInst>(InstOp::POP)) {}

  unsigned int getStackSize();

  void allocateStack(unsigned int size);

  void appendPush(std::shared_ptr<Operand> reg);

  void appendPop(std::shared_ptr<Operand> reg);

  void exportASM(std::ofstream& ofs);
};

class ASM_BasicBlock : public std::enable_shared_from_this<ASM_BasicBlock> {
public:
  static int block_cnt;
  std::string m_label;
  std::list<std::shared_ptr<ASM_Instruction>> m_insts;
  std::list<std::shared_ptr<ASM_Instruction>>::iterator m_branch_pos;
  std::list<std::shared_ptr<ASM_Instruction>> m_mov_filled_list;

  int m_loop_depth;

  std::unordered_set<std::shared_ptr<Operand>> m_def;
  std::unordered_set<std::shared_ptr<Operand>> m_use;
  std::unordered_set<std::shared_ptr<Operand>> m_livein;
  std::unordered_set<std::shared_ptr<Operand>> m_liveout;
  std::vector<std::shared_ptr<ASM_BasicBlock>> m_predecessors;
  std::vector<std::shared_ptr<ASM_BasicBlock>> m_successors;

  ASM_BasicBlock(int depth = 0)
      : m_label(".L" + std::to_string(block_cnt++)),
        m_loop_depth(depth),
        m_branch_pos(m_insts.end()) {}

  void insert(std::shared_ptr<ASM_Instruction> inst);

  void insertSpillLDR(
      std::list<std::shared_ptr<ASM_Instruction>>::iterator iter,
      std::shared_ptr<ASM_Instruction> ldr,
      std::shared_ptr<ASM_Instruction> mov = nullptr);

  void insertSpillSTR(
      std::list<std::shared_ptr<ASM_Instruction>>::iterator iter,
      std::shared_ptr<ASM_Instruction> str,
      std::shared_ptr<ASM_Instruction> mov = nullptr);

  void insertPhiMOV(std::shared_ptr<ASM_Instruction> mov);

  void appendFilledMOV(std::shared_ptr<ASM_Instruction> mov);

  void fillMOV();

  void appendSuccessor(std::shared_ptr<ASM_BasicBlock> succ);

  void appendPredecessor(std::shared_ptr<ASM_BasicBlock> pred);

  std::vector<std::shared_ptr<ASM_BasicBlock>> getSuccessors();

  std::vector<std::shared_ptr<ASM_BasicBlock>> getPredecessors();

  void exportASM(std::ofstream& ofs);
};

class Shift {
public:
  enum class ShiftType { LSL, LSR, ASR, ROR, RRX } s_type;
  int s_val;

  Shift(ShiftType t, int v);

  std::string getShiftName();

  void exportASM(std::ofstream& ofs);
};

class ASM_Instruction {
public:
  InstOp m_op;
  CondType m_cond;

  std::shared_ptr<ASM_BasicBlock> m_block;
  std::unordered_set<std::shared_ptr<Operand>> m_def;
  std::unordered_set<std::shared_ptr<Operand>> m_use;

  std::string getOpName();

  std::string getOpSuffixName();

  std::string getCondName();

  void exportInstHead(std::ofstream& ofs);

  virtual void exportASM(std::ofstream& ofs) = 0;

  void addDef(std::shared_ptr<Operand> def);

  void addUse(std::shared_ptr<Operand> use);

  virtual void replaceDef(std::shared_ptr<Operand> newOp,
                          std::shared_ptr<Operand> oldOp)
      = 0;

  virtual void replaceUse(std::shared_ptr<Operand> newOp,
                          std::shared_ptr<Operand> oldOp)
      = 0;
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

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
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

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
};

// TODO(Huang): class ADRInst
class ADRInst;

class MOVInst : public ASM_Instruction {
public:
  enum class RIType { REG, IMM } m_type;
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_src;

  MOVInst(std::shared_ptr<Operand> dest, int imm);

  MOVInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src);

  void exportASM(std::ofstream& ofs) override;

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
};

// TODO(Huang): class VMOVInst

class PInst : public ASM_Instruction {
public:
  std::set<std::shared_ptr<Operand>> m_regs;

  PInst(InstOp op);

  void exportASM(std::ofstream& ofs) override;

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
};

class BInst : public ASM_Instruction {
public:
  std::shared_ptr<ASM_BasicBlock> m_target;

  BInst(std::shared_ptr<ASM_BasicBlock> block);

  void exportASM(std::ofstream& ofs) override;

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
};

// CALL uses BL inst
class CALLInst : public ASM_Instruction {
public:
  VarType m_type;

  std::string m_label;

  int m_params;

  CALLInst(VarType t, std::string l, int n);

  void exportASM(std::ofstream& ofs) override;

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
};

class ShiftInst : public ASM_Instruction {
public:
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_src;
  std::shared_ptr<Operand> m_sval;

  ShiftInst(InstOp op, std::shared_ptr<Operand> dest,
            std::shared_ptr<Operand> src, std::shared_ptr<Operand> sval);

  void exportASM(std::ofstream& ofs) override;

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
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

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
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

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
};

class SDIVInst : public ASM_Instruction {
public:
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_devidend;
  std::shared_ptr<Operand> m_devisor;

  SDIVInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> devidend,
           std::shared_ptr<Operand> devisor);

  void exportASM(std::ofstream& ofs) override;

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
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

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
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

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
};

class POOLInst : public ASM_Instruction {
public:
  int m_number;

  POOLInst(int number) : m_number(number) {}

  void exportASM(std::ofstream& ofs) override;

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override {}

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override {}
};

// float-related instruction
class VMOVInst : public ASM_Instruction {
public:
  
};



#endif  // BDDD_ASM_H