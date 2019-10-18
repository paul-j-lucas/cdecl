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

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * The signature for a Set option function.
 *
 * @param enabled True if enabled.
 * @param loc The location of the option name.
 */
typedef void (*set_opt_fn_t)( bool enabled, c_loc_t const *loc );

/**
 * cdecl `set` option.
 */
struct set_option {
  char const   *name;                   ///< Option name.
  bool          no_only;                ///< Valid only when "no...".
  set_opt_fn_t  set_fn;                 ///< Set function.
};
typedef struct set_option set_option_t;

////////// local functions ////////////////////////////////////////////////////

static inline char const* maybe_no( bool enabled ) {
  return enabled ? "  " : "no";
}

/**
 * Prints the current option settings.
 */
static void print_options( void ) {
  printf( "  %salt-tokens\n", maybe_no( opt_alt_tokens ) );
#ifdef ENABLE_CDECL_DEBUG
  printf( "  %sdebug\n", maybe_no( opt_debug ) );
#endif /* ENABLE_CDECL_DEBUG */
  printf( " %sgraphs\n", opt_graph == GRAPH_DI ? " di" : opt_graph == GRAPH_TRI ? "tri" : " no" );
  printf( "    lang=%s\n", C_LANG_NAME() );
  printf( "  %sprompt\n", maybe_no( prompt[0][0] != '\0' ) );
  printf( "  %ssemicolon\n", maybe_no( opt_semicolon ) );
#ifdef YYDEBUG
  printf( "  %syydebug\n", maybe_no( yydebug ) );
#endif /* YYDEBUG */
}

/**
 * Sets the alt-tokens option.
 *
 * @param enabled True if enabled.
 * @param loc The location of the option name.
 */
static void set_alt_tokens( bool enabled, c_loc_t const *loc ) {
  (void)loc;
  opt_alt_tokens = enabled;
}

#ifdef ENABLE_CDECL_DEBUG
/**
 * Sets the debug option.
 *
 * @param enabled True if enabled.
 * @param loc The location of the option name.
 */
static void set_debug( bool enabled, c_loc_t const *loc ) {
  (void)loc;
  opt_debug = enabled;
}
#endif /* ENABLE_CDECL_DEBUG */

/**
 * Sets the digraphs-tokens option.
 *
 * @param enabled True if enabled.
 * @param loc The location of the option name.
 */
static void set_digraphs( bool enabled, c_loc_t const *loc ) {
  (void)loc;
  opt_graph = enabled ? GRAPH_DI : GRAPH_NONE;
}

/**
 * Sets the prompt option.
 *
 * @param enabled True if enabled.
 * @param loc The location of the option name.
 */
static void set_prompt( bool enabled, c_loc_t const *loc ) {
  (void)loc;
  cdecl_prompt_enable( enabled );
}

/**
 * Sets the semicolon option.
 *
 * @param enabled True if enabled.
 * @param loc The location of the option name.
 */
static void set_semicolon( bool enabled, c_loc_t const *loc ) {
  (void)loc;
  opt_semicolon = enabled;
}

/**
 * Sets the trigraphs option.
 *
 * @param enabled True if enabled.
 * @param loc The location of the option name.
 */
static void set_trigraphs( bool enabled, c_loc_t const *loc ) {
  opt_graph = enabled ? GRAPH_TRI : GRAPH_NONE;
  if ( opt_graph && opt_lang >= LANG_CPP_17 )
    print_warning( loc,
      "trigraphs are no longer supported in %s", C_LANG_NAME()
    );
}

#ifdef YYDEBUG
/**
 * Sets the yydebug option.
 *
 * @param enabled True if enabled.
 * @param loc The location of the option name.
 */
static void set_yydebug( bool enabled, c_loc_t const *loc ) {
  (void)loc;
  yydebug = enabled;
}
#endif /* YYDEBUG */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Implements the cdecl `set` command.
 *
 * @param loc The location of the option token.
 * @param opt_name The name of the option to set. If null, displays the current
 * values of all options.
 */
void set_option( c_loc_t const *loc, char const *opt_name ) {
  if ( opt_name == NULL || strcmp( opt_name, "options" ) == 0 ) {
    print_options();
    return;
  }

  c_lang_id_t const new_lang = c_lang_find( opt_name );
  if ( new_lang != LANG_NONE ) {
    c_lang_set( new_lang );
    if ( opt_graph == GRAPH_TRI ) {
      loc = NULL;
      set_trigraphs( /*enabled=*/true, loc );
    }
    return;
  }

  char const *const orig_name = opt_name;
  bool const enabled = strncmp( opt_name, "no", 2 ) != 0;
  if ( enabled )
    opt_name += 2/*no*/;

  static set_option_t const SET_OPTIONS[] = {
    { "alt-tokens", false,  &set_alt_tokens },
#ifdef ENABLE_CDECL_DEBUG
    { "debug",      false,  &set_debug      },
#endif /* ENABLE_CDECL_DEBUG */
    { "digraphs",   false,  &set_digraphs   },
    { "graphs",     true,   &set_digraphs   },
    { "prompt",     false,  &set_prompt     },
    { "semicolon",  false,  &set_semicolon  },
    { "trigraphs",  false,  &set_trigraphs  },
#ifdef YYDEBUG
    { "yydebug",    false,  &set_yydebug    },
#endif /* YYDEBUG */
    { NULL,         false,  NULL            }
  };

  for ( set_option_t const *opt = SET_OPTIONS; opt->name != NULL; ++opt ) {
    if ( enabled && opt->no_only )
      continue;
    if ( strcmp( opt_name, opt->name ) == 0 ) {
      (*opt->set_fn)( enabled, loc );
      return;
    }
  } // for

  print_error( loc, "\"%s\": unknown set option", orig_name );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
