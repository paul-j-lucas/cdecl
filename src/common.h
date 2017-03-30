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
#define JSON_INDENT           2         /* speces per JSON indent level */

typedef struct c_ast c_ast_t;

/**
 * A pair of c_ast pointers used as one of the synthesized attribute types in
 * the parser.
 */
struct c_ast_pair {
  c_ast_t *top_ast;
  c_ast_t *target_ast;
};
typedef struct c_ast_pair c_ast_pair_t;

typedef struct {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;

#define YYLTYPE_IS_DECLARED   1
#define YYLTYPE_IS_TRIVIAL    1

#define CARET_CURRENT_LEX_COL (-1)      /* use lex's current column for ^ */

// extern variables
extern char const  *me;                 // program name
extern char const  *prompt[2];          // pointers to current prompts
extern char        *prompt_buf[2];      // buffers for prompts
extern size_t       y_col;
extern size_t       y_col_newline;
#if YYTEXT_POINTER
extern char        *yytext;
#else
extern char         yytext[];
#endif /* YYTEXT_POINTER */

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_common_H */
/* vim:set et sw=2 ts=2: */
