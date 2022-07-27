#ifndef BDDD_ASM_FIXED_H
#define BDDD_ASM_FIXED_H

#include "asm/asm.h"

void storeRegisters(std::shared_ptr<ASM_Function> function, std::shared_ptr<Operand> reg);

#endif  // BDDD_ASM_FIXED_H