#include <cassert>

#include "ast/ast.h"
#include "ir/ir.h"

std::shared_ptr<Value> InitValAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  assert(false);  // nothing generated here
  return nullptr;
}

std::shared_ptr<Value> LValAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  assert(m_decl != nullptr);
  // assert(m_decl->m_addr != nullptr);
  assert(IsArray() || IsSingle());

  if (m_decl->m_addr) {
    auto addr = CodeGenAddr(builder);
    if (IsSingle()) {
      if (addr->m_type.m_num_star > 0)
        return builder->CreateLoadInstruction(addr);
      else
        return addr;
    } else {
      return addr;
    }
  } else {
    // function argument
    // for (auto &dimension : m_indices) {
    //   builder->CreateGetElementPtrInstruction(0, dimension->IntVal())
    // }
    // if single, load the value
    assert(false);
  }
}

std::shared_ptr<Value> LValAST::CodeGenAddr(
    std::shared_ptr<IRBuilder> builder) {
  if (m_indices.empty()) {
    return m_decl->m_addr;
  }
  // need references

  // assert(m_decl->m_addr->m_type.m_num_star > 1);
  // TODO(garen): not guaranteed to work correctly
  auto addr = m_decl->m_addr;
  int cnt = 0;
  while (addr->m_type.m_num_star > 1) {
    assert(addr != nullptr);
    addr = builder->CreateLoadInstruction(addr);
    ++cnt;
  }
  // assert(cnt <= 1);
  std::vector<std::shared_ptr<Value>> indices;
  if (cnt < 1) indices.push_back(builder->GetIntConstant(0));
  for (auto &index : m_indices) {
    indices.push_back(index->CodeGen(builder));
  }
  return builder->CreateGetElementPtrInstruction(addr, std::move(indices));
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
      // return builder->CreateBinaryInstruction(IROp::EQ, lhs_val,
      //                                         builder->GetIntConstant(0));
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
    if (!m_is_param) {
      auto addr = builder->CreateAllocaInstruction(shared_from_base<DeclAST>());
      std::vector<std::shared_ptr<Value>> first_indices;
      first_indices.push_back(builder->GetIntConstant(0));
      for (size_t i = 0; i < m_dimensions.size(); ++i) {
        first_indices.push_back(builder->GetIntConstant(0));
      }
      // invoke memset to zero-initialize local array
      auto first_elem = builder->CreateGetElementPtrInstruction(
          addr, std::move(first_indices));
      auto size = m_products[0];
      std::vector<std::shared_ptr<Value>> params
          = {first_elem, builder->GetIntConstant(0),
             builder->GetIntConstant(4 * size)};
      builder->CreateCallInstruction(VarType::VOID, "memset",
                                     std::move(params));

      if (m_init_val) {
        assert(m_var_type == VarType::INT);  // float is not implemented yet
        for (int i = 0; i < m_flatten_vals.size(); ++i) {
          if (m_flatten_vals[i] != nullptr) {
            auto elem_addr = builder->CreateGetElementPtrInstruction(
                first_elem, {builder->GetIntConstant(i)});
            builder->CreateStoreInstruction(
                elem_addr, m_flatten_vals[i]->CodeGen(builder));
          }
        }
      }
      return m_addr = addr;
    } else {
      assert(m_dimensions[0] == nullptr);
      // actually, we should not allocate an array but a fat pointer
      auto addr = builder->CreateAllocaInstruction(shared_from_base<DeclAST>());
      return m_addr = addr;
    }
  } else {
    // single
    if (m_init_val) {
      assert(m_flatten_vals.size() == 1);
      auto val = m_flatten_vals[0]->CodeGen(builder);
      auto addr
          = builder->CreateAllocaInstruction(shared_from_base<DeclAST>(), val);
      builder->CreateStoreInstruction(addr, val);
      return m_addr = addr;
    } else {
      auto addr = builder->CreateAllocaInstruction(shared_from_base<DeclAST>());
      return m_addr = addr;
    }
  }
}

std::shared_ptr<Value> FuncCallAST::CodeGen(
    std::shared_ptr<IRBuilder> builder) {
  // search from declarations
  auto it = std::find_if(builder->m_module->m_function_decl_list.begin(),
                         builder->m_module->m_function_decl_list.end(),
                         [=](const std::shared_ptr<Function> &ptr) {
                           return ptr->FuncName() == m_func_name;
                         });
  if (it == builder->m_module->m_function_decl_list.end()) {
    it = std::find_if(builder->m_module->m_function_list.begin(),
                      builder->m_module->m_function_list.end(),
                      [=](const std::shared_ptr<Function> &ptr) {
                        return ptr->FuncName() == m_func_name;
                      });
    assert(it != builder->m_module->m_function_list.end());
  }
  auto ptr = *it;
  std::vector<Use> param_uses;
  assert(m_params.size() == ptr->m_args.size());
  for (int i = 0; i < m_params.size(); ++i) {
    auto val = m_params[i]->CodeGen(builder);
    if (val->m_type.m_dimensions.size()
            > ptr->m_args[i]->m_type.m_dimensions.size()
        && val->m_type.m_num_star == ptr->m_args[i]->m_type.m_num_star) {
      // fat pointer to thin pointer
      std::vector<std::shared_ptr<Value>> gep_params(
          val->m_type.m_dimensions.size() + 1
              - ptr->m_args[i]->m_type.m_dimensions.size(),
          builder->GetIntConstant(0));
      val = builder->CreateGetElementPtrInstruction(val, std::move(gep_params));
    }
    assert(val->m_type == ptr->m_args[i]->m_type);

    auto use = Use(val);
    param_uses.push_back(use);
  }
  auto ret
      = builder->CreateCallInstruction(m_func_def->ReturnType(), m_func_name);
  for (auto &param : param_uses) {
    param.SetUser(ret);
  }
  ret->SetParams(std::move(param_uses));
  return ret;
}

std::shared_ptr<Value> CondAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  return m_expr->CodeGen(builder);
}

std::shared_ptr<Value> FuncFParamAST::CodeGen(
    std::shared_ptr<IRBuilder> builder) {
  return m_decl->CodeGen(builder);
}

std::shared_ptr<Value> BlockAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  for (auto &node : m_nodes) {
    node->CodeGen(builder);
  }
  return nullptr;
}

std::shared_ptr<Value> FuncDefAST::CodeGen(std::shared_ptr<IRBuilder> builder) {
  auto func = builder->CreateFunction(shared_from_base<FuncDefAST>());
  builder->CreateBasicBlock("function.prehead");
  // TOOD(garen): add arguments as local variables here
  assert(m_params.size() == func->m_args.size());
  for (int i = 0; i < m_params.size(); ++i) {
    auto addr = m_params[i]->CodeGen(builder);
    builder->CreateStoreInstruction(addr, func->m_args[i]);
  }
  m_block->CodeGen(builder);
  auto bb = builder->m_module->GetCurrentBB();
  if (!bb->LastInstruction()->IsTerminator()) {
    switch (m_return_type) {
      case VarType::INT:
        builder->CreateReturnInstruction(builder->GetIntConstant(0));
        break;
      case VarType::FLOAT:
        builder->CreateReturnInstruction(builder->GetFloatConstant(0.0));
        break;
      case VarType::VOID:
        builder->CreateReturnInstruction();
        break;
      default:
        assert(false);
    }
  }
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
  builder->CreateJumpInstruction(builder->m_while_entry);  // terminator

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
