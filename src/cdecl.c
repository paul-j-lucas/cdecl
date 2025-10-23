/*
**      cdecl -- C gibberish translator
**      src/cdecl.c
**
**      Copyright (C) 2017-2025  Paul J. Lucas, et al.
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
 * Defines main() as well as functions for initialization and clean-up.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cdecl.h"
#include "c_ast.h"
#include "c_keyword.h"
#include "c_typedef.h"
#include "cdecl_command.h"
#include "cdecl_keyword.h"
#include "cdecl_term.h"
#include "cli_options.h"
#include "color.h"
#include "config_file.h"
#include "lexer.h"
#include "options.h"
#include "p_keyword.h"
#include "p_macro.h"
#include "parse.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stddef.h>                     /* for size_t */
#include <stdlib.h>
#include <sysexits.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries.

// extern variable definitions
bool        cdecl_is_initialized;
char const *cdecl_input_path;
bool        cdecl_is_interactive;
bool        cdecl_is_testing;
char const *prog_name;

/// @endcond

////////// local functions ////////////////////////////////////////////////////

/**
 * Cleans-up **cdecl** data at program termination.
 *
 * @note This function is called only via **atexit**(3).
 *
 * @sa main()
 */
static void cdecl_cleanup( void ) {
  c_ast_cleanup();
}

////////// extern functions ///////////////////////////////////////////////////

void cdecl_quit( void ) {
  exit( EX_OK );
}

bool is_cppdecl( void ) {
  static char const *const NAMES[] = {
    CPPDECL,
    "cppdecl",
    "cxxdecl"
  };

  FOREACH_ARRAY_ELEMENT( char const*, name, NAMES ) {
    if ( strcmp( prog_name, *name ) == 0 )
      return true;
  } // for
  return false;
}

/**
 * The main entry point.
 *
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns 0 on success, non-zero on failure.
 */
int main( int argc, char const *const argv[] ) {
  prog_name = base_name( argv[0] );
  ATEXIT( &cdecl_cleanup );
  cdecl_is_testing = str_is_affirmative( getenv( "CDECL_TEST" ) );
  if ( str_is_affirmative( getenv( "CDECL_DEBUG" ) ) )
    wait_for_debugger_attach();

  cli_options_init( &argc, &argv );     // must call before colors_init()
  colors_init();                        // must call before cdecl_term_init()
  cdecl_term_init();                    // call before possible print_error()

  // The order of these doesn't matter.
  c_keywords_init();
  cdecl_keywords_init();
  lexer_init();
  parser_init();
  p_keywords_init();
  p_macros_init();

  // Everything above must be called before c_typedefs_init() since it actually
  // uses the parser.
  c_typedefs_init();

  if ( opt_read_config )
    config_init();

  cdecl_is_initialized = true;

  // cli_options_init() makes argv[0] be the first argument, if any, and no
  // longer the program name.
  return cdecl_parse_cli( STATIC_CAST( size_t, argc ), argv );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
