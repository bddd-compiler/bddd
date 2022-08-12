//
// Created by garen on 8/3/22.
//

#include "ir/ir.h"
#include "ir/loop.h"

// -1: return false
// >= 0: normal
int ComputeCnt(IROp op, int i, int n, int stride) {
  switch (op) {
    case IROp::I_SLE:
      // i <= n
      if (i > n)
        return 0;
      else
        return (n - i) / stride + 1;
    case IROp::I_SLT:
      // i < n ==> i <= n - 1
      if (i >= n)
        return 0;
      else
        return (n - 1 - i) / stride + 1;
    case IROp::I_SGE:
      // i >= n
      if (i < n)
        return 0;
      else
        return (i - n) / (-stride) + 1;
    case IROp::I_SGT:
      // i > n ==> i >= n + 1
      if (i <= n)
        return 0;
      else
        return (i - n - 1) / (-stride) + 1;
    default:
      return -1;
  }
}

// 0: return false
// non-zero: return the stride
int ComputeStride(std::shared_ptr<LoopVar> loop_var,
                  std::shared_ptr<BasicBlock> bb) {
  int stride = 0;
  auto instr = loop_var->m_body_val;
  while (instr != loop_var->m_def_instr && instr->m_bb == bb) {
    auto binary = std::dynamic_pointer_cast<BinaryInstruction>(instr);
    if (binary == nullptr) return 0;
    if (binary->m_op == IROp::ADD) {
      auto lhs_c = std::dynamic_pointer_cast<Constant>(
          binary->m_lhs_val_use->getValue());
      auto rhs_c = std::dynamic_pointer_cast<Constant>(
          binary->m_rhs_val_use->getValue());
      if (lhs_c != nullptr && rhs_c == nullptr) {
        instr = binary->m_rhs_val_use->getValue();
        stride += lhs_c->Evaluate().IntVal();
      } else if (lhs_c == nullptr && rhs_c != nullptr) {
        instr = binary->m_lhs_val_use->getValue();
        stride += rhs_c->Evaluate().IntVal();
      } else {
        return 0;
      }
    } else if (binary->m_op == IROp::SUB) {
      if (auto rhs_c = std::dynamic_pointer_cast<Constant>(
              binary->m_rhs_val_use->getValue())) {
        instr = binary->m_lhs_val_use->getValue();
        stride -= rhs_c->Evaluate().IntVal();
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }
  return stride;
}

bool AnalyzeConstLoop(std::shared_ptr<EasyLoop> loop_ir) {
  auto lhs_val = loop_ir->m_cmp_instr->m_lhs_val_use->getValue();
  auto rhs_val = loop_ir->m_cmp_instr->m_rhs_val_use->getValue();
  auto lhs_const = std::dynamic_pointer_cast<Constant>(lhs_val);
  auto rhs_const = std::dynamic_pointer_cast<Constant>(rhs_val);

  if (lhs_const == nullptr && rhs_const != nullptr) {
    // i < n
    // i <= n
    // i > n
    // i >= n
    if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(lhs_val)) {
      if (!loop_ir->m_loop_vars_by_phi.count(lhs_val)) return false;
      loop_ir->m_cond_var = loop_ir->m_loop_vars_by_phi[lhs_val];
      if (auto init_val = std::dynamic_pointer_cast<Constant>(
              loop_ir->m_cond_var->m_init_val)) {
        int i = init_val->Evaluate().IntVal();
        int n = rhs_const->Evaluate().IntVal();
        int stride = ComputeStride(loop_ir->m_cond_var, loop_ir->m_body_bb);
        if (stride == 0) return false;
        loop_ir->m_stride = stride;
        int cnt
            = ComputeCnt(loop_ir->m_cmp_instr->m_op, i, n, loop_ir->m_stride);
        if (cnt == -1) return false;
        loop_ir->m_cnt = cnt;
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  } else if (lhs_const != nullptr && rhs_const == nullptr) {
    // n < i
    // n <= i
    // n > i
    // n >= i
    if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(rhs_val)) {
      if (!loop_ir->m_loop_vars_by_phi.count(rhs_val)) return false;
      loop_ir->m_cond_var = loop_ir->m_loop_vars_by_phi[rhs_val];
      if (auto init_val = std::dynamic_pointer_cast<Constant>(
              loop_ir->m_cond_var->m_init_val)) {
        int i = init_val->Evaluate().IntVal();
        int n = lhs_const->Evaluate().IntVal();
        int stride = ComputeStride(loop_ir->m_cond_var, loop_ir->m_body_bb);
        if (stride == 0) return false;
        loop_ir->m_stride = stride;
        int cnt
            = ComputeCnt(loop_ir->m_cmp_instr->m_op, i, n, loop_ir->m_stride);
        if (cnt == -1) return false;
        loop_ir->m_cnt = cnt;
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool AnalyzeFlexibleLoop(std::shared_ptr<EasyLoop> easy_loop) {
  //
  auto lhs_val = easy_loop->m_cmp_instr->m_lhs_val_use->getValue();
  auto rhs_val = easy_loop->m_cmp_instr->m_rhs_val_use->getValue();
  auto lhs_phi = std::dynamic_pointer_cast<PhiInstruction>(lhs_val);
  auto rhs_phi = std::dynamic_pointer_cast<PhiInstruction>(rhs_val);
  if (lhs_phi != nullptr
      && (rhs_phi == nullptr
          || rhs_phi->GetValue(easy_loop->m_body_bb) == nullptr)) {
    auto it = easy_loop->m_loop_vars_by_phi.find(lhs_phi);  // found
    assert(it != easy_loop->m_loop_vars_by_phi.end());
    easy_loop->m_cond_var = it->second;
    int stride = ComputeStride(easy_loop->m_cond_var, easy_loop->m_body_bb);
    if (stride == 0) return false;
    easy_loop->m_stride = stride;
    return true;
  } else if (rhs_phi != nullptr
             && (lhs_phi == nullptr
                 || lhs_phi->GetValue(easy_loop->m_body_bb) == nullptr)) {
    auto it = easy_loop->m_loop_vars_by_phi.find(rhs_phi);
    assert(it != easy_loop->m_loop_vars_by_phi.end());
    easy_loop->m_cond_var = it->second;
    int stride = ComputeStride(easy_loop->m_cond_var, easy_loop->m_body_bb);
    if (stride == 0) return false;
    easy_loop->m_stride = stride;
    return true;
  } else {
    return false;
  }
}