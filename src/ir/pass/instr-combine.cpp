//
// Created by garen on 8/16/22.
//

#include "ir/ir-pass-manager.h"

bool CombineBinaryInstr(std::shared_ptr<BinaryInstruction> binary_instr,
                        std::shared_ptr<BinaryInstruction> user_instr,
                        std::shared_ptr<IRBuilder> builder) {
  // example:
  // x1 = add x0, 1 (binary_instr)
  // x2 = add x1, 2 (user_instr)
  // user_use: the use of x1
  // binary_val : the value of x0 (can be constant too)
  // user_const : the value of 2
  // binary_const : the value of 1
  // target:
  // x2 = add x0, 3 (user_instr)

  // not really remove binary_instr or user_instr
  // but the uses of them will change
  // the skipped instruction will be removed by dce
  auto binary_lhs = binary_instr->m_lhs_val_use->getValue();
  auto binary_rhs = binary_instr->m_rhs_val_use->getValue();
  auto user_lhs = user_instr->m_lhs_val_use->getValue();
  auto user_rhs = user_instr->m_rhs_val_use->getValue();
  auto Check = [&](std::shared_ptr<BinaryInstruction> binary_instr,
                   std::shared_ptr<BinaryInstruction> user_instr) {
    return (std::dynamic_pointer_cast<Constant>(binary_lhs)
            || std::dynamic_pointer_cast<Constant>(binary_rhs))
           && ((std::dynamic_pointer_cast<Constant>(user_lhs)
                && !std::dynamic_pointer_cast<Constant>(user_rhs))
               || (!std::dynamic_pointer_cast<Constant>(user_lhs)
                   && std::dynamic_pointer_cast<Constant>(user_rhs)));
  };
  if (!Check(binary_instr, user_instr)) return false;
  // std::cerr << "[debug] combining instruction..." << std::endl;

  if (user_lhs != binary_instr) return false;
  auto &user_use = user_instr->m_lhs_val_use;
  auto &user_const_use = user_instr->m_rhs_val_use;
  auto user_const
      = std::dynamic_pointer_cast<Constant>(user_const_use->getValue());
  assert(user_const != nullptr);
  assert(user_const_use && user_use && user_const_use != user_use);

  // std::shared_ptr<Value> binary_val;
  // if (!std::dynamic_pointer_cast<Constant>(binary_lhs)) {
  //   binary_val = binary_lhs;
  // } else {
  //   // assert(!std::dynamic_pointer_cast<Constant>(binary_rhs));
  //   binary_val = binary_rhs;
  // }

  auto binary_const = std::dynamic_pointer_cast<Constant>(binary_rhs);
  auto binary_val = binary_lhs;
  if (binary_const == nullptr) return false;
  // assert(binary_const != binary_val);

  auto first_val = binary_const->Evaluate();
  auto second_val = user_const->Evaluate();
  auto first_op = binary_instr->m_op;
  auto second_op = user_instr->m_op;
  if ((first_op == IROp::ADD && second_op == IROp::ADD)
      || (first_op == IROp::SUB && second_op == IROp::SUB)) {
    // x + 1 + 2
    // x + (1 + 2)

    // x - 1 - 2
    // x - (1 + 2)
    int temp = first_val.IntVal() + second_val.IntVal();
    if (first_val.IntVal() > 0 && second_val.IntVal() > 0 && temp < 0) {
      return false;
    } else if (first_val.IntVal() < 0 && second_val.IntVal() < 0 && temp > 0) {
      return false;
    }
    user_use->getValue()->KillUse(user_use);
    user_const->KillUse(user_const_use);
    user_use = binary_val->AddUse(user_instr);
    user_const_use = builder->GetIntConstant(temp)->AddUse(user_instr);
    return true;
  } else if ((first_op == IROp::MUL && second_op == IROp::MUL)
             || (first_op == IROp::SDIV && second_op == IROp::SDIV)) {
    // x * 2 * 3
    // x * (2 * 3)

    // x / 2 / 3
    // x / (2 * 3)
    int temp = first_val.IntVal() * second_val.IntVal();
    if (first_val.IntVal() > 0 && second_val.IntVal() > 0 && temp < 0) {
      return false;
    } else if (first_val.IntVal() < 0 && second_val.IntVal() < 0 && temp < 0) {
      return false;
    } else if (first_val.IntVal() > 0 && second_val.IntVal() < 0 && temp > 0) {
      return false;
    } else if (first_val.IntVal() < 0 && second_val.IntVal() > 0 && temp > 0) {
      return false;
    }
    user_use->getValue()->KillUse(user_use);
    user_const->KillUse(user_const_use);
    user_use = binary_val->AddUse(user_instr);
    user_const_use = builder->GetIntConstant(temp)->AddUse(user_instr);
    return true;
  } else if ((first_op == IROp::ADD && second_op == IROp::SUB)
             || (first_op == IROp::SUB && second_op == IROp::ADD)) {
    // x + a - b
    // x - (b - a)

    // x - a + b
    // x + (b - a)
    int temp = second_val.IntVal() - first_val.IntVal();
    if (second_val.IntVal() > 0 && first_val.IntVal() < 0 && temp < 0) {
      return false;
    } else if (second_val.IntVal() < 0 && first_val.IntVal() > 0 && temp > 0) {
      return false;
    }
    user_use->getValue()->KillUse(user_use);
    user_const->KillUse(user_const_use);
    user_use = binary_val->AddUse(user_instr);
    if (temp >= 0) {
      user_const_use = builder->GetIntConstant(temp)->AddUse(user_instr);
    } else {
      if (user_instr->m_op == IROp::ADD) {
        user_instr->m_op = IROp::SUB;
      } else {
        assert(user_instr->m_op == IROp::SUB);
        user_instr->m_op = IROp::ADD;
      }
      user_const_use = builder->GetIntConstant(-temp)->AddUse(user_instr);
    }
    return true;
  } else if ((first_op == IROp::MUL && second_op == IROp::SDIV)
             || (first_op == IROp::SDIV && second_op == IROp::MUL)) {
    // x * a / b
    // x / (b / a)

    // x / a * b
    // x * (b / a)
    if (first_val.IntVal() == 0) return false;
    if (second_val.IntVal() % first_val.IntVal() != 0) return false;
    int temp = second_val.IntVal() / first_val.IntVal();
    user_use->getValue()->KillUse(user_use);
    user_const->KillUse(user_const_use);
    user_use = binary_val->AddUse(user_instr);
    user_const_use = builder->GetIntConstant(temp)->AddUse(user_instr);
    return true;
  }

  /*
    if ((first_op == IROp::F_ADD && second_op == IROp::F_ADD)
        || (first_op == IROp::F_SUB && second_op == IROp::F_SUB)) {
      // x + 1.0 + 2.0
      // x + (1.0 + 2.0)

      // x - 1.0 - 2.0
      // x - (1.0 + 2.0)
      float a = first_val.FloatVal();
      float b = second_val.FloatVal();
      if ((a < 0.0) == (b < 0.0)
          && std::abs(b) > std::numeric_limits<float>::max() - std::abs(a)) {
        return false;
      }
      float temp = a + b;
      user_use->getValue()->KillUse(user_use);
      user_const->KillUse(user_const_use);
      user_use = binary_val->AddUse(user_instr);
      user_const_use = builder->GetFloatConstant(temp)->AddUse(user_instr);
      return true;
    } else if ((first_op == IROp::F_MUL && second_op == IROp::F_MUL)
               || (first_op == IROp::F_DIV && second_op == IROp::F_DIV)) {
      // x * 2 * 3
      // x * (2 * 3)

      // x / 2 / 3
      // x / (2 * 3)
      float a = first_val.FloatVal();
      float b = second_val.FloatVal();
      if (std::abs(b) > std::numeric_limits<float>::max() / std::abs(a)) {
        return false;
      }
      float temp = a * b;
      user_use->getValue()->KillUse(user_use);
      user_const->KillUse(user_const_use);
      user_use = binary_val->AddUse(user_instr);
      user_const_use = builder->GetFloatConstant(temp)->AddUse(user_instr);
      return true;
    } else if ((first_op == IROp::F_ADD && second_op == IROp::F_SUB)
               || (first_op == IROp::F_SUB && second_op == IROp::F_ADD)) {
      // x + a - b
      // x - (b - a)

      // x - a + b
      // x + (b - a)
      float a = first_val.FloatVal();
      float b = second_val.FloatVal();
      if ((a < 0.0) == (b >= 0.0)
          && std::abs(b) > std::numeric_limits<float>::max() - std::abs(a)) {
        return false;
      }
      float temp = b - a;
      user_use->getValue()->KillUse(user_use);
      user_const->KillUse(user_const_use);
      user_use = binary_val->AddUse(user_instr);
      if (temp >= 0) {
        user_const_use = builder->GetFloatConstant(temp)->AddUse(user_instr);
      } else {
        if (user_instr->m_op == IROp::F_ADD) {
          user_instr->m_op = IROp::F_SUB;
        } else {
          assert(user_instr->m_op == IROp::F_SUB);
          user_instr->m_op = IROp::F_ADD;
        }
        user_const_use = builder->GetFloatConstant(-temp)->AddUse(user_instr);
      }
      return true;
    } else if ((first_op == IROp::F_MUL && second_op == IROp::F_DIV)
               || (first_op == IROp::F_DIV && second_op == IROp::F_MUL)) {
      // x * a / b
      // x / (b / a)

      // x / a * b
      // x * (b / a)
      if (first_val.FloatVal() == 0.0) return false;
      float a = first_val.FloatVal();
      float b = second_val.FloatVal();
      float temp = b / a;  // 暂时不考虑这里的溢出问题
      user_use->getValue()->KillUse(user_use);
      user_const->KillUse(user_const_use);
      user_use = binary_val->AddUse(user_instr);
      user_const_use = builder->GetFloatConstant(temp)->AddUse(user_instr);
      return true;
    }
  */
  return false;
}

void IRPassManager::InstrCombiningPass() {
  for (auto &func : m_builder->m_module->m_function_list) {
    int cnt = 0;
    for (auto &bb : func->m_bb_list) {
      assert(!bb->m_instr_list.empty());
      for (auto &instr : bb->m_instr_list) {
        if (auto binary_instr
            = std::dynamic_pointer_cast<BinaryInstruction>(instr)) {
          std::vector<Use *> uses;
          for (auto &use : instr->m_use_list) uses.push_back(use.get());
          for (auto use : uses) {
            if (auto user_instr = std::dynamic_pointer_cast<BinaryInstruction>(
                    use->getUser())) {
              bool temp
                  = CombineBinaryInstr(binary_instr, user_instr, m_builder);
              if (temp) cnt++;
            }
          }
        }
      }
    }
    if (cnt) {
      std::cerr << "[debug] instr combine x" << cnt << std::endl;
    }
    DeadCodeElimination(func);
  }
}