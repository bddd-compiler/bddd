#include <iostream>

#include "ast/symbol-table.h"
#include "exceptions.h"
#include "parser/driver.h"

int main() {
  std::cout << "input test file: >";
  std::string filename;
  std::cin >> filename;
  Driver driver;

  /**
   * parse the source code into AST
   * ASTs can be accessed via driver.comp_unit
   */
  int res = driver.parse(filename);

  std::ofstream ofs(filename.substr(0, filename.rfind('.')) + "_ast.out");
  driver.comp_unit->Debug(ofs, 0);

  try {
    SymbolTable symbol_table;
    driver.comp_unit->TypeCheck(symbol_table);
  } catch (MyException &e) {
    std::cerr << "exception encountered: " << e.Msg() << std::endl;
    return 1;
  }

  std::cout << "res: " << res << std::endl;
  return 0;
}
