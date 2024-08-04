/*
**      cdecl -- C gibberish translator
**      src/show.h
**
**      Copyright (C) 2023-2024  Paul J. Lucas, et al.
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

#ifndef cdecl_show_H
#define cdecl_show_H

/**
 * @file
 * Declares functions for showing types for the **cdecl** `show` command.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "p_macro.h"
#include "types.h"

// standard
#include <stdbool.h>
#include <stdio.h>                      /* for FILE */

/**
 * @defgroup showing-group Showing C/C++ Types & Macros
 * Functions for showing types & macros for the **cdecl** `show` command.
 *
 * @sa \ref printing-c-types-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Shows (prints) the definition of \a macro.
 *
 * @param macro The macro to show.
 * @param fout The `FILE` to print to.
 * @return Returns `false` only if \a macro is dynamic and is _not_ supported
 * in the current language; `true` otherwise.
 *
 * @sa show_macros()
 */
PJL_DISCARD
bool show_macro( p_macro_t const *macro, FILE *fout );

/**
 * Shows (prints) the definition of macros.
 *
 * @param show Which macros to show.
 * @param fout The `FILE` to print to.
 *
 * @sa show_macro()
 */
void show_macros( cdecl_show_t show, FILE *fout );

/**
 * Shows (prints) the definition of \a tdef.
 *
 * @param tdef The \ref c_typedef to show.
 * @param decl_flags The declaration flags to use.
 * @param fout The `FILE` to print to.
 *
 * @note A newline _is_ printed.
 *
 * @sa c_typedef_english()
 * @sa c_typedef_gibberish()
 * @sa print_type_ast()
 * @sa print_type_decl()
 * @sa show_types()
 */
void show_type( c_typedef_t const *tdef, decl_flags_t decl_flags, FILE *fout );

/**
 * Shows (prints) the definition of defined types matching \a glob, if any.
 *
 * @param show Which types to show.
 * @param glob The glob string; may be NULL.
 * @param decl_flags The declaration flags to use.
 * @param fout The `FILE` to print to.
 * @return Returns `true` only if at least one type was shown.
 *
 * @sa print_type_decl()
 * @sa show_type()
 */
PJL_DISCARD
bool show_types( cdecl_show_t show, char const *glob, decl_flags_t decl_flags,
                 FILE *fout );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_show_H */
/* vim:set et sw=2 ts=2: */
