%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0.4"
%defines
%define api.parser.class {parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%code requires
{
#include "ast.h"
#include <string>
#include <memory>
using std::string;
using std::vector;
class Driver;
}
// The parsing context.
%param { Driver & driver }
%locations
%initial-action
{
  // Initialize the initial location.
  @$.begin.filename = @$.end.filename = &driver.file;
};
%define parse.trace
%define parse.error verbose
%code
{
# include "parser/driver.h"
}
%nonassoc "then"
%nonassoc "else"
%define api.token.prefix {TOK_}
%token <int> INTCONST "int const"
%token <float> FLOATCONST "float const"
%token <string> IDENTIFIER "identifier"

%token
    END 0  "end of file"
    CONST "const"
    INT "int"
    FLOAT "float"
    COMMA ","
    SEMICOLON ";"
    LBRACE "{"
    RBRACE "}"
    ASSIGN "="
    LPAREN "("
    RPAREN ")"
    VOID "void"
    IF "if"
    ELSE "else"
    WHILE "while"
    BREAK "break"
    CONTINUE "continue"
    RETURN "return"
    PLUS "+"
    MINUS "-"
    MULT "*"
    DIV "/"
    MOD "%"
    GE ">"
    GEQ ">="
    LE "<"
    LEQ "<="
    EQ "=="
    NEQ "!="
    AND "&&"
    OR "||"
    NOT "!"
;

%type <vector<DeclAST*>> Decl ConstDecl VarDecl
%type <VarType> BType
%type <DeclAST*> VarDef ConstDef
%type <ExprAST*> InitVal
%type <ExprAST*> Exp LOrExp LAndExp EqExp RelExp AddExp MulExp UnaryExp PrimaryExp
%type <FuncCall*> FuncCall
%type <LVal*> LVal
%type <FuncDefAST*> FuncDef
%type <vector<FuncFParam*>> FuncFParams
%type <FuncFParam*> FuncFParam
%type <vector<ExprAST*>> FuncRParams
%type <BlockAST*> Block BlockItems
%type <vector<Node*>> BlockItem
%type <StmtAST*> Stmt
%type <IfStmtAST*> IfStmt
%type <ReturnStmtAST*> ReturnStmt
%type <WhileStmtAST*>  WhileStmt
%type <BreakStmtAST*> BreakStmt
%type <ContinueStmtAST*> ContinueStmt
%type <Cond*> Cond
%type <ExprAST*> Number
%type <Op> AddOp MulOp UnaryOp RelOp
%type <string> IDENT
%start CompUnit

%%

TOK_SEMICOLON: ";" { /* printf("(delimiter, ;)\n"); */ };
TOK_CONST: "const" { /* printf("(keyword, const)\n"); */ };
TOK_COMMA: ","     { /* printf("(delimiter, ,)\n"); */ };
TOK_ASSIGN: "="    { /* printf("(op, =)\n"); */ };
TOK_AND: "&&"      { /* printf("(op, &&)\n"); */ };
TOK_OR: "||"       { /* printf("(op, ||)\n"); */ };
TOK_EQ: "=="       { /* printf("(op, ==)\n"); */ };
TOK_NEQ: "!="      { /* printf("(op, !=)\n"); */ };
TOK_VOID: "void"   { /* printf("(keyword, void)\n"); */ };
TOK_LPAREN: "("    { /* printf("(delimiter, ()\n"); */ };
TOK_RPAREN: ")"    { /* printf("(delimiter, ))\n"); */ };
TOK_LBRACE: "{"    { /* printf("(delimiter, {)\n"); */ };
TOK_RBRACE: "}"    { /* printf("(delimiter, })\n"); */ };
TOK_RETURN: "return" { /* printf("(keyword, return)\n"); */ };
TOK_IF: "if"       { /* printf("(keyword, if)\n"); */ };
TOK_ELSE: "else"   { /* printf("(keyword, else)\n"); */ };
TOK_WHILE: "while" { /* printf("(keyword, while)\n"); */ };
TOK_BREAK: "break" { /* printf("(keyword, break)\n"); */ };
TOK_CONTINUE: "continue" { /* printf("(keyword, continue)\n"); */ };

CompUnit: CompUnit Decl { driver.compUnit->appendDecls($2); }
        | CompUnit FuncDef { driver.compUnit->appendFuncDef($2); }
        | Decl { driver.compUnit->appendDecls($1); }
        | FuncDef { driver.compUnit->appendFuncDef($1); }
        ;

Decl: ConstDecl TOK_SEMICOLON { std::swap($$, $1); }
    | VarDecl TOK_SEMICOLON { std::swap($$, $1); }
    ;

BType: "int" { $$ = VarType::INT; /* printf("(keyword, int)\n"); */ }
    | "float" { $$ = VarType::FLOAT; /* printf("(keyword, float)\n"); */ }
    ;

ConstDecl: TOK_CONST BType ConstDef { $3->isConst = true; $3->type = $2; $$.emplace_back($3); }
         | ConstDecl TOK_COMMA ConstDef { std::swap($$, $1); $3->isConst = true; $$.emplace_back($3); }
    ;

VarDecl: BType VarDef { $2->type = $1; $$.emplace_back($2); }
       | VarDecl TOK_COMMA VarDef { std::swap($$, $1); $$.emplace_back($3); }
       ;

VarDef: IDENT TOK_ASSIGN InitVal { $$ = new DeclAST($1, $3); }
             | IDENT { $$ = new DeclAST($1); }
             ;

ConstDef: IDENT TOK_ASSIGN InitVal { $$ = new DeclAST($1, $3); }
    ;

InitVal: AddExp { $$ = $1; };

Exp: AddExp { $$ = $1; };

LOrExp: LOrExp TOK_OR LAndExp { $$ = new ExprAST(Op::OR, $1, $3); }
      | LAndExp { $$ = $1; }
      ;

LAndExp: EqExp { $$ = $1; }
       | LAndExp TOK_AND EqExp { $$ = new ExprAST(Op::AND, $1, $3); }
       ;

EqExp: RelExp { $$ = $1; }
     | RelExp TOK_EQ RelExp { $$ = new ExprAST(Op::EQ, $1, $3); }
     | RelExp TOK_NEQ RelExp { $$ = new ExprAST(Op::NEQ, $1, $3); }
     ;

RelExp: AddExp { $$ = $1; }
      | RelExp RelOp AddExp { $$ = new ExprAST($2, $1, $3); }
      ;

AddExp: MulExp { $$ = $1; }
      | AddExp AddOp MulExp { $$ = new ExprAST($2, $1, $3); }
      ;

MulExp: UnaryExp { $$ = $1; }
      | MulExp MulOp UnaryExp { $$ = new ExprAST($2, $1, $3); }
      ;

UnaryExp: PrimaryExp { $$ = $1; }
        | FuncCall { $$ = new ExprAST($1); }
        | UnaryOp UnaryExp { $$ = new ExprAST($1, $2); }
        ;

FuncCall: IDENT TOK_LPAREN FuncRParams TOK_RPAREN { $$ = new FuncCall($1); std::swap($$->params, $3); }
        | IDENT TOK_LPAREN TOK_RPAREN { $$ = new FuncCall($1); }
        ;

PrimaryExp: TOK_LPAREN Exp TOK_RPAREN { $$ = $2; }
          | LVal { $$ = new ExprAST($1); }
          | Number { $$ = $1; }
          ;

LVal: IDENT { $$ = new LVal($1); }
    ;

FuncDef: TOK_VOID IDENT TOK_LPAREN FuncFParams TOK_RPAREN Block { $$ = new FuncDefAST(VarType::VOID, $2, $4, $6); }
       | TOK_VOID IDENT TOK_LPAREN TOK_RPAREN Block { $$ = new FuncDefAST(VarType::VOID, $2, $5); }
       | BType IDENT TOK_LPAREN FuncFParams TOK_RPAREN Block { $$ = new FuncDefAST($1, $2, $4, $6); }
       | BType IDENT TOK_LPAREN TOK_RPAREN Block { $$ = new FuncDefAST($1, $2, $5); }
       ;

FuncFParams: FuncFParams TOK_COMMA FuncFParam { std::swap($$, $1); $$.emplace_back($3); }
           | FuncFParam { $$.emplace_back($1); }
           ;

FuncRParams: FuncRParams TOK_COMMA AddExp { std::swap($$, $1); $$.emplace_back($3); }
           | AddExp { $$.emplace_back($1); }
           ;

FuncFParam: BType IDENT { $$ = new FuncFParam($1, $2); };

Block: TOK_LBRACE TOK_RBRACE { $$ = new BlockAST(); }
     | TOK_LBRACE BlockItems TOK_RBRACE { $$ = $2; }
     ;

BlockItems: BlockItem { $$ = new BlockAST(); $$->appendNodes($1); }
          | BlockItems BlockItem { $$ = $1; $$->appendNodes($2); }
          ;

BlockItem: Decl { $$.insert(std::end($$), std::begin($1), std::end($1)); }
         | Stmt { $$.emplace_back($1); }
         ;

Stmt: LVal TOK_ASSIGN Exp TOK_SEMICOLON { $$ = new AssignStmtAST($1, $3); }
    | Exp TOK_SEMICOLON { $$ = new EvalStmtAST($1); }
    | TOK_SEMICOLON { $$ = new BlockAST(); }
    | Block { $$ = $1; }
    | IfStmt { $$ = $1; }
    | WhileStmt { $$ = $1; }
    | BreakStmt { $$ = $1; }
    | ReturnStmt { $$ = $1; }
    | ContinueStmt { $$ = $1; }
    ;

IfStmt: TOK_IF TOK_LPAREN Cond TOK_RPAREN Stmt TOK_ELSE Stmt { $$ = new IfStmtAST($3, $5, $7); }
      | TOK_IF TOK_LPAREN Cond TOK_RPAREN Stmt { $$ = new IfStmtAST($3, $5); } %prec "then"
      ;

ReturnStmt: TOK_RETURN Exp TOK_SEMICOLON { $$ = new ReturnStmtAST($2); }
          | TOK_RETURN TOK_SEMICOLON { $$ = new ReturnStmtAST; }
          ;

WhileStmt: TOK_WHILE TOK_LPAREN Cond TOK_RPAREN Stmt { $$ = new WhileStmtAST($3, $5); };

BreakStmt: TOK_BREAK TOK_SEMICOLON { $$ = new BreakStmtAST; };

ContinueStmt: TOK_CONTINUE TOK_SEMICOLON { $$ = new ContinueStmtAST; }

Cond: LOrExp { $$ = new Cond($1); };

Number: INTCONST { $$ = new ExprAST($1); /* printf("(intconst, %d)\n", $1); */ }
    | FLOATCONST { $$ = new ExprAST($1); /* printf("(floatconst, %f)\n", $1); */ }

AddOp: "+" { $$ = Op::PLUS;  /* printf("(op, +)\n"); */ }
     | "-" { $$ = Op::MINUS; /* printf("(op, -)\n"); */ }
     ;

MulOp: "*" { $$ = Op::MULT; /* printf("(op, *)\n"); */ }
     | "/" { $$ = Op::DIV;  /* printf("(op, /)\n"); */ }
     | "%" { $$ = Op::MOD;  /* printf("(op, %)\n"); */ }
     ;

UnaryOp: "+" { $$ = Op::POSITIVE; /* printf("(op, +)\n"); */ }
       | "-" { $$ = Op::NEGATIVE; /* printf("(op, -)\n"); */ }
       | "!" { $$ = Op::NOT; /* printf("(op, !)\n"); */ }
       ;

RelOp: ">" { $$ = Op::GE;   /* printf("(op, >)\n"); */ }
     | ">=" { $$ = Op::GEQ; /* printf("(op, >=)\n"); */ }
     | "<" { $$ = Op::LE;   /* printf("(op, <)\n"); */ }
     | "<=" { $$ = Op::LEQ; /* printf("(op, <=)\n"); */ }
     ;

IDENT: IDENTIFIER { $$ = $1; /* printf("(identifier, %s)\n", $1.c_str()); */ };

%left "+" "-";
%left "*" "/" "%";


%%

void yy::parser::error (const location_type& l, const std::string& m) {
  driver.error(l, m);
}