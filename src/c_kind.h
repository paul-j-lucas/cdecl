/*
**      cdecl -- C gibberish translator
**      src/c_kind.h
**
**      Copyright (C) 2017-2026  Paul J. Lucas
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

/**
 * @defgroup c-kinds-group C/C++ Declaration Kinds
 * Types and functions for kinds of AST nodes in C/C++ declarations.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Kinds of AST nodes comprising a C/C++ declaration.
 *
 * @note While a given AST node is only of a single kind, kinds can be bitwise-
 * or'd together to test whether an AST node's kind is any _one_ of those
 * kinds. The `K_ANY_*` macros are bitwise-ors of two or more kinds.
 */
enum c_ast_kind {
  /**
   * Temporary node in an AST.
   *
   * @remarks
   * @parblock
   * This is needed in two cases:
   *
   * 1. Array declarations or casts.  Consider:
   *
   *         int a[2][3]
   *
   *    At the first `[`, we know it's an _array 2 of [something of]*_ `int`,
   *    but we don't yet know either what the "something" is or whether it will
   *    turn out to be nothing.  It's not until the second `[` that we know
   *    it's an _array 2 of array 3 of_ `int`.  (Had the `[3]` not been there,
   *    then it would have been just _array 2 of_ `int`.)
   *
   * 2. Nested declarations or casts (inside parentheses).  Consider:
   *
   *         int (*a)[2]
   *
   *    At the `*`, we know it's a _pointer to [something of]*_ `int`, but,
   *    similar to the array case, we don't yet know either what the
   *    "something" is or whether it will turn out to be nothing.  It's not
   *    until the `[` that we know it's a _pointer to array 2 of_ `int`.  (Had
   *    the `[2]` not been there, then it would have been just _pointer to_
   *    `int` (with unnecessary parentheses).
   *
   * In either case, a placeholder node is created to hold the place of the
   * "something" in the AST.
   * @endparblock
   */
  K_PLACEHOLDER         = 1 << 0,

  /**
   * Built-in type, e.g., `void`, `char`, `int`, etc.
   *
   * @sa c_builtin_ast
   */
  K_BUILTIN             = 1 << 1,

  /**
   * C++ lambda capture.
   *
   * @sa c_capture_ast
   */
  K_CAPTURE             = 1 << 2,

  /**
   * A `class,` `struct,` or `union`.
   *
   * @sa c_csu_ast
   */
  K_CLASS_STRUCT_UNION  = 1 << 3,

  /**
   * C++ concept.
   *
   * @sa c_concept_ast
   */
  K_CONCEPT             = 1 << 4,

  /**
   * Name only.
   *
   * @remarks
   * @parblock
   * This is used in two cases:
   *
   *  1. An initial kind for an identifier ("name") until we know its actual
   *     type (if ever).
   *
   *  2. A pre-prototype typeless function definition parameter in K&R&nbsp;C,
   *     e.g., <code>double&nbsp;sin(x)</code>.
   * @endparblock
   */
  K_NAME                = 1 << 5,

  /**
   * A `typedef` type, e.g., `size_t`.
   *
   * @sa c_typedef_ast
   */
  K_TYPEDEF             = 1 << 6,

  /**
   * Variadic (`...`) function parameter.
   */
  K_VARIADIC            = 1 << 7,

  ////////// "parent" kinds ///////////////////////////////////////////////////

  /**
   * Array.
   *
   * @sa c_array_ast
   */
  K_ARRAY               = 1 << 8,

  /**
   * Cast.
   *
   * @sa c_cast_ast
   */
  K_CAST                = 1 << 9,

  /**
   * An `enum`.
   *
   * @note This is a "parent" kind because `enum` in C23/C++11 and later can be
   * "of" a fixed type.
   *
   * @sa c_enum_ast
   */
  K_ENUM                = 1 << 10,

  /**
   * Pointer.
   *
   * @sa c_ptr_ref_ast
   */
  K_POINTER             = 1 << 11,

  /**
   * C++ pointer-to-member.
   *
   * @sa c_ptr_mbr_ast
   */
  K_POINTER_TO_MEMBER   = 1 << 12,

  /**
   * C++ reference.
   *
   * @sa c_ptr_ref_ast
   */
  K_REFERENCE           = 1 << 13,

  /**
   * C++ rvalue reference.
   *
   * @sa c_ptr_ref_ast
   */
  K_RVALUE_REFERENCE    = 1 << 14,

  /**
   * C++ structured binding.
   *
   * @sa c_struct_bind_ast
   */
  K_STRUCTURED_BINDING  = 1 << 15,

  ////////// function-like "parent" kinds /////////////////////////////////////

  /**
   * C++ constructor.
   *
   * @sa c_constructor_ast
   */
  K_CONSTRUCTOR         = 1 << 16,

  /**
   * C++ destructor.
   */
  K_DESTRUCTOR          = 1 << 17,

  ////////// function-like "parent" kinds that have return types //////////////

  /**
   * Block (Apple extension).
   *
   * @sa c_apple_block_ast
   * @sa [Apple's Extensions to C](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1370.pdf)
   * @sa [Blocks Programming Topics](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Blocks)
   */
  K_APPLE_BLOCK         = 1 << 18,

  /**
   * Function.
   *
   * @sa c_function_ast
   */
  K_FUNCTION            = 1 << 19,

  /**
   * C++ lambda.
   *
   * @sa c_lambda_ast
   */
  K_LAMBDA              = 1 << 20,

  /**
   * C++ overloaded operator.
   *
   * @sa c_operator_ast
   */
  K_OPERATOR            = 1 << 21,

  /**
   * C++ user-defined conversion operator.
   *
   * @sa c_udef_conv_ast
   */
  K_USER_DEFINED_CONV   = 1 << 22,

  /**
   * C++ user-defined literal.
   *
   * @sa c_udef_lit_ast
   */
  K_USER_DEFINED_LIT    = 1 << 23,
};
typedef enum c_ast_kind c_ast_kind_t;

///////////////////////////////////////////////////////////////////////////////

/**
 * Shorthand for any kind that can be a bit field: #K_BUILTIN, #K_ENUM, or
 * #K_TYPEDEF.
 *
 * @note Enumerations are allowed to be bit fields only in C++.
 *
 * @sa c_ast_check_enum()
 * @sa #LANG_enum_BITFIELDS
 */
#define K_ANY_BIT_FIELD           ( K_BUILTIN | K_ENUM | K_TYPEDEF )

/**
 * Shorthand for either #K_ENUM or #K_CLASS_STRUCT_UNION.
 */
#define K_ANY_ECSU                ( K_ENUM | K_CLASS_STRUCT_UNION )

/**
 * Shorthand for any kind of function-like AST: #K_APPLE_BLOCK, #K_CONSTRUCTOR,
 * #K_DESTRUCTOR, #K_FUNCTION, #K_LAMBDA, #K_OPERATOR, #K_USER_DEFINED_CONV, or
 * #K_USER_DEFINED_LIT.
 *
 * @sa #K_ANY_FUNCTION_RETURN
 * @sa #K_ANY_TRAILING_RETURN
 */
#define K_ANY_FUNCTION_LIKE       ( K_ANY_FUNCTION_RETURN | K_CONSTRUCTOR \
                                  | K_DESTRUCTOR )

/**
 * Shorthand for any kind of function-like AST that has a return type:
 * #K_APPLE_BLOCK, #K_FUNCTION, #K_LAMBDA, #K_OPERATOR, #K_USER_DEFINED_CONV,
 * or #K_USER_DEFINED_LIT.
 *
 * @sa #K_ANY_FUNCTION_LIKE
 * @sa #K_ANY_TRAILING_RETURN
 */
#define K_ANY_FUNCTION_RETURN     ( K_ANY_TRAILING_RETURN | K_APPLE_BLOCK \
                                  | K_USER_DEFINED_CONV | K_USER_DEFINED_LIT )

/**
 * Shorthand for any kind that has a name: #K_CLASS_STRUCT_UNION, #K_CONCEPT,
 * #K_ENUM, or #K_POINTER_TO_MEMBER.
 *
 * @sa #K_ANY_OBJECT
 */
#define K_ANY_NAME                ( K_ANY_ECSU | K_CONCEPT | K_NAME \
                                  | K_POINTER_TO_MEMBER )

/**
 * Shorthand for any kind of "object" that can be the type of a variable or
 * constant, i.e., something to which `sizeof` can be applied _except_ pointers
 * or references: #K_ARRAY, #K_BUILTIN, #K_CLASS_STRUCT_UNION, #K_CONCEPT,
 * #K_ENUM, #K_NAME, or #K_TYPEDEF.
 *
 * @sa #K_ANY_NAME
 * @sa #K_ANY_OBJECT
 */
#define K_ANY_NON_PTR_REF_OBJECT  ( K_ANY_TYPE_SPECIFIER | K_ARRAY \
                                  | K_CONCEPT | K_TYPEDEF )

/**
 * Shorthand for any kind of "object" that can be the type of a variable or
 * constant, i.e., something to which `sizeof` can be applied: #K_ARRAY,
 * #K_BUILTIN, #K_CLASS_STRUCT_UNION, #K_ENUM, #K_POINTER,
 * #K_POINTER_TO_MEMBER, #K_REFERENCE, #K_RVALUE_REFERENCE, or #K_TYPEDEF.
 *
 * @sa #K_ANY_NAME
 * @sa #K_ANY_NON_PTR_REF_OBJECT
 */
#define K_ANY_OBJECT              ( K_ANY_NON_PTR_REF_OBJECT \
                                  | K_ANY_POINTER_OR_REFERENCE )

/**
 * Shorthand for any kind of parent: #K_APPLE_BLOCK, #K_ARRAY, #K_CAST,
 * #K_ENUM, #K_FUNCTION, #K_OPERATOR, #K_POINTER, #K_POINTER_TO_MEMBER,
 * #K_REFERENCE, #K_RVALUE_REFERENCE, #K_USER_DEFINED_CONV, or
 * #K_USER_DEFINED_LIT.
 *
 * @note #K_TYPEDEF is intentionally _not_ included.
 *
 * @sa c_typedef_ast
 * @sa #K_ANY_REFERRER
 */
#define K_ANY_PARENT              ( K_ANY_FUNCTION_RETURN | K_ANY_POINTER \
                                  | K_ANY_REFERENCE | K_ARRAY | K_CAST \
                                  | K_ENUM )

/**
 * Shorthand for any kind of pointer: #K_POINTER or #K_POINTER_TO_MEMBER.
 *
 * @sa #K_ANY_POINTER_OR_REFERENCE
 */
#define K_ANY_POINTER             ( K_POINTER | K_POINTER_TO_MEMBER )

/**
 * Shorthand for any kind of pointer or reference: #K_POINTER,
 * #K_POINTER_TO_MEMBER, #K_REFERENCE, or #K_RVALUE_REFERENCE.
 *
 * @sa #K_ANY_POINTER
 * @sa #K_ANY_REFERENCE
 */
#define K_ANY_POINTER_OR_REFERENCE \
                                  ( K_ANY_POINTER | K_ANY_REFERENCE )

/**
 * Shorthand for any kind of reference: #K_REFERENCE or #K_RVALUE_REFERENCE.
 *
 * @sa #K_ANY_POINTER_OR_REFERENCE
 */
#define K_ANY_REFERENCE           ( K_REFERENCE | K_RVALUE_REFERENCE )

/**
 * Shorthand for any kind that has a pointer to another AST: #K_ANY_PARENT or
 * #K_TYPEDEF.
 *
 * @sa #K_ANY_PARENT
 */
#define K_ANY_REFERRER            ( K_ANY_PARENT | K_TYPEDEF )

/**
 * Shorthand for any kind of function-like AST that can have a trailing return
 * type: #K_FUNCTION, #K_LAMBDA, or #K_OPERATOR.
 *
 * @sa #K_ANY_FUNCTION_LIKE
 * @sa #K_ANY_FUNCTION_RETURN
 */
#define K_ANY_TRAILING_RETURN     ( K_FUNCTION | K_LAMBDA | K_OPERATOR )

/**
 * Shorthand for any kind that can be a "type specifier" in a declaration, that
 * is the type on the left-hand side: #K_BUILTIN, #K_CLASS_STRUCT_UNION,
 * #K_ENUM, #K_NAME, or #K_TYPEDEF.
 */
#define K_ANY_TYPE_SPECIFIER      ( K_BUILTIN | K_ANY_ECSU | K_NAME \
                                  | K_TYPEDEF )

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
