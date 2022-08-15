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

int main(int argc, char **argv) {
  std::string filename;
  if (argc == 2) {
    filename = argv[1];
  } else if (argc == 1) {
    // std::cout << "input source code file: >";
    // std::cin >> filename;
    // filename = "../testSource/custom_test/loop_unroll2.c";
    filename = "../testSource/hidden_functional/13_LCA.c";
  } else {
    std::cerr << "???";
    return 1;
  }
  Driver driver;

  // std::cout << "compiling: " << filename << std::endl;
  // std::cout << "parsing..." << std::endl;
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

  // std::cout << "generating ir..." << std::endl;
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

  std::ofstream ofs2(filename.substr(0, filename.rfind('.'))
                     + "_ir_example.out");
  builder->m_module->ExportIR(ofs2, 0);
  ofs2.close();

  // pass_manager->EliminateGlobalConstArrayAccess();
  // pass_manager->TailRecursionPass();
  // pass_manager->FunctionInliningPass();
  // pass_manager->LoopUnrollingPass();
  pass_manager->GVNPass();
  pass_manager->GCMPass();

  std::ofstream ofs3(filename.substr(0, filename.rfind('.')) + "_ir.out");
  builder->m_module->ExportIR(ofs3, 0);
  ofs3.close();

  // asm debug
  // std::cout << "generating asm..." << std::endl;
  // auto asm_module = std::make_shared<ASM_Module>();
  // auto asm_builder = std::make_shared<ASM_Builder>(asm_module);
  // GenerateModule(std::move(builder->m_module), asm_builder);
  // std::ofstream ofs4(filename.substr(0, filename.rfind('.')) + "_tmp_asm.s");
  // asm_module->exportASM(ofs4);
  // ofs4.close();
  //
  // std::cout << "allocating..." << std::endl;
  // RegisterAllocator(asm_module, RegType::R).Allocate();
  // RegisterAllocator(asm_module, RegType::S).Allocate();
  //
  // std::cout << "optimizing..." << std::endl;
  // optimize(asm_module);
  // generateLiteralPool(asm_module);
  // std::ofstream ofs5(filename.substr(0, filename.rfind('.')) + "_asm.s");
  // asm_module->exportASM(ofs5);
  // ofs5.close();

  return 0;
}
