/*
**      cdecl -- C gibberish translator
**      src/debug.h
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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
#include "config.h"                     /* must go first */
#include "c_ast.h"

// system
#include <stdio.h>                      /* for FILE */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Dumps the given AST (for debugging).
 *
 * @param ast The c_ast to dump.
 * @paran indent The initial indent.
 * @param key0 The initial key or null for none.
 * @param fout The FILE to dump to.
 */
void c_ast_debug( c_ast_t const *ast, unsigned indent, char const *key0,
                  FILE *fout );

/**
 * Dump the given c_ast_list (for debugging).
 *
 * @param list The c_ast_list to dump.
 * @param indent The initial indent.
 * @param jour The FILE to dump to.
 */
void c_ast_list_debug( c_ast_list_t const *list, unsigned indent, FILE *fout );

/**
 * Prints a key/value pair.
 *
 * @param key The key to print.
 * @param value The value to print, if any.  If either null or the empty
 * string, \c null is printed instead of the value.
 * @param out The FILE to print to.
 */
void print_kv( char const *key, char const *value, FILE *out );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_debug_H */
/* vim:set et sw=2 ts=2: */
