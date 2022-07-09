#include <iostream>

#include "ast/symbol-table.h"
#include "exceptions.h"
#include "parser/driver.h"

int main(int argc, char **argv) {
  std::string filename;
  if (argc == 2) {
    filename = argv[1];
  } else if (argc == 1) {
    // std::cout << "input source code file: >";
    // std::cin >> filename;
    filename = "../tests/functional/55_sort_test1.sy";
  } else {
    std::cerr << "???";
    return 1;
  }
  Driver driver;

  /**
   * parse the source code into AST
   * ASTs can be accessed via driver.comp_unit
   */
  int res = driver.parse(filename);
  if (res != 0) {
    std::cerr << "GG" << std::endl;
    return 1;
  }

  std::ofstream ofs("ast.out");
  driver.comp_unit->Debug(ofs, 0);

  try {
    InitBuiltinFunctions();
    SymbolTable symbol_table(g_builtin_funcs);
    driver.comp_unit->TypeCheck(symbol_table);
  } catch (MyException &e) {
    std::cerr << "exception encountered: " << e.Msg() << std::endl;
    return 1;
  }

  return 0;
}
