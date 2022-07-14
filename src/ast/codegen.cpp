#include <cassert>

#include "ast/ast.h"
#include "ir/ir.h"

std::shared_ptr<Value> InitValAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  // nothing generated here
  return nullptr;
}

std::shared_ptr<Value> LValAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  assert(m_decl != nullptr);
  // assert(m_decl->m_addr != nullptr);
  assert(IsArray() || IsSingle());

  if (m_decl->m_addr) {
    auto addr = CodeGenAddr(builder);
    if (IsSingle()) {
      return builder->CreateLoadInstruction(addr);
    } else {
      return addr;
    }
  } else {
    // zero initializer
    assert(IsSingle());
    // if single, do nothing
    return nullptr;
  }
}

std::shared_ptr<Value> LValAST::CodeGenAddr(
    std::shared_ptr<IRBuilder> builder) {
  if (m_dimensions.empty()) {
    return m_decl->m_addr;
  }
  // need references

  std::vector<std::shared_ptr<Value>> dimensions;
  for (auto &dimension : m_dimensions) {
    dimensions.push_back(dimension->CodeGen(builder));
  }

  return builder->CreateGetElementPtrInstruction(
      m_decl->m_addr, std::move(dimensions), m_decl->m_products);
}

std::shared_ptr<Value> ExprAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  std::shared_ptr<Value> lhs_val, rhs_val;
  switch (m_op) {
    case Op::LVAL:
      return m_lval->CodeGen(builder);
    case Op::FUNC_CALL:
      return m_func_call->CodeGen(builder);
    case Op::CONST_INT:
      return builder->GetIntConstant(m_int_val);
    case Op::CONST_FLOAT:
      return builder->GetFloatConstant(m_float_val);

    case Op::AND:
      return CodeGenAnd(builder);
    case Op::OR:
      return CodeGenOr(builder);

    case Op::POSITIVE:
      lhs_val = m_lhs->CodeGen(builder);
      return lhs_val;
    case Op::NEGATIVE:
      lhs_val = m_lhs->CodeGen(builder);
      // TODO(garen): A SUPER SERIOUS QUESTION: what if it is float?
      return builder->CreateBinaryInstruction(
          IROp::SUB, builder->GetIntConstant(0), lhs_val);
    case Op::NOT:
      lhs_val = m_lhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::EQ, lhs_val,
                                              builder->GetIntConstant(0));

    case Op::PLUS:
      lhs_val = m_lhs->CodeGen(builder);
      rhs_val = m_rhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::ADD, lhs_val, rhs_val);
    case Op::MINUS:
      lhs_val = m_lhs->CodeGen(builder);
      rhs_val = m_rhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::SUB, lhs_val, rhs_val);
    case Op::MULTI:
      lhs_val = m_lhs->CodeGen(builder);
      rhs_val = m_rhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::MUL, lhs_val, rhs_val);
    case Op::DIV:
      lhs_val = m_lhs->CodeGen(builder);
      rhs_val = m_rhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::SDIV, lhs_val, rhs_val);
    case Op::MOD:
      lhs_val = m_lhs->CodeGen(builder);
      rhs_val = m_rhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::SREM, lhs_val, rhs_val);
    case Op::GEQ:
      lhs_val = m_lhs->CodeGen(builder);
      rhs_val = m_rhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::SGEQ, lhs_val, rhs_val);
    case Op::GE:
      lhs_val = m_lhs->CodeGen(builder);
      rhs_val = m_rhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::SGE, lhs_val, rhs_val);
    case Op::LEQ:
      lhs_val = m_lhs->CodeGen(builder);
      rhs_val = m_rhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::SLEQ, lhs_val, rhs_val);
    case Op::LE:
      lhs_val = m_lhs->CodeGen(builder);
      rhs_val = m_rhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::SLE, lhs_val, rhs_val);
    case Op::EQ:
      lhs_val = m_lhs->CodeGen(builder);
      rhs_val = m_rhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::EQ, lhs_val, rhs_val);
    case Op::NEQ:
      lhs_val = m_lhs->CodeGen(builder);
      rhs_val = m_rhs->CodeGen(builder);
      return builder->CreateBinaryInstruction(IROp::NE, lhs_val, rhs_val);
    default:
      assert(false);  // unreachable
  }
}

std::shared_ptr<Value> ExprAST::CodeGenAnd(std::shared_ptr<IRBuilder> builder) {
  auto old_if_then = builder->m_if_true;
  builder->m_if_true = std::make_shared<BasicBlock>("and.true");

  auto lhs_val = m_lhs->CodeGen(builder);
  builder->CreateBranchInstruction(lhs_val, builder->m_if_true,
                                   builder->m_if_false);
  builder->AppendBasicBlock(builder->m_if_true);

  builder->m_if_true = old_if_then;
  return m_rhs->CodeGen(builder);
}

std::shared_ptr<Value> ExprAST::CodeGenOr(std::shared_ptr<IRBuilder> builder) {
  auto old_if_else = builder->m_if_false;
  builder->m_if_false = std::make_shared<BasicBlock>("or.false");

  auto lhs_val = m_lhs->CodeGen(builder);
  builder->CreateBranchInstruction(lhs_val, builder->m_if_true,
                                   builder->m_if_false);
  builder->AppendBasicBlock(builder->m_if_false);

  builder->m_if_false = old_if_else;
  return m_rhs->CodeGen(builder);
}

std::shared_ptr<Value> DeclAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  if (IsConst()) {
    if (IsArray()) {
      assert(false);  // TODO(garen):
    } else {
      auto op = m_flatten_vals[0]->GetOp();
      if (op == Op::CONST_INT)
        return builder->GetIntConstant(m_flatten_vals[0]->IntVal());
      else if (op == Op::CONST_FLOAT)
        return builder->GetFloatConstant(m_flatten_vals[0]->FloatVal());
      else
        assert(false);  // unreachable
    }
  }
  if (IsArray()) {
    if (m_init_val) {
      // TODO(garen): whether to use memset (but not included in sylib?)

      // TODO(garen): store initval into m_addr
    }
    auto addr = builder->CreateAllocaInstruction(shared_from_base<DeclAST>());
    return addr;
  } else {
    // single
    if (m_init_val) {
      assert(m_flatten_vals.size() == 1);
      auto val = m_flatten_vals[0]->CodeGen(builder);
      auto addr
          = builder->CreateAllocaInstruction(shared_from_base<DeclAST>(), val);
      m_addr = addr;
      builder->CreateStoreInstruction(addr, val);
      return addr;
    } else {
      auto addr = builder->CreateAllocaInstruction(shared_from_base<DeclAST>());
      m_addr = addr;
      return addr;
    }
  }
}

std::shared_ptr<Value> FuncCallAST::CodeGen(
    std::shared_ptr<IRBuilder> builder) {
  for (const auto &it : builder->m_module->m_function_list) {
    if (it->FuncName() == m_func_name) {
      return builder->CreateCallInstruction(shared_from_base<FuncCallAST>());
    }
  }
  return nullptr;
}

std::shared_ptr<Value> CondAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  return m_expr->CodeGen(builder);
}

std::shared_ptr<Value> FuncFParamAST::CodeGen(
    std::shared_ptr<IRBuilder> builder) {
  // nothing generated here
  return nullptr;
}

std::shared_ptr<Value> BlockAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  for (auto &node : m_nodes) {
    node->CodeGen(builder);
  }
  return nullptr;
}

std::shared_ptr<Value> FuncDefAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  std::vector<std::unique_ptr<FunctionArg>> args;

  auto func = builder->CreateFunction(shared_from_base<FuncDefAST>());

  builder->CreateBasicBlock("function.prehead");
  m_block->CodeGen(builder);
  return nullptr;
}

std::shared_ptr<Value> AssignStmtAST::CodeGen(
    std::shared_ptr<IRBuilder> builder) {
  auto val = m_rhs->CodeGen(builder);

  auto addr = m_lval->CodeGenAddr(builder);
  return builder->CreateStoreInstruction(addr, val);
}

std::shared_ptr<Value> EvalStmtAST::CodeGen(
    std::shared_ptr<IRBuilder> builder) {
  m_expr->CodeGen(builder);
  return nullptr;
}

std::shared_ptr<Value> IfStmtAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  auto old_if_true = builder->m_if_true, old_if_false = builder->m_if_false,
       old_if_exit = builder->m_if_exit;
  builder->m_if_true = std::make_shared<BasicBlock>("if.then");
  builder->m_if_exit = std::make_shared<BasicBlock>("if.finally");
  if (m_else) {
    builder->m_if_false = std::make_shared<BasicBlock>("if.else");
  } else {
    builder->m_if_false = builder->m_if_exit;
  }

  // codegen begin
  auto cond_val = m_cond->CodeGen(builder);
  builder->CreateBranchInstruction(cond_val, builder->m_if_true,
                                   builder->m_if_false);
  builder->AppendBasicBlock(builder->m_if_true);
  m_then->CodeGen(builder);
  builder->CreateJumpInstruction(builder->m_if_exit);
  if (m_else) {
    builder->AppendBasicBlock(builder->m_if_false);
    m_else->CodeGen(builder);
    builder->CreateJumpInstruction(builder->m_if_exit);
  }
  builder->AppendBasicBlock(builder->m_if_exit);
  // codegen end

  builder->m_if_true = old_if_true;
  builder->m_if_false = old_if_false;
  builder->m_if_exit = old_if_exit;
  return nullptr;
}

std::shared_ptr<Value> ReturnStmtAST::CodeGen(
    std::shared_ptr<IRBuilder> builder) {
  if (m_ret) {
    auto ret_val = m_ret->CodeGen(builder);
    builder->CreateReturnInstruction(ret_val);
  } else {
    builder->CreateReturnInstruction();
  }
  return nullptr;
}

std::shared_ptr<Value> WhileStmtAST::CodeGen(
    std::shared_ptr<IRBuilder> builder) {
  auto old_while_entry = builder->m_while_entry,
       old_while_header = builder->m_while_header,
       old_while_exit = builder->m_while_exit;
  builder->m_while_entry = std::make_shared<BasicBlock>("while.prehead");
  builder->m_while_header = std::make_shared<BasicBlock>("while.do");
  builder->m_while_exit = std::make_shared<BasicBlock>("while.finally");

  // codegen begin
  builder->AppendBasicBlock(builder->m_while_entry);

  auto cond_val = m_cond->CodeGen(builder);
  builder->CreateBranchInstruction(cond_val, builder->m_while_header,
                                   builder->m_while_exit);
  builder->AppendBasicBlock(builder->m_while_header);
  m_stmt->CodeGen(builder);
  builder->CreateJumpInstruction(builder->m_while_entry);
  builder->AppendBasicBlock(builder->m_while_exit);
  // codegen end

  builder->m_while_entry = old_while_entry;
  builder->m_while_header = old_while_header;
  builder->m_while_exit = old_while_exit;
  return nullptr;
}

std::shared_ptr<Value> BreakStmtAST::CodeGen(
    std::shared_ptr<IRBuilder> builder) {
  builder->CreateJumpInstruction(builder->m_while_exit);
  builder->CreateBasicBlock("after_break");
  return nullptr;
}

std::shared_ptr<Value> ContinueStmtAST::CodeGen(
    std::shared_ptr<IRBuilder> builder) {
  builder->CreateJumpInstruction(builder->m_while_entry);
  builder->CreateBasicBlock("after_continue");
  return nullptr;
}

std::shared_ptr<Value> CompUnitAST::CodeGen(
    std::shared_ptr<IRBuilder> builder) {
  for (auto &node : m_nodes) {
    if (auto decl = std::dynamic_pointer_cast<DeclAST>(node)) {
      builder->CreateGlobalVariable(decl);
    } else if (auto func = std::dynamic_pointer_cast<FuncDefAST>(node)) {
      func->CodeGen(builder);
    }
  }
  return nullptr;  // m_module's codegen return nullptr
}
