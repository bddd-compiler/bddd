#ifndef BDDD_IR_PASS_MANAGER_H
#define BDDD_IR_PASS_MANAGER_H

#include "ir/builder.h"

// auxiliary
void ComputeDominanceRelationship(std::shared_ptr<Function> function);
void ComputeDominanceFrontier(std::shared_ptr<Function> function);
void RemoveUnusedBasicBlocks(std::shared_ptr<Function> func);
void RemoveTrivialPhis(std::shared_ptr<Function> func);
void ReplaceTrivialBranchByJump(std::shared_ptr<Function> func);
void RemoveTrivialBasicBlocks(std::shared_ptr<Function> func);
void ComputeLoopRelationship(std::shared_ptr<Function> func);
void DeadCodeElimination(std::shared_ptr<Function> function);
void RemoveUnusedFunctions(std::unique_ptr<Module> &module);

class IRPassManager {
public:
  std::shared_ptr<IRBuilder> m_builder;

  explicit IRPassManager(std::shared_ptr<IRBuilder> builder)
      : m_builder(std::move(builder)) {}

  void Mem2RegPass();

  void FunctionInliningPass();

  void GVNPass();

  void GCMPass();

  void SideEffectPass();

private:
};

#endif  // BDDD_IR_PASS_MANAGER_H
