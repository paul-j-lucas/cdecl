/*
**      cdecl -- C gibberish translator
**      src/color.c
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
 * Defines functions for parsing color specifications.
 */

// local
#include "color.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>                      /* for fileno() */
#include <stdlib.h>                     /* for exit(), getenv() */
#include <string.h>                     /* for str...() */
#include <unistd.h>                     /* for isatty() */

///////////////////////////////////////////////////////////////////////////////

/**
 * Color capability used to map an CDECL_COLORS/GCC_COLORS "capability" either
 * to the variable to set or the function to call.
 */
struct color_cap {
  char const   *cap_name;
  char const  **cap_var_to_set;
};
typedef struct color_cap color_cap_t;

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
 * Color capabilities table.  Upper-case names are unique to us and upper-case
 * to avoid conflict with gcc.
 */
static color_cap_t const COLOR_CAPS[] = {
  { "caret",        &sgr_caret        },
  { "error",        &sgr_error        },
  { "HELP-keyword", &sgr_help_keyword },
  { "HELP-nonterm", &sgr_help_nonterm },
  { "HELP-punct",   &sgr_help_punct   },
  { "HELP-title",   &sgr_help_title   },
  { "locus",        &sgr_locus        },
  { "PROMPT",       &sgr_prompt       },
  { "warning",      &sgr_warning      },
  { NULL,           NULL              }
};

// local functions
static bool parse_sgr( char const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Sets the SGR color for the given capability.
 *
 * @param cap The color capability to set the color for.
 * @param sgr_color The SGR color to set; or null or empty to unset.
 * @return Returns \c true only if \a sgr_color is valid.
 */
static bool cap_set( color_cap_t const *cap, char const *sgr_color ) {
  assert( cap != NULL );
  assert( cap->cap_var_to_set != NULL );

  if ( sgr_color ) {
    if ( !*sgr_color )                  // empty string -> NULL = unset
      sgr_color = NULL;
    else if ( !parse_sgr( sgr_color ) )
      return false;
  }
  *cap->cap_var_to_set = sgr_color;
  return true;
}

/**
 * Parses an SGR (Select Graphic Rendition) value that matches the regular
 * expression of \c n(;n)* or a semicolon-separated list of integers in the
 * range 0-255.
 *
 * See: http://en.wikipedia.org/wiki/ANSI_escape_code
 *
 * @param sgr_color The NULL-terminated allegedly SGR string to parse.
 * @return Returns \c true only only if \a s contains a valid SGR value.
 */
static bool parse_sgr( char const *sgr_color ) {
  if ( !sgr_color )
    return false;
  for ( ;; ) {
    if ( !isdigit( *sgr_color ) )
      return false;
    char *end;
    errno = 0;
    uint64_t const n = strtoull( sgr_color, &end, 10 );
    if ( errno || n > 255 )
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
  bool set_something = false;

  if ( capabilities ) {
    // free this later since the sgr_* variables point to substrings
    char *next_cap = (char*)free_later( check_strdup( capabilities ) );

    for ( char *cap_name_val; (cap_name_val = strsep( &next_cap, ":" )); ) {
      char const *const cap_name = strsep( &cap_name_val, "=" );
      for ( color_cap_t const *cap = COLOR_CAPS; cap->cap_name; ++cap ) {
        if ( strcmp( cap_name, cap->cap_name ) == 0 ) {
          char const *const cap_value = strsep( &cap_name_val, "=" );
          if ( cap_set( cap, cap_value ) )
            set_something = true;
          break;
        }
      } // for
    } // for
  }
  return set_something;
}

bool should_colorize( color_when_t when ) {
  switch ( when ) {                     // handle easy cases
    case COLOR_ALWAYS: return true;
    case COLOR_NEVER : return false;
    default          : break;
  } // switch

  //
  // If TERM is unset, empty, or "dumb", color probably won't work.
  //
  char const *const term = getenv( "TERM" );
  if ( !term || !*term || strcmp( term, "dumb" ) == 0 )
    return false;

  int const fd_out = fileno( fout );
  if ( when == COLOR_ISATTY )           // emulate gcc's --color=auto
    return isatty( fd_out );

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
  return !is_file( fd_out );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
