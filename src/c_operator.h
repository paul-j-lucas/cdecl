/*
**      cdecl -- C gibberish translator
**      src/c_operator.h
**
**      Copyright (C) 2018-2023  Paul J. Lucas
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

#ifndef cdecl_c_operator_H
#define cdecl_c_operator_H

/**
 * @file
 * Declares constants, macros, types, and functions for C++ operators.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <limits.h>
#include <stdbool.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_OPERATOR_H_INLINE
# define C_OPERATOR_H_INLINE _GL_INLINE
#endif /* C_OPERATOR_H_INLINE */

/// @endcond

/**
 * @defgroup cpp-operators-group C++ Operators
 * Macros, types, and functions for C++ operators.
 * @{
 */

/**
 * For c_operator::flags, denotes that the operator overloadability (member or
 * non-member) is unspecified.
 *
 * @sa #C_OP_MEMBER
 * @sa #C_OP_NON_MEMBER
 * @sa #C_OP_NOT_OVERLOADABLE
 * @sa #C_OP_OVERLOADABLE
 */
#define C_OP_UNSPECIFIED          C_FUNC_UNSPECIFIED

/**
 * For c_operator::flags, denotes that the operator is overload{able|ed} as a
 * member only.
 *
 * @sa #C_OP_NON_MEMBER
 * @sa #C_OP_NOT_OVERLOADABLE
 * @sa #C_OP_OVERLOADABLE
 * @sa #C_OP_UNSPECIFIED
 */
#define C_OP_MEMBER               C_FUNC_MEMBER

/**
 * Denotes that an operator is overloaded as a non-member.
 *
 * @sa #C_OP_MEMBER
 * @sa #C_OP_NOT_OVERLOADABLE
 * @sa #C_OP_OVERLOADABLE
 * @sa #C_OP_UNSPECIFIED
 */
#define C_OP_NON_MEMBER           C_FUNC_NON_MEMBER

/**
 * For c_operator::flags, denotes that the operator is not overloadable.
 *
 * @sa #C_OP_MEMBER
 * @sa #C_OP_NON_MEMBER
 * @sa #C_OP_OVERLOADABLE
 * @sa #C_OP_UNSPECIFIED
 */
#define C_OP_NOT_OVERLOADABLE     (1u << 2)

/**
 * For c_operator::flags, denotes that the operator is overloadable as either a
 * member or non-member.
 *
 * @sa #C_OP_MEMBER
 * @sa #C_OP_NON_MEMBER
 * @sa #C_OP_NOT_OVERLOADABLE
 * @sa #C_OP_UNSPECIFIED
 */
#define C_OP_OVERLOADABLE         (C_OP_MEMBER | C_OP_NON_MEMBER)

/**
 * For c_operator::params_max of `operator()` or `operator[]` (in C++23 or
 * later), denotes an unlimited number of parameters.
 */
#define C_OP_PARAMS_UNLIMITED     UINT_MAX

///////////////////////////////////////////////////////////////////////////////

/**
 * C++ operator information.
 *
 * @note There can be multiple `c_operator` objects having the sanme \ref
 * oper_id and \ref literal, but with different values for \ref params_min and
 * \ref params_max by \ref lang_ids.  Currently, `operator[]`, where the
 * parameter values change in C++23, is the only such case.
 *
 * @note \ref params_min and \ref params_max comprise the inclusive range for
 * the union of member and non-member versions.  If you know you're dealing
 * with a member operator, use only \ref params_min; if you know you're dealing
 * with a non-member operator, use only \ref params_max; if you don't know
 * which, use both.
 */
struct c_operator {
  c_oper_id_t oper_id;                  ///< ID.
  char const *literal;                  ///< C string literal of the operator.
  c_lang_id_t lang_ids;                 ///< Language(s) OK in.

  /**
   * Bitwise-or of flags specifying whether the operator is a member, non-
   * member, both, or not overloadable.
   *
   * @sa #C_OP_MEMBER
   * @sa #C_OP_NON_MEMBER
   * @sa #C_OP_OVERLOADABLE
   * @sa #C_OP_NOT_OVERLOADABLE
   */
  unsigned    flags;

  unsigned    params_min;               ///< Minimum number of parameters.
  unsigned    params_max;               ///< Maximum number of parameters.
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the c_operator for \a oper_id.
 *
 * @param oper_id The ID of the c_operator to get.
 * @return Returns a pointer to said c_operator.
 */
NODISCARD
c_operator_t const* c_oper_get( c_oper_id_t oper_id );

/**
 * Checks whether the C++ operator is ambiguous.
 *
 * The operators `&`, `*`, `+`, `++`, `-`, and `--`, when declared as:
 *
 *      T operator OP(U);
 *
 * i.e., having one parameter, are ambiguous (to **cdecl**) between being a
 * member or non-member operator since **cdecl** doesn't have the context in
 * which the operator is declared.  If it were declared in-class, e.g.:
 *
 *      class T {
 *      public:
 *        // ...
 *        T& operator OP(U);
 *      };
 *
 * then clearly it's a member operator; if it were declared at file scope, then
 * clearly it's a non-member operator; but **cdecl** doesn't have this context.
 *
 * We can tell if an operator is ambiguous if it can take 1 parameter when \ref
 * c_operator::params_min "params_min" is 0 and \ref c_operator::params_max
 * "params_max" is 2.
 *
 * @param op The C++ operator to check.
 * @return Returns `true` only if the operator is ambiguous.
 */
NODISCARD C_OPERATOR_H_INLINE
bool c_oper_is_ambiguous( c_operator_t const *op ) {
  return op->params_min == 0 && op->params_max == 2;
}

/**
 * Gets the C++ token for the operator having \a oper_id.
 *
 * @param oper_id The ID of the c_operator to get the token for.
 * @return Returns said token (including alternative or graph tokens, if either
 * is enabled); otherwise, returns the unaltered token.
 *
 * @sa alt_token_c()
 * @sa graph_token_c()
 */
NODISCARD
char const* c_oper_token_c( c_oper_id_t oper_id );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_operator_H */
/* vim:set et sw=2 ts=2: */
