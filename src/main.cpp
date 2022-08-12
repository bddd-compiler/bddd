#include <getopt.h>

#include <ctime>
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
    = {{"generate-assembly", no_argument, NULL, 'S'},
       {"output", required_argument, NULL, 'o'},
       {"optimization", required_argument, NULL, 'O'},
       {"output-ir", required_argument, NULL, 'i'},
       {NULL, no_argument, NULL, 0}};

int main(int argc, char *argv[]) {
  int ch;
  const char *asm_path = nullptr;
  const char *ir_path = nullptr;
  while ((ch = getopt_long(argc, argv, "So:O:i:", long_options, NULL)) != -1) {
    switch (ch) {
      case 'S':
      case 'O':
        break;
      case 'o':
        asm_path = optarg;
        break;
      case 'i':
        ir_path = optarg;
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

  std::cout << "compiling: " << argv[optind] << std::endl;
  /**
   * parse the source code into AST
   * ASTs can be accessed via driver.comp_unit
   */
  int res = driver.parse(argv[optind]);
  if (res != 0) {
    std::cerr << argv[optind] << " GG" << std::endl;
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
  // pass_manager->Mem2RegPass();  // now no allocas for single variable
  // pass_manager->GVNPass();      // global value numbering
  // but IR after GVN may not be executable since the position is incorrect
  // (some virtual registers do not dominate all of its uses)
  // we should hoist these VRs to a proper place, so use GCM
  // pass_manager->GCMPass();

  if (ir_path) {
    std::ofstream ofs(ir_path);
    builder->m_module->ExportIR(ofs, 0);
    ofs.close();
  }

  // generate assembly
  auto asm_module = std::make_shared<ASM_Module>();
  auto asm_builder = std::make_shared<ASM_Builder>(asm_module);
  GenerateModule(std::move(builder->m_module), asm_builder);

  // register allocator
  RegisterAllocator(asm_module, RegType::R).Allocate();
  RegisterAllocator(asm_module, RegType::S).Allocate();

  // fixing and optimization
  fixedParamsOffs(asm_module);
  optimize(asm_module);
  generateLiteralPool(asm_module);
  std::ofstream ofs(asm_path);
  asm_module->exportASM(ofs);
  ofs.close();

  return 0;
}
