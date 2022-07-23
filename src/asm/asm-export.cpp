#include "asm/asm.h"

// Module part
void ASM_Module::exportGlobalVar(std::ofstream& ofs) {
  for (auto& var : m_ir_module->m_global_variable_list) {
    ofs << "\t.data" << std::endl;
    ofs << "\t.global " << var->m_name << std::endl;
    ofs << "\t.align 2" << std::endl;
    ofs << "\t.type " << var->m_name << ", \%object" << std::endl;
    ofs << "\t.size " << var->m_name << ", ";
    if (var->m_is_float)
      exportVarBody(ofs, std::dynamic_pointer_cast<FloatGlobalVariable>(var));
    else
      exportVarBody(ofs, std::dynamic_pointer_cast<IntGlobalVariable>(var));

#if 0
    if (var->m_is_float) {
      auto float_val = std::dynamic_pointer_cast<FloatGlobalVariable>(var);
      int init_size = float_val->m_init_vals.size();
      ofs << std::to_string(init_size * 4) << std::endl;
      ofs << var->m_name << ":" << std::endl;
      int i = 0;
      while (i < init_size) {
        if (float_val->m_init_vals[i] == 0) {
          ofs << "\t.space ";
          int left = i;
          while (i < init_size && float_val->m_init_vals[i] == 0) {
            i++;
          }
          ofs << std::to_string((i - left) * 4) << std::endl;
        } else {
          ofs << "\t.float ";
          int first = true;
          while (i < init_size && float_val->m_init_vals[i] != 0) {
            if (!first) {
              first = false;
            } else {
              ofs << ",";
            }
            ofs << std::to_string(float_val->m_init_vals[i]);
            i++;
          }
          ofs << std::endl;
        }
      }
    } else {
      auto int_val = std::dynamic_pointer_cast<IntGlobalVariable>(var);
      int init_size = int_val->m_init_vals.size();
      ofs << std::to_string(init_size * 4) << std::endl;
      ofs << var->m_name << ":" << std::endl;
      int i = 0;
      while (i < init_size) {
        if (int_val->m_init_vals[i] == 0) {
          ofs << "\t.space ";
          int left = i;
          while (i < init_size && int_val->m_init_vals[i] == 0) {
            i++;
          }
          ofs << std::to_string((i - left) * 4) << std::endl;
        } else {
          ofs << "\t.word ";
          bool first = true;
          while (i < init_size && int_val->m_init_vals[i] != 0) {
            if (first) {
              first = false;
            } else {
              ofs << ",";
            }
            ofs << std::to_string(int_val->m_init_vals[i]);
            i++;
          }
          ofs << std::endl;
        }
      }
    }
#endif
  }
}

template <class T> void ASM_Module::exportVarBody(std::ofstream& ofs,
                                                  std::shared_ptr<T> init_val) {
  assert(std::static_pointer_cast<GlobalVariable>(init_val));
  int size = init_val->m_init_vals.size();
  ofs << std::to_string(size * 4) << std::endl;
  ofs << init_val->m_name << ":" << std::endl;
  int i = 0;
  while (i < size) {
    if (init_val->m_init_vals[i] == 0) {
      ofs << "\t.space ";
      int left = i;
      while (i < size && init_val->m_init_vals[i] == 0) {
        i++;
      }
      ofs << std::to_string((i - left) * 4) << std::endl;
    } else {
      ofs << (init_val->m_is_float ? "\t.float " : "\t.word ");
      int first = true;
      while (i < size && init_val->m_init_vals[i] != 0) {
        if (!first) {
          ofs << ",";
        } else {
          first = false;
        }
        ofs << std::to_string(init_val->m_init_vals[i]);
        i++;
      }
      ofs << std::endl;
    }
  }
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
  m_push->exportASM(ofs);

#ifndef SP_FOR_PARAM
  if (m_params_set_list.size()) {
    int sp_offs = m_push->m_regs.size() * 4;
    if (sp_offs)
      ofs << "\tADD FP, SP, #" << std::to_string(sp_offs) << std::endl;
    else
      ofs << "\tMOV FP, SP" << std::endl;
  }
#endif

  // load params
  for (auto& i : m_params_set_list) {
    i->exportASM(ofs);
  }

  // allocate stack
  int size = m_local_alloc;
  if (size) {
    if (size & 7) {
      size &= ~(unsigned int)7;
      size += 8;
    }

    // this part is taken from tinbaccc directly
    if (Operand::immCheck(size)) {
      ofs << "\tSUB SP, SP, #" << std::to_string(size) << std::endl;
    } else {
      ofs << "\tMOV r12, #" << (size & 0xffff) << std::endl;
      if (size & 0xffff0000)
        ofs << "\tMOVT r12, #" << ((unsigned int)size >> 16) << std::endl;
      ofs << "\tSUB sp, sp, r12" << std::endl;
    }
  }

  for (auto& b : m_blocks) {
    b->exportASM(ofs);
  }

  if (size) {
    // this part is taken from tinbaccc directly
    if (Operand::immCheck(size)) {
      ofs << "\tADD SP, SP, #" << std::to_string(size) << std::endl;
    } else {
      ofs << "\tLDR R12, =" << std::to_string(size) << std::endl;
      ofs << "\tADD sp, sp, r12" << std::endl;
    }
  }

  m_pop->exportASM(ofs);
  ofs << "\t.pool" << std::endl;
  ofs << "\t.size " << m_name << ", .-" << m_name << std::endl;
}

void ASM_BasicBlock::exportASM(std::ofstream& ofs) {
  ofs << m_label << ":" << std::endl;
  for (auto& i : m_insts) {
    i->exportASM(ofs);
  }
}

void Shift::exportASM(std::ofstream& ofs) {
  ofs << getShiftName() << " #" << std::to_string(s_val);
}

void ASM_Instruction::exportInstHead(std::ofstream& ofs) {
  ofs << "\t" << getOpName() << getCondName() << getOpSuffixName() << " ";
}

void LDRInst::exportASM(std::ofstream& ofs) {
  exportInstHead(ofs);
  ofs << m_dest->getName() << ", ";
  if (m_type == Type::LABEL) {
    ofs << "=" << m_label << std::endl;
  } else {
    ofs << "[" << m_src->getName() << ", " << m_offs->getName();
    if (m_shift) {
      ofs << ", ";
      m_shift->exportASM(ofs);
    }
    ofs << "]" << std::endl;
  }
}

void STRInst::exportASM(std::ofstream& ofs) {
  exportInstHead(ofs);
  ofs << m_src->getName() << ", [" << m_dest->getName() << ", "
      << m_offs->getName();
  if (m_shift) {
    ofs << ", ";
    m_shift->exportASM(ofs);
  }
  ofs << "]" << std::endl;
}

void MOVInst::exportASM(std::ofstream& ofs) {
  if (m_src->m_op_type == OperandType::IMM) {
    int imm = m_src->m_immval;
    if (!Operand::immCheck(imm)) {
      if (Operand::immCheck(~imm)) {
        ofs << "\tMVN" << getCondName() << " " << m_dest->getName() << ", #"
            << std::to_string(~imm) << std::endl;
      } else {
        ofs << "\tMOVW" << getCondName() << " " << m_dest->getName() << ", #"
            << std::to_string(imm & 65535) << std::endl;
        ofs << "\tMOVT" << getCondName() << " " << m_dest->getName() << ", #"
            << std::to_string((unsigned int)imm >> 16) << std::endl;
      }
      return;
    }
  }
  exportInstHead(ofs);
  ofs << m_dest->getName() << ", " << m_src->getName() << std::endl;
}

void PInst::exportASM(std::ofstream& ofs) {
  exportInstHead(ofs);
  ofs << "{";
  for (int i = 0; i < m_regs.size(); i++) {
    ofs << m_regs[i]->getName() << ", ";
  }
  if (m_op == InstOp::PUSH)
    ofs << "LR";
  else
    ofs << "PC";
  ofs << "}" << std::endl;
}

void BInst::exportASM(std::ofstream& ofs) {
  exportInstHead(ofs);
  ofs << m_target->m_label << std::endl;
}

void CALLInst::exportASM(std::ofstream& ofs) {
  exportInstHead(ofs);
  ofs << m_label << std::endl;
}

void ShiftInst::exportASM(std::ofstream& ofs) {
  exportInstHead(ofs);
  ofs << m_dest->getName() << ", " << m_src->getName() << ", "
      << m_sval->getName() << std::endl;
}

void ASInst::exportASM(std::ofstream& ofs) {
  exportInstHead(ofs);
  ofs << m_dest->getName() << ", " << m_operand1->getName() << ", "
      << m_operand2->getName();
  if (m_shift) {
    ofs << ", ";
    m_shift->exportASM(ofs);
  }
  ofs << std::endl;
}

void MULInst::exportASM(std::ofstream& ofs) {
  exportInstHead(ofs);
  ofs << m_dest->getName() << ", " << m_operand1->getName() << ", "
      << m_operand2->getName();
  if (m_append) {
    ofs << ", " << m_append->getName();
  }
  ofs << std::endl;
}

void SDIVInst::exportASM(std::ofstream& ofs) {
  exportInstHead(ofs);
  ofs << m_dest->getName() << ", " << m_devidend->getName() << ", "
      << m_devisor->getName() << std::endl;
}

void BITInst::exportASM(std::ofstream& ofs) {
  exportInstHead(ofs);
  ofs << m_dest->getName() << ", " << m_operand1->getName();
  if (m_operand2) {
    ofs << ", " << m_operand2->getName();
  }
  if (m_shift) {
    ofs << ", ";
    m_shift->exportASM(ofs);
  }
  ofs << std::endl;
}

void CTInst::exportASM(std::ofstream& ofs) {
  exportInstHead(ofs);
  ofs << m_operand1->getName() << ", " << m_operand2->getName();
  if (m_shift) {
    ofs << ", ";
    m_shift->exportASM(ofs);
  }
  ofs << std::endl;
}