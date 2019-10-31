/*
**      cdecl -- C gibberish translator
**      src/c_typedef.h
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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

#ifndef cdecl_c_typedef_H
#define cdecl_c_typedef_H

/**
 * @file
 * Declares types and functions for adding and looking up C/C++ `typedef`
 * declarations.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "typedefs.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

/// @endcond

/**
 * Return value for c_typedef_add().
 */
enum td_add_rv {
  TD_ADD_ADDED,                         ///< Type was added.
  TD_ADD_EQUIV,                         ///< Type exists and is equivalent.
  TD_ADD_DIFF                           ///< Type exists and is different.
};

///////////////////////////////////////////////////////////////////////////////

/**
 * C/C++ language typedef information.
 */
struct c_typedef {
  c_ast_t const  *ast;                  ///< AST representing the type.
  bool            user_defined;         ///< Is the type user-defined?
};

/**
 * The signature for a function passed to `c_typedef_visit()`.
 *
 * @param type The `c_typedef` to visit.
 * @param data Optional data passed to the visitor.
 * @return Returning `true` will cause traversal to stop and \a type to be
 * returned to the caller of `c_typedef_visit()`.
 */
typedef bool (*c_typedef_visitor_t)( c_typedef_t const *type, void *data );

////////// extern functions ///////////////////////////////////////////////////

/**
 * Adds a new `c_typedef` to the global set.
 *
 * @param type_ast The AST of the type.  Ownership is taken only if the
 * function returns #TD_ADD_ADDED.
 * @return
 * + #TD_ADD_ADDED only if the type was added;
 * + #TD_ADD_EQUIV only if \a type_ast->name already exists and the types are
 *   equivalent;
 * + #TD_ADD_DIFF only if \a type_ast->name already exists and the types are
 *   not equivalent.
 */
td_add_rv_t c_typedef_add( c_ast_t const *type_ast );

/**
 * Cleans up `c_typedef` data.
 */
void c_typedef_cleanup( void );

/**
 * Gets the `c_typedef` for \a sname.
 *
 * @param sname The scoped name to find.
 * @return Returns a pointer to the corresponding `c_typedef` or null for none.
 */
c_typedef_t const* c_typedef_find( c_sname_t const *sname );

/**
 * Initializes `c_typedef` data.
 */
void c_typedef_init( void );

/**
 * Does an in-order traversal of all `c_typedef`s.
 *
 * @param visitor The visitor to use.
 * @param data Optional data passed to \a visitor.
 * @return Returns a pointer to the `c_typedef` the visitor stopped on or null.
 */
c_typedef_t const* c_typedef_visit( c_typedef_visitor_t visitor, void *data );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_c_typedef_H */
/* vim:set et sw=2 ts=2: */
