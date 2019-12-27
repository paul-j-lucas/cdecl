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
#include <stdint.h>                     /* for uint8_t */
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
  printf( "  %sexplain-by-default\n", maybe_no( opt_explain ) );
  printf( " %sgraphs\n", opt_graph == C_GRAPH_DI ? " di" : opt_graph == C_GRAPH_TRI ? "tri" : " no" );
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
  opt_graph = enabled ? C_GRAPH_DI : C_GRAPH_NONE;
}

/**
 * Sets the explain-by-default option.
 *
 * @param enabled True if enabled.
 * @param loc The location of the option name.
 */
static void set_explain_by_default( bool enabled, c_loc_t const *loc ) {
  (void)loc;
  opt_explain = enabled;
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
  opt_graph = enabled ? C_GRAPH_TRI : C_GRAPH_NONE;
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

/**
 * Compares strings ignoring hyphens.
 *
 * @param s1 The first string.
 * @param s2 The second string.
 * @param n The maximum number of characters to check.
 * @return Returns an integer less than, equal to, or greater than 0 depending
 * upon whether \a s1 is less than, equal to, or greater than \a s2.
 */
static int strn_nohyphen_cmp( char const *s1, char const *s2, size_t n ) {
  while ( n-- > 0 ) {
    if ( *s1 == '-' )
      ++s1;
    else if ( *s2 == '-' )
      ++s2;
    else if ( (int)(uint8_t)*s1 != (int)(uint8_t)*s2 )
      break;
    else
      ++s1, ++s2;
  } // while

  return (int)(uint8_t)*s1 - (int)(uint8_t)*s2;
}

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
    if ( opt_graph == C_GRAPH_TRI ) {
      loc = NULL;
      set_trigraphs( /*enabled=*/true, loc );
    }
    return;
  }

  char const *const orig_name = opt_name;
  bool const is_no = strncmp( opt_name, "no", 2 ) == 0;
  if ( is_no )
    opt_name += 2/*no*/;
  size_t const opt_name_len = strlen( opt_name );

  static set_option_t const SET_OPTIONS[] = {
    { "alt-tokens",         false,  &set_alt_tokens         },
#ifdef ENABLE_CDECL_DEBUG
    { "debug",              false,  &set_debug              },
#endif /* ENABLE_CDECL_DEBUG */
    { "digraphs",           false,  &set_digraphs           },
    { "graphs",             true,   &set_digraphs           },
    { "explain-by-default", false,  &set_explain_by_default },
    { "prompt",             false,  &set_prompt             },
    { "semicolon",          false,  &set_semicolon          },
    { "trigraphs",          false,  &set_trigraphs          },
#ifdef YYDEBUG
    { "yydebug",            false,  &set_yydebug            },
#endif /* YYDEBUG */
    { NULL,                 false,  NULL                    }
  };

  set_option_t const *found_opt = NULL;
  for ( set_option_t const *opt = SET_OPTIONS; opt->name != NULL; ++opt ) {
    if ( !is_no && opt->no_only )
      continue;
    if ( strn_nohyphen_cmp( opt->name, opt_name, opt_name_len ) == 0 ) {
      if ( found_opt != NULL ) {
        print_error( loc,
          "\"%s\": ambiguous set option; could be \"%s%s\" or \"%s%s\"",
          orig_name,
          is_no ? "no" : "", found_opt->name,
          is_no ? "no" : "", opt->name
        );
        return;
      }
      found_opt = opt;
    }
  } // for

  if ( found_opt != NULL )
    (*found_opt->set_fn)( !is_no, loc );
  else
    print_error( loc, "\"%s\": unknown set option", orig_name );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
