/*
**      cdecl -- C gibberish translator
**      src/set.c
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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
#include "cdecl.h"                      /* must go first */
#include "c_lang.h"
#include "color.h"
#include "options.h"
#include "print.h"
#include "prompt.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IF_OPT(LITERAL,...) BLOCK( \
  if ( strcmp( opt, (LITERAL) ) == 0 ) { __VA_ARGS__ return; } )

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

/**
 * Implements the cdecl `set` command.
 *
 * @param loc The location of the option token.
 * @param opt The name of the option to set. If null, displays the current
 * values of all options.
 */
void set_option( c_loc_t const *loc, char const *opt ) {
  if ( opt != NULL ) {
    //
    // First, check to see if the option is the name of a supported language:
    // if so, switch to that language.
    //
    c_lang_id_t const new_lang = c_lang_find( opt );
    if ( new_lang != LANG_NONE ) {
      c_lang_set( new_lang );
      return;
    }

#ifdef ENABLE_CDECL_DEBUG
    IF_OPT(    "debug",     opt_debug = true;             );
    IF_OPT(  "nodebug",     opt_debug = false;            );
#endif /* ENABLE_CDECL_DEBUG */
    IF_OPT(  "digraphs",    opt_graph = GRAPH_DI;         );
    IF_OPT( "trigraphs",    opt_graph = GRAPH_TRI;        );
    IF_OPT(  "nographs",    opt_graph = GRAPH_NONE;       );
    IF_OPT(    "prompt",    cdecl_prompt_enable( true );  );
    IF_OPT(  "noprompt",    cdecl_prompt_enable( false ); );
    IF_OPT(    "semicolon", opt_semicolon = true;         );
    IF_OPT(  "nosemicolon", opt_semicolon = false;        );
#ifdef YYDEBUG
    IF_OPT(    "yydebug",   yydebug = true;               );
    IF_OPT(  "noyydebug",   yydebug = false;              );
#endif /* YYDEBUG */

    if ( strcmp( opt, "options" ) != 0 )
      print_error( loc, "\"%s\": unknown set option", opt );
  }

  printf( "\nValid set options (and command line equivalents, if any) are:\n" );
#ifdef ENABLE_CDECL_DEBUG
  printf( "   debug (-d, --debug) / nodebug\n" );
#endif /* ENABLE_CDECL_DEBUG */
  printf( "   digraphs (-2, --digraphs) / trigraphs (-3, --trigraphs) / nographs\n" );
  printf( "   interactive (-i, --interactive) / nointeractive\n" );
  printf( "   prompt / noprompt (-q, --quiet)\n" );
  printf( "   semicolon / nosemicolon (-s, --no-semicolon)\n" );
#ifdef YYDEBUG
  printf( "   yydebug (-y, --yydebug) / noyydebug\n" );
#endif /* YYDEBUG */

  printf( "\nCurrent set option values are:\n" );
#ifdef ENABLE_CDECL_DEBUG
  printf( "   %sdebug\n", opt_debug ? "  " : "no" );
#endif /* ENABLE_CDECL_DEBUG */
  printf( "  %sgraphs\n", opt_graph == GRAPH_DI ? " di" : opt_graph == GRAPH_TRI ? "tri" : " no" );
  printf( "   %sinteractive\n", opt_interactive ? "  " : "no" );
  printf( "     lang=%s\n", C_LANG_NAME() );
  printf( "   %sprompt\n", prompt[0][0] != '\0' ? "  " : "no" );
  printf( "   %ssemicolon\n", opt_semicolon ? "  " : "no" );
#ifdef YYDEBUG
  printf( "  %syydebug\n", yydebug ? "  " : "no" );
#endif /* YYDEBUG */
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
