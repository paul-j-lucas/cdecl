/*
**      cdecl -- C gibberish translator
**      src/common.c
*/

// local
#include "config.h"                     /* must go first */
#include "color.h"
#include "common.h"
#include "options.h"
#include "util.h"

// standard
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

void c_error( char const *what, char const *hint ) {
  SGR_START_COLOR( stderr, error );
  PRINT_ERR( "error" );
  SGR_END_COLOR( stderr );
  PRINT_ERR( ": %s illegal", what );
  if ( hint )
    PRINT_ERR( " (maybe you mean \"%s\"?)", hint );
  PRINT_ERR( "\n" );
}

void c_warning( char const *what, char const *hint ) {
  SGR_START_COLOR( stderr, warning );
  PRINT_ERR( "warning" );
  SGR_END_COLOR( stderr );
  PRINT_ERR( ": %s illegal in %s", what, lang_name( opt_lang ) );
  if ( hint )
    PRINT_ERR( " (maybe you mean \"%s\"?)", hint );
  PRINT_ERR( "\n" );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
