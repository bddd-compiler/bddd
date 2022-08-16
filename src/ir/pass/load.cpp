//
// Created by garen on 8/15/22.
//

#include "ir/ir-pass-manager.h"

void LoadStoreOptimization(std::shared_ptr<BasicBlock> bb) {
  std::map<std::shared_ptr<Value>, std::shared_ptr<Value>> addr_to_val;
  std::map<std::shared_ptr<Value>, std::shared_ptr<Value>> init_addr_to_val;

  auto GetValFromAddr
      = [&addr_to_val](std::shared_ptr<Value> addr) -> std::shared_ptr<Value> {
    if (auto addr_gep
        = std::dynamic_pointer_cast<GetElementPtrInstruction>(addr)) {
      for (auto &[mp_addr, mp_val] : addr_to_val) {
        if (auto mp_addr_gep
            = std::dynamic_pointer_cast<GetElementPtrInstruction>(mp_addr)) {
          if (addr_gep == mp_addr_gep) {
            return mp_val;
          }
        }
      }
      return nullptr;
    } else {
      auto it = addr_to_val.find(addr);
      if (it != addr_to_val.end()) {
        return it->second;
      } else {
        return nullptr;
      }
    }
  };

  auto Assign = [](std::map<std::shared_ptr<Value>, std::shared_ptr<Value>> &mp,
                   std::shared_ptr<Value> addr, std::shared_ptr<Value> val) {
    // addr can be global singles or GEPs
    if (auto addr_gep
        = std::dynamic_pointer_cast<GetElementPtrInstruction>(addr)) {
      for (auto &[mp_addr, mp_val] : mp) {
        if (auto mp_addr_gep
            = std::dynamic_pointer_cast<GetElementPtrInstruction>(mp_addr)) {
          if (addr_gep == mp_addr_gep) {
            mp.erase(mp_addr);
            break;
          }
        }
      }
      mp[addr] = val;
    } else {
      mp[addr] = val;
    }
  };

  auto ClearAlias = [&addr_to_val](std::shared_ptr<Value> addr) {
    if (auto gep = std::dynamic_pointer_cast<GetElementPtrInstruction>(addr)) {
      for (auto it = addr_to_val.begin(); it != addr_to_val.end();) {
        auto nxt = std::next(it);  // manually ++it
        if (auto map_gep
            = std::dynamic_pointer_cast<GetElementPtrInstruction>(it->first)) {
          if (gep->m_addr->getValue() != map_gep->m_addr->getValue()) {
            if (gep->m_indices.size() == map_gep->m_indices.size()) {
              addr_to_val.erase(it);
            }
          }
        }
        it = nxt;
      }
    }
  };

  if (bb->Predecessors().size() == 1) {
    assert(bb->m_idom == *bb->Predecessors().begin());
    for (auto &instr : bb->m_idom->m_instr_list) {
      if (auto load = std::dynamic_pointer_cast<LoadInstruction>(instr)) {
        Assign(init_addr_to_val, load->m_addr->getValue(), load);
        // init_addr_to_val[load->m_addr->getValue()] = load;
      } else if (auto store
                 = std::dynamic_pointer_cast<StoreInstruction>(instr)) {
        Assign(init_addr_to_val, store->m_addr->getValue(),
               store->m_val->getValue());
        // init_addr_to_val[store->m_addr->getValue()] =
        // store->m_val->getValue();
        ClearAlias(store->m_addr->getValue());
      } else if (instr->m_op == IROp::CALL) {
        init_addr_to_val.clear();
      }
    }
  }

  int cnt = 0;
  while (true) {
    bool changed = false;
    addr_to_val = init_addr_to_val;
    for (auto it = bb->m_instr_list.begin(); it != bb->m_instr_list.end();) {
      // manually ++it
      auto instr = *it;
      if (auto load = std::dynamic_pointer_cast<LoadInstruction>(instr)) {
        auto load_val = GetValFromAddr(load->m_addr->getValue());
        Assign(addr_to_val, load->m_addr->getValue(), load_val);
        // addr_to_val[load->m_addr->getValue()] = load_val;
        if (load_val != nullptr) {
          changed = true;
          cnt++;
          instr->ReplaceUseBy(load_val);
          auto del = it;
          ++it;
          bb->RemoveInstruction(del);
        } else {
          ++it;
        }
      } else if (auto store
                 = std::dynamic_pointer_cast<StoreInstruction>(instr)) {
        Assign(addr_to_val, store->m_addr->getValue(),
               store->m_val->getValue());
        // addr_to_val[store->m_addr->getValue()] = store->m_val->getValue();
        ClearAlias(store->m_addr->getValue());
        ++it;
      } else if (instr->m_op == IROp::CALL) {
        addr_to_val.clear();
        ++it;
      } else {
        ++it;
      }
    }
    if (!changed) break;
  }
  std::cerr << "[debug] load opt: " << cnt << std::endl;
}

void IRPassManager::LoadStoreOptimizationPass() {
  SideEffectPass();
  for (auto &func : m_builder->m_module->m_function_list) {
    ComputeDominanceRelationship(func);
    for (auto &bb : func->m_bb_list) {
      LoadStoreOptimization(bb);
    }
  }
}