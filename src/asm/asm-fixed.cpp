#include "asm/asm-fixed.h"

void storeRegisters(std::shared_ptr<ASM_Function> function, std::shared_ptr<Operand> reg) {
  function->appendPush(reg);
  function->appendPop(reg);
}

