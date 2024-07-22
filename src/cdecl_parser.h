/*
**      cdecl -- C gibberish translator
**      src/cdecl_parser.h
**
**      Copyright (C) 2021-2024  Paul J. Lucas
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

#ifndef cdecl_parser_H
#define cdecl_parser_H

/**
 * @file
 * Wrapper around the Bison-generated `parser.h` to add necessary `#include`s
 * for the types in Bison's <code>\%union</code> declaration as well as a
 * declaration for the parser_cleanup() function.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_type.h"
#include "slist.h"
#include "parser.h"                     /* must go last */

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

/**
 * @ingroup parser-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the parser.
 *
 * @note This function must be called exactly once.
 */
void parser_init( void );

/**
 * Wrapper around `yyparse()` that parses a string.
 *
 * @param s The string to parse.
 * @param s_len The length of \a s.
 * @return Returns `true` only upon success.
 */
NODISCARD
bool yyparse_sn( char const *s, size_t s_len );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_parser_H */
/* vim:set et sw=2 ts=2: */
