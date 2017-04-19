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
#define T_VOID            0x000000000001ull
#define T_AUTO_CPP_11     0x000000000002ull /* C++11 version of "auto" */
#define T_BOOL            0x000000000004ull
#define T_CHAR            0x000000000008ull
#define T_CHAR16_T        0x000000000010ull
#define T_CHAR32_T        0x000000000020ull
#define T_WCHAR_T         0x000000000040ull
#define T_SHORT           0x000000000080ull
#define T_INT             0x000000000100ull
#define T_LONG            0x000000000200ull
#define T_LONG_LONG       0x000000000400ull /* special case */
#define T_SIZE_T          0x000000000800ull /* because it's so common */
#define T_SIGNED          0x000000001000ull
#define T_UNSIGNED        0x000000002000ull
#define T_FLOAT           0x000000004000ull
#define T_DOUBLE          0x000000008000ull
#define T_COMPLEX         0x000000010000ull
#define T_ENUM            0x000000020000ull
#define T_STRUCT          0x000000040000ull
#define T_UNION           0x000000080000ull
#define T_CLASS           0x000000100000ull

// storage classes
#define T_AUTO_C          0x000001000000ull /* C version of "auto" */
#define T_BLOCK           0x000002000000ull /* Apple extension */
#define T_EXTERN          0x000004000000ull
#define T_MUTABLE         0x000008000000ull
#define T_REGISTER        0x000010000000ull
#define T_STATIC          0x000020000000ull
#define T_THREAD_LOCAL    0x000040000000ull
#define T_TYPEDEF         0x000080000000ull

// storage-class-like
#define T_CONSTEXPR       0x000100000000ull
#define T_FINAL           0x000200000000ull
#define T_FRIEND          0x000400000000ull
#define T_INLINE          0x000800000000ull
#define T_NORETURN        0x001000000000ull
#define T_OVERRIDE        0x002000000000ull
#define T_PURE_VIRTUAL    0x004000000000ull
#define T_VIRTUAL         0x008000000000ull

// qualifiers
#define T_ATOMIC          0x010000000000ull
#define T_CONST           0x020000000000ull
#define T_RESTRICT        0x040000000000ull
#define T_VOLATILE        0x080000000000ull

// bit masks
#define T_MASK_TYPE       0x000000FFFFFFull
#define T_MASK_STORAGE    0x00FFFF000000ull
#define T_MASK_QUALIFIER  0x0F0000000000ull

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
