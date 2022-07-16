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

set(PARSER_SOURCE
  ${BISON_MyParser_OUTPUTS}
  ${FLEX_MyScanner_OUTPUTS}
  src/parser/driver.cpp
)

set(AST_SOURCE
  src/ast/ast.cpp
  src/ast/symbol-table.cpp
  src/ast/type-check.cpp
  src/ast/debug.cpp
  src/ast/codegen.cpp
)

set(IR_SOURCE
  src/ir/ir.cpp
  src/ir/export.cpp
)

set(ASM_SOURCE
  src/asm/asm.cpp
)

add_executable(bddd
  src/main.cpp
  ${PARSER_SOURCE}
  ${AST_SOURCE}
  ${IR_SOURCE}
  ${ASM_SOURCE}
)

target_include_directories(bddd PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
