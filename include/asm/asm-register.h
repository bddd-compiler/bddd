#ifndef BDDD_ASM_REGISTER_H
#define BDDD_ASM_REGISTER_H

#include <set>
#include <stack>
#include <unordered_set>

#include "asm/asm.h"
#include "asm/asm-fixed.h"

typedef std::shared_ptr<Operand> OpPtr;

class OpPairHash {
public:
  size_t operator()(const std::pair<OpPtr, OpPtr>& p) const {
    return std::hash<OpPtr>()(p.first) ^ std::hash<OpPtr>()(p.second);
  }
};

// implement the register allocation algorithm in "Iterated Register Coalescing"
class RegisterAllocator {
private:
  RegType m_reg_type;

  std::shared_ptr<ASM_Module> m_module;
  std::shared_ptr<ASM_Function> m_cur_func;
  std::set<RReg> rreg_avaliable;
  std::set<SReg> sreg_avaliable;
  int K;

  std::unordered_map<OpPtr, int> m_depth_map;

  void setCurFunction(std::shared_ptr<ASM_Function> func);

  // Data Structures
  // Node Worklists, Node Sets, and Node Stacks
  std::unordered_set<OpPtr> precolored;  // we didn't use this set
  std::unordered_set<OpPtr> initial;
  std::unordered_set<OpPtr> simplifyWorklist;
  std::unordered_set<OpPtr> freezeWorklist;
  std::unordered_set<OpPtr> spillWorklist;
  std::unordered_set<OpPtr> spilledNodes;
  std::unordered_set<OpPtr> coalescedNodes;
  std::unordered_set<OpPtr> coloredNodes;
  std::vector<OpPtr> selectStack;

  bool isSelectSpill;
  std::unordered_set<OpPtr> coalescedRecord;  // record all the coalescedNodes
                                              // before first selectSpill();

  // Move Sets
  std::unordered_set<std::shared_ptr<MOVInst>> coalescedMoves;
  std::unordered_set<std::shared_ptr<MOVInst>> constrainedMoves;
  std::unordered_set<std::shared_ptr<MOVInst>> frozenMoves;
  std::unordered_set<std::shared_ptr<MOVInst>> worklistMoves;
  std::unordered_set<std::shared_ptr<MOVInst>> activeMoves;

  // Others
  std::unordered_set<std::pair<OpPtr, OpPtr>, OpPairHash> adjSet;
  std::unordered_map<OpPtr, std::unordered_set<OpPtr>> adjList;
  std::unordered_map<OpPtr, int> degree;
  std::unordered_map<OpPtr, std::unordered_set<std::shared_ptr<MOVInst>>>
      moveList;
  std::unordered_map<OpPtr, OpPtr> alias;
  std::unordered_map<OpPtr, RReg> color_r;
  std::unordered_map<OpPtr, SReg> color_s;

  // procedure
  void AllocateCurFunc();  // Main() in "Iterated Register Coalescing"

  void AddEdge(OpPtr u, OpPtr v);

  void Build();

  std::unordered_set<OpPtr> Adjacent(OpPtr n);

  std::unordered_set<std::shared_ptr<MOVInst>> NodeMoves(OpPtr n);

  bool MoveRelated(OpPtr n);

  void MkWorklist();

  void Simplify();

  void DecrementDegree(OpPtr m);

  void EnableMoves(std::unordered_set<OpPtr>& nodes);

  void Coalesce();

  void AddWorkList(OpPtr u);

  bool OK(OpPtr t, OpPtr r);

  bool Conservative(std::unordered_set<OpPtr>& nodes);

  OpPtr GetAlias(OpPtr n);

  void Combine(OpPtr u, OpPtr v);

  void Freeze();

  void FreezeMoves(OpPtr n);

  void SelectSpill();

  void AssignColors();

  void AssignColorsR();

  void AssignColorsS();

  void RewriteProgram();

  bool AllOK(OpPtr u, OpPtr v);

  bool ConservativeAdj(OpPtr u, OpPtr v);

  void init();

  void initialColors();

  void getInitial();

  // taken from tinbaccc
  void updateDepth(std::shared_ptr<ASM_BasicBlock> block, OpPtr node);

public:
  RegisterAllocator(std::shared_ptr<ASM_Module> module, RegType type);

  void Allocate();

  void debug(std::string func);
};

#endif  // BDDD_ASM_REGISTER_H