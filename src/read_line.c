/*
**      cdecl -- C gibberish translator
**      src/read_line.c
**
**      Copyright (C) 2021  Paul J. Lucas, et al.
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
 * Defines read_line() for reading a line of text interactively from stdin.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "read_line.h"
#include "cdecl.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_READLINE_READLINE_H
# include <readline/readline.h>
#endif /* HAVE_READLINE_READLINE_H */
#ifdef HAVE_READLINE_HISTORY_H
# include <readline/history.h>
#endif /* HAVE_READLINE_HISTORY_H */

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

bool strbuf_read_line( strbuf_t *sbuf, char const *prompts[] ) {
  assert( sbuf != NULL );
  assert( prompts != NULL );

  bool is_cont_line = false;

  for (;;) {
    static char *line;
    bool got_line;

#ifdef WITH_READLINE
    extern void readline_init( FILE*, FILE* );
    static bool called_readline_init;
    if ( false_set( &called_readline_init ) )
      readline_init( cdecl_fin, cdecl_fout );
    free( line );
    got_line = (line = readline( prompts[ is_cont_line ] )) != NULL;
#else
    static size_t line_cap;
    FPUTS( prompts[ is_cont_line ], cdecl_fout );
    FFLUSH( cdecl_fout );
    got_line = getline( &line, &line_cap, cdecl_fin ) != -1;
#endif /* WITH_READLINE */

    if ( !got_line ) {
      FERROR( cdecl_fout );
      return false;
    }

    if ( is_blank_line( line ) ) {
      if ( is_cont_line ) {
        //
        // If we've been accumulating continuation lines, a blank line ends it.
        //
        break;
      }
      continue;                         // otherwise, ignore blank lines
    }

    size_t const line_len = strlen( line );
    is_cont_line = ends_with_chr( line, line_len, '\\' );
    strbuf_putsn( sbuf, line, line_len - is_cont_line /* don't copy '\' */ );

    if ( !is_cont_line )
      break;
  } // for

  assert( sbuf->str != NULL );
  assert( sbuf->str[0] != '\0' );
#ifdef WITH_READLINE
  add_history( sbuf->str );
#endif /* WITH_READLINE */
  return true;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
