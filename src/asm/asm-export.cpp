#include "asm/asm.h"

// Module part
void ASM_Module::exportGlobalVar(std::ofstream& ofs) {
  // TODO(Huang): wait for completion of global_var in ir module
}

void ASM_Module::exportASM(std::ofstream& ofs) {
  ofs << "\t.arch armv8-a" << std::endl;
  ofs << "\t.arch_extension crc" << std::endl;
  ofs << "\t.syntax unified" << std::endl;
  ofs << "\t.arm" << std::endl;
  ofs << "\t.fpu crypto-neon-fp-armv8" << std::endl;

  exportGlobalVar(ofs);
  ofs << "\t.text" << std::endl;  // start of the code section
  for (auto& f : m_funcs) {
    f->exportASM(ofs);
  }

  // sylib generates additional outputs and we have to link it.
  // throw an arbitrary branch here. It should be unreachable.
  ofs << "\tB getint" << std::endl;
}

// Function part
void ASM_Function::exportASM(std::ofstream& ofs) {
  ofs << "\t.global " << m_name << std::endl;
  ofs << "\t.align 2" << std::endl;
  ofs << "\t.type " << m_name << ", \%function" << std::endl;
  ofs << m_name << ":" << std::endl;

  // TODO(Huang): push

  // TODO(Huang): stack allocate

  for (auto& b : m_blocks) {
    b->exportASM(ofs);
  }

  // TODO(Huang): stack reclaim

  // TODO(Huang): pop

  ofs << "\t.pool" << std::endl;
  ofs << "\t.size " << m_name << ", .-" << m_name << std::endl;
}

void ASM_BasicBlock::exportASM(std::ofstream& ofs) {
  ofs << m_label << ":" << std::endl;
  for (auto& i : m_insts) {
    i->exportInstHead(ofs);
    i->exportASM(ofs);
  }
}

void ASM_Instruction::exportInstHead(std::ofstream& ofs) {
  ofs << "\t" << getOpName() << " ";

  // TODO(Huang): export cond
}

void LDRInst::exportASM(std::ofstream& ofs) {
  ofs << m_dest->getName() << ", ";
  if (m_type == Type::LABEL) {
    ofs << "=" << m_label << std::endl;
  } else {
    ofs << "[" << m_src->getName() << ", " << m_offs->getName();
    if (m_shift) {
      // TODO(Huang): export shift here
    }
    ofs << "]" << std::endl;
  }
}

void STRInst::exportASM(std::ofstream& ofs) {
  ofs << m_src->getName() << ", [" << m_dest->getName() << ", "
      << m_offs->getName();
  if (m_shift) {
    // TODO(Huang): export shift here
  }
  ofs << "]" << std::endl;
}

void MOVInst::exportASM(std::ofstream& ofs) {
  ofs << m_dest->getName() << ", " << m_src->getName() << std::endl;
}

void PInst::exportASM(std::ofstream& ofs) {
  ofs << "{";
  for (int i = 0; i < m_regs.size(); i++) {
    ofs << m_regs[i]->getName();
    if (i != m_regs.size() - 1) ofs << ", ";
  }
  ofs << "}" << std::endl;
}

void BInst::exportASM(std::ofstream& ofs) {
  ofs << m_target->m_label << std::endl;
}

void CALLInst::exportASM(std::ofstream& ofs) { ofs << m_label << std::endl; }

void ShiftInst::exportASM(std::ofstream& ofs) {
  ofs << m_dest->getName() << ", " << m_src->getName() << ", "
      << m_sval->getName() << std::endl;
}

void ASInst::exportASM(std::ofstream& ofs) {
  ofs << m_dest->getName() << ", " << m_operand1->getName() << ", "
      << m_operand2->getName();
  if (m_shift) {
    // TODO(Huang): export shift here
  }
  ofs << std::endl;
}

void MULInst::exportASM(std::ofstream& ofs) {
  ofs << m_dest->getName() << ", " << m_operand1->getName() << ", "
      << m_operand2->getName();
  if (m_append) {
    ofs << ", " << m_append->getName();
  }
  ofs << std::endl;
}

void SDIVInst::exportASM(std::ofstream& ofs) {
  ofs << m_dest->getName() << ", " << m_devidend->getName() << ", "
      << m_devisor->getName() << std::endl;
}

void BITInst::exportASM(std::ofstream& ofs) {
  ofs << m_dest->getName() << ", " << m_operand1->getName();
  if (m_operand2) {
    ofs << ", " << m_operand2->getName();
  }
  if (m_shift) {
    // TODO(Huang): export shift here
  }
  ofs << std::endl;
}

void CTInst::exportASM(std::ofstream& ofs) {
  ofs << m_operand1->getName() << ", " << m_operand2->getName();
  if (m_shift) {
    // TODO(Huang): export shift here
  }
  ofs << std::endl;
}