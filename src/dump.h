/*
**      cdecl -- C gibberish translator
**      src/dump.h
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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

#ifndef cdecl_dump_H
#define cdecl_dump_H

/**
 * @file
 * Declares functions for dumping types for debugging.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdio.h>                      /* for FILE */

/// @endcond

/**
 * @defgroup printing-dump-group Dumping Debug Output
 * Functions for dumping types for debugging.
 * @{
 */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Dumps \a ast (for debugging).
 *
 * @param ast The AST to dump.
 * @param indent The initial indent.
 * @param key0 The initial key or NULL for none.
 * @param dout The `FILE` to dump to.
 *
 * @sa c_ast_list_dump()
 */
void c_ast_dump( c_ast_t const *ast, unsigned indent, char const *key0,
                 FILE *dout );

/**
 * Dumps \a list of ASTs (for debugging).
 *
 * @param list The `slist` of ASTs to dump.
 * @param indent The initial indent.
 * @param dout The `FILE` to dump to.
 */
void c_ast_list_dump( c_ast_list_t const *list, unsigned indent, FILE *dout );

/**
 * Dumps \a sname (for debugging).
 *
 * @param sname The scoped name to dump.
 * @param dout The `FILE` to dump to.
 *
 * @sa c_ast_dump()
 */
void c_sname_dump( c_sname_t const *sname, FILE *dout );

/**
 * Dumps \a type (for debugging).
 *
 * @param type The <code>\ref c_type</code> to print.
 * @param dout The `FILE` to dump to.
 *
 * @sa c_type_id_dump()
 */
void c_type_dump( c_type_t const *type, FILE *dout );

/**
 * Dumps \a tid (for debugging).
 *
 * @param tid The <code>\ref c_type_id_t</code> to print.
 * @param dout The `FILE` to dump to.
 *
 * @sa c_type_dump()
 */
void c_type_id_dump( c_type_id_t tid, FILE *dout );

/**
 * Prints a key/value pair (for debugging).
 *
 * @param key The key to print.
 * @param value The value to print, if any.  If NULL, `null` is printed instead
 * of the value.
 * @param dout The `FILE` to dump to.
 */
void kv_dump( char const *key, char const *value, FILE *dout );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_dump_H */
/* vim:set et sw=2 ts=2: */
