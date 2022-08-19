#ifndef BDDD_ASM_OPTIMIZATION_H
#define BDDD_ASM_OPTIMIZATION_H

#include "asm/asm.h"

void eliminateRedundantMOV(std::shared_ptr<ASM_Module> module);

void eliminateRedundantJump(std::shared_ptr<ASM_Module> module);

bool switchTargetBlock(std::shared_ptr<ASM_BasicBlock> block,
                       std::shared_ptr<BInst> inst);

void removeUnreachableBlock(std::shared_ptr<ASM_Module> module);

void combineInstruction(std::shared_ptr<ASM_Module> module);

std::shared_ptr<ASM_Instruction> combineMULToADD(std::shared_ptr<MULInst> mul,
                                                 std::shared_ptr<ASInst> as);

void eliminateDeadInstruction(std::shared_ptr<ASM_Module> module);

void optimizeTemp(std::shared_ptr<ASM_Module> module, bool optimization);

void optimize(std::shared_ptr<ASM_Module> module);

#endif  // BDDD_ASM_OPTIMIZATION_H