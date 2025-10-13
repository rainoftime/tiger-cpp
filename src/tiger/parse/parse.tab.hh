/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSE_TAB_HH_INCLUDED
# define YY_YY_PARSE_TAB_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 65 "tiger.y"

#include "tiger/absyn/absyn.h"
#include "tiger/symbol/symbol.h"

#line 54 "parse.tab.hh"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    ID = 258,                      /* ID  */
    STRING = 259,                  /* STRING  */
    INT = 260,                     /* INT  */
    COMMA = 261,                   /* COMMA  */
    COLON = 262,                   /* COLON  */
    SEMICOLON = 263,               /* SEMICOLON  */
    LPAREN = 264,                  /* LPAREN  */
    RPAREN = 265,                  /* RPAREN  */
    LBRACK = 266,                  /* LBRACK  */
    RBRACK = 267,                  /* RBRACK  */
    LBRACE = 268,                  /* LBRACE  */
    RBRACE = 269,                  /* RBRACE  */
    DOT = 270,                     /* DOT  */
    ARRAY = 271,                   /* ARRAY  */
    IF = 272,                      /* IF  */
    THEN = 273,                    /* THEN  */
    ELSE = 274,                    /* ELSE  */
    WHILE = 275,                   /* WHILE  */
    FOR = 276,                     /* FOR  */
    TO = 277,                      /* TO  */
    DO = 278,                      /* DO  */
    LET = 279,                     /* LET  */
    IN = 280,                      /* IN  */
    END = 281,                     /* END  */
    OF = 282,                      /* OF  */
    BREAK = 283,                   /* BREAK  */
    NIL = 284,                     /* NIL  */
    FUNCTION = 285,                /* FUNCTION  */
    VAR = 286,                     /* VAR  */
    TYPE = 287,                    /* TYPE  */
    ASSIGN = 288,                  /* ASSIGN  */
    OR = 289,                      /* OR  */
    AND = 290,                     /* AND  */
    EQ = 291,                      /* EQ  */
    NEQ = 292,                     /* NEQ  */
    LT = 293,                      /* LT  */
    LE = 294,                      /* LE  */
    GT = 295,                      /* GT  */
    GE = 296,                      /* GE  */
    PLUS = 297,                    /* PLUS  */
    MINUS = 298,                   /* MINUS  */
    TIMES = 299,                   /* TIMES  */
    DIVIDE = 300                   /* DIVIDE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 76 "tiger.y"

  int ival;                          /**< Integer literal values */
  std::string* sval;                 /**< String literal values */
  sym::Symbol *sym;                  /**< Symbol table entries */

  absyn::Exp *exp;                   /**< Expression nodes */
  absyn::ExpList *explist;           /**< Expression list nodes */
  absyn::Var *var;                   /**< Variable reference nodes */

  absyn::DecList *declist;           /**< Declaration list nodes */
  absyn::Dec *dec;                   /**< Declaration nodes */
  absyn::EFieldList *efieldlist;     /**< Expression field list nodes */
  absyn::EField *efield;             /**< Expression field nodes */

  absyn::NameAndTyList *tydeclist;    /**< Type declaration list nodes */
  absyn::NameAndTy *tydec;           /**< Type declaration nodes */
  absyn::FieldList *fieldlist;       /**< Field list nodes */
  absyn::Field *field;               /**< Field nodes */

  absyn::FunDecList *fundeclist;     /**< Function declaration list nodes */
  absyn::FunDec *fundec;             /**< Function declaration nodes */
  absyn::Ty *ty;                     /**< Type nodes */

#line 140 "parse.tab.hh"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_PARSE_TAB_HH_INCLUDED  */
