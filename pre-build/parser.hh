// A Bison parser, made by GNU Bison 3.5.1.

// Skeleton interface for Bison LALR(1) parsers in C++

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


/**
 ** \file /root/bddd/build/parser.hh
 ** Define the yy::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

// Undocumented macros, especially those whose name start with YY_,
// are private implementation details.  Do not rely on them.

#ifndef YY_YY_ROOT_BDDD_BUILD_PARSER_HH_INCLUDED
# define YY_YY_ROOT_BDDD_BUILD_PARSER_HH_INCLUDED
// "%code requires" blocks.
#line 7 "/root/bddd/src/parser/parser.y"

#include "ast/ast.h"
#include <string>
#include <memory>
using std::string;
using std::vector;
using std::unique_ptr;
class Driver;

#line 58 "/root/bddd/build/parser.hh"


# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// Support move semantics when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// Support noexcept when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// Support constexpr when possible.
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif
# include "location.hh"

#ifndef YY_ASSERT
# include <cassert>
# define YY_ASSERT assert
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

namespace yy {
#line 192 "/root/bddd/build/parser.hh"




  /// A Bison parser.
  class parser
  {
  public:
#ifndef YYSTYPE
  /// A buffer to store and retrieve objects.
  ///
  /// Sort of a variant, but does not keep track of the nature
  /// of the stored data, since that knowledge is available
  /// via the current parser state.
  class semantic_type
  {
  public:
    /// Type of *this.
    typedef semantic_type self_type;

    /// Empty construction.
    semantic_type () YY_NOEXCEPT
      : yybuffer_ ()
    {}

    /// Construct and fill.
    template <typename T>
    semantic_type (YY_RVREF (T) t)
    {
      YY_ASSERT (sizeof (T) <= size);
      new (yyas_<T> ()) T (YY_MOVE (t));
    }

    /// Destruction, allowed only if empty.
    ~semantic_type () YY_NOEXCEPT
    {}

# if 201103L <= YY_CPLUSPLUS
    /// Instantiate a \a T in here from \a t.
    template <typename T, typename... U>
    T&
    emplace (U&&... u)
    {
      return *new (yyas_<T> ()) T (std::forward <U>(u)...);
    }
# else
    /// Instantiate an empty \a T in here.
    template <typename T>
    T&
    emplace ()
    {
      return *new (yyas_<T> ()) T ();
    }

    /// Instantiate a \a T in here from \a t.
    template <typename T>
    T&
    emplace (const T& t)
    {
      return *new (yyas_<T> ()) T (t);
    }
# endif

    /// Instantiate an empty \a T in here.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build ()
    {
      return emplace<T> ();
    }

    /// Instantiate a \a T in here from \a t.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build (const T& t)
    {
      return emplace<T> (t);
    }

    /// Accessor to a built \a T.
    template <typename T>
    T&
    as () YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Const accessor to a built \a T (for %printer).
    template <typename T>
    const T&
    as () const YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Swap the content with \a that, of same type.
    ///
    /// Both variants must be built beforehand, because swapping the actual
    /// data requires reading it (with as()), and this is not possible on
    /// unconstructed variants: it would require some dynamic testing, which
    /// should not be the variant's responsibility.
    /// Swapping between built and (possibly) non-built is done with
    /// self_type::move ().
    template <typename T>
    void
    swap (self_type& that) YY_NOEXCEPT
    {
      std::swap (as<T> (), that.as<T> ());
    }

    /// Move the content of \a that to this.
    ///
    /// Destroys \a that.
    template <typename T>
    void
    move (self_type& that)
    {
# if 201103L <= YY_CPLUSPLUS
      emplace<T> (std::move (that.as<T> ()));
# else
      emplace<T> ();
      swap<T> (that);
# endif
      that.destroy<T> ();
    }

# if 201103L <= YY_CPLUSPLUS
    /// Move the content of \a that to this.
    template <typename T>
    void
    move (self_type&& that)
    {
      emplace<T> (std::move (that.as<T> ()));
      that.destroy<T> ();
    }
#endif

    /// Copy the content of \a that to this.
    template <typename T>
    void
    copy (const self_type& that)
    {
      emplace<T> (that.as<T> ());
    }

    /// Destroy the stored \a T.
    template <typename T>
    void
    destroy ()
    {
      as<T> ().~T ();
    }

  private:
    /// Prohibit blind copies.
    self_type& operator= (const self_type&);
    semantic_type (const self_type&);

    /// Accessor to raw memory as \a T.
    template <typename T>
    T*
    yyas_ () YY_NOEXCEPT
    {
      void *yyp = yybuffer_.yyraw;
      return static_cast<T*> (yyp);
     }

    /// Const accessor to raw memory as \a T.
    template <typename T>
    const T*
    yyas_ () const YY_NOEXCEPT
    {
      const void *yyp = yybuffer_.yyraw;
      return static_cast<const T*> (yyp);
     }

    /// An auxiliary type to compute the largest semantic type.
    union union_type
    {
      // AddOp
      // MulOp
      // UnaryOp
      // RelOp
      char dummy1[sizeof (Op)];

      // BType
      char dummy2[sizeof (VarType)];

      // "float const"
      char dummy3[sizeof (float)];

      // "int const"
      char dummy4[sizeof (int)];

      // "identifier"
      // IDENT
      char dummy5[sizeof (string)];

      // Block
      // BlockItems
      char dummy6[sizeof (unique_ptr<BlockAST>)];

      // BreakStmt
      char dummy7[sizeof (unique_ptr<BreakStmtAST>)];

      // Cond
      char dummy8[sizeof (unique_ptr<CondAST>)];

      // ContinueStmt
      char dummy9[sizeof (unique_ptr<ContinueStmtAST>)];

      // VarDef
      // VarDefSingle
      // VarDefArray
      // DefArrayBody
      // ConstDef
      // ConstDefSingle
      // ConstDefArray
      char dummy10[sizeof (unique_ptr<DeclAST>)];

      // Exp
      // LOrExp
      // LAndExp
      // EqExp
      // RelExp
      // AddExp
      // MulExp
      // UnaryExp
      // PrimaryExp
      // Number
      char dummy11[sizeof (unique_ptr<ExprAST>)];

      // FuncCall
      char dummy12[sizeof (unique_ptr<FuncCallAST>)];

      // FuncDef
      char dummy13[sizeof (unique_ptr<FuncDefAST>)];

      // FuncFParam
      // FuncFParamSingle
      // FuncFParamArray
      char dummy14[sizeof (unique_ptr<FuncFParamAST>)];

      // IfStmt
      char dummy15[sizeof (unique_ptr<IfStmtAST>)];

      // InitVal
      // InitValArray
      // InitValArrayBody
      char dummy16[sizeof (unique_ptr<InitValAST>)];

      // LVal
      char dummy17[sizeof (unique_ptr<LValAST>)];

      // ReturnStmt
      char dummy18[sizeof (unique_ptr<ReturnStmtAST>)];

      // Stmt
      char dummy19[sizeof (unique_ptr<StmtAST>)];

      // WhileStmt
      char dummy20[sizeof (unique_ptr<WhileStmtAST>)];

      // BlockItem
      char dummy21[sizeof (vector<unique_ptr<AST>>)];

      // Decl
      // ConstDecl
      // VarDecl
      char dummy22[sizeof (vector<unique_ptr<DeclAST>>)];

      // FuncRParams
      char dummy23[sizeof (vector<unique_ptr<ExprAST>>)];

      // FuncFParams
      char dummy24[sizeof (vector<unique_ptr<FuncFParamAST>>)];
    };

    /// The size of the largest semantic type.
    enum { size = sizeof (union_type) };

    /// A buffer to store semantic values.
    union
    {
      /// Strongest alignment constraints.
      long double yyalign_me;
      /// A buffer large enough to store any of the semantic values.
      char yyraw[size];
    } yybuffer_;
  };

#else
    typedef YYSTYPE semantic_type;
#endif
    /// Symbol locations.
    typedef location location_type;

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m)
        : std::runtime_error (m)
        , location (l)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
        , location (s.location)
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;

      location_type location;
    };

    /// Tokens.
    struct token
    {
      enum yytokentype
      {
        TOK_END = 0,
        TOK_INTCONST = 259,
        TOK_FLOATCONST = 260,
        TOK_IDENTIFIER = 261,
        TOK_CONST = 262,
        TOK_INT = 263,
        TOK_FLOAT = 264,
        TOK_COMMA = 265,
        TOK_SEMICOLON = 266,
        TOK_LBRACE = 267,
        TOK_RBRACE = 268,
        TOK_LBRACKET = 269,
        TOK_RBRACKET = 270,
        TOK_LPAREN = 271,
        TOK_RPAREN = 272,
        TOK_ASSIGN = 273,
        TOK_VOID = 274,
        TOK_IF = 275,
        TOK_ELSE = 276,
        TOK_WHILE = 277,
        TOK_BREAK = 278,
        TOK_CONTINUE = 279,
        TOK_RETURN = 280,
        TOK_PLUS = 281,
        TOK_MINUS = 282,
        TOK_MULTI = 283,
        TOK_DIV = 284,
        TOK_MOD = 285,
        TOK_GE = 286,
        TOK_GEQ = 287,
        TOK_LE = 288,
        TOK_LEQ = 289,
        TOK_EQ = 290,
        TOK_NEQ = 291,
        TOK_AND = 292,
        TOK_OR = 293,
        TOK_NOT = 294
      };
    };

    /// (External) token type, as returned by yylex.
    typedef token::yytokentype token_type;

    /// Symbol type: an internal symbol number.
    typedef int symbol_number_type;

    /// The symbol type number to denote an empty symbol.
    enum { empty_symbol = -2 };

    /// Internal symbol number for tokens (subsumed by symbol_number_type).
    typedef signed char token_number_type;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol type
    /// via type_get ().
    ///
    /// Provide access to semantic value and location.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol ()
        : value ()
        , location ()
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      basic_symbol (basic_symbol&& that);
#endif

      /// Copy constructor.
      basic_symbol (const basic_symbol& that);

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, location_type&& l)
        : Base (t)
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const location_type& l)
        : Base (t)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, Op&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const Op& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, VarType&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const VarType& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, float&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const float& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, int&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const int& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, string&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const string& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<BlockAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<BlockAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<BreakStmtAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<BreakStmtAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<CondAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<CondAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<ContinueStmtAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<ContinueStmtAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<DeclAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<DeclAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<ExprAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<ExprAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<FuncCallAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<FuncCallAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<FuncDefAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<FuncDefAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<FuncFParamAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<FuncFParamAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<IfStmtAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<IfStmtAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<InitValAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<InitValAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<LValAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<LValAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<ReturnStmtAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<ReturnStmtAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<StmtAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<StmtAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, unique_ptr<WhileStmtAST>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const unique_ptr<WhileStmtAST>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, vector<unique_ptr<AST>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const vector<unique_ptr<AST>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, vector<unique_ptr<DeclAST>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const vector<unique_ptr<DeclAST>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, vector<unique_ptr<ExprAST>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const vector<unique_ptr<ExprAST>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, vector<unique_ptr<FuncFParamAST>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const vector<unique_ptr<FuncFParamAST>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

      /// Destroy the symbol.
      ~basic_symbol ()
      {
        clear ();
      }

      /// Destroy contents, and record that is empty.
      void clear ()
      {
        // User destructor.
        symbol_number_type yytype = this->type_get ();
        basic_symbol<Base>& yysym = *this;
        (void) yysym;
        switch (yytype)
        {
       default:
          break;
        }

        // Type destructor.
switch (yytype)
    {
      case 105: // AddOp
      case 106: // MulOp
      case 107: // UnaryOp
      case 108: // RelOp
        value.template destroy< Op > ();
        break;

      case 64: // BType
        value.template destroy< VarType > ();
        break;

      case 5: // "float const"
        value.template destroy< float > ();
        break;

      case 4: // "int const"
        value.template destroy< int > ();
        break;

      case 6: // "identifier"
      case 109: // IDENT
        value.template destroy< string > ();
        break;

      case 94: // Block
      case 95: // BlockItems
        value.template destroy< unique_ptr<BlockAST> > ();
        break;

      case 101: // BreakStmt
        value.template destroy< unique_ptr<BreakStmtAST> > ();
        break;

      case 103: // Cond
        value.template destroy< unique_ptr<CondAST> > ();
        break;

      case 102: // ContinueStmt
        value.template destroy< unique_ptr<ContinueStmtAST> > ();
        break;

      case 67: // VarDef
      case 68: // VarDefSingle
      case 69: // VarDefArray
      case 70: // DefArrayBody
      case 71: // ConstDef
      case 72: // ConstDefSingle
      case 73: // ConstDefArray
        value.template destroy< unique_ptr<DeclAST> > ();
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
        value.template destroy< unique_ptr<ExprAST> > ();
        break;

      case 85: // FuncCall
        value.template destroy< unique_ptr<FuncCallAST> > ();
        break;

      case 88: // FuncDef
        value.template destroy< unique_ptr<FuncDefAST> > ();
        break;

      case 91: // FuncFParam
      case 92: // FuncFParamSingle
      case 93: // FuncFParamArray
        value.template destroy< unique_ptr<FuncFParamAST> > ();
        break;

      case 98: // IfStmt
        value.template destroy< unique_ptr<IfStmtAST> > ();
        break;

      case 74: // InitVal
      case 75: // InitValArray
      case 76: // InitValArrayBody
        value.template destroy< unique_ptr<InitValAST> > ();
        break;

      case 87: // LVal
        value.template destroy< unique_ptr<LValAST> > ();
        break;

      case 99: // ReturnStmt
        value.template destroy< unique_ptr<ReturnStmtAST> > ();
        break;

      case 97: // Stmt
        value.template destroy< unique_ptr<StmtAST> > ();
        break;

      case 100: // WhileStmt
        value.template destroy< unique_ptr<WhileStmtAST> > ();
        break;

      case 96: // BlockItem
        value.template destroy< vector<unique_ptr<AST>> > ();
        break;

      case 63: // Decl
      case 65: // ConstDecl
      case 66: // VarDecl
        value.template destroy< vector<unique_ptr<DeclAST>> > ();
        break;

      case 90: // FuncRParams
        value.template destroy< vector<unique_ptr<ExprAST>> > ();
        break;

      case 89: // FuncFParams
        value.template destroy< vector<unique_ptr<FuncFParamAST>> > ();
        break;

      default:
        break;
    }

        Base::clear ();
      }

      /// Whether empty.
      bool empty () const YY_NOEXCEPT;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      semantic_type value;

      /// The location.
      location_type location;

    private:
#if YY_CPLUSPLUS < 201103L
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    /// Type access provider for token (enum) based symbols.
    struct by_type
    {
      /// Default constructor.
      by_type ();

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      by_type (by_type&& that);
#endif

      /// Copy constructor.
      by_type (const by_type& that);

      /// The symbol type as needed by the constructor.
      typedef token_type kind_type;

      /// Constructor from (external) token numbers.
      by_type (kind_type t);

      /// Record that this symbol is empty.
      void clear ();

      /// Steal the symbol type from \a that.
      void move (by_type& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_number_type type_get () const YY_NOEXCEPT;

      /// The symbol type.
      /// \a empty_symbol when empty.
      /// An int, not token_number_type, to be able to store empty_symbol.
      int type;
    };

    /// "External" symbols: returned by the scanner.
    struct symbol_type : basic_symbol<by_type>
    {
      /// Superclass.
      typedef basic_symbol<by_type> super_type;

      /// Empty symbol.
      symbol_type () {}

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, location_type l)
        : super_type(token_type (tok), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_END || tok == 258 || tok == token::TOK_CONST || tok == token::TOK_INT || tok == token::TOK_FLOAT || tok == token::TOK_COMMA || tok == token::TOK_SEMICOLON || tok == token::TOK_LBRACE || tok == token::TOK_RBRACE || tok == token::TOK_LBRACKET || tok == token::TOK_RBRACKET || tok == token::TOK_LPAREN || tok == token::TOK_RPAREN || tok == token::TOK_ASSIGN || tok == token::TOK_VOID || tok == token::TOK_IF || tok == token::TOK_ELSE || tok == token::TOK_WHILE || tok == token::TOK_BREAK || tok == token::TOK_CONTINUE || tok == token::TOK_RETURN || tok == token::TOK_PLUS || tok == token::TOK_MINUS || tok == token::TOK_MULTI || tok == token::TOK_DIV || tok == token::TOK_MOD || tok == token::TOK_GE || tok == token::TOK_GEQ || tok == token::TOK_LE || tok == token::TOK_LEQ || tok == token::TOK_EQ || tok == token::TOK_NEQ || tok == token::TOK_AND || tok == token::TOK_OR || tok == token::TOK_NOT);
      }
#else
      symbol_type (int tok, const location_type& l)
        : super_type(token_type (tok), l)
      {
        YY_ASSERT (tok == token::TOK_END || tok == 258 || tok == token::TOK_CONST || tok == token::TOK_INT || tok == token::TOK_FLOAT || tok == token::TOK_COMMA || tok == token::TOK_SEMICOLON || tok == token::TOK_LBRACE || tok == token::TOK_RBRACE || tok == token::TOK_LBRACKET || tok == token::TOK_RBRACKET || tok == token::TOK_LPAREN || tok == token::TOK_RPAREN || tok == token::TOK_ASSIGN || tok == token::TOK_VOID || tok == token::TOK_IF || tok == token::TOK_ELSE || tok == token::TOK_WHILE || tok == token::TOK_BREAK || tok == token::TOK_CONTINUE || tok == token::TOK_RETURN || tok == token::TOK_PLUS || tok == token::TOK_MINUS || tok == token::TOK_MULTI || tok == token::TOK_DIV || tok == token::TOK_MOD || tok == token::TOK_GE || tok == token::TOK_GEQ || tok == token::TOK_LE || tok == token::TOK_LEQ || tok == token::TOK_EQ || tok == token::TOK_NEQ || tok == token::TOK_AND || tok == token::TOK_OR || tok == token::TOK_NOT);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, float v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_FLOATCONST);
      }
#else
      symbol_type (int tok, const float& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_FLOATCONST);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, int v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_INTCONST);
      }
#else
      symbol_type (int tok, const int& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_INTCONST);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, string v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_IDENTIFIER);
      }
#else
      symbol_type (int tok, const string& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_IDENTIFIER);
      }
#endif
    };

    /// Build a parser object.
    parser (Driver & driver_yyarg);
    virtual ~parser ();

    /// Parse.  An alias for parse ().
    /// \returns  0 iff parsing succeeded.
    int operator() ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

    // Implementation of make_symbol for each symbol type.
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_END (location_type l)
      {
        return symbol_type (token::TOK_END, std::move (l));
      }
#else
      static
      symbol_type
      make_END (const location_type& l)
      {
        return symbol_type (token::TOK_END, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INTCONST (int v, location_type l)
      {
        return symbol_type (token::TOK_INTCONST, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INTCONST (const int& v, const location_type& l)
      {
        return symbol_type (token::TOK_INTCONST, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FLOATCONST (float v, location_type l)
      {
        return symbol_type (token::TOK_FLOATCONST, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_FLOATCONST (const float& v, const location_type& l)
      {
        return symbol_type (token::TOK_FLOATCONST, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IDENTIFIER (string v, location_type l)
      {
        return symbol_type (token::TOK_IDENTIFIER, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_IDENTIFIER (const string& v, const location_type& l)
      {
        return symbol_type (token::TOK_IDENTIFIER, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CONST (location_type l)
      {
        return symbol_type (token::TOK_CONST, std::move (l));
      }
#else
      static
      symbol_type
      make_CONST (const location_type& l)
      {
        return symbol_type (token::TOK_CONST, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INT (location_type l)
      {
        return symbol_type (token::TOK_INT, std::move (l));
      }
#else
      static
      symbol_type
      make_INT (const location_type& l)
      {
        return symbol_type (token::TOK_INT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FLOAT (location_type l)
      {
        return symbol_type (token::TOK_FLOAT, std::move (l));
      }
#else
      static
      symbol_type
      make_FLOAT (const location_type& l)
      {
        return symbol_type (token::TOK_FLOAT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COMMA (location_type l)
      {
        return symbol_type (token::TOK_COMMA, std::move (l));
      }
#else
      static
      symbol_type
      make_COMMA (const location_type& l)
      {
        return symbol_type (token::TOK_COMMA, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SEMICOLON (location_type l)
      {
        return symbol_type (token::TOK_SEMICOLON, std::move (l));
      }
#else
      static
      symbol_type
      make_SEMICOLON (const location_type& l)
      {
        return symbol_type (token::TOK_SEMICOLON, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LBRACE (location_type l)
      {
        return symbol_type (token::TOK_LBRACE, std::move (l));
      }
#else
      static
      symbol_type
      make_LBRACE (const location_type& l)
      {
        return symbol_type (token::TOK_LBRACE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RBRACE (location_type l)
      {
        return symbol_type (token::TOK_RBRACE, std::move (l));
      }
#else
      static
      symbol_type
      make_RBRACE (const location_type& l)
      {
        return symbol_type (token::TOK_RBRACE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LBRACKET (location_type l)
      {
        return symbol_type (token::TOK_LBRACKET, std::move (l));
      }
#else
      static
      symbol_type
      make_LBRACKET (const location_type& l)
      {
        return symbol_type (token::TOK_LBRACKET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RBRACKET (location_type l)
      {
        return symbol_type (token::TOK_RBRACKET, std::move (l));
      }
#else
      static
      symbol_type
      make_RBRACKET (const location_type& l)
      {
        return symbol_type (token::TOK_RBRACKET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LPAREN (location_type l)
      {
        return symbol_type (token::TOK_LPAREN, std::move (l));
      }
#else
      static
      symbol_type
      make_LPAREN (const location_type& l)
      {
        return symbol_type (token::TOK_LPAREN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RPAREN (location_type l)
      {
        return symbol_type (token::TOK_RPAREN, std::move (l));
      }
#else
      static
      symbol_type
      make_RPAREN (const location_type& l)
      {
        return symbol_type (token::TOK_RPAREN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ASSIGN (location_type l)
      {
        return symbol_type (token::TOK_ASSIGN, std::move (l));
      }
#else
      static
      symbol_type
      make_ASSIGN (const location_type& l)
      {
        return symbol_type (token::TOK_ASSIGN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_VOID (location_type l)
      {
        return symbol_type (token::TOK_VOID, std::move (l));
      }
#else
      static
      symbol_type
      make_VOID (const location_type& l)
      {
        return symbol_type (token::TOK_VOID, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IF (location_type l)
      {
        return symbol_type (token::TOK_IF, std::move (l));
      }
#else
      static
      symbol_type
      make_IF (const location_type& l)
      {
        return symbol_type (token::TOK_IF, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ELSE (location_type l)
      {
        return symbol_type (token::TOK_ELSE, std::move (l));
      }
#else
      static
      symbol_type
      make_ELSE (const location_type& l)
      {
        return symbol_type (token::TOK_ELSE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_WHILE (location_type l)
      {
        return symbol_type (token::TOK_WHILE, std::move (l));
      }
#else
      static
      symbol_type
      make_WHILE (const location_type& l)
      {
        return symbol_type (token::TOK_WHILE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BREAK (location_type l)
      {
        return symbol_type (token::TOK_BREAK, std::move (l));
      }
#else
      static
      symbol_type
      make_BREAK (const location_type& l)
      {
        return symbol_type (token::TOK_BREAK, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CONTINUE (location_type l)
      {
        return symbol_type (token::TOK_CONTINUE, std::move (l));
      }
#else
      static
      symbol_type
      make_CONTINUE (const location_type& l)
      {
        return symbol_type (token::TOK_CONTINUE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RETURN (location_type l)
      {
        return symbol_type (token::TOK_RETURN, std::move (l));
      }
#else
      static
      symbol_type
      make_RETURN (const location_type& l)
      {
        return symbol_type (token::TOK_RETURN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PLUS (location_type l)
      {
        return symbol_type (token::TOK_PLUS, std::move (l));
      }
#else
      static
      symbol_type
      make_PLUS (const location_type& l)
      {
        return symbol_type (token::TOK_PLUS, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MINUS (location_type l)
      {
        return symbol_type (token::TOK_MINUS, std::move (l));
      }
#else
      static
      symbol_type
      make_MINUS (const location_type& l)
      {
        return symbol_type (token::TOK_MINUS, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MULTI (location_type l)
      {
        return symbol_type (token::TOK_MULTI, std::move (l));
      }
#else
      static
      symbol_type
      make_MULTI (const location_type& l)
      {
        return symbol_type (token::TOK_MULTI, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DIV (location_type l)
      {
        return symbol_type (token::TOK_DIV, std::move (l));
      }
#else
      static
      symbol_type
      make_DIV (const location_type& l)
      {
        return symbol_type (token::TOK_DIV, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MOD (location_type l)
      {
        return symbol_type (token::TOK_MOD, std::move (l));
      }
#else
      static
      symbol_type
      make_MOD (const location_type& l)
      {
        return symbol_type (token::TOK_MOD, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GE (location_type l)
      {
        return symbol_type (token::TOK_GE, std::move (l));
      }
#else
      static
      symbol_type
      make_GE (const location_type& l)
      {
        return symbol_type (token::TOK_GE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GEQ (location_type l)
      {
        return symbol_type (token::TOK_GEQ, std::move (l));
      }
#else
      static
      symbol_type
      make_GEQ (const location_type& l)
      {
        return symbol_type (token::TOK_GEQ, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LE (location_type l)
      {
        return symbol_type (token::TOK_LE, std::move (l));
      }
#else
      static
      symbol_type
      make_LE (const location_type& l)
      {
        return symbol_type (token::TOK_LE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LEQ (location_type l)
      {
        return symbol_type (token::TOK_LEQ, std::move (l));
      }
#else
      static
      symbol_type
      make_LEQ (const location_type& l)
      {
        return symbol_type (token::TOK_LEQ, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_EQ (location_type l)
      {
        return symbol_type (token::TOK_EQ, std::move (l));
      }
#else
      static
      symbol_type
      make_EQ (const location_type& l)
      {
        return symbol_type (token::TOK_EQ, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NEQ (location_type l)
      {
        return symbol_type (token::TOK_NEQ, std::move (l));
      }
#else
      static
      symbol_type
      make_NEQ (const location_type& l)
      {
        return symbol_type (token::TOK_NEQ, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AND (location_type l)
      {
        return symbol_type (token::TOK_AND, std::move (l));
      }
#else
      static
      symbol_type
      make_AND (const location_type& l)
      {
        return symbol_type (token::TOK_AND, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OR (location_type l)
      {
        return symbol_type (token::TOK_OR, std::move (l));
      }
#else
      static
      symbol_type
      make_OR (const location_type& l)
      {
        return symbol_type (token::TOK_OR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NOT (location_type l)
      {
        return symbol_type (token::TOK_NOT, std::move (l));
      }
#else
      static
      symbol_type
      make_NOT (const location_type& l)
      {
        return symbol_type (token::TOK_NOT, l);
      }
#endif


  private:
    /// This class is not copyable.
    parser (const parser&);
    parser& operator= (const parser&);

    /// Stored state numbers (used for stacks).
    typedef unsigned char state_type;

    /// Generate an error message.
    /// \param yystate   the state where the error occurred.
    /// \param yyla      the lookahead token.
    virtual std::string yysyntax_error_ (state_type yystate,
                                         const symbol_type& yyla) const;

    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue);

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue);

    static const signed char yypact_ninf_;
    static const signed char yytable_ninf_;

    /// Convert a scanner token number \a t to a symbol number.
    /// In theory \a t should be a token_type, but character literals
    /// are valid, yet not members of the token_type enum.
    static token_number_type yytranslate_ (int t);

    // Tables.
    // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
    // STATE-NUM.
    static const short yypact_[];

    // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
    // Performed when YYTABLE does not specify something else to do.  Zero
    // means the default is an error.
    static const unsigned char yydefact_[];

    // YYPGOTO[NTERM-NUM].
    static const short yypgoto_[];

    // YYDEFGOTO[NTERM-NUM].
    static const short yydefgoto_[];

    // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
    // positive, shift that token.  If negative, reduce the rule whose
    // number is the opposite.  If YYTABLE_NINF, syntax error.
    static const unsigned char yytable_[];

    static const short yycheck_[];

    // YYSTOS[STATE-NUM] -- The (internal number of the) accessing
    // symbol of state STATE-NUM.
    static const signed char yystos_[];

    // YYR1[YYN] -- Symbol number of symbol that rule YYN derives.
    static const signed char yyr1_[];

    // YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.
    static const signed char yyr2_[];


    /// Convert the symbol name \a n to a form suitable for a diagnostic.
    static std::string yytnamerr_ (const char *n);


    /// For a symbol, its name in clear.
    static const char* const yytname_[];
#if YYDEBUG
    // YYRLINE[YYN] -- Source line where rule number YYN was defined.
    static const short yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r);
    /// Print the state stack on the debug stream.
    virtual void yystack_print_ ();

    /// Debugging level.
    int yydebug_;
    /// Debug stream.
    std::ostream* yycdebug_;

    /// \brief Display a symbol type, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state () YY_NOEXCEPT;

      /// The symbol type as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s) YY_NOEXCEPT;

      /// Copy constructor.
      by_state (const by_state& that) YY_NOEXCEPT;

      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol type from \a that.
      void move (by_state& that);

      /// The (internal) type number (corresponding to \a state).
      /// \a empty_symbol when empty.
      symbol_number_type type_get () const YY_NOEXCEPT;

      /// The state number used to denote an empty symbol.
      /// We use the initial state, as it does not have a value.
      enum { empty_state = 0 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Move or copy construction.
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      /// Assignment, needed by push_back by some old implementations.
      /// Moves the contents of that.
      stack_symbol_type& operator= (stack_symbol_type& that);

      /// Assignment, needed by push_back by other implementations.
      /// Needed by some other old implementations.
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    /// A stack with random access from its top.
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // Hide our reversed order.
      typedef typename S::reverse_iterator iterator;
      typedef typename S::const_reverse_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200)
        : seq_ (n)
      {}

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Steal the contents of \a t.
      ///
      /// Close to move-semantics.
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      /// Pop elements from the stack.
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      /// Pop all elements from the stack.
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      /// Number of elements on the stack.
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      std::ptrdiff_t
      ssize () const YY_NOEXCEPT
      {
        return std::ptrdiff_t (size ());
      }

      /// Iterator on top of the stack (going downwards).
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.rbegin ();
      }

      /// Bottom of the stack.
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.rend ();
      }

      /// Present a slice of the top of a stack.
      class slice
      {
      public:
        slice (const stack& stack, index_type range)
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
      stack (const stack&);
      stack& operator= (const stack&);
      /// The wrapped container.
      S seq_;
    };


    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param sym  the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a sym.value is stolen.
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    /// Pop \a n symbols from the stack.
    void yypop_ (int n = 1);

    /// Some specific tokens.
    static const token_number_type yy_error_token_ = 1;
    static const token_number_type yy_undef_token_ = 2;

    /// Constants.
    enum
    {
      yyeof_ = 0,
      yylast_ = 276,     ///< Last index in yytable_.
      yynnts_ = 70,  ///< Number of nonterminal symbols.
      yyfinal_ = 16, ///< Termination state number.
      yyntokens_ = 40  ///< Number of tokens.
    };


    // User arguments.
    Driver & driver;
  };

  inline
  parser::token_number_type
  parser::yytranslate_ (int t)
  {
    // YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to
    // TOKEN-NUM as returned by yylex.
    static
    const token_number_type
    translate_table[] =
    {
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39
    };
    const int user_token_number_max_ = 294;

    if (t <= 0)
      return yyeof_;
    else if (t <= user_token_number_max_)
      return translate_table[t];
    else
      return yy_undef_token_;
  }

  // basic_symbol.
#if 201103L <= YY_CPLUSPLUS
  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (basic_symbol&& that)
    : Base (std::move (that))
    , value ()
    , location (std::move (that.location))
  {
    switch (this->type_get ())
    {
      case 105: // AddOp
      case 106: // MulOp
      case 107: // UnaryOp
      case 108: // RelOp
        value.move< Op > (std::move (that.value));
        break;

      case 64: // BType
        value.move< VarType > (std::move (that.value));
        break;

      case 5: // "float const"
        value.move< float > (std::move (that.value));
        break;

      case 4: // "int const"
        value.move< int > (std::move (that.value));
        break;

      case 6: // "identifier"
      case 109: // IDENT
        value.move< string > (std::move (that.value));
        break;

      case 94: // Block
      case 95: // BlockItems
        value.move< unique_ptr<BlockAST> > (std::move (that.value));
        break;

      case 101: // BreakStmt
        value.move< unique_ptr<BreakStmtAST> > (std::move (that.value));
        break;

      case 103: // Cond
        value.move< unique_ptr<CondAST> > (std::move (that.value));
        break;

      case 102: // ContinueStmt
        value.move< unique_ptr<ContinueStmtAST> > (std::move (that.value));
        break;

      case 67: // VarDef
      case 68: // VarDefSingle
      case 69: // VarDefArray
      case 70: // DefArrayBody
      case 71: // ConstDef
      case 72: // ConstDefSingle
      case 73: // ConstDefArray
        value.move< unique_ptr<DeclAST> > (std::move (that.value));
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
        value.move< unique_ptr<ExprAST> > (std::move (that.value));
        break;

      case 85: // FuncCall
        value.move< unique_ptr<FuncCallAST> > (std::move (that.value));
        break;

      case 88: // FuncDef
        value.move< unique_ptr<FuncDefAST> > (std::move (that.value));
        break;

      case 91: // FuncFParam
      case 92: // FuncFParamSingle
      case 93: // FuncFParamArray
        value.move< unique_ptr<FuncFParamAST> > (std::move (that.value));
        break;

      case 98: // IfStmt
        value.move< unique_ptr<IfStmtAST> > (std::move (that.value));
        break;

      case 74: // InitVal
      case 75: // InitValArray
      case 76: // InitValArrayBody
        value.move< unique_ptr<InitValAST> > (std::move (that.value));
        break;

      case 87: // LVal
        value.move< unique_ptr<LValAST> > (std::move (that.value));
        break;

      case 99: // ReturnStmt
        value.move< unique_ptr<ReturnStmtAST> > (std::move (that.value));
        break;

      case 97: // Stmt
        value.move< unique_ptr<StmtAST> > (std::move (that.value));
        break;

      case 100: // WhileStmt
        value.move< unique_ptr<WhileStmtAST> > (std::move (that.value));
        break;

      case 96: // BlockItem
        value.move< vector<unique_ptr<AST>> > (std::move (that.value));
        break;

      case 63: // Decl
      case 65: // ConstDecl
      case 66: // VarDecl
        value.move< vector<unique_ptr<DeclAST>> > (std::move (that.value));
        break;

      case 90: // FuncRParams
        value.move< vector<unique_ptr<ExprAST>> > (std::move (that.value));
        break;

      case 89: // FuncFParams
        value.move< vector<unique_ptr<FuncFParamAST>> > (std::move (that.value));
        break;

      default:
        break;
    }

  }
#endif

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value ()
    , location (that.location)
  {
    switch (this->type_get ())
    {
      case 105: // AddOp
      case 106: // MulOp
      case 107: // UnaryOp
      case 108: // RelOp
        value.copy< Op > (YY_MOVE (that.value));
        break;

      case 64: // BType
        value.copy< VarType > (YY_MOVE (that.value));
        break;

      case 5: // "float const"
        value.copy< float > (YY_MOVE (that.value));
        break;

      case 4: // "int const"
        value.copy< int > (YY_MOVE (that.value));
        break;

      case 6: // "identifier"
      case 109: // IDENT
        value.copy< string > (YY_MOVE (that.value));
        break;

      case 94: // Block
      case 95: // BlockItems
        value.copy< unique_ptr<BlockAST> > (YY_MOVE (that.value));
        break;

      case 101: // BreakStmt
        value.copy< unique_ptr<BreakStmtAST> > (YY_MOVE (that.value));
        break;

      case 103: // Cond
        value.copy< unique_ptr<CondAST> > (YY_MOVE (that.value));
        break;

      case 102: // ContinueStmt
        value.copy< unique_ptr<ContinueStmtAST> > (YY_MOVE (that.value));
        break;

      case 67: // VarDef
      case 68: // VarDefSingle
      case 69: // VarDefArray
      case 70: // DefArrayBody
      case 71: // ConstDef
      case 72: // ConstDefSingle
      case 73: // ConstDefArray
        value.copy< unique_ptr<DeclAST> > (YY_MOVE (that.value));
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
        value.copy< unique_ptr<ExprAST> > (YY_MOVE (that.value));
        break;

      case 85: // FuncCall
        value.copy< unique_ptr<FuncCallAST> > (YY_MOVE (that.value));
        break;

      case 88: // FuncDef
        value.copy< unique_ptr<FuncDefAST> > (YY_MOVE (that.value));
        break;

      case 91: // FuncFParam
      case 92: // FuncFParamSingle
      case 93: // FuncFParamArray
        value.copy< unique_ptr<FuncFParamAST> > (YY_MOVE (that.value));
        break;

      case 98: // IfStmt
        value.copy< unique_ptr<IfStmtAST> > (YY_MOVE (that.value));
        break;

      case 74: // InitVal
      case 75: // InitValArray
      case 76: // InitValArrayBody
        value.copy< unique_ptr<InitValAST> > (YY_MOVE (that.value));
        break;

      case 87: // LVal
        value.copy< unique_ptr<LValAST> > (YY_MOVE (that.value));
        break;

      case 99: // ReturnStmt
        value.copy< unique_ptr<ReturnStmtAST> > (YY_MOVE (that.value));
        break;

      case 97: // Stmt
        value.copy< unique_ptr<StmtAST> > (YY_MOVE (that.value));
        break;

      case 100: // WhileStmt
        value.copy< unique_ptr<WhileStmtAST> > (YY_MOVE (that.value));
        break;

      case 96: // BlockItem
        value.copy< vector<unique_ptr<AST>> > (YY_MOVE (that.value));
        break;

      case 63: // Decl
      case 65: // ConstDecl
      case 66: // VarDecl
        value.copy< vector<unique_ptr<DeclAST>> > (YY_MOVE (that.value));
        break;

      case 90: // FuncRParams
        value.copy< vector<unique_ptr<ExprAST>> > (YY_MOVE (that.value));
        break;

      case 89: // FuncFParams
        value.copy< vector<unique_ptr<FuncFParamAST>> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

  }



  template <typename Base>
  bool
  parser::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return Base::type_get () == empty_symbol;
  }

  template <typename Base>
  void
  parser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    switch (this->type_get ())
    {
      case 105: // AddOp
      case 106: // MulOp
      case 107: // UnaryOp
      case 108: // RelOp
        value.move< Op > (YY_MOVE (s.value));
        break;

      case 64: // BType
        value.move< VarType > (YY_MOVE (s.value));
        break;

      case 5: // "float const"
        value.move< float > (YY_MOVE (s.value));
        break;

      case 4: // "int const"
        value.move< int > (YY_MOVE (s.value));
        break;

      case 6: // "identifier"
      case 109: // IDENT
        value.move< string > (YY_MOVE (s.value));
        break;

      case 94: // Block
      case 95: // BlockItems
        value.move< unique_ptr<BlockAST> > (YY_MOVE (s.value));
        break;

      case 101: // BreakStmt
        value.move< unique_ptr<BreakStmtAST> > (YY_MOVE (s.value));
        break;

      case 103: // Cond
        value.move< unique_ptr<CondAST> > (YY_MOVE (s.value));
        break;

      case 102: // ContinueStmt
        value.move< unique_ptr<ContinueStmtAST> > (YY_MOVE (s.value));
        break;

      case 67: // VarDef
      case 68: // VarDefSingle
      case 69: // VarDefArray
      case 70: // DefArrayBody
      case 71: // ConstDef
      case 72: // ConstDefSingle
      case 73: // ConstDefArray
        value.move< unique_ptr<DeclAST> > (YY_MOVE (s.value));
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
        value.move< unique_ptr<ExprAST> > (YY_MOVE (s.value));
        break;

      case 85: // FuncCall
        value.move< unique_ptr<FuncCallAST> > (YY_MOVE (s.value));
        break;

      case 88: // FuncDef
        value.move< unique_ptr<FuncDefAST> > (YY_MOVE (s.value));
        break;

      case 91: // FuncFParam
      case 92: // FuncFParamSingle
      case 93: // FuncFParamArray
        value.move< unique_ptr<FuncFParamAST> > (YY_MOVE (s.value));
        break;

      case 98: // IfStmt
        value.move< unique_ptr<IfStmtAST> > (YY_MOVE (s.value));
        break;

      case 74: // InitVal
      case 75: // InitValArray
      case 76: // InitValArrayBody
        value.move< unique_ptr<InitValAST> > (YY_MOVE (s.value));
        break;

      case 87: // LVal
        value.move< unique_ptr<LValAST> > (YY_MOVE (s.value));
        break;

      case 99: // ReturnStmt
        value.move< unique_ptr<ReturnStmtAST> > (YY_MOVE (s.value));
        break;

      case 97: // Stmt
        value.move< unique_ptr<StmtAST> > (YY_MOVE (s.value));
        break;

      case 100: // WhileStmt
        value.move< unique_ptr<WhileStmtAST> > (YY_MOVE (s.value));
        break;

      case 96: // BlockItem
        value.move< vector<unique_ptr<AST>> > (YY_MOVE (s.value));
        break;

      case 63: // Decl
      case 65: // ConstDecl
      case 66: // VarDecl
        value.move< vector<unique_ptr<DeclAST>> > (YY_MOVE (s.value));
        break;

      case 90: // FuncRParams
        value.move< vector<unique_ptr<ExprAST>> > (YY_MOVE (s.value));
        break;

      case 89: // FuncFParams
        value.move< vector<unique_ptr<FuncFParamAST>> > (YY_MOVE (s.value));
        break;

      default:
        break;
    }

    location = YY_MOVE (s.location);
  }

  // by_type.
  inline
  parser::by_type::by_type ()
    : type (empty_symbol)
  {}

#if 201103L <= YY_CPLUSPLUS
  inline
  parser::by_type::by_type (by_type&& that)
    : type (that.type)
  {
    that.clear ();
  }
#endif

  inline
  parser::by_type::by_type (const by_type& that)
    : type (that.type)
  {}

  inline
  parser::by_type::by_type (token_type t)
    : type (yytranslate_ (t))
  {}

  inline
  void
  parser::by_type::clear ()
  {
    type = empty_symbol;
  }

  inline
  void
  parser::by_type::move (by_type& that)
  {
    type = that.type;
    that.clear ();
  }

  inline
  int
  parser::by_type::type_get () const YY_NOEXCEPT
  {
    return type;
  }

} // yy
#line 2614 "/root/bddd/build/parser.hh"





#endif // !YY_YY_ROOT_BDDD_BUILD_PARSER_HH_INCLUDED
