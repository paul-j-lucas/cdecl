/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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
     Y_CAST = 258,
     Y_DECLARE = 259,
     Y_EXPLAIN = 260,
     Y_HELP = 261,
     Y_SET = 262,
     Y_QUIT = 263,
     Y_ARRAY = 264,
     Y_AS = 265,
     Y_BLOCK = 266,
     Y_FUNCTION = 267,
     Y_INTO = 268,
     Y_MEMBER = 269,
     Y_OF = 270,
     Y_POINTER = 271,
     Y_REFERENCE = 272,
     Y_RETURNING = 273,
     Y_TO = 274,
     Y_AUTO = 275,
     Y_CHAR = 276,
     Y_DOUBLE = 277,
     Y_EXTERN = 278,
     Y_FLOAT = 279,
     Y_INT = 280,
     Y_LONG = 281,
     Y_REGISTER = 282,
     Y_SHORT = 283,
     Y_STATIC = 284,
     Y_STRUCT = 285,
     Y_TYPEDEF = 286,
     Y_UNION = 287,
     Y_UNSIGNED = 288,
     Y_CONST = 289,
     Y_ENUM = 290,
     Y_SIGNED = 291,
     Y_VOID = 292,
     Y_VOLATILE = 293,
     Y_BOOL = 294,
     Y_COMPLEX = 295,
     Y_RESTRICT = 296,
     Y_WCHAR_T = 297,
     Y_NORETURN = 298,
     Y_THREAD_LOCAL = 299,
     Y_CLASS = 300,
     Y_COLON_COLON = 301,
     Y_CHAR16_T = 302,
     Y_CHAR32_T = 303,
     Y___BLOCK = 304,
     Y_END = 305,
     Y_ERROR = 306,
     Y_NAME = 307,
     Y_NUMBER = 308
   };
#endif
/* Tokens.  */
#define Y_CAST 258
#define Y_DECLARE 259
#define Y_EXPLAIN 260
#define Y_HELP 261
#define Y_SET 262
#define Y_QUIT 263
#define Y_ARRAY 264
#define Y_AS 265
#define Y_BLOCK 266
#define Y_FUNCTION 267
#define Y_INTO 268
#define Y_MEMBER 269
#define Y_OF 270
#define Y_POINTER 271
#define Y_REFERENCE 272
#define Y_RETURNING 273
#define Y_TO 274
#define Y_AUTO 275
#define Y_CHAR 276
#define Y_DOUBLE 277
#define Y_EXTERN 278
#define Y_FLOAT 279
#define Y_INT 280
#define Y_LONG 281
#define Y_REGISTER 282
#define Y_SHORT 283
#define Y_STATIC 284
#define Y_STRUCT 285
#define Y_TYPEDEF 286
#define Y_UNION 287
#define Y_UNSIGNED 288
#define Y_CONST 289
#define Y_ENUM 290
#define Y_SIGNED 291
#define Y_VOID 292
#define Y_VOLATILE 293
#define Y_BOOL 294
#define Y_COMPLEX 295
#define Y_RESTRICT 296
#define Y_WCHAR_T 297
#define Y_NORETURN 298
#define Y_THREAD_LOCAL 299
#define Y_CLASS 300
#define Y_COLON_COLON 301
#define Y_CHAR16_T 302
#define Y_CHAR32_T 303
#define Y___BLOCK 304
#define Y_END 305
#define Y_ERROR 306
#define Y_NAME 307
#define Y_NUMBER 308




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 253 "parser.y"
{
  c_ast_list_t  ast_list; /* for function arguments */
  c_ast_pair_t  ast_pair; /* for the AST being built */
  char const   *name;     /* name being declared or explained */
  int           number;   /* for array sizes */
  c_type_t      type;     /* built-in types, storage classes, & qualifiers */
}
/* Line 1529 of yacc.c.  */
#line 163 "parser.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE yylloc;
