/*
**      cdecl -- C gibberish translator
**      src/set_options.c
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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
 * Defines types and functions that implement the cdecl `set` command.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "set_options.h"
#include "c_lang.h"
#include "options.h"
#include "print.h"
#include "prompt.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdio.h>
#include <string.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// local functions
static void set_alt_tokens( bool, c_loc_t const*, char const*, c_loc_t const* );
#ifdef YYDEBUG
static void set_bison_debug( bool, c_loc_t const*, char const*, c_loc_t const* );
#endif /* YYDEBUG */
#ifdef ENABLE_CDECL_DEBUG
static void set_debug( bool, c_loc_t const*, char const*, c_loc_t const* );
#endif /* ENABLE_CDECL_DEBUG */
static void set_digraphs( bool, c_loc_t const*, char const*, c_loc_t const* );
static void set_east_const( bool, c_loc_t const*, char const*, c_loc_t const* );
static void set_explain_by_default( bool, c_loc_t const*, char const*, c_loc_t const* );
static void set_explicit_int( bool, c_loc_t const*, char const*, c_loc_t const* );
#ifdef ENABLE_FLEX_DEBUG
static void set_flex_debug( bool, c_loc_t const*, char const*, c_loc_t const* );
#endif /* ENABLE_FLEX_DEBUG */
static void set_lang( bool, c_loc_t const*, char const*, c_loc_t const* );
static bool set_lang_impl( char const* );
static void set_prompt( bool, c_loc_t const*, char const*, c_loc_t const* );
static void set_semicolon( bool, c_loc_t const*, char const*, c_loc_t const* );
static void set_trigraphs( bool, c_loc_t const*, char const*, c_loc_t const* );

///////////////////////////////////////////////////////////////////////////////

/**
 * cdecl `set` options.
 */
static set_option_t const SET_OPTIONS[] = {
  { "alt-tokens",         SET_TOGGLE,   false,  &set_alt_tokens         },
#ifdef YYDEBUG
  { "bison-debug",        SET_TOGGLE,   false,  &set_bison_debug        },
#endif /* YYDEBUG */
#ifdef ENABLE_CDECL_DEBUG
  { "debug",              SET_TOGGLE,   false,  &set_debug              },
#endif /* ENABLE_CDECL_DEBUG */
  { "digraphs",           SET_AFF_ONLY, false,  &set_digraphs           },
  { "graphs",             SET_NEG_ONLY, false,  &set_digraphs           },
  { "east-const",         SET_TOGGLE,   false,  &set_east_const         },
  { "explain-by-default", SET_TOGGLE,   false,  &set_explain_by_default },
  { "explicit-int",       SET_TOGGLE,   true,   &set_explicit_int       },
#ifdef ENABLE_FLEX_DEBUG
  { "flex-debug",         SET_TOGGLE,   false,  &set_flex_debug         },
#endif /* ENABLE_FLEX_DEBUG */
  { "lang",               SET_AFF_ONLY, true,   &set_lang               },
  { "prompt",             SET_TOGGLE,   false,  &set_prompt             },
  { "semicolon",          SET_TOGGLE,   false,  &set_semicolon          },
  { "trigraphs",          SET_AFF_ONLY, false,  &set_trigraphs          },
  { NULL,                 SET_TOGGLE,   false,  NULL                    }
};

////////// local functions ////////////////////////////////////////////////////

static inline char const* maybe_no( bool enabled ) {
  return enabled ? "  " : "no";
}

/**
 * Prints the current option settings.
 */
static void print_options( void ) {
  printf( "  %salt-tokens\n", maybe_no( opt_alt_tokens ) );
#ifdef YYDEBUG
  printf( "  %sbison-debug\n", maybe_no( opt_bison_debug ) );
#endif /* YYDEBUG */
#ifdef ENABLE_CDECL_DEBUG
  printf( "  %sdebug\n", maybe_no( opt_cdecl_debug ) );
#endif /* ENABLE_CDECL_DEBUG */
  printf( "  %seast-const\n", maybe_no( opt_east_const ) );
  printf( "  %sexplain-by-default\n", maybe_no( opt_explain ) );

  if ( any_explicit_int() ) {
    PUTS( "    explicit-int=" );
    print_opt_explicit_int();
    PUTC( '\n' );
  } else {
    PUTS( "  noexplicit-int\n" );
  }

#ifdef ENABLE_FLEX_DEBUG
  printf( "  %sflex-debug\n", maybe_no( opt_flex_debug ) );
#endif /* ENABLE_FLEX_DEBUG */
  printf( " %sgraphs\n", opt_graph == C_GRAPH_DI ? " di" : opt_graph == C_GRAPH_TRI ? "tri" : " no" );
  printf( "    lang=%s\n", C_LANG_NAME() );
  printf( "  %sprompt\n", maybe_no( cdecl_prompt[0][0] != '\0' ) );
  printf( "  %ssemicolon\n", maybe_no( opt_semicolon ) );
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

#ifdef YYDEBUG
/**
 * Sets the Bison debugging option.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_bison_debug( bool enabled, c_loc_t const *opt_name_loc,
                             char const *opt_value,
                             c_loc_t const *opt_value_loc ) {
  (void)opt_name_loc;
  (void)opt_value;
  (void)opt_value_loc;
  opt_bison_debug = enabled;
}
#endif /* YYDEBUG */

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
  opt_cdecl_debug = enabled;
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
  if ( opt_graph && opt_lang < LANG_C_95 ) {
    print_warning( opt_name_loc,
      "digraphs are not supported until %s\n", c_lang_name( LANG_C_95 )
    );
  }
}

/**
 * Sets the east-const option.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_east_const( bool enabled, c_loc_t const *opt_name_loc,
                            char const *opt_value,
                            c_loc_t const *opt_value_loc ) {
  (void)opt_name_loc;
  (void)opt_value;
  (void)opt_value_loc;
  opt_east_const = enabled;
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
    parse_opt_explicit_int( NULL, "" );
}

#ifdef ENABLE_FLEX_DEBUG
/**
 * Sets the Flex debugging option.
 *
 * @param enabled True if enabled.
 * @param opt_name_loc The location of the option name.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value.
 */
static void set_flex_debug( bool enabled, c_loc_t const *opt_name_loc,
                            char const *opt_value,
                            c_loc_t const *opt_value_loc ) {
  (void)opt_name_loc;
  (void)opt_value;
  (void)opt_value_loc;
  opt_flex_debug = enabled;
}
#endif /* ENABLE_FLEX_DEBUG */

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

  if ( !set_lang_impl( opt_value ) )
    print_error( opt_value_loc, "\"%s\": unknown language\n", opt_value );
}

/**
 * Sets the current language.
 *
 * @param name The language name.
 * @return Returns `true` only if \a name corresponds to a supported language
 * and the language was set.
 */
static bool set_lang_impl( char const *name ) {
  c_lang_id_t const new_lang_id = c_lang_find( name );
  if ( new_lang_id != LANG_NONE ) {
    c_lang_set( new_lang_id );
    if ( opt_graph == C_GRAPH_TRI ) {
      //
      // Every time the language changes, re-set trigraph mode so the user is
      // warned that trigraphs are not supported if the language is C++17 or
      // later.
      //
      set_trigraphs( /*enabled=*/true, NULL, NULL, NULL );
    }
    return true;
  }
  return false;
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
  if ( enabled && opt_lang >= LANG_CPP_17 ) {
    print_warning( opt_name_loc,
      "trigraphs are no longer supported in %s\n", C_LANG_NAME()
    );
  }
}

/**
 * Compares strings for at most \a n characters ignoring hyphens for equality.
 *
 * @param s1 The first string.
 * @param s2 The second string.
 * @param n The maximum number of characters to check.
 * @return Returns `true` only if \a s1 equals \a s2 (ignoring hyphens)
 * for \a n characters.
 */
static bool strn_nohyphen_eq( char const *s1, char const *s2, size_t n ) {
  while ( n-- > 0 ) {
    if ( *s1 == '-' )
      ++s1;
    else if ( *s2 == '-' )
      ++s2;
    else if ( *s1++ != *s2++ )
      return false;
  } // while
  return true;
}

////////// extern functions ///////////////////////////////////////////////////

PJL_WARN_UNUSED_RESULT
set_option_t const* option_next( set_option_t const *opt ) {
  if ( opt == NULL )
    opt = SET_OPTIONS;
  else if ( (++opt)->name == NULL )
    opt = NULL;
  return opt;
}

void set_option( char const *opt_name, c_loc_t const *opt_name_loc,
                 char const *opt_value, c_loc_t const *opt_value_loc ) {
  if ( opt_name == NULL || strcmp( opt_name, "options" ) == 0 ) {
    print_options();
    return;
  }

  if ( set_lang_impl( opt_name ) )
    return;

  assert( opt_name_loc != NULL );
  assert( opt_value == NULL || opt_value_loc != NULL );

  char const *const orig_name = opt_name;
  bool const is_no = strncmp( opt_name, "no", 2 ) == 0;
  if ( is_no )
    opt_name += 2/*no*/;
  size_t const opt_name_len = strlen( opt_name );

  set_option_t const *found_opt = NULL;
  for ( set_option_t const *opt = SET_OPTIONS; opt->name != NULL; ++opt ) {
    if ( strn_nohyphen_eq( opt->name, opt_name, opt_name_len ) ) {
      if ( found_opt != NULL ) {
        print_error( opt_name_loc,
          "\"%s\": ambiguous set option; could be \"%s%s\" or \"%s%s\"\n",
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
    print_error( opt_name_loc, "\"%s\": unknown set option\n", orig_name );
    return;
  }

  switch ( found_opt->type ) {
    case SET_TOGGLE:
      break;
    case SET_AFF_ONLY:
      if ( is_no ) {
        print_error( opt_name_loc,
          "\"no\" not valid for \"%s\"\n", found_opt->name
        );
        return;
      }
      break;
    case SET_NEG_ONLY:
      if ( !is_no ) {
        print_error( opt_name_loc,
          "\"no\" required for \"%s\"\n", found_opt->name
        );
        return;
      }
      break;
  } // switch

  if ( opt_value == NULL ) {
    if ( !is_no && found_opt->takes_value ) {
      print_error( opt_name_loc,
        "\"%s\" set option requires =<value>\n",
        orig_name
      );
      return;
    }
  } else {
    if ( is_no ) {
      print_error( opt_value_loc, "\"no\" set options take no value\n" );
      return;
    }
    if ( !found_opt->takes_value ) {
      print_error( opt_value_loc,
        "\"%s\": set option \"%s\" takes no value\n",
        opt_value, orig_name
      );
      return;
    }
  }

  (*found_opt->set_fn)( !is_no, opt_name_loc, opt_value, opt_value_loc );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
