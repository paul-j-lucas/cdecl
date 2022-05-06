/*
**      cdecl -- C gibberish translator
**      src/c_typedef.h
**
**      Copyright (C) 2017-2022  Paul J. Lucas
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

/**
 * Convenience macro for specifying a \ref c_typedef literal having \a AST.
 *
 * @param AST The AST.
 */
#define C_TYPEDEF_AST_LIT(AST) (c_typedef_t const) \
  { (AST), LANG_ANY, .predefined = false, .defined_in_english = false }

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
  bool            predefined;           ///< Is the type predefined?
  bool            defined_in_english;   ///< Originally defined in English?
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
 * Adds a new `typedef` or `using` to the global set.
 *
 * @param type_ast The AST of the type to add.  Ownership is taken only if the
 * type was added.
 * @return Returns the \ref c_typedef of either:
 * + The newly added type (its AST's \ref c_ast.unique_id "unique_id" is equal
 *   to \a type_ast's `unique_id`); or:
 * + The previously added type having the same scoped name.
 */
PJL_WARN_UNUSED_RESULT
c_typedef_t const* c_typedef_add( c_ast_t const *type_ast );

/**
 * Gets the \ref c_typedef for \a name.
 *
 * @param name The name to find.  It may contain `::`.
 * @return Returns a pointer to the corresponding \ref c_typedef or NULL for
 * none.
 *
 * @sa c_typedef_find_sname()
 */
PJL_WARN_UNUSED_RESULT
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
PJL_WARN_UNUSED_RESULT
c_typedef_t const* c_typedef_find_sname( c_sname_t const *sname );

/**
 * Initializes all \ref c_typedef data.
 */
void c_typedef_init( void );

/**
 * Does an in-order traversal of all \ref c_typedef.
 *
 * @param visit_fn The visitor function to use.
 * @param v_data Optional data passed to \a visit_fn.
 * @return Returns a pointer to the \ref c_typedef the visitor stopped on or
 * NULL.
 */
PJL_NOWARN_UNUSED_RESULT
c_typedef_t const* c_typedef_visit( c_typedef_visit_fn_t visit_fn,
                                    void *v_data );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_c_typedef_H */
/* vim:set et sw=2 ts=2: */
