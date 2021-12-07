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

#ifndef cdecl_read_line_H
#define cdecl_read_line_H

/**
 * @file
 * Declares read_line() for reading a line of text interactively from stdin.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "strbuf.h"

// standard
#include <stdbool.h>

////////// extern functions ///////////////////////////////////////////////////

/**
 * Reads an input line interactively.
 *
 *  + Returns only non-whitespace-only lines.
 *  + Stitches multiple lines ending with `\` together.
 *
 * If GNU **readline**(3) is compiled in, also:
 *
 *  + Adds non-whitespace-only lines to the history.
 *
 * @param sbuf The strbuf to use.
 * @param prompts A pointer to a 2-element array of the prompts to use: the
 * primary prompt and the the secondary prompt to use for a continuation line
 * (a line after ones ending with `\`).
 * @return Returns `false` only if encountered EOF.
 */
PJL_WARN_UNUSED_RESULT
bool strbuf_read_line( strbuf_t *sbuf, char const *prompts[] );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_read_line_H */
/* vim:set et sw=2 ts=2: */
