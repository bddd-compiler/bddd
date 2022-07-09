#include "ast/ast.h"
#include "ir/ir.h"

std::shared_ptr<Value> InitValAST::CodeGen(IRBuilder &builder) {
  // TODO(garen): nothing generated here
  return nullptr;
}

std::shared_ptr<Value> LValAST::CodeGen(IRBuilder &builder) {
  // TODO(garen): nothing generated here
  return nullptr;
}

std::shared_ptr<Value> LValAST::CodeGenGEP(IRBuilder &builder) {
  // TODO(garen): nothing generated here
  return nullptr;
}

std::shared_ptr<Value> ExprAST::CodeGen(IRBuilder &builder) {
  if (m_op == Op::LVAL)
    return m_lval->CodeGen(builder);
  else if (m_op == Op::FUNC_CALL)
    return m_func_call->CodeGen(builder);
  else if (m_op == Op::AND)
    return CodeGenAnd(builder);
  else if (m_op == Op::OR)
    return CodeGenOr(builder);
  else if (m_op == Op::CONST_INT)
    return builder.CreateConstant(m_int_val);
  else if (m_op == Op::CONST_FLOAT)
    return builder.CreateConstant(m_float_val);

  // TODO(garen): codegen of ExprAST has not started yet
  return nullptr;
}

std::shared_ptr<Value> ExprAST::CodeGenAnd(IRBuilder &builder) {
  // TODO(garen): codegen for AND operation of ExprAST has not started yet
  return nullptr;
}

std::shared_ptr<Value> ExprAST::CodeGenOr(IRBuilder &builder) {
  // TODO(garen): codegen for OR operation of ExprAST has not started yet
  return nullptr;
}

std::shared_ptr<Value> DeclAST::CodeGen(IRBuilder &builder) {
  // TODO(garen): codegen of DeclAST has not started yet
  return nullptr;
}

std::shared_ptr<Value> FuncCallAST::CodeGen(IRBuilder &builder) {
  for (const auto &it : builder.module.function_list) {
    if (it->funcName() == m_func_name) {
      // TODO(garen): create call instruction
    }
  }
  return nullptr;
}

std::shared_ptr<Value> CondAST::CodeGen(IRBuilder &builder) {
  return m_expr->CodeGen(builder);
}

std::shared_ptr<Value> FuncFParamAST::CodeGen(IRBuilder &builder) {
  // TODO(garen): nothing generated here
  return nullptr;
}

std::shared_ptr<Value> BlockAST::CodeGen(IRBuilder &builder) {
  for (auto &node : m_nodes) {
    node->CodeGen(builder);
  }
  return nullptr;
}

std::shared_ptr<Value> FuncDefAST::CodeGen(IRBuilder &builder) {
  builder.CreateFunction(m_func_name, m_return_type);
  // TODO(garen): how to deal with params of function?

  builder.CreateBasicBlock("function.preheader");
  m_block->CodeGen(builder);
  return nullptr;
}

std::shared_ptr<Value> AssignStmtAST::CodeGen(IRBuilder &builder) {
  auto val = m_rhs->CodeGen(builder);
  if (m_lval->IsArray()) {
    auto addr = m_lval->CodeGenGEP(builder);
    return builder.CreateStoreInstruction(addr, val);
  } else {
    // TODO(garen): what to do next?
  }
  return val;
}

std::shared_ptr<Value> EvalStmtAST::CodeGen(IRBuilder &builder) {
  m_expr->CodeGen(builder);
  return nullptr;
}

std::shared_ptr<Value> IfStmtAST::CodeGen(IRBuilder &builder) {
  auto old_if_then = builder.if_then, old_if_else = builder.if_else,
       old_if_finally = builder.if_finally;
  builder.if_then = std::make_shared<BasicBlock>("if.then");
  builder.if_finally = std::make_shared<BasicBlock>("if.finally");
  if (m_else) {
    builder.if_else = std::make_shared<BasicBlock>("if.else");
  } else {
    builder.if_else = builder.if_finally;
  }

  // codegen begin
  auto cond_val = m_cond->CodeGen(builder);
  builder.CreateBranchInstruction(cond_val, builder.if_then, builder.if_else);
  builder.AppendBlock(builder.if_then);
  m_then->CodeGen(builder);
  builder.CreateJumpInstruction(builder.if_finally);
  if (m_else) {
    builder.AppendBlock(builder.if_else);
    m_else->CodeGen(builder);
    builder.CreateJumpInstruction(builder.if_finally);
  }
  builder.AppendBlock(builder.if_finally);
  // codegen end

  builder.if_then = old_if_then;
  builder.if_else = old_if_else;
  builder.if_finally = old_if_finally;
  return nullptr;
}

std::shared_ptr<Value> ReturnStmtAST::CodeGen(IRBuilder &builder) {
  if (m_ret) {
    auto ret_val = m_ret->CodeGen(builder);
    builder.CreateReturnInstruction(ret_val);
  } else {
    builder.CreateReturnInstruction();
  }
  return nullptr;
}

std::shared_ptr<Value> WhileStmtAST::CodeGen(IRBuilder &builder) {
  auto old_while_preheader = builder.while_preheader,
       old_while_do = builder.while_do,
       old_while_finally = builder.while_finally;
  builder.while_preheader = std::make_shared<BasicBlock>("while.preheader");
  builder.while_do = std::make_shared<BasicBlock>("while.do");
  builder.while_finally = std::make_shared<BasicBlock>("while.finally");

  // codegen begin
  builder.AppendBlock(builder.while_preheader);

  auto cond_val = m_cond->CodeGen(builder);
  builder.CreateBranchInstruction(cond_val, builder.while_do,
                                  builder.while_finally);
  builder.AppendBlock(builder.while_do);
  m_stmt->CodeGen(builder);
  builder.CreateJumpInstruction(builder.while_preheader);
  builder.AppendBlock(builder.while_finally);
  // codegen end

  builder.while_preheader = old_while_preheader;
  builder.while_do = old_while_do;
  builder.while_finally = old_while_finally;
  return nullptr;
}

std::shared_ptr<Value> BreakStmtAST::CodeGen(IRBuilder &builder) {
  builder.CreateJumpInstruction(builder.while_finally);
  builder.CreateBasicBlock("after_break");
  return nullptr;
}

std::shared_ptr<Value> ContinueStmtAST::CodeGen(IRBuilder &builder) {
  builder.CreateJumpInstruction(builder.while_preheader);
  builder.CreateBasicBlock("after_continue");
  return nullptr;
}

std::shared_ptr<Value> CompUnitAST::CodeGen(IRBuilder &builder) {
  for (auto &node : m_nodes) {
    if (auto decl = std::dynamic_pointer_cast<DeclAST>(node)) {
      builder.CreateGlobalValue(decl);
    } else if (auto func = std::dynamic_pointer_cast<FuncDefAST>(node)) {
      func->CodeGen(builder);
    }
  }
  return nullptr;  // module's codegen return nullptr
}
