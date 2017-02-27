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
  lang_t const new_lang = lang_find( opt );
  if ( new_lang ) {
    opt_lang = new_lang;
    return;
  }

  SET_OPTION( opt, "create", opt_make_c, true, false );
#ifdef WITH_CDECL_DEBUG
  SET_OPTION( opt, "debug", opt_debug, true, false );
#endif /* WITH_CDECL_DEBUG */
  SET_OPTION( opt, "prompt", prompt, prompt_buf, "" );
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

  printf( "\nValid set options (and command line equivalents) are:\n" );
  printf( "  options\n" );
  printf( "  create (-c) / nocreate\n" );
  printf( "  prompt / noprompt (-q)\n" );
#ifndef HAVE_READLINE
  printf( "  interactive (-i) / nointeractive\n" );
#endif /* HAVE_READLINE */
  printf( "  preansi (-p) / ansi (-a) / cplusplus (-+)\n" );
#ifdef WITH_CDECL_DEBUG
  printf( "  debug (-d) / nodebug\n" );
#endif /* WITH_CDECL_DEBUG */
#ifdef YYDEBUG
  printf( "  yydebug (-D) / noyydebug\n" );
#endif /* YYDEBUG */
  printf( "\nCurrent set values are:\n" );
  printf( "  %screate\n", opt_make_c ? "   " : " no" );
#ifdef WITH_CDECL_DEBUG
  printf( "  %sdebug\n", opt_debug ? "   " : " no" );
#endif /* WITH_CDECL_DEBUG */
  printf( "  %sinteractive\n", opt_interactive ? "   " : " no" );
  printf( "  %sprompt\n", prompt[0] ? "   " : " no" );
  printf( "   lang=%s\n", lang_name( opt_lang ) );
#ifdef YYDEBUG
  printf( "  %syydebug\n", yydebug ? "   " : " no" );
#endif /* YYDEBUG */
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
