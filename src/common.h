/*
**      cdecl -- C gibberish translator
**      src/common.h
*/

#ifndef cdecl_common_H
#define cdecl_common_H

// local
#include "config.h"

///////////////////////////////////////////////////////////////////////////////

#define JSON_INDENT               2     /* speces per JSON indent level */

typedef struct {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;

#define YYLTYPE_IS_DECLARED       1
#define YYLTYPE_IS_TRIVIAL        1

#define CARET_CURRENT_LEX_COL     (-1)  // use lex's current column for caret

// extern variables
extern char const  *me;                 // program name
extern char const  *prompt;
extern char         prompt_buf[];
#if YYTEXT_POINTER
extern char        *yytext;
#else
extern char         yytext[];
#endif /* YYTEXT_POINTER */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints a '^' (in color, if possible and requested) under the offending
 * token.
 *
 * @param col The zero-based column to print the caret at.
 */
void print_caret( int col );

/**
 * Prints an error message to standard error.
 *
 * @param loc The location of the error.
 * @param format The \c printf() style format string.
 */
void print_error( YYLTYPE const *loc, char const *format, ... );

/**
 * Prints a hint message to standard error.
 *
 * @param format The \c printf() style format string.
 */
void print_hint( char const *format, ... );

/**
 * Prints a warning message to standard error.
 *
 * @param loc The location of the warning.
 * @param format The \c printf() style format string.
 */
void print_warning( YYLTYPE const *loc, char const *format, ... );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_common_H */
/* vim:set et sw=2 ts=2: */
