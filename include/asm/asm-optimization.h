#ifndef BDDD_ASM_OPTIMIZATION_H
#define BDDD_ASM_OPTIMIZATION_H

#include "asm/asm.h"

void reduceRedundantMOV(std::shared_ptr<ASM_Module> module);

void optimize(std::shared_ptr<ASM_Module> module);

#endif  // BDDD_ASM_OPTIMIZATION_H