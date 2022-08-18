//
// Created by garen on 8/3/22.
//

#ifndef BDDD_LOOP_H
#define BDDD_LOOP_H

#include "ir/ir.h"

class Loop {
public:
  std::unordered_set<std::shared_ptr<BasicBlock>> m_preheaders;
  std::shared_ptr<BasicBlock> m_header;
  std::set<std::shared_ptr<BasicBlock>> m_bbs;
  std::set<std::shared_ptr<Loop>> m_sub_loops;
  std::shared_ptr<Loop> m_fa_loop;

  std::set<std::shared_ptr<BasicBlock>> m_latches;
  std::unordered_set<std::shared_ptr<BasicBlock>> m_exiting_bbs;  // 可能有多个
  std::unordered_set<std::shared_ptr<BasicBlock>> m_exit_bbs;  // 可能有多个

  int m_loop_depth;
  bool m_visited;

  explicit Loop() : m_loop_depth(-1), m_visited(false) {}

  explicit Loop(std::shared_ptr<BasicBlock> header)
      : m_header(std::move(header)), m_loop_depth(-1), m_visited(false) {}

  // not nullptr when have only one preheader (see LoopSimplifyPass)
  std::shared_ptr<BasicBlock> Preheader() const {
    if (m_preheaders.size() == 1)
      return *m_preheaders.begin();
    else
      return nullptr;
  }
  // not nullptr when have only one latch (see LoopSimplifyPass)
  std::shared_ptr<BasicBlock> Latch() const {
    if (m_latches.size() == 1)
      return *m_latches.begin();
    else
      return nullptr;
  }
};

// loop var is a variable inside a easy loop
class LoopVar {
public:
  std::shared_ptr<Loop> m_loop;
  std::shared_ptr<PhiInstruction> m_def_instr;  // the phi instruction
  std::shared_ptr<Value> m_init_val;            // incoming value from preheader
  std::shared_ptr<Value> m_body_val;            // incoming value from body bb

  explicit LoopVar() {}

  explicit LoopVar(std::shared_ptr<Loop> loop,
                   std::shared_ptr<PhiInstruction> phi,
                   std::shared_ptr<BasicBlock> body_bb) {
    assert(loop->m_preheaders.size() == 1);
    m_loop = loop;
    m_def_instr = phi;
    m_init_val = m_def_instr->GetValue(loop->Preheader());
    m_body_val = m_def_instr->GetValue(body_bb);
  }
};

// easy loop: m_bbs only contains cond block and body block
class EasyLoop {
public:
  std::shared_ptr<Loop> m_loop;
  std::shared_ptr<BasicBlock> m_cond_bb;
  std::shared_ptr<BasicBlock> m_body_bb;
  std::shared_ptr<BinaryInstruction> m_cmp_instr;
  std::shared_ptr<BranchInstruction> m_br_instr;
  std::shared_ptr<LoopVar> m_cond_var;
  std::vector<std::shared_ptr<PhiInstruction>> m_phis;

  std::map<std::shared_ptr<Value>, std::shared_ptr<LoopVar>> m_loop_vars_by_phi;
  std::map<std::shared_ptr<Value>, std::vector<std::shared_ptr<LoopVar>>>
      m_loop_vars_by_val;  // a value in body bb may be used by some loop vars
                           // in the header bb, so record them
  int m_stride;
  int m_cnt;

  explicit EasyLoop() : m_stride(0), m_cnt(-1) {}

  explicit EasyLoop(std::shared_ptr<Loop> loop) : EasyLoop() {
    m_loop = loop;
    m_cond_bb = loop->m_header;
    assert(loop->m_bbs.size() == 2);
    for (auto &bb : loop->m_bbs) {
      if (bb != loop->m_header) {
        m_body_bb = bb;
      }
    }

    auto br = std::dynamic_pointer_cast<BranchInstruction>(
        m_cond_bb->LastInstruction());
    assert(br != nullptr);
    m_br_instr = br;
    auto cmp = std::dynamic_pointer_cast<BinaryInstruction>(
        m_br_instr->m_cond->getValue());
    assert(cmp != nullptr);
    m_cmp_instr = cmp;

    for (auto &instr : m_cond_bb->m_instr_list) {
      if (auto phi = std::dynamic_pointer_cast<PhiInstruction>(instr)) {
        m_phis.push_back(phi);
        auto loop_var = std::make_shared<LoopVar>(loop, phi, m_body_bb);
        m_loop_vars_by_phi[phi] = loop_var;
        m_loop_vars_by_val[phi->GetValue(m_body_bb)].push_back(loop_var);
      }
    }
  }

  std::shared_ptr<PhiInstruction> GetPhi(std::shared_ptr<EasyLoop> easy_loop,
                                         std::shared_ptr<PhiInstruction> phi) {
    auto it
        = std::find(easy_loop->m_phis.begin(), easy_loop->m_phis.end(), phi);
    if (it == easy_loop->m_phis.end()) return nullptr;
    auto dis = std::distance(easy_loop->m_phis.begin(), it);
    assert(dis < m_phis.size());
    return m_phis[dis];
  }
};

int ComputeCnt(IROp op, int i, int n, int stride);
int ComputeStride(std::shared_ptr<LoopVar> loop_var,
                  std::shared_ptr<BasicBlock> bb);

bool AnalyzeConstLoop(std::shared_ptr<EasyLoop> loop_ir);
bool AnalyzeFlexibleLoop(std::shared_ptr<EasyLoop> easy_loop);

#endif  // BDDD_LOOP_H
