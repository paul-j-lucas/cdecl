/*
**      cdecl -- C gibberish translator
**      src/set.c
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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
#include <assert.h>
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
 * @param opt_name_loc The location of the option name.
 */
typedef void (*set_opt_fn_t)( bool enabled, c_loc_t const *opt_name_loc,
                              char const *opt_value,
                              c_loc_t const *opt_value_loc );

/**
 * cdecl `set` option type.
 */
enum set_option_type {
  SET_TOGGLE,                           ///< Toggle: `foo` & `nofoo`.
  SET_AFF_ONLY,                         ///< Affirmative only, e.g., `foo`.
  SET_NEG_ONLY                          ///< Negative only, e.g., `nofoo`.
};
typedef enum set_option_type set_option_type_t;

/**
 * cdecl `set` option.
 */
struct set_option {
  char const       *name;               ///< Option name.
  set_option_type_t type;               ///< Option type.
  bool              takes_value;        ///< Takes a value?
  set_opt_fn_t      set_fn;             ///< Set function.
};
typedef struct set_option set_option_t;

// local functions
static void set_trigraphs( bool, c_loc_t const*, char const*, c_loc_t const* );

////////// local functions ////////////////////////////////////////////////////

static inline char const* maybe_no( bool enabled ) {
  return enabled ? "  " : "no";
}

/**
 * Prints the string representation of the explicit `int` option.
 */
static void print_opt_explicit_int( void ) {
  bool const is_explicit_s   = is_explicit_int( T_SHORT );
  bool const is_explicit_i   = is_explicit_int( T_INT );
  bool const is_explicit_l   = is_explicit_int( T_LONG );
  bool const is_explicit_ll  = is_explicit_int( T_LONG_LONG );

  bool const is_explicit_us  = is_explicit_int( T_UNSIGNED | T_SHORT );
  bool const is_explicit_ui  = is_explicit_int( T_UNSIGNED | T_INT );
  bool const is_explicit_ul  = is_explicit_int( T_UNSIGNED | T_LONG );
  bool const is_explicit_ull = is_explicit_int( T_UNSIGNED | T_LONG_LONG );

  if ( is_explicit_s & is_explicit_i && is_explicit_l && is_explicit_ll ) {
    PUTC_OUT( 'i' );
  }
  else {
    if ( is_explicit_s   ) PUTC_OUT(  's'  );
    if ( is_explicit_i   ) PUTC_OUT(  'i'  );
    if ( is_explicit_l   ) PUTC_OUT(  'l'  );
    if ( is_explicit_ll  ) PUTS_OUT(  "ll" );
  }

  if ( is_explicit_us & is_explicit_ui && is_explicit_ul && is_explicit_ull ) {
    PUTC_OUT( 'u' );
  }
  else {
    if ( is_explicit_us  ) PUTS_OUT( "us"  );
    if ( is_explicit_ui  ) PUTS_OUT( "ui"  );
    if ( is_explicit_ul  ) PUTS_OUT( "ul"  );
    if ( is_explicit_ull ) PUTS_OUT( "ull" );
  }
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

  if ( opt_explicit_int[0] != T_NONE || opt_explicit_int[1] != T_NONE ) {
    PUTS_OUT( "    explicit-int=" );
    print_opt_explicit_int();
    PUTC_OUT( '\n' );
  } else {
    PUTS_OUT( "  noexplicit-int\n" );
  }

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
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_alt_tokens( bool enabled, c_loc_t const *opt_name_loc,
                            char const *opt_value,
                            c_loc_t const *opt_value_loc ) {
  (void)opt_name_loc;
  (void)opt_value;
  (void)opt_value_loc;
  opt_alt_tokens = enabled;
}

#ifdef ENABLE_CDECL_DEBUG
/**
 * Sets the debug option.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_debug( bool enabled, c_loc_t const *opt_name_loc,
                       char const *opt_value,
                       c_loc_t const *opt_value_loc ) {
  (void)opt_name_loc;
  (void)opt_value;
  (void)opt_value_loc;
  opt_debug = enabled;
}
#endif /* ENABLE_CDECL_DEBUG */

/**
 * Sets the digraphs-tokens option.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_digraphs( bool enabled, c_loc_t const *opt_name_loc,
                          char const *opt_value,
                          c_loc_t const *opt_value_loc ) {
  (void)opt_name_loc;
  (void)opt_value;
  (void)opt_value_loc;
  opt_graph = enabled ? C_GRAPH_DI : C_GRAPH_NONE;
}

/**
 * Sets the explain-by-default option.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_explain_by_default( bool enabled, c_loc_t const *opt_name_loc,
                                    char const *opt_value,
                                    c_loc_t const *opt_value_loc ) {
  (void)opt_name_loc;
  (void)opt_value;
  (void)opt_value_loc;
  opt_explain = enabled;
}

/**
 * Sets the explicit-int option.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_explicit_int( bool enabled, c_loc_t const *opt_name_loc,
                              char const *opt_value,
                              c_loc_t const *opt_value_loc ) {
  (void)opt_name_loc;
  if ( enabled )
    parse_opt_explicit_int( opt_value_loc, opt_value );
  else
    set_opt_explicit_int( T_NONE );
}

/**
 * Sets the current language.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_lang( bool enabled, c_loc_t const *opt_name_loc,
                      char const *opt_value, c_loc_t const *opt_value_loc ) {
  assert( enabled );
  (void)opt_name_loc;

  c_lang_id_t const new_lang = c_lang_find( opt_value );
  if ( new_lang != LANG_NONE ) {
    c_lang_set( new_lang );
    if ( opt_graph == C_GRAPH_TRI )
      set_trigraphs( /*enabled=*/true, NULL, NULL, NULL );
  } else {
    print_error( opt_value_loc, "\"%s\": unknown language", opt_value );
  }
}

/**
 * Sets the prompt option.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_prompt( bool enabled, c_loc_t const *opt_name_loc,
                        char const *opt_value,
                        c_loc_t const *opt_value_loc ) {
  (void)opt_name_loc;
  (void)opt_value;
  (void)opt_value_loc;
  cdecl_prompt_enable( enabled );
}

/**
 * Sets the semicolon option.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_semicolon( bool enabled, c_loc_t const *opt_name_loc,
                           char const *opt_value,
                           c_loc_t const *opt_value_loc ) {
  (void)opt_name_loc;
  (void)opt_value;
  (void)opt_value_loc;
  opt_semicolon = enabled;
}

/**
 * Sets the trigraphs option.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_trigraphs( bool enabled, c_loc_t const *opt_name_loc,
                           char const *opt_value,
                           c_loc_t const *opt_value_loc ) {
  (void)opt_value;
  (void)opt_value_loc;
  opt_graph = enabled ? C_GRAPH_TRI : C_GRAPH_NONE;
  if ( opt_graph && opt_lang >= LANG_CPP_17 )
    print_warning( opt_name_loc,
      "trigraphs are no longer supported in %s", C_LANG_NAME()
    );
}

#ifdef YYDEBUG
/**
 * Sets the yydebug option.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_yydebug( bool enabled, c_loc_t const *opt_name_loc,
                         char const *opt_value,
                         c_loc_t const *opt_value_loc ) {
  (void)opt_name_loc;
  (void)opt_value;
  (void)opt_value_loc;
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
 * @param opt_name The name of the option to set. If null, displays the current
 * values of all options.
 * @param opt_name_loc The location of the option token.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
void set_option( char const *opt_name, c_loc_t const *opt_name_loc,
                 char const *opt_value, c_loc_t const *opt_value_loc ) {
  if ( opt_name == NULL || strcmp( opt_name, "options" ) == 0 ) {
    print_options();
    return;
  }

  c_lang_id_t const new_lang = c_lang_find( opt_name );
  if ( new_lang != LANG_NONE ) {
    c_lang_set( new_lang );
    if ( opt_graph == C_GRAPH_TRI ) {
      opt_name_loc = NULL;
      set_trigraphs( /*enabled=*/true, opt_name_loc, NULL, NULL );
    }
    return;
  }

  char const *const orig_name = opt_name;
  bool const is_no = strncmp( opt_name, "no", 2 ) == 0;
  if ( is_no )
    opt_name += 2/*no*/;
  size_t const opt_name_len = strlen( opt_name );

  static set_option_t const SET_OPTIONS[] = {
    { "alt-tokens",         SET_TOGGLE,   false,  &set_alt_tokens         },
#ifdef ENABLE_CDECL_DEBUG
    { "debug",              SET_TOGGLE,   false,  &set_debug              },
#endif /* ENABLE_CDECL_DEBUG */
    { "digraphs",           SET_AFF_ONLY, false,  &set_digraphs           },
    { "graphs",             SET_NEG_ONLY, false,  &set_digraphs           },
    { "explain-by-default", SET_TOGGLE,   false,  &set_explain_by_default },
    { "explicit-int",       SET_TOGGLE,   true,   &set_explicit_int       },
    { "lang",               SET_AFF_ONLY, true,   &set_lang               },
    { "prompt",             SET_TOGGLE,   false,  &set_prompt             },
    { "semicolon",          SET_TOGGLE,   false,  &set_semicolon          },
    { "trigraphs",          SET_AFF_ONLY, false,  &set_trigraphs          },
#ifdef YYDEBUG
    { "yydebug",            SET_TOGGLE,   false,  &set_yydebug            },
#endif /* YYDEBUG */
    { NULL,                 SET_TOGGLE,   false,  NULL                    }
  };

  set_option_t const *found_opt = NULL;
  for ( set_option_t const *opt = SET_OPTIONS; opt->name != NULL; ++opt ) {
    if ( strn_nohyphen_cmp( opt->name, opt_name, opt_name_len ) == 0 ) {
      if ( found_opt != NULL ) {
        print_error( opt_name_loc,
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

  if ( found_opt == NULL ) {
    print_error( opt_name_loc, "\"%s\": unknown set option", orig_name );
    return;
  }

  switch ( found_opt->type ) {
    case SET_TOGGLE:
      break;
    case SET_AFF_ONLY:
      if ( is_no ) {
        print_error( opt_name_loc,
          "\"no\" not valid for \"%s\"", found_opt->name
        );
        return;
      }
      break;
    case SET_NEG_ONLY:
      if ( !is_no ) {
        print_error( opt_name_loc,
          "\"no\" required for \"%s\"", found_opt->name
        );
        return;
      }
      break;
  } // switch

  if ( opt_value == NULL ) {
    if ( !is_no && found_opt->takes_value ) {
      print_error( opt_name_loc,
        "\"%s\" set option requires =<value>",
        orig_name
      );
      return;
    }
  } else {
    if ( is_no ) {
      print_error( opt_value_loc, "\"no\" set options take no value" );
      return;
    }
    if ( !found_opt->takes_value ) {
      print_error( opt_value_loc,
        "\"%s\": set option \"%s\" takes no value",
        opt_value, orig_name
      );
      return;
    }
  }

  (*found_opt->set_fn)( !is_no, opt_name_loc, opt_value, opt_value_loc );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
