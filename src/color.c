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
#include "cdecl.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>                      /* for fileno() */
#include <stdlib.h>                     /* for getenv() */
#include <string.h>                     /* for str...() */
#include <unistd.h>                     /* for isatty() */

//
// Color capabilities.  Names containing Upper-case are unique to cdecl and
// upper-case to avoid conflict with gcc.
//
#define CAP_CARET               "caret"
#define CAP_ERROR               "error"
#define CAP_HELP_KEYWORD        "HELP-keyword"
#define CAP_HELP_NONTERM        "HELP-nonterm"
#define CAP_HELP_PUNCT          "HELP-punct"
#define CAP_HELP_TITLE          "HELP-title"
#define CAP_LOCUS               "locus"
#define CAP_PROMPT              "PROMPT"
#define CAP_WARNING             "warning"

/// @endcond

/**
 * @addtogroup sgr-group
 * @{
 */

#define SGR_BG_BLACK        "40"        /**< Background black.            */
#define SGR_BG_RED          "41"        /**< Background red.              */
#define SGR_BG_GREEN        "42"        /**< Background green.            */
#define SGR_BG_YELLOW       "43"        /**< Background yellow.           */
#define SGR_BG_BLUE         "44"        /**< Background blue.             */
#define SGR_BG_MAGENTA      "45"        /**< Background magenta.          */
#define SGR_BG_CYAN         "46"        /**< Background cyan.             */
#define SGR_BG_WHITE        "47"        /**< Background white.            */

#define SGR_FG_BLACK        "30"        /**< Foreground black.            */
#define SGR_FG_RED          "31"        /**< Foreground red.              */
#define SGR_FG_GREEN        "32"        /**< Foreground green.            */
#define SGR_FG_YELLOW       "33"        /**< Foreground yellow.           */
#define SGR_FG_BLUE         "34"        /**< Foreground blue.             */
#define SGR_FG_MAGENTA      "35"        /**< Foreground magenta.          */
#define SGR_FG_CYAN         "36"        /**< Foreground cyan.             */
#define SGR_FG_WHITE        "37"        /**< Foreground white.            */

#define SGR_BOLD            "1"         /**< Bold.                        */
#define SGR_CAP_SEP         ":"         /**< Capability separator.        */
#define SGR_SEP             ";"         /**< Attribute/RGB separator.     */

/** @} */

/**
 * @addtogroup printing-color-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Color capability used to map a `CDECL_COLORS` or `GCC_COLORS` "capability"
 * to the variable to set.
 */
struct color_cap {
  char const   *cap_name;               ///< Capability name.
  char const  **cap_var_to_set;         ///< Pointer to variable to set.
};
typedef struct color_cap color_cap_t;

// extern constant definitions
char const  COLORS_DEFAULT[] =
  CAP_CARET         "=" SGR_FG_GREEN  SGR_SEP SGR_BOLD  SGR_CAP_SEP
  CAP_ERROR         "=" SGR_FG_RED    SGR_SEP SGR_BOLD  SGR_CAP_SEP
  CAP_HELP_KEYWORD  "="                       SGR_BOLD  SGR_CAP_SEP
  CAP_HELP_NONTERM  "=" SGR_FG_CYAN                     SGR_CAP_SEP
  CAP_HELP_PUNCT    "=" SGR_FG_BLACK  SGR_SEP SGR_BOLD  SGR_CAP_SEP
  CAP_HELP_TITLE    "=" SGR_FG_BLUE   SGR_SEP SGR_BOLD  SGR_CAP_SEP
  CAP_LOCUS         "="                       SGR_BOLD  SGR_CAP_SEP
  CAP_PROMPT        "=" SGR_FG_GREEN                    SGR_CAP_SEP
  CAP_WARNING       "=" SGR_FG_YELLOW SGR_SEP SGR_BOLD  ;

// extern variable definitions
bool        colorize;
char const *sgr_caret;
char const *sgr_error;
char const *sgr_help_keyword;
char const *sgr_help_nonterm;
char const *sgr_help_punct;
char const *sgr_help_title;
char const *sgr_locus;
char const *sgr_prompt;
char const *sgr_warning;

/**
 * Color capabilities table.
 */
static color_cap_t const COLOR_CAPS[] = {
  { CAP_CARET,        &sgr_caret        },
  { CAP_ERROR,        &sgr_error        },
  { CAP_HELP_KEYWORD, &sgr_help_keyword },
  { CAP_HELP_NONTERM, &sgr_help_nonterm },
  { CAP_HELP_PUNCT,   &sgr_help_punct   },
  { CAP_HELP_TITLE,   &sgr_help_title   },
  { CAP_LOCUS,        &sgr_locus        },
  { CAP_PROMPT,       &sgr_prompt       },
  { CAP_WARNING,      &sgr_warning      },
  { NULL,             NULL              }
};

// local functions
NODISCARD
static bool sgr_is_valid( char const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Sets the SGR color for the given capability.
 *
 * @param cap The color capability to set the color for.
 * @param sgr_color The SGR color to set; or NULL or empty to unset.
 * @return Returns `true` only if \a sgr_color is valid.
 */
NODISCARD
static bool sgr_cap_set( color_cap_t const *cap, char const *sgr_color ) {
  assert( cap != NULL );
  assert( cap->cap_var_to_set != NULL );

  if ( sgr_color != NULL ) {
    if ( *sgr_color == '\0' )           // empty string -> NULL = unset
      sgr_color = NULL;
    else if ( !sgr_is_valid( sgr_color ) )
      return false;
  }
  *cap->cap_var_to_set = sgr_color;
  return true;
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

////////// extern functions ///////////////////////////////////////////////////

bool colors_parse( char const *capabilities ) {
  static bool set_any;                  // set at least one?
  assert( !set_any );

  if ( capabilities == NULL )
    return false;
  char *const capabilities_dup = check_strdup( capabilities );

  for ( char *next_cap = capabilities_dup, *cap_name_val;
        (cap_name_val = strsep( &next_cap, ":" )) != NULL; ) {
    char const *const cap_name = strsep( &cap_name_val, "=" );
    for ( color_cap_t const *cap = COLOR_CAPS; cap->cap_name; ++cap ) {
      if ( strcmp( cap_name, cap->cap_name ) == 0 ) {
        char const *const cap_value = strsep( &cap_name_val, "=" );
        if ( sgr_cap_set( cap, cap_value ) )
          set_any = true;
        break;
      }
    } // for
  } // for

  if ( set_any )
    free_later( capabilities_dup );     // sgr_* variables point to substrings
  else
    free( capabilities_dup );

  return set_any;
}

bool should_colorize( color_when_t when ) {
  switch ( when ) {                     // handle easy cases
    case COLOR_ALWAYS: return true;
    case COLOR_NEVER : return false;
    default          : break;
  } // switch

  int const fd = fileno( stdout );
  if ( when == COLOR_ISATTY )           // emulate gcc's --color=auto
    return isatty( fd );

  assert( when == COLOR_NOT_FILE );
  //
  // Otherwise we want to do color only we're writing either to a TTY or to a
  // pipe (so the common case of piping to less(1) will still show color) but
  // NOT when writing to a file because we don't want the escape sequences
  // polluting it.
  //
  // Results from testing using isatty(3) and fstat(3) are given in the
  // following table:
  //
  //    COMMAND      Should? isatty ISCHR ISFIFO ISREG
  //    ============ ======= ====== ===== ====== =====
  //    cdecl           T      T      T     F      F
  //    cdecl > file    F      F      F     F    >>T<<
  //    cdecl | less    T      F      F     T      F
  //
  // Hence, we want to do color _except_ when ISREG=T.
  //
  return !fd_is_file( fd );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
