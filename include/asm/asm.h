#ifndef BDDD_ASM_H
#define BDDD_ASM_H

#include <bitset>
#include <cmath>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <stack>
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
  S0,
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
  MSR,
  MRS,
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
  VNEG,
  VCMP,
  VMSR,
  VMRS,
  VLDR,
  VSTR,
  VPUSH,
  VPOP
};

enum class OperandType { SPECIAL_REG, REG, VREG, IMM };

enum class CondType { 
  NONE,     //  Any
  EQ,       //  Z == 1
  NE,       //  Z == 0
  CS,       //  C == 1
  CC,       //  C == 0
  MI,       //  N == 1
  PL,       //  N == 0
  VS,       //  V == 1
  VC,       //  V == 0
  HI,       //  C == 1 and Z == 0
  LS,       //  C == 0 or Z == 1
  GE,       //  N == V
  LT,       //  N != V
  GT,       //  Z == 0 and N == V
  LE        //  Z == 1 or N != V
};

enum class MOVType { REG, IMM };

enum class RegType { R, S };

CondType GetCondFromIR(IROp op);

class ASM_Instruction;

class Operand : public std::enable_shared_from_this<Operand> {
public:
  static int vrreg_cnt;
  static int vsreg_cnt;
  static std::unordered_map<std::shared_ptr<Operand>, std::string> vreg_map;
  static std::unordered_map<RReg, std::shared_ptr<Operand>> rreg_map;
  static std::unordered_map<SReg, std::shared_ptr<Operand>> sreg_map;

  std::shared_ptr<ASM_Instruction> m_inst;
  OperandType m_op_type;
  std::string m_name;
  RReg m_rreg;
  SReg m_sreg;
  RegType m_reg_type;
  std::string m_special_reg;

  bool m_is_float;
  int m_int_val;
  float m_float_val;

  // taken from tinbaccc
  int lifespan = 0;
  bool rejected = false;

  Operand(OperandType t, bool f = false) : m_op_type(t), m_is_float(f) {}

  Operand(int val)
      : m_op_type(OperandType::IMM), m_int_val(val), m_is_float(false) {}

  Operand(float val)
      : m_op_type(OperandType::IMM), m_float_val(val), m_is_float(true) {}

  Operand(std::string reg)
      : m_op_type(OperandType::SPECIAL_REG), m_special_reg(reg) {}

  std::string getName();

  std::string getRegName();

  std::string getVRegName();

  RegType getRegType();

  static std::shared_ptr<Operand> getRReg(RReg r);

  static std::shared_ptr<Operand> getSReg(SReg s);

  static bool immCheck(int imm);

  static bool immCheck(float imm);

  static bool addrOffsCheck(int offs, bool is_float);
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
  std::unique_ptr<PInst> m_f_push, m_f_pop;
  std::list<std::shared_ptr<ASM_Instruction>> m_params_set_list;
  std::unordered_map<std::shared_ptr<Operand>, int> m_stack_params_offs;
  std::unordered_map<std::shared_ptr<ASM_Instruction>,
                     std::list<std::shared_ptr<ASM_Instruction>>::iterator>
      m_params_pos_map;
  int m_params_size;
  int m_local_alloc;
  std::stack<int> m_sp_alloc_size;

  ASM_Function(std::shared_ptr<Function> ir_func)
      : m_ir_func(ir_func),
        m_name(ir_func->FuncName()),
        m_rblock(std::make_shared<ASM_BasicBlock>()),
        m_local_alloc(0),
        m_push(std::make_unique<PInst>(InstOp::PUSH)),
        m_pop(std::make_unique<PInst>(InstOp::POP)),
        m_f_push(std::make_unique<PInst>(InstOp::VPUSH)),
        m_f_pop(std::make_unique<PInst>(InstOp::VPOP)) {}

  void LivenessAnalysis(RegType reg_type);

  int getPushSize();

  int getStackSize();

  void allocateStack(int size);

  void appendPush(std::shared_ptr<Operand> reg);

  void appendPop(std::shared_ptr<Operand> reg);

  void exportASM(std::ofstream& ofs);
};

class MRSInst;

class ASM_BasicBlock : public std::enable_shared_from_this<ASM_BasicBlock> {
public:
  static int block_id;
  std::string m_label;
  std::list<std::shared_ptr<ASM_Instruction>> m_insts;
  std::list<std::shared_ptr<ASM_Instruction>>::iterator m_branch_pos;
  std::list<std::shared_ptr<ASM_Instruction>> m_mov_filled_list;
  std::shared_ptr<MRSInst> m_status_load_inst;

  int m_loop_depth;

  std::unordered_set<std::shared_ptr<Operand>> m_def;
  std::unordered_set<std::shared_ptr<Operand>> m_use;
  std::unordered_set<std::shared_ptr<Operand>> m_livein;
  std::unordered_set<std::shared_ptr<Operand>> m_liveout;
  std::vector<std::shared_ptr<ASM_BasicBlock>> m_predecessors;
  std::vector<std::shared_ptr<ASM_BasicBlock>> m_successors;

  ASM_BasicBlock(int depth = 0)
      : m_loop_depth(depth),
        m_branch_pos(m_insts.end()),
        m_label(".L" + std::to_string(block_id++)),
        m_status_load_inst(nullptr) {}

  void insert(std::shared_ptr<ASM_Instruction> inst);

  void insertSpillLDR(
      std::list<std::shared_ptr<ASM_Instruction>>::iterator iter,
      std::shared_ptr<ASM_Instruction> ldr,
      std::shared_ptr<ASM_Instruction> add,
      std::shared_ptr<ASM_Instruction> mov);

  void insertSpillSTR(
      std::list<std::shared_ptr<ASM_Instruction>>::iterator iter,
      std::shared_ptr<ASM_Instruction> str,
      std::shared_ptr<ASM_Instruction> add,
      std::shared_ptr<ASM_Instruction> mov);

  void insertPhiMOV(std::shared_ptr<ASM_Instruction> mov);

  void appendFilledMOV(std::shared_ptr<ASM_Instruction> mov);

  void fillMOV();

  void appendSuccessor(std::shared_ptr<ASM_BasicBlock> succ);

  void appendPredecessor(std::shared_ptr<ASM_BasicBlock> pred);

  void removeSuccessor(std::shared_ptr<ASM_BasicBlock> succ);

  void removePredecessor(std::shared_ptr<ASM_BasicBlock> pred);

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
  bool m_set_flag;

  int m_params_offset;
  bool m_is_mov;  // for register allocation
  bool m_is_deleted;

  std::shared_ptr<ASM_BasicBlock> m_block;
  std::unordered_set<std::shared_ptr<Operand>> m_def;
  std::unordered_set<std::shared_ptr<Operand>> m_use;
  std::unordered_set<std::shared_ptr<Operand>> m_f_def;
  std::unordered_set<std::shared_ptr<Operand>> m_f_use;

  ASM_Instruction()
      : m_set_flag(false), m_params_offset(0), m_is_mov(false), m_is_deleted(false) {}

  std::string getOpName();

  std::string getOpSuffixName();

  std::string getCondName();

  static CondType getOppositeCond(CondType cond);

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

class MOVInst : public ASM_Instruction {
public:
  MOVType m_type;
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_src;

  MOVInst(std::shared_ptr<Operand> dest, int imm);

  MOVInst(std::shared_ptr<Operand> dest, float imm);

  MOVInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> src);

  void exportASM(std::ofstream& ofs) override;

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
};

// MRS MSR VMRS VMSR
class MRSInst : public ASM_Instruction {
public:
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_src;

  MRSInst(std::string reg, std::shared_ptr<Operand> src);

  MRSInst(std::shared_ptr<Operand> dest, std::string reg);

  void exportASM(std::ofstream& ofs) override;

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
};

class PInst : public ASM_Instruction {
public:
  std::set<std::shared_ptr<Operand>> m_regs;

  PInst(InstOp op);

  void exportASM(std::ofstream& ofs) override;

  void exportBody(std::ofstream& ofs,
                  std::vector<std::shared_ptr<Operand>> regs, int l, int r);

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

class VNEGInst : public ASM_Instruction {
public:
  std::shared_ptr<Operand> m_dest;
  std::shared_ptr<Operand> m_operand;

  VNEGInst(std::shared_ptr<Operand> dest, std::shared_ptr<Operand> operand);

  void exportASM(std::ofstream& ofs) override;

  void replaceDef(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;

  void replaceUse(std::shared_ptr<Operand> newOp,
                  std::shared_ptr<Operand> oldOp) override;
};

#endif  // BDDD_ASM_H