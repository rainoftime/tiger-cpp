/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 17 "tiger.y"

/**
 * @brief Includes and declarations for the parser
 */
#include <string>
#include <iostream>
#include <memory>
#include "tiger/absyn/absyn.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/symbol/symbol.h"

/**
 * @brief External lexer function declaration
 */
extern int yylex();

/**
 * @brief Error reporting function called by the parser on syntax errors
 * @param s Error message string
 */
void yyerror(const char *s);

/**
 * @brief Initialize the lexical analyzer with error handler
 * @param errormsg Error message handler for position tracking
 */
void InitLexer(err::ErrorMsg *errormsg);

/**
 * @brief Global AST tree being constructed by the parser
 */
static std::unique_ptr<absyn::AbsynTree> absyn_tree_;

/**
 * @brief Global error message handler for position tracking
 */
static err::ErrorMsg *errormsg_;

/**
 * @brief Get current token position for AST node construction
 * @return Current token position from error handler
 */
int GetTokPos() {
    return errormsg_->GetTokPos();
}


#line 119 "parse.tab.cc"

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

#include "parse.tab.hh"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ID = 3,                         /* ID  */
  YYSYMBOL_STRING = 4,                     /* STRING  */
  YYSYMBOL_INT = 5,                        /* INT  */
  YYSYMBOL_COMMA = 6,                      /* COMMA  */
  YYSYMBOL_COLON = 7,                      /* COLON  */
  YYSYMBOL_SEMICOLON = 8,                  /* SEMICOLON  */
  YYSYMBOL_LPAREN = 9,                     /* LPAREN  */
  YYSYMBOL_RPAREN = 10,                    /* RPAREN  */
  YYSYMBOL_LBRACK = 11,                    /* LBRACK  */
  YYSYMBOL_RBRACK = 12,                    /* RBRACK  */
  YYSYMBOL_LBRACE = 13,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 14,                    /* RBRACE  */
  YYSYMBOL_DOT = 15,                       /* DOT  */
  YYSYMBOL_ARRAY = 16,                     /* ARRAY  */
  YYSYMBOL_IF = 17,                        /* IF  */
  YYSYMBOL_THEN = 18,                      /* THEN  */
  YYSYMBOL_ELSE = 19,                      /* ELSE  */
  YYSYMBOL_WHILE = 20,                     /* WHILE  */
  YYSYMBOL_FOR = 21,                       /* FOR  */
  YYSYMBOL_TO = 22,                        /* TO  */
  YYSYMBOL_DO = 23,                        /* DO  */
  YYSYMBOL_LET = 24,                       /* LET  */
  YYSYMBOL_IN = 25,                        /* IN  */
  YYSYMBOL_END = 26,                       /* END  */
  YYSYMBOL_OF = 27,                        /* OF  */
  YYSYMBOL_BREAK = 28,                     /* BREAK  */
  YYSYMBOL_NIL = 29,                       /* NIL  */
  YYSYMBOL_FUNCTION = 30,                  /* FUNCTION  */
  YYSYMBOL_VAR = 31,                       /* VAR  */
  YYSYMBOL_TYPE = 32,                      /* TYPE  */
  YYSYMBOL_ASSIGN = 33,                    /* ASSIGN  */
  YYSYMBOL_OR = 34,                        /* OR  */
  YYSYMBOL_AND = 35,                       /* AND  */
  YYSYMBOL_EQ = 36,                        /* EQ  */
  YYSYMBOL_NEQ = 37,                       /* NEQ  */
  YYSYMBOL_LT = 38,                        /* LT  */
  YYSYMBOL_LE = 39,                        /* LE  */
  YYSYMBOL_GT = 40,                        /* GT  */
  YYSYMBOL_GE = 41,                        /* GE  */
  YYSYMBOL_PLUS = 42,                      /* PLUS  */
  YYSYMBOL_MINUS = 43,                     /* MINUS  */
  YYSYMBOL_TIMES = 44,                     /* TIMES  */
  YYSYMBOL_DIVIDE = 45,                    /* DIVIDE  */
  YYSYMBOL_YYACCEPT = 46,                  /* $accept  */
  YYSYMBOL_program = 47,                   /* program  */
  YYSYMBOL_exp = 48,                       /* exp  */
  YYSYMBOL_expop = 49,                     /* expop  */
  YYSYMBOL_expseq = 50,                    /* expseq  */
  YYSYMBOL_sequencing_exps = 51,           /* sequencing_exps  */
  YYSYMBOL_actuals = 52,                   /* actuals  */
  YYSYMBOL_nonemptyactuals = 53,           /* nonemptyactuals  */
  YYSYMBOL_lvalue = 54,                    /* lvalue  */
  YYSYMBOL_oneormore = 55,                 /* oneormore  */
  YYSYMBOL_one = 56,                       /* one  */
  YYSYMBOL_tydec = 57,                     /* tydec  */
  YYSYMBOL_tydec_one = 58,                 /* tydec_one  */
  YYSYMBOL_ty = 59,                        /* ty  */
  YYSYMBOL_tyfields = 60,                  /* tyfields  */
  YYSYMBOL_tyfields_nonempty = 61,         /* tyfields_nonempty  */
  YYSYMBOL_tyfield = 62,                   /* tyfield  */
  YYSYMBOL_fundec = 63,                    /* fundec  */
  YYSYMBOL_fundec_one = 64,                /* fundec_one  */
  YYSYMBOL_rec = 65,                       /* rec  */
  YYSYMBOL_rec_nonempty = 66,              /* rec_nonempty  */
  YYSYMBOL_rec_one = 67,                   /* rec_one  */
  YYSYMBOL_vardec = 68,                    /* vardec  */
  YYSYMBOL_decs = 69,                      /* decs  */
  YYSYMBOL_decs_nonempty = 70,             /* decs_nonempty  */
  YYSYMBOL_decs_nonempty_s = 71            /* decs_nonempty_s  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
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
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
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


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  40
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   330

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  46
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  26
/* YYNRULES -- Number of rules.  */
#define YYNRULES  78
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  151

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   300


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   173,   173,   194,   195,   196,   197,   198,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   235,   236,   237,   243,   244,   249,
     250,   254,   255,   269,   271,   276,   277,   279,   283,   285,
     302,   303,   307,   311,   312,   313,   317,   318,   322,   323,
     327,   342,   343,   347,   348,   362,   363,   367,   368,   372,
     386,   387,   401,   402,   406,   407,   411,   412,   413
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "ID", "STRING", "INT",
  "COMMA", "COLON", "SEMICOLON", "LPAREN", "RPAREN", "LBRACK", "RBRACK",
  "LBRACE", "RBRACE", "DOT", "ARRAY", "IF", "THEN", "ELSE", "WHILE", "FOR",
  "TO", "DO", "LET", "IN", "END", "OF", "BREAK", "NIL", "FUNCTION", "VAR",
  "TYPE", "ASSIGN", "OR", "AND", "EQ", "NEQ", "LT", "LE", "GT", "GE",
  "PLUS", "MINUS", "TIMES", "DIVIDE", "$accept", "program", "exp", "expop",
  "expseq", "sequencing_exps", "actuals", "nonemptyactuals", "lvalue",
  "oneormore", "one", "tydec", "tydec_one", "ty", "tyfields",
  "tyfields_nonempty", "tyfield", "fundec", "fundec_one", "rec",
  "rec_nonempty", "rec_one", "vardec", "decs", "decs_nonempty",
  "decs_nonempty_s", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-63)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      81,     7,   -63,   -63,    54,    81,    81,    14,    -7,   -63,
     -63,    81,    10,   188,   -63,   -12,    -2,   -63,    81,    81,
      24,    25,   -63,   117,    46,   203,   262,    27,    58,    63,
      64,   -63,    40,   -63,    43,   -63,    52,   -63,    -7,   -37,
     -63,    81,    81,    81,    81,    81,    81,    81,    81,    81,
      81,    81,    81,    81,    81,    76,    -5,    70,   -63,   163,
      45,    73,   -63,    83,   -63,    81,   -63,   -63,    81,    81,
      81,    82,    55,    56,   -63,   -63,    81,   -63,   141,   226,
     250,   250,   250,   250,   250,   250,   -37,   -37,   -63,   -63,
     188,   175,   -63,    81,   -63,    66,    81,   -63,    24,   129,
     -63,   215,   188,   239,    91,    92,    81,    -1,   129,    74,
     -63,   -63,   -63,    81,   188,   -63,    81,    81,    96,    89,
     -63,    98,    75,   188,   -63,    91,    80,   -63,   -63,   188,
     188,   285,   108,    19,    91,    81,   100,   109,    81,   -63,
     112,    81,   -63,   188,   -63,   -63,   188,    84,   188,    81,
     188
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    43,     4,     3,     0,     0,     0,     0,    73,    16,
       5,     0,     0,     2,     8,     6,    44,    47,    40,     0,
      66,     0,    19,     0,     0,     0,     0,     0,     0,     0,
       0,    76,    51,    78,    62,    77,     0,    72,    75,    33,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    42,     0,    39,     0,
       0,     0,    65,    68,    49,     0,    20,    10,     0,     0,
       0,     0,     0,     0,    50,    61,    36,    74,    32,    31,
      25,    26,    27,    28,    29,    30,    21,    22,    23,    24,
      11,     0,    46,     0,     7,    48,     0,     9,     0,    37,
      38,    12,    14,     0,    57,     0,     0,     0,    35,     0,
      34,    45,    41,     0,    69,    67,     0,     0,     0,     0,
      56,    59,     0,    70,    53,    57,     0,    52,    17,    18,
      13,     0,     0,     0,     0,     0,     0,     0,     0,    60,
       0,     0,    58,    71,    54,    55,    15,     0,    63,     0,
      64
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -63,   -63,     0,   -63,   -63,   -62,   -63,    26,   -63,   -63,
     -63,    86,   -63,   -63,    -4,   -11,   -63,    88,   -63,   -63,
      28,   -63,   -63,   -63,    90,   -63
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    12,    56,    14,   109,    24,    57,    58,    15,    16,
      17,    31,    32,   127,   119,   120,   121,    33,    34,    61,
      62,    63,    35,    36,    37,    38
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      13,    93,   124,   100,    23,    25,    26,    51,    52,    54,
      40,    39,   125,    55,   110,   126,    18,    27,    19,    59,
      20,    53,    21,    28,    29,    30,   140,    60,    64,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,   141,    67,     1,     2,     3,
      70,    71,   105,     4,    22,    99,    72,    73,   101,   102,
     103,     5,    30,    28,     6,     7,   108,    76,     8,    92,
      94,    96,     9,    10,     1,     2,     3,    97,   106,    98,
       4,   104,   107,   113,   118,   122,   114,    11,     5,   133,
     128,     6,     7,   132,   134,     8,   123,   137,   135,     9,
      10,   139,   145,   129,   144,   147,   130,   131,    74,   112,
     149,   136,    75,   142,    11,    65,   115,    66,    77,     0,
       0,     0,     0,     0,     0,   143,     0,    65,   146,     0,
       0,   148,     0,     0,     0,     0,     0,     0,     0,   150,
       0,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    95,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,   111,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    68,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,   116,     0,     0,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,   117,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,     0,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    69,    -1,    -1,    -1,    -1,
      -1,    -1,    49,    50,    51,    52,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,   138,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52
};

static const yytype_int16 yycheck[] =
{
       0,     6,     3,    65,     4,     5,     6,    44,    45,    11,
       0,    11,    13,    15,    76,    16,     9,     3,    11,    19,
      13,    33,    15,    30,    31,    32,     7,     3,     3,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    36,    10,     3,     4,     5,
      33,     3,     7,     9,    10,    65,     3,     3,    68,    69,
      70,    17,    32,    30,    20,    21,    76,    25,    24,     3,
      10,    36,    28,    29,     3,     4,     5,    14,    33,     6,
       9,     9,    36,    27,     3,     3,    96,    43,    17,    10,
      26,    20,    21,     7,     6,    24,   106,    27,    33,    28,
      29,     3,     3,   113,    14,     3,   116,   117,    32,    93,
      36,   125,    34,   134,    43,     8,    98,    10,    38,    -1,
      -1,    -1,    -1,    -1,    -1,   135,    -1,     8,   138,    -1,
      -1,   141,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   149,
      -1,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    12,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    12,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    18,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    19,    -1,    -1,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    22,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    -1,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    23,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     9,    17,    20,    21,    24,    28,
      29,    43,    47,    48,    49,    54,    55,    56,     9,    11,
      13,    15,    10,    48,    51,    48,    48,     3,    30,    31,
      32,    57,    58,    63,    64,    68,    69,    70,    71,    48,
       0,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    33,    11,    15,    48,    52,    53,    48,
       3,    65,    66,    67,     3,     8,    10,    10,    18,    23,
      33,     3,     3,     3,    57,    63,    25,    70,    48,    48,
      48,    48,    48,    48,    48,    48,    48,    48,    48,    48,
      48,    48,     3,     6,    10,    12,    36,    14,     6,    48,
      51,    48,    48,    48,     9,     7,    33,    36,    48,    50,
      51,    12,    53,    27,    48,    66,    19,    22,     3,    60,
      61,    62,     3,    48,     3,    13,    16,    59,    26,    48,
      48,    48,     7,    10,     6,    33,    60,    27,    23,     3,
       7,    36,    61,    48,    14,     3,    48,     3,    48,    36,
      48
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    46,    47,    48,    48,    48,    48,    48,    48,    48,
      48,    48,    48,    48,    48,    48,    48,    48,    48,    48,
      48,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    49,    49,    49,    50,    50,    50,    51,    51,    52,
      52,    53,    53,    54,    54,    55,    55,    55,    56,    56,
      57,    57,    58,    59,    59,    59,    60,    60,    61,    61,
      62,    63,    63,    64,    64,    65,    65,    66,    66,    67,
      68,    68,    69,    69,    70,    70,    71,    71,    71
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     4,     1,     4,
       3,     3,     4,     6,     4,     8,     1,     5,     6,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     1,     1,     0,     3,     3,     1,
       0,     3,     1,     1,     1,     4,     3,     1,     4,     3,
       2,     1,     4,     1,     3,     3,     1,     0,     3,     1,
       3,     2,     1,     7,     9,     1,     0,     3,     1,     3,
       4,     6,     1,     0,     2,     1,     1,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: exp  */
#line 173 "tiger.y"
               { absyn_tree_ = std::make_unique<absyn::AbsynTree>((yyvsp[0].exp)); }
#line 1313 "parse.tab.cc"
    break;

  case 3: /* exp: INT  */
#line 194 "tiger.y"
        { (yyval.exp) = new absyn::IntExp(GetTokPos(), (yyvsp[0].ival)); }
#line 1319 "parse.tab.cc"
    break;

  case 4: /* exp: STRING  */
#line 195 "tiger.y"
           { (yyval.exp) = new absyn::StringExp(GetTokPos(), (yyvsp[0].sval)); }
#line 1325 "parse.tab.cc"
    break;

  case 5: /* exp: NIL  */
#line 196 "tiger.y"
        { (yyval.exp) = new absyn::NilExp(GetTokPos()); }
#line 1331 "parse.tab.cc"
    break;

  case 6: /* exp: lvalue  */
#line 197 "tiger.y"
           { (yyval.exp) = new absyn::VarExp(GetTokPos(), (yyvsp[0].var)); }
#line 1337 "parse.tab.cc"
    break;

  case 7: /* exp: ID LPAREN actuals RPAREN  */
#line 198 "tiger.y"
                             {
     (yyval.exp) = new absyn::CallExp(GetTokPos(), (yyvsp[-3].sym), (yyvsp[-1].explist)); }
#line 1344 "parse.tab.cc"
    break;

  case 8: /* exp: expop  */
#line 200 "tiger.y"
          { (yyval.exp) = (yyvsp[0].exp); }
#line 1350 "parse.tab.cc"
    break;

  case 9: /* exp: ID LBRACE rec RBRACE  */
#line 201 "tiger.y"
                         { (yyval.exp) = new absyn::RecordExp(GetTokPos(), (yyvsp[-3].sym), (yyvsp[-1].efieldlist)); }
#line 1356 "parse.tab.cc"
    break;

  case 10: /* exp: LPAREN sequencing_exps RPAREN  */
#line 202 "tiger.y"
                                  { (yyval.exp) = new absyn::SeqExp(GetTokPos(), (yyvsp[-1].explist)); }
#line 1362 "parse.tab.cc"
    break;

  case 11: /* exp: lvalue ASSIGN exp  */
#line 203 "tiger.y"
                      { (yyval.exp) = new absyn::AssignExp(GetTokPos(), (yyvsp[-2].var), (yyvsp[0].exp)); }
#line 1368 "parse.tab.cc"
    break;

  case 12: /* exp: IF exp THEN exp  */
#line 204 "tiger.y"
                    { (yyval.exp) = new absyn::IfExp(GetTokPos(), (yyvsp[-2].exp), (yyvsp[0].exp), NULL); }
#line 1374 "parse.tab.cc"
    break;

  case 13: /* exp: IF exp THEN exp ELSE exp  */
#line 205 "tiger.y"
                             { (yyval.exp) = new absyn::IfExp(GetTokPos(), (yyvsp[-4].exp), (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1380 "parse.tab.cc"
    break;

  case 14: /* exp: WHILE exp DO exp  */
#line 206 "tiger.y"
                     { (yyval.exp) = new absyn::WhileExp(GetTokPos(), (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1386 "parse.tab.cc"
    break;

  case 15: /* exp: FOR ID ASSIGN exp TO exp DO exp  */
#line 207 "tiger.y"
                                    { (yyval.exp) = new absyn::ForExp(GetTokPos(), (yyvsp[-6].sym), (yyvsp[-4].exp), (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1392 "parse.tab.cc"
    break;

  case 16: /* exp: BREAK  */
#line 208 "tiger.y"
          { (yyval.exp) = new absyn::BreakExp(GetTokPos()); }
#line 1398 "parse.tab.cc"
    break;

  case 17: /* exp: LET decs IN expseq END  */
#line 209 "tiger.y"
                           { (yyval.exp) = new absyn::LetExp(GetTokPos(), (yyvsp[-3].declist), (yyvsp[-1].exp)); }
#line 1404 "parse.tab.cc"
    break;

  case 18: /* exp: ID LBRACK exp RBRACK OF exp  */
#line 210 "tiger.y"
                                { (yyval.exp) = new absyn::ArrayExp(GetTokPos(), (yyvsp[-5].sym), (yyvsp[-3].exp), (yyvsp[0].exp)); }
#line 1410 "parse.tab.cc"
    break;

  case 19: /* exp: LPAREN RPAREN  */
#line 211 "tiger.y"
                  { (yyval.exp) = new absyn::VoidExp(GetTokPos()); }
#line 1416 "parse.tab.cc"
    break;

  case 20: /* exp: LPAREN exp RPAREN  */
#line 212 "tiger.y"
                      { (yyval.exp) = (yyvsp[-1].exp); }
#line 1422 "parse.tab.cc"
    break;

  case 21: /* expop: exp PLUS exp  */
#line 216 "tiger.y"
                 { (yyval.exp) = new absyn::OpExp(GetTokPos(), absyn::PLUS_OP, (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1428 "parse.tab.cc"
    break;

  case 22: /* expop: exp MINUS exp  */
#line 217 "tiger.y"
                  { (yyval.exp) = new absyn::OpExp(GetTokPos(), absyn::MINUS_OP, (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1434 "parse.tab.cc"
    break;

  case 23: /* expop: exp TIMES exp  */
#line 218 "tiger.y"
                  { (yyval.exp) = new absyn::OpExp(GetTokPos(), absyn::TIMES_OP, (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1440 "parse.tab.cc"
    break;

  case 24: /* expop: exp DIVIDE exp  */
#line 219 "tiger.y"
                   { (yyval.exp) = new absyn::OpExp(GetTokPos(), absyn::DIVIDE_OP, (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1446 "parse.tab.cc"
    break;

  case 25: /* expop: exp EQ exp  */
#line 220 "tiger.y"
               { (yyval.exp) = new absyn::OpExp(GetTokPos(), absyn::EQ_OP, (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1452 "parse.tab.cc"
    break;

  case 26: /* expop: exp NEQ exp  */
#line 221 "tiger.y"
                { (yyval.exp) = new absyn::OpExp(GetTokPos(), absyn::NEQ_OP, (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1458 "parse.tab.cc"
    break;

  case 27: /* expop: exp LT exp  */
#line 222 "tiger.y"
               { (yyval.exp) = new absyn::OpExp(GetTokPos(), absyn::LT_OP, (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1464 "parse.tab.cc"
    break;

  case 28: /* expop: exp LE exp  */
#line 223 "tiger.y"
               { (yyval.exp) = new absyn::OpExp(GetTokPos(), absyn::LE_OP, (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1470 "parse.tab.cc"
    break;

  case 29: /* expop: exp GT exp  */
#line 224 "tiger.y"
               { (yyval.exp) = new absyn::OpExp(GetTokPos(), absyn::GT_OP, (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1476 "parse.tab.cc"
    break;

  case 30: /* expop: exp GE exp  */
#line 225 "tiger.y"
               { (yyval.exp) = new absyn::OpExp(GetTokPos(), absyn::GE_OP, (yyvsp[-2].exp), (yyvsp[0].exp)); }
#line 1482 "parse.tab.cc"
    break;

  case 31: /* expop: exp AND exp  */
#line 226 "tiger.y"
                { (yyval.exp) = new absyn::IfExp(GetTokPos(), (yyvsp[-2].exp), (yyvsp[0].exp), new absyn::IntExp(GetTokPos(), 0)); }
#line 1488 "parse.tab.cc"
    break;

  case 32: /* expop: exp OR exp  */
#line 227 "tiger.y"
               { (yyval.exp) = new absyn::IfExp(GetTokPos(), (yyvsp[-2].exp), new absyn::IntExp(GetTokPos(), 1), (yyvsp[0].exp)); }
#line 1494 "parse.tab.cc"
    break;

  case 33: /* expop: MINUS exp  */
#line 228 "tiger.y"
              {
     (yyval.exp) = new absyn::OpExp(GetTokPos(), absyn::MINUS_OP, new absyn::IntExp(GetTokPos(), 0), (yyvsp[0].exp)); }
#line 1501 "parse.tab.cc"
    break;

  case 34: /* expseq: sequencing_exps  */
#line 235 "tiger.y"
                    { (yyval.exp) = new absyn::SeqExp(GetTokPos(), (yyvsp[0].explist)); }
#line 1507 "parse.tab.cc"
    break;

  case 35: /* expseq: exp  */
#line 236 "tiger.y"
        { (yyval.exp) = new absyn::SeqExp(GetTokPos(), new absyn::ExpList((yyvsp[0].exp))); }
#line 1513 "parse.tab.cc"
    break;

  case 36: /* expseq: %empty  */
#line 237 "tiger.y"
   { (yyval.exp) = new absyn::VoidExp(GetTokPos()); }
#line 1519 "parse.tab.cc"
    break;

  case 37: /* sequencing_exps: exp SEMICOLON exp  */
#line 243 "tiger.y"
                      { (yyval.explist) = new absyn::ExpList((yyvsp[0].exp)); (yyval.explist)->Prepend((yyvsp[-2].exp)); }
#line 1525 "parse.tab.cc"
    break;

  case 38: /* sequencing_exps: exp SEMICOLON sequencing_exps  */
#line 244 "tiger.y"
                                  { (yyval.explist) = (yyvsp[0].explist)->Prepend((yyvsp[-2].exp)); }
#line 1531 "parse.tab.cc"
    break;

  case 39: /* actuals: nonemptyactuals  */
#line 249 "tiger.y"
                    { (yyval.explist) = (yyvsp[0].explist); }
#line 1537 "parse.tab.cc"
    break;

  case 40: /* actuals: %empty  */
#line 250 "tiger.y"
   { (yyval.explist) = new absyn::ExpList(); }
#line 1543 "parse.tab.cc"
    break;

  case 41: /* nonemptyactuals: exp COMMA nonemptyactuals  */
#line 254 "tiger.y"
                              { (yyval.explist) = (yyvsp[0].explist)->Prepend((yyvsp[-2].exp)); }
#line 1549 "parse.tab.cc"
    break;

  case 42: /* nonemptyactuals: exp  */
#line 255 "tiger.y"
        { (yyval.explist) = new absyn::ExpList((yyvsp[0].exp)); }
#line 1555 "parse.tab.cc"
    break;

  case 43: /* lvalue: ID  */
#line 269 "tiger.y"
       {
     (yyval.var) = new absyn::SimpleVar(GetTokPos(), (yyvsp[0].sym)); }
#line 1562 "parse.tab.cc"
    break;

  case 44: /* lvalue: oneormore  */
#line 271 "tiger.y"
              {
     (yyval.var) = (yyvsp[0].var); }
#line 1569 "parse.tab.cc"
    break;

  case 45: /* oneormore: oneormore LBRACK exp RBRACK  */
#line 276 "tiger.y"
                                { (yyval.var) = new absyn::SubscriptVar(GetTokPos(), (yyvsp[-3].var), (yyvsp[-1].exp)); }
#line 1575 "parse.tab.cc"
    break;

  case 46: /* oneormore: oneormore DOT ID  */
#line 277 "tiger.y"
                     {
     (yyval.var) = new absyn::FieldVar(GetTokPos(), (yyvsp[-2].var), (yyvsp[0].sym)); }
#line 1582 "parse.tab.cc"
    break;

  case 47: /* oneormore: one  */
#line 279 "tiger.y"
        { (yyval.var) = (yyvsp[0].var); }
#line 1588 "parse.tab.cc"
    break;

  case 48: /* one: ID LBRACK exp RBRACK  */
#line 283 "tiger.y"
                         {
     (yyval.var) = new absyn::SubscriptVar(GetTokPos(), new absyn::SimpleVar(GetTokPos(), (yyvsp[-3].sym)), (yyvsp[-1].exp)); }
#line 1595 "parse.tab.cc"
    break;

  case 49: /* one: ID DOT ID  */
#line 285 "tiger.y"
              {
     (yyval.var) = new absyn::FieldVar(GetTokPos(), new absyn::SimpleVar(GetTokPos(), (yyvsp[-2].sym)), (yyvsp[0].sym)); }
#line 1602 "parse.tab.cc"
    break;

  case 50: /* tydec: tydec_one tydec  */
#line 302 "tiger.y"
                    { (yyval.tydeclist) = (yyvsp[0].tydeclist)->Prepend((yyvsp[-1].tydec)); }
#line 1608 "parse.tab.cc"
    break;

  case 51: /* tydec: tydec_one  */
#line 303 "tiger.y"
              { (yyval.tydeclist) = new absyn::NameAndTyList((yyvsp[0].tydec)); }
#line 1614 "parse.tab.cc"
    break;

  case 52: /* tydec_one: TYPE ID EQ ty  */
#line 307 "tiger.y"
                  { (yyval.tydec) = new absyn::NameAndTy((yyvsp[-2].sym), (yyvsp[0].ty)); }
#line 1620 "parse.tab.cc"
    break;

  case 53: /* ty: ID  */
#line 311 "tiger.y"
       { (yyval.ty) = new absyn::NameTy(GetTokPos(), (yyvsp[0].sym)); }
#line 1626 "parse.tab.cc"
    break;

  case 54: /* ty: LBRACE tyfields RBRACE  */
#line 312 "tiger.y"
                           { (yyval.ty) = new absyn::RecordTy(GetTokPos(), (yyvsp[-1].fieldlist)); }
#line 1632 "parse.tab.cc"
    break;

  case 55: /* ty: ARRAY OF ID  */
#line 313 "tiger.y"
                { (yyval.ty) = new absyn::ArrayTy(GetTokPos(), (yyvsp[0].sym)); }
#line 1638 "parse.tab.cc"
    break;

  case 56: /* tyfields: tyfields_nonempty  */
#line 317 "tiger.y"
                      { (yyval.fieldlist) = (yyvsp[0].fieldlist); }
#line 1644 "parse.tab.cc"
    break;

  case 57: /* tyfields: %empty  */
#line 318 "tiger.y"
   { (yyval.fieldlist) = new absyn::FieldList(); }
#line 1650 "parse.tab.cc"
    break;

  case 58: /* tyfields_nonempty: tyfield COMMA tyfields_nonempty  */
#line 322 "tiger.y"
                                    { (yyval.fieldlist) = (yyvsp[0].fieldlist)->Prepend((yyvsp[-2].field)); }
#line 1656 "parse.tab.cc"
    break;

  case 59: /* tyfields_nonempty: tyfield  */
#line 323 "tiger.y"
            { (yyval.fieldlist) = new absyn::FieldList((yyvsp[0].field)); }
#line 1662 "parse.tab.cc"
    break;

  case 60: /* tyfield: ID COLON ID  */
#line 327 "tiger.y"
                { (yyval.field) = new absyn::Field(GetTokPos(), (yyvsp[-2].sym), (yyvsp[0].sym)); }
#line 1668 "parse.tab.cc"
    break;

  case 61: /* fundec: fundec_one fundec  */
#line 342 "tiger.y"
                      { (yyval.fundeclist) = (yyvsp[0].fundeclist)->Prepend((yyvsp[-1].fundec)); }
#line 1674 "parse.tab.cc"
    break;

  case 62: /* fundec: fundec_one  */
#line 343 "tiger.y"
               { (yyval.fundeclist) = new absyn::FunDecList((yyvsp[0].fundec)); }
#line 1680 "parse.tab.cc"
    break;

  case 63: /* fundec_one: FUNCTION ID LPAREN tyfields RPAREN EQ exp  */
#line 347 "tiger.y"
                                              { (yyval.fundec) = new absyn::FunDec(GetTokPos(), (yyvsp[-5].sym), (yyvsp[-3].fieldlist), NULL, (yyvsp[0].exp)); }
#line 1686 "parse.tab.cc"
    break;

  case 64: /* fundec_one: FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp  */
#line 348 "tiger.y"
                                                       { (yyval.fundec) = new absyn::FunDec(GetTokPos(), (yyvsp[-7].sym), (yyvsp[-5].fieldlist), (yyvsp[-2].sym), (yyvsp[0].exp)); }
#line 1692 "parse.tab.cc"
    break;

  case 65: /* rec: rec_nonempty  */
#line 362 "tiger.y"
                 { (yyval.efieldlist) = (yyvsp[0].efieldlist); }
#line 1698 "parse.tab.cc"
    break;

  case 66: /* rec: %empty  */
#line 363 "tiger.y"
   { (yyval.efieldlist) = new absyn::EFieldList(); }
#line 1704 "parse.tab.cc"
    break;

  case 67: /* rec_nonempty: rec_one COMMA rec_nonempty  */
#line 367 "tiger.y"
                               { (yyval.efieldlist) = (yyvsp[0].efieldlist)->Prepend((yyvsp[-2].efield)); }
#line 1710 "parse.tab.cc"
    break;

  case 68: /* rec_nonempty: rec_one  */
#line 368 "tiger.y"
            { (yyval.efieldlist) = new absyn::EFieldList((yyvsp[0].efield)); }
#line 1716 "parse.tab.cc"
    break;

  case 69: /* rec_one: ID EQ exp  */
#line 372 "tiger.y"
              { (yyval.efield) = new absyn::EField((yyvsp[-2].sym), (yyvsp[0].exp)); }
#line 1722 "parse.tab.cc"
    break;

  case 70: /* vardec: VAR ID ASSIGN exp  */
#line 386 "tiger.y"
                      { (yyval.dec) = new absyn::VarDec(GetTokPos(), (yyvsp[-2].sym), NULL, (yyvsp[0].exp)); }
#line 1728 "parse.tab.cc"
    break;

  case 71: /* vardec: VAR ID COLON ID ASSIGN exp  */
#line 387 "tiger.y"
                               { (yyval.dec) = new absyn::VarDec(GetTokPos(), (yyvsp[-4].sym), (yyvsp[-2].sym), (yyvsp[0].exp)); }
#line 1734 "parse.tab.cc"
    break;

  case 72: /* decs: decs_nonempty  */
#line 401 "tiger.y"
                  { (yyval.declist) = (yyvsp[0].declist); }
#line 1740 "parse.tab.cc"
    break;

  case 73: /* decs: %empty  */
#line 402 "tiger.y"
   { (yyval.declist) = new absyn::DecList(); }
#line 1746 "parse.tab.cc"
    break;

  case 74: /* decs_nonempty: decs_nonempty_s decs_nonempty  */
#line 406 "tiger.y"
                                  { (yyval.declist) = (yyvsp[0].declist)->Prepend((yyvsp[-1].dec)); }
#line 1752 "parse.tab.cc"
    break;

  case 75: /* decs_nonempty: decs_nonempty_s  */
#line 407 "tiger.y"
                    { (yyval.declist) = new absyn::DecList((yyvsp[0].dec)); }
#line 1758 "parse.tab.cc"
    break;

  case 76: /* decs_nonempty_s: tydec  */
#line 411 "tiger.y"
          { (yyval.dec) = new absyn::TypeDec(GetTokPos(), (yyvsp[0].tydeclist)); }
#line 1764 "parse.tab.cc"
    break;

  case 77: /* decs_nonempty_s: vardec  */
#line 412 "tiger.y"
           { (yyval.dec) = (yyvsp[0].dec); }
#line 1770 "parse.tab.cc"
    break;

  case 78: /* decs_nonempty_s: fundec  */
#line 413 "tiger.y"
           { (yyval.dec) = new absyn::FunctionDec(GetTokPos(), (yyvsp[0].fundeclist)); }
#line 1776 "parse.tab.cc"
    break;


#line 1780 "parse.tab.cc"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 416 "tiger.y"


/**
 * @brief Error reporting function called by the parser on syntax errors
 * @param s Error message string
 */
void yyerror(const char *s) {
    errormsg_->Error(GetTokPos(), "%s", s);
}

/**
 * @brief Main parsing function for Tiger source files
 *
 * This function orchestrates the complete parsing process:
 * 1. Initializes error handling and lexer state
 * 2. Opens the source file for lexing
 * 3. Invokes the parser (yyparse)
 * 4. Returns the constructed AST if parsing succeeds
 * 5. Handles file I/O and error conditions
 *
 * @param fname Path to the Tiger source file to parse
 * @return Unique pointer to the parsed AST, or nullptr if parsing failed
 */
std::unique_ptr<absyn::AbsynTree> Parse(const std::string &fname) {
    errormsg_ = new err::ErrorMsg(fname);
    InitLexer(errormsg_);
    
    // Open the input file for the lexer
    extern FILE *yyin;
    yyin = fopen(fname.c_str(), "r");
    if (!yyin) {
        errormsg_->Error(0, "Cannot open file %s", fname.c_str());
        return nullptr;
    }
    
    int result = yyparse();
    fclose(yyin);
    
    // If parsing failed or there were errors, don't return the tree
    if (result != 0 || errormsg_->AnyErrors()) {
        return nullptr;
    }
    
    return std::move(absyn_tree_);
}

err::ErrorMsg *GetErrorMsg() {
    return errormsg_;
}
