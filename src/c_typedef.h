/*
**      cdecl -- C gibberish translator
**      src/c_typedef.h
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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
#include "red_black.h"
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

/// @endcond

/**
 * Convenience macro for specifying a \ref c_typedef literal.
 *
 * @param AST The AST.
 * @param DECL_FLAGS The declaration flags to use.
 */
#define C_TYPEDEF_LIT(AST,DECL_FLAGS) (c_typedef_t const) \
  { (AST), LANG_ANY, (DECL_FLAGS), .is_predefined = false }

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
  unsigned        decl_flags;           ///< How was the type defined?
  bool            is_predefined;        ///< Was the type predefined?
};

/**
 * The signature for a function passed to c_typedef_visit().
 *
 * @param tdef The \ref c_typedef to visit.
 * @param v_data Optional data passed to the visitor.
 * @return Returning `true` will cause traversal to stop and a pointer to the
 * \ref c_typedef the visitor stopped on to be returned to the caller of
 * c_typedef_visit().
 */
typedef bool (*c_typedef_visit_fn_t)( c_typedef_t const *tdef, void *v_data );

////////// extern functions ///////////////////////////////////////////////////

/**
 * Adds a new `typedef` (or `using`) to the global set.
 *
 * @param type_ast The AST of the type to add.  Ownership is taken only if the
 * type was added.
 * @param decl_flags The declaration flag indicating how the type was created;
 * must only be one of #C_ENG_DECL, #C_GIB_TYPEDEF, or #C_GIB_USING.
 * @return Returns an rb_node where \ref rb_node.data "data" points to a \ref
 * c_typedef of either:
 * + The newly added type (its \ref c_typedef.ast "ast" is equal to \a
 *   type_ast); or:
 * + The previously added type having the same scoped name.
 *
 * @sa c_typedef_remove()
 */
NODISCARD
rb_node_t* c_typedef_add( c_ast_t const *type_ast, unsigned decl_flags );

/**
 * Gets the \ref c_typedef for \a name.
 *
 * @param name The name to find.  It may contain `::`.
 * @return Returns a pointer to the corresponding \ref c_typedef or NULL for
 * none.
 *
 * @sa c_typedef_find_sname()
 */
NODISCARD
c_typedef_t const* c_typedef_find_name( char const *name );

/**
 * Gets the \ref c_typedef for \a sname.
 *
 * @param sname The scoped name to find.
 * @return Returns a pointer to the corresponding \ref c_typedef or NULL for
 * none.
 *
 * @sa c_typedef_find_name()
 */
NODISCARD
c_typedef_t const* c_typedef_find_sname( c_sname_t const *sname );

/**
 * Initializes all \ref c_typedef data.
 *
 * @note This function must be called exactly once.
 */
void c_typedef_init( void );

/**
 * Removes a `typedef` (or `using`) from the global set.
 *
 * @param node The rb_node containing the `typedef` to remove.
 * @return Returns the removed `typedef`.  The caller is responsible for
 * deleting it if necessary.
 *
 * @sa c_typedef_add()
 */
NODISCARD
c_typedef_t* c_typedef_remove( rb_node_t *node );

/**
 * Does an in-order traversal of all \ref c_typedef.
 *
 * @param visit_fn The visitor function to use.
 * @param v_data Optional data passed to \a visit_fn.
 * @return Returns a pointer to the \ref c_typedef the visitor stopped on or
 * NULL.
 */
PJL_DISCARD
c_typedef_t const* c_typedef_visit( c_typedef_visit_fn_t visit_fn,
                                    void *v_data );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_c_typedef_H */
/* vim:set et sw=2 ts=2: */
