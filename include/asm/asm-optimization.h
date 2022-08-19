#ifndef BDDD_ASM_OPTIMIZATION_H
#define BDDD_ASM_OPTIMIZATION_H

#include "asm/asm.h"

void eliminateRedundantMOV(std::shared_ptr<ASM_Module> module);

void eliminateRedundantJump(std::shared_ptr<ASM_Module> module);

bool switchTargetBlock(std::shared_ptr<ASM_BasicBlock> block,
                       std::shared_ptr<BInst> inst);

void removeUnreachableBlock(std::shared_ptr<ASM_Module> module);

void eliminateDeadInstruction(std::shared_ptr<ASM_Module> module);

void optimizeTemp(std::shared_ptr<ASM_Module> module, bool optimization);

void optimize(std::shared_ptr<ASM_Module> module);

#endif  // BDDD_ASM_OPTIMIZATION_H