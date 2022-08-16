//
// Created by garen on 7/21/22.
//
#include "ir/ir-pass-manager.h"
#include "ir/ir.h"

bool FunctionInlining(std::shared_ptr<Function> inline_func) {
  // first identify whether an inline_func can be inlined
  if (inline_func->FuncName() == "main") return false;  // obviously
  int cnt = 0;
  for (auto &bb : inline_func->m_bb_list) {
    for (auto &instr : bb->m_instr_list) {
      ++cnt;
      if (auto call_instr = std::dynamic_pointer_cast<CallInstruction>(instr)) {
        if (call_instr->m_function == inline_func)
          return false;  // recursive inline_func cannot be inlined
      }
    }
  }
  if (inline_func->ReturnType() == VarType::FLOAT) return false;
  for (auto &arg : inline_func->m_args) {
    if (arg->m_type.m_base_type == BasicType::FLOAT) return false;
  }
  auto func_name = inline_func->FuncName();
  // std::cerr << "[debug] # of instrs: " << cnt << std::endl;
  if (cnt >= 50)
    return false;  // inlined inline_func cannot have too many instructions

  std::cerr << "[debug] inlining function " << inline_func->FuncName()
            << std::endl;

  for (auto &[call, original_func] : inline_func->m_calls) {
    // now start inlining an inline_func into call's basic block
    auto original_bb = call->m_bb;
    auto call_it = std::find(original_bb->m_instr_list.begin(),
                             original_bb->m_instr_list.end(), call);

    // create return bb (may have phi for different incoming return values)
    auto return_bb = std::make_shared<BasicBlock>("func-inline.ret");

    // move all instrs after call instr into return_bb
    assert(return_bb->m_instr_list.empty());
    return_bb->m_instr_list.insert(return_bb->m_instr_list.end(),
                                   std::next(call_it),
                                   original_bb->m_instr_list.end());
    return_bb->m_instr_list.erase(std::next(call_it),
                                  original_bb->m_instr_list.end());
    for (auto &moved_instr : return_bb->m_instr_list) {
      moved_instr->m_bb = return_bb;  // the owning basic block is changed
    }
    // insert return_bb right after original_bb
    auto original_bb_it
        = std::find(original_func->m_bb_list.begin(),
                    original_func->m_bb_list.end(), original_bb);
    assert(original_bb_it != original_func->m_bb_list.end());
    auto return_bb_it
        = original_func->m_bb_list.insert(std::next(original_bb_it), return_bb);

    // create basic blocks according to the inline_func (but initially empty)
    std::map<std::shared_ptr<BasicBlock>, std::shared_ptr<BasicBlock>> new_bbs;
    for (auto &bb : inline_func->m_bb_list) {
      auto new_bb = std::make_shared<BasicBlock>(bb->Name() + "(inlined)");
      new_bbs[bb] = new_bb;
      original_func->m_bb_list.insert(return_bb_it, new_bb);
    }

    // create a jump from original_bb to the corresponding entry block
    auto new_entry = new_bbs[inline_func->m_bb_list.front()];
    auto jump_to_new_entry
        = std::make_shared<JumpInstruction>(new_entry, original_bb);
    original_bb->PushBackInstruction(jump_to_new_entry);
    new_entry->AddPredecessor(original_bb);

    std::map<std::shared_ptr<Value>, std::shared_ptr<Value>> new_vals;
    auto GetNewVal =
        [&new_vals](std::shared_ptr<Value> old_val) -> std::shared_ptr<Value> {
      if (old_val == nullptr) return nullptr;
      if (std::dynamic_pointer_cast<Constant>(old_val)
          || std::dynamic_pointer_cast<GlobalVariable>(old_val)) {
        return old_val;
      }
      auto it = new_vals.find(old_val);
      assert(it != new_vals.end());
      return it->second;
    };
    assert(inline_func->m_args.size() == call->m_params.size());
    auto sz = inline_func->m_args.size();
    for (int i = 0; i < sz; ++i) {
      new_vals[inline_func->m_args[i]] = call->m_params[i]->getValue();
      // 不急，后面删掉function的时候再维护use-def和def-use
    }
    // trivially copy instructions
    std::vector<std::shared_ptr<PhiInstruction>> old_phis;
    std::vector<std::pair<std::shared_ptr<BasicBlock>, std::shared_ptr<Value>>>
        new_rets;
    for (auto &func_bb : inline_func->m_bb_list) {
      assert(new_bbs.find(func_bb) != new_bbs.end());
      auto new_bb = new_bbs[func_bb];
      for (auto &instr : func_bb->m_instr_list) {
        if (auto alloca = std::dynamic_pointer_cast<AllocaInstruction>(instr)) {
          auto new_alloca = std::make_shared<AllocaInstruction>(
              alloca->m_type, new_bb, false, alloca->m_is_const);
          new_bb->PushBackInstruction(new_alloca);
          new_vals[alloca] = new_alloca;
        } else if (auto binary
                   = std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
          auto new_binary
              = std::make_shared<BinaryInstruction>(binary->m_op, new_bb);
          new_binary->m_lhs_val_use
              = GetNewVal(binary->m_lhs_val_use->getValue())
                    ->AddUse(new_binary);
          new_binary->m_rhs_val_use
              = GetNewVal(binary->m_rhs_val_use->getValue())
                    ->AddUse(new_binary);
          new_bb->PushBackInstruction(new_binary);
          new_vals[binary] = new_binary;
        } else if (auto bit_cast
                   = std::dynamic_pointer_cast<BitCastInstruction>(instr)) {
          auto new_bit_cast = std::make_shared<BitCastInstruction>(
              bit_cast->m_target_type, new_bb);
          new_bit_cast->m_val
              = GetNewVal(bit_cast->m_val->getValue())->AddUse(new_bit_cast);
          new_bb->PushBackInstruction(new_bit_cast);
          new_vals[bit_cast] = new_bit_cast;
        } else if (auto zext
                   = std::dynamic_pointer_cast<ZExtInstruction>(instr)) {
          auto new_zext = std::make_shared<ZExtInstruction>(new_bb);
          new_zext->m_val
              = GetNewVal(zext->m_val->getValue())->AddUse(new_zext);
          new_bb->PushBackInstruction(new_zext);
          new_vals[zext] = new_zext;
        } else if (auto sitofp
                   = std::dynamic_pointer_cast<SIToFPInstruction>(instr)) {
          auto new_sitofp = std::make_shared<SIToFPInstruction>(new_bb);
          new_sitofp->m_val
              = GetNewVal(sitofp->m_val->getValue())->AddUse(new_sitofp);
          new_bb->PushBackInstruction(new_sitofp);
          new_vals[sitofp] = new_sitofp;
        } else if (auto fptosi
                   = std::dynamic_pointer_cast<FPToSIInstruction>(instr)) {
          auto new_fptosi = std::make_shared<FPToSIInstruction>(new_bb);
          new_fptosi->m_val
              = GetNewVal(fptosi->m_val->getValue())->AddUse(new_fptosi);
          new_bb->PushBackInstruction(new_fptosi);
          new_vals[fptosi] = new_fptosi;
        } else if (auto ret
                   = std::dynamic_pointer_cast<ReturnInstruction>(instr)) {
          auto jmp = std::make_shared<JumpInstruction>(return_bb, new_bb);
          new_bb->PushBackInstruction(jmp);
          new_vals[ret] = jmp;
          return_bb->AddPredecessor(new_bb);
          if (inline_func->ReturnType() != VarType::VOID) {
            assert(ret != nullptr);
            new_rets.emplace_back(new_bb, GetNewVal(ret->m_ret->getValue()));
          }
        } else if (auto fneg
                   = std::dynamic_pointer_cast<FNegInstruction>(instr)) {
          auto new_fneg = std::make_shared<FNegInstruction>(new_bb);
          new_fneg->m_lhs_val_use
              = GetNewVal(fneg->m_lhs_val_use->getValue())->AddUse(new_fneg);
          new_bb->PushBackInstruction(new_fneg);
          new_vals[fneg] = new_fneg;
        } else if (auto call_instr
                   = std::dynamic_pointer_cast<CallInstruction>(instr)) {
          auto new_call_instr = std::make_shared<CallInstruction>(
              call_instr->m_type.m_base_type, call_instr->m_func_name,
              call_instr->m_function, new_bb);
          std::vector<Use *> new_params_uses;
          for (auto use : call_instr->m_params) {
            new_params_uses.push_back(
                GetNewVal(use->getValue())->AddUse(new_call_instr));
          }
          new_call_instr->SetParams(std::move(new_params_uses));
          new_bb->PushBackInstruction(new_call_instr);
          new_vals[call_instr] = new_call_instr;
        } else if (auto jmp
                   = std::dynamic_pointer_cast<JumpInstruction>(instr)) {
          auto new_jmp = std::make_shared<JumpInstruction>(
              new_bbs[jmp->m_target_block], new_bb);
          new_bb->PushBackInstruction(new_jmp);
          new_vals[jmp] = new_jmp;
          new_bbs[jmp->m_target_block]->AddPredecessor(new_bb);
        } else if (auto br
                   = std::dynamic_pointer_cast<BranchInstruction>(instr)) {
          auto new_br = std::make_shared<BranchInstruction>(
              new_bbs[br->m_true_block], new_bbs[br->m_false_block], new_bb);
          new_br->m_cond = GetNewVal(br->m_cond->getValue())->AddUse(new_br);
          new_bb->PushBackInstruction(new_br);
          new_vals[br] = new_br;
          new_bbs[br->m_true_block]->AddPredecessor(new_bb);
          new_bbs[br->m_false_block]->AddPredecessor(new_bb);
        } else if (auto store
                   = std::dynamic_pointer_cast<StoreInstruction>(instr)) {
          auto new_store = std::make_shared<StoreInstruction>(new_bb);
          new_store->m_addr
              = GetNewVal(store->m_addr->getValue())->AddUse(new_store);
          new_store->m_val
              = GetNewVal(store->m_val->getValue())->AddUse(new_store);
          new_bb->PushBackInstruction(new_store);
          new_vals[store] = new_store;
        } else if (auto load
                   = std::dynamic_pointer_cast<LoadInstruction>(instr)) {
          auto new_load = std::make_shared<LoadInstruction>(new_bb);
          new_load->m_addr
              = GetNewVal(load->m_addr->getValue())->AddUse(new_load);
          new_load->m_type = load->m_type;
          new_bb->PushBackInstruction(new_load);
          new_vals[load] = new_load;
        } else if (auto gep
                   = std::dynamic_pointer_cast<GetElementPtrInstruction>(
                       instr)) {
          auto new_gep = std::make_shared<GetElementPtrInstruction>(new_bb);
          new_gep->m_addr = GetNewVal(gep->m_addr->getValue())->AddUse(new_gep);
          for (auto use : gep->m_indices) {
            new_gep->m_indices.push_back(
                GetNewVal(use->getValue())->AddUse(new_gep));
          }
          new_gep->m_type = gep->m_type;
          new_bb->PushBackInstruction(new_gep);
          new_vals[gep] = new_gep;
        } else if (auto phi
                   = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
          // 哎哟别急，先弄个空phi指令占位，后面会补上的
          old_phis.push_back(phi);
          auto new_phi = std::make_shared<PhiInstruction>(phi->m_type, new_bb);
          new_bb->PushBackInstruction(new_phi);
          new_vals[phi] = new_phi;
        } else {
          assert(false);  // unimplemented
        }
      }
    }
    for (auto &old_phi : old_phis) {
      auto new_phi
          = std::dynamic_pointer_cast<PhiInstruction>(new_vals[old_phi]);
      for (auto [incoming_bb, use] : old_phi->m_contents) {
        auto val = use != nullptr ? use->getValue() : nullptr;
        new_phi->AddPhiOperand(new_bbs[incoming_bb], GetNewVal(val));
      }
    }
    // add phi node for return value if return type is int or float
    if (inline_func->ReturnType() != VarType::VOID) {
      if (inline_func->ReturnType() == VarType::INT) {
        for (auto &[incoming_bb, ret_val] : new_rets) {
          assert(ret_val->m_type.IsBasicInt());
        }
      } else {
        assert(inline_func->ReturnType() == VarType::FLOAT);
        for (auto &[incoming_bb, ret_val] : new_rets) {
          assert(ret_val->m_type.IsBasicFloat());
        }
      }
      auto new_phi = std::make_shared<PhiInstruction>(
          ValueType(inline_func->ReturnType()), return_bb);
      for (auto &[incoming_bb, ret_val] : new_rets) {
        new_phi->AddPhiOperand(incoming_bb, ret_val);
      }
      return_bb->PushFrontInstruction(new_phi);
      call->ReplaceUseBy(new_phi);
    }
    for (auto &s : return_bb->Successors()) {
      s->ReplacePredecessorsBy(original_bb, {return_bb});
    }

    // at last, remember to remove the call instruction itself from m_instr_list
    assert(call->m_use_list.empty());
    original_bb->RemoveInstruction(call_it);
  }
  return true;
}

void IRPassManager::FunctionInliningPass() {
  SideEffectPass();
  std::vector<std::shared_ptr<Function>> funcs;
  for (auto &func : m_builder->m_module->m_function_list) {
    funcs.push_back(func);
  }
  std::sort(funcs.begin(), funcs.end(), [](const auto &a, const auto &b) {
    return a->m_called_depth < b->m_called_depth;
  });
  for (auto &func : funcs) {
    bool inlined = FunctionInlining(func);
    if (inlined) {
      ComputeDominanceRelationship(func);
    }
    // bool inlined = FunctionInlining(func);
    // if (inlined) {
    //   for (auto &[call_instr, caller_func] : func->m_calls) {
    //     RemoveTrivialPhis(caller_func);
    //     RemoveTrivialBasicBlocks(caller_func);
    //   }
    // }
  }
  RemoveUnusedFunctions(m_builder->m_module);
}
