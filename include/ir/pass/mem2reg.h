#ifndef BDDD_MEM2REG_H
#define BDDD_MEM2REG_H

#include "ir/builder.h"
#include "ir/ir.h"

void Mem2Reg(std::shared_ptr<Function> function,
             std::shared_ptr<IRBuilder> builder);

#endif  // BDDD_MEM2REG_H
