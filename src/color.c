/*
**      cdecl -- C gibberish translator
**      src/color.c
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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
 * Defines functions for parsing color specifications.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define COLOR_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "color.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>                     /* for getenv() */
#include <string.h>                     /* for str...() */
#include <unistd.h>                     /* for isatty() */

//
// Color capabilities.  Names containing Upper-case are unique to cdecl and
// upper-case to avoid conflict with gcc.
//
#define COLOR_CAP_CARET           "caret"
#define COLOR_CAP_ERROR           "error"
#define COLOR_CAP_HELP_KEYWORD    "HELP-keyword"
#define COLOR_CAP_HELP_NONTERM    "HELP-nonterm"
#define COLOR_CAP_HELP_PUNCT      "HELP-punct"
#define COLOR_CAP_HELP_TITLE      "HELP-title"
#define COLOR_CAP_LOCUS           "locus"
#define COLOR_CAP_PROMPT          "PROMPT"
#define COLOR_CAP_WARNING         "warning"

/// @endcond

/**
 * @addtogroup printing-color-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Color capability used to map a `CDECL_COLORS` or `GCC_COLORS` "capability"
 * to the variable to set.
 */
struct color_cap_map {
  char const   *cap_name;               ///< Capability name.
  char const  **sgr_var;                ///< Pointer to SGR variable to set.
};
typedef struct color_cap_map color_cap_map_t;

// local functions
NODISCARD
static bool sgr_var_set( char const**, char const* );

// extern variable definitions
char const *sgr_caret;
char const *sgr_error;
char const *sgr_help_keyword;
char const *sgr_help_nonterm;
char const *sgr_help_punct;
char const *sgr_help_title;
char const *sgr_locus;
char const *sgr_prompt;
char const *sgr_warning;

////////// local functions ////////////////////////////////////////////////////

/**
 * Gets the \ref color_cap_map corresponding to \a name.
 *
 * @param name The name to get the corresponding \ref color_cap_map for.
 * @return Returns said \ref color_cap_map or NULL if \a name doesn't
 * correspond to any capability.
 */
static color_cap_map_t const* color_cap_find( char const *name ) {
  assert( name != NULL );

  static color_cap_map_t const COLOR_CAP_MAP[] = {
    { COLOR_CAP_CARET,        &sgr_caret        },
    { COLOR_CAP_ERROR,        &sgr_error        },
    { COLOR_CAP_HELP_KEYWORD, &sgr_help_keyword },
    { COLOR_CAP_HELP_NONTERM, &sgr_help_nonterm },
    { COLOR_CAP_HELP_PUNCT,   &sgr_help_punct   },
    { COLOR_CAP_HELP_TITLE,   &sgr_help_title   },
    { COLOR_CAP_LOCUS,        &sgr_locus        },
    { COLOR_CAP_PROMPT,       &sgr_prompt       },
    { COLOR_CAP_WARNING,      &sgr_warning      },
    { NULL,                   NULL              }
  };

  for ( color_cap_map_t const *m = COLOR_CAP_MAP; m->cap_name != NULL; ++m ) {
    if ( strcmp( name, m->cap_name ) == 0 )
      return m;
  } // for

  return NULL;
}

/**
 * Parses and sets the sequence of gcc color capabilities.
 *
 * @param capabilities The gcc capabilities to parse.  It's of the form:
 *  <table border="0">
 *    <tr><td>&nbsp;</td><td>&nbsp;</td></tr>
 *    <tr>
 *      <td><i>capapilities</i></td>
 *      <td>::= <i>capability</i> [<tt>:</tt><i>capability</i>]*</td>
 *    </tr>
 *    <tr>
 *      <td><i>capability</i></td>
 *      <td>::= <i>cap-name</i><tt>=</tt><i>sgr-list</i></td>
 *    </tr>
 *    <tr>
 *      <td><i>cap-name</i></td>
 *      <td>::= [<tt>a-zA-Z-</tt>]+</td>
 *    </tr>
 *    <tr>
 *      <td><i>sgr-list</i></td>
 *      <td>::= <i>sgr</i>[<tt>;</tt><i>sgr</i>]*</td>
 *    </tr>
 *    <tr>
 *      <td><i>sgr</i></td>
 *      <td>::= [<tt>1-9</tt>][<tt>0-9</tt>]*</td>
 *    </tr>
 *    <tr><td>&nbsp;</td><td>&nbsp;</td></tr>
 *  </table>
 * where <i>sgr</i> is a [Select Graphics
 * Rendition](https://en.wikipedia.org/wiki/ANSI_escape_code#SGR) code.  An
 * example \a capabilities is: `caret=42;1:error=41;1:warning=43;1`.
 * @return Returns `true` only if at least one capability was parsed
 * successfully.
 *
 * @warning If this function returns `true`, it must never be called again.
 */
NODISCARD
static bool colors_parse( char const *capabilities ) {
  static bool set_any;                  // set at least one?
  assert( !set_any );

  if ( capabilities == NULL )
    return false;
  char *const capabilities_dup = check_strdup( capabilities );

  for ( char *next_cap = capabilities_dup, *cap_name_val;
        (cap_name_val = strsep( &next_cap, SGR_CAP_SEP )) != NULL; ) {
    char const *const cap_name = strsep( &cap_name_val, "=" );
    color_cap_map_t const *const cap = color_cap_find( cap_name );
    if ( cap != NULL ) {
      char const *const cap_value = strsep( &cap_name_val, "=" );
      if ( sgr_var_set( cap->sgr_var, cap_value ) )
        set_any = true;
    }
  } // for

  if ( set_any )
    free_later( capabilities_dup );     // sgr_* variables point to substrings
  else
    free( capabilities_dup );

  return set_any;
}

/**
 * Parses an SGR (Select Graphic Rendition) value that matches the regular
 * expression of `n(;n)*` or a semicolon-separated list of integers in the
 * range 0-255.
 *
 * @param sgr_color The null-terminated allegedly SGR string to parse.
 * @return Returns `true` only if \a sgr_color contains a valid SGR value.
 *
 * @sa [ANSI escape code](http://en.wikipedia.org/wiki/ANSI_escape_code)
 */
NODISCARD
static bool sgr_is_valid( char const *sgr_color ) {
  if ( sgr_color == NULL )
    return false;
  for (;;) {
    if ( unlikely( !isdigit( *sgr_color ) ) )
      return false;
    char *end;
    errno = 0;
    unsigned long long const n = strtoull( sgr_color, &end, 10 );
    if ( unlikely( errno != 0 || n > 255 ) )
      return false;
    switch ( *end ) {
      case '\0':
        return true;
      case ';':
        sgr_color = end + 1;
        continue;
      default:
        return false;
    } // switch
  } // for
}

/**
 * Sets an SGR variable to the given color.
 *
 * @param sgr_var A pointer to the SGR variable to set.
 * @param sgr_color The SGR color to set; or NULL or empty to unset.
 * @return Returns `true` only if \a sgr_color is valid, NULL, or empty.
 */
NODISCARD
static bool sgr_var_set( char const **sgr_var, char const *sgr_color ) {
  assert( sgr_var != NULL );
  sgr_color = null_if_empty( sgr_color );
  if ( sgr_color != NULL && !sgr_is_valid( sgr_color ) )
    return false;
  *sgr_var = sgr_color;
  return true;
}

/**
 * Determines whether we should emit escape sequences for color.
 *
 * @param when The \ref color_when value.
 * @return Returns `true` only if we should do color.
 */
NODISCARD
static bool should_colorize( color_when_t when ) {
  switch ( when ) {
    case COLOR_ALWAYS:
      return true;
    case COLOR_ISATTY:
      return isatty( STDOUT_FILENO );   // emulate gcc's --color=auto
    case COLOR_NEVER:
      return false;
    case COLOR_NOT_FILE:
      //
      // Do color only we're writing either to a TTY or to a pipe (so the
      // common case of piping to less(1) will still show color) but NOT when
      // writing to a file because we don't want the escape sequences polluting
      // it.
      //
      // Results from testing using isatty(3) and fstat(3) are given in the
      // following table:
      //
      //      COMMAND      Should? isatty ISCHR ISFIFO ISREG
      //      ============ ======= ====== ===== ====== =====
      //      cdecl           T      T      T     F      F
      //      cdecl > file    F      F      F     F    >>T<<
      //      cdecl | less    T      F      F     T      F
      //
      // Hence, we want to do color _except_ when ISREG=T.
      //
      return !fd_is_file( STDOUT_FILENO );
  } // switch
  UNEXPECTED_INT_VALUE( when );
}

////////// extern functions ///////////////////////////////////////////////////

void color_init( void ) {
  ASSERT_RUN_ONCE();

  if ( !should_colorize( opt_color_when ) )
    return;
  if ( colors_parse( getenv( "CDECL_COLORS" ) ) )
    return;
  if ( colors_parse( getenv( "GCC_COLORS"  ) ) )
    return;

  static char const COLORS_DEFAULT[] =
    COLOR_CAP_CARET         "=" SGR_FG_GREEN  SGR_SEP SGR_BOLD  SGR_CAP_SEP
    COLOR_CAP_ERROR         "=" SGR_FG_RED    SGR_SEP SGR_BOLD  SGR_CAP_SEP
    COLOR_CAP_HELP_KEYWORD  "="                       SGR_BOLD  SGR_CAP_SEP
    COLOR_CAP_HELP_NONTERM  "=" SGR_FG_CYAN                     SGR_CAP_SEP
    COLOR_CAP_HELP_PUNCT    "=" SGR_FG_BLACK  SGR_SEP SGR_BOLD  SGR_CAP_SEP
    COLOR_CAP_HELP_TITLE    "=" SGR_FG_BLUE   SGR_SEP SGR_BOLD  SGR_CAP_SEP
    COLOR_CAP_LOCUS         "="                       SGR_BOLD  SGR_CAP_SEP
    COLOR_CAP_PROMPT        "=" SGR_FG_GREEN                    SGR_CAP_SEP
    COLOR_CAP_WARNING       "=" SGR_FG_YELLOW SGR_SEP SGR_BOLD  ;

  PJL_IGNORE_RV( colors_parse( COLORS_DEFAULT ) );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
