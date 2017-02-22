/*
**      cdecl -- C gibberish translator
**      src/types.h
*/

#ifndef cdecl_types_H
#define cdecl_types_H

// local
#include "config.h"

// standard
#include <stdbool.h>

////////// types //////////////////////////////////////////////////////////////

// types
#define T_NONE          0
#define T_VOID          0x00000001
#define T_BOOL          0x00000002
#define T_CHAR          0x00000004
#define T_CHAR16_T      0x00000008
#define T_CHAR32_T      0x00000010
#define T_WCHAR_T       0x00000020
#define T_SHORT         0x00000040
#define T_INT           0x00000080
#define T_LONG          0x00000100
#define T_LONG_LONG     0x00000200
#define T_SIGNED        0x00000400
#define T_UNSIGNED      0x00000800
#define T_FLOAT         0x00001000
#define T_DOUBLE        0x00002000
#define T_COMPLEX       0x00004000
#define T_ENUM          0x00008000
#define T_STRUCT        0x00010000
#define T_UNION         0x00020000
#define T_CLASS         0x00040000

// storage classes
#define T_AUTO          0x00100000
#define T_BLOCK         0x00200000      /* Apple extension */
#define T_EXTERN        0x00400000
#define T_REGISTER      0x00800000
#define T_STATIC        0x01000000
#define T_THREAD_LOCAL  0x02000000
#define T_TYPEDEF       0x04000000

// qualifiers
#define T_CONST         0x10000000
#define T_RESTRICT      0x20000000
#define T_VOLATILE      0x40000000

typedef unsigned c_type_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Checks that the given type is valid.
 *
 * @param type The type to check.
 */
bool c_type_check( c_type_t type );

/**
 * Given a type, get its name.
 *
 * @param type The type to get the name for.
 */
char const* c_type_name( c_type_t type );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_types_H */
/* vim:set et sw=2 ts=2: */
