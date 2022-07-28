#ifndef BDDD_ASM_FIXED_H
#define BDDD_ASM_FIXED_H

#include "asm/asm.h"


#define POOL_RANGE 500

void storeRegisters(std::shared_ptr<ASM_Function> function, std::shared_ptr<Operand> reg);

void generateLiteralPool(std::shared_ptr<ASM_Module> module);

#endif  // BDDD_ASM_FIXED_H