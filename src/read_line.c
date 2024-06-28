/*
**      cdecl -- C gibberish translator
**      src/read_line.c
**
**      Copyright (C) 2021-2024  Paul J. Lucas, et al.
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
#include <stddef.h>                     /* for NULL */
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

////////// extern functions ///////////////////////////////////////////////////

bool strbuf_read_line( strbuf_t *sbuf, FILE *fin,
                       char const *const prompts[const], int *line_no ) {
  assert( sbuf != NULL );
  assert( fin != NULL );
  assert( prompts == NULL || (prompts[0] != NULL && prompts[1] != NULL) );

  bool const is_interactive = isatty( fileno( fin ) ) && prompts != NULL;
  bool is_cont_line = false;

  do {
    bool got_line;
    char *line = NULL;
    size_t line_len = 0;

    if ( is_interactive ) {
      // LCOV_EXCL_START -- tests are not interactive
#ifdef WITH_READLINE
      readline_init( fin, stdout );
      static char *readline_line;
      free( readline_line );
      // Note: readline() does NOT include the '\n'.
      readline_line = readline( prompts[ is_cont_line ] );
      got_line = readline_line != NULL;
      if ( got_line ) {
        line = readline_line;
        line_len = strlen( line );
      }
      // LCOV_EXCL_STOP
    }
    else
#else /* WITH_READLINE */
      PUTS( prompts[ is_cont_line ] );
      FFLUSH( stdout );
    }
#endif /* WITH_READLINE */
    {                                   // needed for "else" for WITH_READLINE
      static char *getline_line;
      static size_t getline_cap;
      // Note: getline() DOES include the '\n'.
      ssize_t const rv = getline( &getline_line, &getline_cap, fin );
      got_line = rv != -1;
      if ( got_line ) {
        line = getline_line;
        line_len = STATIC_CAST( size_t, rv );
        // Chop off the newline so it's consistent with readline().
        str_chomp( line, &line_len );
      }
    }

    if ( !got_line ) {
      FERROR( fin );
      return false;
    }

    is_cont_line = line_len >= 1 && line[ line_len - 1 ] == '\\';
    if ( is_cont_line ) {
      --line_len;                       // eat '\'
      if ( line_no != NULL )
        ++*line_no;
    }

    strbuf_putsn( sbuf, line, line_len );
  } while ( is_cont_line );

  strbuf_putc( sbuf, '\n' );

#ifdef HAVE_READLINE_HISTORY_H
  if ( is_interactive && !str_is_empty( sbuf->str ) )
    add_history( sbuf->str );           // LCOV_EXCL_LINE
#endif /* HAVE_READLINE_HISTORY_H */
  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
