/*
**      cdecl -- C gibberish translator
**      src/debug.h
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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

#ifndef cdecl_debug_H
#define cdecl_debug_H

/**
 * @file
 * Declares functions for printing abstract syntax trees for debugging.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "slist.h"
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdio.h>                      /* for FILE */

/// @endcond

/**
 * @defgroup printing-debug-group Printing Debug Output
 * Functions for printing an AST for debugging.
 * @{
 */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Dumps \a ast (for debugging).
 *
 * @param ast The `c_ast` to dump.
 * @param indent The initial indent.
 * @param key0 The initial key or null for none.
 * @param dout The `FILE` to dump to.
 */
void c_ast_debug( c_ast_t const *ast, unsigned indent, char const *key0,
                  FILE *dout );

/**
 * Dumps \a list (for debugging).
 *
 * @param list The `slist` to dump.
 * @param indent The initial indent.
 * @param dout The `FILE` to dump to.
 */
void c_ast_list_debug( slist_t const *list, unsigned indent, FILE *dout );

/**
 * Dumps \a type_id (for debugging).
 *
 * @param type_id The <code>\ref c_type_id_t</code> to print.
 * @param dout The `FILE` to dump to.
 */
void c_type_debug( c_type_id_t type_id, FILE *dout );

/**
 * Prints a key/value pair (for debugging).
 *
 * @param key The key to print.
 * @param value The value to print, if any.  If either null or the empty
 * string, `null` is printed instead of the value.
 * @param dout The `FILE` to dump to.
 */
void kv_debug( char const *key, char const *value, FILE *dout );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_debug_H */
/* vim:set et sw=2 ts=2: */
