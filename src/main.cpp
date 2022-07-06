#include "parser/driver.h"

#include <iostream>

int main() {
    std::cout << "input test file: >";
    std::string filename;
    std::cin >> filename;
    Driver driver;
    int res = driver.parse(filename);
    try {
        bool checkRes = driver.compUnit->typecheck();
        if (checkRes) {
            driver.compUnit->codegen();
        } else {

        }
    } catch (GrammarException &e) {
        std::cerr << e << std::endl;
        return 1;
    }
    return 0;
}