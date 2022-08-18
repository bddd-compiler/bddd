#include <getopt.h>

#include <iostream>
#include <memory>

#include "asm/asm-builder.h"
#include "asm/asm-fixed.h"
#include "asm/asm-optimization.h"
#include "asm/asm-register.h"
#include "asm/asm.h"
#include "ast/symbol-table.h"
#include "exceptions.h"
#include "ir/ir-pass-manager.h"
#include "ir/ir.h"
#include "parser/driver.h"

static const struct option long_options[]
    = {{"generate-assembly", no_argument, nullptr, 'S'},
       {"output", required_argument, nullptr, 'o'},
       {"optimization", required_argument, nullptr, 'O'},
       {"output-ir", required_argument, nullptr, 'i'},
       {"output-tmp-asm", required_argument, nullptr, 't'},
       {nullptr, no_argument, nullptr, 0}};

int main(int argc, char *argv[]) {
  int ch;
  bool optimization = false;
  const char *asm_path = nullptr;
  const char *ir_path = nullptr;
  const char *tmp_asm_path = nullptr;
  while ((ch = getopt_long(argc, argv, "So:O:i:t:", long_options, NULL))
         != -1) {
    switch (ch) {
      case 'S':
        break;
      case 'O':
        optimization = true;
        break;
      case 'o':
        asm_path = optarg;
        break;
      case 'i':
        ir_path = optarg;
        break;
      case 't':
        tmp_asm_path = optarg;
        break;
      default:
        return -1;
    }
  }

  Driver driver;

  if (optind == argc) {
    std::cerr << "no input file" << std::endl;
    return 0;
  }

  auto src_path = argv[optind];
  std::cout << "compiling: " << src_path << std::endl;
  std::cout << "parsing..." << std::endl;
  /**
   * parse the source code into AST
   * ASTs can be accessed via driver.comp_unit
   */
  int res = driver.parse(src_path);
  if (res != 0) {
    std::cerr << src_path << " GG" << std::endl;
    return 1;
  }
  InitBuiltinFunctions();
  try {
    SymbolTable symbol_table(g_builtin_funcs);
    driver.comp_unit->TypeCheck(symbol_table);
  } catch (MyException &e) {
    std::cerr << "exception during typechecking: " << e.Msg() << std::endl;
    return 1;
  }

  std::cout << "generating ir..." << std::endl;

  auto module = std::make_unique<Module>();
  for (auto &builtin_func : g_builtin_funcs) {
    auto func_decl = std::make_shared<Function>(builtin_func);
    module->AppendFunctionDecl(std::move(func_decl));
  }
  auto builder = std::make_shared<IRBuilder>(std::move(module));
  try {
    driver.comp_unit->CodeGen(builder);
  } catch (MyException &e) {
    std::cerr << "exception during codegen: " << e.Msg() << std::endl;
    return 1;
  }
  builder->m_module->RemoveInstrsAfterTerminator();

  auto pass_manager = std::make_unique<IRPassManager>(builder);
  pass_manager->Mem2RegPass();  // now no allocas for single variable
  pass_manager->EliminateGlobalConstArrayAccess();
  pass_manager->TailRecursionPass();
  pass_manager->FunctionInliningPass();
  pass_manager->LoadStoreOptimizationPass();
  pass_manager->LoopUnrollingPass();
  pass_manager->GVNPass();
  pass_manager->GCMPass();
  
  if (optimization) {
    pass_manager->InstrCombiningPass();
    pass_manager->LoopSimplifyPass();
  }

  if (ir_path) {
    std::ofstream ofs(ir_path);
    builder->m_module->ExportIR(ofs, 0);
    ofs.close();
  }

  if (tmp_asm_path == nullptr && asm_path == nullptr) return 0;
  std::cout << "generating asm..." << std::endl;

  // generate assembly
  auto asm_module = std::make_shared<ASM_Module>();
  auto asm_builder = std::make_shared<ASM_Builder>(asm_module);
  GenerateModule(std::move(builder->m_module), asm_builder);

  if (tmp_asm_path) {
    std::ofstream ofs(tmp_asm_path);
    asm_module->exportASM(ofs);
    ofs.close();
  }

  // optimize for temp asm
  optimizeTemp(asm_module, optimization);

  std::cout << "allocating..." << std::endl;

  // register allocator
  RegisterAllocator(asm_module, RegType::S).Allocate();
  RegisterAllocator(asm_module, RegType::R).Allocate();

  // fixing and optimization
  fixedParamsOffs(asm_module);
  optimize(asm_module);
  generateLiteralPool(asm_module);
  std::ofstream ofs(asm_path);
  asm_module->exportASM(ofs);
  ofs.close();

  return 0;
}
