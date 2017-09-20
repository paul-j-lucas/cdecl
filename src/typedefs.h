/*
**      cdecl -- C gibberish translator
**      src/typedefs.h
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

#ifndef cdecl_typedefs_H
#define cdecl_typedefs_H

// local
#include "config.h"                     /* must go first */

typedef struct c_ast c_ast_t;

// standard
#include <stdbool.h>

/**
 * @file
 * Declares types and functions for adding and looking up C/C++ typedef
 * declarations.
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * C/C++ language typedef information.
 */
struct c_typedef {
  char const *type_name;
  c_ast_t const *ast;
  bool user_defined;
};
typedef struct c_typedef c_typedef_t;

/**
 * The signature for a function passed to c_typedef_visit().
 *
 * @param type The c_typedef to visit.
 * @param data Optional data passed to the visitor.
 * @return Returning \c true will cause traversal to stop and \a type to be
 * returned to the caller of c_typedef_visit().
 */
typedef bool (*c_typedef_visitor_t)( c_typedef_t const *type, void *data );

////////// extern functions ///////////////////////////////////////////////////

/**
 * Adds a new \c typedef to the global set.
 *
 * @param type_name The name of the type.  Ownership of the C string is taken
 * only if the function returns \a true.
 * @param type_ast The AST of the type.  Ownership is taken only if the
 * function returns \a true.
 * @return Returns \c true only if the type was either added or \a name already
 * exists and the types are equivalent; \c false if \a name already exists and
 * the types are not equivalent.
 */
bool c_typedef_add( char const *type_name, c_ast_t const *type_ast );

/**
 * Cleans up \c typedef data.
 */
void c_typedef_cleanup( void );

/**
 * Gets the c_typedef for \a name.
 *
 * @param name The name to find.
 * @return Returns a pointer to the corresponding c_typedef or NULL for none.
 */
c_typedef_t const* c_typedef_find( char const *name );

/**
 * Initializes typedef data.
 */
void c_typedef_init( void );

/**
 * Does an in-order traversal of all typedefs.
 *
 * @param visitor The visitor to use.
 * @param data Optional data passed to \a visitor.
 * @return Returns a pointer to the c_typedef the visitor stopped on or NULL.
 */
c_typedef_t const* c_typedef_visit( c_typedef_visitor_t visitor, void *data );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_typedefs_H */
/* vim:set et sw=2 ts=2: */
