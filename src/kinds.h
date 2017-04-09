/*
**      cdecl -- C gibberish translator
**      src/kinds.h
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

#ifndef cdecl_kinds_H
#define cdecl_kinds_H

/**
 * @file
 * Declares types and functions for kinds of things in C/C++ declarations.
 */

// local
#include "config.h"                     /* must go first */

// standard
#include <stdbool.h>

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_KINDS_INLINE
# define CDECL_KINDS_INLINE _GL_INLINE
#endif /* CDECL_KINDS_INLINE */

///////////////////////////////////////////////////////////////////////////////

/**
 * Kinds of things comprising a C/C++ declaration.
 *
 * A given thing may only have a single kind and \e not be a bitwise-or of
 * kinds.  However, a bitwise-or of kinds may be used to test whether a given
 * thing is any \e one of those kinds.
 */
enum c_kind {
  K_NONE                    = 0x0001,
  K_BUILTIN                 = 0x0002,   // void, char, int, etc.
  K_ENUM_CLASS_STRUCT_UNION = 0x0004,
  K_NAME                    = 0x0008,   // typeless function argument in K&R C
  K_VARIADIC                = 0x0010,   // variadic ("...") function argument
  // "parent" kinds
  K_ARRAY                   = 0x0020,
  K_BLOCK                   = 0x0040,   // Apple extension
  K_FUNCTION                = 0x0080,
  K_POINTER                 = 0x0100,
  // "parent" kinds (C++ only)
  K_POINTER_TO_MEMBER       = 0x0200,
  K_REFERENCE               = 0x0400,
  K_RVALUE_REFERENCE        = 0x0800,
};
typedef enum c_kind c_kind_t;

#define K_PARENT_MIN          K_ARRAY

////////// extern functions ///////////////////////////////////////////////////

/**
 * Checks whether the given AST node is a parent node.
 *
 * @param ast The \c c_ast to check.  May be null.
 * @return Returns \c true only if it is.
 */
CDECL_KINDS_INLINE bool c_kind_is_parent( c_kind_t kind ) {
  return kind >= K_PARENT_MIN;
}

/**
 * Gets the name of the given kind.
 *
 * @param kind The kind to get the name for.
 * @return Returns said name.
 */
char const* c_kind_name( c_kind_t kind );

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_kinds_H */
/* vim:set et sw=2 ts=2: */
