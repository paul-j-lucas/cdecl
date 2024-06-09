/*
**      cdecl -- C gibberish translator
**      src/p_kind.h
**
**      Copyright (C) 2023-2024  Paul J. Lucas
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

#ifndef cdecl_p_kind_H
#define cdecl_p_kind_H

/**
 * @file
 * Declares types, macros, and functions for C preprocessor macros.
 */

// local
#include "pjl_config.h"                 /* must go first */

/**
 * @defgroup p-token-kinds-group C Preprocessor Token Kinds
 * Types and functions for kinds of C Preprocessor tokens.
 * @{
 */

////////// enumerations ///////////////////////////////////////////////////////

/**
 * Kinds of C preprocessor tokens.
 *
 * @note While a given token is only of a single kind, kinds can be bitwise-
 * or'd together to test whether a token's kind is any _one_ of those kinds.
 */
enum p_token_kind {
  P_CHAR_LIT    = 1u << 0,              ///< Character literal.
  P_CONCAT      = 1u << 1,              ///< Concatenation operator `##`.
  P_IDENTIFIER  = 1u << 2,              ///< An identifier.
  P_NUM_LIT     = 1u << 3,              ///< Integer or floating point literal.
  P_OTHER       = 1u << 4,              ///< `@`, `$`, or <code>`</code>.

  ///
  /// Placemarker token.
  ///
  /// @remarks
  /// @parblock
  /// This pseudo-token is used when a macro parameter's argument has no
  /// tokens.  This is used by `#` and `##`:
  ///
  ///     cdecl> #define Q2(A,B)        A = # B
  ///     cdecl> expand Q2(x,)
  ///     Q2(x,) => x = # {PLACEMARKER}
  ///     Q2(x,) => x = ""
  ///
  ///     cdecl> #define NAME2(A,B)     A ## B
  ///     cdecl> expand NAME2(,y)
  ///     NAME2(, y) => {PLACEMARKER} ## y
  ///     NAME2(, y) => y
  ///
  /// Without the placemarker, neither `#` nor `##` could distinguish the above
  /// valid cases from invalid cases where they have no argument.
  /// @endparblock
  ///
  P_PLACEMARKER = 1u << 5,

  P_PUNCTUATOR  = 1u << 6,              ///< Operators and other punctuation.

  ///
  /// Whitespace.
  ///
  /// @remarks
  /// @parblock
  /// Ordinarily, whitespace is skipped over by the lexer.  The C preprocessor,
  /// however, needs to maintain whitespace:
  ///
  /// 1. To know if a macro name is _immediately_ followed by a `(` without an
  ///    intervening space to know whether the macro is a function-like macro.
  ///
  /// 2. For stringification via #P_STRINGIFY, e.g.:
  ///    @code
  ///    cdecl> #define Q(X)      #X
  ///    cdecl> expand Q(( a , b ))
  ///    Q(( a , b )) => #X
  ///    Q(( a , b )) => "( a , b )"
  ///    @endcode
  ///
  /// 3. To avoid token pasting via macro parameter expansion forming a
  ///    different token, e.g.:
  ///    @code
  ///    cdecl> #define P(X)      -X
  ///    cdecl> expand P(-)
  ///    P(-) => -X
  ///    | X => -
  ///    P(-) => - -
  ///    @endcode
  ///
  /// 4. To avoid token pasting via comment elision where a comment has to turn
  ///    into a space, e.g.:
  ///    @code
  ///    cdecl> #define P(A,B)    A/**/B
  ///    cdecl> expand P(x,y)
  ///    P(x, y) => A B
  ///    | A => x
  ///    | B => y
  ///    P(x, y) => x y
  ///    @endcode
  /// @endparblock
  ///
  /// @sa avoid_paste()
  ///
  P_SPACE       = 1u << 7,

  P_STRINGIFY   = 1u << 8,              ///< Stringify operator `#`.
  P_STR_LIT     = 1u << 9,              ///< String literal.

  P___VA_ARGS__ = 1u << 10,             ///< `__VA_ARGS__`.
  P___VA_OPT__  = 1u << 11,             ///< `__VA_OPT__`.
};
typedef enum p_token_kind p_token_kind_t;

/**
 * Shorthand for any "opaque" \ref p_token_kind --- all kinds _except_ either
 * #P_PLACEMARKER or #P_SPACE.
 *
 * @sa #P_ANY_TRANSPARENT
 */
#define P_ANY_OPAQUE \
  STATIC_CAST( p_token_kind_t, ~P_ANY_TRANSPARENT )

/**
 * Shorthand for either the #P_CONCAT or #P_STRINGIFY \ref p_token_kind.
 */
#define P_ANY_OPERATOR            ( P_CONCAT | P_STRINGIFY )

/**
 * Shorthand for either the #P_PLACEMARKER or #P_SPACE \ref p_token_kind.
 *
 * @sa #P_ANY_OPAQUE
 */
#define P_ANY_TRANSPARENT         ( P_PLACEMARKER | P_SPACE )

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the name of \a kind.
 *
 * @param kind The \ref p_token_kind to get the name for.
 * @return Returns said name.
 */
NODISCARD
char const* p_kind_name( p_token_kind_t kind );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_p_kind_H */
/* vim:set et sw=2 ts=2: */
