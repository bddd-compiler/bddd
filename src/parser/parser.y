%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0.4"
%defines
%define api.parser.class {parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%code requires
{
#include "ast/ast.h"
#include <string>
#include <memory>
using std::string;
using std::vector;
using std::unique_ptr;
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
    END 0 "end of file"
    CONST "const"
    INT "int"
    FLOAT "float"
    COMMA ","
    SEMICOLON ";"
    LBRACE "{"
    RBRACE "}"
    LBRACKET "["
    RBRACKET "]"
    LPAREN "("
    RPAREN ")"
    ASSIGN "="
    VOID "void"
    IF "if"
    ELSE "else"
    WHILE "while"
    BREAK "break"
    CONTINUE "continue"
    RETURN "return"
    PLUS "+"
    MINUS "-"
    MULTI "*"
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

%type <vector<unique_ptr<DeclAST>>> Decl ConstDecl VarDecl
%type <VarType> BType
%type <unique_ptr<DeclAST>> VarDef ConstDef VarDefSingle VarDefArray DefArrayBody ConstDefSingle ConstDefArray
%type <unique_ptr<InitValAST>> InitVal InitValArray InitValArrayBody
%type <unique_ptr<ExprAST>> Exp LOrExp LAndExp EqExp RelExp AddExp MulExp UnaryExp PrimaryExp Number
%type <unique_ptr<FuncCallAST>> FuncCall
%type <unique_ptr<LValAST>> LVal
%type <unique_ptr<FuncDefAST>> FuncDef
%type <vector<unique_ptr<FuncFParamAST>>> FuncFParams
%type <unique_ptr<FuncFParamAST>> FuncFParam FuncFParamSingle FuncFParamArray
%type <vector<unique_ptr<ExprAST>>> FuncRParams
%type <unique_ptr<BlockAST>> Block BlockItems
%type <vector<unique_ptr<AST>>> BlockItem
%type <unique_ptr<StmtAST>> Stmt
%type <unique_ptr<IfStmtAST>> IfStmt
%type <unique_ptr<ReturnStmtAST>> ReturnStmt
%type <unique_ptr<WhileStmtAST>>  WhileStmt
%type <unique_ptr<BreakStmtAST>> BreakStmt
%type <unique_ptr<ContinueStmtAST>> ContinueStmt
%type <unique_ptr<CondAST>> Cond
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
TOK_LBRACKET: "["  { /* printf("(delimiter, [)\n"); */ };
TOK_RBRACKET: "]"  { /* printf("(delimiter, ])\n"); */ };
TOK_RETURN: "return" { /* printf("(keyword, return)\n"); */ };
TOK_IF: "if"       { /* printf("(keyword, if)\n"); */ };
TOK_ELSE: "else"   { /* printf("(keyword, else)\n"); */ };
TOK_WHILE: "while" { /* printf("(keyword, while)\n"); */ };
TOK_BREAK: "break" { /* printf("(keyword, break)\n"); */ };
TOK_CONTINUE: "continue" { /* printf("(keyword, continue)\n"); */ };

CompUnit: CompUnit Decl { driver.comp_unit->AppendDecls(std::move($2)); }
        | CompUnit FuncDef { driver.comp_unit->AppendFuncDef(std::move($2)); }
        | Decl { driver.comp_unit->AppendDecls(std::move($1)); }
        | FuncDef { driver.comp_unit->AppendFuncDef(std::move($1)); }
        ;

Decl: ConstDecl TOK_SEMICOLON { $$.assign(std::make_move_iterator(std::begin($1)), std::make_move_iterator(std::end($1))); $1.clear(); }
    | VarDecl TOK_SEMICOLON { $$.assign(std::make_move_iterator(std::begin($1)), std::make_move_iterator(std::end($1))); $1.clear(); }
    ;

BType: "int" { $$ = VarType::INT; /* printf("(keyword, int)\n"); */ }
     | "float" { $$ = VarType::FLOAT; /* printf("(keyword, float)\n"); */ }
     ;

ConstDecl: TOK_CONST BType ConstDef { $3->SetIsConst(true); $3->SetVarType($2); $$.push_back(std::move($3)); }
         | ConstDecl TOK_COMMA ConstDef { $$.assign(std::make_move_iterator(std::begin($1)), std::make_move_iterator(std::end($1))); $1.clear(); $3->SetVarType($$[0]->GetVarType()); $3->SetIsConst(true); $$.push_back(std::move($3)); }
         ;

VarDecl: BType VarDef { $2->SetVarType($1); $$.push_back(std::move($2)); }
       | VarDecl TOK_COMMA VarDef { $$.assign(std::make_move_iterator(std::begin($1)), std::make_move_iterator(std::end($1))); $1.clear(); $3->SetVarType($$[0]->GetVarType()); $$.push_back(std::move($3)); }
       ;

VarDef: VarDefSingle { $$ = std::move($1); }
      | VarDefArray { $$ = std::move($1); }
      ;
VarDefSingle: IDENT TOK_ASSIGN InitVal { $$ = std::make_unique<DeclAST>(std::move($1), std::move($3)); }
            | IDENT { $$ = std::make_unique<DeclAST>(std::move($1)); }
            ;
VarDefArray: DefArrayBody TOK_ASSIGN InitValArray { $$ = std::move($1); $$->SetInitVal(std::move($3)); }
           | DefArrayBody { $$ = std::move($1); }
           ;

DefArrayBody: DefArrayBody TOK_LBRACKET Exp TOK_RBRACKET { $$ = std::move($1); $$->AddDimension(std::move($3)); }
     | IDENT TOK_LBRACKET Exp TOK_RBRACKET { $$ = std::make_unique<DeclAST>($1); $$->AddDimension(std::move($3)); }
     ;

ConstDef: ConstDefSingle { $$ = std::move($1); }
        | ConstDefArray { $$ = std::move($1); }
        ;

ConstDefSingle: IDENT TOK_ASSIGN InitVal { $$ = std::make_unique<DeclAST>(std::move($1), std::move($3)); }
              ;

ConstDefArray: DefArrayBody TOK_ASSIGN InitValArray { $$ = std::move($1); $$->SetInitVal(std::move($3)); }
             ;

InitVal: AddExp { $$ = std::make_unique<InitValAST>(std::move($1)); };

InitValArray: TOK_LBRACE InitValArrayBody TOK_RBRACE { $$ = std::move($2); }
            | TOK_LBRACE TOK_RBRACE { $$ = std::make_unique<InitValAST>(); }
            ;

InitValArrayBody: InitValArrayBody TOK_COMMA InitValArray { $$ = std::move($1); $$->AppendVal(std::move($3)); }
                | InitValArrayBody TOK_COMMA InitVal { $$ = std::move($1); $$->AppendVal(std::move($3)); }
                | InitValArray { $$ = std::make_unique<InitValAST>(std::move($1)); }
                | InitVal { $$ = std::make_unique<InitValAST>(std::move($1)); }
                ;

Exp: AddExp { $$ = std::move($1); };

LOrExp: LOrExp TOK_OR LAndExp { $$ = std::make_unique<ExprAST>(Op::OR, std::move($1), std::move($3)); }
      | LAndExp { $$ = std::move($1); }
      ;

LAndExp: EqExp { $$ = std::move($1); }
       | LAndExp TOK_AND EqExp { $$ = std::make_unique<ExprAST>(Op::AND, std::move($1), std::move($3)); }
       ;

EqExp: RelExp { $$ = std::move($1); }
     | RelExp TOK_EQ RelExp { $$ = std::make_unique<ExprAST>(Op::EQ, std::move($1), std::move($3)); }
     | RelExp TOK_NEQ RelExp { $$ = std::make_unique<ExprAST>(Op::NEQ, std::move($1), std::move($3)); }
     ;

RelExp: AddExp { $$ = std::move($1); }
      | RelExp RelOp AddExp { $$ = std::make_unique<ExprAST>($2, std::move($1), std::move($3)); }
      ;

AddExp: MulExp { $$ = std::move($1); }
      | AddExp AddOp MulExp { $$ = std::make_unique<ExprAST>($2, std::move($1), std::move($3)); }
      ;

MulExp: UnaryExp { $$ = std::move($1); }
      | MulExp MulOp UnaryExp { $$ = std::make_unique<ExprAST>($2, std::move($1), std::move($3)); }
      ;

UnaryExp: PrimaryExp { $$ = std::move($1); }
        | FuncCall { $$ = std::make_unique<ExprAST>(std::move($1)); }
        | UnaryOp UnaryExp { $$ = std::make_unique<ExprAST>($1, std::move($2)); }
        ;

FuncCall: IDENT TOK_LPAREN FuncRParams TOK_RPAREN { $$ = std::make_unique<FuncCallAST>(std::move($1)); $$->AssignParams(std::move($3)); }
        | IDENT TOK_LPAREN TOK_RPAREN { $$ = std::make_unique<FuncCallAST>(std::move($1)); }
        ;

PrimaryExp: TOK_LPAREN Exp TOK_RPAREN { $$ = std::move($2); }
          | LVal { $$ = std::make_unique<ExprAST>(std::move($1)); }
          | Number { $$ = std::move($1); }
          ;

LVal: LVal TOK_LBRACKET Exp TOK_RBRACKET { $$ = std::move($1); $$->AddDimension(std::move($3)); }
    | IDENT { $$ = std::make_unique<LValAST>(std::move($1)); }
    ;

FuncDef: TOK_VOID IDENT TOK_LPAREN FuncFParams TOK_RPAREN Block { $$ = std::make_unique<FuncDefAST>(VarType::VOID, std::move($2), std::move($4), std::move($6)); }
       | TOK_VOID IDENT TOK_LPAREN TOK_RPAREN Block { $$ = std::make_unique<FuncDefAST>(VarType::VOID, std::move($2), std::move($5)); }
       | BType IDENT TOK_LPAREN FuncFParams TOK_RPAREN Block { $$ = std::make_unique<FuncDefAST>($1, std::move($2), std::move($4), std::move($6)); }
       | BType IDENT TOK_LPAREN TOK_RPAREN Block { $$ = std::make_unique<FuncDefAST>($1, std::move($2), std::move($5)); }
       ;

FuncFParams: FuncFParams TOK_COMMA FuncFParam { $$.assign(std::make_move_iterator(std::begin($1)), std::make_move_iterator(std::end($1))); $1.clear(); $$.push_back(std::move($3)); }
           | FuncFParam { $$.push_back(std::move($1)); }
           ;

FuncRParams: FuncRParams TOK_COMMA AddExp { $$.assign(std::make_move_iterator(std::begin($1)), std::make_move_iterator(std::end($1))); $1.clear(); $$.push_back(std::move($3)); }
           | AddExp { $$.push_back(std::move($1)); }
           ;

FuncFParam: FuncFParamSingle { $$ = std::move($1); }
          | FuncFParamArray { $$ = std::move($1); }
          ;

FuncFParamSingle: BType IDENT { $$ = std::make_unique<FuncFParamAST>($1, std::move($2)); };

FuncFParamArray: BType IDENT TOK_LBRACKET TOK_RBRACKET { $$ = std::make_unique<FuncFParamAST>($1, std::move($2)); $$->AddDimension(nullptr); }
               | FuncFParamArray TOK_LBRACKET Exp TOK_RBRACKET { $$ = std::move($1); $$->AddDimension(std::move($3)); }
               ;

Block: TOK_LBRACE TOK_RBRACE { $$ = std::make_unique<BlockAST>(); }
     | TOK_LBRACE BlockItems TOK_RBRACE { $$ = std::move($2); }
     ;

BlockItems: BlockItem { $$ = std::make_unique<BlockAST>(); $$->AppendNodes(std::move($1)); }
          | BlockItems BlockItem { $$ = std::move($1); $$->AppendNodes(std::move($2)); }
          ;

BlockItem: Decl { $$.insert(std::end($$), std::make_move_iterator(std::begin($1)), std::make_move_iterator(std::end($1))); $1.clear(); }
         | Stmt { $$.push_back(std::move($1)); }
         ;

Stmt: LVal TOK_ASSIGN Exp TOK_SEMICOLON { $$ = std::make_unique<AssignStmtAST>(std::move($1), std::move($3)); }
    | Exp TOK_SEMICOLON { $$ = std::make_unique<EvalStmtAST>(std::move($1)); }
    | TOK_SEMICOLON { $$ = std::make_unique<BlockAST>(); }
    | Block { $$ = std::move($1); }
    | IfStmt { $$ = std::move($1); }
    | WhileStmt { $$ = std::move($1); }
    | BreakStmt { $$ = std::move($1); }
    | ContinueStmt { $$ = std::move($1); }
    | ReturnStmt { $$ = std::move($1); }
    ;

IfStmt: TOK_IF TOK_LPAREN Cond TOK_RPAREN Stmt TOK_ELSE Stmt { $$ = std::make_unique<IfStmtAST>(std::move($3), std::move($5), std::move($7)); }
      | TOK_IF TOK_LPAREN Cond TOK_RPAREN Stmt { $$ = std::make_unique<IfStmtAST>(std::move($3), std::move($5)); } %prec "then"
      ;

ReturnStmt: TOK_RETURN Exp TOK_SEMICOLON { $$ = std::make_unique<ReturnStmtAST>(std::move($2)); }
          | TOK_RETURN TOK_SEMICOLON { $$ = std::make_unique<ReturnStmtAST>(); }
          ;

WhileStmt: TOK_WHILE TOK_LPAREN Cond TOK_RPAREN Stmt { $$ = std::make_unique<WhileStmtAST>(std::move($3), std::move($5)); };

BreakStmt: TOK_BREAK TOK_SEMICOLON { $$ = std::make_unique<BreakStmtAST>(); };

ContinueStmt: TOK_CONTINUE TOK_SEMICOLON { $$ = std::make_unique<ContinueStmtAST>(); }

Cond: LOrExp { $$ = std::make_unique<CondAST>(std::move($1)); };

Number: INTCONST { $$ = std::make_unique<ExprAST>($1); /* printf("(intconst, %d)\n", $1); */ }
    | FLOATCONST { $$ = std::make_unique<ExprAST>($1); /* printf("(floatconst, %f)\n", $1); */ }

AddOp: "+" { $$ = Op::PLUS;  /* printf("(op, +)\n"); */ }
     | "-" { $$ = Op::MINUS; /* printf("(op, -)\n"); */ }
     ;

MulOp: "*" { $$ = Op::MULTI; /* printf("(op, *)\n"); */ }
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

IDENT: IDENTIFIER { $$ = std::move($1); /* printf("(identifier, %s)\n", $1.c_str()); */ };

%left "+" "-";
%left "*" "/" "%";


%%

void yy::parser::error (const location_type& l, const std::string& m) {
  driver.error(l, m);
}