/*
**      cdecl -- C gibberish translator
**      src/types.h
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

#ifndef cdecl_types_H
#define cdecl_types_H

/**
 * @file
 * Declares constants, types, and functions for C/C++ types.
 */

// local
#include "config.h"                     /* must go first */
#include "common.h"                     /* for YYLTYPE */
#include "lang.h"                       /* for c_lang_t */

// standard
#include <stdbool.h>
#include <stdint.h>

////////// types //////////////////////////////////////////////////////////////

// types
#define T_NONE            0
#define T_VOID            0x0000000001ul
#define T_BOOL            0x0000000002ul
#define T_CHAR            0x0000000004ul
#define T_CHAR16_T        0x0000000008ul
#define T_CHAR32_T        0x0000000010ul
#define T_WCHAR_T         0x0000000020ul
#define T_SHORT           0x0000000040ul
#define T_INT             0x0000000080ul
#define T_LONG            0x0000000100ul
#define T_LONG_LONG       0x0000000200ul  /* special case */
#define T_SIZE_T          0x0000000400ul  /* because it's so common */
#define T_SIGNED          0x0000000800ul
#define T_UNSIGNED        0x0000001000ul
#define T_FLOAT           0x0000002000ul
#define T_DOUBLE          0x0000004000ul
#define T_COMPLEX         0x0000008000ul
#define T_ENUM            0x0000010000ul
#define T_STRUCT          0x0000020000ul
#define T_UNION           0x0000040000ul
#define T_CLASS           0x0000080000ul

// storage classes
#define T_AUTO            0x0000100000ul
#define T_BLOCK           0x0000200000ul  /* Apple extension */
#define T_EXTERN          0x0000400000ul
#define T_FRIEND          0x0000800000ul
#define T_REGISTER        0x0001000000ul
#define T_STATIC          0x0002000000ul
#define T_THREAD_LOCAL    0x0004000000ul
#define T_TYPEDEF         0x0008000000ul
#define T_VIRTUAL         0x0010000000ul
#define T_PURE_VIRTUAL    0x0020000000ul

// qualifiers
#define T_CONST           0x0100000000ul
#define T_RESTRICT        0x0200000000ul
#define T_VOLATILE        0x0400000000ul

// bit masks
#define T_MASK_TYPE       0x00000FFFFFul
#define T_MASK_STORAGE    0x00FFF00000ul
#define T_MASK_QUALIFIER  0xFF00000000ul

typedef uint64_t c_type_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Adds a type to an existing type, e.g., \c short to \c int, ensuring that a
 * particular type is never added more than once, e.g., \c int to \c int.
 *
 * A special case has to be made for \c long to allow for <code>long
 * long</code> yet not allow for <code>long long long</code>.
 *
 * @param dest_type A pointer to the type to add to.
 * @param new_type The type to add.
 * @param loc The source location of \a new_type.
 * @return Returns \c true only if the type added successfully.
 */
bool c_type_add( c_type_t *dest_type, c_type_t new_type, YYLTYPE const *loc );

/**
 * Checks that the given type is valid.
 *
 * @param type The type to check.
 * @return Returns the bitwise-or of the language(s) \a type is legal in.
 */
c_lang_t c_type_check( c_type_t type );

/**
 * Given a type, get its name.
 *
 * @param type The type to get the name for.
 * @return Returns said name.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same \c printf() statement.
 */
char const* c_type_name( c_type_t type );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_types_H */
/* vim:set et sw=2 ts=2: */
