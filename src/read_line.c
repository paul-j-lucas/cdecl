/*
**      cdecl -- C gibberish translator
**      src/read_line.c
**
**      Copyright (C) 2021-2025  Paul J. Lucas, et al.
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
 * Defines strbuf_read_line() for reading a line of text from a file or
 * interactively from stdin.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "read_line.h"
#ifdef WITH_READLINE
#include "autocomplete.h"
#endif /* WITH_READLINE */
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>                     /* for isatty(3) */

#ifdef WITH_READLINE
# ifdef HAVE_READLINE_READLINE_H
#   include <readline/readline.h>
# endif /* HAVE_READLINE_READLINE_H */
# ifdef HAVE_READLINE_HISTORY_H
#   include <readline/history.h>
# endif /* HAVE_READLINE_HISTORY_H */
#endif /* WITH_READLINE */

/// @endcond

#define TRI_BS  "?\?/"                  /**< Trigraph <code>"\\"</code>. */

////////// local functions ////////////////////////////////////////////////////

/**
 * Wrapper around **getline**(3).
 *
 * @param fin The file to read from.
 * @param prompt The prompt to use.  May be NULL.
 * @param pline_len A pointer to receive the length of the line read.
 * @return Returns the line read or NULL for EOF.
 */
NODISCARD
static char const* getline_wrapper( FILE *fin, char const *prompt,
                                    size_t *pline_len ) {
  assert( fin != NULL );
  assert( pline_len != NULL );

  if ( prompt != NULL ) {
    PUTS( prompt );
    FFLUSH( stdout );
  }

  static char *line;  // must be distinct from "line" in readline_wrapper()
  static size_t line_cap;

  ssize_t const line_len = getline( &line, &line_cap, fin );
  if ( line_len == -1 )
    return NULL;

  *pline_len = STATIC_CAST( size_t, line_len );
  // Note: getline() DOES include a '\n', so chop it off so it's consistent
  // with readline().
  strn_rtrim( line, pline_len );

  return line;
}

/**
 * Checks whether \a s is a "continued line," that is a line that ends with a
 * `\` or `??/` (the trigraph sequence for `\`).
 *
 * @param s The string to check.
 * @param ps_len A pointer to the length of \a s.  If \a s is a continued line,
 * this is decremented by the number of characters comprising the continuation
 * sequence.
 * @return Returns `true` only if \a s is a continued line.
 */
NODISCARD
static bool is_continued_line( char const *s, size_t *ps_len ) {
  assert( s != NULL );
  assert( ps_len != NULL );

  if ( *ps_len >= 1 ) {
    if ( s[ *ps_len - 1 ] == '\\' ) {
      --*ps_len;
      return true;
    }
    if ( *ps_len >= STRLITLEN( TRI_BS ) &&
         STRNCMPLIT( s + *ps_len - STRLITLEN( TRI_BS ), TRI_BS ) == 0 ) {
      *ps_len -= STRLITLEN( TRI_BS );
      return true;
    }
  }

  return false;
}

#ifdef WITH_READLINE
// LCOV_EXCL_START -- tests are not interactive
/**
 * Wrapper around GNU **readline**(3).
 *
 * @param fin The file to read from.
 * @param prompt The prompt to use.  May be NULL.
 * @param pline_len A pointer to receive the length of the line read.
 * @return Returns the line read or NULL for EOF.
 */
NODISCARD
static char const* readline_wrapper( FILE *fin, char const *prompt,
                                     size_t *pline_len ) {
  assert( fin != NULL );
  assert( pline_len != NULL );

  static char *line;  // must be distinct from "line" in getline_wrapper()
  free( line );

  RUN_ONCE autocomplete_init();
  rl_instream  = fin;
  rl_outstream = stdout;

  line = readline( prompt );
  *pline_len = line != NULL ? strlen( line ) : 0;
  // Note: readline() does NOT include a '\n'.
  return line;
}
// LCOV_EXCL_STOP
#endif /* WITH_READLINE */

/**
 * Reads a line of input.
 *
 * @param fin The file to read from.
 * @param prompt The prompt to use.  If NULL, does not read interactively.
 * @param pline_len A pointer to receive the length of the line read.
 * @return Returns the line read or NULL for EOF.
 */
NODISCARD
static char const* read_line( FILE *fin, char const *prompt,
                              size_t *pline_len ) {
  assert( fin != NULL );
  assert( pline_len != NULL );

#ifdef WITH_READLINE
  if ( prompt != NULL ) {
    // LCOV_EXCL_START -- tests are not interactive
    return readline_wrapper( fin, prompt, pline_len );
    // LCOV_EXCL_STOP
  }
#endif /* WITH_READLINE */

  return getline_wrapper( fin, prompt, pline_len );
}

////////// extern functions ///////////////////////////////////////////////////

bool strbuf_read_line( strbuf_t *sbuf, FILE *fin,
                       char const *const prompts[const], int *pline_no ) {
  assert( sbuf != NULL );
  assert( fin != NULL );
  assert( prompts == NULL || (prompts[0] != NULL && prompts[1] != NULL) );

  bool const is_interactive = isatty( fileno( fin ) ) && prompts != NULL;
  bool is_cont_line = false;

  do {
    size_t line_len;
    char const *const line = read_line(
      fin, is_interactive ? prompts[ is_cont_line ] : NULL, &line_len
    );

    if ( line == NULL ) {
      FERROR( fin );
      return false;
    }

    is_cont_line = is_continued_line( line, &line_len );
    if ( is_cont_line && pline_no != NULL )
      ++*pline_no;

    strbuf_putsn( sbuf, line, line_len );
  } while ( is_cont_line );

#ifdef HAVE_READLINE_HISTORY_H
  if ( is_interactive && !str_is_empty( sbuf->str ) )
    add_history( sbuf->str );           // LCOV_EXCL_LINE
#endif /* HAVE_READLINE_HISTORY_H */

  strbuf_putc( sbuf, '\n' );
  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
