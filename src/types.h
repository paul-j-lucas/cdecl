/*
**      cdecl -- C gibberish translator
**      src/types.h
**
**      Paul J. Lucas
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

////////// types //////////////////////////////////////////////////////////////

// types
#define T_NONE            0
#define T_VOID            0x00000001
#define T_BOOL            0x00000002
#define T_CHAR            0x00000004
#define T_CHAR16_T        0x00000008
#define T_CHAR32_T        0x00000010
#define T_WCHAR_T         0x00000020
#define T_SHORT           0x00000040
#define T_INT             0x00000080
#define T_LONG            0x00000100
#define T_LONG_LONG       0x00000200    /* special case */
#define T_SIGNED          0x00000400
#define T_UNSIGNED        0x00000800
#define T_FLOAT           0x00001000
#define T_DOUBLE          0x00002000
#define T_COMPLEX         0x00004000
#define T_ENUM            0x00008000
#define T_STRUCT          0x00010000
#define T_UNION           0x00020000
#define T_CLASS           0x00040000

// storage classes
#define T_AUTO            0x00100000
#define T_BLOCK           0x00200000    /* Apple extension */
#define T_EXTERN          0x00400000
#define T_REGISTER        0x00800000
#define T_STATIC          0x01000000
#define T_THREAD_LOCAL    0x02000000
#define T_TYPEDEF         0x04000000
#define T_VIRTUAL         0x08000000

// qualifiers
#define T_CONST           0x10000000
#define T_RESTRICT        0x20000000
#define T_VOLATILE        0x40000000

// bit masks
#define T_MASK_TYPE       0x000FFFFF
#define T_MASK_STORAGE    0x0FF00000
#define T_MASK_QUALIFIER  0xF0000000

typedef unsigned c_type_t;

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
 * @return Returns the bitwise-or of the language(s) \a type is illegal in.
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
