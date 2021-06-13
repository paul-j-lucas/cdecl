/*
**      cdecl -- C gibberish translator
**      src/c_kind.h
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_KIND_INLINE
# define C_KIND_INLINE _GL_INLINE
#endif /* C_KIND_INLINE */

/// @endcond

/**
 * @defgroup c-kinds-group C/C++ Kinds of Declarations
 * Types and functions for kinds of things in C/C++ declarations.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Kinds of things comprising a C/C++ declaration.
 *
 * A given thing may only have a single kind and _not_ be a bitwise-or of
 * kinds.  However, a bitwise-or of kinds may be used to test whether a given
 * thing is any _one_ of those kinds.
 */
enum c_kind_id {
  K_NONE                    = 0,       ///< No kind.
  K_PLACEHOLDER             = 0x00001, ///< Temporary node in AST.
  K_BUILTIN                 = 0x00002, ///< `void,` `char,` `int,` etc.
  K_NAME                    = 0x00004, ///< Typeless function parameter in K&R C
  K_TYPEDEF                 = 0x00008, ///< `typedef` type, e.g., `size_t`.
  K_VARIADIC                = 0x00010, ///< Variadic (`...`) function parameter.
  // "parent" kinds
  K_ARRAY                   = 0x00080, ///< Array.
  K_ENUM_CLASS_STRUCT_UNION = 0x00100, ///< `enum,` `class,` `struct,` `union`
  K_POINTER                 = 0x00200, ///< Pointer.
  K_POINTER_TO_MEMBER       = 0x00400, ///< Pointer-to-member (C++ only).
  K_REFERENCE               = 0x00800, ///< Reference (C++ only).
  K_RVALUE_REFERENCE        = 0x01000, ///< Rvalue reference (C++ only).
  // function-like "parent" kinds
  K_CONSTRUCTOR             = 0x02000, ///< Constructor (C++ only).
  K_DESTRUCTOR              = 0x04000, ///< Destructor (C++ only).
  // function-like "parent" kinds that have return values
  K_APPLE_BLOCK             = 0x08000, ///< Block.
  K_FUNCTION                = 0x10000, ///< Function.
  K_OPERATOR                = 0x20000, ///< Overloaded operator (C++ only).
  K_USER_DEF_CONVERSION     = 0x40000, ///< User-defined conversion (C++ only).
  K_USER_DEF_LITERAL        = 0x80000, ///< User-defined literal (C++ only).
};

/**
 * Shorthand for any kind of function-like parent: #K_APPLE_BLOCK,
 * #K_CONSTRUCTOR, #K_DESTRUCTOR, #K_FUNCTION, #K_OPERATOR,
 * #K_USER_DEF_CONVERSION, or #K_USER_DEF_LITERAL.
 */
#define K_ANY_FUNCTION_LIKE   ( K_APPLE_BLOCK | K_CONSTRUCTOR | K_DESTRUCTOR \
                              | K_FUNCTION | K_OPERATOR \
                              | K_USER_DEF_CONVERSION \
                              | K_USER_DEF_LITERAL )

/**
 * Shorthand for any kind of "object" that can be the type of a variable or
 * constant, i.e., something to which `sizeof` can be applied: #K_ARRAY,
 * #K_BUILTIN, #K_ENUM_CLASS_STRUCT_UNION, #K_POINTER, #K_POINTER_TO_MEMBER,
 * #K_REFERENCE, #K_RVALUE_REFERENCE, or #K_TYPEDEF.
 */
#define K_ANY_OBJECT          ( K_ARRAY | K_BUILTIN \
                              | K_ENUM_CLASS_STRUCT_UNION | K_POINTER \
                              | K_POINTER_TO_MEMBER | K_REFERENCE \
                              | K_RVALUE_REFERENCE | K_TYPEDEF )

/**
 * Shorthand for any kind of parent: #K_APPLE_BLOCK, #K_ARRAY, #K_CONSTRUCTOR,
 * #K_DESTRUCTOR, #K_ENUM_CLASS_STRUCT_UNION, #K_FUNCTION, #K_OPERATOR,
 * #K_POINTER, #K_POINTER_TO_MEMBER, #K_REFERENCE, #K_RVALUE_REFERENCE,
 * #K_USER_DEF_CONVERSION, or #K_USER_DEF_LITERAL.
 */
#define K_ANY_PARENT          ( K_APPLE_BLOCK | K_ARRAY | K_CONSTRUCTOR \
                              | K_DESTRUCTOR | K_ENUM_CLASS_STRUCT_UNION \
                              | K_FUNCTION | K_OPERATOR | K_POINTER \
                              | K_POINTER_TO_MEMBER | K_REFERENCE \
                              | K_RVALUE_REFERENCE | K_USER_DEF_CONVERSION \
                              | K_USER_DEF_LITERAL )

/**
 * Shorthand for any kind of pointer: #K_POINTER or #K_POINTER_TO_MEMBER.
 */
#define K_ANY_POINTER         ( K_POINTER | K_POINTER_TO_MEMBER )

/**
 * Shorthand for any kind of reference: #K_REFERENCE or #K_RVALUE_REFERENCE.
 */
#define K_ANY_REFERENCE       ( K_REFERENCE | K_RVALUE_REFERENCE )

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the name of \a kind_id.
 *
 * @param kind_id The <code>\ref c_kind_id</code> to get the name for.
 * @return Returns said name.
 */
PJL_WARN_UNUSED_RESULT
char const* c_kind_name( c_kind_id_t kind_id );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_kind_H */
/* vim:set et sw=2 ts=2: */
