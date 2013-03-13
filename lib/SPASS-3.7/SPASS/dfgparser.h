/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
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

/* Line 2068 of yacc.c  */
#line 184 "./SPASS/dfgparser.y"

  int       number;
  char*     string;
  SYMBOL    symbol;
  SPROPERTY property;
  TERM      term;
  LIST      list;
  DFG_STATE state;
  BOOL      bool;



/* Line 2068 of yacc.c  */
#line 154 "SPASS/dfgparser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE dfg_lval;


