/*
**      cdecl -- C gibberish translator
**      src/common.h
*/

#ifndef cdecl_common_H
#define cdecl_common_H

// local
#include "config.h"

// standard
#include <stddef.h>                     /* for size_t */

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
extern char const  *prompt;             // pointer to current prompt
extern char         prompt_buf[];
extern size_t       y_col;
extern size_t       y_col_newline;
#if YYTEXT_POINTER
extern char        *yytext;
#else
extern char         yytext[];
#endif /* YYTEXT_POINTER */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the current column where the error started.
 *
 * @return Returns said zero-based column.
 */
int error_column( void );

/**
 * Prints a '^' (in color, if possible and requested) under the offending
 * token.
 *
 * @param col The zero-based column to print the caret at
 * or the special value \c CARET_CURRENT_LEX_COL that means use the lexer's
 * notion of what the current column is.
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
