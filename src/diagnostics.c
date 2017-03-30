/*
**      cdecl -- C gibberish translator
**      src/diagnostics.c
**
**      Paul J. Lucas
*/

// local
#include "config.h"                     /* must go first */
#include "color.h"
#include "diagnostics.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

int error_column( void ) {
  if ( y_col == 0 )
    return (int)y_col_newline;

  if ( *yytext == '\n' )
    return (int)y_col;

  return (int)y_col - (int)strlen( yytext );
}

void print_caret( int col ) {
  if ( col == CARET_CURRENT_LEX_COL )
    col = error_column();
  assert( col >= 0 );
  size_t const caret_col = strlen( prompt[0] ) + col;
  PRINT_ERR( "%*s", (int)caret_col, "" );
  SGR_START_COLOR( stderr, caret );
  FPUTC( '^', stderr );
  SGR_END_COLOR( stderr );
  FPUTC( '\n', stderr );
}

void print_error( YYLTYPE const *loc, char const *format, ... ) {
  if ( loc ) {
    print_caret( loc->first_column );
    PRINT_ERR( "%d: ", loc->first_column );
  }
  SGR_START_COLOR( stderr, error );
  PRINT_ERR( "error" );
  SGR_END_COLOR( stderr );
  PRINT_ERR( ": " );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );

  PRINT_ERR( "\n" );
}

void print_hint( char const *format, ... ) {
  PRINT_ERR( "\t(did you mean " );
  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
  PRINT_ERR( "?)\n" );
}

void print_warning( YYLTYPE const *loc, char const *format, ... ) {
  if ( loc ) {
    print_caret( loc->first_column );
    PRINT_ERR( "%d: ", loc->first_column );
  }
  SGR_START_COLOR( stderr, warning );
  PRINT_ERR( "warning" );
  SGR_END_COLOR( stderr );
  PRINT_ERR( ": " );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );

  PRINT_ERR( " illegal in %s\n", lang_name( opt_lang ) );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
