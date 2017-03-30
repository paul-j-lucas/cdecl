/*
**      cdecl -- C gibberish translator
**      src/set.c
*/

// local
#include "config.h"                     /* must go first */
#include "color.h"
#include "common.h"
#include "lang.h"
#include "options.h"
#include "util.h"

// standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SET_OPTION(OPT,LITERAL,VAR,ON,NO) BLOCK(  \
  if ( strcmp( (OPT), LITERAL ) == 0 )            \
    { VAR = (ON); return; }                       \
  if ( strcmp( (OPT), "no" LITERAL ) == 0 )       \
    { VAR = (NO); return; } )

////////// extern functions ///////////////////////////////////////////////////

/**
 * Implements the cdecl "set" command.
 *
 * @param opt The name of the option to set. If null, displays the current
 * values of all options.
 */
void set_option( char const *opt ) {
  if ( opt ) {
    lang_t const new_lang = lang_find( opt );
    if ( new_lang ) {
      opt_lang = new_lang;
      return;
    }

#ifdef WITH_CDECL_DEBUG
    SET_OPTION( opt, "debug", opt_debug, true, false );
#endif /* WITH_CDECL_DEBUG */

    if ( strcmp( opt, "prompt" ) == 0 )
      { prompt[0] = prompt_buf[0]; prompt[1] = prompt_buf[1]; return; }
    if ( strcmp( opt, "noprompt" ) == 0 )
      { prompt[0] = prompt[1] = ""; return; }

    SET_OPTION( opt, "semicolon", opt_semicolon, true, false );

#ifdef YYDEBUG
    SET_OPTION( opt, "yydebug", yydebug, true, false );
#endif /* YYDEBUG */

    if ( strcmp( opt, "options" ) != 0 ) {
      PRINT_ERR( "\"%s\": ", opt );
      SGR_START_COLOR( stderr, error );
      PRINT_ERR( "error" );
      SGR_END_COLOR( stderr );
      PRINT_ERR( ": unknown set option\n" );
    }
  }

  printf( "\nValid set options (and command line equivalents) are:\n" );
  printf( "  options\n" );
#ifdef WITH_CDECL_DEBUG
  printf( "  debug (-d, --debug) / nodebug\n" );
#endif /* WITH_CDECL_DEBUG */
#ifndef HAVE_READLINE
  printf( "  interactive (-i, --interactive) / nointeractive\n" );
#endif /* HAVE_READLINE */
  printf( "  prompt / noprompt (-q, --quiet)\n" );
  printf( "  semicolon / nosemicolon (-s, --no-semicolon)\n" );
#ifdef YYDEBUG
  printf( "  yydebug (-y, --yydebug) / noyydebug\n" );
#endif /* YYDEBUG */

  printf( "\nCurrent set option values are:\n" );
#ifdef WITH_CDECL_DEBUG
  printf( "  %sdebug\n", opt_debug ? "  " : "no" );
#endif /* WITH_CDECL_DEBUG */
  printf( "  %sinteractive\n", opt_interactive ? "  " : "no" );
  printf( "    lang=%s\n", lang_name( opt_lang ) );
  printf( "  %sprompt\n", prompt[0][0] ? "  " : "no" );
  printf( "  %ssemicolon\n", opt_semicolon ? "  " : "no" );
#ifdef YYDEBUG
  printf( "  %syydebug\n", yydebug ? "  " : "no" );
#endif /* YYDEBUG */
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
