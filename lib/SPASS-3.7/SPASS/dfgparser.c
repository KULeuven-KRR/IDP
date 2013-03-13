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
#define yyparse         dfg_parse
#define yylex           dfg_lex
#define yyerror         dfg_error
#define yylval          dfg_lval
#define yychar          dfg_char
#define yydebug         dfg_debug
#define yynerrs         dfg_nerrs


/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 41 "./SPASS/dfgparser.y"


#include <ctype.h>
#include "dfg.h"
#include "symbol.h"
#include "term.h"
#include "foldfg.h"
#include "strings.h"
#include "eml.h"
	
  /* Used for delayed parsing of plain clauses */
  typedef struct {
    LIST constraint;
    LIST antecedent;
    LIST succedent;
    TERM selected;
  } DFG_PLAINCLAUSEHELP, *DFG_PLAINCLAUSE;

  static DFGDESCRIPTION      dfg_DESC;
  static LIST                dfg_AXIOMLIST;
  static LIST                dfg_CONJECLIST;
  static LIST                dfg_SORTDECLLIST;
  static LIST                dfg_INCLUDELIST;
  /* symbol precedence explicitly defined by user */
  static LIST                dfg_USERPRECEDENCE;
  static LIST                dfg_USERSELECTION;
  static LIST                dfg_CLAXRELATION;
  static LIST                dfg_CLAXAXIOMS;
  static LIST                dfg_AXCLAUSES;
  static LIST                dfg_CONCLAUSES;
  static LIST                dfg_PROOFLIST;     /* list_of_proofs */
  static LIST                dfg_TERMLIST;      /* list_of_terms  */
  static LIST                dfg_PLAINAXCLAUSES; /* list of DFG_PLAINCLAUSE */
  static LIST                dfg_PLAINCONCLAUSES; /* list of DFG_PLAINCLAUSE */
  static LIST                dfg_TEMPPLAINCLAUSES; /* temporal list of DFG_PlAINCLAUSE, until they are concatenated to dfg_PLAINAXCLAUSES or dfg_PLAINCONCLAUSES */
  static TERM                dfg_SELECTED_LITERAL; /* last selected literal */
  static BOOL                dfg_IGNORE;      /* tokens are ignored while TRUE */
  static BOOL		   dfg_IGNORESETTINGS; /* issue a warning for included file with SPASS settings */
  static FLAGSTORE           dfg_FLAGS;
  static PRECEDENCE          dfg_PRECEDENCE;

  /* used also in the scanner */
  NAT                        dfg_LINENUMBER;
  BOOL			   dfg_IGNORETEXT;

  void yyerror(const char*);
  int  yylex(void);		/* Defined in dfgscanner.l */

  static void   dfg_SymbolDecl(int, char*, int);
  static SYMBOL dfg_Symbol(char*, NAT);
  static void   dfg_SubSort(char*, char*);
  static void   dfg_SymbolGenerated(SYMBOL, BOOL, LIST);
  static void   dfg_TranslPairDecl(char*, char*);
  static TERM   dfg_CreateQuantifier(SYMBOL, LIST, TERM);

  static __inline__ TERM dfg_TermCreate(char* Name, LIST Arguments)
    /***************************************************************
  INPUT:   A string <Name> and a list of terms <Arguments>.
  RETURNS: Does a symbol lookup for <Name> and creates out of
           the found or otherwise created new symbol and <Arguments>
           a new term.
  CAUTION: Deletes the string <Name>.
    ***************************************************************/
    {
      SYMBOL s;
      NAT    arity;
      arity = list_Length(Arguments);
      s = dfg_Symbol(Name, arity); /* Frees the string */
      if (!symbol_IsVariable(s) && !symbol_IsFunction(s)) {
	misc_StartUserErrorReport();
	misc_UserErrorReport("\n Line %d: is not a function.\n", dfg_LINENUMBER);
	misc_FinishUserErrorReport();
      }
      return term_Create(s, Arguments);
    }

  static __inline__ TERM dfg_AtomCreate(char* Name, LIST Arguments)
    /* Look up the symbol, check its arity and create the atom term */
    {
      SYMBOL s;
      s = dfg_Symbol(Name, list_Length(Arguments)); /* Frees the string */
      if (symbol_IsVariable(s) || !symbol_IsPredicate(s)) {
	misc_StartUserErrorReport();
	misc_UserErrorReport("\n Line %d: Symbol is not a predicate.\n", dfg_LINENUMBER);
	misc_FinishUserErrorReport();
      }
      return term_Create(s, Arguments);
    }

  static __inline__ void dfg_DeleteStringList(LIST List)
    {
      list_DeleteWithElement(List, (void (*)(POINTER)) string_StringFree);
    }

  /**************************************************************/
  /* Functions that handle symbols with unspecified arity       */
  /**************************************************************/

  /* The symbol list holds all symbols whose arity wasn't       */
  /* specified in the symbol declaration section.               */
  /* If the arity of a symbol was not specified in this section */
  /* it is first set to 0. If the symbol occurs with always the */
  /* same arity 'A' the arity of this symbol is set to 'A'.     */
  static LIST   dfg_SYMBOLLIST;

  static void dfg_SymAdd(SYMBOL);
  static void dfg_SymCheck(SYMBOL, NAT);
  static void dfg_SymCleanUp(void);

  /**************************************************************/
  /* Functions that handle variable names                       */
  /**************************************************************/

  /* List of quantified variables in the current input formula. */
  /* This list is used to find symbols that by mistake weren't  */
  /* declared in the symbol declaration section                 */
  /* --> free variables                                         */
  /* This is a list of lists, since each time a quantifier is   */
  /* reached, a new list is added to the global list.           */
  static LIST dfg_VARLIST;
  static BOOL dfg_VARDECL;

  static void   dfg_VarStart(void);
  static void   dfg_VarStop(void);
  static void   dfg_VarBacktrack(void);
  static void   dfg_VarCheck(void);
  static SYMBOL dfg_VarLookup(char*);
  static DFG_PLAINCLAUSE dfg_PlainClauseCreate(void);
  static void dfg_PlainClauseFree(DFG_PLAINCLAUSE Clause);

  static char * dfg_StripQuotes(char * str); /* strip quotes from include filename */

  /*
    #define DEBUG_PARSER
  */
#ifdef DEBUG_PARSER
#define YYERROR_VERBOSE
#define YYDEBUG 1000

  dfg_debug = 1;
#endif
  

/* Line 268 of yacc.c  */
#line 223 "SPASS/dfgparser.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
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
     DFG_AND = 258,
     DFG_AUTHOR = 259,
     DFG_AXIOMS = 260,
     DFG_BEGPROB = 261,
     DFG_BY = 262,
     DFG_CLAUSE = 263,
     DFG_CLOSEBRACE = 264,
     DFG_CLSLIST = 265,
     DFG_CNF = 266,
     DFG_CONJECS = 267,
     DFG_DATE = 268,
     DFG_DECLLIST = 269,
     DFG_INCLUDELIST = 270,
     DFG_INCLUDE = 271,
     DFG_DESC = 272,
     DFG_DESCLIST = 273,
     DFG_DNF = 274,
     DFG_DOMPRED = 275,
     DFG_ENDLIST = 276,
     DFG_ENDPROB = 277,
     DFG_EQUAL = 278,
     DFG_EQUIV = 279,
     DFG_EXISTS = 280,
     DFG_FALSE = 281,
     DFG_FORMLIST = 282,
     DFG_FORMULA = 283,
     DFG_FORALL = 284,
     DFG_FREELY = 285,
     DFG_FUNC = 286,
     DFG_GENERATED = 287,
     DFG_GENSET = 288,
     DFG_HYPOTH = 289,
     DFG_IMPLIED = 290,
     DFG_IMPLIES = 291,
     DFG_LOGIC = 292,
     DFG_NAME = 293,
     DFG_NOT = 294,
     DFG_OPENBRACE = 295,
     DFG_XOR = 296,
     DFG_NOR = 297,
     DFG_NAND = 298,
     DFG_OPERAT = 299,
     DFG_OR = 300,
     DFG_PREC = 301,
     DFG_CLFORE = 302,
     DFG_PRED = 303,
     DFG_PRDICAT = 304,
     DFG_PRFLIST = 305,
     DFG_QUANTIF = 306,
     DFG_SATIS = 307,
     DFG_SELECT = 308,
     DFG_SETFLAG = 309,
     DFG_SETTINGS = 310,
     DFG_SYMLIST = 311,
     DFG_SORT = 312,
     DFG_SORTS = 313,
     DFG_STATUS = 314,
     DFG_STEP = 315,
     DFG_SUBSORT = 316,
     DFG_TERMLIST = 317,
     DFG_TRUE = 318,
     DFG_UNKNOWN = 319,
     DFG_UNSATIS = 320,
     DFG_VERSION = 321,
     DFG_END_OF_CONSTRAINT_LITERALS = 322,
     DFG_END_OF_ANTECEDENT_LITERALS = 323,
     DFG_LITERAL_IS_SELECTED = 324,
     DFG_EML = 325,
     DFG_PROP_FORMULA = 326,
     DFG_REL_FORMULA = 327,
     DFG_SPECIALFORMLIST = 328,
     DFG_TRANSLPAIRS = 329,
     DFG_BOX = 330,
     DFG_DIA = 331,
     DFG_TEST = 332,
     DFG_DOMAIN = 333,
     DFG_RANGE = 334,
     DFG_COMP = 335,
     DFG_SUM = 336,
     DFG_CONV = 337,
     DFG_IDENTITY = 338,
     DFG_DIVERSITY = 339,
     DFG_DOMRESTR = 340,
     DFG_RANRESTR = 341,
     DFG_NUM = 342,
     DFG_MINUS1 = 343,
     DFG_ID = 344,
     DFG_TEXT = 345,
     single_quoted = 346
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 184 "./SPASS/dfgparser.y"

  int       number;
  char*     string;
  SYMBOL    symbol;
  SPROPERTY property;
  TERM      term;
  LIST      list;
  DFG_STATE state;
  BOOL      bool;



/* Line 293 of yacc.c  */
#line 363 "SPASS/dfgparser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 375 "SPASS/dfgparser.c"

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
#define YYLAST   823

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  99
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  131
/* YYNRULES -- Number of rules.  */
#define YYNRULES  280
/* YYNRULES -- Number of states.  */
#define YYNSTATES  666

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   346

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      92,    93,     2,     2,    97,     2,    94,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    98,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    95,     2,    96,     2,     2,     2,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,    15,    27,    33,    39,    45,    51,    52,
      58,    59,    65,    66,    72,    74,    76,    78,    86,    87,
      98,    99,   105,   107,   111,   113,   119,   120,   126,   128,
     132,   134,   140,   141,   147,   149,   153,   154,   160,   162,
     166,   168,   174,   175,   181,   183,   187,   189,   195,   197,
     199,   200,   206,   208,   212,   213,   219,   220,   226,   227,
     230,   232,   240,   243,   251,   252,   253,   265,   275,   276,
     278,   280,   284,   286,   290,   299,   301,   303,   304,   307,
     308,   316,   317,   320,   322,   327,   334,   339,   340,   341,
     352,   353,   355,   357,   361,   363,   365,   367,   369,   371,
     373,   375,   377,   379,   381,   383,   385,   387,   389,   391,
     393,   395,   397,   401,   403,   408,   419,   420,   423,   424,
     432,   440,   448,   449,   451,   452,   454,   456,   460,   462,
     466,   468,   470,   472,   477,   482,   487,   494,   499,   506,
     508,   510,   512,   517,   519,   521,   526,   531,   538,   545,
     552,   557,   559,   561,   563,   565,   567,   569,   570,   573,
     584,   585,   597,   598,   606,   607,   609,   610,   611,   619,
     621,   622,   623,   634,   639,   640,   642,   646,   647,   650,
     651,   654,   658,   660,   665,   667,   671,   673,   675,   677,
     684,   689,   690,   698,   699,   701,   703,   712,   717,   719,
     724,   726,   730,   731,   734,   735,   745,   746,   762,   764,
     768,   769,   774,   778,   784,   785,   789,   791,   793,   795,
     797,   799,   801,   803,   805,   807,   808,   812,   820,   822,
     824,   825,   828,   829,   836,   837,   841,   842,   845,   851,
     852,   855,   862,   867,   868,   870,   874,   876,   877,   880,
     886,   887,   897,   899,   903,   904,   908,   913,   918,   923,
     928,   935,   937,   941,   943,   950,   952,   956,   962,   964,
     968,   970,   972,   976,   978,   979,   982,   984,   987,   993,
     995
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     100,     0,    -1,     6,    92,   152,    93,    94,   101,   110,
     205,   212,    22,    94,    -1,    18,    94,   102,   103,   106,
     107,   104,   105,   108,    21,    94,    -1,    38,    92,    90,
      93,    94,    -1,     4,    92,    90,    93,    94,    -1,    59,
      92,   109,    93,    94,    -1,    17,    92,    90,    93,    94,
      -1,    -1,    66,    92,    90,    93,    94,    -1,    -1,    37,
      92,    90,    93,    94,    -1,    -1,    13,    92,    90,    93,
      94,    -1,    52,    -1,    65,    -1,    64,    -1,   111,   130,
     141,   156,   166,   189,   201,    -1,    -1,    56,    94,   112,
     115,   118,   120,   123,   127,    21,    94,    -1,    -1,    31,
      95,   113,    96,    94,    -1,   114,    -1,   113,    97,   114,
      -1,   152,    -1,    92,   152,    97,   126,    93,    -1,    -1,
      49,    95,   116,    96,    94,    -1,   117,    -1,   116,    97,
     117,    -1,   152,    -1,    92,   152,    97,   126,    93,    -1,
      -1,    58,    95,   119,    96,    94,    -1,   152,    -1,   119,
      97,   152,    -1,    -1,    44,    95,   121,    96,    94,    -1,
     122,    -1,   121,    97,   122,    -1,   152,    -1,    92,   152,
      97,   126,    93,    -1,    -1,    51,    95,   124,    96,    94,
      -1,   125,    -1,   124,    97,   125,    -1,   152,    -1,    92,
     152,    97,   126,    93,    -1,    88,    -1,    87,    -1,    -1,
      74,    95,   128,    96,    94,    -1,   129,    -1,   128,    97,
     129,    -1,    -1,    92,   152,    97,   152,    93,    -1,    -1,
      14,    94,   131,    21,    94,    -1,    -1,   131,   132,    -1,
     135,    -1,    61,    92,   152,    97,   152,    93,    94,    -1,
     182,    94,    -1,    48,    92,   152,    97,   138,    93,    94,
      -1,    -1,    -1,    29,    92,    95,   133,   153,   134,    96,
      97,   182,    93,    94,    -1,    57,   152,   136,    32,     7,
      95,   137,    96,    94,    -1,    -1,    30,    -1,   152,    -1,
     137,    97,   152,    -1,   152,    -1,   138,    97,   152,    -1,
      27,    92,   140,    93,    94,   142,    21,    94,    -1,     5,
      -1,    12,    -1,    -1,   141,   139,    -1,    -1,   142,    28,
      92,   147,   143,    93,    94,    -1,    -1,    97,   152,    -1,
     182,    -1,    39,    92,   144,    93,    -1,   149,    92,   144,
      97,   144,    93,    -1,   150,    92,   148,    93,    -1,    -1,
      -1,   151,    92,    95,   145,   153,   146,    96,    97,   144,
      93,    -1,    -1,   144,    -1,   144,    -1,   148,    97,   144,
      -1,    24,    -1,    35,    -1,    36,    -1,    41,    -1,    42,
      -1,    43,    -1,     3,    -1,    45,    -1,    25,    -1,    29,
      -1,    89,    -1,    87,    -1,    54,    -1,    20,    -1,    46,
      -1,    47,    -1,    53,    -1,   154,    -1,   153,    97,   154,
      -1,   152,    -1,   152,    92,   152,    93,    -1,    73,    92,
     140,    97,    70,    93,    94,   157,    21,    94,    -1,    -1,
     156,   155,    -1,    -1,   157,    28,    92,   147,   143,    93,
      94,    -1,   157,    71,    92,   158,   143,    93,    94,    -1,
     157,    72,    92,   159,   143,    93,    94,    -1,    -1,   162,
      -1,    -1,   163,    -1,   162,    -1,   160,    97,   162,    -1,
     163,    -1,   161,    97,   163,    -1,   152,    -1,    63,    -1,
      26,    -1,    39,    92,   162,    93,    -1,    78,    92,   163,
      93,    -1,    79,    92,   163,    93,    -1,   149,    92,   162,
      97,   162,    93,    -1,   150,    92,   160,    93,    -1,   164,
      92,   163,    97,   162,    93,    -1,   152,    -1,    63,    -1,
      26,    -1,    39,    92,   163,    93,    -1,    83,    -1,    84,
      -1,    82,    92,   163,    93,    -1,    77,    92,   162,    93,
      -1,    85,    92,   163,    97,   162,    93,    -1,    86,    92,
     163,    97,   162,    93,    -1,   149,    92,   163,    97,   163,
      93,    -1,   165,    92,   161,    93,    -1,    75,    -1,    76,
      -1,     3,    -1,    45,    -1,    80,    -1,    81,    -1,    -1,
     166,   167,    -1,    10,    92,   140,    97,    11,    93,    94,
     169,    21,    94,    -1,    -1,    10,    92,   140,    97,    19,
      93,    94,   168,   183,    21,    94,    -1,    -1,   169,     8,
      92,   170,   143,    93,    94,    -1,    -1,   173,    -1,    -1,
      -1,   171,   178,    67,   179,    68,   178,   172,    -1,   176,
      -1,    -1,    -1,    29,    92,    95,   174,   153,   175,    96,
      97,   176,    93,    -1,    45,    92,   177,    93,    -1,    -1,
     180,    -1,   177,    97,   180,    -1,    -1,   178,   180,    -1,
      -1,   179,   180,    -1,   179,   180,    69,    -1,   182,    -1,
      39,    92,   182,    93,    -1,   182,    -1,   181,    97,   182,
      -1,   152,    -1,    63,    -1,    26,    -1,    23,    92,   187,
      97,   187,    93,    -1,   152,    92,   188,    93,    -1,    -1,
     183,     8,    92,   184,   143,    93,    94,    -1,    -1,   185,
      -1,   186,    -1,    25,    92,    95,   181,    96,    97,   186,
      93,    -1,     3,    92,   177,    93,    -1,   152,    -1,   152,
      92,   188,    93,    -1,   187,    -1,   188,    97,   187,    -1,
      -1,   189,   190,    -1,    -1,    50,    92,   152,    93,    94,
     191,   192,    21,    94,    -1,    -1,   192,    60,    92,   196,
      97,   200,    97,   152,    97,    95,   193,    96,   194,    93,
      94,    -1,   196,    -1,   193,    97,   196,    -1,    -1,    97,
      95,   195,    96,    -1,   196,    98,   196,    -1,   195,    97,
     196,    98,   196,    -1,    -1,   198,   197,   199,    -1,   152,
      -1,    39,    -1,    24,    -1,    35,    -1,    36,    -1,     3,
      -1,    45,    -1,    29,    -1,    25,    -1,    -1,    92,   148,
      93,    -1,    92,    95,   153,    96,    97,   144,    93,    -1,
     173,    -1,   185,    -1,    -1,   201,   202,    -1,    -1,    62,
      94,   203,   204,    21,    94,    -1,    -1,   204,   187,    94,
      -1,    -1,   205,   206,    -1,    15,    94,   207,    21,    94,
      -1,    -1,   207,   208,    -1,    16,    92,   211,   209,    93,
      94,    -1,    97,    95,   210,    96,    -1,    -1,   152,    -1,
     152,    97,   210,    -1,    91,    -1,    -1,   212,   213,    -1,
      33,    94,   227,    21,    94,    -1,    -1,    55,    92,   152,
     214,    93,    94,   215,    21,    94,    -1,    90,    -1,    40,
     216,     9,    -1,    -1,   216,   217,    94,    -1,    46,    92,
     218,    93,    -1,    53,    92,   224,    93,    -1,    47,    92,
     220,    93,    -1,    20,    92,   229,    93,    -1,    54,    92,
      89,    97,   126,    93,    -1,   219,    -1,   218,    97,   219,
      -1,   152,    -1,    92,   152,    97,    87,   226,    93,    -1,
     221,    -1,   220,    97,   221,    -1,    92,    87,    97,   222,
      93,    -1,   223,    -1,   222,    97,   223,    -1,   152,    -1,
     225,    -1,   224,    97,   225,    -1,   152,    -1,    -1,    97,
      89,    -1,   228,    -1,   227,   228,    -1,    34,    95,   229,
      96,    94,    -1,   152,    -1,   229,    97,   152,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   242,   242,   255,   266,   270,   274,   278,   282,   283,
     287,   288,   292,   293,   297,   298,   299,   306,   319,   320,
     330,   331,   334,   335,   338,   339,   343,   344,   347,   348,
     351,   352,   355,   356,   359,   360,   363,   364,   367,   368,
     371,   372,   375,   376,   379,   380,   383,   384,   387,   388,
     391,   392,   395,   396,   399,   399,   406,   407,   412,   413,
     416,   417,   419,   420,   422,   423,   422,   432,   436,   437,
     440,   441,   444,   445,   452,   462,   463,   466,   467,   470,
     471,   485,   486,   489,   490,   492,   494,   496,   497,   496,
     504,   505,   508,   510,   514,   515,   516,   517,   518,   519,
     522,   523,   526,   527,   530,   536,   538,   540,   542,   544,
     546,   550,   552,   556,   567,   591,   602,   603,   606,   607,
     620,   632,   648,   649,   652,   653,   660,   661,   664,   665,
     668,   670,   672,   674,   676,   678,   680,   682,   684,   688,
     690,   692,   694,   696,   698,   700,   702,   704,   706,   708,
     710,   714,   715,   718,   719,   720,   721,   729,   730,   733,
     755,   754,   761,   762,   776,   777,   778,   778,   778,   796,
     797,   798,   797,   805,   809,   810,   812,   816,   817,   821,
     822,   824,   839,   840,   844,   845,   848,   850,   852,   854,
     856,   863,   864,   867,   868,   871,   872,   875,   882,   884,
     888,   890,   898,   899,   903,   902,   916,   917,   938,   940,
     945,   946,   949,   957,   968,   967,   979,   980,   981,   982,
     983,   984,   985,   986,   987,   990,   991,   992,   995,   996,
    1004,  1005,  1008,  1008,  1015,  1016,  1023,  1024,  1027,  1030,
    1031,  1034,  1037,  1038,  1041,  1042,  1047,  1054,  1055,  1064,
    1065,  1065,  1073,  1076,  1079,  1080,  1083,  1084,  1085,  1086,
    1106,  1118,  1119,  1122,  1134,  1151,  1152,  1155,  1162,  1163,
    1166,  1172,  1173,  1176,  1195,  1196,  1213,  1214,  1217,  1221,
    1222
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "DFG_AND", "DFG_AUTHOR", "DFG_AXIOMS",
  "DFG_BEGPROB", "DFG_BY", "DFG_CLAUSE", "DFG_CLOSEBRACE", "DFG_CLSLIST",
  "DFG_CNF", "DFG_CONJECS", "DFG_DATE", "DFG_DECLLIST", "DFG_INCLUDELIST",
  "DFG_INCLUDE", "DFG_DESC", "DFG_DESCLIST", "DFG_DNF", "DFG_DOMPRED",
  "DFG_ENDLIST", "DFG_ENDPROB", "DFG_EQUAL", "DFG_EQUIV", "DFG_EXISTS",
  "DFG_FALSE", "DFG_FORMLIST", "DFG_FORMULA", "DFG_FORALL", "DFG_FREELY",
  "DFG_FUNC", "DFG_GENERATED", "DFG_GENSET", "DFG_HYPOTH", "DFG_IMPLIED",
  "DFG_IMPLIES", "DFG_LOGIC", "DFG_NAME", "DFG_NOT", "DFG_OPENBRACE",
  "DFG_XOR", "DFG_NOR", "DFG_NAND", "DFG_OPERAT", "DFG_OR", "DFG_PREC",
  "DFG_CLFORE", "DFG_PRED", "DFG_PRDICAT", "DFG_PRFLIST", "DFG_QUANTIF",
  "DFG_SATIS", "DFG_SELECT", "DFG_SETFLAG", "DFG_SETTINGS", "DFG_SYMLIST",
  "DFG_SORT", "DFG_SORTS", "DFG_STATUS", "DFG_STEP", "DFG_SUBSORT",
  "DFG_TERMLIST", "DFG_TRUE", "DFG_UNKNOWN", "DFG_UNSATIS", "DFG_VERSION",
  "DFG_END_OF_CONSTRAINT_LITERALS", "DFG_END_OF_ANTECEDENT_LITERALS",
  "DFG_LITERAL_IS_SELECTED", "DFG_EML", "DFG_PROP_FORMULA",
  "DFG_REL_FORMULA", "DFG_SPECIALFORMLIST", "DFG_TRANSLPAIRS", "DFG_BOX",
  "DFG_DIA", "DFG_TEST", "DFG_DOMAIN", "DFG_RANGE", "DFG_COMP", "DFG_SUM",
  "DFG_CONV", "DFG_IDENTITY", "DFG_DIVERSITY", "DFG_DOMRESTR",
  "DFG_RANRESTR", "DFG_NUM", "DFG_MINUS1", "DFG_ID", "DFG_TEXT",
  "single_quoted", "'('", "')'", "'.'", "'['", "']'", "','", "':'",
  "$accept", "problem", "description", "name", "author", "status",
  "desctext", "versionopt", "logicopt", "dateopt", "log_state",
  "logicalpart", "symbollistopt", "functionsopt", "functionlist", "func",
  "predicatesopt", "predicatelist", "pred", "sortsopt", "sortlist",
  "operatorsopt", "operatorlist", "op", "quantifiersopt", "quantifierlist",
  "quant", "number", "translpairsopt", "translpairlist", "translpair",
  "declarationlistopt", "decllistopt", "decl", "$@1", "$@2", "gendecl",
  "freelyopt", "funclist", "sortdecl", "formulalist", "origin",
  "formulalistsopt", "formulalistopt", "labelopt", "formula", "$@3", "$@4",
  "formulaopt", "arglist", "binsymbol", "nsymbol", "quantsymbol", "id",
  "qtermlist", "qterm", "specialformulalist", "specialformulalistsopt",
  "specialformulalistopt", "propformulaopt", "relformulaopt",
  "proparglist", "relarglist", "propformula", "relformula",
  "relpropbinsymbol", "relnsymbol", "clauselistsopt", "clauselist", "$@5",
  "cnfclausesopt", "cnfclauseopt", "$@6", "$@7", "cnfclause", "$@8", "$@9",
  "cnfclausebody", "litlist", "litlist_ws", "selected_litlist_ws", "lit",
  "atomlist", "atom", "dnfclausesopt", "dnfclauseopt", "dnfclause",
  "dnfclausebody", "term", "termlist", "prooflistsopt", "prooflist",
  "$@10", "prooflistopt", "parentlist", "assoclistopt", "assoclist",
  "id_or_formula", "$@11", "anysymbol", "optargs", "clause",
  "listOfTermsopt", "listOfTerms", "$@12", "terms", "includelistopt",
  "includelist", "includes", "include", "formula_selection", "name_list",
  "filename", "settinglistsopt", "settinglist", "$@13", "flags",
  "spassflags", "spassflag", "preclist", "precitem", "clfolist",
  "clfoitem", "clfoaxseq", "clfoaxseqitem", "selectlist", "selectitem",
  "statopt", "gsettings", "gsetting", "labellist", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,    40,    41,    46,    91,    93,    44,    58
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    99,   100,   101,   102,   103,   104,   105,   106,   106,
     107,   107,   108,   108,   109,   109,   109,   110,   111,   111,
     112,   112,   113,   113,   114,   114,   115,   115,   116,   116,
     117,   117,   118,   118,   119,   119,   120,   120,   121,   121,
     122,   122,   123,   123,   124,   124,   125,   125,   126,   126,
     127,   127,   128,   128,   129,   129,   130,   130,   131,   131,
     132,   132,   132,   132,   133,   134,   132,   135,   136,   136,
     137,   137,   138,   138,   139,   140,   140,   141,   141,   142,
     142,   143,   143,   144,   144,   144,   144,   145,   146,   144,
     147,   147,   148,   148,   149,   149,   149,   149,   149,   149,
     150,   150,   151,   151,   152,   152,   152,   152,   152,   152,
     152,   153,   153,   154,   154,   155,   156,   156,   157,   157,
     157,   157,   158,   158,   159,   159,   160,   160,   161,   161,
     162,   162,   162,   162,   162,   162,   162,   162,   162,   163,
     163,   163,   163,   163,   163,   163,   163,   163,   163,   163,
     163,   164,   164,   165,   165,   165,   165,   166,   166,   167,
     168,   167,   169,   169,   170,   170,   171,   172,   170,   173,
     174,   175,   173,   176,   177,   177,   177,   178,   178,   179,
     179,   179,   180,   180,   181,   181,   182,   182,   182,   182,
     182,   183,   183,   184,   184,   185,   185,   186,   187,   187,
     188,   188,   189,   189,   191,   190,   192,   192,   193,   193,
     194,   194,   195,   195,   197,   196,   198,   198,   198,   198,
     198,   198,   198,   198,   198,   199,   199,   199,   200,   200,
     201,   201,   203,   202,   204,   204,   205,   205,   206,   207,
     207,   208,   209,   209,   210,   210,   211,   212,   212,   213,
     214,   213,   215,   215,   216,   216,   217,   217,   217,   217,
     217,   218,   218,   219,   219,   220,   220,   221,   222,   222,
     223,   224,   224,   225,   226,   226,   227,   227,   228,   229,
     229
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,    11,    11,     5,     5,     5,     5,     0,     5,
       0,     5,     0,     5,     1,     1,     1,     7,     0,    10,
       0,     5,     1,     3,     1,     5,     0,     5,     1,     3,
       1,     5,     0,     5,     1,     3,     0,     5,     1,     3,
       1,     5,     0,     5,     1,     3,     1,     5,     1,     1,
       0,     5,     1,     3,     0,     5,     0,     5,     0,     2,
       1,     7,     2,     7,     0,     0,    11,     9,     0,     1,
       1,     3,     1,     3,     8,     1,     1,     0,     2,     0,
       7,     0,     2,     1,     4,     6,     4,     0,     0,    10,
       0,     1,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     1,     4,    10,     0,     2,     0,     7,
       7,     7,     0,     1,     0,     1,     1,     3,     1,     3,
       1,     1,     1,     4,     4,     4,     6,     4,     6,     1,
       1,     1,     4,     1,     1,     4,     4,     6,     6,     6,
       4,     1,     1,     1,     1,     1,     1,     0,     2,    10,
       0,    11,     0,     7,     0,     1,     0,     0,     7,     1,
       0,     0,    10,     4,     0,     1,     3,     0,     2,     0,
       2,     3,     1,     4,     1,     3,     1,     1,     1,     6,
       4,     0,     7,     0,     1,     1,     8,     4,     1,     4,
       1,     3,     0,     2,     0,     9,     0,    15,     1,     3,
       0,     4,     3,     5,     0,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     3,     7,     1,     1,
       0,     2,     0,     6,     0,     3,     0,     2,     5,     0,
       2,     6,     4,     0,     1,     3,     1,     0,     2,     5,
       0,     9,     1,     3,     0,     3,     4,     4,     4,     4,
       6,     1,     3,     1,     6,     1,     3,     5,     1,     3,
       1,     1,     3,     1,     0,     2,     1,     2,     5,     1,
       3
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,     0,     0,     1,   107,   108,   109,   110,   106,
     105,   104,     0,     0,     0,     0,    18,     0,     0,   236,
      56,     0,     0,    20,   247,     0,    77,     0,     0,     8,
       0,    26,     0,   237,     0,    58,   116,     0,     0,     0,
      10,     0,     0,    32,   239,     0,     0,     0,   248,     0,
       0,    78,   157,     0,     0,     0,     0,     0,     0,     0,
      22,    24,     0,     0,    36,     0,     2,     0,     0,     0,
       0,   188,     0,     0,     0,     0,   187,    59,    60,   186,
       0,     0,     0,   117,   202,     4,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    28,    30,     0,     0,
      42,     0,     0,   240,     0,     0,   276,   250,    57,     0,
       0,     0,    68,     0,     0,    62,    75,    76,     0,     0,
       0,   158,   230,     5,     0,     0,     0,     0,    12,     0,
      21,    23,     0,     0,     0,     0,    34,     0,     0,    50,
       0,   238,     0,     0,   277,     0,   198,     0,    64,     0,
      69,     0,     0,   200,     0,     0,     0,     0,     0,   203,
      17,     9,     0,    14,    16,    15,     0,     0,     0,     0,
      49,    48,     0,     0,    27,    29,     0,     0,     0,     0,
      38,    40,     0,     0,     0,   246,   243,   279,     0,   249,
       0,     0,     0,     0,     0,     0,     0,   190,     0,    79,
       0,     0,     0,     0,   231,    11,     0,     0,     0,     0,
      25,     0,    33,    35,     0,     0,     0,     0,     0,    44,
      46,    54,     0,     0,     0,     0,     0,     0,     0,     0,
     113,    65,   111,     0,    72,     0,     0,   201,     0,     0,
       0,     0,   232,     6,     0,     0,     3,    31,     0,    37,
      39,     0,     0,     0,     0,     0,    52,    19,     0,     0,
     278,   280,   254,   252,     0,   199,   189,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     234,     7,     0,     0,     0,    43,    45,     0,     0,    54,
     244,     0,   241,     0,     0,     0,   112,     0,    63,    73,
       0,    70,    61,    74,    90,   118,     0,     0,   204,     0,
      13,    41,     0,     0,    51,    53,     0,   242,   253,     0,
       0,     0,     0,     0,     0,   251,   114,     0,     0,     0,
     100,    94,   102,   103,    95,    96,     0,    97,    98,    99,
     101,    91,    81,     0,     0,     0,    83,     0,   162,   160,
     206,     0,     0,    47,     0,   245,     0,     0,     0,     0,
       0,   255,     0,    67,    71,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   191,     0,   233,   235,
      55,     0,     0,   263,     0,   261,     0,     0,   265,   273,
       0,   271,     0,     0,     0,    82,     0,     0,    92,     0,
      87,   115,    90,   122,   124,     0,     0,     0,     0,     0,
     259,     0,   256,     0,     0,   258,     0,   257,     0,     0,
      66,    84,    80,     0,    86,     0,     0,    81,   132,     0,
     131,   151,   152,     0,     0,     0,     0,   130,    81,   123,
       0,   153,   141,     0,   154,   140,     0,   155,   156,     0,
     143,   144,     0,     0,     0,   139,    81,   125,     0,   166,
     159,     0,     0,   205,     0,     0,   262,     0,   266,   272,
       0,     0,    93,    88,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    81,   177,   165,   169,   193,   161,   221,   218,
     224,   223,   219,   220,   217,   222,   216,     0,   214,   274,
     270,     0,   268,   260,    85,     0,     0,     0,     0,     0,
       0,     0,   126,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   128,     0,   174,     0,     0,     0,     0,
      81,   194,   195,     0,   225,     0,     0,   267,     0,     0,
     119,   133,   134,   135,     0,   137,     0,   120,     0,   142,
     146,   145,     0,     0,     0,   121,   150,     0,   170,     0,
       0,   175,   182,     0,   179,   178,   174,     0,     0,   228,
     229,     0,     0,   215,   275,   264,   269,     0,     0,   127,
       0,     0,     0,     0,   129,     0,     0,   173,     0,   163,
       0,     0,     0,     0,     0,     0,     0,     0,   136,   138,
     147,   148,   149,   171,     0,   176,   177,   180,   197,     0,
     184,   192,     0,     0,   226,    89,     0,   183,   167,   181,
       0,     0,     0,     0,     0,   168,     0,   185,     0,     0,
       0,     0,     0,   208,     0,     0,   196,   210,     0,   227,
     172,     0,     0,   209,     0,     0,     0,     0,   207,   211,
       0,     0,     0,   212,     0,   213
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,    16,    22,    29,    90,   128,    40,    57,   169,
     166,    19,    20,    31,    59,    60,    43,    95,    96,    64,
     135,   100,   179,   180,   139,   218,   219,   172,   184,   255,
     256,    26,    49,    77,   193,   269,    78,   151,   300,   233,
      51,   118,    36,   238,   367,   341,   426,   515,   342,   399,
     454,   344,   345,    79,   231,   232,    83,    52,   347,   438,
     456,   521,   532,   439,   457,   440,   458,    84,   121,   376,
     375,   492,   493,   635,   494,   595,   626,   495,   570,   537,
     600,   571,   619,   346,   407,   540,   541,   542,   153,   154,
     122,   159,   350,   377,   642,   652,   656,   507,   544,   508,
     583,   581,   160,   204,   280,   309,    24,    33,    65,   103,
     224,   291,   186,    34,    48,   145,   264,   293,   324,   384,
     385,   387,   388,   511,   512,   390,   391,   546,   105,   106,
     188
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -526
static const yytype_int16 yypact[] =
{
      15,   -74,    27,   479,  -526,  -526,  -526,  -526,  -526,  -526,
    -526,  -526,   -21,   -12,    78,     4,    48,    69,    19,  -526,
     124,    54,   115,   147,   179,   110,  -526,   107,   114,   145,
     122,   171,   133,  -526,    47,  -526,   208,   160,   167,   166,
     222,   228,   168,   204,  -526,   172,   173,   181,  -526,   639,
     184,  -526,   198,   189,   191,   187,   194,   229,   479,   -47,
    -526,  -526,   288,   192,   245,    30,  -526,   256,   479,   197,
     200,  -526,   201,   202,   479,   203,  -526,  -526,  -526,   205,
     206,    65,   207,  -526,   286,  -526,   209,   211,   212,   213,
     281,   214,   215,   228,   479,    33,  -526,  -526,   479,   217,
     250,   224,   220,  -526,   223,     2,  -526,  -526,  -526,   479,
     226,   479,   289,   479,   479,  -526,  -526,  -526,   230,    65,
     233,  -526,   272,  -526,   237,   239,    22,   244,   325,    57,
    -526,  -526,   247,   254,   288,    75,  -526,   349,   257,   285,
     264,  -526,   479,   266,  -526,   268,   270,   267,  -526,   269,
    -526,   333,   271,  -526,     0,   276,   274,    65,   280,  -526,
     311,  -526,   282,  -526,  -526,  -526,   290,   284,   292,   357,
    -526,  -526,   293,    57,  -526,  -526,   287,   479,   479,   105,
    -526,  -526,   367,   294,   361,  -526,   291,  -526,   113,  -526,
     296,   479,   479,   479,   479,   378,   479,  -526,   479,  -526,
     322,   297,   479,   303,  -526,  -526,   310,   305,   315,   312,
    -526,   316,  -526,  -526,   314,   318,   349,   479,   119,  -526,
    -526,   324,   323,   313,   326,   328,   479,   -34,    12,   332,
     334,   321,  -526,    35,  -526,   336,   339,  -526,    71,   341,
       9,   342,  -526,  -526,   343,   351,  -526,  -526,    57,  -526,
    -526,   345,   354,   367,   479,   137,  -526,  -526,   479,   358,
    -526,  -526,  -526,  -526,   432,  -526,  -526,   479,   479,   359,
     366,   479,   479,   368,   369,   373,   372,   376,   377,   383,
    -526,  -526,   391,   395,    57,  -526,  -526,   374,   396,   324,
     399,   401,  -526,   178,   398,   405,  -526,   403,  -526,  -526,
     142,  -526,  -526,  -526,   404,  -526,   409,   410,  -526,    96,
    -526,  -526,   412,   479,  -526,  -526,   479,  -526,  -526,   414,
     415,   416,   417,   418,   419,  -526,  -526,   590,   420,   479,
    -526,  -526,  -526,  -526,  -526,  -526,   423,  -526,  -526,  -526,
    -526,  -526,   421,   424,   425,   427,  -526,   -11,  -526,  -526,
    -526,   426,   428,  -526,   430,  -526,   479,   448,   429,   479,
     422,  -526,   431,  -526,  -526,   404,   479,   434,   404,   404,
     435,   437,   442,   444,   446,    36,  -526,    -7,  -526,  -526,
    -526,    40,   479,  -526,    43,  -526,   452,    59,  -526,  -526,
      60,  -526,   445,   447,   436,  -526,   449,   450,  -526,    61,
    -526,  -526,   404,   628,   543,   457,   462,    37,   463,   466,
    -526,   465,  -526,   448,   468,  -526,   429,  -526,   479,    57,
    -526,  -526,  -526,   404,  -526,   404,   479,   421,  -526,   478,
    -526,  -526,  -526,   480,   481,   482,   483,  -526,   421,  -526,
     484,  -526,  -526,   485,  -526,  -526,   489,  -526,  -526,   495,
    -526,  -526,   499,   501,   502,  -526,   421,  -526,   503,   -14,
    -526,   506,   477,  -526,   183,   512,  -526,   479,  -526,  -526,
     451,   507,  -526,   321,   510,   628,   543,   543,   628,   628,
     511,   543,   543,   628,   543,   543,   543,   543,   514,   543,
     513,   516,   421,  -526,  -526,  -526,    16,  -526,  -526,  -526,
    -526,  -526,  -526,  -526,  -526,  -526,  -526,   515,  -526,   517,
    -526,    62,  -526,  -526,  -526,   519,   523,   518,   525,   526,
     524,    68,  -526,   528,   536,   541,   545,   546,   544,   549,
     550,   548,    70,  -526,   554,   734,   547,   685,   558,   563,
     421,  -526,  -526,    23,   564,   520,   573,  -526,   479,   575,
    -526,  -526,  -526,  -526,   628,  -526,   628,  -526,   628,  -526,
    -526,  -526,   628,   628,   543,  -526,  -526,   543,  -526,   584,
      73,  -526,  -526,   586,  -526,  -526,   734,   583,   591,  -526,
    -526,   592,   304,  -526,  -526,  -526,  -526,   404,   595,  -526,
     597,   601,   602,   604,  -526,   479,   590,  -526,   734,  -526,
     690,    87,   590,   589,   479,   479,   103,   605,  -526,  -526,
    -526,  -526,  -526,   321,   606,  -526,  -526,   443,  -526,   144,
    -526,  -526,   612,   146,  -526,  -526,   616,  -526,   734,  -526,
     617,   590,   623,   622,   624,  -526,   698,  -526,   183,   404,
     675,   629,   148,  -526,   630,   632,  -526,   633,   183,  -526,
    -526,   638,   634,  -526,   183,   640,   150,   637,  -526,  -526,
     183,   183,   642,  -526,   183,  -526
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,
    -526,  -526,  -526,  -526,  -526,   648,  -526,  -526,   608,  -526,
    -526,  -526,  -526,   529,  -526,  -526,   493,  -170,  -526,  -526,
     458,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,
    -526,  -103,  -526,  -526,  -416,  -361,  -526,  -526,   347,   169,
    -302,  -394,  -526,    -3,  -413,   487,  -526,  -526,  -526,  -526,
    -526,  -526,  -526,  -436,  -452,  -526,  -526,  -526,  -526,  -526,
    -526,  -526,  -526,  -526,   216,  -526,  -526,   116,   174,   149,
    -526,  -525,  -526,   -48,  -526,  -526,   218,   126,  -104,   572,
    -526,  -526,  -526,  -526,  -526,  -526,  -526,  -513,  -526,  -526,
    -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,
    -526,   453,  -526,  -526,  -526,  -526,  -526,  -526,  -526,  -526,
     353,  -526,   348,  -526,   219,  -526,   350,  -526,  -526,   665,
     433
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -165
static const yytype_int16 yytable[] =
{
      12,    80,   343,   211,   394,   147,   262,   397,   398,   436,
     371,   474,   575,   473,   408,   490,   156,   372,     3,   538,
     277,     1,   480,   143,   518,   519,   538,     4,   278,   524,
     525,   491,   527,   528,   529,   530,   104,   533,    61,   517,
     488,   539,   520,   522,   405,   461,   101,   526,   539,    92,
      93,   102,   490,   409,   201,    91,   263,   406,   462,    97,
     373,   374,   471,   343,   472,   107,   343,   343,   491,    45,
     116,   112,    13,   615,   163,   617,   536,   117,   283,  -164,
      46,   436,    14,  -164,   436,   436,   164,   165,   229,   436,
      61,   132,   274,   197,   237,   136,    15,   198,    17,   275,
     343,   435,    47,   575,    18,   265,   146,    21,   149,   198,
     152,   146,   593,    23,   312,   594,     5,   351,   588,    28,
     589,   343,   590,   343,   578,   643,   591,   592,   270,   133,
     134,    97,   271,   410,   181,   653,   412,   226,    25,   187,
     413,   657,     6,     7,   170,   171,    27,   662,   663,     8,
       9,   665,   415,   417,   424,   547,   416,   418,   425,   548,
     436,   555,   436,   566,   436,   556,   597,   567,   436,   436,
     598,   176,   177,   435,   213,   214,   435,   435,    30,   220,
     618,   435,   613,    10,   598,    11,   498,   318,   146,   146,
     230,   234,   623,   236,    32,   146,   624,    37,   319,   241,
     425,   215,   216,     5,    35,   352,    38,   499,   500,   225,
     226,    39,   501,   181,   251,   252,   253,    41,   502,   503,
      42,   398,   504,   261,   320,   321,   607,    44,   505,     6,
       7,   322,   323,   288,   289,    50,     8,     9,   328,   329,
     630,   631,   633,   268,   647,   648,   659,   660,     5,   470,
     220,   287,   435,    53,   435,   290,   435,    54,    55,    56,
     435,   435,    63,    62,   295,   230,    66,    67,   299,   301,
      10,    82,    11,    68,     6,     7,    81,    87,   644,   362,
     343,     8,     9,    85,    86,   343,    88,    98,    89,    99,
     104,   108,   109,   110,   111,   113,   120,   114,   127,   119,
     115,   138,   125,   123,   124,   126,   146,   330,     5,   130,
     354,   129,   137,   290,   141,    10,   140,    11,   142,   150,
      58,   148,   158,   155,     5,   157,   364,    70,   331,   332,
      71,   161,   162,   333,     6,     7,   167,   343,   168,   334,
     335,     8,     9,   336,   173,   337,   338,   339,   174,   340,
       6,     7,   182,   187,   383,   185,   389,     8,     9,   183,
     189,   190,   191,   395,   192,   195,   194,    76,   196,     5,
     199,   200,   202,   203,   207,    10,   205,    11,   209,   411,
      94,   212,   222,   206,   208,   235,   210,     5,   223,   221,
     227,    10,   239,    11,   240,     6,     7,   242,   244,   605,
     437,   455,     8,     9,   243,   245,   246,   330,   258,   247,
     383,   248,   249,     6,     7,   389,   254,   257,   268,   259,
       8,     9,   260,   230,     5,   266,   267,    70,   331,   332,
      71,   272,   273,   333,   276,   279,    10,   281,    11,   334,
     335,   178,   284,   336,   282,   337,   338,   339,   285,   340,
       6,     7,   292,   294,    10,   297,    11,     8,     9,   217,
     298,   506,   302,   303,   510,   304,   305,    76,     5,   306,
     307,   313,   437,   455,   455,   437,   437,   308,   455,   455,
     437,   455,   455,   455,   455,   310,   455,   572,   311,   572,
     314,    10,   325,    11,     6,     7,   316,   317,   326,     5,
     327,     8,     9,   348,   349,   353,   356,   357,   358,   359,
     360,   392,   629,   361,   363,   365,   368,   369,   366,   370,
     378,   386,   379,   380,   393,     6,     7,   396,   572,   421,
     400,   401,     8,     9,   402,    10,   403,    11,   404,   414,
     382,   420,   419,   422,   513,   510,   441,   423,   614,   459,
     572,   437,   572,   437,   620,   437,   460,   463,   464,   437,
     437,   455,   465,     5,   455,   467,    10,   331,    11,   442,
     475,   497,   476,   477,   478,   479,   481,   482,   334,   335,
     572,   483,   443,   637,   337,   338,   339,   484,   444,     6,
       7,   485,   230,   486,   487,   489,     8,     9,   496,   509,
     514,   622,   230,   516,   523,   534,   445,   531,   535,   584,
       5,   551,   543,    70,   545,   549,    71,   550,   552,   553,
     446,   554,   557,   447,   448,   449,   450,   451,   452,   453,
      10,   330,    11,   558,   559,   506,     6,     7,   560,   561,
     573,   562,   565,     8,     9,   506,   563,   564,     5,   568,
     576,   506,   331,    76,   428,   577,   582,   506,   506,     5,
      69,   506,    70,   334,   335,    71,   585,   429,    72,   337,
     338,   339,   587,   340,     6,     7,   596,    10,   602,    11,
     599,     8,     9,   621,   603,     6,     7,    73,   608,   604,
     609,   430,     8,     9,   610,   611,    74,   612,   625,   627,
      75,   538,    76,   431,   432,     5,   433,   434,    70,   632,
       5,    71,   634,    70,   636,    10,    71,    11,   638,   639,
     491,   640,   646,   649,   569,   650,    10,   655,    11,   569,
     651,     6,     7,   654,   658,   661,     6,     7,     8,     9,
     664,   131,   175,     8,     9,   250,   286,   315,    76,   427,
     601,   606,   574,    76,     5,   296,   645,    70,   616,   579,
      71,   580,   641,   228,   468,   628,   466,   586,   469,   355,
     144,     0,    10,   569,    11,     0,     0,    10,     0,    11,
       6,     7,     0,     0,     0,     0,     0,     8,     9,   381,
       0,     0,     0,     0,     0,     0,     0,    76,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    10,     0,    11
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-526))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       3,    49,   304,   173,   365,   109,    40,   368,   369,   403,
      21,   427,   537,   426,    21,    29,   119,    28,    92,     3,
      11,     6,   438,    21,   476,   477,     3,     0,    19,   481,
     482,    45,   484,   485,   486,   487,    34,   489,    41,   475,
     456,    25,   478,   479,     8,     8,    16,   483,    25,    96,
      97,    21,    29,    60,   157,    58,    90,    21,    21,    62,
      71,    72,   423,   365,   425,    68,   368,   369,    45,    22,
       5,    74,    93,   598,    52,   600,   492,    12,   248,    93,
      33,   475,    94,    97,   478,   479,    64,    65,   192,   483,
      93,    94,    21,    93,   198,    98,    18,    97,    94,    28,
     402,   403,    55,   628,    56,    93,   109,    38,   111,    97,
     113,   114,   564,    94,   284,   567,    20,    21,   554,     4,
     556,   423,   558,   425,   540,   638,   562,   563,    93,    96,
      97,   134,    97,    93,   137,   648,    93,    97,    14,   142,
      97,   654,    46,    47,    87,    88,    92,   660,   661,    53,
      54,   664,    93,    93,    93,    93,    97,    97,    97,    97,
     554,    93,   556,    93,   558,    97,    93,    97,   562,   563,
      97,    96,    97,   475,   177,   178,   478,   479,    31,   182,
      93,   483,   595,    87,    97,    89,     3,     9,   191,   192,
     193,   194,   605,   196,    15,   198,    93,    90,    20,   202,
      97,    96,    97,    20,    94,   309,    92,    24,    25,    96,
      97,    66,    29,   216,   217,    96,    97,    95,    35,    36,
      49,   582,    39,   226,    46,    47,   587,    94,    45,    46,
      47,    53,    54,    96,    97,    27,    53,    54,    96,    97,
      96,    97,    96,    97,    96,    97,    96,    97,    20,   419,
     253,   254,   554,    93,   556,   258,   558,    90,    92,    37,
     562,   563,    58,    95,   267,   268,    94,    94,   271,   272,
      87,    73,    89,    92,    46,    47,    92,    90,   639,   327,
     582,    53,    54,    94,    93,   587,    92,    95,    59,    44,
      34,    94,    92,    92,    92,    92,    10,    92,    17,    92,
      94,    51,    90,    94,    93,    92,   309,     3,    20,    94,
     313,    97,    95,   316,    94,    87,    92,    89,    95,    30,
      92,    95,    50,    93,    20,    92,   329,    23,    24,    25,
      26,    94,    93,    29,    46,    47,    92,   639,    13,    35,
      36,    53,    54,    39,    97,    41,    42,    43,    94,    45,
      46,    47,    95,   356,   357,    91,   359,    53,    54,    74,
      94,    93,    92,   366,    97,    32,    97,    63,    97,    20,
      94,    97,    92,    62,    90,    87,    94,    89,    21,   382,
      92,    94,    21,    93,    92,     7,    93,    20,    97,    95,
      94,    87,    70,    89,    97,    46,    47,    94,    93,    95,
     403,   404,    53,    54,    94,    90,    94,     3,    95,    93,
     413,    97,    94,    46,    47,   418,    92,    94,    97,    93,
      53,    54,    94,   426,    20,    93,    92,    23,    24,    25,
      26,    95,    93,    29,    93,    93,    87,    94,    89,    35,
      36,    92,    97,    39,    93,    41,    42,    43,    94,    45,
      46,    47,    94,    21,    87,    96,    89,    53,    54,    92,
      94,   464,    94,    94,   467,    92,    94,    63,    20,    93,
      93,    97,   475,   476,   477,   478,   479,    94,   481,   482,
     483,   484,   485,   486,   487,    94,   489,   535,    93,   537,
      94,    87,    94,    89,    46,    47,    97,    96,    93,    20,
      97,    53,    54,    94,    94,    93,    92,    92,    92,    92,
      92,    89,    69,    94,    94,    92,    92,    92,    97,    92,
      94,    92,    94,    93,    93,    46,    47,    93,   576,    93,
      95,    94,    53,    54,    92,    87,    92,    89,    92,    87,
      92,    94,    97,    94,    93,   548,     3,    97,   596,    92,
     598,   554,   600,   556,   602,   558,    94,    94,    92,   562,
     563,   564,    97,    20,   567,    97,    87,    24,    89,    26,
      92,    94,    92,    92,    92,    92,    92,    92,    35,    36,
     628,    92,    39,   631,    41,    42,    43,    92,    45,    46,
      47,    92,   595,    92,    92,    92,    53,    54,    92,    87,
      93,   604,   605,    93,    93,    92,    63,    93,    92,    89,
      20,    93,    97,    23,    97,    96,    26,    94,    93,    93,
      77,    97,    94,    80,    81,    82,    83,    84,    85,    86,
      87,     3,    89,    97,    93,   638,    46,    47,    93,    93,
      93,    97,    94,    53,    54,   648,    97,    97,    20,    95,
      92,   654,    24,    63,    26,    92,    92,   660,   661,    20,
      21,   664,    23,    35,    36,    26,    93,    39,    29,    41,
      42,    43,    97,    45,    46,    47,    92,    87,    95,    89,
      94,    53,    54,    94,    93,    46,    47,    48,    93,    97,
      93,    63,    53,    54,    93,    93,    57,    93,    93,    93,
      61,     3,    63,    75,    76,    20,    78,    79,    23,    97,
      20,    26,    96,    23,    97,    87,    26,    89,    95,    97,
      45,    97,    93,    93,    39,    93,    87,    93,    89,    39,
      97,    46,    47,    95,    94,    98,    46,    47,    53,    54,
      98,    93,   134,    53,    54,   216,   253,   289,    63,   402,
     576,   582,    67,    63,    20,   268,   640,    23,    68,   543,
      26,   543,   636,   191,   416,   616,   413,   548,   418,   316,
     105,    -1,    87,    39,    89,    -1,    -1,    87,    -1,    89,
      46,    47,    -1,    -1,    -1,    -1,    -1,    53,    54,   356,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    63,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    87,    -1,    89
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     6,   100,    92,     0,    20,    46,    47,    53,    54,
      87,    89,   152,    93,    94,    18,   101,    94,    56,   110,
     111,    38,   102,    94,   205,    14,   130,    92,     4,   103,
      31,   112,    15,   206,   212,    94,   141,    90,    92,    66,
     106,    95,    49,   115,    94,    22,    33,    55,   213,   131,
      27,   139,   156,    93,    90,    92,    37,   107,    92,   113,
     114,   152,    95,    58,   118,   207,    94,    94,    92,    21,
      23,    26,    29,    48,    57,    61,    63,   132,   135,   152,
     182,    92,    73,   155,   166,    94,    93,    90,    92,    59,
     104,   152,    96,    97,    92,   116,   117,   152,    95,    44,
     120,    16,    21,   208,    34,   227,   228,   152,    94,    92,
      92,    92,   152,    92,    92,    94,     5,    12,   140,    92,
      10,   167,   189,    94,    93,    90,    92,    17,   105,    97,
      94,   114,   152,    96,    97,   119,   152,    95,    51,   123,
      92,    94,    95,    21,   228,   214,   152,   187,    95,   152,
      30,   136,   152,   187,   188,    93,   140,    92,    50,   190,
     201,    94,    93,    52,    64,    65,   109,    92,    13,   108,
      87,    88,   126,    97,    94,   117,    96,    97,    92,   121,
     122,   152,    95,    74,   127,    91,   211,   152,   229,    94,
      93,    92,    97,   133,    97,    32,    97,    93,    97,    94,
      97,   140,    92,    62,   202,    94,    93,    90,    92,    21,
      93,   126,    94,   152,   152,    96,    97,    92,   124,   125,
     152,    95,    21,    97,   209,    96,    97,    94,   188,   187,
     152,   153,   154,   138,   152,     7,   152,   187,   142,    70,
      97,   152,    94,    94,    93,    90,    94,    93,    97,    94,
     122,   152,    96,    97,    92,   128,   129,    94,    95,    93,
      94,   152,    40,    90,   215,    93,    93,    92,    97,   134,
      93,    97,    95,    93,    21,    28,    93,    11,    19,    93,
     203,    94,    93,   126,    97,    94,   125,   152,    96,    97,
     152,   210,    94,   216,    21,   152,   154,    96,    94,   152,
     137,   152,    94,    94,    92,    94,    93,    93,    94,   204,
      94,    93,   126,    97,    94,   129,    97,    96,     9,    20,
      46,    47,    53,    54,   217,    94,    93,    97,    96,    97,
       3,    24,    25,    29,    35,    36,    39,    41,    42,    43,
      45,   144,   147,   149,   150,   151,   182,   157,    94,    94,
     191,    21,   187,    93,   152,   210,    92,    92,    92,    92,
      92,    94,   182,    94,   152,    92,    97,   143,    92,    92,
      92,    21,    28,    71,    72,   169,   168,   192,    94,    94,
      93,   229,    92,   152,   218,   219,    92,   220,   221,   152,
     224,   225,    89,    93,   144,   152,    93,   144,   144,   148,
      95,    94,    92,    92,    92,     8,    21,   183,    21,    60,
      93,   152,    93,    97,    87,    93,    97,    93,    97,    97,
      94,    93,    94,    97,    93,    97,   145,   147,    26,    39,
      63,    75,    76,    78,    79,   149,   150,   152,   158,   162,
     164,     3,    26,    39,    45,    63,    77,    80,    81,    82,
      83,    84,    85,    86,   149,   152,   159,   163,   165,    92,
      94,     8,    21,    94,    92,    97,   219,    97,   221,   225,
     126,   144,   144,   153,   143,    92,    92,    92,    92,    92,
     143,    92,    92,    92,    92,    92,    92,    92,   143,    92,
      29,    45,   170,   171,   173,   176,    92,    94,     3,    24,
      25,    29,    35,    36,    39,    45,   152,   196,   198,    87,
     152,   222,   223,    93,    93,   146,    93,   162,   163,   163,
     162,   160,   162,    93,   163,   163,   162,   163,   163,   163,
     163,    93,   161,   163,    92,    92,   143,   178,     3,    25,
     184,   185,   186,    97,   197,    97,   226,    93,    97,    96,
      94,    93,    93,    93,    97,    93,    97,    94,    97,    93,
      93,    93,    97,    97,    97,    94,    93,    97,    95,    39,
     177,   180,   182,    93,    67,   180,    92,    92,   143,   173,
     185,   200,    92,   199,    89,    93,   223,    97,   162,   162,
     162,   162,   162,   163,   163,   174,    92,    93,    97,    94,
     179,   177,    95,    93,    97,    95,   148,   144,    93,    93,
      93,    93,    93,   153,   182,   180,    68,   180,    93,   181,
     182,    94,   152,   153,    93,    93,   175,    93,   178,    69,
      96,    97,    97,    96,    96,   172,    97,   182,    95,    97,
      97,   186,   193,   196,   144,   176,    93,    96,    97,    93,
      93,    97,   194,   196,    95,    93,   195,   196,    94,    96,
      97,    98,   196,   196,    98,   196
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
#line 247 "./SPASS/dfgparser.y"
    { string_StringFree((yyvsp[(3) - (11)].string));
		  YYACCEPT; /* Stop immediately */ }
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 267 "./SPASS/dfgparser.y"
    { dfg_DESC->name = (yyvsp[(3) - (5)].string); }
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 271 "./SPASS/dfgparser.y"
    { dfg_DESC->author = (yyvsp[(3) - (5)].string); }
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 275 "./SPASS/dfgparser.y"
    { dfg_DESC->status = (yyvsp[(3) - (5)].state); }
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 279 "./SPASS/dfgparser.y"
    { dfg_DESC->description = (yyvsp[(3) - (5)].string); }
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 284 "./SPASS/dfgparser.y"
    { dfg_DESC->version = (yyvsp[(3) - (5)].string); }
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 289 "./SPASS/dfgparser.y"
    { dfg_DESC->logic = (yyvsp[(3) - (5)].string); }
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 294 "./SPASS/dfgparser.y"
    { dfg_DESC->date = (yyvsp[(3) - (5)].string); }
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 297 "./SPASS/dfgparser.y"
    { (yyval.state) = DFG_SATISFIABLE; }
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 298 "./SPASS/dfgparser.y"
    { (yyval.state) = DFG_UNSATISFIABLE; }
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 299 "./SPASS/dfgparser.y"
    { (yyval.state) = DFG_UNKNOWNSTATE; }
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 338 "./SPASS/dfgparser.y"
    { dfg_SymbolDecl(DFG_FUNC, (yyvsp[(1) - (1)].string), -2); }
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 340 "./SPASS/dfgparser.y"
    { dfg_SymbolDecl(DFG_FUNC, (yyvsp[(2) - (5)].string), (yyvsp[(4) - (5)].number)); }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 351 "./SPASS/dfgparser.y"
    { dfg_SymbolDecl(DFG_PRDICAT, (yyvsp[(1) - (1)].string), -2); }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 352 "./SPASS/dfgparser.y"
    { dfg_SymbolDecl(DFG_PRDICAT, (yyvsp[(2) - (5)].string), (yyvsp[(4) - (5)].number)); }
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 359 "./SPASS/dfgparser.y"
    { dfg_SymbolDecl(DFG_PRDICAT, (yyvsp[(1) - (1)].string), 1); }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 360 "./SPASS/dfgparser.y"
    { dfg_SymbolDecl(DFG_PRDICAT, (yyvsp[(3) - (3)].string), 1); }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 371 "./SPASS/dfgparser.y"
    { dfg_SymbolDecl(DFG_OPERAT, (yyvsp[(1) - (1)].string), -2); }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 372 "./SPASS/dfgparser.y"
    { dfg_SymbolDecl(DFG_OPERAT, (yyvsp[(2) - (5)].string), (yyvsp[(4) - (5)].number)); }
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 383 "./SPASS/dfgparser.y"
    { dfg_SymbolDecl(DFG_QUANTIF, (yyvsp[(1) - (1)].string), -2); }
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 384 "./SPASS/dfgparser.y"
    { dfg_SymbolDecl(DFG_QUANTIF, (yyvsp[(2) - (5)].string), (yyvsp[(4) - (5)].number)); }
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 387 "./SPASS/dfgparser.y"
    { (yyval.number) = -1; }
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 388 "./SPASS/dfgparser.y"
    { (yyval.number) = (yyvsp[(1) - (1)].number); }
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 399 "./SPASS/dfgparser.y"
    { dfg_TranslPairDecl((yyvsp[(2) - (5)].string), (yyvsp[(4) - (5)].string)); }
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 418 "./SPASS/dfgparser.y"
    { dfg_SubSort((yyvsp[(3) - (7)].string),(yyvsp[(5) - (7)].string)); }
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 419 "./SPASS/dfgparser.y"
    { dfg_SORTDECLLIST = list_Nconc(dfg_SORTDECLLIST,list_List(list_PairCreate(NULL,(yyvsp[(1) - (2)].term)))); }
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 421 "./SPASS/dfgparser.y"
    { string_StringFree((yyvsp[(3) - (7)].string)); }
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 422 "./SPASS/dfgparser.y"
    { dfg_VarStart(); }
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 423 "./SPASS/dfgparser.y"
    { dfg_VarStop();  }
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 424 "./SPASS/dfgparser.y"
    { TERM term;
  dfg_VarBacktrack();
  dfg_VarCheck();
  term = dfg_CreateQuantifier(fol_All(),(yyvsp[(5) - (11)].list),(yyvsp[(9) - (11)].term));
  dfg_SORTDECLLIST = list_Nconc(dfg_SORTDECLLIST,list_List(list_PairCreate(NULL,term)));
}
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 433 "./SPASS/dfgparser.y"
    { dfg_SymbolGenerated(dfg_Symbol((yyvsp[(2) - (9)].string),1), (yyvsp[(3) - (9)].bool), (yyvsp[(7) - (9)].list)); }
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 436 "./SPASS/dfgparser.y"
    { (yyval.bool) = FALSE; }
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 437 "./SPASS/dfgparser.y"
    { (yyval.bool) = TRUE; }
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 440 "./SPASS/dfgparser.y"
    { (yyval.list) = list_List((yyvsp[(1) - (1)].string)); }
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 441 "./SPASS/dfgparser.y"
    { (yyval.list) = list_Cons((yyvsp[(3) - (3)].string), (yyvsp[(1) - (3)].list)); }
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 444 "./SPASS/dfgparser.y"
    { string_StringFree((yyvsp[(1) - (1)].string)); }
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 445 "./SPASS/dfgparser.y"
    { string_StringFree((yyvsp[(3) - (3)].string)); }
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 454 "./SPASS/dfgparser.y"
    { list_NReverse((yyvsp[(6) - (8)].list));
		  if ((yyvsp[(3) - (8)].bool)) /* Axioms */
		    dfg_AXIOMLIST = list_Nconc(dfg_AXIOMLIST, (yyvsp[(6) - (8)].list));
		  else
		    dfg_CONJECLIST = list_Nconc(dfg_CONJECLIST, (yyvsp[(6) - (8)].list));
}
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 462 "./SPASS/dfgparser.y"
    { (yyval.bool) = TRUE;  }
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 463 "./SPASS/dfgparser.y"
    { (yyval.bool) = FALSE; }
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 470 "./SPASS/dfgparser.y"
    { (yyval.list) = list_Nil(); }
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 472 "./SPASS/dfgparser.y"
    { LIST pair;
  if ((yyvsp[(4) - (7)].term) == NULL) { /* No term */
    if ((yyvsp[(5) - (7)].string) != NULL)
      string_StringFree((yyvsp[(5) - (7)].string));
    (yyval.list) = (yyvsp[(1) - (7)].list);
  } else {
    pair = list_PairCreate((yyvsp[(5) - (7)].string), (yyvsp[(4) - (7)].term));
    (yyval.list) = list_Cons(pair, (yyvsp[(1) - (7)].list));
  }
  dfg_VarCheck();
}
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 485 "./SPASS/dfgparser.y"
    { (yyval.string) = NULL; }
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 486 "./SPASS/dfgparser.y"
    { (yyval.string) = (yyvsp[(2) - (2)].string);   }
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 489 "./SPASS/dfgparser.y"
    { (yyval.term) = (yyvsp[(1) - (1)].term); }
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 491 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(fol_Not(),list_List((yyvsp[(3) - (4)].term))); }
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 493 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create((yyvsp[(1) - (6)].symbol), list_Cons((yyvsp[(3) - (6)].term), list_List((yyvsp[(5) - (6)].term)))); }
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 495 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create((yyvsp[(1) - (4)].symbol), (yyvsp[(3) - (4)].list)); }
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 496 "./SPASS/dfgparser.y"
    { dfg_VarStart(); }
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 497 "./SPASS/dfgparser.y"
    { dfg_VarStop(); }
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 499 "./SPASS/dfgparser.y"
    { dfg_VarBacktrack();
  (yyval.term) = dfg_IGNORE ? NULL : dfg_CreateQuantifier((yyvsp[(1) - (10)].symbol),(yyvsp[(5) - (10)].list),(yyvsp[(9) - (10)].term));
}
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 504 "./SPASS/dfgparser.y"
    { (yyval.term) = NULL; }
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 505 "./SPASS/dfgparser.y"
    { (yyval.term) = (yyvsp[(1) - (1)].term); }
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 509 "./SPASS/dfgparser.y"
    { (yyval.list) = dfg_IGNORE ? list_Nil() : list_List((yyvsp[(1) - (1)].term)); }
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 511 "./SPASS/dfgparser.y"
    { (yyval.list) = dfg_IGNORE ? (yyvsp[(1) - (3)].list) : list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(3) - (3)].term))); }
    break;

  case 94:

/* Line 1806 of yacc.c  */
#line 514 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_Equiv();    }
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 515 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_Implied();  }
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 516 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_Implies();  }
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 517 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_Xor();      }
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 518 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_Nor();      }
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 519 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_Nand();     }
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 522 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_And(); }
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 523 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_Or();  }
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 526 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_Exist(); }
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 527 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_All(); }
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 530 "./SPASS/dfgparser.y"
    { if (dfg_IGNORE) {
		    string_StringFree((yyvsp[(1) - (1)].string));
		    (yyval.string) = NULL;
		  } else
					    (yyval.string) = (yyvsp[(1) - (1)].string);
}
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 537 "./SPASS/dfgparser.y"
    { (yyval.string) = dfg_IGNORE ? NULL : string_IntToString((yyvsp[(1) - (1)].number)); }
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 539 "./SPASS/dfgparser.y"
    { (yyval.string) = dfg_IGNORE ? NULL : string_StringCopy("set_flag"); }
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 541 "./SPASS/dfgparser.y"
    { (yyval.string) = dfg_IGNORE ? NULL : string_StringCopy("set_DomPred"); }
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 543 "./SPASS/dfgparser.y"
    { (yyval.string) = dfg_IGNORE ? NULL : string_StringCopy("set_precedence"); }
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 545 "./SPASS/dfgparser.y"
    { (yyval.string) = dfg_IGNORE ? NULL : string_StringCopy("set_ClauseFormulaRelation"); }
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 547 "./SPASS/dfgparser.y"
    { (yyval.string) = dfg_IGNORE ? NULL : string_StringCopy("set_selection"); }
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 551 "./SPASS/dfgparser.y"
    { (yyval.list) = dfg_IGNORE ? list_Nil() : list_List((yyvsp[(1) - (1)].term)); }
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 553 "./SPASS/dfgparser.y"
    { (yyval.list) = dfg_IGNORE ? (yyvsp[(1) - (3)].list) : list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(3) - (3)].term))); }
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 557 "./SPASS/dfgparser.y"
    { if (!dfg_IGNORE) {
    SYMBOL s = dfg_Symbol((yyvsp[(1) - (1)].string),0);
    if (!symbol_IsVariable(s)) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Symbol is not a variable.\n",dfg_LINENUMBER);
      misc_FinishUserErrorReport();
    }
    (yyval.term) = term_Create(s, list_Nil());
  }
}
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 568 "./SPASS/dfgparser.y"
    { if (!dfg_IGNORE) {
    SYMBOL p, v;
    p = dfg_Symbol((yyvsp[(1) - (4)].string), 1);
    if (!symbol_IsPredicate(p)) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Symbol is not a predicate.\n",dfg_LINENUMBER);
      misc_FinishUserErrorReport();
    }
    v = dfg_Symbol((yyvsp[(3) - (4)].string), 0);
    if (!symbol_IsVariable(v)) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Symbol is not a variable.\n",dfg_LINENUMBER);
      misc_FinishUserErrorReport();
    }
    (yyval.term) = term_Create(p, list_List(term_Create(v,list_Nil())));
  }
}
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 593 "./SPASS/dfgparser.y"
    { list_NReverse((yyvsp[(8) - (10)].list));
			  if ((yyvsp[(3) - (10)].bool)) /* Axioms */
			    dfg_AXIOMLIST = list_Nconc(dfg_AXIOMLIST, (yyvsp[(8) - (10)].list));
			  else
			    dfg_CONJECLIST = list_Nconc(dfg_CONJECLIST, (yyvsp[(8) - (10)].list));
			  flag_SetFlagIntValue(dfg_FLAGS, flag_EML, flag_EMLON);
}
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 606 "./SPASS/dfgparser.y"
    { (yyval.list) = list_Nil(); }
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 609 "./SPASS/dfgparser.y"
    { LIST pair;
  if ((yyvsp[(4) - (7)].term) == NULL) { /* No term */
    if ((yyvsp[(5) - (7)].string) != NULL)
      string_StringFree((yyvsp[(5) - (7)].string));
    (yyval.list) = (yyvsp[(1) - (7)].list);
  } else {
    pair = list_PairCreate((yyvsp[(5) - (7)].string), (yyvsp[(4) - (7)].term));
    (yyval.list) = list_Cons(pair, (yyvsp[(1) - (7)].list));
  }
  dfg_VarCheck();
}
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 622 "./SPASS/dfgparser.y"
    { LIST pair;
  if ((yyvsp[(4) - (7)].term) == NULL) { /* No term */
    if ((yyvsp[(5) - (7)].string) != NULL)
      string_StringFree((yyvsp[(5) - (7)].string));
    (yyval.list) = (yyvsp[(1) - (7)].list);
  } else {
    pair = list_PairCreate((yyvsp[(5) - (7)].string), (yyvsp[(4) - (7)].term));
    (yyval.list) = list_Nconc((yyvsp[(1) - (7)].list), list_List(pair));
  }
}
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 634 "./SPASS/dfgparser.y"
    { LIST pair;
  if ((yyvsp[(4) - (7)].term) == NULL) { /* No term */
    if ((yyvsp[(5) - (7)].string) != NULL)
      string_StringFree((yyvsp[(5) - (7)].string));
    (yyval.list) = (yyvsp[(1) - (7)].list);
  } else {
    pair = list_PairCreate((yyvsp[(5) - (7)].string), (yyvsp[(4) - (7)].term));
    (yyval.list) = list_Nconc((yyvsp[(1) - (7)].list), list_List(pair));
  }
}
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 648 "./SPASS/dfgparser.y"
    { (yyval.term) = NULL; }
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 649 "./SPASS/dfgparser.y"
    { (yyval.term) = (yyvsp[(1) - (1)].term); }
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 652 "./SPASS/dfgparser.y"
    { (yyval.term) = NULL; }
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 653 "./SPASS/dfgparser.y"
    { (yyval.term) = term_Create(eml_Box(), 
				   list_Cons(term_Create(fol_Not(), list_List((yyvsp[(1) - (1)].term))), 
					     list_List(term_Create(fol_False(), list_Nil())))); 
    /* since r = 1 iff (r':1)' = 1 */
}
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 660 "./SPASS/dfgparser.y"
    { (yyval.list) = list_List((yyvsp[(1) - (1)].term)); }
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 661 "./SPASS/dfgparser.y"
    { (yyval.list) = list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(3) - (3)].term))); }
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 664 "./SPASS/dfgparser.y"
    { (yyval.list) = list_List((yyvsp[(1) - (1)].term)); }
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 665 "./SPASS/dfgparser.y"
    { (yyval.list) = list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(3) - (3)].term))); }
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 669 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : dfg_AtomCreate((yyvsp[(1) - (1)].string), list_Nil()); }
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 671 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(fol_True(),list_Nil()); }
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 673 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(fol_False(),list_Nil()); }
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 675 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(fol_Not(),list_List((yyvsp[(3) - (4)].term))); }
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 677 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(eml_Domain(),list_List((yyvsp[(3) - (4)].term))); }
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 679 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(eml_Range(),list_List((yyvsp[(3) - (4)].term))); }
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 681 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create((yyvsp[(1) - (6)].symbol), list_Cons((yyvsp[(3) - (6)].term), list_List((yyvsp[(5) - (6)].term)))); }
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 683 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create((yyvsp[(1) - (4)].symbol), (yyvsp[(3) - (4)].list)); }
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 685 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create((yyvsp[(1) - (6)].symbol), list_Cons((yyvsp[(3) - (6)].term), list_List((yyvsp[(5) - (6)].term)))); }
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 689 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : dfg_AtomCreate((yyvsp[(1) - (1)].string), list_Nil()); }
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 691 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(fol_True(),list_Nil()); }
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 693 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(fol_False(),list_Nil()); }
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 695 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(fol_Not(),list_List((yyvsp[(3) - (4)].term))); }
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 697 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(eml_Id(),list_Nil()); }
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 699 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(eml_Div(),list_Nil()); }
    break;

  case 145:

/* Line 1806 of yacc.c  */
#line 701 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(eml_Conv(),list_List((yyvsp[(3) - (4)].term))); }
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 703 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(eml_Test(),list_List((yyvsp[(3) - (4)].term))); }
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 705 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(eml_DomRestr(), list_Cons((yyvsp[(3) - (6)].term), list_List((yyvsp[(5) - (6)].term)))); }
    break;

  case 148:

/* Line 1806 of yacc.c  */
#line 707 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(eml_RanRestr(), list_Cons((yyvsp[(3) - (6)].term), list_List((yyvsp[(5) - (6)].term)))); }
    break;

  case 149:

/* Line 1806 of yacc.c  */
#line 709 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create((yyvsp[(1) - (6)].symbol), list_Cons((yyvsp[(3) - (6)].term), list_List((yyvsp[(5) - (6)].term)))); }
    break;

  case 150:

/* Line 1806 of yacc.c  */
#line 711 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create((yyvsp[(1) - (4)].symbol), (yyvsp[(3) - (4)].list)); }
    break;

  case 151:

/* Line 1806 of yacc.c  */
#line 714 "./SPASS/dfgparser.y"
    { (yyval.symbol) = eml_Box();  }
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 715 "./SPASS/dfgparser.y"
    { (yyval.symbol) = eml_Dia();  }
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 718 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_And();  }
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 719 "./SPASS/dfgparser.y"
    { (yyval.symbol) = fol_Or();   }
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 720 "./SPASS/dfgparser.y"
    { (yyval.symbol) = eml_Comp(); }
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 721 "./SPASS/dfgparser.y"
    { (yyval.symbol) = eml_Sum();  }
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 736 "./SPASS/dfgparser.y"
    { list_NReverse((yyvsp[(8) - (10)].list));
  if (!list_Empty(dfg_TEMPPLAINCLAUSES)) {
    /* Plain clauses are temporarily stored in the list dfg_TEMPPLAINCLAUSES.
       They need to be concatenated to their respective axiom or conjecture 
       plain clause lists. */
    if ((yyvsp[(3) - (10)].bool)) /* Axioms */
      dfg_PLAINAXCLAUSES = list_Nconc(dfg_PLAINAXCLAUSES, list_NReverse(dfg_TEMPPLAINCLAUSES));
    else
      dfg_PLAINCONCLAUSES = list_Nconc(dfg_PLAINCONCLAUSES, list_NReverse(dfg_TEMPPLAINCLAUSES));
    dfg_TEMPPLAINCLAUSES = list_Nil();
  }
  if (!list_Empty((yyvsp[(8) - (10)].list))) {
    if ((yyvsp[(3) - (10)].bool)) /* Axioms */
      dfg_AXCLAUSES = list_Nconc(dfg_AXCLAUSES, (yyvsp[(8) - (10)].list));
    else
      dfg_CONCLAUSES = list_Nconc(dfg_CONCLAUSES, (yyvsp[(8) - (10)].list));
  }
}
    break;

  case 160:

/* Line 1806 of yacc.c  */
#line 755 "./SPASS/dfgparser.y"
    { stack_Push((POINTER)dfg_IGNORE); dfg_IGNORE = TRUE; }
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 758 "./SPASS/dfgparser.y"
    { dfg_IGNORE = (BOOL)stack_PopResult(); }
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 761 "./SPASS/dfgparser.y"
    { (yyval.list) = list_Nil(); }
    break;

  case 163:

/* Line 1806 of yacc.c  */
#line 763 "./SPASS/dfgparser.y"
    { LIST pair;
  if ((yyvsp[(4) - (7)].term) == NULL) { /* No clause */
    if ((yyvsp[(5) - (7)].string) != NULL)
      string_StringFree((yyvsp[(5) - (7)].string));
    (yyval.list) = (yyvsp[(1) - (7)].list);
  } else {
    pair = list_PairCreate((yyvsp[(5) - (7)].string), (yyvsp[(4) - (7)].term));
    (yyval.list) = list_Cons(pair, (yyvsp[(1) - (7)].list));
  }
  dfg_VarCheck();
}
    break;

  case 164:

/* Line 1806 of yacc.c  */
#line 776 "./SPASS/dfgparser.y"
    { (yyval.term) = NULL; }
    break;

  case 165:

/* Line 1806 of yacc.c  */
#line 777 "./SPASS/dfgparser.y"
    { (yyval.term) = (yyvsp[(1) - (1)].term); }
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 778 "./SPASS/dfgparser.y"
    {dfg_VarStart();}
    break;

  case 167:

/* Line 1806 of yacc.c  */
#line 778 "./SPASS/dfgparser.y"
    {dfg_VarStop();}
    break;

  case 168:

/* Line 1806 of yacc.c  */
#line 779 "./SPASS/dfgparser.y"
    { dfg_VarBacktrack();
  if (dfg_IGNORE) 
    (yyval.term) = NULL;
  else {
    /* C || A -> S */
    DFG_PLAINCLAUSE clause = dfg_PlainClauseCreate();
    clause->constraint = (yyvsp[(2) - (7)].list);
    clause->antecedent = (yyvsp[(4) - (7)].list);
    clause->succedent = (yyvsp[(6) - (7)].list);
    clause->selected = dfg_SELECTED_LITERAL;
    dfg_SELECTED_LITERAL = (TERM) NULL;
    dfg_TEMPPLAINCLAUSES = list_Cons(clause,dfg_TEMPPLAINCLAUSES);
    (yyval.term) = NULL;
  }
}
    break;

  case 169:

/* Line 1806 of yacc.c  */
#line 796 "./SPASS/dfgparser.y"
    { (yyval.term) = (yyvsp[(1) - (1)].term); }
    break;

  case 170:

/* Line 1806 of yacc.c  */
#line 797 "./SPASS/dfgparser.y"
    { dfg_VarStart(); }
    break;

  case 171:

/* Line 1806 of yacc.c  */
#line 798 "./SPASS/dfgparser.y"
    { dfg_VarStop();  }
    break;

  case 172:

/* Line 1806 of yacc.c  */
#line 800 "./SPASS/dfgparser.y"
    { dfg_VarBacktrack();
  (yyval.term) = dfg_IGNORE ? NULL : dfg_CreateQuantifier(fol_All(),(yyvsp[(5) - (10)].list),(yyvsp[(9) - (10)].term));
}
    break;

  case 173:

/* Line 1806 of yacc.c  */
#line 806 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(fol_Or(), (yyvsp[(3) - (4)].list)); }
    break;

  case 174:

/* Line 1806 of yacc.c  */
#line 809 "./SPASS/dfgparser.y"
    {(yyval.list) = list_Nil();}
    break;

  case 175:

/* Line 1806 of yacc.c  */
#line 811 "./SPASS/dfgparser.y"
    { (yyval.list) = dfg_IGNORE ? list_Nil() : list_List((yyvsp[(1) - (1)].term)); }
    break;

  case 176:

/* Line 1806 of yacc.c  */
#line 813 "./SPASS/dfgparser.y"
    { (yyval.list) = dfg_IGNORE ? (yyvsp[(1) - (3)].list) : list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(3) - (3)].term))); }
    break;

  case 177:

/* Line 1806 of yacc.c  */
#line 816 "./SPASS/dfgparser.y"
    {(yyval.list) = list_Nil();}
    break;

  case 178:

/* Line 1806 of yacc.c  */
#line 818 "./SPASS/dfgparser.y"
    { (yyval.list) = dfg_IGNORE ? (yyvsp[(1) - (2)].list) : list_Nconc((yyvsp[(1) - (2)].list), list_List((yyvsp[(2) - (2)].term))); }
    break;

  case 179:

/* Line 1806 of yacc.c  */
#line 821 "./SPASS/dfgparser.y"
    {(yyval.list) = list_Nil();}
    break;

  case 180:

/* Line 1806 of yacc.c  */
#line 823 "./SPASS/dfgparser.y"
    { (yyval.list) = dfg_IGNORE ? (yyvsp[(1) - (2)].list) : list_Nconc((yyvsp[(1) - (2)].list), list_List((yyvsp[(2) - (2)].term))); }
    break;

  case 181:

/* Line 1806 of yacc.c  */
#line 825 "./SPASS/dfgparser.y"
    { if (dfg_IGNORE)
    (yyval.list) = (yyvsp[(1) - (3)].list);
  else {
    if(dfg_SELECTED_LITERAL != NULL) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Trying to select two literals in a clause.");
      misc_FinishUserErrorReport();
    } 
    dfg_SELECTED_LITERAL = (yyvsp[(2) - (3)].term);
    (yyval.list) = list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(2) - (3)].term)));
  } 
}
    break;

  case 182:

/* Line 1806 of yacc.c  */
#line 839 "./SPASS/dfgparser.y"
    { (yyval.term) = (yyvsp[(1) - (1)].term); }
    break;

  case 183:

/* Line 1806 of yacc.c  */
#line 841 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? (yyvsp[(3) - (4)].term) : term_Create(fol_Not(),list_List((yyvsp[(3) - (4)].term))); }
    break;

  case 184:

/* Line 1806 of yacc.c  */
#line 844 "./SPASS/dfgparser.y"
    { (yyval.list) = list_List((yyvsp[(1) - (1)].term)); }
    break;

  case 185:

/* Line 1806 of yacc.c  */
#line 845 "./SPASS/dfgparser.y"
    { (yyval.list) = list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(3) - (3)].term))); }
    break;

  case 186:

/* Line 1806 of yacc.c  */
#line 849 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : dfg_AtomCreate((yyvsp[(1) - (1)].string),list_Nil()); }
    break;

  case 187:

/* Line 1806 of yacc.c  */
#line 851 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(fol_True(),list_Nil()); }
    break;

  case 188:

/* Line 1806 of yacc.c  */
#line 853 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(fol_False(),list_Nil()); }
    break;

  case 189:

/* Line 1806 of yacc.c  */
#line 855 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : term_Create(fol_Equality(),list_Cons((yyvsp[(3) - (6)].term),list_List((yyvsp[(5) - (6)].term)))); }
    break;

  case 190:

/* Line 1806 of yacc.c  */
#line 857 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : dfg_AtomCreate((yyvsp[(1) - (4)].string), (yyvsp[(3) - (4)].list)); }
    break;

  case 198:

/* Line 1806 of yacc.c  */
#line 883 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : dfg_TermCreate((yyvsp[(1) - (1)].string),list_Nil()); }
    break;

  case 199:

/* Line 1806 of yacc.c  */
#line 885 "./SPASS/dfgparser.y"
    { (yyval.term) = dfg_IGNORE ? NULL : dfg_TermCreate((yyvsp[(1) - (4)].string), (yyvsp[(3) - (4)].list)); }
    break;

  case 200:

/* Line 1806 of yacc.c  */
#line 889 "./SPASS/dfgparser.y"
    { (yyval.list) = dfg_IGNORE ? list_Nil() : list_List((yyvsp[(1) - (1)].term)); }
    break;

  case 201:

/* Line 1806 of yacc.c  */
#line 891 "./SPASS/dfgparser.y"
    { (yyval.list) = dfg_IGNORE ? (yyvsp[(1) - (3)].list) : list_Nconc((yyvsp[(1) - (3)].list),list_List((yyvsp[(3) - (3)].term))); }
    break;

  case 204:

/* Line 1806 of yacc.c  */
#line 903 "./SPASS/dfgparser.y"
    { if (!string_Equal((yyvsp[(3) - (5)].string),"SPASS")) {
    stack_Push((POINTER)dfg_IGNORE);
    dfg_IGNORE = TRUE;
  }
}
    break;

  case 205:

/* Line 1806 of yacc.c  */
#line 910 "./SPASS/dfgparser.y"
    { if (!string_Equal((yyvsp[(3) - (9)].string),"SPASS"))
    dfg_IGNORE = (BOOL)stack_PopResult();
  string_StringFree((yyvsp[(3) - (9)].string));
}
    break;

  case 207:

/* Line 1806 of yacc.c  */
#line 919 "./SPASS/dfgparser.y"
    { if (!dfg_IGNORE && (yyvsp[(4) - (15)].string)!=NULL && (yyvsp[(6) - (15)].term)!=NULL && !list_Empty((yyvsp[(11) - (15)].list))) {
    LIST tupel;
    RULE Rule = clause_GetOriginFromString((yyvsp[(8) - (15)].string));
    string_StringFree((yyvsp[(8) - (15)].string));
    /* Build a tuple (label,clause,parentlist,split level,origin) */
    tupel = list_Cons((POINTER)(yyvsp[(13) - (15)].number),list_List((POINTER)Rule));
    tupel = list_Cons((yyvsp[(4) - (15)].string),list_Cons((yyvsp[(6) - (15)].term),list_Cons((yyvsp[(11) - (15)].list),tupel)));
    dfg_PROOFLIST = list_Cons(tupel, dfg_PROOFLIST);
  } else {
    /* ignore DNF clauses and clauses with incomplete data */
    if ((yyvsp[(4) - (15)].string) != NULL) string_StringFree((yyvsp[(4) - (15)].string));
    if ((yyvsp[(6) - (15)].term) != NULL) term_Delete((yyvsp[(6) - (15)].term));
    if ((yyvsp[(8) - (15)].string) != NULL) string_StringFree((yyvsp[(8) - (15)].string));
    dfg_DeleteStringList((yyvsp[(11) - (15)].list));
  }
  dfg_VarCheck();
}
    break;

  case 208:

/* Line 1806 of yacc.c  */
#line 939 "./SPASS/dfgparser.y"
    { (yyval.list) = (dfg_IGNORE||(yyvsp[(1) - (1)].string)==NULL) ? list_Nil() : list_List((yyvsp[(1) - (1)].string)); }
    break;

  case 209:

/* Line 1806 of yacc.c  */
#line 941 "./SPASS/dfgparser.y"
    { (yyval.list) = (dfg_IGNORE||(yyvsp[(3) - (3)].string)==NULL) ? (yyvsp[(1) - (3)].list) : list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(3) - (3)].string))); }
    break;

  case 210:

/* Line 1806 of yacc.c  */
#line 945 "./SPASS/dfgparser.y"
    { (yyval.number) = 0; }
    break;

  case 211:

/* Line 1806 of yacc.c  */
#line 946 "./SPASS/dfgparser.y"
    { (yyval.number) = (yyvsp[(3) - (4)].number); }
    break;

  case 212:

/* Line 1806 of yacc.c  */
#line 950 "./SPASS/dfgparser.y"
    { if (!dfg_IGNORE && (yyvsp[(1) - (3)].string)!=NULL && (yyvsp[(3) - (3)].string)!=NULL && string_Equal((yyvsp[(1) - (3)].string),"splitlevel"))
    string_StringToInt((yyvsp[(3) - (3)].string), TRUE, &(yyval.number));
  else
    (yyval.number) = 0;
  if ((yyvsp[(1) - (3)].string) != NULL) string_StringFree((yyvsp[(1) - (3)].string));
  if ((yyvsp[(3) - (3)].string) != NULL) string_StringFree((yyvsp[(3) - (3)].string));
}
    break;

  case 213:

/* Line 1806 of yacc.c  */
#line 958 "./SPASS/dfgparser.y"
    { if (!dfg_IGNORE && (yyvsp[(3) - (5)].string)!=NULL && (yyvsp[(5) - (5)].string)!=NULL && string_Equal((yyvsp[(3) - (5)].string),"splitlevel"))
    string_StringToInt((yyvsp[(5) - (5)].string), TRUE, &(yyval.number));
  else
    (yyval.number) = (yyvsp[(1) - (5)].number);
  if ((yyvsp[(3) - (5)].string) != NULL) string_StringFree((yyvsp[(3) - (5)].string));
  if ((yyvsp[(5) - (5)].string) != NULL) string_StringFree((yyvsp[(5) - (5)].string));
}
    break;

  case 214:

/* Line 1806 of yacc.c  */
#line 968 "./SPASS/dfgparser.y"
    { stack_Push((POINTER) dfg_IGNORE); dfg_IGNORE = TRUE; }
    break;

  case 215:

/* Line 1806 of yacc.c  */
#line 970 "./SPASS/dfgparser.y"
    { dfg_IGNORE = (BOOL) stack_PopResult();
  if ((yyvsp[(3) - (3)].bool)) {
    if ((yyvsp[(1) - (3)].string) != NULL) string_StringFree((yyvsp[(1) - (3)].string));
    (yyval.string) = NULL;
  } else
    (yyval.string) = (yyvsp[(1) - (3)].string);
}
    break;

  case 216:

/* Line 1806 of yacc.c  */
#line 979 "./SPASS/dfgparser.y"
    { (yyval.string) = (yyvsp[(1) - (1)].string); }
    break;

  case 217:

/* Line 1806 of yacc.c  */
#line 980 "./SPASS/dfgparser.y"
    { (yyval.string) = NULL; }
    break;

  case 218:

/* Line 1806 of yacc.c  */
#line 981 "./SPASS/dfgparser.y"
    { (yyval.string) = NULL; }
    break;

  case 219:

/* Line 1806 of yacc.c  */
#line 982 "./SPASS/dfgparser.y"
    { (yyval.string) = NULL; }
    break;

  case 220:

/* Line 1806 of yacc.c  */
#line 983 "./SPASS/dfgparser.y"
    { (yyval.string) = NULL; }
    break;

  case 221:

/* Line 1806 of yacc.c  */
#line 984 "./SPASS/dfgparser.y"
    { (yyval.string) = NULL; }
    break;

  case 222:

/* Line 1806 of yacc.c  */
#line 985 "./SPASS/dfgparser.y"
    { (yyval.string) = NULL; }
    break;

  case 223:

/* Line 1806 of yacc.c  */
#line 986 "./SPASS/dfgparser.y"
    { (yyval.string) = NULL; }
    break;

  case 224:

/* Line 1806 of yacc.c  */
#line 987 "./SPASS/dfgparser.y"
    { (yyval.string) = NULL; }
    break;

  case 225:

/* Line 1806 of yacc.c  */
#line 990 "./SPASS/dfgparser.y"
    { (yyval.bool) = FALSE; }
    break;

  case 226:

/* Line 1806 of yacc.c  */
#line 991 "./SPASS/dfgparser.y"
    { (yyval.bool) = TRUE; }
    break;

  case 227:

/* Line 1806 of yacc.c  */
#line 992 "./SPASS/dfgparser.y"
    { (yyval.bool) = TRUE; }
    break;

  case 228:

/* Line 1806 of yacc.c  */
#line 995 "./SPASS/dfgparser.y"
    { (yyval.term) = (yyvsp[(1) - (1)].term);   }
    break;

  case 229:

/* Line 1806 of yacc.c  */
#line 996 "./SPASS/dfgparser.y"
    { (yyval.term) = NULL; }
    break;

  case 232:

/* Line 1806 of yacc.c  */
#line 1008 "./SPASS/dfgparser.y"
    { dfg_VarStart(); }
    break;

  case 233:

/* Line 1806 of yacc.c  */
#line 1009 "./SPASS/dfgparser.y"
    {
  dfg_VarStop();
  dfg_VarBacktrack();
  dfg_VarCheck(); }
    break;

  case 235:

/* Line 1806 of yacc.c  */
#line 1016 "./SPASS/dfgparser.y"
    { dfg_TERMLIST = list_Nconc(dfg_TERMLIST, list_List((yyvsp[(2) - (3)].term))); }
    break;

  case 238:

/* Line 1806 of yacc.c  */
#line 1027 "./SPASS/dfgparser.y"
    { dfg_INCLUDELIST = list_Nconc(dfg_INCLUDELIST,(yyvsp[(3) - (5)].list)); }
    break;

  case 239:

/* Line 1806 of yacc.c  */
#line 1030 "./SPASS/dfgparser.y"
    { (yyval.list) = list_Nil(); }
    break;

  case 240:

/* Line 1806 of yacc.c  */
#line 1031 "./SPASS/dfgparser.y"
    { (yyval.list) = list_Cons((yyvsp[(2) - (2)].list),(yyvsp[(1) - (2)].list)); }
    break;

  case 241:

/* Line 1806 of yacc.c  */
#line 1034 "./SPASS/dfgparser.y"
    { (yyval.list) = list_PairCreate((yyvsp[(3) - (6)].string),(yyvsp[(4) - (6)].list)); }
    break;

  case 242:

/* Line 1806 of yacc.c  */
#line 1037 "./SPASS/dfgparser.y"
    { (yyval.list) = (yyvsp[(3) - (4)].list); }
    break;

  case 243:

/* Line 1806 of yacc.c  */
#line 1038 "./SPASS/dfgparser.y"
    { (yyval.list) = list_Nil(); }
    break;

  case 244:

/* Line 1806 of yacc.c  */
#line 1041 "./SPASS/dfgparser.y"
    { (yyval.list) = list_List((yyvsp[(1) - (1)].string)); }
    break;

  case 245:

/* Line 1806 of yacc.c  */
#line 1042 "./SPASS/dfgparser.y"
    {
  (yyval.list) = list_Cons((yyvsp[(1) - (3)].string),(yyvsp[(3) - (3)].list));
}
    break;

  case 246:

/* Line 1806 of yacc.c  */
#line 1047 "./SPASS/dfgparser.y"
    { (yyval.string) = dfg_StripQuotes((yyvsp[(1) - (1)].string)); }
    break;

  case 248:

/* Line 1806 of yacc.c  */
#line 1055 "./SPASS/dfgparser.y"
    {
  if (dfg_IGNORESETTINGS) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Settings not allowed in included files\n");			
    misc_FinishUserErrorReport();
  }
}
    break;

  case 250:

/* Line 1806 of yacc.c  */
#line 1065 "./SPASS/dfgparser.y"
    { if (string_Equal((yyvsp[(3) - (3)].string),"SPASS"))
		    dfg_IGNORETEXT = FALSE;
		  string_StringFree((yyvsp[(3) - (3)].string));
}
    break;

  case 251:

/* Line 1806 of yacc.c  */
#line 1070 "./SPASS/dfgparser.y"
    { dfg_IGNORETEXT = TRUE; }
    break;

  case 252:

/* Line 1806 of yacc.c  */
#line 1073 "./SPASS/dfgparser.y"
    { /* no SPASS flags */
  string_StringFree((yyvsp[(1) - (1)].string));
}
    break;

  case 259:

/* Line 1806 of yacc.c  */
#line 1087 "./SPASS/dfgparser.y"
    { SYMBOL s;
  for ( ; !list_Empty((yyvsp[(3) - (4)].list)); (yyvsp[(3) - (4)].list) = list_Pop((yyvsp[(3) - (4)].list))) {
    s = symbol_Lookup(list_Car((yyvsp[(3) - (4)].list)));
    if (s == 0) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Undefined symbol %s", list_Car((yyvsp[(3) - (4)].list)));
      misc_UserErrorReport(" in DomPred list.\n"); 
      misc_FinishUserErrorReport(); 
    }
    if (!symbol_IsPredicate(s)) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Symbol %s isn't a predicate", list_Car((yyvsp[(3) - (4)].list)));
      misc_UserErrorReport(" in DomPred list.\n");
      misc_FinishUserErrorReport();
    }
    string_StringFree(list_Car((yyvsp[(3) - (4)].list))); 
    symbol_AddProperty(s, DOMPRED);
  }
}
    break;

  case 260:

/* Line 1806 of yacc.c  */
#line 1107 "./SPASS/dfgparser.y"
    { int flag = flag_Id((yyvsp[(3) - (6)].string));
  if (flag == -1) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Found unknown flag %s", (yyvsp[(3) - (6)].string));
    misc_FinishUserErrorReport();
  }
  string_StringFree((yyvsp[(3) - (6)].string));
  flag_SetFlagIntValue(dfg_FLAGS, flag, (yyvsp[(5) - (6)].number));
}
    break;

  case 263:

/* Line 1806 of yacc.c  */
#line 1123 "./SPASS/dfgparser.y"
    { SYMBOL s = symbol_Lookup((yyvsp[(1) - (1)].string));
  if (s == 0) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Undefined symbol %s ", (yyvsp[(1) - (1)].string));
    misc_UserErrorReport(" in precedence list.\n"); 
    misc_FinishUserErrorReport(); 
  }
  string_StringFree((yyvsp[(1) - (1)].string));
  symbol_SetIncreasedOrdering(dfg_PRECEDENCE, s);
  dfg_USERPRECEDENCE = list_Cons((POINTER)s, dfg_USERPRECEDENCE);
}
    break;

  case 264:

/* Line 1806 of yacc.c  */
#line 1135 "./SPASS/dfgparser.y"
    { SYMBOL s = symbol_Lookup((yyvsp[(2) - (6)].string));
  if (s == 0) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Undefined symbol %s", (yyvsp[(2) - (6)].string));
    misc_UserErrorReport("in precedence list.\n");
    misc_FinishUserErrorReport(); 
  }
  string_StringFree((yyvsp[(2) - (6)].string));
  symbol_SetIncreasedOrdering(dfg_PRECEDENCE, s);
  dfg_USERPRECEDENCE = list_Cons((POINTER)s, dfg_USERPRECEDENCE);
  symbol_SetWeight(s, (yyvsp[(4) - (6)].number));
  if ((yyvsp[(5) - (6)].property) != 0)
    symbol_AddProperty(s, (yyvsp[(5) - (6)].property));
}
    break;

  case 267:

/* Line 1806 of yacc.c  */
#line 1156 "./SPASS/dfgparser.y"
    {
  dfg_CLAXRELATION = list_Cons(list_Cons((POINTER)(yyvsp[(2) - (5)].number), dfg_CLAXAXIOMS), dfg_CLAXRELATION);
  dfg_CLAXAXIOMS   = list_Nil();
}
    break;

  case 270:

/* Line 1806 of yacc.c  */
#line 1167 "./SPASS/dfgparser.y"
    {
  dfg_CLAXAXIOMS = list_Cons((POINTER)(yyvsp[(1) - (1)].string), dfg_CLAXAXIOMS);
}
    break;

  case 273:

/* Line 1806 of yacc.c  */
#line 1177 "./SPASS/dfgparser.y"
    { SYMBOL s = symbol_Lookup((yyvsp[(1) - (1)].string));
  if (s == 0) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Undefined symbol %s ", (yyvsp[(1) - (1)].string));
    misc_UserErrorReport(" in selection list.\n"); 
    misc_FinishUserErrorReport(); 
  }
  if (!symbol_IsPredicate(s)) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Symbol %s isn't a predicate", (yyvsp[(1) - (1)].string));
    misc_UserErrorReport(" in selection list.\n");
    misc_FinishUserErrorReport();
  }
  string_StringFree((yyvsp[(1) - (1)].string));
  dfg_USERSELECTION = list_Cons((POINTER)s, dfg_USERSELECTION);
}
    break;

  case 274:

/* Line 1806 of yacc.c  */
#line 1195 "./SPASS/dfgparser.y"
    { (yyval.property) = 0; /* left */ }
    break;

  case 275:

/* Line 1806 of yacc.c  */
#line 1197 "./SPASS/dfgparser.y"
    { if ((yyvsp[(2) - (2)].string)[1] != '\0' ||
      ((yyvsp[(2) - (2)].string)[0]!='l' && (yyvsp[(2) - (2)].string)[0]!='m' && (yyvsp[(2) - (2)].string)[0]!='r')) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Invalid symbol status %s", (yyvsp[(2) - (2)].string));
    misc_UserErrorReport(" in precedence list.");
    misc_FinishUserErrorReport();
  }
  switch ((yyvsp[(2) - (2)].string)[0]) {
  case 'm': (yyval.property) = ORDMUL;   break;
  case 'r': (yyval.property) = ORDRIGHT; break;
  default:  (yyval.property) = 0;
  }
  string_StringFree((yyvsp[(2) - (2)].string));
}
    break;

  case 278:

/* Line 1806 of yacc.c  */
#line 1218 "./SPASS/dfgparser.y"
    { dfg_DeleteStringList((yyvsp[(3) - (5)].list)); }
    break;

  case 279:

/* Line 1806 of yacc.c  */
#line 1221 "./SPASS/dfgparser.y"
    { (yyval.list) = list_List((yyvsp[(1) - (1)].string)); }
    break;

  case 280:

/* Line 1806 of yacc.c  */
#line 1222 "./SPASS/dfgparser.y"
    { (yyval.list) = list_Nconc((yyvsp[(1) - (3)].list), list_List((yyvsp[(3) - (3)].string))); }
    break;



/* Line 1806 of yacc.c  */
#line 3964 "SPASS/dfgparser.c"
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
#line 1225 "./SPASS/dfgparser.y"


void yyerror(const char *s)
{
  misc_StartUserErrorReport();
  misc_UserErrorReport("\n Line %i: %s\n", dfg_LINENUMBER, s);
  misc_FinishUserErrorReport();
}

static void dfg_Init(FILE* Input, FLAGSTORE Flags, PRECEDENCE Precedence, DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   The input file stream for the parser, a flag store,
           a precedence and a problem description store.
  RETURNS: Nothing.
  EFFECT:  The parser and scanner are initialized.
           The parser will use the flag store and the precedence
	   to memorize the settings from the input file.
***************************************************************/
{
  extern FILE* dfg_in;  /* declared in dfgscanner */

  dfg_in               = Input;
  dfg_LINENUMBER       = 1;
  dfg_IGNORETEXT       = TRUE;
  dfg_AXIOMLIST        = list_Nil();
  dfg_CONJECLIST       = list_Nil();
  dfg_INCLUDELIST      = list_Nil();
  dfg_SORTDECLLIST     = list_Nil();
  dfg_USERPRECEDENCE   = list_Nil();
  dfg_USERSELECTION    = list_Nil();
  dfg_CLAXRELATION     = list_Nil();
  dfg_CLAXAXIOMS       = list_Nil();
  dfg_AXCLAUSES        = list_Nil();
  dfg_CONCLAUSES       = list_Nil();
  dfg_PROOFLIST        = list_Nil();
  dfg_TERMLIST         = list_Nil();
  dfg_SYMBOLLIST       = list_Nil();
  dfg_VARLIST          = list_Nil();
  dfg_PLAINAXCLAUSES   = list_Nil();
  dfg_PLAINCONCLAUSES  = list_Nil();
  dfg_TEMPPLAINCLAUSES = list_Nil();
  dfg_SELECTED_LITERAL = (TERM) NULL;
  dfg_VARDECL          = FALSE;
  dfg_IGNORE           = FALSE;
  dfg_FLAGS            = Flags;
  dfg_PRECEDENCE       = Precedence;
  dfg_DESC             = Description;
}

void dfg_DeleteClAxRelation(LIST ClAxRelation)
{
  LIST Scan1, Scan2;

  for (Scan1=ClAxRelation;!list_Empty(Scan1);Scan1=list_Cdr(Scan1)) {
    for (Scan2=list_Cdr(list_Car(Scan1));!list_Empty(Scan2);Scan2=list_Cdr(Scan2))
      string_StringFree((char *)list_Car(Scan2));
    list_Delete(list_Car(Scan1));
  }
  list_Delete(ClAxRelation);
}

FILE* dfg_OpenFile(const char * FileName, char ** const DiscoveredName)
/**************************************************************
  INPUT: A string filename, and a pointer to char pointer
           to hold the fullpath name of the file 
           that was opened in the end (can be null).
  RETURNS: A opened file.
  EFFECT:
  Opens a file for reading.
  The file is searched for using the extended search mechanism
  wrt SPASSINPUT environment variable.  
***************************************************************/
{  
  return misc_OpenFileEnv(FileName,"r","SPASSINPUT",DiscoveredName);
}

static LIST dfg_TestList; /* list of strings used in nonmember tests */
	
static BOOL dfg_LabelFormulaPairIsNonmember(POINTER pair) {
  if (list_PairFirst(pair) == NULL)
    return TRUE;
	
  return !list_Member(dfg_TestList,list_PairFirst(pair),(BOOL (*)(POINTER, POINTER))string_Equal);
}

static void dfg_DeleteLabelTermPair(POINTER pair)	{
  term_Delete(list_PairSecond(pair));
  if (list_PairFirst(pair) != NULL)
    string_StringFree(list_PairFirst(pair));  /* Free the label */
  list_PairFree(pair);
}

void dfg_FilterClausesBySelection(LIST* Clauses, LIST* ClAxRelation, LIST selection)
/**************************************************************
  INPUT: A pointer to a clause list, a pointer to a ClAxRelation list
         and a char* list.
  RETURNS: Nothing.
  EFFECT: The clause list is filtered in such a way that clauses
          with names not in the selection list are deleted.
          The name for the clauses is found in the ClAxRelation structure.
          And the corresponding entry in ClAxRelation is also 
          deleted if the clause is.
          The clause list and the ClAxRelation list are assumed to be coherent.
  NOTE: The coherence is assured when the lists come from
        an included file, because in includes settings sections are forbidden
        and so the user cannot specify ClAxRelation herself.
***************************************************************/
{
  /* result lists */
  LIST RClauses;
  LIST RClAxRelation;

  /* pointers to last element in the result */
  LIST RLClauses = list_Nil();
  LIST RLClAxRelation = list_Nil();

  RClauses = list_Nil();
  RClAxRelation = list_Nil();

  /*
    Performs parallel tranversal of Clauses and ClAxRelation.
    Either deleting from both or keeping both for the result.
  */

  while (!list_Empty(*Clauses)) {  
    CLAUSE Clause;
    LIST ClAx;
    LIST Labels;
    char* Label;

    /* current list elements */
    LIST CClause;
    LIST CClAxRelation;

    CClause = *Clauses;
    CClAxRelation = *ClAxRelation;

    Clause = (CLAUSE) list_Car(CClause);

#ifdef CHECK
    if (list_Empty(CClAxRelation)) {
      misc_StartUserErrorReport();
      misc_ErrorReport("\n In dfg_FilterClausesBySelection: ClAxRelation too short!\n");
      misc_FinishUserErrorReport();
    }
#endif

    ClAx = (LIST) list_Car(CClAxRelation);

#ifdef CHECK
    if ((int)list_Car(ClAx) != clause_Number(Clause)) {
      misc_StartUserErrorReport();
      misc_ErrorReport("\n In dfg_FilterClausesBySelection: Incompatible ClAxRelation!\n");
      misc_FinishUserErrorReport();
    }
#endif

    Labels = list_Cdr(ClAx);
    if (list_Empty(Labels)) 
      Label = NULL;
    else
      Label = (char*)list_Car(Labels);

    /*and we can already move on*/
    *Clauses = list_Cdr(*Clauses);
    *ClAxRelation = list_Cdr(*ClAxRelation);

    if (!Label || !list_Member(selection,Label,(BOOL (*)(POINTER, POINTER))string_Equal)) {
      /* deleting */
      if (RClauses) { /*if RClauses exists so does RLClauses and RLClAxRelation*/
        list_Rplacd(RLClauses,*Clauses);
        list_Rplacd(RLClAxRelation,*ClAxRelation);
      }

      clause_Delete(Clause);
      list_DeleteWithElement(Labels,(void (*)(POINTER))string_StringFree);
      list_Free(ClAx);
      list_Free(CClause);
      list_Free(CClAxRelation);

    } else {
      /* keeping */
      if (!RClauses) {
        /* the result will contain at least one element */
        RClauses = CClause;
        RClAxRelation = CClAxRelation;
      }

      /* if we delete the next, whis will need to be updated */
      RLClauses = CClause;
      RLClAxRelation = CClAxRelation;
    }
  }

  *Clauses = RClauses;
  *ClAxRelation = RClAxRelation;
}

LIST dfg_DFGParser(FILE* File, FLAGSTORE Flags, PRECEDENCE Precedence, DFGDESCRIPTION Description,
		   LIST* Axioms, LIST* Conjectures, LIST* SortDecl,
		   LIST* UserDefinedPrecedence, LIST* UserDefinedSelection,
		   LIST* ClAxRelation, BOOL* HasPlainClauses)
/**************************************************************
  A stub around the real parser that takes care of following
  include directives.
  
  NOTE: This function is almost the same as tptp_TPTPParser form tptpparser.y
    any change in its implementation should propably be also propagated there.
***************************************************************/
{
  LIST Includes, Clauses;
  LIST FilesRead;
  DFGDESCRIPTION description_dummy;

  FilesRead = list_Nil();
  Includes = list_Nil();	

  dfg_IGNORESETTINGS = FALSE; 
  Clauses = dfg_DFGParserIncludesExplicit(File,Flags,Precedence,Description,FALSE,Axioms,Conjectures,SortDecl,UserDefinedPrecedence,UserDefinedSelection,ClAxRelation,&Includes,HasPlainClauses);
	
  while (list_Exist(Includes)) {
    LIST pair;
    char* filename;
    LIST selection;
  
    pair      = list_Top(Includes);
    filename  = (char*)list_PairFirst(pair);
    selection = (LIST)list_PairSecond(pair);
    list_PairFree(pair);
    Includes = list_Pop(Includes);
		
    if (list_Member(FilesRead,filename,(BOOL (*)(POINTER, POINTER))string_Equal)) {
      misc_UserWarning("File %s already included, skipped!\n",filename);
      string_StringFree(filename);
    } else {
      LIST Axs, Conjs, Cls, UDS, CAR;
      BOOL HPC;
      FILE* FileToInclude;      
				
      FileToInclude = dfg_OpenFile(filename, NULL);

      Axs = Conjs = UDS = CAR = list_Nil();

      dfg_IGNORESETTINGS = TRUE; 
      description_dummy = desc_Create();
      Cls = dfg_DFGParserIncludesExplicit(FileToInclude, Flags, Precedence, description_dummy, TRUE, &Axs, &Conjs, SortDecl, UserDefinedPrecedence, &UDS, &CAR, &Includes, &HPC);
      desc_Delete(description_dummy);

      if (list_Exist(selection)) { /*use the selection to filter Axs and Conjs */
        dfg_FilterClausesBySelection(&Cls,&CAR,selection);
      
        dfg_TestList = selection;
        Axs = list_DeleteElementIfFree(Axs,dfg_LabelFormulaPairIsNonmember,dfg_DeleteLabelTermPair);
	Conjs = list_DeleteElementIfFree(Conjs,dfg_LabelFormulaPairIsNonmember,dfg_DeleteLabelTermPair);
      }

      Clauses = list_Nconc(Clauses, Cls); 

      *Axioms = list_Nconc(*Axioms,Axs);
      *Conjectures = list_Nconc(*Conjectures,Conjs);

      /* "Summing up" UserDefinedSelection and ClAxRelation from all the files included. */
      *UserDefinedSelection = list_Nconc(*UserDefinedSelection, UDS);
      /*
	No explicit (user specified) ClAxRelation coming in dfg from includes!      
	*ClAxRelation = list_Nconc(*ClAxRelation, CAR);
	*/
      dfg_DeleteClAxRelation(CAR);

      /*The whole thing has plain clauses only if all the files have!*/			
      if (!HPC)
	*HasPlainClauses = FALSE;

      misc_CloseFile(FileToInclude,filename);
      FilesRead = list_Cons(filename,FilesRead);
    }

    list_DeleteWithElement(selection,(void (*)(POINTER))string_StringFree);		
  }
		
  list_DeleteWithElement(FilesRead,(void (*)(POINTER))string_StringFree);		
  return Clauses;
}

void dfg_InitExplicitParser() 
/**************************************************************
  This function should be called 
  prior to calling dfg_DFGParserIncludesExplicit
  from outside the dfg module.
***************************************************************/
{
  dfg_IGNORESETTINGS = FALSE; 
}

void dfg_CreateClausesFromTerms(LIST* Axioms, LIST* Conjectures,
				LIST* ClAxRelation, BOOL BuildClAx,
				FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   Pointers to two lists of term-label pairs,
            pointer to ClAx list,
            boolean flag and
            a flag store and a precedence.
  EFFECT:  Applies dfg_CreateClauseFromTerm to the 
           pairs in the lists treating the firsts as axioms 
           and second as conjectures, releasing the pair struct
           and the labels.
           If BuildClAx is set the labels are not deleted,
           but instead used to record the relation 
           between the clauses and their original names
           in the ClAxRelation structure
           (otherwise ClAxRelation remains intact).
***************************************************************/
{
  LIST    scan, tupel;
  TERM    clauseTerm;
  CLAUSE  clause;
  LIST    ClAxContribution, labels;

  ClAxContribution = list_Nil();
 
  /* Remove clause labels and create clauses from the terms */
  for (scan = *Axioms; !list_Empty(scan); scan = list_Cdr(scan)) {
    tupel = list_Car(scan);
    clauseTerm = list_PairSecond(tupel);
    clause = dfg_CreateClauseFromTerm(clauseTerm,TRUE, Flags, Precedence);
    list_Rplaca(scan, clause);

    if (BuildClAx) {
      if (list_PairFirst(tupel) != NULL)
        labels = list_List(list_PairFirst(tupel));
      else
        labels = list_Nil();
      
      ClAxContribution = list_Cons(list_Cons((POINTER)clause_Number(clause),labels),ClAxContribution);
    } else {
      if (list_PairFirst(tupel) != NULL)        /* Label is defined */
        string_StringFree(list_PairFirst(tupel));  /* Delete the label */
    }
  
    list_PairFree(tupel);
  }
  /* Since dfg_CreateClauseFromTerm() returns NULL for trivial tautologies */
  /* we now delete those NULL pointers from the clause list.               */
  *Axioms = list_PointerDeleteElement(*Axioms, NULL);
  for (scan = *Conjectures; !list_Empty(scan); scan = list_Cdr(scan)) {
    tupel = list_Car(scan);
    clauseTerm = list_PairSecond(tupel);
    clause = dfg_CreateClauseFromTerm(clauseTerm,FALSE, Flags, Precedence);
    list_Rplaca(scan, clause);

    if (BuildClAx) {
      if (list_PairFirst(tupel) != NULL)
        labels = list_List(list_PairFirst(tupel));
      else
        labels = list_Nil();
      
      ClAxContribution = list_Cons(list_Cons((POINTER)clause_Number(clause),labels),ClAxContribution);
    } else {
      if (list_PairFirst(tupel) != NULL)        /* Label is defined */
        string_StringFree(list_PairFirst(tupel));  /* Delete the label */
    }         
  
    list_PairFree(tupel);
  }
  /* Since dfg_CreateClauseFromTerm() returns NULL for trivial tautologies */
  /* we now delete those NULL pointers from the clause list.               */
  *Conjectures = list_PointerDeleteElement(*Conjectures, NULL);

  if (BuildClAx) {
    /* appending the contribution to the end! */
    *ClAxRelation = list_Nconc(*ClAxRelation,list_NReverse(ClAxContribution));
  }
}

LIST dfg_DFGParserIncludesExplicit(FILE* File, FLAGSTORE Flags, PRECEDENCE Precedence,
				   DFGDESCRIPTION Description, BOOL AppendImplicitClAx,
				   LIST* Axioms, LIST* Conjectures, LIST* SortDecl,
				   LIST* UserDefinedPrecedence, LIST* UserDefinedSelection,
				   LIST* ClAxRelation, LIST* Includes, BOOL* HasPlainClauses)
/**************************************************************
  INPUT:   The input file containing clauses or formulae in DFG syntax,
           a flag store, a precedence used to memorize settings
	       from the file, and Description to store problem description.
           Boolean flag determinig whether the implicit ClAxRelation
           from Clauses should be appended to the explicit one 
           coming from settings.
           Axioms, Conjectures, SortDecl, UserDefinedPrecedence
           UserDefinedSelection, and ClAxRelation, and Includes are
	   pointers to lists used as return values.
           HasPlainClauses is a return value to indicate if
           the problem had clauses in the plain format.
  RETURNS: The list of clauses from File.
  EFFECT:  Reads formulae and clauses from the input file.
           The axioms, conjectures, sort declarations, user-defined
	   precedences, and includes are appended to the respective lists,
       the lists are not deleted!
	   The Includes list contains pairs (filename, selection),
	   where selection is a list of names of formulas chosen to include.
	   All lists except the returned clause list contain pairs
	   (label, term), where <label> may be NULL, if no
	   label was specified for that term.
	   <UserDefinedPrecedence> contains symbols sorted by decreasing
	   precedence. This list will only be changed, if the precedence
	   is explicitly defined in the input file. This can be done
	   by the 'set_precedence' flag in the SPASS settings list in
	   the DFG input file.
           <UserDefinedSelection> is set to a list of predicates to be
           selected in clause inferences by the 'set_selection' flag
           <ClAxRelation> is set to a clause, axiom relation where the
           clauses are given by their number, the axioms by name (string)
  CAUTION: The weight of the clauses is not correct and the literals
           are not oriented!
***************************************************************/
{
  LIST  scan,to_delete;
  NAT   bottom;

  dfg_Init(File, Flags, Precedence, Description);  /* Initialize the parser and scanner */
  bottom = stack_Bottom();
  dfg_parse();          /* Invoke the parser */
#ifdef CHECK 
  if (!stack_Empty(bottom)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In dfg_DFGParser: Stack not empty!\n");
    misc_FinishErrorReport();
  }
#endif
  dfg_SymCleanUp();

  dfg_CreateClausesFromTerms(&dfg_AXCLAUSES,&dfg_CONCLAUSES,&dfg_CLAXRELATION,AppendImplicitClAx,Flags,Precedence);

  /* Delete the proof list */
  dfg_DeleteProofList(dfg_PROOFLIST);

  /* Delete the list_of_terms, since it'll be ignored */
  term_DeleteTermList(dfg_TERMLIST);

  if (list_Empty(dfg_PLAINAXCLAUSES) && list_Empty(dfg_PLAINCONCLAUSES))
    *HasPlainClauses = FALSE;
  else
    *HasPlainClauses = TRUE;
  
  scan = dfg_PLAINAXCLAUSES;
  while(!list_Empty(scan)) {
    DFG_PLAINCLAUSE clause = list_Car(scan);
    CLAUSE newclause = clause_CreateFromLiteralLists(clause->constraint, clause->antecedent, 
                                                     clause->succedent, FALSE, clause->selected);
    dfg_AXCLAUSES = list_Cons(newclause,dfg_AXCLAUSES);
  
    dfg_PlainClauseFree(clause);
    to_delete = scan;
    scan = list_Cdr(scan);    
    list_Free(to_delete);
  }  
  
  scan = dfg_PLAINCONCLAUSES;
  while(!list_Empty(scan)) {   
    DFG_PLAINCLAUSE clause = list_Car(scan);
    CLAUSE newclause = clause_CreateFromLiteralLists(clause->constraint, clause->antecedent, 
                                                     clause->succedent, TRUE, clause->selected);
    dfg_CONCLAUSES = list_Cons(newclause,dfg_CONCLAUSES);
     
    dfg_PlainClauseFree(clause);
    to_delete = scan;
    scan = list_Cdr(scan);    
    list_Free(to_delete);
  }  
   
  scan = list_Nconc(list_NReverse(dfg_AXCLAUSES), list_NReverse(dfg_CONCLAUSES));

  *Axioms      = list_Nconc(*Axioms, dfg_AXIOMLIST);
  *Conjectures = list_Nconc(*Conjectures, dfg_CONJECLIST);
  *SortDecl    = list_Nconc(*SortDecl, dfg_SORTDECLLIST);
  *Includes    = list_Nconc(*Includes, dfg_INCLUDELIST);
  dfg_USERPRECEDENCE = list_NReverse(dfg_USERPRECEDENCE);
  *UserDefinedPrecedence = list_Nconc(*UserDefinedPrecedence, dfg_USERPRECEDENCE);
  *UserDefinedSelection  = dfg_USERSELECTION;
  *ClAxRelation          = dfg_CLAXRELATION;

  return scan;
}

LIST dfg_ProofParser(FILE* File, FLAGSTORE Flags, PRECEDENCE Precedence, DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   The input file containing clauses in DFG syntax,
           a flag store and a precedence used to memorize settings
           from the file.
  RETURNS: A list of tuples (label,clause,justificationlist,splitlevel,origin)
           representing a proof.
  EFFECT:  Reads inputs clauses with labels and the proof lists
           from the input file.
           The elements of the list are lists with five items.
	   1. the label (a string) of a clause,
	   2. the clause in TERM format,
           3. the list of justification labels (strings, too),
           4. the split level of the clause,
           5. the origin of the clause (RULE struct from clause.h).
	   Note that the justification list is empty for input
	   clauses.
***************************************************************/
{
  LIST  scan, tupel;
  TERM  term;
  NAT   bottom; 
  
  dfg_Init(File, Flags, Precedence, Description);  /* Initialize the parser and scanner */
  bottom = stack_Bottom();
  
  dfg_parse();          /* Invoke the parser */
#ifdef CHECK 
  if (!stack_Empty(bottom)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In dfg_ProofParser: Stack not empty!\n");
    misc_FinishErrorReport();
  }
#endif

  dfg_SymCleanUp();

  /* Build the union of axiom and conjecture clauses */
  dfg_AXCLAUSES  = list_Nconc(dfg_AXCLAUSES, dfg_CONCLAUSES);
  dfg_CONCLAUSES = list_Nil();
  for (scan = dfg_AXCLAUSES; !list_Empty(scan); scan = list_Cdr(scan)) {
    tupel = list_Car(scan);
    term  = list_PairSecond(tupel);
    if (list_PairFirst(tupel) == NULL) {
      /* Ignore input clauses without label */
      term_Delete(term);
      list_PairFree(tupel);
      list_Rplaca(scan, NULL);
    } else
      /* Expand the pair to a tuple                            */
      /* (label,clause,justificationlist, split level, origin) */
      /* For input clauses the justificationlist is empty.     */
      /* Input clauses have split level 0.                     */
      list_Rplacd(tupel, list_Cons(term,list_Cons(list_Nil(),list_Cons(0, list_List((POINTER)INPUT)))));
  }
  /* Now delete the list items without labels */
  dfg_AXCLAUSES = list_PointerDeleteElement(dfg_AXCLAUSES, NULL);

  /* Delete the formula lists */
  dfg_DeleteFormulaPairList(dfg_AXIOMLIST);
  dfg_DeleteFormulaPairList(dfg_CONJECLIST);
  /* Delete includes list*/
  dfg_DeleteIncludePairList(dfg_INCLUDELIST);	  
  /* Delete the list of sort declarations */
  dfg_DeleteFormulaPairList(dfg_SORTDECLLIST);
  /* Delete the list_of_terms, since it'll be ignored */
  term_DeleteTermList(dfg_TERMLIST);

  /* Finally append the proof list to the list of input clauses with labels */
  dfg_PROOFLIST = list_NReverse(dfg_PROOFLIST);
  dfg_AXCLAUSES = list_Nconc(dfg_AXCLAUSES, dfg_PROOFLIST);

  return dfg_AXCLAUSES;
}


LIST dfg_TermParser(FILE* File, FLAGSTORE Flags, PRECEDENCE Precedence, DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   The input file containing a list of terms in DFG syntax,
           a flag store and a precedence used to memorize settings
	   from the file.
  RETURNS: The list of terms from <File>.
  EFFECT:  Reads terms from the list_of_terms from the input file.
***************************************************************/
{
  NAT bottom;
  
  dfg_Init(File, Flags, Precedence, Description);   /* Initialize the parser and scanner */
  bottom = stack_Bottom();
  dfg_parse();          /* Invoke the parser */
#ifdef CHECK 
  if (!stack_Empty(bottom)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In dfg_TermParser: Stack not empty!\n");
    misc_FinishErrorReport();
  }
#endif

  dfg_SymCleanUp();

  /* Delete the clause lists */
  dfg_DeleteFormulaPairList(dfg_AXCLAUSES);
  dfg_DeleteFormulaPairList(dfg_CONCLAUSES);
  /* Delete the formula lists */
  dfg_DeleteFormulaPairList(dfg_AXIOMLIST);
  dfg_DeleteFormulaPairList(dfg_CONJECLIST);
  /* Delete includes list*/
  dfg_DeleteIncludePairList(dfg_INCLUDELIST);	  
  /* Delete the proof list */
  dfg_DeleteProofList(dfg_PROOFLIST);
  /* Delete the list of sort declarations */
  dfg_DeleteFormulaPairList(dfg_SORTDECLLIST);

  return dfg_TERMLIST;
}

void dfg_DeleteFormulaPairList(LIST FormulaPairs)
/**************************************************************
  INPUT:   A list of pairs (label, formula).
  RETURNS: Nothing.
  EFFECT:  The list and the pairs with their strings and terms
           are completely deleted.
***************************************************************/
{
  LIST pair;

  for ( ; !list_Empty(FormulaPairs); FormulaPairs = list_Pop(FormulaPairs)) {
    pair = list_Car(FormulaPairs);  /* (label, term) */
    term_Delete(list_PairSecond(pair));
    if (list_PairFirst(pair) != NULL)
      string_StringFree(list_PairFirst(pair));  /* Free the label */
    list_PairFree(pair);
  }
}

void dfg_DeleteIncludePairList(LIST IncludePairs) 
/**************************************************************
  INPUT:   A list of pairs (filename, selectionlist).
  RETURNS: Nothing.
  EFFECT:  The list and the pairs with their strings
           are completely deleted.
***************************************************************/
{
  LIST pair;

  for ( ; !list_Empty(IncludePairs); IncludePairs = list_Pop(IncludePairs)) {
    pair = list_Car(IncludePairs);  /* (filename, selectionlist) */

    string_StringFree((char *)list_PairFirst(pair));
    list_DeleteWithElement(list_PairSecond(pair),(void (*)(POINTER))string_StringFree);
    list_PairFree(pair);
  }	
}

void dfg_StripLabelsFromList(LIST FormulaPairs)
/**************************************************************
  INPUT:   A list of pairs (label, formula).
  RETURNS: Nothing.
  EFFECT:  The pairs are replaced by the respective formula
           and the pairs with their label strings are deleted.
***************************************************************/
{
  LIST pair, scan;

  for (scan = FormulaPairs; !list_Empty(scan); scan = list_Cdr(scan)) {
    pair = list_Car(scan);  /* (label, term) */
    list_Rplaca(scan, list_PairSecond(pair));
    if (list_PairFirst(pair) != NULL)
      string_StringFree(list_PairFirst(pair));  /* Free the label */
    list_PairFree(pair);
  }
}

void dfg_DeleteProofList(LIST Proof)
/**************************************************************
  INPUT:   A list of tuples (label, term, justificationlist, split level).
  RETURNS: Nothing.
  EFFECT:  All memory used by the proof list is freed.
           The labels must NOT be NULL entries!
***************************************************************/
{
  /* Delete the proof list */
  for ( ; !list_Empty(Proof); Proof = list_Pop(Proof)) {
    LIST tupel = list_Car(Proof);
    string_StringFree(list_First(tupel));
    term_Delete(list_Second(tupel));
    dfg_DeleteStringList(list_Third(tupel));
    list_Delete(tupel);
  }
}

/**************************************************************/
/* Static Functions                                           */
/**************************************************************/

static void dfg_SymbolDecl(int SymbolType, char* Name, int Arity)
/**************************************************************
  INPUT:   The type of a symbol, the name, and the arity.
  RETURNS: Nothing.
  EFFECT:  This function handles the declaration of symbols.
           If <Arity> is -2, it means that the arity of the symbol
	   was not specified, if it is -1 the symbol is declared
	   with arbitrary arity. User defined symbols with arbitrary
           arity are not allowed.
	   The <Name> is deleted.
***************************************************************/
{
  NAT    arity;
  SYMBOL symbol;

  switch (Arity) {
  case -2: /* not specified */
    arity = 0;
    break;
  case -1:
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %u: symbols with arbitrary arity are not allowed.\n",
			 dfg_LINENUMBER);
    misc_FinishUserErrorReport();
  default:
    arity = Arity;
  }

  /* Check if this symbol was declared earlier */
  symbol = symbol_Lookup(Name);
  if (symbol != 0) {
    /* Symbol was declared before */
    /* Check if the old and new symbol type are equal */
    if ((SymbolType == DFG_FUNC && !symbol_IsFunction(symbol)) ||
	(SymbolType == DFG_PRDICAT && !symbol_IsPredicate(symbol)) ||
	((SymbolType == DFG_OPERAT || SymbolType == DFG_QUANTIF) && 
	 !symbol_IsJunctor(symbol))) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %u: symbol %s was already declared as ",
			   dfg_LINENUMBER, Name);
      switch (symbol_Type(symbol)) {
      case symbol_CONSTANT:
      case symbol_FUNCTION:
	misc_UserErrorReport("function.\n"); break;
      case symbol_PREDICATE:
	misc_UserErrorReport("predicate.\n"); break;
      case symbol_JUNCTOR:
	misc_UserErrorReport("junctor.\n"); break;
      default:
	misc_UserErrorReport("unknown type.\n");
      }
      misc_FinishUserErrorReport();
    }
    /* Now check the old and new arity if specified */
    if (Arity != -2 && Arity != symbol_Arity(symbol)) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %u: symbol %s was already declared with arity %d\n",
			   dfg_LINENUMBER, Name, symbol_Arity(symbol));
      misc_FinishUserErrorReport();
    }
  } else {
    /* Symbol was not declared before */
    switch (SymbolType) {
    case DFG_FUNC:
      symbol = symbol_CreateFunction(Name, arity, symbol_STATLEX,dfg_PRECEDENCE);
      break;
    case DFG_PRDICAT:
      symbol = symbol_CreatePredicate(Name, arity,symbol_STATLEX,dfg_PRECEDENCE);
      break;
    default:
      symbol = symbol_CreateJunctor(Name, arity, symbol_STATLEX, dfg_PRECEDENCE);
    }
    if (Arity == -2)
      /* Arity wasn't specified so check the arity for each occurrence */
      dfg_SymAdd(symbol);
  }

  string_StringFree(Name);  /* Name was copied */
}


static SYMBOL dfg_Symbol(char* Name, NAT Arity)
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
    string_StringFree(Name);
    dfg_SymCheck(symbol, Arity); /* Check the arity */
  } else {
    /* Variable */
    if (Arity > 0) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Undefined symbol %s.\n",dfg_LINENUMBER,Name);
      misc_FinishUserErrorReport();
    }
    symbol = dfg_VarLookup(Name);
  }
  return symbol;
}


static TERM dfg_CreateQuantifier(SYMBOL Symbol, LIST VarTermList, TERM Term)
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


CLAUSE dfg_CreateClauseFromTerm(TERM Clause, BOOL IsAxiom, FLAGSTORE Flags,
				PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A clause term, a boolean value, a flag store and a precedence.
  RETURNS: The clause term converted to a CLAUSE.
  EFFECT:  This function converts a clause stored as term into an
           EARL clause structure.
	   If 'IsAxiom' is TRUE the clause is treated as axiom
	   clause else as conjecture clause.
           The function deletes the literals "false" and "not(true)"
           if they occur in <Clause>.
	   The contents of the flag store and the precedence are changed
	   because the parser read flag and precedence settings from
  MEMORY:  The clause term is deleted.
***************************************************************/
{
  LIST   literals, scan;
  TERM   literal;
  CLAUSE result;
  
  if (term_TopSymbol(Clause) == fol_All()) {
    /* Remove and free the quantifier and the OR term */
    literals = term_ArgumentList(term_SecondArgument(Clause));
    term_RplacArgumentList(term_SecondArgument(Clause), list_Nil());
  } else {
    /* Remove and free the OR term */
    literals = term_ArgumentList(Clause);
    term_RplacArgumentList(Clause, list_Nil());
  }
  term_Delete(Clause);

  for (scan = literals; !list_Empty(scan); scan = list_Cdr(scan)) {
    literal = (TERM) list_Car(scan);
    if (symbol_IsPredicate(term_TopSymbol(literal))) {  /* Positive literal */
      if (fol_IsFalse(literal)) {
	/* Ignore this literal */
	term_Delete(literal);
	list_Rplaca(scan, NULL); /* Mark the actual list element */
      }
    } else {
      /* Found a negative literal */
      TERM atom = term_FirstArgument(literal);
      if (fol_IsTrue(atom)) {
	/* Ignore this literal */
	term_Delete(literal);
	list_Rplaca(literals, NULL); /* Mark the actual list element */
      }
    }
  }
  

  literals = list_PointerDeleteElement(literals, NULL);
  /* Remove the special literals treated above from the list */
  result = clause_CreateFromLiterals(literals, FALSE, !IsAxiom, FALSE, Flags, Precedence);
  /* Don't create sorts! */
  list_Delete(literals);

  return result;
}


static void dfg_SubSort(char* Name1, char* Name2)
/**************************************************************
  INPUT:   Two sort symbol names.
  RETURNS: Nothing.
  EFFECT:  This functions adds the formula
           forall([U], implies(Name1(U), Name2(U)))
	   to the list of axiom formulas. Both <Name1> and <Name2>
	   are deleted.
***************************************************************/
{
  SYMBOL s1, s2;
  TERM   varterm, t1, t2, term;

  s1 = dfg_Symbol(Name1, 1);   /* Should be unary predicates */
  s2 = dfg_Symbol(Name2, 1);
  if (!symbol_IsPredicate(s1)) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %d: Symbol is not a sort predicate.\n", dfg_LINENUMBER);
    misc_FinishUserErrorReport();
  }
  if (!symbol_IsPredicate(s2)) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %d: Symbol is not a sort predicate.\n", dfg_LINENUMBER);
    misc_FinishUserErrorReport();
  }

  varterm = term_Create(symbol_CreateStandardVariable(), list_Nil());
  symbol_ResetStandardVarCounter();
  
  t1   = term_Create(s1, list_List(varterm));
  t2   = term_Create(s2, list_List(term_Copy(varterm)));
  term = term_Create(fol_Implies(), list_Cons(t1, list_List(t2)));
  term = fol_CreateQuantifier(fol_All(), list_List(term_Copy(varterm)),
			      list_List(term));
  dfg_SORTDECLLIST = list_Nconc(dfg_SORTDECLLIST, list_List(list_PairCreate(NULL,term)));
}


static void dfg_SymbolGenerated(SYMBOL SortPredicate, BOOL FreelyGenerated,
				LIST GeneratedBy)
/**************************************************************
  INPUT:   A sort predicate, a boolean flag, and a list of function
           symbol names.
  RETURNS: Nothing.
  EFFECT:  This function stores the information that the <SortPredicate>
           is generated by the function symbols from the <GeneratedBy>
           list. The list contains only symbol names!
	   The <SortPredicate> AND the symbols from the list get
           the property GENERATED. Additionally the symbols get
	   the property FREELY, if the flag <FreelyGenerated> is TRUE.
***************************************************************/
{
  SYMBOL symbol;
  LIST   scan;

  if (!symbol_IsPredicate(SortPredicate)) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %d: Symbol is not a sort predicate.\n", dfg_LINENUMBER);
    misc_FinishUserErrorReport();
  }
  /* First reset the old information */
  symbol_RemoveProperty(SortPredicate, GENERATED);
  symbol_RemoveProperty(SortPredicate, FREELY);
  list_Delete(symbol_GeneratedBy(SortPredicate));
  /* Now set the new information */
  symbol_AddProperty(SortPredicate, GENERATED);
  if (FreelyGenerated)
    symbol_AddProperty(SortPredicate, FREELY);
  for (scan = GeneratedBy; !list_Empty(scan); scan = list_Cdr(scan)) {
    symbol = symbol_Lookup(list_Car(scan));
    if (symbol == 0) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: undefined symbol %s.\n", dfg_LINENUMBER,
			   (char*)list_Car(scan));
      misc_FinishUserErrorReport();
    } else if (!symbol_IsFunction(symbol)) { /* must be function or constant */
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Symbol is not a function.\n", dfg_LINENUMBER);
      misc_FinishUserErrorReport();
    }
    string_StringFree(list_Car(scan));
    list_Rplaca(scan, (POINTER)symbol);  /* change the name list to a symbol list */
    /* Set GENERATED properties for generating symbols */
    symbol_AddProperty(symbol, GENERATED);
    if (FreelyGenerated)
      symbol_AddProperty(symbol, FREELY);
  }
  symbol_SetGeneratedBy(SortPredicate, GeneratedBy);
}

static void dfg_TranslPairDecl(char* PropName, char* FoName)
/**************************************************************
  INPUT:   A propositional name/first order name pair
  RETURNS: Nothing.
  EFFECT:  Updates the appropriate list.
***************************************************************/
{
  SYMBOL PropSymbol, FoSymbol;
  
  PropSymbol = symbol_Lookup(PropName);
  FoSymbol = symbol_Lookup(FoName);
  
  if (PropSymbol == 0) {
    fprintf(stderr, "Line %u: Undefined symbol %s\n", 
	    dfg_LINENUMBER, PropName); 
    misc_Error(); 
  }
  else if (FoName == 0) {
    fprintf(stderr, "Line %u: Undefined symbol %s\n", 
	    dfg_LINENUMBER, FoName); 
    misc_Error(); 
  }
  else 
    eml_SetPropFoSymbolAssocList(PropSymbol,list_List((POINTER)FoSymbol));
  string_StringFree(PropName);  /* Names were copied */
  string_StringFree(FoName);  
}

/**************************************************************/
/* Functions for the Symbol Table                             */
/**************************************************************/

typedef struct {
  SYMBOL symbol;
  BOOL   valid;
  int    arity;
} DFG_SYMENTRY, *DFG_SYM;

static __inline__ DFG_SYM dfg_SymCreate(void)
{
  return (DFG_SYM) memory_Malloc(sizeof(DFG_SYMENTRY));
}

static __inline__ void dfg_SymFree(DFG_SYM Entry)
{
  memory_Free(Entry, sizeof(DFG_SYMENTRY));
}


static void dfg_SymAdd(SYMBOL Symbol)
/**************************************************************
  INPUT:   A symbol.
  RETURNS: Nothing.
  EFFECT:  This function adds 'Symbol' to the symbol list.
           The arity of these symbols will be checked every time
	   the symbol occurs.
***************************************************************/
{
  DFG_SYM newEntry = dfg_SymCreate();
  newEntry->symbol = Symbol;
  newEntry->valid  = FALSE;
  newEntry->arity  = 0;
  dfg_SYMBOLLIST = list_Cons(newEntry, dfg_SYMBOLLIST);
}


static void dfg_SymCheck(SYMBOL Symbol, NAT Arity)
/**************************************************************
  INPUT:   A symbol and the current arity of this symbol.
  RETURNS: Nothing.
  EFFECT:  This function compares the previous arity of 'Symbol'
           with the actual 'Arity'. If these values differ
	   the symbol's arity is set to arbitrary.
	   The arity of symbols whose arity was specified in
	   the symbol declaration section is checked and a warning
	   is printed to stderr in case of differences.
***************************************************************/
{
  LIST scan = dfg_SYMBOLLIST;
  while (!list_Empty(scan)) {
    DFG_SYM actEntry = (DFG_SYM) list_Car(scan);
    if (actEntry->symbol == Symbol) {
      if (actEntry->valid) {
	if (actEntry->arity != Arity) {
	  misc_StartUserErrorReport();
	  misc_UserErrorReport("\n Line %u:", dfg_LINENUMBER);
	  misc_UserErrorReport(" The actual arity %u", Arity);
	  misc_UserErrorReport(" of symbol %s differs", symbol_Name(Symbol));
	  misc_UserErrorReport(" from the previous arity %u.\n", actEntry->arity);
	  misc_FinishUserErrorReport();
	}
      } else {
	/* Not valid => first time */
	actEntry->arity = Arity;
	actEntry->valid = TRUE;
      }
      return;
    }
    scan = list_Cdr(scan);
  }

  /* Symbol isn't in SymbolList, so its arity was specified.        */
  /* Check if the specified arity corresponds with the actual arity */
  if (symbol_Arity(Symbol) != Arity) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %u: Symbol %s was declared with arity %u.\n",
			 dfg_LINENUMBER, symbol_Name(Symbol), symbol_Arity(Symbol));
    misc_FinishUserErrorReport();
  }
}


static void dfg_SymCleanUp(void)
/**************************************************************
  INPUT:   None.
  RETURNS: Nothing.
  EFFECT:  This function corrects all symbols whose arity wasn't
           specified in the symbol declaration section but seem
	   to occur with always the same arity.
	   The memory for the symbol list is freed.
***************************************************************/
{
  while (!list_Empty(dfg_SYMBOLLIST)) {
    DFG_SYM actEntry  = (DFG_SYM) list_Car(dfg_SYMBOLLIST);
    SYMBOL  actSymbol = actEntry->symbol;

    if (actEntry->arity != symbol_Arity(actSymbol))
      symbol_SetArity(actSymbol, actEntry->arity);

    dfg_SymFree(actEntry);
    dfg_SYMBOLLIST = list_Pop(dfg_SYMBOLLIST);
  }
}


/**************************************************************/
/* Functions for the Variable Table                           */
/**************************************************************/
  
typedef struct {
  char*  name;
  SYMBOL symbol;
} DFG_VARENTRY, *DFG_VAR;

static __inline__ char* dfg_VarName(DFG_VAR Entry)
{
  return Entry->name;
}

static __inline__ SYMBOL dfg_VarSymbol(DFG_VAR Entry)
{
  return Entry->symbol;
}

static __inline__ DFG_VAR dfg_VarCreate(void)
{
  return (DFG_VAR) memory_Malloc(sizeof(DFG_VARENTRY));
}

static void dfg_VarFree(DFG_VAR Entry)
{
  string_StringFree(Entry->name);
  memory_Free(Entry, sizeof(DFG_VARENTRY));
}

static void dfg_VarStart(void)
{
  dfg_VARLIST = list_Push(list_Nil(), dfg_VARLIST);
  dfg_VARDECL = TRUE;
}

static void dfg_VarStop(void)
{
  dfg_VARDECL = FALSE;
}

static void dfg_VarBacktrack(void)
{
  list_DeleteWithElement(list_Top(dfg_VARLIST), (void (*)(POINTER)) dfg_VarFree);
  dfg_VARLIST = list_Pop(dfg_VARLIST);
}

static void dfg_VarCheck(void)
/* Should be called after a complete clause or formula was parsed */
{
  if (!list_Empty(dfg_VARLIST)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In dfg_VarCheck: List of variables should be empty!\n");
    misc_FinishErrorReport();
  }
  symbol_ResetStandardVarCounter();
}

static SYMBOL dfg_VarLookup(char* Name)
/**************************************************************
  INPUT:   A variable name.
  RETURNS: The corresponding variable symbol.
  EFFECT:  If the variable name was quantified before, the
           corresponding symbol is returned and the <Name> is freed.
	   If the variable name was not quantified, and <dfg_VARDECL>
	   is TRUE, a new variable is created, else an error
	   message is printed and the program exits.
***************************************************************/
{
  LIST   scan, scan2;
  SYMBOL symbol = symbol_Null();

  scan  = dfg_VARLIST;
  scan2 = list_Nil();
  while (!list_Empty(scan) && list_Empty(scan2)) {
    scan2 = list_Car(scan);
    while (!list_Empty(scan2) &&
	   (!string_Equal(dfg_VarName(list_Car(scan2)), Name)))
      scan2 = list_Cdr(scan2);
    scan = list_Cdr(scan);
  }

  if (!list_Empty(scan2)) {
    /* Found variable */
    string_StringFree(Name);
    symbol = dfg_VarSymbol(list_Car(scan2));
  } else {
    /* Variable not found */
    if (dfg_VARDECL) {
      DFG_VAR newEntry = dfg_VarCreate();
      newEntry->name   = Name;
      newEntry->symbol = symbol_CreateStandardVariable();
      /* Add <newentry> to the first list in dfg_VARLIST */
      list_Rplaca(dfg_VARLIST, list_Cons(newEntry,list_Car(dfg_VARLIST)));
      symbol = dfg_VarSymbol(newEntry);
    } else {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %u: Free Variable %s.\n", dfg_LINENUMBER, Name);
      misc_FinishUserErrorReport();
    }
  }
  return symbol;
}

/**************************************************************/
/* Functions for the plain clauses                            */
/**************************************************************/
static DFG_PLAINCLAUSE dfg_PlainClauseCreate(void)
{
  return (DFG_PLAINCLAUSE) memory_Malloc(sizeof(DFG_PLAINCLAUSEHELP));
}

static void dfg_PlainClauseFree(DFG_PLAINCLAUSE Clause)
{
  list_Delete(Clause->constraint);
  list_Delete(Clause->antecedent);
  list_Delete(Clause->succedent);
  memory_Free(Clause, sizeof(DFG_PLAINCLAUSEHELP));
}

/**************************************************************/
/* Miscelaneous                                               */
/**************************************************************/

/*Get rid of leading and trailing qoutes of a string.
  Returns a newly allocated thing and frees the one destroyed.*/
static char * dfg_StripQuotes(char * str) {
  char * newstr;
  int len = strlen(str);
    
  str[len-1] = '\0'; /*one characer shorter from end*/

  newstr = string_StringCopy(str+1);    /*start duplicating from the second char*/

  memory_Free(str,len+1);

  return newstr;
}

