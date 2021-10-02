/*
**      cdecl -- C gibberish translator
**      src/check.h
**
**      Copyright (C) 2021  Paul J. Lucas, et al.
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

#ifndef cdecl_check_H
#define cdecl_check_H

/**
 * @file
 * Declares functions for performing semantic checks.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

/**
 * @addtogroup ast-functions-group
 * @{
 */

/**
 * Checks an entire AST for semantic errors and warnings.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast error-free.
 */
PJL_WARN_UNUSED_RESULT
bool c_ast_check( c_ast_t const *ast );

/** @} */

/**
 * Checks a scoped name for valid scope order.
 *
 * @param sname The scoped name to check.
 * @param sname_loc The location of \a sname.
 * @return Returns `true` only if all checks passed.
 */
bool c_sname_check( c_sname_t const *sname, c_loc_t const *sname_loc );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_check_H */
/* vim:set et sw=2 ts=2: */
