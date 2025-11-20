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

#ifndef cdecl_read_line_H
#define cdecl_read_line_H

/**
 * @file
 * Declares strbuf_read_line() for reading a line of text from a file or
 * interactively from stdin.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "strbuf.h"

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdio.h>                      /* for FILE */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Checks whether \a s is a "continued line," that is a line that typically
 * ends with a `\`.
 *
 * @param s The string to check.
 * @param ps_len A pointer to the length of \a s.  If \a s is a continued line,
 * then on return, this must be decremented by the number of characters
 * comprising the continuation sequence.
 * @return Returns `true` only if \a s is a continued line.
 */
typedef bool (*sbrl_is_cont_line_fn_t)( char const *s, size_t *ps_len );

/**
 * Reads a line from \a fin, perhaps interactively with editing and
 * autocompletion.
 *
 * @remarks
 * @parblock
 * Only if:
 *
 *  + \a fin is connected to a TTY; and:
 *  + \a prompts is non-NULL; and:
 *  + GNU **readline**(3) is compiled in;
 *
 * then reads interactively by:
 *
 *  + Using GNU **readline**(3) to read a line with editing and autocompletion.
 *  + Adding non-whitespace-only lines to the history.
 *
 * Multiple lines separated by `\` are joined together and returned as a single
 * line.  Lines always end with a newline.
 * @endparblock
 *
 * @param sbuf The \ref strbuf to use.
 * @param fin The file to read from.  If \a fin is not connected to a TTY, does
 * not read interactively.
 * @param prompts A pointer to a 2-element array of prompts to use: the primary
 * prompt and the secondary prompt for continuation lines (lines after ones
 * ending with `\`).  If NULL, does not read interactively.
 * @param is_cont_line_fn A pointer to the \ref sbrl_is_cont_line_fn_t function
 * to use.
 * @param pline_no A pointer to the current line number within a file that will
 * be incremented for every `\`-newline encountered. May be NULL.
 * @return Returns `false` only if encountered EOF.
 */
NODISCARD
bool strbuf_read_line( strbuf_t *sbuf, FILE *fin, char const *const prompts[],
                       sbrl_is_cont_line_fn_t is_cont_line_fn, int *pline_no );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_read_line_H */
/* vim:set et sw=2 ts=2: */
