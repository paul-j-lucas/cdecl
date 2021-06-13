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
#include "did_you_mean.h"
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

/**
 * Convenience `struct` for passing arguments to a `set_*()` function.
 */
struct set_option_fn_args {
  bool           opt_enabled;           ///< True if the option is enabled.
  c_loc_t const *opt_name_loc;          ///< The location of the option name.
  char const    *opt_value;             ///< The option value, if any.
  c_loc_t const *opt_value_loc;         ///< The location of \a opt_value.
};

/**
 * Convenience macro for declaring a `set` option function.
 *
 * @param NAME The option name with `-` replaced by `_`.
 */
#define DECLARE_SET_OPTION_FN(NAME) \
  static void set_##NAME( set_option_fn_args_t const* )

// local functions
DECLARE_SET_OPTION_FN( alt_tokens );
#ifdef YYDEBUG
DECLARE_SET_OPTION_FN( bison_debug );
#endif /* YYDEBUG */
#ifdef ENABLE_CDECL_DEBUG
DECLARE_SET_OPTION_FN( debug );
#endif /* ENABLE_CDECL_DEBUG */
DECLARE_SET_OPTION_FN( digraphs );
DECLARE_SET_OPTION_FN( east_const );
DECLARE_SET_OPTION_FN( explain_by_default );
DECLARE_SET_OPTION_FN( explicit_int );
#ifdef ENABLE_FLEX_DEBUG
DECLARE_SET_OPTION_FN( flex_debug );
#endif /* ENABLE_FLEX_DEBUG */
DECLARE_SET_OPTION_FN( lang );
DECLARE_SET_OPTION_FN( prompt );
DECLARE_SET_OPTION_FN( semicolon );
DECLARE_SET_OPTION_FN( trigraphs );

PJL_WARN_UNUSED_RESULT
static bool set_lang_impl( char const* );

///////////////////////////////////////////////////////////////////////////////

/**
 * cdecl `set` options.
 */
static set_option_t const SET_OPTIONS[] = {
  { "alt-tokens",         SET_OPT_TOGGLE,   false,  &set_alt_tokens         },
#ifdef YYDEBUG
  { "bison-debug",        SET_OPT_TOGGLE,   false,  &set_bison_debug        },
#endif /* YYDEBUG */
#ifdef ENABLE_CDECL_DEBUG
  { "debug",              SET_OPT_TOGGLE,   false,  &set_debug              },
#endif /* ENABLE_CDECL_DEBUG */
  { "digraphs",           SET_OPT_AFF_ONLY, false,  &set_digraphs           },
  { "graphs",             SET_OPT_NEG_ONLY, false,  &set_digraphs           },
  { "east-const",         SET_OPT_TOGGLE,   false,  &set_east_const         },
  { "explain-by-default", SET_OPT_TOGGLE,   false,  &set_explain_by_default },
  { "explicit-int",       SET_OPT_TOGGLE,   true,   &set_explicit_int       },
#ifdef ENABLE_FLEX_DEBUG
  { "flex-debug",         SET_OPT_TOGGLE,   false,  &set_flex_debug         },
#endif /* ENABLE_FLEX_DEBUG */
  { "lang",               SET_OPT_AFF_ONLY, true,   &set_lang               },
  { "prompt",             SET_OPT_TOGGLE,   false,  &set_prompt             },
  { "semicolon",          SET_OPT_TOGGLE,   false,  &set_semicolon          },
  { "trigraphs",          SET_OPT_AFF_ONLY, false,  &set_trigraphs          },
  { NULL,                 SET_OPT_TOGGLE,   false,  NULL                    }
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Convenience function for getting `"no"` or not to print.
 *
 * @param enabled Whether a toggle option is enabled.
 * @return If \a enabled is `true`, returns `" "` (two spaces); if `false`,
 * returns `"no"`.
 */
PJL_WARN_UNUSED_RESULT
static inline char const* maybe_no( bool enabled ) {
  return enabled ? "  " : "no";
}

/**
 * Prints the current option settings.
 *
 * @param out The `FILE` to print to.
 */
static void print_options( FILE *out ) {
  FPRINTF( out, "  %salt-tokens\n", maybe_no( opt_alt_tokens ) );
#ifdef YYDEBUG
  FPRINTF( out, "  %sbison-debug\n", maybe_no( opt_bison_debug ) );
#endif /* YYDEBUG */
#ifdef ENABLE_CDECL_DEBUG
  FPRINTF( out, "  %sdebug\n", maybe_no( opt_cdecl_debug ) );
#endif /* ENABLE_CDECL_DEBUG */
  FPRINTF( out, "  %seast-const\n", maybe_no( opt_east_const ) );
  FPRINTF( out, "  %sexplain-by-default\n", maybe_no( opt_explain ) );

  if ( any_explicit_int() ) {
    FPUTS( "    explicit-int=", out );
    print_explicit_int( out );
    FPUTC( '\n', out );
  } else {
    FPUTS( "  noexplicit-int\n", out );
  }

#ifdef ENABLE_FLEX_DEBUG
  FPRINTF( out, "  %sflex-debug\n", maybe_no( opt_flex_debug ) );
#endif /* ENABLE_FLEX_DEBUG */
  FPRINTF( out, " %sgraphs\n", opt_graph == C_GRAPH_DI ? " di" : opt_graph == C_GRAPH_TRI ? "tri" : " no" );
  FPRINTF( out, "    lang=%s\n", c_lang_name( opt_lang ) );
  FPRINTF( out, "  %sprompt\n", maybe_no( cdecl_prompt[0][0] != '\0' ) );
  FPRINTF( out, "  %ssemicolon\n", maybe_no( opt_semicolon ) );
}

/**
 * Sets the alt-tokens option.
 *
 * @param args The set option arguments.
 */
static void set_alt_tokens( set_option_fn_args_t const *args ) {
  opt_alt_tokens = args->opt_enabled;
}

#ifdef YYDEBUG
/**
 * Sets the Bison debugging option.
 *
 * @param args The set option arguments.
 */
static void set_bison_debug( set_option_fn_args_t const *args ) {
  opt_bison_debug = args->opt_enabled;
}
#endif /* YYDEBUG */

#ifdef ENABLE_CDECL_DEBUG
/**
 * Sets the debug option.
 *
 * @param args The set option arguments.
 */
static void set_debug( set_option_fn_args_t const *args ) {
  opt_cdecl_debug = args->opt_enabled;
}
#endif /* ENABLE_CDECL_DEBUG */

/**
 * Sets the digraphs-tokens option.
 *
 * @param args The set option arguments.
 */
static void set_digraphs( set_option_fn_args_t const *args ) {
  opt_graph = args->opt_enabled ? C_GRAPH_DI : C_GRAPH_NONE;
  if ( opt_graph && opt_lang < LANG_C_95 ) {
    print_warning( args->opt_name_loc,
      "digraphs are not supported until C95\n"
    );
  }
}

/**
 * Sets the east-const option.
 *
 * @param args The set option arguments.
 */
static void set_east_const( set_option_fn_args_t const *args ) {
  opt_east_const = args->opt_enabled;
}

/**
 * Sets the explain-by-default option.
 *
 * @param args The set option arguments.
 */
static void set_explain_by_default( set_option_fn_args_t const *args ) {
  opt_explain = args->opt_enabled;
}

/**
 * Sets the explicit-int option.
 *
 * @param args The set option arguments.
 */
static void set_explicit_int( set_option_fn_args_t const *args ) {
  if ( args->opt_enabled )
    parse_explicit_int( args->opt_value, args->opt_value_loc );
  else
    parse_explicit_int( "", /*loc=*/NULL );
}

#ifdef ENABLE_FLEX_DEBUG
/**
 * Sets the Flex debugging option.
 *
 * @param args The set option arguments.
 */
static void set_flex_debug( set_option_fn_args_t const *args ) {
  opt_flex_debug = args->opt_enabled;
}
#endif /* ENABLE_FLEX_DEBUG */

/**
 * Sets the current language.
 *
 * @param args The set option arguments.
 */
static void set_lang( set_option_fn_args_t const *args ) {
  assert( args->opt_enabled );
  if ( !set_lang_impl( args->opt_value ) ) {
    print_error( args->opt_value_loc,
      "\"%s\": unknown language\n", args->opt_value
    );
  }
}

/**
 * Sets the current language.
 *
 * @param name The language name.
 * @return Returns `true` only if \a name corresponds to a supported language
 * and the language was set.
 */
PJL_WARN_UNUSED_RESULT
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
      set_trigraphs( &(set_option_fn_args_t){ true, NULL, NULL, NULL } );
    }
    return true;
  }
  return false;
}

/**
 * Sets the prompt option.
 *
 * @param args The set option arguments.
 */
static void set_prompt( set_option_fn_args_t const *args ) {
  cdecl_prompt_enable( args->opt_enabled );
}

/**
 * Sets the semicolon option.
 *
 * @param args The set option arguments.
 */
static void set_semicolon( set_option_fn_args_t const *args ) {
  opt_semicolon = args->opt_enabled;
}

/**
 * Sets the trigraphs option.
 *
 * @param args The set option arguments.
 */
static void set_trigraphs( set_option_fn_args_t const *args ) {
  opt_graph = args->opt_enabled ? C_GRAPH_TRI : C_GRAPH_NONE;
  if ( args->opt_enabled && opt_lang >= LANG_CPP_17 ) {
    print_warning( args->opt_name_loc,
      "trigraphs are no longer supported in C++17\n"
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
PJL_WARN_UNUSED_RESULT
static bool strn_nohyphen_equal( char const *s1, char const *s2, size_t n ) {
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

void option_set( char const *opt_name, c_loc_t const *opt_name_loc,
                 char const *opt_value, c_loc_t const *opt_value_loc ) {
  if ( opt_name == NULL || strcmp( opt_name, "options" ) == 0 ) {
    print_options( fout );
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
    if ( strn_nohyphen_equal( opt->name, opt_name, opt_name_len ) ) {
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
    print_error( opt_name_loc, "\"%s\": unknown set option", orig_name );
    print_suggestions( DYM_SET_OPTIONS, orig_name );
    EPUTC( '\n' );
    return;
  }

  switch ( found_opt->type ) {
    case SET_OPT_TOGGLE:
      break;
    case SET_OPT_AFF_ONLY:
      if ( is_no ) {
        print_error( opt_name_loc,
          "\"no\" not valid for \"%s\"\n", found_opt->name
        );
        return;
      }
      break;
    case SET_OPT_NEG_ONLY:
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

  set_option_fn_args_t const args = {
    !is_no, opt_name_loc, opt_value, opt_value_loc
  };
  (*found_opt->set_fn)( &args );
}

set_option_t const* set_option_next( set_option_t const *opt ) {
  if ( opt == NULL )
    opt = SET_OPTIONS;
  else if ( (++opt)->name == NULL )
    opt = NULL;
  return opt;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
