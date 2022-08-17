//
// Created by garen on 8/2/22.
//

#include "ir/ir-pass-manager.h"
#include "ir/loop.h"

const int K = 4;

void CopyInstructions(
    const std::vector<std::shared_ptr<Instruction>> &original_instrs,
    std::shared_ptr<BasicBlock> bb, std::shared_ptr<EasyLoop> easy_loop,
    std::shared_ptr<EasyLoop> original_easy_loop) {
  std::map<std::shared_ptr<Value>, std::shared_ptr<Value>> new_vals;
  auto GetNewVal
      = [&](std::shared_ptr<Value> old_val) -> std::shared_ptr<Value> {
    auto it = original_easy_loop->m_loop_vars_by_phi.find(old_val);
    if (it != original_easy_loop->m_loop_vars_by_phi.end()) {
      if (easy_loop == original_easy_loop) {
        return it->second->m_def_instr->GetValue(easy_loop->m_body_bb);
      } else {
        auto phi = std::dynamic_pointer_cast<PhiInstruction>(old_val);
        assert(phi != nullptr);
        return easy_loop->GetPhi(original_easy_loop, phi);
      }
    } else if (std::dynamic_pointer_cast<Constant>(old_val)
               || std::dynamic_pointer_cast<GlobalVariable>(old_val)
               || old_val->m_bb != original_easy_loop->m_body_bb) {
      return old_val;
    } else if (new_vals.find(old_val) != new_vals.end()) {
      return new_vals[old_val];
    } else {
      assert(false);  // not considered yet
    }
  };
  std::vector<std::shared_ptr<Instruction>> new_instrs;
  for (auto &instr : original_instrs) {
    if (auto alloca = std::dynamic_pointer_cast<AllocaInstruction>(instr)) {
      auto new_alloca = std::make_shared<AllocaInstruction>(
          alloca->m_type, bb, false, alloca->m_is_const);
      bb->PushBackInstruction(new_alloca);
      new_vals[alloca] = new_alloca;
    } else if (auto binary
               = std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
      auto new_binary = std::make_shared<BinaryInstruction>(binary->m_op, bb);
      new_binary->m_lhs_val_use
          = GetNewVal(binary->m_lhs_val_use->getValue())->AddUse(new_binary);
      new_binary->m_rhs_val_use
          = GetNewVal(binary->m_rhs_val_use->getValue())->AddUse(new_binary);
      bb->PushBackInstruction(new_binary);
      new_vals[binary] = new_binary;
    } else if (auto bitcast
               = std::dynamic_pointer_cast<BitCastInstruction>(instr)) {
      auto new_bitcast
          = std::make_shared<BitCastInstruction>(bitcast->m_target_type, bb);
      new_bitcast->m_val
          = GetNewVal(bitcast->m_val->getValue())->AddUse(new_bitcast);
      bb->PushBackInstruction(new_bitcast);
      new_vals[bitcast] = new_bitcast;
    } else if (auto zext = std::dynamic_pointer_cast<ZExtInstruction>(instr)) {
      auto new_zext = std::make_shared<ZExtInstruction>(bb);
      new_zext->m_val = GetNewVal(zext->m_val->getValue())->AddUse(new_zext);
      bb->PushBackInstruction(new_zext);
      new_vals[zext] = new_zext;
    } else if (auto fneg = std::dynamic_pointer_cast<FNegInstruction>(instr)) {
      auto new_fneg = std::make_shared<FNegInstruction>(bb);
      new_fneg->m_lhs_val_use
          = GetNewVal(fneg->m_lhs_val_use->getValue())->AddUse(new_fneg);
      bb->PushBackInstruction(new_fneg);
      new_vals[fneg] = new_fneg;
    } else if (auto sitofp
               = std::dynamic_pointer_cast<SIToFPInstruction>(instr)) {
      auto new_sitofp = std::make_shared<SIToFPInstruction>(bb);
      new_sitofp->m_val
          = GetNewVal(sitofp->m_val->getValue())->AddUse(new_sitofp);
      bb->PushBackInstruction(new_sitofp);
      new_vals[sitofp] = new_sitofp;
    } else if (auto fptosi
               = std::dynamic_pointer_cast<FPToSIInstruction>(instr)) {
      auto new_fptosi = std::make_shared<FPToSIInstruction>(bb);
      new_fptosi->m_val
          = GetNewVal(fptosi->m_val->getValue())->AddUse(new_fptosi);
      bb->PushBackInstruction(new_fptosi);
      new_vals[fptosi] = new_fptosi;
    } else if (auto call = std::dynamic_pointer_cast<CallInstruction>(instr)) {
      auto new_call = std::make_shared<CallInstruction>(
          call->m_type.m_base_type, call->m_func_name, call->m_function, bb);
      std::vector<Use *> new_params_uses;
      for (auto use : call->m_params) {
        new_params_uses.push_back(GetNewVal(use->getValue())->AddUse(new_call));
      }
      new_call->SetParams(std::move(new_params_uses));
      bb->PushBackInstruction(new_call);
      new_vals[call] = new_call;
    } else if (auto load = std::dynamic_pointer_cast<LoadInstruction>(instr)) {
      auto new_load = std::make_shared<LoadInstruction>(bb);
      new_load->m_addr = GetNewVal(load->m_addr->getValue())->AddUse(new_load);
      new_load->m_type = load->m_type;
      bb->PushBackInstruction(new_load);
      new_vals[load] = new_load;
    } else if (auto store
               = std::dynamic_pointer_cast<StoreInstruction>(instr)) {
      auto new_store = std::make_shared<StoreInstruction>(bb);
      new_store->m_addr
          = GetNewVal(store->m_addr->getValue())->AddUse(new_store);
      new_store->m_val = GetNewVal(store->m_val->getValue())->AddUse(new_store);
      bb->PushBackInstruction(new_store);
      new_vals[store] = new_store;
    } else if (auto gep
               = std::dynamic_pointer_cast<GetElementPtrInstruction>(instr)) {
      auto new_gep = std::make_shared<GetElementPtrInstruction>(bb);
      new_gep->m_addr = GetNewVal(gep->m_addr->getValue())->AddUse(new_gep);
      for (auto use : gep->m_indices) {
        new_gep->m_indices.push_back(
            GetNewVal(use->getValue())->AddUse(new_gep));
      }
      new_gep->m_type = gep->m_type;
      bb->PushBackInstruction(new_gep);
      new_vals[gep] = new_gep;
    } else {
      // return, jump, branch, phi cannot appear here
      assert(false);
    }

    // if instr is used as result of loop vars
    if (original_easy_loop->m_loop_vars_by_val.count(instr)) {
      if (easy_loop == original_easy_loop) {
        for (auto &loop_var : easy_loop->m_loop_vars_by_val[instr]) {
          loop_var->m_def_instr->ReplacePhiValue(
              easy_loop->m_body_bb, easy_loop->m_body_bb->LastInstruction());
        }
      } else {
        for (auto &original_loop_var :
             original_easy_loop->m_loop_vars_by_val[instr]) {
          auto loop_var = std::make_shared<LoopVar>();
          loop_var->m_loop = easy_loop->m_loop;
          loop_var->m_init_val = original_loop_var->m_def_instr;
          loop_var->m_def_instr = easy_loop->GetPhi(
              original_easy_loop, original_loop_var->m_def_instr);
          loop_var->m_body_val = easy_loop->m_body_bb->LastInstruction();
          easy_loop->m_loop_vars_by_phi[loop_var->m_def_instr] = loop_var;
          easy_loop->m_loop_vars_by_val[loop_var->m_body_val].push_back(
              loop_var);
        }
      }
    }
  }
}

void InsertResetLoop(
    std::shared_ptr<Function> func, std::shared_ptr<EasyLoop> reset_easy_loop,
    std::shared_ptr<EasyLoop> easy_loop,
    const std::vector<std::shared_ptr<Instruction>> &original_instrs) {
  auto reset_loop = std::make_shared<Loop>();

  reset_easy_loop->m_loop = reset_loop;

  reset_easy_loop->m_cond_bb
      = std::make_shared<BasicBlock>("unroll_reset_cond");
  reset_easy_loop->m_body_bb
      = std::make_shared<BasicBlock>("unroll_reset_body");

  for (auto &phi : easy_loop->m_phis) {
    auto new_phi = std::make_shared<PhiInstruction>(phi->m_type,
                                                    reset_easy_loop->m_cond_bb);
    new_phi->AddPhiOperand(easy_loop->m_cond_bb, phi);
    reset_easy_loop->m_phis.push_back(new_phi);
    reset_easy_loop->m_cond_bb->PushBackInstruction(new_phi);
  }
  assert(easy_loop->m_phis.size() == reset_easy_loop->m_phis.size());
  for (int i = 0; i < easy_loop->m_phis.size(); ++i) {
    auto phi = easy_loop->m_phis[i];
    auto new_phi = reset_easy_loop->m_phis[i];
    auto val = phi->GetValue(easy_loop->m_body_bb);
    if (easy_loop->m_loop_vars_by_phi.count(val)) {
      new_phi->AddPhiOperand(
          reset_easy_loop->m_body_bb,
          reset_easy_loop->GetPhi(
              easy_loop, easy_loop->m_loop_vars_by_phi[val]->m_def_instr));
      std::cerr << "[debug] what does it mean???" << std::endl;
    }
  }

  auto it = std::find(func->m_bb_list.begin(), func->m_bb_list.end(),
                      easy_loop->m_body_bb);
  ++it;
  it = func->m_bb_list.insert(it, reset_easy_loop->m_cond_bb);
  ++it;
  func->m_bb_list.insert(it, reset_easy_loop->m_body_bb);

  CopyInstructions(original_instrs, reset_easy_loop->m_body_bb, reset_easy_loop,
                   easy_loop);
  auto jmp_instr = std::make_shared<JumpInstruction>(
      reset_easy_loop->m_cond_bb, reset_easy_loop->m_body_bb);
  reset_easy_loop->m_body_bb->PushBackInstruction(jmp_instr);

  auto def_instr
      = reset_easy_loop->GetPhi(easy_loop, easy_loop->m_cond_var->m_def_instr);
  assert(reset_easy_loop->m_loop_vars_by_phi.count(def_instr));
  reset_easy_loop->m_cond_var = reset_easy_loop->m_loop_vars_by_phi[def_instr];
  auto lhs_val = easy_loop->m_cmp_instr->m_lhs_val_use->getValue();
  auto rhs_val = easy_loop->m_cmp_instr->m_rhs_val_use->getValue();
  if (lhs_val == easy_loop->m_cond_var->m_def_instr) {
    lhs_val = reset_easy_loop->m_cond_var->m_def_instr;
  } else if (rhs_val == easy_loop->m_cond_var->m_def_instr) {
    rhs_val = reset_easy_loop->m_cond_var->m_def_instr;
  } else {
    assert(false);
  }

  auto reset_cmp_instr = std::make_shared<BinaryInstruction>(
      easy_loop->m_cmp_instr->m_op, reset_easy_loop->m_cond_bb);
  reset_cmp_instr->m_lhs_val_use = lhs_val->AddUse(reset_cmp_instr);
  reset_cmp_instr->m_rhs_val_use = rhs_val->AddUse(reset_cmp_instr);
  reset_easy_loop->m_cmp_instr = reset_cmp_instr;
  auto reset_br_instr = std::make_shared<BranchInstruction>(
      reset_easy_loop->m_body_bb, easy_loop->m_br_instr->m_false_block,
      reset_easy_loop->m_cond_bb);
  reset_br_instr->m_cond = reset_cmp_instr->AddUse(reset_br_instr);
  reset_easy_loop->m_br_instr = reset_br_instr;
  reset_easy_loop->m_cond_bb->PushBackInstruction(reset_cmp_instr);
  reset_easy_loop->m_cond_bb->PushBackInstruction(reset_br_instr);

  for (auto &[enter_var, loop_vars] : reset_easy_loop->m_loop_vars_by_val) {
    for (auto &loop_var : loop_vars) {
      loop_var->m_def_instr->AddPhiOperand(reset_easy_loop->m_body_bb,
                                           enter_var);
      std::cerr << "[debug] not null" << std::endl;
    }
  }
  for (auto &instr : reset_easy_loop->m_cond_bb->m_instr_list) {
    if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
      if (!phi->m_contents.count(easy_loop->m_cond_bb)) {
        phi->AddPhiOperand(easy_loop->m_cond_bb, nullptr);
        std::cerr << "[debug] null" << std::endl;
      }
      if (!phi->m_contents.count(reset_easy_loop->m_body_bb)) {
        phi->AddPhiOperand(reset_easy_loop->m_body_bb, nullptr);
        std::cerr << "[debug] null" << std::endl;
      }
    } else {
      break;
    }
  }

  auto out_block = easy_loop->m_br_instr->m_false_block;
  easy_loop->m_cond_bb->ReplaceSuccessorBy(easy_loop->m_br_instr->m_false_block,
                                           reset_easy_loop->m_cond_bb);
  reset_easy_loop->m_cond_bb->AddPredecessor(reset_easy_loop->m_body_bb);
  reset_easy_loop->m_body_bb->AddPredecessor(reset_easy_loop->m_cond_bb);
  out_block->AddPredecessor(reset_easy_loop->m_cond_bb);
  reset_loop->m_preheader = easy_loop->m_cond_bb;
  reset_loop->m_loop_depth = easy_loop->m_loop->m_loop_depth;

  func->m_deepest_loops.push_back(reset_loop);
  func->m_loops.insert(reset_loop);
  if (easy_loop->m_loop->m_fa_loop) {
    easy_loop->m_loop->m_fa_loop->m_sub_loops.insert(reset_loop);
  }
  reset_loop->m_bbs.insert(reset_easy_loop->m_cond_bb);
  reset_loop->m_bbs.insert(reset_easy_loop->m_body_bb);

  auto FoundInCurrentLoop = [&](std::shared_ptr<BasicBlock> bb) {
    return easy_loop->m_loop->m_bbs.find(bb) != easy_loop->m_loop->m_bbs.end()
           || reset_easy_loop->m_loop->m_bbs.find(bb)
                  != reset_easy_loop->m_loop->m_bbs.end();
  };

  assert(easy_loop->m_phis.size() == reset_easy_loop->m_phis.size());
  for (int i = 0; i < easy_loop->m_phis.size(); ++i) {
    auto original_phi = easy_loop->m_phis[i];
    for (auto phi_it = original_phi->m_use_list.begin();
         phi_it != original_phi->m_use_list.end();) {
      // manually ++it
      auto use = phi_it->get();
      ++phi_it;
      if (FoundInCurrentLoop(use->getUser()->m_bb)) continue;

      auto reset_phi = reset_easy_loop->m_phis[i];
      // replace the use by the corresponding phi in reset easy loop
      assert(use->getValue() == original_phi);
      auto use_ptr = original_phi->KillUse(use, true);
      assert(use_ptr.get() == use);
      use_ptr->m_value = reset_phi;
      reset_phi->m_use_list.push_back(std::move(use_ptr));
      std::cerr << "[debug] rep" << std::endl;
    }
  }
}

void MakeRunOnce(std::shared_ptr<EasyLoop> easy_loop) {
  // the easy loop will be no longer a loop
  // choice 1: update loop info manually
  // choice 2: delete all of its info
  for (auto &phi : easy_loop->m_phis) {
    auto body_val = phi->m_contents[easy_loop->m_body_bb]->getValue();
    // specially replace uses out of cond_bb and body_bb
    for (auto it = phi->m_use_list.begin(); it != phi->m_use_list.end();) {
      // manually ++it
      auto use = it->get();
      ++it;
      assert(use->getValue() == phi);
      auto user = use->getUser();
      if (user->m_bb != easy_loop->m_body_bb
          && user->m_bb != easy_loop->m_cond_bb) {
        auto use_ptr = phi->KillUse(use, true);
        assert(use_ptr.get() == use);
        use_ptr->m_value = body_val;
        body_val->m_use_list.push_back(std::move(use_ptr));
      }
    }
  }
  // cond <-> body
  // cond -> body -> false_block
  easy_loop->m_cond_bb->RemovePredecessor(easy_loop->m_body_bb);
  auto old_jmp_instr = easy_loop->m_body_bb->LastInstruction();
  auto new_jmp_instr = std::make_shared<JumpInstruction>(
      easy_loop->m_br_instr->m_false_block, easy_loop->m_body_bb);
  easy_loop->m_br_instr->m_false_block->ReplacePredecessorsBy(
      easy_loop->m_cond_bb, {easy_loop->m_body_bb});
  easy_loop->m_body_bb->m_instr_list.pop_back();
  easy_loop->m_body_bb->PushBackInstruction(new_jmp_instr);

  auto old_br_instr = easy_loop->m_cond_bb->LastInstruction();
  auto new_jmp_instr2 = std::make_shared<JumpInstruction>(easy_loop->m_body_bb,
                                                          easy_loop->m_cond_bb);
  easy_loop->m_cond_bb->m_instr_list.pop_back();
  easy_loop->m_cond_bb->PushBackInstruction(new_jmp_instr2);
}

// TODO: only support int
void FixUnrollLoopCond(std::shared_ptr<EasyLoop> easy_loop, int stride,
                       int reset_cnt, std::shared_ptr<IRBuilder> builder,
                       bool c = false) {
  // i < n
  // i < n - reset_cnt
  auto lhs_val = easy_loop->m_cmp_instr->m_lhs_val_use->getValue();
  auto rhs_val = easy_loop->m_cmp_instr->m_rhs_val_use->getValue();
  if (c) {
    auto rhs_const = std::dynamic_pointer_cast<Constant>(rhs_val);
    auto lhs_const = std::dynamic_pointer_cast<Constant>(lhs_val);
    if (easy_loop->m_cond_var->m_def_instr == lhs_val && rhs_const != nullptr) {
      auto old_n = rhs_const->Evaluate().IntVal();
      rhs_const->KillUse(easy_loop->m_cmp_instr->m_rhs_val_use);
      auto new_n = stride > 0 ? old_n - reset_cnt : old_n + reset_cnt;
      easy_loop->m_cmp_instr->m_rhs_val_use
          = builder->GetIntConstant(new_n)->AddUse(easy_loop->m_cmp_instr);
      return;
    } else if (easy_loop->m_cond_var->m_def_instr == rhs_val
               && lhs_const != nullptr) {
      auto old_n = lhs_const->Evaluate().IntVal();
      lhs_const->KillUse(easy_loop->m_cmp_instr->m_lhs_val_use);
      auto new_n = stride > 0 ? old_n - reset_cnt : old_n + reset_cnt;
      easy_loop->m_cmp_instr->m_lhs_val_use
          = builder->GetIntConstant(new_n)->AddUse(easy_loop->m_cmp_instr);
      return;
    }
  }
  // non-const

  std::shared_ptr<Value> i = nullptr, n = nullptr;
  if (lhs_val == easy_loop->m_cond_var->m_def_instr) {
    i = lhs_val;
    n = rhs_val;
    assert(i->m_type.IsBasicInt());
    assert(n->m_type.IsBasicInt());
    // i < n
    // K * stride < n - i
    auto sub_instr
        = std::make_shared<BinaryInstruction>(IROp::SUB, easy_loop->m_cond_bb);
    sub_instr->m_lhs_val_use = n->AddUse(sub_instr);
    sub_instr->m_rhs_val_use = i->AddUse(sub_instr);
    auto it = std::find(easy_loop->m_cond_bb->m_instr_list.begin(),
                        easy_loop->m_cond_bb->m_instr_list.end(),
                        easy_loop->m_cmp_instr);
    easy_loop->m_cond_bb->m_instr_list.insert(it, sub_instr);

    easy_loop->m_cmp_instr->m_lhs_val_use->getValue()->KillUse(
        easy_loop->m_cmp_instr->m_lhs_val_use);
    easy_loop->m_cmp_instr->m_lhs_val_use
        = builder->GetIntConstant(K * stride)->AddUse(easy_loop->m_cmp_instr);
    easy_loop->m_cmp_instr->m_rhs_val_use->getValue()->KillUse(
        easy_loop->m_cmp_instr->m_rhs_val_use);
    easy_loop->m_cmp_instr->m_rhs_val_use
        = sub_instr->AddUse(easy_loop->m_cmp_instr);
  } else {
    assert(rhs_val == easy_loop->m_cond_var->m_def_instr);
    i = rhs_val;
    n = lhs_val;
    assert(i->m_type.IsBasicInt());
    assert(n->m_type.IsBasicInt());
    // x[1] > i
    // x[1] - i > K * stride
    auto sub_instr
        = std::make_shared<BinaryInstruction>(IROp::SUB, easy_loop->m_cond_bb);
    sub_instr->m_lhs_val_use = n->AddUse(sub_instr);
    sub_instr->m_rhs_val_use = i->AddUse(sub_instr);
    auto it = std::find(easy_loop->m_cond_bb->m_instr_list.begin(),
                        easy_loop->m_cond_bb->m_instr_list.end(),
                        easy_loop->m_cmp_instr);
    easy_loop->m_cond_bb->m_instr_list.insert(it, sub_instr);

    easy_loop->m_cmp_instr->m_lhs_val_use->getValue()->KillUse(
        easy_loop->m_cmp_instr->m_lhs_val_use);
    easy_loop->m_cmp_instr->m_lhs_val_use
        = sub_instr->AddUse(easy_loop->m_cmp_instr);
    easy_loop->m_cmp_instr->m_rhs_val_use->getValue()->KillUse(
        easy_loop->m_cmp_instr->m_rhs_val_use);
    easy_loop->m_cmp_instr->m_rhs_val_use
        = builder->GetIntConstant(K * stride)->AddUse(easy_loop->m_cmp_instr);
  }
}

bool unroll(std::shared_ptr<Function> func, std::shared_ptr<Loop> loop,
            std::shared_ptr<IRBuilder> builder) {
  // only consider the header and the unique body
  if (loop->m_bbs.size() != 2) return false;
  std::shared_ptr<BasicBlock> loop_body;
  int cnt = 0;
  for (auto &bb : loop->m_bbs) {
    if (bb == loop->m_header) {
      ++cnt;
    } else {
      loop_body = bb;
      --cnt;
    }
  }
  assert(cnt == 0);
  for (auto &instr : loop_body->m_instr_list) {
    if (instr->m_op == IROp::RETURN || instr->m_op == IROp::PHI) return false;
  }
  for (auto &instr : loop->m_header->m_instr_list) {
    if (instr->m_type.IsBasicFloat()) return false;
  }

  auto easy_loop = std::make_shared<EasyLoop>(loop);
  if (easy_loop->m_cmp_instr == nullptr) return false;
  auto op = easy_loop->m_cmp_instr->m_op;
  if (op != IROp::I_SGE && op != IROp::I_SGT && op != IROp::I_SLE
      && op != IROp::I_SLT)
    return false;

  if (AnalyzeConstLoop(easy_loop)) {
    assert(easy_loop->m_cnt >= 0);
    int total_exec_cnt = easy_loop->m_cnt;

    if (total_exec_cnt == 0) return false;  // execute 0 times
    int exec_cnt = total_exec_cnt / K;
    if (exec_cnt == 0) return false;  // cannot unroll it
    auto jmp_instr = easy_loop->m_body_bb->LastInstruction();
    easy_loop->m_body_bb->m_instr_list.pop_back();  // not really remove
    std::vector<std::shared_ptr<Instruction>> original_instrs(
        easy_loop->m_body_bb->m_instr_list.begin(),
        easy_loop->m_body_bb->m_instr_list.end());
    for (int i = 0; i < K - 1; ++i) {
      CopyInstructions(original_instrs, easy_loop->m_body_bb, easy_loop,
                       easy_loop);
    }
    easy_loop->m_body_bb->m_instr_list.push_back(jmp_instr);

    int reset_cnt = total_exec_cnt % K;
    if (reset_cnt > 0) {
      // i = 100, K = 2, n = 101, 不能直接是i < n，不然i会跑到102
      // 解决方案是控制i只跑到100，剩下的通过reset easy loop完成
      // 也就是要来个不完整的循环展开，弄一个没有回跳的“循环”就可以了
      std::cerr << "[debug] loop unrolling... (const)" << std::endl;
      FixUnrollLoopCond(easy_loop, easy_loop->m_stride, reset_cnt, builder,
                        true);
      auto reset_easy_loop = std::make_shared<EasyLoop>();
      InsertResetLoop(func, reset_easy_loop, easy_loop, original_instrs);
      if (reset_cnt > 1) {
        auto reset_jmp_instr = reset_easy_loop->m_body_bb->m_instr_list.back();
        reset_easy_loop->m_body_bb->m_instr_list.pop_back();
        std::vector<std::shared_ptr<Instruction>> original_reset_instrs(
            reset_easy_loop->m_body_bb->m_instr_list.begin(),
            reset_easy_loop->m_body_bb->m_instr_list.end());
        for (int i = 0; i < reset_cnt - 1; ++i) {
          CopyInstructions(original_reset_instrs, reset_easy_loop->m_body_bb,
                           reset_easy_loop, reset_easy_loop);
        }
        reset_easy_loop->m_body_bb->m_instr_list.push_back(reset_jmp_instr);
      }
      MakeRunOnce(reset_easy_loop);
    }
    if (exec_cnt == 1) {
      MakeRunOnce(easy_loop);
    }
    return true;
  } else if (AnalyzeFlexibleLoop(easy_loop)) {
    std::cerr << "[debug] loop unrolling... (non-const)" << std::endl;
    assert(easy_loop->m_stride != 0);  // stride known, but cnt unknown
    auto jmp_instr = easy_loop->m_body_bb->m_instr_list.back();
    easy_loop->m_body_bb->m_instr_list.pop_back();
    std::vector<std::shared_ptr<Instruction>> original_instrs(
        easy_loop->m_body_bb->m_instr_list.begin(),
        easy_loop->m_body_bb->m_instr_list.end());
    for (int i = 0; i < K - 1; ++i) {
      CopyInstructions(original_instrs, easy_loop->m_body_bb, easy_loop,
                       easy_loop);
    }
    easy_loop->m_body_bb->m_instr_list.push_back(jmp_instr);

    auto reset_easy_loop = std::make_shared<EasyLoop>();
    InsertResetLoop(func, reset_easy_loop, easy_loop, original_instrs);
    FixUnrollLoopCond(easy_loop, easy_loop->m_stride, 233, builder);
    return true;
  } else {
    return false;
  }
}

void IRPassManager::LoopUnrollingPass() {
  RemoveUnusedFunctions(m_builder->m_module);
  for (auto &func : m_builder->m_module->m_function_list) {
    ComputeLoopRelationship(func);
    auto deepest_loops = func->m_deepest_loops;
    for (auto &loop : deepest_loops) {
      unroll(func, loop, m_builder);
    }
    DeadCodeElimination(func);
  }
}