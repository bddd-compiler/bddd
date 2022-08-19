//
// Created by garen on 8/1/22.
// TODO: untested
//

#include <stack>

#include "ir/ir-pass-manager.h"
#include "ir/loop.h"

void StrengthReduction(std::shared_ptr<Loop> loop,
                       std::shared_ptr<IRBuilder> builder) {
  // Value => (basic induction variable, multiplicative factor, additive factor)
  std::map<std::shared_ptr<Value>, std::tuple<std::shared_ptr<Value>, int, int>>
      ivs;
  // for all basic or non-basic ivs
  std::map<std::shared_ptr<Value>, std::shared_ptr<Value>> iv_init_vals;
  // initially only basic ivs know stride
  std::map<std::shared_ptr<Value>, int> strides;
  // 先只考虑整数吧，反正也一样的
  assert(loop->m_preheaders.size() == 1);
  assert(loop->m_latches.size() == 1);
  auto preheader = loop->Preheader();
  auto latch = loop->Latch();

  auto InsertNonBasicIV
      = [&ivs](std::shared_ptr<Value> instr, std::shared_ptr<Value> basic_iv,
               int a, int b) {
          for (auto &iv : ivs) {
            auto [i, aa, bb] = iv.second;
            if (i == basic_iv && aa == a && bb == b) return false;
          }
          ivs[instr] = std::make_tuple(basic_iv, a, b);
          return true;
        };

  // 首先找到basic induction variable
  for (auto &instr : loop->m_header->m_instr_list) {
    if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
      auto updated_val = phi->GetValue(latch);
      if (auto binary
          = std::dynamic_pointer_cast<BinaryInstruction>(updated_val)) {
        int sign = 1;
        if (binary->m_op == IROp::ADD || binary->m_op == IROp::SUB) {
          if (binary->m_op == IROp::SUB) sign = -1;
          // i = i + 1
          if (binary->m_lhs_val_use->getValue()->m_type.IsConst()
              && binary->m_rhs_val_use->getValue() == phi) {
            auto c = std::dynamic_pointer_cast<Constant>(
                binary->m_lhs_val_use->getValue());
            ivs[phi] = std::make_tuple(phi, 1, 0);
            iv_init_vals[phi] = phi->GetValue(preheader);
            strides[phi] = sign * c->Evaluate().IntVal();
          } else if (binary->m_rhs_val_use->getValue()->m_type.IsConst()
                     && binary->m_lhs_val_use->getValue() == phi) {
            auto c = std::dynamic_pointer_cast<Constant>(
                binary->m_rhs_val_use->getValue());
            ivs[phi] = std::make_tuple(phi, 1, 0);
            iv_init_vals[phi] = phi->GetValue(preheader);
            strides[phi] = sign * c->Evaluate().IntVal();
          } else {
            std::cerr << "[debug] not basic induction variable" << std::endl;
          }
        }
      }
    } else {
      break;
    }
  }
  std::stack<std::shared_ptr<BasicBlock>> stack;
  for (auto &bb : loop->m_bbs) {
    bb->m_visited = false;
  }
  stack.push(loop->m_header);
  while (!stack.empty()) {
    std::shared_ptr<BasicBlock> bb = stack.top();
    stack.pop();
    if (bb->m_visited) continue;
    bb->m_visited = true;

    for (auto &instr : bb->m_instr_list) {
      if (auto binary = std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
        if (binary->m_op == IROp::MUL || binary->m_op == IROp::ADD
            || binary->m_op == IROp::SUB) {
          // k = b * j
          // k = j + b
          auto lhs_it = ivs.find(binary->m_lhs_val_use->getValue());
          auto rhs_it = ivs.find(binary->m_rhs_val_use->getValue());
          if (lhs_it != ivs.end()) {
            auto [i, c, d] = lhs_it->second;
            if (auto rhs_const = std::dynamic_pointer_cast<Constant>(
                    binary->m_rhs_val_use->getValue())) {
              int b = rhs_const->Evaluate().IntVal();
              if (binary->m_op == IROp::SUB) b = -b;

              if (c == 1 && d == 0) {
                assert(strides.find(i) != strides.end());
                if (strides[i] == b) {
                  std::cerr << "[debug] skip basic induction variable"
                            << std::endl;
                  continue;
                }
              }
              if (c == 1) continue;  // do not perform strength increase

              if (binary->m_op == IROp::MUL) {
                // ivs[binary] = std::make_tuple(i, b * c, d);
                assert(b > 0);
                InsertNonBasicIV(binary, i, b * c, d);
              } else {
                // ivs[binary] = std::make_tuple(i, c, d + b);
                InsertNonBasicIV(binary, i, c, d + b);
              }
            }
          } else if (rhs_it != ivs.end()) {
            auto [i, c, d] = rhs_it->second;
            if (auto lhs_const = std::dynamic_pointer_cast<Constant>(
                    binary->m_lhs_val_use->getValue())) {
              int b = lhs_const->Evaluate().IntVal();
              if (binary->m_op == IROp::SUB) b = -b;

              if (c == 1 && d == 0) {
                assert(strides.find(i) != strides.end());
                if (strides[i] == b) {
                  std::cerr << "[debug] skip basic induction variable"
                            << std::endl;
                  continue;
                }
              }
              if (c == 1) continue;  // do not perform strength increase

              if (binary->m_op == IROp::MUL) {
                // ivs[binary] = std::make_tuple(i, b * c, d);
                InsertNonBasicIV(binary, i, b * c, d);
              } else if (binary->m_op == IROp::ADD) {
                // ivs[binary] = std::make_tuple(i, c, d + b);
                InsertNonBasicIV(binary, i, c, d + b);
              } else if (binary->m_op == IROp::SUB) {
                // ivs[binary] = std::make_tuple(i, -c, b - d);
                assert(false);
                InsertNonBasicIV(binary, i, -c, b - d);
              }
            }
          }
        }
      }
    }

    auto successors = bb->Successors();
    for (auto it = successors.rbegin(); it != successors.rend(); ++it) {
      auto s = *it;
      if (!s->m_visited) {
        stack.push(s);
      }
    }
  }
  std::cerr << "for strength reduction of " << loop << ":" << std::endl;
  for (auto [j, tuple] : ivs) {
    auto [i, a, b] = tuple;
    std::cerr << j << ": [" << i << ", " << a << ", " << b << "]" << std::endl;
  }

  // work now
  auto preheader_it = preheader->m_instr_list.begin();
  for (; preheader_it != preheader->m_instr_list.end(); ++preheader_it) {
    if ((*preheader_it)->m_op != IROp::PHI) break;
  }
  auto latch_it = latch->m_instr_list.begin();
  for (; latch_it != latch->m_instr_list.end(); ++latch_it) {
    if ((*latch_it)->m_op != IROp::PHI) break;
  }
  // new instructions just insert before preheader_it
  for (auto [j, tuple] : ivs) {
    auto [i, a, b] = tuple;
    if (j == i) continue;  // skip basic induction vars

    // 1. calculate the init value of j in preheader
    assert(iv_init_vals.find(i) != iv_init_vals.end());
    auto now = iv_init_vals[i];
    if (a != 1) {
      auto mul_instr
          = std::make_shared<BinaryInstruction>(IROp::MUL, preheader);
      mul_instr->m_lhs_val_use = now->AddUse(mul_instr);
      mul_instr->m_rhs_val_use = builder->GetIntConstant(a)->AddUse(mul_instr);
      preheader->m_instr_list.insert(preheader_it, mul_instr);
      now = mul_instr;
    }
    if (b != 0) {
      auto add_instr
          = std::make_shared<BinaryInstruction>(IROp::ADD, preheader);
      add_instr->m_lhs_val_use = now->AddUse(add_instr);
      add_instr->m_rhs_val_use = builder->GetIntConstant(b)->AddUse(add_instr);
      preheader->m_instr_list.insert(preheader_it, add_instr);
      now = add_instr;
    }
    iv_init_vals[j] = now;

    // Add one Phi node for this non-basic induction variable into the loop
    // header, and set the incoming value of the Phi node to the initial value
    // we just computed
    auto phi = std::make_shared<PhiInstruction>(now->m_type, loop->m_header);
    phi->AddPhiOperand(preheader, now);
    loop->m_header->PushFrontInstruction(phi);

    // calculate stride
    assert(strides.find(i) != strides.end());
    strides[j] = strides[i] * a + b;
    std::cerr << "stride of " << j << " is " << strides[j] << std::endl;

    // At the latch of the loop body, insert an update instruction for this
    // non-basic induction variable, and set it as the other incoming value of
    // the Phi node we just inserted
    auto add_instr = std::make_shared<BinaryInstruction>(IROp::ADD, latch);
    add_instr->m_lhs_val_use = phi->AddUse(add_instr);
    add_instr->m_rhs_val_use
        = builder->GetIntConstant(strides[j])->AddUse(add_instr);
    latch->m_instr_list.insert(latch_it, add_instr);
    phi->AddPhiOperand(latch, add_instr);
    j->ReplaceUseBy(phi);

    // remove j from bb
    auto j_instr = std::dynamic_pointer_cast<Instruction>(j);
    assert(j_instr != nullptr);
    auto j_bb = j_instr->m_bb;
    j_bb->RemoveInstruction(j_instr);
  }
}

void IRPassManager::StrengthReductionPass() {
  for (auto &func : m_builder->m_module->m_function_list) {
    ComputeLoopRelationship(func);
    for (auto &loop : func->m_loops) {
      StrengthReduction(loop, m_builder);
    }
    DeadCodeElimination(func);
    ReplaceTrivialBranchByJump(func);
    RemoveTrivialBasicBlocks(func);
  }
}