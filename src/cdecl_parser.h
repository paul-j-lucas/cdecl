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
 * Wrapper around the Bison-generated `parser.h` to add necessary `#include`s
 * for the types in Bison's <code>\%union</code> declaration as well as a
 * declaration for the parser_cleanup() function.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_type.h"
#include "slist.h"
#include "parser.h"                     /* must go last */

/**
 * @ingroup parser-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Cleans up global parser data at program termination.
 *
 * @remarks The parser uses a "cleanup" function rather than an "init" function
 * (that calls **atexit**(3) with <code>%parser_cleanup()</code>) because
 * parser clean-up needs to be done at a specific point in the program's clean-
 * up sequence and that's trivial to do by having to call this explicitly.
 *
 * @sa cdecl_cleanup()
 */
void parser_cleanup( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_parser_H */
/* vim:set et sw=2 ts=2: */
