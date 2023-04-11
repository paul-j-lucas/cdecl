/*
**      cdecl -- C gibberish translator
**      src/cdecl_parser.h
**
**      Copyright (C) 2021-2023  Paul J. Lucas
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
 * Wrapper around the Bison-generated `parser.h` to add `#include` guards as
 * well as declarations for Bison types and functions.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "parser.h"

// standard
#include <attribute.h>

/**
 * @ingroup parser-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

typedef enum yytokentype yytokentype;

/**
 * Cleans up global parser data at program termination.
 */
void parser_cleanup( void );

#ifdef __GNUC__
# pragma GCC diagnostic push
  // Some versions of Bison don't declare yyparse() in the generated header
  // file so we declare it below.  However, if the current version of Bison
  // does declare it, then we'd get a redundant declaration warning -- so
  // suppress that.
# pragma GCC diagnostic ignored "-Wredundant-decls"
#endif /* __GNUC__ */

/**
 * Bison: parse input.
 *
 * @return Returns 0 only if parsing was successful.
 */
NODISCARD
int yyparse( void );

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif /* __GNUC__ */

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_parser_H */
/* vim:set et sw=2 ts=2: */
