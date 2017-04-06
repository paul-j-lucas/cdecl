/*
**      cdecl -- C gibberish translator
**      src/common.h
**
**      Paul J. Lucas
*/

#ifndef cdecl_common_H
#define cdecl_common_H

// local
#include "config.h"                     /* must go first */

// standard
#include <stddef.h>                     /* for size_t */

///////////////////////////////////////////////////////////////////////////////

#define CPPDECL               "c++decl"
#define DEBUG_INDENT          2         /* spaces per debug indent level */

typedef struct {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;

#define YYLTYPE_IS_DECLARED   1
#define YYLTYPE_IS_TRIVIAL    1

// extern variables
extern char const  *me;                 // program name
extern size_t       my_col;             // current lexer column
extern size_t       my_col_newline;     // lexer column of newline
extern char const  *my_text;            // guaranteed pointer of yytext

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_common_H */
/* vim:set et sw=2 ts=2: */
