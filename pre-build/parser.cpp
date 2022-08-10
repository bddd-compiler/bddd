// A Bison parser, made by GNU Bison 3.5.1.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2020 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// Undocumented macros, especially those whose name start with YY_,
// are private implementation details.  Do not rely on them.





#include "parser.hh"


// Unqualified %code blocks.
#line 29 "/root/bddd/src/parser/parser.y"

# include "parser/driver.h"

#line 49 "/root/bddd/build/parser.cpp"


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yystack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

namespace yy {
#line 140 "/root/bddd/build/parser.cpp"


  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  parser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr;
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              else
                goto append;

            append:
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }


  /// Build a parser object.
  parser::parser (Driver & driver_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      driver (driver_yyarg)
  {}

  parser::~parser ()
  {}

  parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------------.
  | Symbol types.  |
  `---------------*/



  // by_state.
  parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  parser::symbol_number_type
  parser::by_state::type_get () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return empty_symbol;
    else
      return yystos_[+state];
  }

  parser::stack_symbol_type::stack_symbol_type ()
  {}

  parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.type_get ())
    {
      case 105: // AddOp
      case 106: // MulOp
      case 107: // UnaryOp
      case 108: // RelOp
        value.YY_MOVE_OR_COPY< Op > (YY_MOVE (that.value));
        break;

      case 64: // BType
        value.YY_MOVE_OR_COPY< VarType > (YY_MOVE (that.value));
        break;

      case 5: // "float const"
        value.YY_MOVE_OR_COPY< float > (YY_MOVE (that.value));
        break;

      case 4: // "int const"
        value.YY_MOVE_OR_COPY< int > (YY_MOVE (that.value));
        break;

      case 6: // "identifier"
      case 109: // IDENT
        value.YY_MOVE_OR_COPY< string > (YY_MOVE (that.value));
        break;

      case 94: // Block
      case 95: // BlockItems
        value.YY_MOVE_OR_COPY< unique_ptr<BlockAST> > (YY_MOVE (that.value));
        break;

      case 101: // BreakStmt
        value.YY_MOVE_OR_COPY< unique_ptr<BreakStmtAST> > (YY_MOVE (that.value));
        break;

      case 103: // Cond
        value.YY_MOVE_OR_COPY< unique_ptr<CondAST> > (YY_MOVE (that.value));
        break;

      case 102: // ContinueStmt
        value.YY_MOVE_OR_COPY< unique_ptr<ContinueStmtAST> > (YY_MOVE (that.value));
        break;

      case 67: // VarDef
      case 68: // VarDefSingle
      case 69: // VarDefArray
      case 70: // DefArrayBody
      case 71: // ConstDef
      case 72: // ConstDefSingle
      case 73: // ConstDefArray
        value.YY_MOVE_OR_COPY< unique_ptr<DeclAST> > (YY_MOVE (that.value));
        break;

      case 77: // Exp
      case 78: // LOrExp
      case 79: // LAndExp
      case 80: // EqExp
      case 81: // RelExp
      case 82: // AddExp
      case 83: // MulExp
      case 84: // UnaryExp
      case 86: // PrimaryExp
      case 104: // Number
        value.YY_MOVE_OR_COPY< unique_ptr<ExprAST> > (YY_MOVE (that.value));
        break;

      case 85: // FuncCall
        value.YY_MOVE_OR_COPY< unique_ptr<FuncCallAST> > (YY_MOVE (that.value));
        break;

      case 88: // FuncDef
        value.YY_MOVE_OR_COPY< unique_ptr<FuncDefAST> > (YY_MOVE (that.value));
        break;

      case 91: // FuncFParam
      case 92: // FuncFParamSingle
      case 93: // FuncFParamArray
        value.YY_MOVE_OR_COPY< unique_ptr<FuncFParamAST> > (YY_MOVE (that.value));
        break;

      case 98: // IfStmt
        value.YY_MOVE_OR_COPY< unique_ptr<IfStmtAST> > (YY_MOVE (that.value));
        break;

      case 74: // InitVal
      case 75: // InitValArray
      case 76: // InitValArrayBody
        value.YY_MOVE_OR_COPY< unique_ptr<InitValAST> > (YY_MOVE (that.value));
        break;

      case 87: // LVal
        value.YY_MOVE_OR_COPY< unique_ptr<LValAST> > (YY_MOVE (that.value));
        break;

      case 99: // ReturnStmt
        value.YY_MOVE_OR_COPY< unique_ptr<ReturnStmtAST> > (YY_MOVE (that.value));
        break;

      case 97: // Stmt
        value.YY_MOVE_OR_COPY< unique_ptr<StmtAST> > (YY_MOVE (that.value));
        break;

      case 100: // WhileStmt
        value.YY_MOVE_OR_COPY< unique_ptr<WhileStmtAST> > (YY_MOVE (that.value));
        break;

      case 96: // BlockItem
        value.YY_MOVE_OR_COPY< vector<unique_ptr<AST>> > (YY_MOVE (that.value));
        break;

      case 63: // Decl
      case 65: // ConstDecl
      case 66: // VarDecl
        value.YY_MOVE_OR_COPY< vector<unique_ptr<DeclAST>> > (YY_MOVE (that.value));
        break;

      case 90: // FuncRParams
        value.YY_MOVE_OR_COPY< vector<unique_ptr<ExprAST>> > (YY_MOVE (that.value));
        break;

      case 89: // FuncFParams
        value.YY_MOVE_OR_COPY< vector<unique_ptr<FuncFParamAST>> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.type_get ())
    {
      case 105: // AddOp
      case 106: // MulOp
      case 107: // UnaryOp
      case 108: // RelOp
        value.move< Op > (YY_MOVE (that.value));
        break;

      case 64: // BType
        value.move< VarType > (YY_MOVE (that.value));
        break;

      case 5: // "float const"
        value.move< float > (YY_MOVE (that.value));
        break;

      case 4: // "int const"
        value.move< int > (YY_MOVE (that.value));
        break;

      case 6: // "identifier"
      case 109: // IDENT
        value.move< string > (YY_MOVE (that.value));
        break;

      case 94: // Block
      case 95: // BlockItems
        value.move< unique_ptr<BlockAST> > (YY_MOVE (that.value));
        break;

      case 101: // BreakStmt
        value.move< unique_ptr<BreakStmtAST> > (YY_MOVE (that.value));
        break;

      case 103: // Cond
        value.move< unique_ptr<CondAST> > (YY_MOVE (that.value));
        break;

      case 102: // ContinueStmt
        value.move< unique_ptr<ContinueStmtAST> > (YY_MOVE (that.value));
        break;

      case 67: // VarDef
      case 68: // VarDefSingle
      case 69: // VarDefArray
      case 70: // DefArrayBody
      case 71: // ConstDef
      case 72: // ConstDefSingle
      case 73: // ConstDefArray
        value.move< unique_ptr<DeclAST> > (YY_MOVE (that.value));
        break;

      case 77: // Exp
      case 78: // LOrExp
      case 79: // LAndExp
      case 80: // EqExp
      case 81: // RelExp
      case 82: // AddExp
      case 83: // MulExp
      case 84: // UnaryExp
      case 86: // PrimaryExp
      case 104: // Number
        value.move< unique_ptr<ExprAST> > (YY_MOVE (that.value));
        break;

      case 85: // FuncCall
        value.move< unique_ptr<FuncCallAST> > (YY_MOVE (that.value));
        break;

      case 88: // FuncDef
        value.move< unique_ptr<FuncDefAST> > (YY_MOVE (that.value));
        break;

      case 91: // FuncFParam
      case 92: // FuncFParamSingle
      case 93: // FuncFParamArray
        value.move< unique_ptr<FuncFParamAST> > (YY_MOVE (that.value));
        break;

      case 98: // IfStmt
        value.move< unique_ptr<IfStmtAST> > (YY_MOVE (that.value));
        break;

      case 74: // InitVal
      case 75: // InitValArray
      case 76: // InitValArrayBody
        value.move< unique_ptr<InitValAST> > (YY_MOVE (that.value));
        break;

      case 87: // LVal
        value.move< unique_ptr<LValAST> > (YY_MOVE (that.value));
        break;

      case 99: // ReturnStmt
        value.move< unique_ptr<ReturnStmtAST> > (YY_MOVE (that.value));
        break;

      case 97: // Stmt
        value.move< unique_ptr<StmtAST> > (YY_MOVE (that.value));
        break;

      case 100: // WhileStmt
        value.move< unique_ptr<WhileStmtAST> > (YY_MOVE (that.value));
        break;

      case 96: // BlockItem
        value.move< vector<unique_ptr<AST>> > (YY_MOVE (that.value));
        break;

      case 63: // Decl
      case 65: // ConstDecl
      case 66: // VarDecl
        value.move< vector<unique_ptr<DeclAST>> > (YY_MOVE (that.value));
        break;

      case 90: // FuncRParams
        value.move< vector<unique_ptr<ExprAST>> > (YY_MOVE (that.value));
        break;

      case 89: // FuncFParams
        value.move< vector<unique_ptr<FuncFParamAST>> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.type = empty_symbol;
  }

#if YY_CPLUSPLUS < 201103L
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.type_get ())
    {
      case 105: // AddOp
      case 106: // MulOp
      case 107: // UnaryOp
      case 108: // RelOp
        value.copy< Op > (that.value);
        break;

      case 64: // BType
        value.copy< VarType > (that.value);
        break;

      case 5: // "float const"
        value.copy< float > (that.value);
        break;

      case 4: // "int const"
        value.copy< int > (that.value);
        break;

      case 6: // "identifier"
      case 109: // IDENT
        value.copy< string > (that.value);
        break;

      case 94: // Block
      case 95: // BlockItems
        value.copy< unique_ptr<BlockAST> > (that.value);
        break;

      case 101: // BreakStmt
        value.copy< unique_ptr<BreakStmtAST> > (that.value);
        break;

      case 103: // Cond
        value.copy< unique_ptr<CondAST> > (that.value);
        break;

      case 102: // ContinueStmt
        value.copy< unique_ptr<ContinueStmtAST> > (that.value);
        break;

      case 67: // VarDef
      case 68: // VarDefSingle
      case 69: // VarDefArray
      case 70: // DefArrayBody
      case 71: // ConstDef
      case 72: // ConstDefSingle
      case 73: // ConstDefArray
        value.copy< unique_ptr<DeclAST> > (that.value);
        break;

      case 77: // Exp
      case 78: // LOrExp
      case 79: // LAndExp
      case 80: // EqExp
      case 81: // RelExp
      case 82: // AddExp
      case 83: // MulExp
      case 84: // UnaryExp
      case 86: // PrimaryExp
      case 104: // Number
        value.copy< unique_ptr<ExprAST> > (that.value);
        break;

      case 85: // FuncCall
        value.copy< unique_ptr<FuncCallAST> > (that.value);
        break;

      case 88: // FuncDef
        value.copy< unique_ptr<FuncDefAST> > (that.value);
        break;

      case 91: // FuncFParam
      case 92: // FuncFParamSingle
      case 93: // FuncFParamArray
        value.copy< unique_ptr<FuncFParamAST> > (that.value);
        break;

      case 98: // IfStmt
        value.copy< unique_ptr<IfStmtAST> > (that.value);
        break;

      case 74: // InitVal
      case 75: // InitValArray
      case 76: // InitValArrayBody
        value.copy< unique_ptr<InitValAST> > (that.value);
        break;

      case 87: // LVal
        value.copy< unique_ptr<LValAST> > (that.value);
        break;

      case 99: // ReturnStmt
        value.copy< unique_ptr<ReturnStmtAST> > (that.value);
        break;

      case 97: // Stmt
        value.copy< unique_ptr<StmtAST> > (that.value);
        break;

      case 100: // WhileStmt
        value.copy< unique_ptr<WhileStmtAST> > (that.value);
        break;

      case 96: // BlockItem
        value.copy< vector<unique_ptr<AST>> > (that.value);
        break;

      case 63: // Decl
      case 65: // ConstDecl
      case 66: // VarDecl
        value.copy< vector<unique_ptr<DeclAST>> > (that.value);
        break;

      case 90: // FuncRParams
        value.copy< vector<unique_ptr<ExprAST>> > (that.value);
        break;

      case 89: // FuncFParams
        value.copy< vector<unique_ptr<FuncFParamAST>> > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    return *this;
  }

  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.type_get ())
    {
      case 105: // AddOp
      case 106: // MulOp
      case 107: // UnaryOp
      case 108: // RelOp
        value.move< Op > (that.value);
        break;

      case 64: // BType
        value.move< VarType > (that.value);
        break;

      case 5: // "float const"
        value.move< float > (that.value);
        break;

      case 4: // "int const"
        value.move< int > (that.value);
        break;

      case 6: // "identifier"
      case 109: // IDENT
        value.move< string > (that.value);
        break;

      case 94: // Block
      case 95: // BlockItems
        value.move< unique_ptr<BlockAST> > (that.value);
        break;

      case 101: // BreakStmt
        value.move< unique_ptr<BreakStmtAST> > (that.value);
        break;

      case 103: // Cond
        value.move< unique_ptr<CondAST> > (that.value);
        break;

      case 102: // ContinueStmt
        value.move< unique_ptr<ContinueStmtAST> > (that.value);
        break;

      case 67: // VarDef
      case 68: // VarDefSingle
      case 69: // VarDefArray
      case 70: // DefArrayBody
      case 71: // ConstDef
      case 72: // ConstDefSingle
      case 73: // ConstDefArray
        value.move< unique_ptr<DeclAST> > (that.value);
        break;

      case 77: // Exp
      case 78: // LOrExp
      case 79: // LAndExp
      case 80: // EqExp
      case 81: // RelExp
      case 82: // AddExp
      case 83: // MulExp
      case 84: // UnaryExp
      case 86: // PrimaryExp
      case 104: // Number
        value.move< unique_ptr<ExprAST> > (that.value);
        break;

      case 85: // FuncCall
        value.move< unique_ptr<FuncCallAST> > (that.value);
        break;

      case 88: // FuncDef
        value.move< unique_ptr<FuncDefAST> > (that.value);
        break;

      case 91: // FuncFParam
      case 92: // FuncFParamSingle
      case 93: // FuncFParamArray
        value.move< unique_ptr<FuncFParamAST> > (that.value);
        break;

      case 98: // IfStmt
        value.move< unique_ptr<IfStmtAST> > (that.value);
        break;

      case 74: // InitVal
      case 75: // InitValArray
      case 76: // InitValArrayBody
        value.move< unique_ptr<InitValAST> > (that.value);
        break;

      case 87: // LVal
        value.move< unique_ptr<LValAST> > (that.value);
        break;

      case 99: // ReturnStmt
        value.move< unique_ptr<ReturnStmtAST> > (that.value);
        break;

      case 97: // Stmt
        value.move< unique_ptr<StmtAST> > (that.value);
        break;

      case 100: // WhileStmt
        value.move< unique_ptr<WhileStmtAST> > (that.value);
        break;

      case 96: // BlockItem
        value.move< vector<unique_ptr<AST>> > (that.value);
        break;

      case 63: // Decl
      case 65: // ConstDecl
      case 66: // VarDecl
        value.move< vector<unique_ptr<DeclAST>> > (that.value);
        break;

      case 90: // FuncRParams
        value.move< vector<unique_ptr<ExprAST>> > (that.value);
        break;

      case 89: // FuncFParams
        value.move< vector<unique_ptr<FuncFParamAST>> > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  parser::yy_print_ (std::ostream& yyo,
                                     const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    symbol_number_type yytype = yysym.type_get ();
#if defined __GNUC__ && ! defined __clang__ && ! defined __ICC && __GNUC__ * 100 + __GNUC_MINOR__ <= 408
    // Avoid a (spurious) G++ 4.8 warning about "array subscript is
    // below array bounds".
    if (yysym.empty ())
      std::abort ();
#endif
    yyo << (yytype < yyntokens_ ? "token" : "nterm")
        << ' ' << yytname_[yytype] << " ("
        << yysym.location << ": ";
    YYUSE (yytype);
    yyo << ')';
  }
#endif

  void
  parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  parser::yypop_ (int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - yyntokens_] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - yyntokens_];
  }

  bool
  parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::operator() ()
  {
    return parse ();
  }

  int
  parser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    // User initialization code.
#line 22 "/root/bddd/src/parser/parser.y"
{
  // Initialize the initial location.
  yyla.location.begin.filename = yyla.location.end.filename = &driver.file;
}

#line 938 "/root/bddd/build/parser.cpp"


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token: ";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex (driver));
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.type_get ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.type_get ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case 105: // AddOp
      case 106: // MulOp
      case 107: // UnaryOp
      case 108: // RelOp
        yylhs.value.emplace< Op > ();
        break;

      case 64: // BType
        yylhs.value.emplace< VarType > ();
        break;

      case 5: // "float const"
        yylhs.value.emplace< float > ();
        break;

      case 4: // "int const"
        yylhs.value.emplace< int > ();
        break;

      case 6: // "identifier"
      case 109: // IDENT
        yylhs.value.emplace< string > ();
        break;

      case 94: // Block
      case 95: // BlockItems
        yylhs.value.emplace< unique_ptr<BlockAST> > ();
        break;

      case 101: // BreakStmt
        yylhs.value.emplace< unique_ptr<BreakStmtAST> > ();
        break;

      case 103: // Cond
        yylhs.value.emplace< unique_ptr<CondAST> > ();
        break;

      case 102: // ContinueStmt
        yylhs.value.emplace< unique_ptr<ContinueStmtAST> > ();
        break;

      case 67: // VarDef
      case 68: // VarDefSingle
      case 69: // VarDefArray
      case 70: // DefArrayBody
      case 71: // ConstDef
      case 72: // ConstDefSingle
      case 73: // ConstDefArray
        yylhs.value.emplace< unique_ptr<DeclAST> > ();
        break;

      case 77: // Exp
      case 78: // LOrExp
      case 79: // LAndExp
      case 80: // EqExp
      case 81: // RelExp
      case 82: // AddExp
      case 83: // MulExp
      case 84: // UnaryExp
      case 86: // PrimaryExp
      case 104: // Number
        yylhs.value.emplace< unique_ptr<ExprAST> > ();
        break;

      case 85: // FuncCall
        yylhs.value.emplace< unique_ptr<FuncCallAST> > ();
        break;

      case 88: // FuncDef
        yylhs.value.emplace< unique_ptr<FuncDefAST> > ();
        break;

      case 91: // FuncFParam
      case 92: // FuncFParamSingle
      case 93: // FuncFParamArray
        yylhs.value.emplace< unique_ptr<FuncFParamAST> > ();
        break;

      case 98: // IfStmt
        yylhs.value.emplace< unique_ptr<IfStmtAST> > ();
        break;

      case 74: // InitVal
      case 75: // InitValArray
      case 76: // InitValArrayBody
        yylhs.value.emplace< unique_ptr<InitValAST> > ();
        break;

      case 87: // LVal
        yylhs.value.emplace< unique_ptr<LValAST> > ();
        break;

      case 99: // ReturnStmt
        yylhs.value.emplace< unique_ptr<ReturnStmtAST> > ();
        break;

      case 97: // Stmt
        yylhs.value.emplace< unique_ptr<StmtAST> > ();
        break;

      case 100: // WhileStmt
        yylhs.value.emplace< unique_ptr<WhileStmtAST> > ();
        break;

      case 96: // BlockItem
        yylhs.value.emplace< vector<unique_ptr<AST>> > ();
        break;

      case 63: // Decl
      case 65: // ConstDecl
      case 66: // VarDecl
        yylhs.value.emplace< vector<unique_ptr<DeclAST>> > ();
        break;

      case 90: // FuncRParams
        yylhs.value.emplace< vector<unique_ptr<ExprAST>> > ();
        break;

      case 89: // FuncFParams
        yylhs.value.emplace< vector<unique_ptr<FuncFParamAST>> > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2:
#line 102 "/root/bddd/src/parser/parser.y"
                   { /* printf("(delimiter, ;)\n"); */ }
#line 1187 "/root/bddd/build/parser.cpp"
    break;

  case 3:
#line 103 "/root/bddd/src/parser/parser.y"
                   { /* printf("(keyword, const)\n"); */ }
#line 1193 "/root/bddd/build/parser.cpp"
    break;

  case 4:
#line 104 "/root/bddd/src/parser/parser.y"
                   { /* printf("(delimiter, ,)\n"); */ }
#line 1199 "/root/bddd/build/parser.cpp"
    break;

  case 5:
#line 105 "/root/bddd/src/parser/parser.y"
                   { /* printf("(op, =)\n"); */ }
#line 1205 "/root/bddd/build/parser.cpp"
    break;

  case 6:
#line 106 "/root/bddd/src/parser/parser.y"
                   { /* printf("(op, &&)\n"); */ }
#line 1211 "/root/bddd/build/parser.cpp"
    break;

  case 7:
#line 107 "/root/bddd/src/parser/parser.y"
                   { /* printf("(op, ||)\n"); */ }
#line 1217 "/root/bddd/build/parser.cpp"
    break;

  case 8:
#line 108 "/root/bddd/src/parser/parser.y"
                   { /* printf("(op, ==)\n"); */ }
#line 1223 "/root/bddd/build/parser.cpp"
    break;

  case 9:
#line 109 "/root/bddd/src/parser/parser.y"
                   { /* printf("(op, !=)\n"); */ }
#line 1229 "/root/bddd/build/parser.cpp"
    break;

  case 10:
#line 110 "/root/bddd/src/parser/parser.y"
                   { /* printf("(keyword, void)\n"); */ }
#line 1235 "/root/bddd/build/parser.cpp"
    break;

  case 11:
#line 111 "/root/bddd/src/parser/parser.y"
                   { /* printf("(delimiter, ()\n"); */ }
#line 1241 "/root/bddd/build/parser.cpp"
    break;

  case 12:
#line 112 "/root/bddd/src/parser/parser.y"
                   { /* printf("(delimiter, ))\n"); */ }
#line 1247 "/root/bddd/build/parser.cpp"
    break;

  case 13:
#line 113 "/root/bddd/src/parser/parser.y"
                   { /* printf("(delimiter, {)\n"); */ }
#line 1253 "/root/bddd/build/parser.cpp"
    break;

  case 14:
#line 114 "/root/bddd/src/parser/parser.y"
                   { /* printf("(delimiter, })\n"); */ }
#line 1259 "/root/bddd/build/parser.cpp"
    break;

  case 15:
#line 115 "/root/bddd/src/parser/parser.y"
                   { /* printf("(delimiter, [)\n"); */ }
#line 1265 "/root/bddd/build/parser.cpp"
    break;

  case 16:
#line 116 "/root/bddd/src/parser/parser.y"
                   { /* printf("(delimiter, ])\n"); */ }
#line 1271 "/root/bddd/build/parser.cpp"
    break;

  case 17:
#line 117 "/root/bddd/src/parser/parser.y"
                     { /* printf("(keyword, return)\n"); */ }
#line 1277 "/root/bddd/build/parser.cpp"
    break;

  case 18:
#line 118 "/root/bddd/src/parser/parser.y"
                   { /* printf("(keyword, if)\n"); */ }
#line 1283 "/root/bddd/build/parser.cpp"
    break;

  case 19:
#line 119 "/root/bddd/src/parser/parser.y"
                   { /* printf("(keyword, else)\n"); */ }
#line 1289 "/root/bddd/build/parser.cpp"
    break;

  case 20:
#line 120 "/root/bddd/src/parser/parser.y"
                   { /* printf("(keyword, while)\n"); */ }
#line 1295 "/root/bddd/build/parser.cpp"
    break;

  case 21:
#line 121 "/root/bddd/src/parser/parser.y"
                   { /* printf("(keyword, break)\n"); */ }
#line 1301 "/root/bddd/build/parser.cpp"
    break;

  case 22:
#line 122 "/root/bddd/src/parser/parser.y"
                         { /* printf("(keyword, continue)\n"); */ }
#line 1307 "/root/bddd/build/parser.cpp"
    break;

  case 23:
#line 124 "/root/bddd/src/parser/parser.y"
                        { driver.comp_unit->AppendDecls(std::move(yystack_[0].value.as < vector<unique_ptr<DeclAST>> > ())); }
#line 1313 "/root/bddd/build/parser.cpp"
    break;

  case 24:
#line 125 "/root/bddd/src/parser/parser.y"
                           { driver.comp_unit->AppendFuncDef(std::move(yystack_[0].value.as < unique_ptr<FuncDefAST> > ())); }
#line 1319 "/root/bddd/build/parser.cpp"
    break;

  case 25:
#line 126 "/root/bddd/src/parser/parser.y"
               { driver.comp_unit->AppendDecls(std::move(yystack_[0].value.as < vector<unique_ptr<DeclAST>> > ())); }
#line 1325 "/root/bddd/build/parser.cpp"
    break;

  case 26:
#line 127 "/root/bddd/src/parser/parser.y"
                  { driver.comp_unit->AppendFuncDef(std::move(yystack_[0].value.as < unique_ptr<FuncDefAST> > ())); }
#line 1331 "/root/bddd/build/parser.cpp"
    break;

  case 27:
#line 130 "/root/bddd/src/parser/parser.y"
                              { yylhs.value.as < vector<unique_ptr<DeclAST>> > ().assign(std::make_move_iterator(std::begin(yystack_[1].value.as < vector<unique_ptr<DeclAST>> > ())), std::make_move_iterator(std::end(yystack_[1].value.as < vector<unique_ptr<DeclAST>> > ()))); yystack_[1].value.as < vector<unique_ptr<DeclAST>> > ().clear(); }
#line 1337 "/root/bddd/build/parser.cpp"
    break;

  case 28:
#line 131 "/root/bddd/src/parser/parser.y"
                            { yylhs.value.as < vector<unique_ptr<DeclAST>> > ().assign(std::make_move_iterator(std::begin(yystack_[1].value.as < vector<unique_ptr<DeclAST>> > ())), std::make_move_iterator(std::end(yystack_[1].value.as < vector<unique_ptr<DeclAST>> > ()))); yystack_[1].value.as < vector<unique_ptr<DeclAST>> > ().clear(); }
#line 1343 "/root/bddd/build/parser.cpp"
    break;

  case 29:
#line 134 "/root/bddd/src/parser/parser.y"
             { yylhs.value.as < VarType > () = VarType::INT; /* printf("(keyword, int)\n"); */ }
#line 1349 "/root/bddd/build/parser.cpp"
    break;

  case 30:
#line 135 "/root/bddd/src/parser/parser.y"
               { yylhs.value.as < VarType > () = VarType::FLOAT; /* printf("(keyword, float)\n"); */ }
#line 1355 "/root/bddd/build/parser.cpp"
    break;

  case 31:
#line 138 "/root/bddd/src/parser/parser.y"
                                    { yystack_[0].value.as < unique_ptr<DeclAST> > ()->SetIsConst(true); yystack_[0].value.as < unique_ptr<DeclAST> > ()->SetVarType(yystack_[1].value.as < VarType > ()); yylhs.value.as < vector<unique_ptr<DeclAST>> > ().push_back(std::move(yystack_[0].value.as < unique_ptr<DeclAST> > ())); }
#line 1361 "/root/bddd/build/parser.cpp"
    break;

  case 32:
#line 139 "/root/bddd/src/parser/parser.y"
                                        { yylhs.value.as < vector<unique_ptr<DeclAST>> > ().assign(std::make_move_iterator(std::begin(yystack_[2].value.as < vector<unique_ptr<DeclAST>> > ())), std::make_move_iterator(std::end(yystack_[2].value.as < vector<unique_ptr<DeclAST>> > ()))); yystack_[2].value.as < vector<unique_ptr<DeclAST>> > ().clear(); yystack_[0].value.as < unique_ptr<DeclAST> > ()->SetVarType(yylhs.value.as < vector<unique_ptr<DeclAST>> > ()[0]->GetVarType()); yystack_[0].value.as < unique_ptr<DeclAST> > ()->SetIsConst(true); yylhs.value.as < vector<unique_ptr<DeclAST>> > ().push_back(std::move(yystack_[0].value.as < unique_ptr<DeclAST> > ())); }
#line 1367 "/root/bddd/build/parser.cpp"
    break;

  case 33:
#line 142 "/root/bddd/src/parser/parser.y"
                      { yystack_[0].value.as < unique_ptr<DeclAST> > ()->SetVarType(yystack_[1].value.as < VarType > ()); yylhs.value.as < vector<unique_ptr<DeclAST>> > ().push_back(std::move(yystack_[0].value.as < unique_ptr<DeclAST> > ())); }
#line 1373 "/root/bddd/build/parser.cpp"
    break;

  case 34:
#line 143 "/root/bddd/src/parser/parser.y"
                                  { yylhs.value.as < vector<unique_ptr<DeclAST>> > ().assign(std::make_move_iterator(std::begin(yystack_[2].value.as < vector<unique_ptr<DeclAST>> > ())), std::make_move_iterator(std::end(yystack_[2].value.as < vector<unique_ptr<DeclAST>> > ()))); yystack_[2].value.as < vector<unique_ptr<DeclAST>> > ().clear(); yystack_[0].value.as < unique_ptr<DeclAST> > ()->SetVarType(yylhs.value.as < vector<unique_ptr<DeclAST>> > ()[0]->GetVarType()); yylhs.value.as < vector<unique_ptr<DeclAST>> > ().push_back(std::move(yystack_[0].value.as < unique_ptr<DeclAST> > ())); }
#line 1379 "/root/bddd/build/parser.cpp"
    break;

  case 35:
#line 146 "/root/bddd/src/parser/parser.y"
                     { yylhs.value.as < unique_ptr<DeclAST> > () = std::move(yystack_[0].value.as < unique_ptr<DeclAST> > ()); }
#line 1385 "/root/bddd/build/parser.cpp"
    break;

  case 36:
#line 147 "/root/bddd/src/parser/parser.y"
                    { yylhs.value.as < unique_ptr<DeclAST> > () = std::move(yystack_[0].value.as < unique_ptr<DeclAST> > ()); }
#line 1391 "/root/bddd/build/parser.cpp"
    break;

  case 37:
#line 149 "/root/bddd/src/parser/parser.y"
                                       { yylhs.value.as < unique_ptr<DeclAST> > () = std::make_unique<DeclAST>(std::move(yystack_[2].value.as < string > ()), std::move(yystack_[0].value.as < unique_ptr<InitValAST> > ())); }
#line 1397 "/root/bddd/build/parser.cpp"
    break;

  case 38:
#line 150 "/root/bddd/src/parser/parser.y"
                    { yylhs.value.as < unique_ptr<DeclAST> > () = std::make_unique<DeclAST>(std::move(yystack_[0].value.as < string > ())); }
#line 1403 "/root/bddd/build/parser.cpp"
    break;

  case 39:
#line 152 "/root/bddd/src/parser/parser.y"
                                                  { yylhs.value.as < unique_ptr<DeclAST> > () = std::move(yystack_[2].value.as < unique_ptr<DeclAST> > ()); yylhs.value.as < unique_ptr<DeclAST> > ()->SetInitVal(std::move(yystack_[0].value.as < unique_ptr<InitValAST> > ())); }
#line 1409 "/root/bddd/build/parser.cpp"
    break;

  case 40:
#line 153 "/root/bddd/src/parser/parser.y"
                          { yylhs.value.as < unique_ptr<DeclAST> > () = std::move(yystack_[0].value.as < unique_ptr<DeclAST> > ()); }
#line 1415 "/root/bddd/build/parser.cpp"
    break;

  case 41:
#line 156 "/root/bddd/src/parser/parser.y"
                                                         { yylhs.value.as < unique_ptr<DeclAST> > () = std::move(yystack_[3].value.as < unique_ptr<DeclAST> > ()); yylhs.value.as < unique_ptr<DeclAST> > ()->AddDimension(std::move(yystack_[1].value.as < unique_ptr<ExprAST> > ())); }
#line 1421 "/root/bddd/build/parser.cpp"
    break;

  case 42:
#line 157 "/root/bddd/src/parser/parser.y"
                                           { yylhs.value.as < unique_ptr<DeclAST> > () = std::make_unique<DeclAST>(yystack_[3].value.as < string > ()); yylhs.value.as < unique_ptr<DeclAST> > ()->AddDimension(std::move(yystack_[1].value.as < unique_ptr<ExprAST> > ())); }
#line 1427 "/root/bddd/build/parser.cpp"
    break;

  case 43:
#line 160 "/root/bddd/src/parser/parser.y"
                         { yylhs.value.as < unique_ptr<DeclAST> > () = std::move(yystack_[0].value.as < unique_ptr<DeclAST> > ()); }
#line 1433 "/root/bddd/build/parser.cpp"
    break;

  case 44:
#line 161 "/root/bddd/src/parser/parser.y"
                        { yylhs.value.as < unique_ptr<DeclAST> > () = std::move(yystack_[0].value.as < unique_ptr<DeclAST> > ()); }
#line 1439 "/root/bddd/build/parser.cpp"
    break;

  case 45:
#line 164 "/root/bddd/src/parser/parser.y"
                                         { yylhs.value.as < unique_ptr<DeclAST> > () = std::make_unique<DeclAST>(std::move(yystack_[2].value.as < string > ()), std::move(yystack_[0].value.as < unique_ptr<InitValAST> > ())); }
#line 1445 "/root/bddd/build/parser.cpp"
    break;

  case 46:
#line 167 "/root/bddd/src/parser/parser.y"
                                                    { yylhs.value.as < unique_ptr<DeclAST> > () = std::move(yystack_[2].value.as < unique_ptr<DeclAST> > ()); yylhs.value.as < unique_ptr<DeclAST> > ()->SetInitVal(std::move(yystack_[0].value.as < unique_ptr<InitValAST> > ())); }
#line 1451 "/root/bddd/build/parser.cpp"
    break;

  case 47:
#line 170 "/root/bddd/src/parser/parser.y"
                { yylhs.value.as < unique_ptr<InitValAST> > () = std::make_unique<InitValAST>(std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1457 "/root/bddd/build/parser.cpp"
    break;

  case 48:
#line 172 "/root/bddd/src/parser/parser.y"
                                                     { yylhs.value.as < unique_ptr<InitValAST> > () = std::move(yystack_[1].value.as < unique_ptr<InitValAST> > ()); }
#line 1463 "/root/bddd/build/parser.cpp"
    break;

  case 49:
#line 173 "/root/bddd/src/parser/parser.y"
                                    { yylhs.value.as < unique_ptr<InitValAST> > () = std::make_unique<InitValAST>(); }
#line 1469 "/root/bddd/build/parser.cpp"
    break;

  case 50:
#line 176 "/root/bddd/src/parser/parser.y"
                                                          { yylhs.value.as < unique_ptr<InitValAST> > () = std::move(yystack_[2].value.as < unique_ptr<InitValAST> > ()); yylhs.value.as < unique_ptr<InitValAST> > ()->AppendVal(std::move(yystack_[0].value.as < unique_ptr<InitValAST> > ())); }
#line 1475 "/root/bddd/build/parser.cpp"
    break;

  case 51:
#line 177 "/root/bddd/src/parser/parser.y"
                                                     { yylhs.value.as < unique_ptr<InitValAST> > () = std::move(yystack_[2].value.as < unique_ptr<InitValAST> > ()); yylhs.value.as < unique_ptr<InitValAST> > ()->AppendVal(std::move(yystack_[0].value.as < unique_ptr<InitValAST> > ())); }
#line 1481 "/root/bddd/build/parser.cpp"
    break;

  case 52:
#line 178 "/root/bddd/src/parser/parser.y"
                               { yylhs.value.as < unique_ptr<InitValAST> > () = std::make_unique<InitValAST>(std::move(yystack_[0].value.as < unique_ptr<InitValAST> > ())); }
#line 1487 "/root/bddd/build/parser.cpp"
    break;

  case 53:
#line 179 "/root/bddd/src/parser/parser.y"
                          { yylhs.value.as < unique_ptr<InitValAST> > () = std::make_unique<InitValAST>(std::move(yystack_[0].value.as < unique_ptr<InitValAST> > ())); }
#line 1493 "/root/bddd/build/parser.cpp"
    break;

  case 54:
#line 182 "/root/bddd/src/parser/parser.y"
            { yylhs.value.as < unique_ptr<ExprAST> > () = std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ()); }
#line 1499 "/root/bddd/build/parser.cpp"
    break;

  case 55:
#line 184 "/root/bddd/src/parser/parser.y"
                              { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(Op::OR, std::move(yystack_[2].value.as < unique_ptr<ExprAST> > ()), std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1505 "/root/bddd/build/parser.cpp"
    break;

  case 56:
#line 185 "/root/bddd/src/parser/parser.y"
                { yylhs.value.as < unique_ptr<ExprAST> > () = std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ()); }
#line 1511 "/root/bddd/build/parser.cpp"
    break;

  case 57:
#line 188 "/root/bddd/src/parser/parser.y"
               { yylhs.value.as < unique_ptr<ExprAST> > () = std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ()); }
#line 1517 "/root/bddd/build/parser.cpp"
    break;

  case 58:
#line 189 "/root/bddd/src/parser/parser.y"
                               { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(Op::AND, std::move(yystack_[2].value.as < unique_ptr<ExprAST> > ()), std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1523 "/root/bddd/build/parser.cpp"
    break;

  case 59:
#line 192 "/root/bddd/src/parser/parser.y"
              { yylhs.value.as < unique_ptr<ExprAST> > () = std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ()); }
#line 1529 "/root/bddd/build/parser.cpp"
    break;

  case 60:
#line 193 "/root/bddd/src/parser/parser.y"
                            { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(Op::EQ, std::move(yystack_[2].value.as < unique_ptr<ExprAST> > ()), std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1535 "/root/bddd/build/parser.cpp"
    break;

  case 61:
#line 194 "/root/bddd/src/parser/parser.y"
                             { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(Op::NEQ, std::move(yystack_[2].value.as < unique_ptr<ExprAST> > ()), std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1541 "/root/bddd/build/parser.cpp"
    break;

  case 62:
#line 197 "/root/bddd/src/parser/parser.y"
               { yylhs.value.as < unique_ptr<ExprAST> > () = std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ()); }
#line 1547 "/root/bddd/build/parser.cpp"
    break;

  case 63:
#line 198 "/root/bddd/src/parser/parser.y"
                            { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(yystack_[1].value.as < Op > (), std::move(yystack_[2].value.as < unique_ptr<ExprAST> > ()), std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1553 "/root/bddd/build/parser.cpp"
    break;

  case 64:
#line 201 "/root/bddd/src/parser/parser.y"
               { yylhs.value.as < unique_ptr<ExprAST> > () = std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ()); }
#line 1559 "/root/bddd/build/parser.cpp"
    break;

  case 65:
#line 202 "/root/bddd/src/parser/parser.y"
                            { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(yystack_[1].value.as < Op > (), std::move(yystack_[2].value.as < unique_ptr<ExprAST> > ()), std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1565 "/root/bddd/build/parser.cpp"
    break;

  case 66:
#line 205 "/root/bddd/src/parser/parser.y"
                 { yylhs.value.as < unique_ptr<ExprAST> > () = std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ()); }
#line 1571 "/root/bddd/build/parser.cpp"
    break;

  case 67:
#line 206 "/root/bddd/src/parser/parser.y"
                              { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(yystack_[1].value.as < Op > (), std::move(yystack_[2].value.as < unique_ptr<ExprAST> > ()), std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1577 "/root/bddd/build/parser.cpp"
    break;

  case 68:
#line 209 "/root/bddd/src/parser/parser.y"
                     { yylhs.value.as < unique_ptr<ExprAST> > () = std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ()); }
#line 1583 "/root/bddd/build/parser.cpp"
    break;

  case 69:
#line 210 "/root/bddd/src/parser/parser.y"
                   { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(std::move(yystack_[0].value.as < unique_ptr<FuncCallAST> > ())); }
#line 1589 "/root/bddd/build/parser.cpp"
    break;

  case 70:
#line 211 "/root/bddd/src/parser/parser.y"
                           { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(yystack_[1].value.as < Op > (), std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1595 "/root/bddd/build/parser.cpp"
    break;

  case 71:
#line 214 "/root/bddd/src/parser/parser.y"
                                                  { yylhs.value.as < unique_ptr<FuncCallAST> > () = std::make_unique<FuncCallAST>(std::move(yystack_[3].value.as < string > ())); yylhs.value.as < unique_ptr<FuncCallAST> > ()->AssignParams(std::move(yystack_[1].value.as < vector<unique_ptr<ExprAST>> > ())); }
#line 1601 "/root/bddd/build/parser.cpp"
    break;

  case 72:
#line 215 "/root/bddd/src/parser/parser.y"
                                      { yylhs.value.as < unique_ptr<FuncCallAST> > () = std::make_unique<FuncCallAST>(std::move(yystack_[2].value.as < string > ())); }
#line 1607 "/root/bddd/build/parser.cpp"
    break;

  case 73:
#line 218 "/root/bddd/src/parser/parser.y"
                                      { yylhs.value.as < unique_ptr<ExprAST> > () = std::move(yystack_[1].value.as < unique_ptr<ExprAST> > ()); }
#line 1613 "/root/bddd/build/parser.cpp"
    break;

  case 74:
#line 219 "/root/bddd/src/parser/parser.y"
                 { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(std::move(yystack_[0].value.as < unique_ptr<LValAST> > ())); }
#line 1619 "/root/bddd/build/parser.cpp"
    break;

  case 75:
#line 220 "/root/bddd/src/parser/parser.y"
                   { yylhs.value.as < unique_ptr<ExprAST> > () = std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ()); }
#line 1625 "/root/bddd/build/parser.cpp"
    break;

  case 76:
#line 223 "/root/bddd/src/parser/parser.y"
                                         { yylhs.value.as < unique_ptr<LValAST> > () = std::move(yystack_[3].value.as < unique_ptr<LValAST> > ()); yylhs.value.as < unique_ptr<LValAST> > ()->AddDimension(std::move(yystack_[1].value.as < unique_ptr<ExprAST> > ())); }
#line 1631 "/root/bddd/build/parser.cpp"
    break;

  case 77:
#line 224 "/root/bddd/src/parser/parser.y"
            { yylhs.value.as < unique_ptr<LValAST> > () = std::make_unique<LValAST>(std::move(yystack_[0].value.as < string > ())); }
#line 1637 "/root/bddd/build/parser.cpp"
    break;

  case 78:
#line 227 "/root/bddd/src/parser/parser.y"
                                                                { yylhs.value.as < unique_ptr<FuncDefAST> > () = std::make_unique<FuncDefAST>(VarType::VOID, std::move(yystack_[4].value.as < string > ()), std::move(yystack_[2].value.as < vector<unique_ptr<FuncFParamAST>> > ()), std::move(yystack_[0].value.as < unique_ptr<BlockAST> > ())); }
#line 1643 "/root/bddd/build/parser.cpp"
    break;

  case 79:
#line 228 "/root/bddd/src/parser/parser.y"
                                                    { yylhs.value.as < unique_ptr<FuncDefAST> > () = std::make_unique<FuncDefAST>(VarType::VOID, std::move(yystack_[3].value.as < string > ()), std::move(yystack_[0].value.as < unique_ptr<BlockAST> > ())); }
#line 1649 "/root/bddd/build/parser.cpp"
    break;

  case 80:
#line 229 "/root/bddd/src/parser/parser.y"
                                                             { yylhs.value.as < unique_ptr<FuncDefAST> > () = std::make_unique<FuncDefAST>(yystack_[5].value.as < VarType > (), std::move(yystack_[4].value.as < string > ()), std::move(yystack_[2].value.as < vector<unique_ptr<FuncFParamAST>> > ()), std::move(yystack_[0].value.as < unique_ptr<BlockAST> > ())); }
#line 1655 "/root/bddd/build/parser.cpp"
    break;

  case 81:
#line 230 "/root/bddd/src/parser/parser.y"
                                                 { yylhs.value.as < unique_ptr<FuncDefAST> > () = std::make_unique<FuncDefAST>(yystack_[4].value.as < VarType > (), std::move(yystack_[3].value.as < string > ()), std::move(yystack_[0].value.as < unique_ptr<BlockAST> > ())); }
#line 1661 "/root/bddd/build/parser.cpp"
    break;

  case 82:
#line 233 "/root/bddd/src/parser/parser.y"
                                              { yylhs.value.as < vector<unique_ptr<FuncFParamAST>> > ().assign(std::make_move_iterator(std::begin(yystack_[2].value.as < vector<unique_ptr<FuncFParamAST>> > ())), std::make_move_iterator(std::end(yystack_[2].value.as < vector<unique_ptr<FuncFParamAST>> > ()))); yystack_[2].value.as < vector<unique_ptr<FuncFParamAST>> > ().clear(); yylhs.value.as < vector<unique_ptr<FuncFParamAST>> > ().push_back(std::move(yystack_[0].value.as < unique_ptr<FuncFParamAST> > ())); }
#line 1667 "/root/bddd/build/parser.cpp"
    break;

  case 83:
#line 234 "/root/bddd/src/parser/parser.y"
                        { yylhs.value.as < vector<unique_ptr<FuncFParamAST>> > ().push_back(std::move(yystack_[0].value.as < unique_ptr<FuncFParamAST> > ())); }
#line 1673 "/root/bddd/build/parser.cpp"
    break;

  case 84:
#line 237 "/root/bddd/src/parser/parser.y"
                                          { yylhs.value.as < vector<unique_ptr<ExprAST>> > ().assign(std::make_move_iterator(std::begin(yystack_[2].value.as < vector<unique_ptr<ExprAST>> > ())), std::make_move_iterator(std::end(yystack_[2].value.as < vector<unique_ptr<ExprAST>> > ()))); yystack_[2].value.as < vector<unique_ptr<ExprAST>> > ().clear(); yylhs.value.as < vector<unique_ptr<ExprAST>> > ().push_back(std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1679 "/root/bddd/build/parser.cpp"
    break;

  case 85:
#line 238 "/root/bddd/src/parser/parser.y"
                    { yylhs.value.as < vector<unique_ptr<ExprAST>> > ().push_back(std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1685 "/root/bddd/build/parser.cpp"
    break;

  case 86:
#line 241 "/root/bddd/src/parser/parser.y"
                             { yylhs.value.as < unique_ptr<FuncFParamAST> > () = std::move(yystack_[0].value.as < unique_ptr<FuncFParamAST> > ()); }
#line 1691 "/root/bddd/build/parser.cpp"
    break;

  case 87:
#line 242 "/root/bddd/src/parser/parser.y"
                            { yylhs.value.as < unique_ptr<FuncFParamAST> > () = std::move(yystack_[0].value.as < unique_ptr<FuncFParamAST> > ()); }
#line 1697 "/root/bddd/build/parser.cpp"
    break;

  case 88:
#line 245 "/root/bddd/src/parser/parser.y"
                              { yylhs.value.as < unique_ptr<FuncFParamAST> > () = std::make_unique<FuncFParamAST>(yystack_[1].value.as < VarType > (), std::move(yystack_[0].value.as < string > ())); }
#line 1703 "/root/bddd/build/parser.cpp"
    break;

  case 89:
#line 247 "/root/bddd/src/parser/parser.y"
                                                       { yylhs.value.as < unique_ptr<FuncFParamAST> > () = std::make_unique<FuncFParamAST>(yystack_[3].value.as < VarType > (), std::move(yystack_[2].value.as < string > ())); yylhs.value.as < unique_ptr<FuncFParamAST> > ()->AddDimension(nullptr); }
#line 1709 "/root/bddd/build/parser.cpp"
    break;

  case 90:
#line 248 "/root/bddd/src/parser/parser.y"
                                                               { yylhs.value.as < unique_ptr<FuncFParamAST> > () = std::move(yystack_[3].value.as < unique_ptr<FuncFParamAST> > ()); yylhs.value.as < unique_ptr<FuncFParamAST> > ()->AddDimension(std::move(yystack_[1].value.as < unique_ptr<ExprAST> > ())); }
#line 1715 "/root/bddd/build/parser.cpp"
    break;

  case 91:
#line 251 "/root/bddd/src/parser/parser.y"
                             { yylhs.value.as < unique_ptr<BlockAST> > () = std::make_unique<BlockAST>(); }
#line 1721 "/root/bddd/build/parser.cpp"
    break;

  case 92:
#line 252 "/root/bddd/src/parser/parser.y"
                                        { yylhs.value.as < unique_ptr<BlockAST> > () = std::move(yystack_[1].value.as < unique_ptr<BlockAST> > ()); }
#line 1727 "/root/bddd/build/parser.cpp"
    break;

  case 93:
#line 255 "/root/bddd/src/parser/parser.y"
                      { yylhs.value.as < unique_ptr<BlockAST> > () = std::make_unique<BlockAST>(); yylhs.value.as < unique_ptr<BlockAST> > ()->AppendNodes(std::move(yystack_[0].value.as < vector<unique_ptr<AST>> > ())); }
#line 1733 "/root/bddd/build/parser.cpp"
    break;

  case 94:
#line 256 "/root/bddd/src/parser/parser.y"
                                 { yylhs.value.as < unique_ptr<BlockAST> > () = std::move(yystack_[1].value.as < unique_ptr<BlockAST> > ()); yylhs.value.as < unique_ptr<BlockAST> > ()->AppendNodes(std::move(yystack_[0].value.as < vector<unique_ptr<AST>> > ())); }
#line 1739 "/root/bddd/build/parser.cpp"
    break;

  case 95:
#line 259 "/root/bddd/src/parser/parser.y"
                { yylhs.value.as < vector<unique_ptr<AST>> > ().insert(std::end(yylhs.value.as < vector<unique_ptr<AST>> > ()), std::make_move_iterator(std::begin(yystack_[0].value.as < vector<unique_ptr<DeclAST>> > ())), std::make_move_iterator(std::end(yystack_[0].value.as < vector<unique_ptr<DeclAST>> > ()))); yystack_[0].value.as < vector<unique_ptr<DeclAST>> > ().clear(); }
#line 1745 "/root/bddd/build/parser.cpp"
    break;

  case 96:
#line 260 "/root/bddd/src/parser/parser.y"
                { yylhs.value.as < vector<unique_ptr<AST>> > ().push_back(std::move(yystack_[0].value.as < unique_ptr<StmtAST> > ())); }
#line 1751 "/root/bddd/build/parser.cpp"
    break;

  case 97:
#line 263 "/root/bddd/src/parser/parser.y"
                                        { yylhs.value.as < unique_ptr<StmtAST> > () = std::make_unique<AssignStmtAST>(std::move(yystack_[3].value.as < unique_ptr<LValAST> > ()), std::move(yystack_[1].value.as < unique_ptr<ExprAST> > ())); }
#line 1757 "/root/bddd/build/parser.cpp"
    break;

  case 98:
#line 264 "/root/bddd/src/parser/parser.y"
                        { yylhs.value.as < unique_ptr<StmtAST> > () = std::make_unique<EvalStmtAST>(std::move(yystack_[1].value.as < unique_ptr<ExprAST> > ())); }
#line 1763 "/root/bddd/build/parser.cpp"
    break;

  case 99:
#line 265 "/root/bddd/src/parser/parser.y"
                    { yylhs.value.as < unique_ptr<StmtAST> > () = std::make_unique<BlockAST>(); }
#line 1769 "/root/bddd/build/parser.cpp"
    break;

  case 100:
#line 266 "/root/bddd/src/parser/parser.y"
            { yylhs.value.as < unique_ptr<StmtAST> > () = std::move(yystack_[0].value.as < unique_ptr<BlockAST> > ()); }
#line 1775 "/root/bddd/build/parser.cpp"
    break;

  case 101:
#line 267 "/root/bddd/src/parser/parser.y"
             { yylhs.value.as < unique_ptr<StmtAST> > () = std::move(yystack_[0].value.as < unique_ptr<IfStmtAST> > ()); }
#line 1781 "/root/bddd/build/parser.cpp"
    break;

  case 102:
#line 268 "/root/bddd/src/parser/parser.y"
                { yylhs.value.as < unique_ptr<StmtAST> > () = std::move(yystack_[0].value.as < unique_ptr<WhileStmtAST> > ()); }
#line 1787 "/root/bddd/build/parser.cpp"
    break;

  case 103:
#line 269 "/root/bddd/src/parser/parser.y"
                { yylhs.value.as < unique_ptr<StmtAST> > () = std::move(yystack_[0].value.as < unique_ptr<BreakStmtAST> > ()); }
#line 1793 "/root/bddd/build/parser.cpp"
    break;

  case 104:
#line 270 "/root/bddd/src/parser/parser.y"
                   { yylhs.value.as < unique_ptr<StmtAST> > () = std::move(yystack_[0].value.as < unique_ptr<ContinueStmtAST> > ()); }
#line 1799 "/root/bddd/build/parser.cpp"
    break;

  case 105:
#line 271 "/root/bddd/src/parser/parser.y"
                 { yylhs.value.as < unique_ptr<StmtAST> > () = std::move(yystack_[0].value.as < unique_ptr<ReturnStmtAST> > ()); }
#line 1805 "/root/bddd/build/parser.cpp"
    break;

  case 106:
#line 274 "/root/bddd/src/parser/parser.y"
                                                             { yylhs.value.as < unique_ptr<IfStmtAST> > () = std::make_unique<IfStmtAST>(std::move(yystack_[4].value.as < unique_ptr<CondAST> > ()), std::move(yystack_[2].value.as < unique_ptr<StmtAST> > ()), std::move(yystack_[0].value.as < unique_ptr<StmtAST> > ())); }
#line 1811 "/root/bddd/build/parser.cpp"
    break;

  case 107:
#line 275 "/root/bddd/src/parser/parser.y"
                                               { yylhs.value.as < unique_ptr<IfStmtAST> > () = std::make_unique<IfStmtAST>(std::move(yystack_[2].value.as < unique_ptr<CondAST> > ()), std::move(yystack_[0].value.as < unique_ptr<StmtAST> > ())); }
#line 1817 "/root/bddd/build/parser.cpp"
    break;

  case 108:
#line 278 "/root/bddd/src/parser/parser.y"
                                         { yylhs.value.as < unique_ptr<ReturnStmtAST> > () = std::make_unique<ReturnStmtAST>(std::move(yystack_[1].value.as < unique_ptr<ExprAST> > ())); }
#line 1823 "/root/bddd/build/parser.cpp"
    break;

  case 109:
#line 279 "/root/bddd/src/parser/parser.y"
                                     { yylhs.value.as < unique_ptr<ReturnStmtAST> > () = std::make_unique<ReturnStmtAST>(); }
#line 1829 "/root/bddd/build/parser.cpp"
    break;

  case 110:
#line 282 "/root/bddd/src/parser/parser.y"
                                                     { yylhs.value.as < unique_ptr<WhileStmtAST> > () = std::make_unique<WhileStmtAST>(std::move(yystack_[2].value.as < unique_ptr<CondAST> > ()), std::move(yystack_[0].value.as < unique_ptr<StmtAST> > ())); }
#line 1835 "/root/bddd/build/parser.cpp"
    break;

  case 111:
#line 284 "/root/bddd/src/parser/parser.y"
                                   { yylhs.value.as < unique_ptr<BreakStmtAST> > () = std::make_unique<BreakStmtAST>(); }
#line 1841 "/root/bddd/build/parser.cpp"
    break;

  case 112:
#line 286 "/root/bddd/src/parser/parser.y"
                                         { yylhs.value.as < unique_ptr<ContinueStmtAST> > () = std::make_unique<ContinueStmtAST>(); }
#line 1847 "/root/bddd/build/parser.cpp"
    break;

  case 113:
#line 288 "/root/bddd/src/parser/parser.y"
             { yylhs.value.as < unique_ptr<CondAST> > () = std::make_unique<CondAST>(std::move(yystack_[0].value.as < unique_ptr<ExprAST> > ())); }
#line 1853 "/root/bddd/build/parser.cpp"
    break;

  case 114:
#line 290 "/root/bddd/src/parser/parser.y"
                 { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(yystack_[0].value.as < int > ()); /* printf("(intconst, %d)\n", $1); */ }
#line 1859 "/root/bddd/build/parser.cpp"
    break;

  case 115:
#line 291 "/root/bddd/src/parser/parser.y"
                 { yylhs.value.as < unique_ptr<ExprAST> > () = std::make_unique<ExprAST>(yystack_[0].value.as < float > ()); /* printf("(floatconst, %f)\n", $1); */ }
#line 1865 "/root/bddd/build/parser.cpp"
    break;

  case 116:
#line 293 "/root/bddd/src/parser/parser.y"
           { yylhs.value.as < Op > () = Op::PLUS;  /* printf("(op, +)\n"); */ }
#line 1871 "/root/bddd/build/parser.cpp"
    break;

  case 117:
#line 294 "/root/bddd/src/parser/parser.y"
           { yylhs.value.as < Op > () = Op::MINUS; /* printf("(op, -)\n"); */ }
#line 1877 "/root/bddd/build/parser.cpp"
    break;

  case 118:
#line 297 "/root/bddd/src/parser/parser.y"
           { yylhs.value.as < Op > () = Op::MULTI; /* printf("(op, *)\n"); */ }
#line 1883 "/root/bddd/build/parser.cpp"
    break;

  case 119:
#line 298 "/root/bddd/src/parser/parser.y"
           { yylhs.value.as < Op > () = Op::DIV;  /* printf("(op, /)\n"); */ }
#line 1889 "/root/bddd/build/parser.cpp"
    break;

  case 120:
#line 299 "/root/bddd/src/parser/parser.y"
           { yylhs.value.as < Op > () = Op::MOD;  /* printf("(op, %)\n"); */ }
#line 1895 "/root/bddd/build/parser.cpp"
    break;

  case 121:
#line 302 "/root/bddd/src/parser/parser.y"
             { yylhs.value.as < Op > () = Op::POSITIVE; /* printf("(op, +)\n"); */ }
#line 1901 "/root/bddd/build/parser.cpp"
    break;

  case 122:
#line 303 "/root/bddd/src/parser/parser.y"
             { yylhs.value.as < Op > () = Op::NEGATIVE; /* printf("(op, -)\n"); */ }
#line 1907 "/root/bddd/build/parser.cpp"
    break;

  case 123:
#line 304 "/root/bddd/src/parser/parser.y"
             { yylhs.value.as < Op > () = Op::NOT; /* printf("(op, !)\n"); */ }
#line 1913 "/root/bddd/build/parser.cpp"
    break;

  case 124:
#line 307 "/root/bddd/src/parser/parser.y"
           { yylhs.value.as < Op > () = Op::GE;   /* printf("(op, >)\n"); */ }
#line 1919 "/root/bddd/build/parser.cpp"
    break;

  case 125:
#line 308 "/root/bddd/src/parser/parser.y"
            { yylhs.value.as < Op > () = Op::GEQ; /* printf("(op, >=)\n"); */ }
#line 1925 "/root/bddd/build/parser.cpp"
    break;

  case 126:
#line 309 "/root/bddd/src/parser/parser.y"
           { yylhs.value.as < Op > () = Op::LE;   /* printf("(op, <)\n"); */ }
#line 1931 "/root/bddd/build/parser.cpp"
    break;

  case 127:
#line 310 "/root/bddd/src/parser/parser.y"
            { yylhs.value.as < Op > () = Op::LEQ; /* printf("(op, <=)\n"); */ }
#line 1937 "/root/bddd/build/parser.cpp"
    break;

  case 128:
#line 313 "/root/bddd/src/parser/parser.y"
                  { yylhs.value.as < string > () = std::move(yystack_[0].value.as < string > ()); /* printf("(identifier, %s)\n", $1.c_str()); */ }
#line 1943 "/root/bddd/build/parser.cpp"
    break;


#line 1947 "/root/bddd/build/parser.cpp"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;
      YY_STACK_PRINT ();

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        error (yyla.location, yysyntax_error_ (yystack_[0].state, yyla));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.type_get () == yyeof_)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    {
      stack_symbol_type error_token;
      for (;;)
        {
          yyn = yypact_[+yystack_[0].state];
          if (!yy_pact_value_is_default_ (yyn))
            {
              yyn += yy_error_token_;
              if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == yy_error_token_)
                {
                  yyn = yytable_[yyn];
                  if (0 < yyn)
                    break;
                }
            }

          // Pop the current state because it cannot handle the error token.
          if (yystack_.size () == 1)
            YYABORT;

          yyerror_range[1].location = yystack_[0].location;
          yy_destroy_ ("Error: popping", yystack_[0]);
          yypop_ ();
          YY_STACK_PRINT ();
        }

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  // Generate an error message.
  std::string
  parser::yysyntax_error_ (state_type yystate, const symbol_type& yyla) const
  {
    // Number of reported tokens (one for the "unexpected", one per
    // "expected").
    std::ptrdiff_t yycount = 0;
    // Its maximum.
    enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
    // Arguments of yyformat.
    char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];

    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state merging
         (from LALR or IELR) and default reductions corrupt the expected
         token list.  However, the list is correct for canonical LR with
         one exception: it will still contain any token that will not be
         accepted due to an error action in a later state.
    */
    if (!yyla.empty ())
      {
        symbol_number_type yytoken = yyla.type_get ();
        yyarg[yycount++] = yytname_[yytoken];

        int yyn = yypact_[+yystate];
        if (!yy_pact_value_is_default_ (yyn))
          {
            /* Start YYX at -YYN if negative to avoid negative indexes in
               YYCHECK.  In other words, skip the first -YYN actions for
               this state because they are default actions.  */
            int yyxbegin = yyn < 0 ? -yyn : 0;
            // Stay within bounds of both yycheck and yytname.
            int yychecklim = yylast_ - yyn + 1;
            int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
            for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
              if (yycheck_[yyx + yyn] == yyx && yyx != yy_error_token_
                  && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
                {
                  if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                    {
                      yycount = 1;
                      break;
                    }
                  else
                    yyarg[yycount++] = yytname_[yyx];
                }
          }
      }

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    std::ptrdiff_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += yytnamerr_ (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const signed char parser::yypact_ninf_ = -97;

  const signed char parser::yytable_ninf_ = -1;

  const short
  parser::yypact_[] =
  {
      68,   -97,   -97,   -97,   -97,    21,    10,    82,   -97,    10,
     137,   137,   -97,    10,   -97,    23,   -97,   -97,   -97,   -97,
     -97,   -97,    86,   103,   -97,   -97,   -97,    10,   -97,    10,
      86,   -97,   -97,   -97,    86,   -97,    44,   -97,   -97,     7,
     228,   228,    44,   228,   -97,   -97,    86,     7,   228,   -97,
       7,    10,    54,   -97,   -97,    37,   -97,   196,   -97,   -97,
     -97,   -97,   -97,   -97,   228,    41,   139,   101,   -97,   -97,
     -97,    37,   -97,   228,    23,   -97,   139,     7,    54,    41,
     -97,   -97,   148,   -97,    37,    21,     7,   228,   -97,   -97,
     -97,   -97,    35,    42,   -97,   -97,   -97,   -97,   228,   -97,
     -97,   -97,   228,   228,   -97,   199,   -97,     7,   -97,   -97,
     -97,   -97,   -97,   -97,   -97,   -97,   213,    23,    23,    67,
      67,   -97,    10,    67,    86,   -97,   148,   -97,   -97,   -97,
     -97,   -97,   -97,   -97,    41,   -97,   -97,    41,   237,   -97,
     -97,   101,   -97,    41,   -97,   139,    54,   -97,   -97,    67,
     228,   228,   -97,   -97,   -97,   228,   -97,   -97,   -97,   -97,
     -97,   -97,   -97,   228,   -97,   -97,    47,    70,   -97,    63,
     139,    42,    42,    67,   139,   -97,   228,   -97,   228,   -97,
     -97,   -97,   -97,   -97,   -97,   228,   228,   228,   172,   172,
     -97,    70,   -97,   112,   112,   139,    84,   -97,   -97,   172,
     -97
  };

  const unsigned char
  parser::yydefact_[] =
  {
       0,     3,    29,    30,    10,     0,     0,     0,    25,     0,
       0,     0,    26,     0,   128,     0,     1,    23,    24,    33,
      35,    36,    40,    38,     4,     2,    27,     0,    28,     0,
       0,    31,    43,    44,     0,    11,     0,    15,     5,     0,
       0,     0,     0,     0,    32,    34,    38,     0,     0,    12,
       0,     0,     0,    83,    86,    87,    13,     0,    39,   114,
     115,   121,   122,   123,     0,     0,    54,    64,    66,    69,
      68,    74,    75,     0,    77,    37,    47,     0,     0,     0,
      46,    45,     0,    79,    88,     0,     0,     0,    14,    49,
      53,    52,     0,     0,    16,    41,   116,   117,     0,   118,
     119,   120,     0,     0,    70,     0,    81,     0,    42,    18,
      20,    21,    22,    17,    99,    91,     0,     0,     0,     0,
       0,    95,     0,     0,    74,   100,     0,    93,    96,   101,
     105,   102,   103,   104,     0,    82,    78,     0,     0,    48,
      73,    65,    67,     0,    72,    85,     0,    80,   109,     0,
       0,     0,   111,   112,    98,     0,    92,    94,    89,    90,
      51,    50,    76,     0,    71,   108,   113,    56,    57,    59,
      62,     0,     0,     0,    84,     7,     0,     6,     0,   124,
     125,   126,   127,     8,     9,     0,     0,     0,     0,     0,
      97,    55,    58,    60,    61,    63,   107,   110,    19,     0,
     106
  };

  const short
  parser::yypgoto_[] =
  {
     -97,   -10,   -97,    -8,     4,   -97,   -97,   -97,   -97,   -97,
      -6,   -38,     3,   -46,     2,   -71,   -97,   -97,   -97,   -97,
     -97,   -97,   -97,    74,     6,   -97,   -97,    91,   -97,   -97,
      20,    96,   -97,   -97,   -20,   -22,   -97,   -33,   -97,   -39,
     -16,    -5,   -36,    60,   -58,   -97,   -97,   -64,   160,   127,
     -97,    94,   -97,   -97,   -28,   -97,    56,   -96,   -97,   -97,
     -97,   -97,   -97,    34,   -97,   -97,   -97,   -97,   -97,    14
  };

  const short
  parser::yydefgoto_[] =
  {
      -1,   114,     5,    85,    41,   178,   176,   185,   186,     6,
      64,    50,    82,    89,    43,    95,   116,   117,   199,   118,
     119,   120,     7,   121,    51,    10,    11,    19,    20,    21,
      22,    31,    32,    33,    75,    58,    92,   123,   166,   167,
     168,   169,    66,    67,    68,    69,    70,    71,    12,    52,
     146,    53,    54,    55,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   171,    72,    98,   102,    73,   187,    74
  };

  const unsigned char
  parser::yytable_[] =
  {
      26,    28,    27,    29,    77,    76,     9,    65,   108,    36,
      79,    13,    76,     9,    86,   104,    14,    42,   124,    56,
      15,    76,    83,    23,    40,    80,    39,    34,    81,     2,
       3,    93,    40,    30,    47,    91,   115,    90,    48,    35,
     107,    34,    57,    46,   142,    24,   139,    30,    88,   106,
      57,    37,     2,     3,   137,   140,    94,    87,   136,    49,
      57,    49,   124,   158,    24,    84,   159,   144,   105,   145,
     143,    49,   162,   103,     8,     1,     2,     3,    25,   147,
     156,    17,    16,   149,   138,   175,   134,     4,   122,     1,
       2,     3,   196,   197,   179,   180,   181,   182,   183,   184,
      37,     4,    76,   200,    38,   198,   148,   177,   164,   152,
     153,   150,   151,   154,   170,   170,   161,    37,   160,    35,
      45,    38,   173,    44,   124,   124,   103,   174,   155,    99,
     100,   101,   122,   188,   189,   124,    46,   191,   163,   165,
     170,    57,   170,   179,   180,   181,   182,    24,    25,   170,
     170,   195,    59,    60,    14,     1,     2,     3,   141,    25,
      56,    88,   192,   190,    35,    96,    97,    18,   109,    78,
     110,   111,   112,   113,    61,    62,    59,    60,    14,   135,
     193,   194,   157,    25,    56,   172,     0,    63,    35,     0,
       0,     0,   109,     0,   110,   111,   112,   113,    61,    62,
      59,    60,    14,    59,    60,    14,     0,     0,    56,    88,
       0,    63,    35,     0,     0,    35,    49,    59,    60,    14,
       0,     0,    61,    62,    25,    61,    62,     0,     0,    35,
       0,     0,    59,    60,    14,    63,     0,     0,    63,    61,
      62,    59,    60,    14,    35,     0,     0,     0,     0,    56,
       0,     0,    63,    35,    61,    62,     0,     0,     0,     0,
       0,     0,     0,    61,    62,     0,     0,    63,     0,     0,
       0,     0,     0,     0,     0,     0,    63
  };

  const short
  parser::yycheck_[] =
  {
      10,    11,    10,    11,    42,    41,     0,    40,    79,    15,
      43,     5,    48,     7,    52,    73,     6,    23,    82,    12,
       6,    57,    50,     9,    22,    47,    22,    13,    48,     8,
       9,    64,    30,    13,    30,    57,    82,    57,    34,    16,
      78,    27,    39,    29,   102,    10,    92,    27,    13,    77,
      47,    14,     8,     9,    87,    93,    15,    55,    86,    17,
      57,    17,   126,   134,    10,    51,   137,   105,    74,   105,
     103,    17,   143,    71,     0,     7,     8,     9,    11,   107,
     126,     7,     0,   116,    92,    38,    84,    19,    82,     7,
       8,     9,   188,   189,    31,    32,    33,    34,    35,    36,
      14,    19,   138,   199,    18,    21,   116,    37,   146,   119,
     120,   117,   118,   123,   150,   151,   138,    14,   138,    16,
      29,    18,   155,    27,   188,   189,   124,   163,   124,    28,
      29,    30,   126,   171,   172,   199,   122,   176,   146,   149,
     176,   138,   178,    31,    32,    33,    34,    10,    11,   185,
     186,   187,     4,     5,     6,     7,     8,     9,    98,    11,
      12,    13,   178,   173,    16,    26,    27,     7,    20,    42,
      22,    23,    24,    25,    26,    27,     4,     5,     6,    85,
     185,   186,   126,    11,    12,   151,    -1,    39,    16,    -1,
      -1,    -1,    20,    -1,    22,    23,    24,    25,    26,    27,
       4,     5,     6,     4,     5,     6,    -1,    -1,    12,    13,
      -1,    39,    16,    -1,    -1,    16,    17,     4,     5,     6,
      -1,    -1,    26,    27,    11,    26,    27,    -1,    -1,    16,
      -1,    -1,     4,     5,     6,    39,    -1,    -1,    39,    26,
      27,     4,     5,     6,    16,    -1,    -1,    -1,    -1,    12,
      -1,    -1,    39,    16,    26,    27,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    26,    27,    -1,    -1,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39
  };

  const signed char
  parser::yystos_[] =
  {
       0,     7,     8,     9,    19,    42,    49,    62,    63,    64,
      65,    66,    88,    64,     6,   109,     0,    63,    88,    67,
      68,    69,    70,   109,    10,    11,    41,    43,    41,    43,
      70,    71,    72,    73,   109,    16,    50,    14,    18,    44,
      54,    44,    50,    54,    71,    67,   109,    44,    44,    17,
      51,    64,    89,    91,    92,    93,    12,    52,    75,     4,
       5,    26,    27,    39,    50,    77,    82,    83,    84,    85,
      86,    87,   104,   107,   109,    74,    82,    51,    89,    77,
      75,    74,    52,    94,   109,    43,    51,    54,    13,    53,
      74,    75,    76,    77,    15,    55,    26,    27,   105,    28,
      29,    30,   106,    54,    84,    50,    94,    51,    55,    20,
      22,    23,    24,    25,    41,    53,    56,    57,    59,    60,
      61,    63,    64,    77,    87,    94,    95,    96,    97,    98,
      99,   100,   101,   102,    54,    91,    94,    77,    43,    53,
      51,    83,    84,    77,    51,    82,    90,    94,    41,    77,
      50,    50,    41,    41,    41,    44,    53,    96,    55,    55,
      74,    75,    55,    43,    51,    41,    78,    79,    80,    81,
      82,   103,   103,    77,    82,    38,    46,    37,    45,    31,
      32,    33,    34,    35,    36,    47,    48,   108,    51,    51,
      41,    79,    80,    81,    81,    82,    97,    97,    21,    58,
      97
  };

  const signed char
  parser::yyr1_[] =
  {
       0,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    62,    62,    62,    63,    63,    64,
      64,    65,    65,    66,    66,    67,    67,    68,    68,    69,
      69,    70,    70,    71,    71,    72,    73,    74,    75,    75,
      76,    76,    76,    76,    77,    78,    78,    79,    79,    80,
      80,    80,    81,    81,    82,    82,    83,    83,    84,    84,
      84,    85,    85,    86,    86,    86,    87,    87,    88,    88,
      88,    88,    89,    89,    90,    90,    91,    91,    92,    93,
      93,    94,    94,    95,    95,    96,    96,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    98,    98,    99,    99,
     100,   101,   102,   103,   104,   104,   105,   105,   106,   106,
     106,   107,   107,   107,   108,   108,   108,   108,   109
  };

  const signed char
  parser::yyr2_[] =
  {
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     1,     1,     2,     2,     1,
       1,     3,     3,     2,     3,     1,     1,     3,     1,     3,
       1,     4,     4,     1,     1,     3,     3,     1,     3,     2,
       3,     3,     1,     1,     1,     3,     1,     1,     3,     1,
       3,     3,     1,     3,     1,     3,     1,     3,     1,     1,
       2,     4,     3,     3,     1,     1,     4,     1,     6,     5,
       6,     5,     3,     1,     3,     1,     1,     1,     2,     4,
       4,     2,     3,     1,     2,     1,     1,     4,     2,     1,
       1,     1,     1,     1,     1,     1,     7,     5,     3,     2,
       5,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1
  };



  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a yyntokens_, nonterminals.
  const char*
  const parser::yytname_[] =
  {
  "\"end of file\"", "error", "$undefined", "\"then\"", "\"int const\"",
  "\"float const\"", "\"identifier\"", "\"const\"", "\"int\"", "\"float\"",
  "\",\"", "\";\"", "\"{\"", "\"}\"", "\"[\"", "\"]\"", "\"(\"", "\")\"",
  "\"=\"", "\"void\"", "\"if\"", "\"else\"", "\"while\"", "\"break\"",
  "\"continue\"", "\"return\"", "\"+\"", "\"-\"", "\"*\"", "\"/\"",
  "\"%\"", "\">\"", "\">=\"", "\"<\"", "\"<=\"", "\"==\"", "\"!=\"",
  "\"&&\"", "\"||\"", "\"!\"", "$accept", "TOK_SEMICOLON", "TOK_CONST",
  "TOK_COMMA", "TOK_ASSIGN", "TOK_AND", "TOK_OR", "TOK_EQ", "TOK_NEQ",
  "TOK_VOID", "TOK_LPAREN", "TOK_RPAREN", "TOK_LBRACE", "TOK_RBRACE",
  "TOK_LBRACKET", "TOK_RBRACKET", "TOK_RETURN", "TOK_IF", "TOK_ELSE",
  "TOK_WHILE", "TOK_BREAK", "TOK_CONTINUE", "CompUnit", "Decl", "BType",
  "ConstDecl", "VarDecl", "VarDef", "VarDefSingle", "VarDefArray",
  "DefArrayBody", "ConstDef", "ConstDefSingle", "ConstDefArray", "InitVal",
  "InitValArray", "InitValArrayBody", "Exp", "LOrExp", "LAndExp", "EqExp",
  "RelExp", "AddExp", "MulExp", "UnaryExp", "FuncCall", "PrimaryExp",
  "LVal", "FuncDef", "FuncFParams", "FuncRParams", "FuncFParam",
  "FuncFParamSingle", "FuncFParamArray", "Block", "BlockItems",
  "BlockItem", "Stmt", "IfStmt", "ReturnStmt", "WhileStmt", "BreakStmt",
  "ContinueStmt", "Cond", "Number", "AddOp", "MulOp", "UnaryOp", "RelOp",
  "IDENT", YY_NULLPTR
  };

#if YYDEBUG
  const short
  parser::yyrline_[] =
  {
       0,   102,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   124,   125,   126,   127,   130,   131,   134,
     135,   138,   139,   142,   143,   146,   147,   149,   150,   152,
     153,   156,   157,   160,   161,   164,   167,   170,   172,   173,
     176,   177,   178,   179,   182,   184,   185,   188,   189,   192,
     193,   194,   197,   198,   201,   202,   205,   206,   209,   210,
     211,   214,   215,   218,   219,   220,   223,   224,   227,   228,
     229,   230,   233,   234,   237,   238,   241,   242,   245,   247,
     248,   251,   252,   255,   256,   259,   260,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   274,   275,   278,   279,
     282,   284,   286,   288,   290,   291,   293,   294,   297,   298,
     299,   302,   303,   304,   307,   308,   309,   310,   313
  };

  // Print the state stack on the debug stream.
  void
  parser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  parser::yy_reduce_print_ (int yyrule)
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG


} // yy
#line 2505 "/root/bddd/build/parser.cpp"

#line 319 "/root/bddd/src/parser/parser.y"


void yy::parser::error (const location_type& l, const std::string& m) {
  driver.error(l, m);
}
