#include "asm/asm.h"

// Module part
void ASM_Module::printGlobalVar(std::ofstream& ofs) {
  // TODO(Huang): wait for completion of global_var in ir module
}

void ASM_Module::print(std::ofstream& ofs) {
  ofs << "\t.arch armv8-a" << std::endl;
  ofs << "\t.arch_extension crc" << std::endl;
  ofs << "\t.syntax unified" << std::endl;
  ofs << "\t.arm" << std::endl;
  ofs << "\t.fpu crypto-neon-fp-armv8" << std::endl;

  printGlobalVar(ofs);
  ofs << "\t.text" << std::endl;  // start of the code section
  for (auto& p : m_funcs) {
    p->print(ofs);
  }

  // sylib generates additional outputs and we have to link it.
  // throw an arbitrary branch here. It should be unreachable.
  ofs << "\tB getint" << std::endl;
}

// Function part
void ASM_Function::print(std::ofstream& ofs) {
  ofs << "\t.global " << m_name << std::endl;
  ofs << "\t.align 2" << std::endl;
  ofs << "\t.type " << m_name << ", \%function" << std::endl;
  ofs << m_name << ":" << std::endl;

  // TODO(Huang): print PUSH, Block and POP

  ofs << "\t.pool" << std::endl;
  ofs << "\t.size " << m_name << ", .-" << m_name << std::endl;
}
