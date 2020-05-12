/*
**      cdecl -- C gibberish translator
**      src/c_kind.h
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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

#ifndef cdecl_c_kind_H
#define cdecl_c_kind_H

/**
 * @file
 * Declares types and functions for kinds of things in C/C++ declarations.
 */

#ifndef CDECL_CONFIGURE                 /* defined only during ../configure */
//
// During configure time, we need to get sizeof(c_kind_t), so we #include this
// header.  However, this header indirectly includes config.h that doesn't get
// created until after configure finishes.
//
// Therefore, configure defines CDECL_CONFIGURE so that we can #include this
// header that will NOT #include anything else (or define anything that depends
// on #include'd definitions).
//

// local
#include "cdecl.h"                      /* must go first */
#include "typedefs.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_KIND_INLINE
# define C_KIND_INLINE _GL_INLINE
#endif /* C_KIND_INLINE */

/// @endcond

#else /* CDECL_CONFIGURE */

// local
#include "typedefs.h"                   /* for c_kind_t */

#endif /* CDECL_CONFIGURE */

/**
 * @defgroup c-kinds-group C/C++ Kinds of Declarations
 * Types and functions for kinds of things in C/C++ declarations.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Kinds of things comprising a C/C++ declaration.
 *
 * A given thing may only have a single kind and \e not be a bitwise-or of
 * kinds.  However, a bitwise-or of kinds may be used to test whether a given
 * thing is any \e one of those kinds.
 */
enum c_kind {
  K_NONE                    = 0,        ///< No kind.
  K_PLACEHOLDER             = 0x00001,  ///< Temporary node in AST.
  K_BUILTIN                 = 0x00002,  ///< `void,` `char,` `int,` etc.
  K_ENUM_CLASS_STRUCT_UNION = 0x00004,  ///< `enum,` `class,` `struct,` `union`
  K_NAME                    = 0x00008,  ///< Typeless function argument in K&R C
  K_TYPEDEF                 = 0x00010,  ///< `typedef` type, e.g., `size_t`.
  K_VARIADIC                = 0x00020,  ///< Variadic (`...`) function argument.
  // "parent" kinds
  K_ARRAY                   = 0x00100,  ///< Array.
  K_POINTER                 = 0x00200,  ///< Pointer.
  K_POINTER_TO_MEMBER       = 0x00400,  ///< Pointer-to-member (C++ only).
  K_REFERENCE               = 0x00800,  ///< Reference (C++ only).
  K_RVALUE_REFERENCE        = 0x01000,  ///< Rvalue reference (C++ only).
  // function-like "parent" kinds
  K_CONSTRUCTOR             = 0x02000,  ///< Constructor (C++ only).
  K_DESTRUCTOR              = 0x04000,  ///< Destructor (C++ only).
  // function-like "parent" kinds that have return values
  K_APPLE_BLOCK             = 0x08000,  ///< Block.
  K_FUNCTION                = 0x10000,  ///< Function.
  K_OPERATOR                = 0x20000,  ///< Overloaded operator (C++ only).
  K_USER_DEF_CONVERSION     = 0x40000,  ///< User-defined conversion (C++ only).
  K_USER_DEF_LITERAL        = 0x80000,  ///< User-defined literal (C++ only).
};

#ifndef CDECL_CONFIGURE

/**
 * Shorthand for any kind of pointer: #K_POINTER or #K_POINTER_TO_MEMBER.
 */
#define K_ANY_POINTER         (K_POINTER | K_POINTER_TO_MEMBER)

/**
 * Shorthand for any kind of reference: #K_REFERENCE or #K_RVALUE_REFERENCE.
 */
#define K_ANY_REFERENCE       (K_REFERENCE | K_RVALUE_REFERENCE)

#define K_MASK_PARENT         0xFFF00   /**< Parent bitmask. */
#define K_MASK_FUNCTION_LIKE  0xFE000   /**< Function-like bitmask. */

////////// inline functions ///////////////////////////////////////////////////

/**
 * Frees the data for a `c_kind_t`.
 * @note
 * For platforms with 64-bit pointers, this is a no-op.
 *
 * @param data The data to free.
 */
C_KIND_INLINE
void c_kind_data_free( void *data ) {
#if SIZEOF_C_KIND_T > SIZEOF_VOIDP
  free( data );
#else
  (void)data;
#endif /* SIZEOF_C_KIND_T > SIZEOF_VOIDP */
}

/**
 * Gets the `c_kind_t` value from a `void*` data value.
 *
 * @param data The data to get the `c_kind_t` from.
 * @return Returns the `c_kind_t`.
 *
 * @sa c_kind_data_new(c_kind_t)
 */
C_WARN_UNUSED_RESULT C_KIND_INLINE
c_kind_t c_kind_data_get( void *data ) {
#if SIZEOF_C_KIND_T > SIZEOF_VOIDP
  return *REINTERPRET_CAST( c_kind_t*, data );
#else
  return REINTERPRET_CAST( c_kind_t, data );
#endif /* SIZEOF_C_KIND_T > SIZEOF_VOIDP */
}

/**
 * Creates an opaque data handle for a `c_kind_t`.
 *
 * @param kind_id The `c_kind_t` to use.
 * @return Returns said handle.
 *
 * @sa c_kind_data_free(void*)
 */
C_WARN_UNUSED_RESULT C_KIND_INLINE
void* c_kind_data_new( c_kind_t kind_id ) {
#if SIZEOF_C_KIND_T > SIZEOF_VOIDP
  c_kind_t *const data = MALLOC( c_kind_t, 1 );
  *data = kind_id;
  return data;
#else
  return REINTERPRET_CAST( void*, kind_id );
#endif /* SIZEOF_C_KIND_T > SIZEOF_VOIDP */
}

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the name of \a kind_id.
 *
 * @param kind_id The <code>\ref c_kind</code> to get the name for.
 * @return Returns said name.
 */
C_WARN_UNUSED_RESULT
char const* c_kind_name( c_kind_t kind_id );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* CDECL_CONFIGURE */
#endif /* cdecl_c_kind_H */
/* vim:set et sw=2 ts=2: */
