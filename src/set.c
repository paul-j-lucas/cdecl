/*
**      cdecl -- C gibberish translator
**      src/set.c
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
**
**      This program is free software: you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation, either version 3 of the License, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 * Defines the function that implements the cdecl `set` command.
 */

// local
#include "config.h"                     /* must go first */
#include "c_lang.h"
#include "color.h"
#include "options.h"
#include "prompt.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SET_OPTION(OPT,LITERAL,VAR,ON,NO) BLOCK(  \
  if ( strcmp( (OPT), LITERAL ) == 0 )            \
    { VAR = (ON); return; }                       \
  if ( strcmp( (OPT), "no" LITERAL ) == 0 )       \
    { VAR = (NO); return; } )

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

/**
 * Implements the cdecl `set` command.
 *
 * @param opt The name of the option to set. If null, displays the current
 * values of all options.
 */
void set_option( char const *opt ) {
  if ( opt != NULL ) {
    c_lang_t const new_lang = c_lang_find( opt );
    if ( new_lang != LANG_NONE ) {
      c_lang_set( new_lang );
      return;
    }

#ifdef ENABLE_CDECL_DEBUG
    SET_OPTION( opt, "debug", opt_debug, true, false );
#endif /* ENABLE_CDECL_DEBUG */

    if ( strcmp( opt, "prompt" ) == 0 )
      { cdecl_prompt_enable( true ); return; }
    if ( strcmp( opt, "noprompt" ) == 0 )
      { cdecl_prompt_enable( false ); return; }

    SET_OPTION( opt, "semicolon", opt_semicolon, true, false );

#ifdef YYDEBUG
    SET_OPTION( opt, "yydebug", yydebug, true, false );
#endif /* YYDEBUG */

    if ( strcmp( opt, "options" ) != 0 ) {
      PRINT_ERR( "\"%s\": ", opt );
      SGR_START_COLOR( stderr, error );
      PUTS_ERR( "error" );
      SGR_END_COLOR( stderr );
      PUTS_ERR( ": unknown set option\n" );
    }
  }

  printf( "\nValid set options (and command line equivalents) are:\n" );
  printf( "  options\n" );
#ifdef ENABLE_CDECL_DEBUG
  printf( "  debug (-d, --debug) / nodebug\n" );
#endif /* ENABLE_CDECL_DEBUG */
  printf( "  interactive (-i, --interactive) / nointeractive\n" );
  printf( "  prompt / noprompt (-q, --quiet)\n" );
  printf( "  semicolon / nosemicolon (-s, --no-semicolon)\n" );
#ifdef YYDEBUG
  printf( "  yydebug (-y, --yydebug) / noyydebug\n" );
#endif /* YYDEBUG */

  printf( "\nCurrent set option values are:\n" );
#ifdef ENABLE_CDECL_DEBUG
  printf( "  %sdebug\n", opt_debug ? "  " : "no" );
#endif /* ENABLE_CDECL_DEBUG */
  printf( "  %sinteractive\n", opt_interactive ? "  " : "no" );
  printf( "    lang=%s\n", C_LANG_NAME() );
  printf( "  %sprompt\n", prompt[0][0] ? "  " : "no" );
  printf( "  %ssemicolon\n", opt_semicolon ? "  " : "no" );
#ifdef YYDEBUG
  printf( "  %syydebug\n", yydebug ? "  " : "no" );
#endif /* YYDEBUG */
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
