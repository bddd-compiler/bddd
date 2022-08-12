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
  std::map<std::shared_ptr<Value>, std::shared_ptr<Value>>
      iv_init_vals;  // map<phi node, init value>
  std::map<std::shared_ptr<Value>, int> strides;
  // 先只考虑整数吧，反正也一样的

  for (auto &instr : loop->m_header->m_instr_list) {
    if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
      for (auto &end : loop->m_ends) {
        auto updated_val = phi->GetValue(end);
        if (auto binary
            = std::dynamic_pointer_cast<BinaryInstruction>(updated_val)) {
          if (binary->m_op == IROp::ADD) {
            if (binary->m_lhs_val_use->getValue()->m_type.IsConst()
                && binary->m_rhs_val_use->getValue() == phi) {
              auto c = std::dynamic_pointer_cast<Constant>(
                  binary->m_lhs_val_use->getValue());
              ivs[phi] = std::make_tuple(phi, 1, 0);
              iv_init_vals[phi] = phi->GetValue(loop->m_preheader);
              strides[phi] = c->Evaluate().IntVal();
            } else if (binary->m_rhs_val_use->getValue()->m_type.IsConst()
                       && binary->m_lhs_val_use->getValue() == phi) {
              auto c = std::dynamic_pointer_cast<Constant>(
                  binary->m_rhs_val_use->getValue());
              ivs[phi] = std::make_tuple(phi, 1, 0);
              iv_init_vals[phi] = phi->GetValue(loop->m_preheader);
              strides[phi] = c->Evaluate().IntVal();
            } else {
              std::cerr << "[debug] not basic induction variable" << std::endl;
            }
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
        if (binary->m_op == IROp::MUL || binary->m_op == IROp::ADD) {
          // k = b * j
          // k = j + b
          auto lhs_it = ivs.find(binary->m_lhs_val_use->getValue());
          if (lhs_it != ivs.end()) {
            auto [i, c, d] = lhs_it->second;
            if (auto rhs_const = std::dynamic_pointer_cast<Constant>(
                    binary->m_rhs_val_use->getValue())) {
              assert(ivs.find(binary) == ivs.end());
              int b = rhs_const->Evaluate().IntVal();
              if (binary->m_op == IROp::MUL) {
                ivs[binary] = std::make_tuple(i, b * c, d);
              } else {
                ivs[binary] = std::make_tuple(i, c, d + b);
              }
            }
          }
          auto rhs_it = ivs.find(binary->m_rhs_val_use->getValue());
          if (rhs_it != ivs.end()) {
            auto [i, c, d] = rhs_it->second;
            if (auto lhs_const = std::dynamic_pointer_cast<Constant>(
                    binary->m_lhs_val_use->getValue())) {
              assert(ivs.find(binary) == ivs.end());
              int b = lhs_const->Evaluate().IntVal();
              if (binary->m_op == IROp::MUL) {
                ivs[binary] = std::make_tuple(i, b * c, d);
              } else {
                ivs[binary] = std::make_tuple(i, c, d + b);
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
  std::cerr << "for strength reduction:" << std::endl;
  for (auto [j, tuple] : ivs) {
    auto [i, a, b] = tuple;
    std::cerr << j << ": [" << i << ", " << a << ", " << b << "]" << std::endl;
  }

  // work now
  for (auto [j, tuple] : ivs) {
    auto [i, a, b] = tuple;
    if (j == i) continue;  // skip basic induction vars

    // 1. calculate the init value of j in preheader
    assert(iv_init_vals.find(i) != iv_init_vals.end());
    auto now = iv_init_vals[i];
    if (a != 1) {
      auto mul_instr
          = std::make_shared<BinaryInstruction>(IROp::MUL, loop->m_preheader);
      mul_instr->m_lhs_val_use = now->AddUse(mul_instr);
      mul_instr->m_rhs_val_use = builder->GetIntConstant(a)->AddUse(mul_instr);
      loop->m_preheader->InsertFrontInstruction(
          loop->m_preheader->LastInstruction(), mul_instr);
      now = mul_instr;
    }
    if (b != 0) {
      auto add_instr
          = std::make_shared<BinaryInstruction>(IROp::ADD, loop->m_preheader);
      add_instr->m_lhs_val_use = now->AddUse(add_instr);
      add_instr->m_rhs_val_use = builder->GetIntConstant(b)->AddUse(add_instr);
      loop->m_preheader->InsertFrontInstruction(
          loop->m_preheader->LastInstruction(), add_instr);
      now = add_instr;
    }
    iv_init_vals[j] = now;
    // 先考虑只有一个出口的情况吧
    auto phi = std::make_shared<PhiInstruction>(now->m_type, loop->m_header);
    phi->AddPhiOperand(loop->m_preheader, now);
    loop->m_header->PushFrontInstruction(phi);

    assert(strides.find(i) != strides.end());
    strides[j] = strides[i] * a + b;
    std::cerr << "stride of " << j << " is " << strides[j] << std::endl;
    for (auto &end : loop->m_ends) {
      auto add_instr = std::make_shared<BinaryInstruction>(IROp::ADD, end);
      add_instr->m_lhs_val_use = phi->AddUse(add_instr);
      add_instr->m_rhs_val_use
          = builder->GetIntConstant(strides[j])->AddUse(add_instr);
      end->InsertFrontInstruction(end->LastInstruction(), add_instr);
      phi->AddPhiOperand(end, add_instr);

      j->ReplaceUseBy(phi);
    }
  }
}

void IRPassManager::StrengthReductionPass() {
  for (auto &func : m_builder->m_module->m_function_list) {
    ComputeLoopRelationship(func);
    for (auto &loop : func->m_loops) {
      StrengthReduction(loop, m_builder);
    }
    // DeadCodeElimination(func);
  }
}