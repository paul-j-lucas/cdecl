/*
**      cdecl -- C gibberish translator
**      src/c_ast_warn.h
**
**      Copyright (C) 2024-2026  Paul J. Lucas
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

#ifndef cdecl_c_ast_warn_H
#define cdecl_c_ast_warn_H

/**
 * @file
 * Declares functions for checking an AST for semantic warnings.
 */

// local
#include "pjl_config.h"                 /* IWYU pragma: keep */
#include "types.h"

/// @cond DOXYGEN_IGNORE

/// @endcond

/**
 * @ingroup ast-functions-group
 * @{
 */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Checks an entire AST for semantic warnings.
 *
 * @param ast The AST to check.
 *
 * @sa c_ast_check()
 * @sa c_type_ast_warn()
 */
void c_ast_warn( c_ast_t const *ast );

/**
 * Checks an entire AST of a type for semantic warnings.
 *
 * @param type_ast The AST of a type to check.
 *
 * @sa c_ast_warn()
 * @sa c_type_ast_check()
 */
void c_type_ast_warn( c_ast_t const *type_ast );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_c_ast_warn_H */
/* vim:set et sw=2 ts=2: */
