/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         ia_parse
#define yylex           ia_lex
#define yyerror         ia_error
#define yylval          ia_lval
#define yychar          ia_char
#define yydebug         ia_debug
#define yynerrs         ia_nerrs


/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 41 "./SPASS/iaparser.y"


#include "flags.h"
#include "ia.h"
#include "symbol.h"
#include "term.h"
#include "foldfg.h"
#include "clause.h"

extern NAT dfg_LINENUMBER;    /* Defined in dfgparser.y */
LIST       ia_PROOFREQUEST;   /* A pair! */
FLAGSTORE  ia_FLAGS;

void yyerror(const char*);
int  yylex(void);		/* Defined in iascanner.l */

static SYMBOL ia_Symbol(char*, NAT);
static TERM   ia_CreateQuantifier(SYMBOL, LIST, TERM);

static __inline__ void  ia_StringFree(char* String)
{
  memory_Free(String, sizeof(char)*(strlen(String)+1));
}

static __inline__ TERM ia_TermCreate(char* Name, LIST Arguments)
/* Look up the symbol, check its arity and create the term */
{
  return term_Create(ia_Symbol(Name,list_Length(Arguments)), Arguments);
}

/**************************************************************/
/* Functions to check the arity of symbols                    */
/**************************************************************/

static void ia_SymCheck(SYMBOL, NAT);

/**************************************************************/
/* Functions that handle variable names                       */
/**************************************************************/

/* List of quantified variables in the current input formula. */
/* This list is used to find symbols that by mistake weren't  */
/* declared in the symbol declaration section                 */
/* --> free variables                                         */
/* This is a list of lists, since each time a quantifier is   */
/* reached, a new list is added to the global list.           */
static LIST ia_VARLIST;
static BOOL ia_VARDECL;

static void   ia_VarStart(void);
static void   ia_VarStop(void);
static void   ia_VarBacktrack(void);
static void   ia_VarCheck(void);
static SYMBOL ia_VarLookup(char*);

#define YY_INPUT(buf,result,max_size) \
{ \
  int c = getc(ia_in); \
  result = (c == EOF) ? YY_NULL : (buf[0] = c, 1); \
}

#define YYERROR_VERBOSE



/* Line 268 of yacc.c  */
#line 145 "SPASS/iaparser.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IA_AND = 258,
     IA_EQUAL = 259,
     IA_EQUIV = 260,
     IA_EXISTS = 261,
     IA_FALSE = 262,
     IA_FORALL = 263,
     IA_IMPLIED = 264,
     IA_IMPLIES = 265,
     IA_NOT = 266,
     IA_OR = 267,
     IA_PROVE = 268,
     IA_TRUE = 269,
     IA_NUM = 270,
     IA_ID = 271
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 106 "./SPASS/iaparser.y"

  int       number;
  char*     string;
  SYMBOL    symbol;
  TERM      term;
  LIST      list;



/* Line 293 of yacc.c  */
#line 207 "SPASS/iaparser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 219 "SPASS/iaparser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   83

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  23
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  16
/* YYNRULES -- Number of rules.  */
#define YYNRULES  36
/* YYNRULES -- Number of states.  */
#define YYNSTATES  77

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   271

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      17,    19,     2,     2,    18,     2,    20,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    21,     2,    22,     2,     2,     2,     2,     2,     2,
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
      15,    16
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,    14,    16,    20,    22,    24,    26,
      31,    38,    43,    48,    49,    50,    61,    62,    63,    74,
      76,    78,    80,    82,    84,    86,    88,    90,    92,    94,
      96,   100,   102,   107,   110,   114,   116
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      24,     0,    -1,    -1,    13,    17,    26,    18,    37,    18,
      15,    19,    20,    -1,    26,    -1,    25,    18,    26,    -1,
      34,    -1,    14,    -1,     7,    -1,    11,    17,    26,    19,
      -1,    31,    17,    26,    18,    26,    19,    -1,    32,    17,
      25,    19,    -1,    34,    17,    25,    19,    -1,    -1,    -1,
      33,    17,    21,    27,    35,    28,    22,    18,    26,    19,
      -1,    -1,    -1,    34,    17,    21,    29,    35,    30,    22,
      18,    26,    19,    -1,     4,    -1,     5,    -1,     9,    -1,
      10,    -1,     3,    -1,    12,    -1,     6,    -1,     8,    -1,
      16,    -1,    15,    -1,    36,    -1,    35,    18,    36,    -1,
      34,    -1,    34,    17,    34,    19,    -1,    21,    22,    -1,
      21,    38,    22,    -1,    34,    -1,    38,    18,    34,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,   129,   129,   130,   142,   143,   146,   147,   148,   149,
     151,   153,   155,   157,   158,   157,   163,   164,   163,   172,
     173,   174,   175,   178,   179,   182,   183,   186,   187,   190,
     191,   194,   204,   225,   226,   229,   230
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IA_AND", "IA_EQUAL", "IA_EQUIV",
  "IA_EXISTS", "IA_FALSE", "IA_FORALL", "IA_IMPLIED", "IA_IMPLIES",
  "IA_NOT", "IA_OR", "IA_PROVE", "IA_TRUE", "IA_NUM", "IA_ID", "'('",
  "','", "')'", "'.'", "'['", "']'", "$accept", "proofrequest", "termlist",
  "term", "$@1", "$@2", "$@3", "$@4", "binsymbol", "nsymbol",
  "quantsymbol", "id", "qtermlist", "qterm", "labellistopt", "labellist", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,    40,    44,    41,
      46,    91,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    23,    24,    24,    25,    25,    26,    26,    26,    26,
      26,    26,    26,    27,    28,    26,    29,    30,    26,    31,
      31,    31,    31,    32,    32,    33,    33,    34,    34,    35,
      35,    36,    36,    37,    37,    38,    38
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     9,     1,     3,     1,     1,     1,     4,
       6,     4,     4,     0,     0,    10,     0,     0,    10,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     4,     2,     3,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     0,     0,     1,    23,    19,    20,    25,     8,
      26,    21,    22,     0,    24,     7,    28,    27,     0,     0,
       0,     0,     6,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     4,    13,    16,     0,     9,    33,
      35,     0,     0,     0,     0,    11,     0,     0,    12,     0,
      34,     0,     0,     5,    31,    14,    29,    17,    36,     0,
      10,     0,     0,     0,     0,     3,     0,    30,     0,     0,
      32,     0,     0,     0,     0,    15,    18
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     2,    33,    34,    46,    63,    47,    64,    19,    20,
      21,    22,    55,    56,    31,    41
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -29
static const yytype_int8 yypact[] =
{
     -12,    12,    35,     0,   -29,   -29,   -29,   -29,   -29,   -29,
     -29,   -29,   -29,    19,   -29,   -29,   -29,   -29,    20,    22,
      40,    41,    42,     0,    31,     0,     0,    43,    39,    18,
       8,    38,    44,     7,   -29,   -29,   -29,     9,   -29,   -29,
     -29,    -5,    46,     0,     0,   -29,    16,    16,   -29,    16,
     -29,    47,    48,   -29,    53,    45,   -29,    45,   -29,    51,
     -29,    16,    16,    50,    52,   -29,    54,   -29,    57,    58,
     -29,     0,     0,    59,    60,   -29,   -29
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -29,   -29,    37,    -3,   -29,   -29,   -29,   -29,   -29,   -29,
     -29,   -28,    30,    21,   -29,   -29
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      18,     1,    40,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    49,    15,    16,    17,    50,    54,    54,
      29,    58,    32,    16,    17,    44,    45,    44,    48,     3,
      39,    16,    17,    66,    54,     4,    23,    38,    24,    25,
      52,    53,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    30,    15,    16,    17,    42,    26,    27,    28,
      36,    51,    43,    62,    35,    37,    59,    60,    73,    74,
      61,    65,    68,    70,    69,    71,    72,    57,    75,    76,
       0,     0,     0,    67
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-29))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int8 yycheck[] =
{
       3,    13,    30,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    18,    14,    15,    16,    22,    46,    47,
      23,    49,    25,    15,    16,    18,    19,    18,    19,    17,
      22,    15,    16,    61,    62,     0,    17,    19,    18,    17,
      43,    44,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    21,    14,    15,    16,    18,    17,    17,    17,
      21,    15,    18,    18,    21,    28,    19,    19,    71,    72,
      17,    20,    22,    19,    22,    18,    18,    47,    19,    19,
      -1,    -1,    -1,    62
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    13,    24,    17,     0,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    14,    15,    16,    26,    31,
      32,    33,    34,    17,    18,    17,    17,    17,    17,    26,
      21,    37,    26,    25,    26,    21,    21,    25,    19,    22,
      34,    38,    18,    18,    18,    19,    27,    29,    19,    18,
      22,    15,    26,    26,    34,    35,    36,    35,    34,    19,
      19,    17,    18,    28,    30,    20,    34,    36,    22,    22,
      19,    18,    18,    26,    26,    19,    19
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
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


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
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
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

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

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1806 of yacc.c  */
#line 129 "./SPASS/iaparser.y"
    { YYABORT; }
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 130 "./SPASS/iaparser.y"
    {
					  ia_VarCheck();
					  ia_PROOFREQUEST = list_PairCreate((yyvsp[(3) - (9)].term),(yyvsp[(5) - (9)].list));
					  flag_SetFlagIntValue(ia_FLAGS,flag_TIMELIMIT,(yyvsp[(7) - (9)].number));
					  YYACCEPT;
                                        }
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 142 "./SPASS/iaparser.y"
    { (yyval.list) = list_List((yyvsp[(1) - (1)].term)); }
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 143 "./SPASS/iaparser.y"
    { (yyval.list) = list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(3) - (3)].term))); }
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 146 "./SPASS/iaparser.y"
    { (yyval.term) = ia_TermCreate((yyvsp[(1) - (1)].string), list_Nil()); }
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 147 "./SPASS/iaparser.y"
    { (yyval.term) = term_Create(fol_True(),list_Nil()); }
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 148 "./SPASS/iaparser.y"
    { (yyval.term) = term_Create(fol_False(),list_Nil()); }
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 150 "./SPASS/iaparser.y"
    { (yyval.term) = term_Create(fol_Not(),list_List((yyvsp[(3) - (4)].term))); }
    break;

  case 10:

/* Line 1806 of yacc.c  */
#line 152 "./SPASS/iaparser.y"
    { (yyval.term) = term_Create((yyvsp[(1) - (6)].symbol), list_Cons((yyvsp[(3) - (6)].term), list_List((yyvsp[(5) - (6)].term)))); }
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 154 "./SPASS/iaparser.y"
    { (yyval.term) = term_Create((yyvsp[(1) - (4)].symbol), (yyvsp[(3) - (4)].list)); }
    break;

  case 12:

/* Line 1806 of yacc.c  */
#line 156 "./SPASS/iaparser.y"
    { (yyval.term) = ia_TermCreate((yyvsp[(1) - (4)].string), (yyvsp[(3) - (4)].list)); }
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 157 "./SPASS/iaparser.y"
    { ia_VarStart(); }
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 158 "./SPASS/iaparser.y"
    { ia_VarStop(); }
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 160 "./SPASS/iaparser.y"
    { ia_VarBacktrack();
				  (yyval.term) = ia_CreateQuantifier((yyvsp[(1) - (10)].symbol),(yyvsp[(5) - (10)].list),(yyvsp[(9) - (10)].term));
				}
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 163 "./SPASS/iaparser.y"
    { ia_VarStart(); }
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 164 "./SPASS/iaparser.y"
    { ia_VarStop(); }
    break;

  case 18:

/* Line 1806 of yacc.c  */
#line 166 "./SPASS/iaparser.y"
    { misc_StartUserErrorReport();
				  misc_UserErrorReport("\n Line %d: SPASS can't handle the quantifier %s.\n", dfg_LINENUMBER, (yyvsp[(1) - (10)].string));
				  misc_FinishUserErrorReport();
				}
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 172 "./SPASS/iaparser.y"
    { (yyval.symbol) = fol_Equality(); }
    break;

  case 20:

/* Line 1806 of yacc.c  */
#line 173 "./SPASS/iaparser.y"
    { (yyval.symbol) = fol_Equiv();    }
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 174 "./SPASS/iaparser.y"
    { (yyval.symbol) = fol_Implied();  }
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 175 "./SPASS/iaparser.y"
    { (yyval.symbol) = fol_Implies();  }
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 178 "./SPASS/iaparser.y"
    { (yyval.symbol) = fol_And(); }
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 179 "./SPASS/iaparser.y"
    { (yyval.symbol) = fol_Or();  }
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 182 "./SPASS/iaparser.y"
    { (yyval.symbol) = fol_Exist(); }
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 183 "./SPASS/iaparser.y"
    { (yyval.symbol) = fol_All(); }
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 186 "./SPASS/iaparser.y"
    { (yyval.string) = (yyvsp[(1) - (1)].string); }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 187 "./SPASS/iaparser.y"
    { (yyval.string) = string_IntToString((yyvsp[(1) - (1)].number)); }
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 190 "./SPASS/iaparser.y"
    { (yyval.list) = list_List((yyvsp[(1) - (1)].term)); }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 191 "./SPASS/iaparser.y"
    { (yyval.list) = list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(3) - (3)].term))); }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 194 "./SPASS/iaparser.y"
    { SYMBOL s = ia_Symbol((yyvsp[(1) - (1)].string),0);
					  if (!symbol_IsVariable(s)) {
					    misc_StartUserErrorReport();
					    misc_UserErrorReport("\n Line %d: %s",dfg_LINENUMBER,
								 symbol_Name(s));
					    misc_UserErrorReport(" is not a variable.\n");
					    misc_FinishUserErrorReport();
					  }
					  (yyval.term) = term_Create(s, list_Nil());
					}
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 204 "./SPASS/iaparser.y"
    { SYMBOL p, v;
					  p = ia_Symbol((yyvsp[(1) - (4)].string), 1);
					  if (!symbol_IsPredicate(p)) {
					    misc_StartUserErrorReport();
					    misc_UserErrorReport("\n Line %d: %s",dfg_LINENUMBER,
								 symbol_Name(p));
					    misc_UserErrorReport(" is not a predicate.\n");
					    misc_FinishUserErrorReport();
					  }
					  v = ia_Symbol((yyvsp[(3) - (4)].string), 0);
					  if (!symbol_IsVariable(v)) {
					    misc_StartUserErrorReport();
					    misc_UserErrorReport("\n Line %d: %s",dfg_LINENUMBER,
								 symbol_Name(v));
					    misc_UserErrorReport(" is not a variable.\n");
					    misc_FinishUserErrorReport();
					  }
					  (yyval.term) = term_Create(p, list_List(term_Create(v,list_Nil())));
					}
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 225 "./SPASS/iaparser.y"
    { (yyval.list) = list_Nil(); }
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 226 "./SPASS/iaparser.y"
    { (yyval.list) = (yyvsp[(2) - (3)].list); }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 229 "./SPASS/iaparser.y"
    { (yyval.list) = list_List((yyvsp[(1) - (1)].string)); }
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 230 "./SPASS/iaparser.y"
    { (yyval.list) = list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(3) - (3)].string))); }
    break;



/* Line 1806 of yacc.c  */
#line 1780 "SPASS/iaparser.c"
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
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
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

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 2067 of yacc.c  */
#line 233 "./SPASS/iaparser.y"



void yyerror(const char *s)
{
  misc_StartUserErrorReport();
  misc_UserErrorReport("\n Line %i: %s\n", dfg_LINENUMBER, s);
  misc_FinishUserErrorReport();

}

LIST ia_GetNextRequest(FILE* Input, FLAGSTORE Flags)
/**************************************************************
  INPUT:   An input file containing one proof request from KIV.
  RETURNS: The proof request as pair (formula, labellist),
           list_Nil(), if EOF was reached.
  EFFECT:  Reads ONE proof request from the file.
           <Input> may also be a UNIX pipe.
***************************************************************/
{
  extern FILE* ia_in;  /* defined in kivscanner */

  ia_in           = Input;
  ia_PROOFREQUEST = list_Nil();
  ia_FLAGS        = Flags;
  ia_parse();
  
  return ia_PROOFREQUEST;
}


/**************************************************************/
/* Static Functions                                           */
/**************************************************************/

static SYMBOL ia_Symbol(char* Name, NAT Arity)
/**************************************************************
  INPUT:   The name of a symbol and the actual arity of the symbol.
  RETURNS: The corresponding SYMBOL.
  EFFECT:  This function checks if the <Name> was declared as
           symbol or variable. If not, an error message is printed
	   to stderr.
	   The <Name> is deleted.
***************************************************************/
{
  SYMBOL symbol;

  symbol = symbol_Lookup(Name);
  if (symbol != 0) {
    ia_StringFree(Name);
    ia_SymCheck(symbol, Arity); /* Check the arity */
  } else {
    /* Variable */
    if (Arity > 0) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Undefined symbol %s.\n",dfg_LINENUMBER,Name);
      misc_FinishUserErrorReport();
    }
    symbol = ia_VarLookup(Name);
  }
  return symbol;
}


static TERM ia_CreateQuantifier(SYMBOL Symbol, LIST VarTermList, TERM Term)
/**************************************************************
  INPUT:   A quantifier symbol, a list possibly containing sorts,
           and a term.
  RETURNS: The created quantifier term..
***************************************************************/
{
  LIST varlist, sortlist, scan;
  TERM helpterm;

  /* First collect the variable symbols in varlist and the sorts in sortlist */
  varlist = sortlist = list_Nil();
  for ( ; !list_Empty(VarTermList); VarTermList = list_Pop(VarTermList)) {
    helpterm = list_Car(VarTermList);
    if (term_IsVariable(helpterm)) {
      varlist = list_Nconc(varlist, list_List((POINTER)term_TopSymbol(helpterm)));
      term_Delete(helpterm);
    } else {
      SYMBOL var = term_TopSymbol(term_FirstArgument(helpterm));
      varlist  = list_Nconc(varlist, list_List((POINTER)var));
      sortlist = list_Nconc(sortlist, list_List(helpterm));
    }
  }

  varlist = list_PointerDeleteDuplicates(varlist);
  /* Now create terms from the variables */
  for (scan = varlist; !list_Empty(scan); scan = list_Cdr(scan))
    list_Rplaca(scan, term_Create((SYMBOL)list_Car(scan), list_Nil()));

  if (!list_Empty(sortlist)) {
    if (symbol_Equal(fol_All(), Symbol)) {
      /* The conjunction of all sortterms implies the Term */
      if (symbol_Equal(fol_Or(), term_TopSymbol(Term))) {
	/* Special treatment if <Term> is a term with "or" like */
	/* in clauses: add all sort terms negated to the args    */
	/* of the "or" */
	for (scan = sortlist; !list_Empty(scan); scan = list_Cdr(scan))
	  /* Negate the sort terms */
	  list_Rplaca(scan, term_Create(fol_Not(), list_List(list_Car(scan))));
	sortlist = list_Nconc(sortlist, term_ArgumentList(Term));
	term_RplacArgumentList(Term, sortlist);
      } else {
	/* No "or" term, so build the implication term */
	if (list_Empty(list_Cdr(sortlist))) {
	  /* Only one sort term */
	  list_Rplacd(sortlist, list_List(Term));
	  Term = term_Create(fol_Implies(), sortlist);
	} else {
	  /* More than one sort term */
	  helpterm = term_Create(fol_And(), sortlist);
	  Term = term_Create(fol_Implies(), list_Cons(helpterm, list_List(Term)));
	}
      }
    } else if (symbol_Equal(fol_Exist(), Symbol)) {
      /* Quantify the conjunction of all sort terms and <Term> */
      if (symbol_Equal(fol_And(), term_TopSymbol(Term))) {
	/* Special treatment if <Term> has an "and" as top symbol: */
	/* just add the sort terms to the args of the "and".       */
	sortlist = list_Nconc(sortlist, term_ArgumentList(Term));
	term_RplacArgumentList(Term, sortlist);
      } else {
	sortlist = list_Nconc(sortlist, list_List(Term));
	Term = term_Create(fol_And(), sortlist);
      }
    }
  }
  helpterm = fol_CreateQuantifier(Symbol, varlist, list_List(Term));
  return helpterm;
}


/**************************************************************/
/* Functions for the Symbol Table                             */
/**************************************************************/

static void ia_SymCheck(SYMBOL Symbol, NAT Arity)
/**************************************************************
  INPUT:   A symbol and the current arity of this symbol.
  RETURNS: Nothing.
  EFFECT:  This function compares the previous arity of 'Symbol'
           with the actual 'Arity'. If these values differ
	   a warning is printed to stderr and the program exits.
***************************************************************/
{
  /* Check if the specified arity corresponds with the actual arity */
  if (symbol_Arity(Symbol) != symbol_ArbitraryArity() &&
      symbol_Arity(Symbol) != Arity) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %u: Symbol %s", dfg_LINENUMBER, symbol_Name(Symbol));
    misc_UserErrorReport(" was declared with arity %u.\n", symbol_Arity(Symbol));
    misc_FinishUserErrorReport();
  }
}


/**************************************************************/
/* Functions for the Variable Table                           */
/**************************************************************/
  
typedef struct {
  char*  name;
  SYMBOL symbol;
} IA_VARENTRY, *IA_VAR;

static __inline__ char* ia_VarName(IA_VAR Entry)
{
  return Entry->name;
}

static __inline__ SYMBOL ia_VarSymbol(IA_VAR Entry)
{
  return Entry->symbol;
}

static __inline__ IA_VAR ia_VarCreate(void)
{
  return (IA_VAR) memory_Malloc(sizeof(IA_VARENTRY));
}

static void ia_VarFree(IA_VAR Entry)
{
  ia_StringFree(Entry->name);
  memory_Free(Entry, sizeof(IA_VARENTRY));
}

static void ia_VarStart(void)
{
  ia_VARLIST = list_Push(list_Nil(), ia_VARLIST);
  ia_VARDECL = TRUE;
}

static void ia_VarStop(void)
{
  ia_VARDECL = FALSE;
}

static void ia_VarBacktrack(void)
{
  list_DeleteWithElement(list_Top(ia_VARLIST), (void (*)(POINTER)) ia_VarFree);
  ia_VARLIST = list_Pop(ia_VARLIST);
}

static void ia_VarCheck(void)
/* Should be called after a complete clause or formula was parsed */
{
  if (!list_Empty(ia_VARLIST)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In ia_VarCheck: List of variables should be empty!\n");
    misc_FinishErrorReport();
  }
  symbol_ResetStandardVarCounter();
}

static SYMBOL ia_VarLookup(char* Name)
/**************************************************************
  INPUT:   A variable name.
  RETURNS: The corresponding variable symbol.
  EFFECT:  If the variable name was quantified before, the
           corresponding symbol is returned and the <Name> is freed.
	   If the variable name was not quantified, and <ia_VARDECL>
	   is TRUE, a new variable is created, else an error
	   message is printed and the program exits.
***************************************************************/
{
  LIST   scan, scan2;
  SYMBOL symbol;

  symbol = symbol_Null();

  scan  = ia_VARLIST;
  scan2 = list_Nil();
  while (!list_Empty(scan) && list_Empty(scan2)) {
    scan2 = list_Car(scan);
    while (!list_Empty(scan2) &&
	   !string_Equal(ia_VarName(list_Car(scan2)), Name))
      scan2 = list_Cdr(scan2);
    scan = list_Cdr(scan);
  }

  if (!list_Empty(scan2)) {
    /* Found variable */
    ia_StringFree(Name);
    symbol = ia_VarSymbol(list_Car(scan2));
  } else {
    /* Variable not found */
    if (ia_VARDECL) {
      IA_VAR newEntry = ia_VarCreate();
      newEntry->name   = Name;
      newEntry->symbol = symbol_CreateStandardVariable();
      /* Add <newentry> to the first list in ia_VARLIST */
      list_Rplaca(ia_VARLIST, list_Cons(newEntry,list_Car(ia_VARLIST)));
      symbol = ia_VarSymbol(newEntry);
    } else {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %u: Free Variable %s.\n", dfg_LINENUMBER, Name);
      misc_FinishUserErrorReport();
    }
  }
  return symbol;
}

