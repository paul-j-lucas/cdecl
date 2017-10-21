/*
**      cdecl -- C gibberish translator
**      src/c_type.h
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

#ifndef cdecl_c_type_H
#define cdecl_c_type_H

/**
 * @file
 * Declares constants, types, and functions for C/C++ types.
 */

// local
#include "config.h"                     /* must go first */
#include "c_lang.h"                     /* for c_lang_t */
#include "typedefs.h"                   /* for c_loc_t */

// standard
#include <stdbool.h>
#include <inttypes.h>                   /* for PRIX64, etc. */
#include <stdint.h>

////////// types //////////////////////////////////////////////////////////////

// types
#define T_NONE                0
#define T_VOID                0x0000000000000001ull
#define T_AUTO_CPP_11         0x0000000000000002ull /* C++11 version of auto */
#define T_BOOL                0x0000000000000004ull
#define T_CHAR                0x0000000000000008ull
#define T_CHAR16_T            0x0000000000000010ull
#define T_CHAR32_T            0x0000000000000020ull
#define T_WCHAR_T             0x0000000000000040ull
#define T_SHORT               0x0000000000000080ull
#define T_INT                 0x0000000000000100ull
#define T_LONG                0x0000000000000200ull
#define T_LONG_LONG           0x0000000000000400ull /* special case */
#define T_SIGNED              0x0000000000000800ull
#define T_UNSIGNED            0x0000000000001000ull
#define T_FLOAT               0x0000000000002000ull
#define T_DOUBLE              0x0000000000004000ull
#define T_COMPLEX             0x0000000000008000ull
#define T_IMAGINARY           0x0000000000010000ull
#define T_ENUM                0x0000000000020000ull
#define T_STRUCT              0x0000000000040000ull
#define T_UNION               0x0000000000080000ull
#define T_CLASS               0x0000000000100000ull
#define T_TYPEDEF_TYPE        0x0000000000200000ull /* e.g., size_t */

// storage classes
#define T_AUTO_C              0x0000000001000000ull /* C version of auto */
#define T_BLOCK               0x0000000002000000ull /* Apple extension */
#define T_EXTERN              0x0000000004000000ull
#define T_MUTABLE             0x0000000008000000ull
#define T_REGISTER            0x0000000010000000ull
#define T_STATIC              0x0000000020000000ull
#define T_THREAD_LOCAL        0x0000000040000000ull
#define T_TYPEDEF             0x0000000080000000ull

// storage-class-like
#define T_CONSTEXPR           0x0000000100000000ull
#define T_FINAL               0x0000000200000000ull
#define T_FRIEND              0x0000000400000000ull
#define T_INLINE              0x0000000800000000ull
#define T_NOEXCEPT            0x0000001000000000ull
#define T_OVERRIDE            0x0000002000000000ull
#define T_PURE_VIRTUAL        0x0000004000000000ull
#define T_THROW               0x0000008000000000ull
#define T_VIRTUAL             0x0000010000000000ull

// attributes
#define T_CARRIES_DEPENDENCY  0x0000100000000000ull
#define T_DEPRECATED          0x0000200000000000ull
#define T_MAYBE_UNUSED        0x0000400000000000ull
#define T_NODISCARD           0x0000800000000000ull
#define T_NORETURN            0x0001000000000000ull

// qualifiers
#define T_ATOMIC              0x0010000000000000ull
#define T_CONST               0x0020000000000000ull
#define T_RESTRICT            0x0040000000000000ull
#define T_VOLATILE            0x0080000000000000ull

// ref-qualifiers
#define T_REFERENCE           0x0100000000000000ull /* void f() & */
#define T_RVALUE_REFERENCE    0x0200000000000000ull /* void f() && */

// bit masks
#define T_MASK_TYPE           0x0000000000FFFFFFull
#define T_MASK_STORAGE        0x00000FFFFF000000ull
#define T_MASK_ATTRIBUTE      0x000FF00000000000ull
#define T_MASK_QUALIFIER      0x00F0000000000000ull
#define T_MASK_REF_QUALIFIER  0x0F00000000000000ull

/**
 * Bitmask for combination of types.
 */
typedef uint64_t c_type_t;

/**
 * Hexadecimal print conversion specifier for c_type_t.
 */
#define PRIX_C_TYPE_T         PRIX64

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
bool c_type_add( c_type_t *dest_type, c_type_t new_type, c_loc_t const *loc );

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

/**
 * Given a type, get its name for part of an error message.
 * If translating from English to gibberish and the type has an English alias,
 * return the alias, e.g., \c non-returning rather than \c noreturn.
 *
 * @param type The type to get the name for.
 * @return Returns said name.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same \c printf() statement.
 */
char const* c_type_name_error( c_type_t type );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_c_type_H */
/* vim:set et sw=2 ts=2: */
