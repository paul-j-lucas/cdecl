/*
**      cdecl -- C gibberish translator
**      src/p_macro.h
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

#ifndef cdecl_p_macro_H
#define cdecl_p_macro_H

/**
 * @file
 * Declares types, macros, and functions for C preprocessor macros.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "slist.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdio.h>                      /* for FILE */

_GL_INLINE_HEADER_BEGIN
#ifndef P_MACRO_H_INLINE
# define P_MACRO_H_INLINE _GL_INLINE
#endif /* P_MACRO_H_INLINE */

/// @endcond

/**
 * @defgroup p-macro-group C Preprocessor Macros
 * Types and functions for C Preprocessor macros.
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
  P_CHAR_LIT    = (1u << 0),            ///< Character literal.
  P_CONCAT      = (1u << 1),            ///< `##`.
  P_IDENTIFIER  = (1u << 2),            ///< An identifier.
  P_NUM_LIT     = (1u << 3),            ///< Integer or floating point literal.
  P_OTHER       = (1u << 4),            ///< `@`, `$`, or <code>`</code>.

  ///
  /// Placemarker token.
  ///
  /// @remarks
  /// @parblock
  /// This pseudo-token is used when a macro parameter's argument has no
  /// tokens.  This is used by `#` and `##`:
  ///
  ///     #define Q2(A,B)         A = # B
  ///     expand Q2(x,)
  ///     Q2(x,) => x = # {PLACEMARKER}
  ///     Q2(x,) => x = ""
  ///
  ///     #define NAME2(A,B)      A ## B
  ///     expand NAME2(,y)
  ///     NAME2(, y) => {PLACEMARKER} ## y
  ///     NAME2(, y) => y
  /// @endparblock
  ///
  P_PLACEMARKER = (1u << 5),

  P_PUNCTUATOR  = (1u << 6),            ///< Operators and other punctuation.

  ///
  /// Whitespace.
  ///
  /// @remarks Ordinarily, whitespace is skipped over by the lexer.  The C
  /// preprocessor, however, needs to maintain whitespace to know whether a
  /// function-like macro name is _immediately_ followed by a `(` without an
  /// intervening space to know whether to perform expansion on it.
  ///
  P_SPACE       = (1u << 7),

  P_STRINGIFY   = (1u << 8),            ///< `#`.
  P_STR_LIT     = (1u << 9),            ///< String literal.

  P___VA_ARGS__ = (1u << 10),           ///< `__VA_ARGS__`.
  P___VA_OPT__  = (1u << 11),           ///< `__VA_OPT__`.
};

////////// typedefs ///////////////////////////////////////////////////////////

/**
 * The signature for a function to generate a macro's value dynamically.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Returns the bitwise-or of languages the macro has a value in.
 */
typedef c_lang_id_t (*p_macro_dyn_fn_t)( p_token_t **ptoken );

/**
 * The signature for a function passed to p_macro_visit().
 *
 * @param macro The \ref p_macro to visit.
 * @param v_data Optional data passed to the visitor.
 * @return Returning `true` will cause traversal to stop and a pointer to the
 * \ref p_macro the visitor stopped on to be returned to the caller of
 * c_macro_visit().
 */
typedef bool (*p_macro_visit_fn_t)( p_macro_t const *macro, void *v_data );

typedef enum p_token_kind p_token_kind_t;

////////// structs ////////////////////////////////////////////////////////////

/**
 * C preprocessor macro parameter.
 */
struct p_param {
  char const *name;                     ///< Parameter name.
  c_loc_t     loc;                      ///< Source location.
};

/**
 * C preprocessor token.
 */
struct p_token {
  p_token_kind_t  kind;                 ///< Token kind.
  c_loc_t         loc;                  ///< Source location.
  bool            is_substituted;       ///< Substituted from argument?

  /**
   * Additional data for each \ref kind.
   */
  union {
    /**
     * #P_IDENTIFIER members.
     */
    struct {
      char const *name;                 ///< Identifier name.
      bool        ineligible;           ///< Ineligible for expansion?
    } ident;

    /**
     * #P_CHAR_LIT, #P_NUM_LIT, or #P_STR_LIT members.
     */
    struct {
      char const *value;
    } lit;

    /**
     * #P_OTHER members.
     */
    struct {
      char        value;                ///< #P_OTHER value.
    } other;

    /**
     * #P_PUNCTUATOR members.
     */
    struct {
      ///
      /// #P_PUNCTUATOR value.
      ///
      /// @remarks It's large enough to hold the longest operators of `->*`,
      /// `<<=`, `<=>`, or `>>=`, plus a terminating `\0`.
      ///
      char        value[4];
    } punct;
  };
};

/**
 * C preprocessor macro.
 */
struct p_macro {
  char const         *name;             ///< Macro name.
  bool                is_dynamic;       ///< Is value dynamically generated?

  /**
   * Additional data based on whether \ref is_dynamic is `true` or `false`.
   */
  union {
    p_macro_dyn_fn_t  dyn_fn;           ///< Dynamic value function.

    /**
     * Static macro members.
     */
    struct {
      /**
       * Parameter(s), if any.
       *
       * @remarks This is a pointer to a \ref p_param_list_t rather than a \ref
       * p_param_list_t so that NULL can distinguish an object-like macro from
       * a function-like macro that has zero parameters.
       */
      p_param_list_t *param_list;

      p_token_list_t  replace_list;     ///< Replacement tokens, if any.
    };
  };
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Frees all memory used by \a arg_list but _not_ \a arg_list itself.
 *
 * @param arg_list The macro argument list to cleanup.
 */
void p_arg_list_cleanup( p_arg_list_t *arg_list );

/**
 * Gets the name of \a kind.
 *
 * @param kind The \ref p_token_kind to get the name for.
 * @return Returns said name.
 */
NODISCARD
char const* p_kind_name( p_token_kind_t kind );

/**
 * Defines a new \ref p_macro.
 *
 * @param name The name of the macro to define.  Ownership is taken only if the
 * macro is defined successfully.
 * @param name_loc The source location of \a name.
 * @param param_list The parameter list, if any.
 * @param replace_list The replacement token list, if any.
 * @return Returns a pointer to the new macro or NULL if unsuccessful.
 *
 * @sa p_macro_undef()
 */
NODISCARD
p_macro_t* p_macro_define( char *name, c_loc_t const *name_loc,
                           p_param_list_t *param_list,
                           p_token_list_t *replace_list );

/**
 * Expands a macro named \a name using \a arg_list.
 *
 * @param name The name of the macro to expand.
 * @param name_loc The source location of the macro's name.
 * @param arg_list The list of macro argument tokens, if any.
 * @param extra_list The list of extra tokens at the end of the `expand`
 * command, if any.
 * @param fout The `FILE` to print to.
 * @return Returns `true` only if the macro expanded successfully.
 */
NODISCARD
bool p_macro_expand( char const *name, c_loc_t const *name_loc,
                     p_arg_list_t *arg_list, p_token_list_t *extra_list,
                     FILE *fout );

/**
 * Gets the \ref p_macro having \a name.
 *
 * @param name The name of the macro to find.
 * @return Returns a pointer to the \ref p_macro having \a name or NULL if it's
 * not defined.
 */
NODISCARD
p_macro_t const* p_macro_find( char const *name );

/**
 * Undefines a macro having \a name.
 *
 * @param name The name of the macro to undefine.
 * @param name_loc The source location of \a name.
 * @return Returns `true` only if a macro having \a name was undefined.
 *
 * @sa p_macro_define()
 */
NODISCARD
bool p_macro_undef( char const *name, c_loc_t const *name_loc );

/**
 * Does an in-order traversal of all \ref p_macro.
 *
 * @param visit_fn The visitor function to use.
 * @param v_data Optional data passed to \a visit_fn.
 * @return Returns a pointer to the \ref p_macro the visitor stopped on or
 * NULL.
 */
PJL_DISCARD
p_macro_t const* p_macro_visit( p_macro_visit_fn_t visit_fn, void *v_data );

/**
 * Initializes all C preprocessor macro data.
 *
 * @note This function must be called exactly once.
 */
void p_macros_init( void );

/**
 * Frees all memory used by \a param _including_ \a param itself.
 *
 * @param param The \ref p_param to free.  If NULL, does nothing.
 *
 * @sa p_param_list_cleanup()
 */
void p_param_free( p_param_t *param );

/**
 * Cleans-up \a param_list by freeing only its nodes but _not_ \a param_list
 * itself.
 *
 * @param param_list The list of \ref p_param to free.
 *
 * @sa p_param_free()
 */
void p_param_list_cleanup( p_param_list_t *param_list );

/**
 * Frees all memory used by \a token _including_ \a token itself.
 *
 * @param token The \ref p_token to free.  If NULL, does nothing.
 *
 * @sa p_token_list_cleanup()
 * @sa p_token_new()
 * @sa p_token_new_loc()
 */
void p_token_free( p_token_t *token );

/**
 * Checks whether the #P_PUNCTUATOR \a token is _any single_ character.
 *
 * @param token The #P_PUNCTUATOR \ref p_token to check.
 * @return Returns `true` only if it is.
 *
 * @sa p_punct_token_is_char()
 * @sa p_token_is_any_char()
 */
NODISCARD P_MACRO_H_INLINE
bool p_punct_token_is_any_char( p_token_t const *token ) {
  return token->punct.value[1] == '\0';
}

/**
 * Checks whether the #P_PUNCTUATOR \a token is equal to \a c.
 *
 * @param token The #P_PUNCTUATOR \ref p_token to check.
 * @param c The character to check.
 * @return Returns `true` only if it is.
 *
 * @sa p_punct_token_is_any_char()
 * @sa p_token_is_punct()
 */
NODISCARD P_MACRO_H_INLINE
bool p_punct_token_is_char( p_token_t const *token, char c ) {
  return token->punct.value[0] == c && p_punct_token_is_any_char( token );
}

/**
 * Checks whether the #P_PUNCTUATOR \a token is any _single_ character.
 *
 * @param token The #P_PUNCTUATOR \ref p_token to check.
 * @return Returns `true` only if it is.
 *
 * @sa p_punct_token_is_any_char()
 * @sa p_token_is_punct()
 */
NODISCARD P_MACRO_H_INLINE
bool p_token_is_any_char( p_token_t const *token ) {
  return token->kind == P_PUNCTUATOR && p_punct_token_is_any_char( token );
}

/**
 * Checks whether \a token is of kind #P_PUNCTUATOR and if it's equal to \a
 * punct.
 *
 * @param token The \ref p_token to check.
 * @param punct The punctuation character to check.
 * @return Returns `true` only if it is.
 *
 * @sa p_punct_token_is_char()
 * @sa p_token_is_any_char()
 */
NODISCARD P_MACRO_H_INLINE
bool p_token_is_punct( p_token_t const *token, char punct ) {
  return token->kind == P_PUNCTUATOR && p_punct_token_is_char( token, punct );
}

/**
 * Cleans-up \a token_list by freeing only its nodes but _not_ \a token_list
 * itself.
 *
 * @param token_list The list of \ref p_token to free.
 *
 * @sa p_token_free()
 */
void p_token_list_cleanup( p_token_list_t *token_list );

/**
 * Creates a new \ref p_token.
 *
 * @param kind The kind of token to create.
 * @param loc The source location, if any.
 * @param literal
 * @parblock
 * The literal for the token, if any.  If \a kind is:
 *  + #P_CHAR_LIT, #P_IDENTIFIER, #P_NUM_LIT, or #P_STR_LIT, ownership of \a
 *    literal is taken (so it might need to be duplicated first);
 *
 * Otherwise, ownership of \a literal is _not_ taken; however, if \a kind is:
 *  + #P_OTHER, only \a literal<code>[0]</code> is copied;
 *  + #P_PUNCTUATOR, \a literal is copied;
 *  + Any other kind, \a literal is not used.
 * @endparblock
 * @return Returns a pointer to a new \ref p_token.  The caller is responsible
 * for freeing it.
 *
 * @sa p_token_free()
 * @sa p_token_new()
 */
NODISCARD
p_token_t* p_token_new_loc( p_token_kind_t kind, c_loc_t const *loc,
                            char const *literal );

/**
 * Creates a new \ref p_token.
 *
 * @param kind The kind of token to create.
 * @param literal
 * @parblock
 * The literal for the token, if any.  If \a kind is:
 *  + #P_CHAR_LIT, #P_IDENTIFIER, #P_NUM_LIT, or #P_STR_LIT, ownership of \a
 *    literal is taken (so it might need to be duplicated first);
 *
 * Otherwise, ownership of \a literal is _not_ taken; however, if \a kind is:
 *  + #P_OTHER, only \a literal<code>[0]</code> is copied;
 *  + #P_PUNCTUATOR, \a literal is copied;
 *  + Any other kind, \a literal is not used.
 * @endparblock
 * @return Returns a pointer to a new \ref p_token.  The caller is responsible
 * for freeing it.
 *
 * @sa p_token_free()
 * @sa p_token_new_loc()
 */
NODISCARD P_MACRO_H_INLINE
p_token_t* p_token_new( p_token_kind_t kind, char const *literal ) {
  return p_token_new_loc( kind, /*loc=*/NULL, literal );
}

/**
 * Gets the string representation of \a token.
 *
 * @param token The \ref p_token to stringify.
 * @return Returns said representation.
 *
 * @warning For #P_CHAR_LIT, #P_OTHER, or #P_STR_LIT tokens only, the pointer
 * returned is to a static buffer, so you can't do something like call this
 * twice in the same `printf()` statement.
 *
 * @sa p_token_list_str()
 */
NODISCARD
char const* p_token_str( p_token_t const *token );

/**
 * Prints \a token_list.
 *
 * @param token_list The list of \ref p_token to print.
 * @param fout The `FILE` to print to.
 */
void print_token_list( p_token_list_t const *token_list, FILE *fout );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_p_macro_H */
/* vim:set et sw=2 ts=2: */
