CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
PROJECT(bddd VERSION 0.1)
SET(CMAKE_CXX_STANDARD 17)

include_directories(include)

FIND_PACKAGE(BISON)
FIND_PACKAGE(FLEX)

BISON_TARGET(MyParser
  src/parser/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hh
  )

FLEX_TARGET(
  MyScanner
  src/parser/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lexer.hh
)

ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)
add_executable(bddd
  src/main.cpp

  ${BISON_MyParser_OUTPUTS} ${FLEX_MyScanner_OUTPUTS} src/parser/driver.cpp
  src/ast/ast.cpp src/ast/symbol-table.cpp src/ast/type-check.cpp src/ast/debug.cpp src/ast/codegen.cpp
  src/ir/ir.cpp
  src/ir/export.cpp src/ir/builder.cpp src/ir/pass/mem2reg.cpp src/ir/ir-name-allocator.cpp src/ir/pass/function-inlining.cpp src/ir/pass/gvn.cpp src/ir/pass/gcm.cpp src/ir/pass/dead-code-elimination.cpp src/ir/pass/side-effect.cpp src/ir/pass/dom.cpp src/ir/pass/rpo.cpp src/ir/pass/basic.cpp src/ir/pass/tail-recursion.cpp)

target_include_directories(bddd PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
