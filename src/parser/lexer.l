%{
#include "parser/driver.h"
#include "parser.hh"
#include <cstdio>
#include <string>

# undef yywrap
# define yywrap() 1
// #define YY_DECL yy::parser::symbol_type yylex()
static yy::location loc;
%}

%option noyywrap noinput nounput batch debug
%x BLOCK_COMMENT
%x SINGLE_COMMENT

Identifier [a-zA-Z_][a-zA-Z0-9_]*
HexConst "0"[xX][0-9a-fA-F]+
OctConst "0"[0-7]*
DecConst [1-9][0-9]*
whitespace [ \t]
DecFloatConst1 [0-9]*"."[0-9]+([eE][+-]?[0-9]+)?
DecFloatConst2 [0-9]+"."([eE][+-]?[0-9]+)?
DecFloatConst3 [0-9]+[eE][+-]?[0-9]+
HexFloatConst1 "0"[xX][0-9a-fA-F]*"."[0-9a-fA-F]+"p"[+-]?[0-9]+
HexFloatConst2 "0"[xX][0-9a-fA-F]+"."?"p"[+-]?[0-9]+

%{
#define YY_USER_ACTION loc.columns(yyleng);
%}

%%

%{
    loc.step();
%}


"/*" { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/" { BEGIN(INITIAL); }
<BLOCK_COMMENT>. {}

{whitespace}+   loc.columns(); loc.step();
[\n]      loc.lines(yyleng); loc.step();
"const"    return yy::parser::make_CONST(loc);
"int"      return yy::parser::make_INT(loc);
"float"    return yy::parser::make_FLOAT(loc);
","        return yy::parser::make_COMMA(loc);
";"        return yy::parser::make_SEMICOLON(loc);
"("        return yy::parser::make_LPAREN(loc);
")"        return yy::parser::make_RPAREN(loc);
"["        return yy::parser::make_LBRACKET(loc);
"]"        return yy::parser::make_RBRACKET(loc);
"{"        return yy::parser::make_LBRACE(loc);
"}"        return yy::parser::make_RBRACE(loc);
"void"      return yy::parser::make_VOID(loc);
"if"      return yy::parser::make_IF(loc);
"else"      return yy::parser::make_ELSE(loc);
"while"      return yy::parser::make_WHILE(loc);
"break"      return yy::parser::make_BREAK(loc);
"continue"      return yy::parser::make_CONTINUE(loc);
"return"      return yy::parser::make_RETURN(loc);

"+"        return yy::parser::make_PLUS(loc);
"-"        return yy::parser::make_MINUS(loc);
"*"        return yy::parser::make_MULTI(loc);
"/"        return yy::parser::make_DIV(loc);
"%"        return yy::parser::make_MOD(loc);
">"        return yy::parser::make_GE(loc);
">="        return yy::parser::make_GEQ(loc);
"<"        return yy::parser::make_LE(loc);
"<="        return yy::parser::make_LEQ(loc);
"=="        return yy::parser::make_EQ(loc);
"="        return yy::parser::make_ASSIGN(loc);
"!="        return yy::parser::make_NEQ(loc);

"&&"        return yy::parser::make_AND(loc);
"||"        return yy::parser::make_OR(loc);
"!"        return yy::parser::make_NOT(loc);

"//" { BEGIN(SINGLE_COMMENT); }
<SINGLE_COMMENT>\n { loc.lines(yyleng); loc.step(); BEGIN(INITIAL); }
<SINGLE_COMMENT>. {}

{DecConst}   {
  long long n = strtoll(yytext, NULL, 10);
  return yy::parser::make_INTCONST(n, loc);
}

{HexConst}   {
  long long n = strtoll(yytext, NULL, 0);
  return yy::parser::make_INTCONST(n, loc);
}

{OctConst}   {
  long long n = strtoll(yytext, NULL, 0);
  return yy::parser::make_INTCONST(n, loc);
}

{DecFloatConst1} {
  float n = std::stof(std::string(yytext));
  return yy::parser::make_FLOATCONST(n, loc);
}

{DecFloatConst2} {
  float n = std::stof(std::string(yytext));
  return yy::parser::make_FLOATCONST(n, loc);
}

{DecFloatConst3} {
  float n = std::stof(std::string(yytext));
  return yy::parser::make_FLOATCONST(n, loc);
}

{HexFloatConst1} {
  float n = std::stof(std::string(yytext));
  return yy::parser::make_FLOATCONST(n, loc);
}

{HexFloatConst2} {
  float n = std::stof(std::string(yytext));
  return yy::parser::make_FLOATCONST(n, loc);
}


{Identifier}       return yy::parser::make_IDENTIFIER(yytext, loc);
. {
  // if (strcmp(yytext, "end of file") == 0) {
  //   printf("[lexical error] missing */ in block comment\n");
  // } else if (strstr(yytext, "int const") != NULL) {
  //   printf("[lexical error] invalid number for int const\n");
  // } else if (strstr(yytext, "float const") != NULL) {
  //   printf("[lexical error] invalid number for float const\n");
  // }
  printf("[lexical error] invalid character(s): %s\n", yytext);
}
<<EOF>>    return yy::parser::make_END(loc);

%%

void Driver::scan_begin () {
  yy_flex_debug = trace_scanning;
  if (file.empty () || file == "-")
    yyin = stdin;
  else if (!(yyin = fopen (file.c_str (), "r")))
    {
      error ("cannot open " + file + ": " + strerror(errno));
      exit (EXIT_FAILURE);
    }
}

void Driver::scan_end () {
  fclose (yyin);
}

