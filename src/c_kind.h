/*
**      cdecl -- C gibberish translator
**      src/c_kind.h
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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
 * Declares types and functions for kinds of AST nodes in C/C++ declarations.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/**
 * @defgroup c-kinds-group C/C++ Declaration Kinds
 * Types and functions for kinds of AST nodes in C/C++ declarations.
 * @{
 */

/**
 * Shorthand for use inside a `switch` statement on an AST's \ref c_ast::kind
 * "kind" to:
 *
 * 1. Assert that it's not #K_PLACEHOLDER because it shouldn't occur in a
 *    completed AST.
 *
 * 2. Suppress the warning that otherwise would be given because #K_PLACEHOLDER
 *    is not handled by the `switch`.
 *
 * For example:
 *
 *      switch ( ast->kind ) {
 *        // ...
 *        CASE_K_PLACEHOLDER;
 *      }
 */
#define CASE_K_PLACEHOLDER  \
  case K_PLACEHOLDER:       \
    assert( 0 );            \
    break

///////////////////////////////////////////////////////////////////////////////

/**
 * Shorthand for any kind that can be a bit field.
 *
 * @note Enumerations are allowed to be bit fields only in C++.
 */
#define K_ANY_BIT_FIELD           ( K_BUILTIN | K_ENUM | K_TYPEDEF )

/**
 * Shorthand for either #K_ENUM or #K_CLASS_STRUCT_UNION.
 */
#define K_ANY_ENUM_CLASS_STRUCT_UNION \
                                  ( K_ENUM | K_CLASS_STRUCT_UNION )

/**
 * Shorthand for any kind of function-like AST: #K_APPLE_BLOCK, #K_CONSTRUCTOR,
 * #K_DESTRUCTOR, #K_FUNCTION, #K_OPERATOR, #K_USER_DEF_CONVERSION, or
 * #K_USER_DEF_LITERAL.
 */
#define K_ANY_FUNCTION_LIKE       ( K_APPLE_BLOCK | K_CONSTRUCTOR \
                                  | K_DESTRUCTOR | K_FUNCTION | K_OPERATOR \
                                  | K_USER_DEF_CONVERSION | K_USER_DEF_LITERAL )

/**
 * Shorthand for any kind of "object" that can be the type of a variable or
 * constant, i.e., something to which `sizeof` can be applied: #K_ARRAY,
 * #K_BUILTIN, #K_CLASS_STRUCT_UNION, #K_ENUM, #K_POINTER,
 * #K_POINTER_TO_MEMBER, #K_REFERENCE, #K_RVALUE_REFERENCE, or #K_TYPEDEF.
 */
#define K_ANY_OBJECT              ( K_ANY_POINTER | K_ANY_REFERENCE | K_ARRAY \
                                  | K_ANY_ENUM_CLASS_STRUCT_UNION | K_BUILTIN \
                                  | K_TYPEDEF )

/**
 * Shorthand for any kind of parent: #K_APPLE_BLOCK, #K_ARRAY, #K_CAST,
 * #K_CONSTRUCTOR, #K_DESTRUCTOR, #K_ENUM, #K_FUNCTION, #K_OPERATOR,
 * #K_POINTER, #K_POINTER_TO_MEMBER, #K_REFERENCE, #K_RVALUE_REFERENCE,
 * #K_USER_DEF_CONVERSION, or #K_USER_DEF_LITERAL.
 *
 * @note #K_TYPEDEF is intentionally _not_ included.
 *
 * @sa #K_ANY_REFERRER
 */
#define K_ANY_PARENT              ( K_ANY_FUNCTION_LIKE | K_ANY_POINTER \
                                  | K_ANY_REFERENCE | K_ARRAY | K_CAST \
                                  | K_ENUM )

/**
 * Shorthand for any kind of pointer: #K_POINTER or #K_POINTER_TO_MEMBER.
 */
#define K_ANY_POINTER             ( K_POINTER | K_POINTER_TO_MEMBER )

/**
 * Shorthand for any kind of reference: #K_REFERENCE or #K_RVALUE_REFERENCE.
 */
#define K_ANY_REFERENCE           ( K_REFERENCE | K_RVALUE_REFERENCE )

/**
 * Shorthand for any kind that has a pointer to another AST: #K_ANY_PARENT or
 * #K_TYPEDEF.
 *
 * @sa #K_ANY_PARENT
 */
#define K_ANY_REFERRER            ( K_ANY_PARENT | K_TYPEDEF )

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the name of \a kind.
 *
 * @param kind The \ref c_ast_kind to get the name for.
 * @return Returns said name.
 */
NODISCARD
char const* c_kind_name( c_ast_kind_t kind );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_c_kind_H */
/* vim:set et sw=2 ts=2: */
