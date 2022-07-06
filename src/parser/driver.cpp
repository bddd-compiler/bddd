#include "parser/driver.h"

Driver::Driver(): trace_scanning(false), trace_parsing(false) {
    compUnit = std::make_unique<CompUnit>();
}

Driver::~Driver() {}

int Driver::parse(const std::string &f) {
    file = f;
    scan_begin();
    yy::parser parser(*this);
    parser.set_debug_level(trace_parsing);
    int res = parser.parse();
    scan_end();
    return res;
}

void Driver::error(const yy::location &l, const std::string &m) {
    if (m.find("expecting identifier") != std::string::npos) {
        printf("[lexical error] invalid identifier\n");
    } else if (m.find("expecting end of file") != std::string::npos) {
        printf("[lexical error] missing */ in block comment\n");
    } else if (m.find("unexpected int const") != std::string::npos) {
        printf("[lexical error] invalid number for int const\n");
    } else if (m.find("unexpected float const") != std::string::npos) {
        printf("[lexical error] invalid number for float const\n");
    } else {

    }
    std::cerr << l << ": " << m << std::endl;
}

void Driver::error(const std::string &m) {
    std::cerr << m << std::endl;
}