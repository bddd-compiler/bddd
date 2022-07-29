#include <iostream>
#include <memory>
#include <ctime>

#include "asm/asm-optimization.h"
#include "asm/asm-builder.h"
#include "asm/asm-register.h"
#include "asm/asm-fixed.h"
#include "asm/asm.h"
#include "ast/symbol-table.h"
#include "exceptions.h"
#include "ir/ir-pass-manager.h"
#include "ir/ir.h"
#include "parser/driver.h"

int main(int argc, char **argv) {
  std::string filename;
  if (argc == 2) {
    filename = argv[1];
  } else if (argc == 1) {
    // std::cout << "input source code file: >";
    // std::cin >> filename;
<<<<<<< HEAD
    filename = "../testSource/functional/62_percolation.c";
=======
    // filename = "../testSource/buaa/part12/test1.c";
    filename = "../testSource/functional/86_long_code2.c";
>>>>>>> garen_dev
  } else {
    std::cerr << "???";
    return 1;
  }
  Driver driver;

  std::cout << "compiling: " << filename << std::endl;
  std::cout << "parsing..." << std::endl;
  /**
   * parse the source code into AST
   * ASTs can be accessed via driver.comp_unit
   */
  int res = driver.parse(filename);
  if (res != 0) {
    std::cerr << filename << " GG" << std::endl;
    return 1;
  }

  // std::ofstream ofs1(filename.substr(0, filename.rfind('.')) + "_ast.out");
  // driver.comp_unit->Debug(ofs1, 0);
  // ofs1.close();

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

  // pass_manager->GVNPass();  // global value numbering
  //
  // std::ofstream ofs2(filename.substr(0, filename.rfind('.'))
  //                    + "_ir_example.out");
  // builder->m_module->ExportIR(ofs2, 0);
  // ofs2.close();
  //
  // // but IR after GVN may not be executable since the position is incorrect
  // // (some virtual registers do not dominate all of its uses)
  // // we should hoist these VRs to a proper place, so use GCM
  //
  // pass_manager->GCMPass();

  std::ofstream ofs3(filename.substr(0, filename.rfind('.')) + "_ir.out");
  builder->m_module->ExportIR(ofs3, 0);
  ofs3.close();

  // asm debug
  std::cout << "generating asm..." << std::endl;
  auto asm_module = std::make_shared<ASM_Module>();
  auto asm_builder = std::make_shared<ASM_Builder>(asm_module);
  GenerateModule(std::move(builder->m_module), asm_builder);
  std::ofstream ofs4(filename.substr(0, filename.rfind('.')) + "_tmp_asm.s");
  asm_module->exportASM(ofs4);
  ofs4.close();
  
  std::cout << "allocating..." << std::endl;
  RegisterAllocator(asm_module).Allocate();
  std::ofstream ofs6(filename.substr(0, filename.rfind('.')) + "_asm_1.s");
  asm_module->exportASM(ofs6);
  ofs6.close();

  std::cout << "optimizing..." << std::endl;
  optimize(asm_module);
  generateLiteralPool(asm_module);
  std::ofstream ofs5(filename.substr(0, filename.rfind('.')) + "_asm.s");
  asm_module->exportASM(ofs5);
  ofs5.close();

  return 0;
}
