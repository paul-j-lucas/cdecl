/*
**      cdecl -- C gibberish translator
**      src/debug.h
**
**      Paul J. Lucas
*/

#ifndef cdecl_debug_H
#define cdecl_debug_H

// local
#include "config.h"                     /* must go first */
#include "ast.h"

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
 * @param jout The FILE to print to.
 */
void print_kv( char const *key, char const *value, FILE *jout );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_debug_H */
/* vim:set et sw=2 ts=2: */
