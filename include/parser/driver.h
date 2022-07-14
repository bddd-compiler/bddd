#ifndef BDDD_DRIVER_H
#define BDDD_DRIVER_H

#include <ostream>

#include "parser.hh"

#define YY_DECL yy::parser::symbol_type yylex(Driver& driver)

YY_DECL;
class Driver {
public:
  Driver();

  // Final AST
  std::unique_ptr<CompUnitAST> comp_unit;
  // Handling the scanner.
  void scan_begin();
  void scan_end();
  bool trace_scanning;
  // Run the parser on file F.
  // Return 0 on success.
  int parse(const std::string& f);
  // The m_name of the file being parsed.
  // Used later to pass the file m_name to the location tracker.
  std::string file;
  // Whether parser traces should be generated.
  bool trace_parsing;
  // Error handling.
  void error(const yy::location& l, const std::string& m);
  void error(const std::string& m);
};

#endif  // BDDD_DRIVER_H