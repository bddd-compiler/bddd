#include <iostream>
#include <stack>
#include <unordered_set>

#include "ir/ir-pass-manager.h"

void Mem2Reg(std::shared_ptr<Function> function,
             std::shared_ptr<IRBuilder> builder) {
  // prerequisites
  ComputeDominanceRelationship(function);
  ComputeDominanceFrontier(function);

  size_t alloca_cnt = 0;
  std::vector<std::shared_ptr<AllocaInstruction>> allocas;
  for (auto &bb : function->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      if (auto alloca_instr
          = std::dynamic_pointer_cast<AllocaInstruction>(instr)) {
        if (alloca_instr->m_type.Equals(BasicType::INT, true)
            || alloca_instr->m_type.Equals(BasicType::FLOAT, true)) {
          // single local variable (not an array)
          alloca_instr->m_alloca_id = alloca_cnt++;
          // alloca_instr->m_bb = bb;
          allocas.push_back(alloca_instr);
        } else {
          alloca_instr->m_alloca_id = -1;
        }
      }
    }
  }
  // std::cerr << "[debug] alloca_cnt: " << alloca_cnt << std::endl;

  // std::vector<std::vector<std::shared_ptr<BasicBlock>>> defs(allocas.size());
  for (auto &bb : function->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      if (auto store_instr
          = std::dynamic_pointer_cast<StoreInstruction>(instr)) {
        if (auto alloca_instr = std::dynamic_pointer_cast<AllocaInstruction>(
                store_instr->m_addr->m_value)) {
          if (alloca_instr->m_alloca_id >= 0) {
            alloca_instr->m_defs.push_back(bb);
          }
        }
      }
    }
  }

  // part1: insert phi functions
  std::unordered_map<std::shared_ptr<PhiInstruction>,
                     std::shared_ptr<AllocaInstruction>>
      phis;
  for (auto &alloca : allocas) {
    std::stack<std::shared_ptr<BasicBlock>> w;
    for (auto &bb : function->m_bb_list) {
      bb->m_visited = false;
    }
    for (auto &bb : alloca->m_defs) {
      w.push(bb);
    }
    while (!w.empty()) {
      std::shared_ptr<BasicBlock> x = w.top();  // x is basic block
      w.pop();
      for (auto &y : x->m_dominance_frontier) {  // y is basic block too
        if (!y->m_visited) {                     // y not in f
          y->m_visited = true;                   // add y to f
          auto phi_instr
              = builder->CreatePhiInstruction(alloca->m_type.Dereference(), y);
          phis[phi_instr] = alloca;
          if (std::find(alloca->m_defs.begin(), alloca->m_defs.end(), y)
              == alloca->m_defs.end()) {
            w.push(y);  // push when y not in alloca->m_defs
          }
        }
      }
    }
  }
  // std::cerr << "[debug] phis.size(): " << phis.size() << std::endl;

  // part 2: rename

  for (const auto &bb : function->m_bb_list) {
    bb->m_visited = false;
  }
  std::stack<std::pair<std::shared_ptr<BasicBlock>,
                       std::vector<std::shared_ptr<Value>>>>
      stack;
  stack.push(std::make_pair(
      function->m_bb_list.front(),
      std::vector<std::shared_ptr<Value>>(allocas.size(), nullptr)));
  while (!stack.empty()) {
    auto [bb, reaching_defs] = stack.top();
    stack.pop();
    if (bb->m_visited) continue;
    bb->m_visited = true;
    for (auto it = bb->m_instr_list.begin(); it != bb->m_instr_list.end();) {
      // manually ++it
      auto instr = *it;
      if (auto alloca = std::dynamic_pointer_cast<AllocaInstruction>(instr)) {
        if (alloca->m_alloca_id >= 0) {
          // alloca, just remove it
          auto del = it;
          ++it;
          bb->m_instr_list.erase(del);
        } else {
          ++it;
        }
      } else if (auto load_instr
                 = std::dynamic_pointer_cast<LoadInstruction>(instr)) {
        // load, replace it by its reaching def
        auto alloca = std::dynamic_pointer_cast<AllocaInstruction>(
            load_instr->m_addr->m_value);
        if (alloca != nullptr && alloca->m_alloca_id >= 0) {
          load_instr->ReplaceUseBy(reaching_defs[alloca->m_alloca_id]);
          auto del = it;
          ++it;
          bb->m_instr_list.erase(del);
          // load_instr->m_addr.reset();
        } else {
          ++it;
        }
      } else if (auto store_instr
                 = std::dynamic_pointer_cast<StoreInstruction>(instr)) {
        // store, update reaching def
        auto alloca = std::dynamic_pointer_cast<AllocaInstruction>(
            store_instr->m_addr->m_value);
        if (alloca != nullptr && alloca->m_alloca_id >= 0) {
          reaching_defs[alloca->m_alloca_id] = store_instr->m_val->m_value;
          auto del = it;
          ++it;
          bb->m_instr_list.erase(del);
          // store_instr->m_addr.reset();
        } else {
          ++it;
        }
      } else if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
        auto phi_it = phis.find(phi);
        if (phi_it != phis.end()) {
          assert(phi_it->second->m_alloca_id >= 0);
          reaching_defs[phi_it->second->m_alloca_id] = phi;
        }
        ++it;
      } else {
        ++it;
      }
    }

    for (auto suc : bb->Successors()) {
      assert(suc != nullptr);
      if (!suc->m_visited) {
        stack.push(std::make_pair(suc, reaching_defs));
      }
      for (auto instr : suc->m_instr_list) {
        if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
          auto phi_it = phis.find(phi);
          if (phi_it != phis.end()) {
            assert(phi_it->second->m_alloca_id >= 0);
            phi->AddPhiOperand(bb, reaching_defs[phi_it->second->m_alloca_id]);
          }
        } else {
          break;
        }
      }
    }
  }

  // simple checking
  for (auto bb : function->m_bb_list) {
    auto predecessors = bb->Predecessors();
    for (auto instr : bb->m_instr_list) {
      if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
        if (!phi->IsValid())
          throw MyException("existing uncovered predecessor block");
      } else {
        break;
      }
    }
  }
}

void IRPassManager::Mem2RegPass() {
  for (auto func : m_builder->m_module->m_function_list) {
    Mem2Reg(func, m_builder);
  }
}
