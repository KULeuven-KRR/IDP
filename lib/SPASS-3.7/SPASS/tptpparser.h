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
     AMPERSAND = 258,
     COLON = 259,
     COMMA = 260,
     EQUALS = 261,
     EQUALS_GREATER = 262,
     EXCLAMATION = 263,
     EXCLAMATION_EQUALS = 264,
     LBRKT = 265,
     LESS_EQUALS = 266,
     LESS_EQUALS_GREATER = 267,
     LESS_TILDE_GREATER = 268,
     LPAREN = 269,
     PERIOD = 270,
     QUESTION = 271,
     RBRKT = 272,
     RPAREN = 273,
     TILDE = 274,
     TILDE_AMPERSAND = 275,
     TILDE_VLINE = 276,
     VLINE = 277,
     _DLR_cnf = 278,
     _DLR_fof = 279,
     _DLR_fot = 280,
     _LIT_cnf = 281,
     _LIT_fof = 282,
     _LIT_include = 283,
     comment_line = 284,
     distinct_object = 285,
     dollar_dollar_word = 286,
     dollar_word = 287,
     lower_word = 288,
     real = 289,
     signed_integer = 290,
     single_quoted = 291,
     unsigned_integer = 292,
     upper_word = 293,
     unrecognized = 294
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 91 "./SPASS/tptpparser.y"
  
  char*     string;
  SYMBOL    symbol;
  TERM      term;
  LIST      list;
  BOOL      bool;



/* Line 2068 of yacc.c  */
#line 99 "SPASS/tptpparser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE tptp_lval;


