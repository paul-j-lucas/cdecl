/*
**      cdecl -- C gibberish translator
**      src/c_typedef.h
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

#ifndef cdecl_c_typedef_H
#define cdecl_c_typedef_H

/**
 * @file
 * Declares types and functions for adding and looking up C/C++ `typedef`
 * declarations.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup c-typedef-group C/C++ Typedef Declarations
 * Types and functions for adding and looking up C/C++ `typedef` or `using`
 * declarations.
 * @{
 */

/**
 * C/C++ `typedef` or `using` information.
 */
struct c_typedef {
  c_ast_t const  *ast;                  ///< AST representing the type.
  c_lang_id_t     lang_ids;             ///< Language(s) available in.
  bool            user_defined;         ///< Is the type user-defined?
};

/**
 * The signature for a function passed to c_typedef_visit().
 *
 * @param tdef The <code>\ref c_typedef</code> to visit.
 * @param data Optional data passed to the visitor.
 * @return Returning `true` will cause traversal to stop and \a type to be
 * returned to the caller of c_typedef_visit().
 */
typedef bool (*c_typedef_visitor_t)( c_typedef_t const *tdef, void *data );

////////// extern functions ///////////////////////////////////////////////////

/**
 * Adds a new `typedef` or `using` to the global set.
 *
 * @param type_ast The AST of the type.  Ownership is taken only if the
 * function returns NULL.
 * @return If:
 * + \a type_ast was added, returns NULL; or:
 * + \a type_ast->name already exists and the types are equivalent, returns a
 *   <code>\ref c_typedef_t</code> where \a ast is NULL; or:
 * + \a type_ast->name already exists and the types are _not_ equivalent,
 *   returns the a <code>\ref c_typedef_t</code> of the existing type.
 */
PJL_WARN_UNUSED_RESULT
c_typedef_t const* c_typedef_add( c_ast_t const *type_ast );

/**
 * Cleans up <code>\ref c_typedef</code> data.
 *
 * @sa c_typedef_init()
 */
void c_typedef_cleanup( void );

/**
 * Gets the <code>\ref c_typedef</code> for \a sname.
 *
 * @param sname The scoped name to find.
 * @return Returns a pointer to the corresponding <code>\ref c_typedef</code>
 * or null for none.
 */
PJL_WARN_UNUSED_RESULT
c_typedef_t const* c_typedef_find( c_sname_t const *sname );

/**
 * Initializes all <code>\ref c_typedef</code> data.
 *
 * @sa c_typedef_cleanup()
 */
void c_typedef_init( void );

/**
 * Does an in-order traversal of all <code>\ref c_typedef</code>s.
 *
 * @param visitor The visitor to use.
 * @param data Optional data passed to \a visitor.
 * @return Returns a pointer to the <code>\ref c_typedef</code> the visitor
 * stopped on or null.
 */
PJL_NOWARN_UNUSED_RESULT
c_typedef_t const* c_typedef_visit( c_typedef_visitor_t visitor, void *data );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_c_typedef_H */
/* vim:set et sw=2 ts=2: */
