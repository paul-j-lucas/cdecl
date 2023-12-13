/*
**      cdecl -- C gibberish translator
**      src/read_line.c
**
**      Copyright (C) 2021-2023  Paul J. Lucas, et al.
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

bool strbuf_read_line( strbuf_t *sbuf, char const *prog_name, FILE *fin,
                       char const *const prompts[const] ) {
  assert( sbuf != NULL );
  assert( fin != NULL );
  assert( prompts == NULL || (prompts[0] != NULL && prompts[1] != NULL) );

  bool const is_interactive = isatty( fileno( fin ) ) && prompts != NULL;
  bool is_cont_line = false;

  for (;;) {
    static char *line;
    bool got_line;

    if ( is_interactive ) {
      // LCOV_EXCL_START -- tests are not interactive
#ifdef WITH_READLINE
      readline_init( prog_name, fin, stdout );
      free( line );
      got_line = (line = readline( prompts[ is_cont_line ] )) != NULL;
      // LCOV_EXCL_STOP
    }
    else
#else /* WITH_READLINE */
      (void)prog_name;
      PUTS( prompts[ is_cont_line ] );
      FFLUSH( stdout );
    }
#endif /* WITH_READLINE */
    {                                   // needed for "else" for WITH_READLINE
      static size_t line_cap;
      got_line = getline( &line, &line_cap, fin ) != -1;
    }

    if ( !got_line ) {
      FERROR( fin );
      return false;
    }

    if ( str_is_empty( line ) ) {
      if ( is_cont_line ) {
        //
        // If we've been accumulating continuation lines, a blank line ends it.
        //
        break;
      }
      continue;                         // otherwise, ignore blank lines
    }

    size_t line_len = strlen( line );
    if ( line_len > 0 && line[ line_len - 1 ] == '\n' )
      --line_len;                       // chop off newline
    is_cont_line = line_len > 0 && line[ line_len - 1 ] == '\\';
    if ( is_cont_line )
      --line_len;                       // eat '\'
    strbuf_putsn( sbuf, line, line_len );

    if ( !is_cont_line )
      break;
  } // for

  assert( sbuf->str != NULL );
  assert( sbuf->str[0] != '\0' );
#ifdef HAVE_READLINE_HISTORY_H
  if ( is_interactive )
    add_history( sbuf->str );           // LCOV_EXCL_LINE
#endif /* HAVE_READLINE_HISTORY_H */
  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
