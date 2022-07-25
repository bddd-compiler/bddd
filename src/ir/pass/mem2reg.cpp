#include <iostream>
#include <stack>
#include <unordered_set>

#include "ir/ir-pass-manager.h"

void updateReachingDefinition(std::shared_ptr<Value> v,
                              std::shared_ptr<Value> instr) {
  // auto v = load_instr->m_addr->m_value;
  auto r = v->m_reaching_def;
  while (!(r == nullptr || r->m_bb == nullptr
           || std::find_if(
                  r->m_bb->m_dominators.begin(), r->m_bb->m_dominators.end(),
                  [=](const auto &x) { return x.get() == instr->m_bb.get(); })
                  != r->m_bb->m_dominators.end())) {
    r = r->m_reaching_def;
  }
  v->m_reaching_def = r;
}

void Mem2Reg(std::shared_ptr<Function> function,
             std::shared_ptr<IRBuilder> builder) {
  size_t alloca_cnt = 0;
  std::vector<std::shared_ptr<AllocaInstruction>> allocas;
  for (auto &bb : function->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      instr->m_reaching_def = nullptr;
      if (auto alloca_instr
          = std::dynamic_pointer_cast<AllocaInstruction>(instr)) {
        if (alloca_instr->m_type.m_dimensions.empty()) {
          // single local variable (not an array)
          alloca_instr->m_alloca_id = ++alloca_cnt;
          // alloca_instr->m_bb = bb;
          allocas.push_back(alloca_instr);
        }
      }
    }
  }

  // std::vector<std::vector<std::shared_ptr<BasicBlock>>> defs(allocas.size());
  for (auto &bb : function->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      if (auto store_instr
          = std::dynamic_pointer_cast<StoreInstruction>(instr)) {
        if (auto alloca_instr = std::dynamic_pointer_cast<AllocaInstruction>(
                store_instr->m_addr->m_value)) {
          if (alloca_instr->m_alloca_id != 0) {
            alloca_instr->m_defs.push_back(bb);
          }
        }
      }
    }
  }

  // part1: insert phi functions
  ComputeDominanceRelationship(function);
  ComputeDominanceFrontier(function);

  std::unordered_map<std::shared_ptr<PhiInstruction>,
                     std::shared_ptr<AllocaInstruction>>
      phis;
  for (auto &alloca : allocas) {
    std::unordered_set<std::shared_ptr<BasicBlock>> f;
    std::stack<std::shared_ptr<BasicBlock>> w;
    for (auto &bb : alloca->m_defs) {
      w.push(bb);
    }
    while (!w.empty()) {
      std::shared_ptr<BasicBlock> x = w.top();  // x is basic block
      w.pop();
      for (auto &y : x->m_dominance_frontier) {  // y is basic block too
        if (std::find_if(f.begin(), f.end(),
                         [=](auto x) { return x.get() == y.get(); })
            == f.end()) {
          auto phi_instr
              = builder->CreatePhiInstruction(alloca->m_type.Dereference(), y);
          phis[phi_instr] = alloca;
          f.insert(y);
          if (std::find_if(alloca->m_defs.begin(), alloca->m_defs.end(),
                           [=](auto x) { return x.get() == y.get(); })
              == alloca->m_defs.end()) {
            w.push(y);
          }
        }
      }
    }
  }

  // part 2: rename

  for (const auto &bb : function->m_bb_list) {
    bb->m_visited = false;
  }
  std::stack<std::shared_ptr<BasicBlock>> stack;
  stack.push(function->m_bb_list.front());
  while (!stack.empty()) {
    std::shared_ptr<BasicBlock> bb = stack.top();
    stack.pop();
    if (bb->m_visited) continue;
    bb->m_visited = true;
    // std::cerr << "idx: " << bb->m_id << std::endl;
    // reassign loop_it before deletion
    for (auto loop_it = bb->m_instr_list.begin();
         loop_it != bb->m_instr_list.end();) {
      auto &instr = *loop_it;
      auto it = std::find_if(allocas.begin(), allocas.end(),
                             [=](auto x) { return x.get() == instr.get(); });
      if (it != allocas.end()) {
        ++loop_it;
        bb->RemoveInstruction(instr);
        // instr not in bb->m_instr_list now, so must continue immediately
        continue;
      }

      // for variables used by non-phi instructions
      if (auto load_instr = std::dynamic_pointer_cast<LoadInstruction>(instr)) {
        it = std::find_if(allocas.begin(), allocas.end(), [=](auto x) {
          return x.get() == load_instr->m_addr->m_value.get();
        });
        if (it != allocas.end()) {
          // load_instr->m_addr->m_value->m_reaching_def may not dominate the
          // current load instruction
          updateReachingDefinition(load_instr->m_addr->m_value, load_instr);

          load_instr->ReplaceUseBy(load_instr->m_addr->m_value->m_reaching_def);
          ++loop_it;
          bb->RemoveInstruction(instr);
          continue;  // similarly
        }
      }

      // for variables defined by i

      auto store_instr = std::dynamic_pointer_cast<StoreInstruction>(instr);
      if (store_instr != nullptr) {
        it = std::find_if(allocas.begin(), allocas.end(), [=](auto x) {
          return x.get() == store_instr->m_addr->m_value.get();
        });
        if (it != allocas.end()) {
          // simply update
          store_instr->m_val->m_value->m_reaching_def
              = store_instr->m_addr->m_value->m_reaching_def;
          store_instr->m_addr->m_value->m_reaching_def
              = store_instr->m_val->m_value;
          ++loop_it;
          bb->RemoveInstruction(instr);
          continue;
        }
      }

      auto phi_instr = std::dynamic_pointer_cast<PhiInstruction>(instr);
      if (phi_instr != nullptr) {
        for (auto &phi : phis) {
          if (phi.first.get() == phi_instr.get()) {
            // simply update
            auto &alloca = phi.second;
            phi_instr->m_reaching_def = alloca->m_reaching_def;
            alloca->m_reaching_def = phi_instr;
          }
        }
      }
      ++loop_it;
    }

    auto successors = bb->Successors();
    for (auto it = successors.rbegin(); it != successors.rend(); ++it) {
      stack.push(*it);
    }
    for (auto &x : successors) {
      for (auto &instr : x->m_instr_list) {
        if (auto phi_instr = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
          for (auto &phi : phis) {
            if (phi.first.get() == phi_instr.get()) {
              if (phi.second->m_reaching_def != nullptr) {
                std::shared_ptr<Value> r = phi.second->m_reaching_def;
                while (r->m_bb
                       && std::find_if(
                              bb->m_dominated.begin(), bb->m_dominated.end(),
                              [=](const auto &lhs) { return lhs == r->m_bb; })
                              == bb->m_dominated.end()) {
                  r = r->m_reaching_def;
                }
                phi.second->m_reaching_def = r;

                phi_instr->AddPhiOperand(bb, phi.second->m_reaching_def);
              }
            }
          }
        } else {
          break;  // not phi instructions after now
        }
      }
    }
  }

  // remove unused or trivial phis (replace use by its unique incoming value)
  // std::vector<
  //     std::pair<std::shared_ptr<PhiInstruction>,
  //     std::shared_ptr<BasicBlock>>> trivial_phis;
  for (auto &bb : function->m_bb_list) {
    for (auto it = bb->m_instr_list.begin(); it != bb->m_instr_list.end();) {
      auto instr = *it;
      if (auto phi_instr = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
        // std::cerr << "size: " << phi_instr->m_use_list.size()
        //           << ", # operands: " << phi_instr->m_contents.size()
        //           << std::endl;
        if (phi_instr->m_use_list.empty()) {
          ++it;
          bb->RemoveInstruction(phi_instr);
        } else {
          ++it;
        }
      } else {
        break;
      }
    }
  }

  // for (auto &[trivial_phi, bb] : trivial_phis) {
  //   bb->RemoveInstruction(trivial_phi);
  // }
}

void IRPassManager::Mem2RegPass() {
  for (auto func : m_builder->m_module->m_function_list) {
    Mem2Reg(func, m_builder);
  }
}
