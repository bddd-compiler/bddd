//
// Created by garen on 7/26/22.
//

#include "ir/ir-pass-manager.h"

void TailRecursionOptimization(std::shared_ptr<Function> func) {
  std::vector<std::shared_ptr<PhiInstruction>> param_phis;
  auto old_entry = func->m_bb_list.front();
  for (auto& bb : func->m_bb_list) {
    if (bb->m_instr_list.size() >= 2) {
      // at least a call instruction and a ret instruction
      auto last_one_instr = bb->m_instr_list.back();
      auto last_two_instr = *std::next(bb->m_instr_list.rbegin());
      if (auto ret_instr
          = std::dynamic_pointer_cast<ReturnInstruction>(last_one_instr)) {
        if (auto call_instr
            = std::dynamic_pointer_cast<CallInstruction>(last_two_instr)) {
          if (call_instr->m_function == func
              && (((func->ReturnType() == VarType::INT
                    || func->ReturnType() == VarType::FLOAT)
                   && ret_instr->m_ret
                   && ret_instr->m_ret->getValue() == call_instr)
                  || (func->ReturnType() == VarType::VOID
                      && ret_instr->m_ret == nullptr))) {
            // 具体就是把原来的参数的使用换成了函数entry里面的phi nodes
            bool first
                = func->m_bb_list.front()->Name() != "tail_recursion_entry";
            if (first) {
              for (auto& param : func->m_args) {
                auto phi = std::make_shared<PhiInstruction>(param->m_type,
                                                            old_entry);
                old_entry->PushFrontInstruction(phi);
                param->ReplaceUseBy(phi);
                param_phis.push_back(phi);
              }

              auto new_entry
                  = std::make_shared<BasicBlock>("tail_recursion_entry");
              func->m_bb_list.push_front(new_entry);
              auto jump_instr
                  = std::make_shared<JumpInstruction>(old_entry, new_entry);
              new_entry->PushBackInstruction(jump_instr);
              old_entry->AddPredecessor(new_entry);

              assert(param_phis.size() == func->m_args.size());
              for (int i = 0; i < param_phis.size(); ++i) {
                param_phis[i]->AddPhiOperand(new_entry, func->m_args[i]);
                assert(!func->m_args[i]->m_use_list.empty());
              }
              auto new_jump_instr
                  = std::make_shared<JumpInstruction>(old_entry, bb);
              bb->PushBackInstruction(new_jump_instr);
              old_entry->AddPredecessor(bb);

              assert(call_instr->m_params.size() == param_phis.size());
              for (int i = 0; i < param_phis.size(); ++i) {
                param_phis[i]->AddPhiOperand(
                    bb, call_instr->m_params[i]->getValue());
              }
              bb->RemoveInstruction(ret_instr);
              bb->RemoveInstruction(call_instr);
            } else {
              assert(func->m_args.size() == param_phis.size());
              auto new_jump_instr
                  = std::make_shared<JumpInstruction>(old_entry, bb);
              bb->PushBackInstruction(new_jump_instr);
              old_entry->AddPredecessor(bb);

              assert(call_instr->m_params.size() == param_phis.size());
              for (int i = 0; i < param_phis.size(); ++i) {
                param_phis[i]->AddPhiOperand(
                    bb, call_instr->m_params[i]->getValue());
              }
              bb->RemoveInstruction(ret_instr);
              bb->RemoveInstruction(call_instr);
            }
          }
        }
      }
    }
  }
}

void IRPassManager::TailRecursionPass() {
  for (auto& func : m_builder->m_module->m_function_list) {
    TailRecursionOptimization(func);
  }
}