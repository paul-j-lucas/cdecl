/*
**      cdecl -- C gibberish translator
**      src/color.c
**
**      Copyright (C) 2017-2026  Paul J. Lucas
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
#include "color.h"
#include "options.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>                     /* for getenv() */
#include <string.h>                     /* for str...() */
#include <sys/stat.h>                   /* for stat() */
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
#define COLOR_CAP_MACRO_NO_EXPAND "MACRO-no-expand"
#define COLOR_CAP_MACRO_PUNCT     "MACRO-punct"
#define COLOR_CAP_MACRO_SUBST     "MACRO-subst"
#define COLOR_CAP_PROMPT          "PROMPT"
#define COLOR_CAP_WARNING         "warning"

/// @endcond

/**
 * @addtogroup printing-color-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

NODISCARD
static char const** sgr_var_find( char const* );

NODISCARD
static bool         sgr_var_set( char const**, char const* );

////////// extern variables ///////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries.

// extern variable definitions
char const *sgr_caret;
char const *sgr_error;
char const *sgr_help_keyword;
char const *sgr_help_nonterm;
char const *sgr_help_punct;
char const *sgr_help_title;
char const *sgr_locus;
char const *sgr_macro_no_expand;
char const *sgr_macro_punct;
char const *sgr_macro_subst;
char const *sgr_prompt;
char const *sgr_warning;

/// @endcond

////////// local variables ////////////////////////////////////////////////////

static char const *color_capabilities;  ///< Parsed color capabilities.

/**
 * Indexed by a file decriptor, whether we should emit color on it.
 *
 * @note Element 0 for `stdin` is not used.
 */
static bool fd_in_color[3];

////////// local functions ////////////////////////////////////////////////////

/**
 * Cleans-up all memory used by colors at program termination.
 *
 * @note This function is called only via **atexit**(3).
 *
 * @sa colors_init()
 */
static void colors_cleanup( void ) {
  FREE( color_capabilities );
}

/**
 * Parses and sets the sequence of SGR color capabilities.
 *
 * @param capabilities The SGR capabilities to parse.  It's of the form:
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
 * example \a capabilities is: `caret=42;1:error=41;1:warning=43;1`.  If NULL,
 * does nothing.
 * @return Returns a pointer to the parsed color capabilities string only if at
 * least one capability was parsed successfully.  The caller is responsible for
 * freeing it.  Otherwise returns NULL.
 *
 * @warning If this function returns non-NULL, it must never be called again.
 */
NODISCARD
static char const* colors_parse( char const *capabilities ) {
  if ( null_if_empty( capabilities ) == NULL )
    return NULL;

  char *const capabilities_dup = check_strdup( capabilities );
  bool set_any = false;

  for ( char *next_cap = capabilities_dup, *cap_name_val;
        (cap_name_val = strsep( &next_cap, SGR_CAP_SEP )) != NULL; ) {
    char const *const cap_name = strsep( &cap_name_val, "=" );
    char const **const sgr_var = sgr_var_find( cap_name );
    if ( sgr_var != NULL ) {
      char const *const cap_value = strsep( &cap_name_val, "=" );
      if ( sgr_var_set( sgr_var, cap_value ) )
        set_any = true;
    }
  } // for

  if ( set_any )
    return capabilities_dup;

  free( capabilities_dup );
  return NULL;
}

/**
 * Checks whether \a fd refers to a regular file.
 *
 * @param fd The file descriptor to check.
 * @return Returns `true` only if \a fd refers to a regular file.
 */
NODISCARD
static bool fd_is_file( int fd ) {
  struct stat fd_stat;
  if ( fstat( fd, &fd_stat ) == -1 )
    return false;
  return S_ISREG( fd_stat.st_mode );
}

/**
 * Determines whether we should emit escape sequences for color for \a fd.
 *
 * @param fd The file desciptor to check.
 * @param when The \ref color_when value.
 * @return Returns `true` only if we should do color for \a fd.
 */
NODISCARD
static bool fd_should_color( int fd, color_when_t when ) {
  switch ( when ) {
    case COLOR_ALWAYS:
      return true;
    case COLOR_ISATTY:
      return isatty( fd );              // emulate gcc's --color=auto
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
      //      COMMAND             Should? isatty ISCHR ISFIFO ISREG
      //      =================== ======= ====== ===== ====== =====
      //      include-tidy           T      T      T     F      F
      //      include-tidy > file    F      F      F     F    >>T<<
      //      include-tidy | less    T      F      F     T      F
      //
      // Hence, we want to do color _except_ when ISREG=T.
      //
      return !fd_is_file( fd );
  } // switch
  UNEXPECTED_INT_VALUE( when );
}

/**
 * Gets whether we should emit color to \a file.
 *
 * @param file The `FILE` to check.
 * @return Returns `true` only if we should emit color to \a file.
 */
NODISCARD
static bool file_in_color( FILE *file ) {
  assert( file != NULL );
  int const fd = fileno( file );
  switch ( fd ) {
    case STDOUT_FILENO:
    case STDERR_FILENO:
      return fd_in_color[ fd ];
  } // switch
  return false;
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
 * Gets a pointer to the SGR (Select Graphic Rendition) variable corresponding
 * to \a name, if any.
 *
 * @param name The name to get the corresponding SGR variable for.
 * @return Returns said pointer or NULL if \a name doesn't correspond to any
 * SGR variable.
 */
static char const** sgr_var_find( char const *name ) {
  struct cap_var_map {
    char const   *cap_name;             // color capability name
    char const  **sgr_var;              // pointer to SGR variable
  };
  typedef struct cap_var_map cap_var_map_t;

  static cap_var_map_t const CAP_VAR_MAP[] = {
    { COLOR_CAP_CARET,            &sgr_caret            },
    { COLOR_CAP_ERROR,            &sgr_error            },
    { COLOR_CAP_HELP_KEYWORD,     &sgr_help_keyword     },
    { COLOR_CAP_HELP_NONTERM,     &sgr_help_nonterm     },
    { COLOR_CAP_HELP_PUNCT,       &sgr_help_punct       },
    { COLOR_CAP_HELP_TITLE,       &sgr_help_title       },
    { COLOR_CAP_LOCUS,            &sgr_locus            },
    { COLOR_CAP_MACRO_NO_EXPAND,  &sgr_macro_no_expand  },
    { COLOR_CAP_MACRO_PUNCT,      &sgr_macro_punct      },
    { COLOR_CAP_MACRO_SUBST,      &sgr_macro_subst      },
    { COLOR_CAP_PROMPT,           &sgr_prompt           },
    { COLOR_CAP_WARNING,          &sgr_warning          },
  };

  assert( name != NULL );

  FOREACH_ARRAY_ELEMENT( cap_var_map_t, map, CAP_VAR_MAP ) {
    if ( strcmp( name, map->cap_name ) == 0 )
      return map->sgr_var;
  } // for

  return NULL;
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

////////// extern functions ///////////////////////////////////////////////////

void colors_init( void ) {
  ASSERT_RUN_ONCE();

  for ( int fd = STDOUT_FILENO; fd <= STDERR_FILENO; ++fd )
    fd_in_color[ fd ] = fd_should_color( fd, opt_color_when );

  if ( !fd_in_color[ STDOUT_FILENO ] && !fd_in_color[ STDERR_FILENO ] )
    return;

  color_capabilities = colors_parse( getenv( "CDECL_COLORS" ) );
  if ( color_capabilities == NULL ) {
    static char const COLORS_DEFAULT[] =
      COLOR_CAP_CARET           "=" SGR_FG_GREEN  SGR_SEP SGR_BOLD  SGR_CAP_SEP
      COLOR_CAP_ERROR           "=" SGR_FG_RED    SGR_SEP SGR_BOLD  SGR_CAP_SEP
      COLOR_CAP_HELP_KEYWORD    "="                       SGR_BOLD  SGR_CAP_SEP
      COLOR_CAP_HELP_NONTERM    "=" SGR_FG_CYAN                     SGR_CAP_SEP
      COLOR_CAP_HELP_PUNCT      "=" SGR_FG_YELLOW SGR_SEP SGR_BOLD  SGR_CAP_SEP
      COLOR_CAP_HELP_TITLE      "=" SGR_FG_BLUE   SGR_SEP SGR_BOLD  SGR_CAP_SEP
      COLOR_CAP_LOCUS           "="                       SGR_BOLD  SGR_CAP_SEP
      COLOR_CAP_MACRO_NO_EXPAND "=" SGR_FG_MAGENTA                  SGR_CAP_SEP
      COLOR_CAP_MACRO_PUNCT     "=" SGR_FG_GREEN  SGR_SEP SGR_BOLD  SGR_CAP_SEP
      COLOR_CAP_MACRO_SUBST     "=" SGR_FG_CYAN                     SGR_CAP_SEP
      COLOR_CAP_PROMPT          "=" SGR_FG_GREEN                    SGR_CAP_SEP
      COLOR_CAP_WARNING         "=" SGR_FG_YELLOW SGR_SEP SGR_BOLD  ;

    color_capabilities = colors_parse( COLORS_DEFAULT );
    assert( color_capabilities != NULL );
  }

  ATEXIT( &colors_cleanup );
}

void color_end( FILE *file, char const *sgr_color ) {
  if ( sgr_color != NULL && file_in_color( file ) )
    FPUTS( SGR_END SGR_EL, file );
}

void color_start( FILE *file, char const *sgr_color ) {
  if ( sgr_color != NULL && file_in_color( file ) )
    FPRINTF( file, SGR_START SGR_EL, sgr_color );
}
///////////////////////////////////////////////////////////////////////////////

/** @} */

extern inline void color_strbuf_end( strbuf_t*, char const* );
extern inline void color_strbuf_start( strbuf_t*, char const* );

/* vim:set et sw=2 ts=2: */
