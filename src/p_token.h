/*
**      cdecl -- C gibberish translator
**      src/p_token.h
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

#ifndef cdecl_p_token_H
#define cdecl_p_token_H

/**
 * @file
 * Declares types, macros, and functions for C preprocessor macros.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "strbuf.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdio.h>                      /* for FILE */

_GL_INLINE_HEADER_BEGIN
#ifndef P_TOKEN_H_INLINE
# define P_TOKEN_H_INLINE _GL_INLINE
#endif /* P_TOKEN_H_INLINE */

/// @endcond

/**
 * @defgroup p-token-group C Preprocessor Tokens
 * Types and functions for C Preprocessor tokens.
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
typedef enum p_token_kind p_token_kind_t;

/**
 * Shorthand for any "opaque" \ref p_token_kind --- all kinds _except_ either
 * #P_PLACEMARKER or #P_SPACE.
 *
 * @sa #P_ANY_TRANSPARENT
 */
#define P_ANY_OPAQUE              ( P_ANY_OPERATOR | P_CHAR_LIT | P_IDENTIFIER \
                                  | P_NUM_LIT | P_OTHER | P_PUNCTUATOR \
                                  | P_STR_LIT | P___VA_ARGS__ | P___VA_OPT__ )

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

////////// structs ////////////////////////////////////////////////////////////

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

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the name of \a kind.
 *
 * @param kind The \ref p_token_kind to get the name for.
 * @return Returns said name.
 */
NODISCARD
char const* p_kind_name( p_token_kind_t kind );

/**
 * Duplicates \a token.
 *
 * @param token The p_token to duplicate; may be NULL.
 * @return Returns the duplicated token or NULL only if \a token is NULL.
 *
 * @sa p_token_free()
 * @sa p_token_new()
 * @sa p_token_new_loc()
 */
NODISCARD
p_token_t* p_token_dup( p_token_t const *token );

/**
 * Frees all memory used by \a token _including_ \a token itself.
 *
 * @param token The \ref p_token to free.  If NULL, does nothing.
 *
 * @sa p_token_dup()
 * @sa p_token_list_cleanup()
 * @sa p_token_new()
 * @sa p_token_new_loc()
 */
void p_token_free( p_token_t *token );

/**
 * Lexes \a sbuf into a \ref p_token.
 *
 * @remarks The need to re-lex a token from a string happens only as the result
 * of the concatenation operator `##`.
 *
 * @param loc The source location whence the string in \a sbuf came.
 * @param sbuf The \ref strbuf to lex.
 * @return Returns a pointer to a new token only if exactly one token was lex'd
 * successfully; otherwise returns NULL.
 *
 * @sa p_token_free()
 */
NODISCARD
p_token_t* p_token_lex( c_loc_t const *loc, strbuf_t *sbuf );

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
 * Adjusts the \ref c_loc::first_column "first_column" and \ref
 * c_loc::last_column "last_column" of \ref p_token::loc "loc" for every token
 * in \a token_list starting at \a first_column using the lengths of the
 * stringified tokens to calculate subsequent token locations.
 *
 * @param token_list The \ref p_token_list_t of tokens' locations to adjust.
 * @param first_column The zero-based column to start at.
 * @return Returns one past the last column of the last stringified token in \a
 * token_list.
 *
 * @sa mex_relocate_expand_list()
 */
NODISCARD
size_t p_token_list_relocate( p_token_list_t *token_list, size_t first_column );

/**
 * Gets the string representation of \a token_list concatenated.
 *
 * @param token_list The list of \a p_token to stringify.
 * @return Returns said representation.
 *
 * @warning The pointer returned is to a static buffer.
 *
 * @sa p_token_str()
 */
NODISCARD
char const* p_token_list_str( p_token_list_t const *token_list );

/**
 * Pushes \a token onto \a token_list taking care to avoid pasting what would
 * become a different combined token.
 *
 * @param token_list The p_token_list to push onto.
 * @param token The \ref p_token to push back.
 */
void p_token_list_push_back( p_token_list_t *token_list, p_token_t *token );

/**
 * Trims both leading and trailing #P_SPACE tokens from \a token_list as well
 * as squashes multiple consecutive intervening #P_SPACE to a single #P_SPACE
 * within \a token_list.
 *
 * @param token_list The list of \ref p_token to trim.
 *
 * @sa trim_args()
 */
void p_token_list_trim( p_token_list_t *token_list );

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
 * Checks whether the \ref p_token to which \a token_node points is one of \a
 * kinds.
 *
 * @param token_node The \ref p_token_node_t to check.  May be NULL.
 * @param kinds The bitwise-or of kind(s) to check for.
 * @return Returns `true` only if \a token_node is not NULL and its token is
 * one of \a kinds.
 */
NODISCARD 
bool p_token_node_is_any( p_token_node_t const *token_node,
                          p_token_kind_t kinds );

/**
 * Checks whether \a token_node is not NULL and whether the \ref p_token to
 * which it points is a #P_PUNCTUATOR that is equal to \a punct.
 *
 * @param token_node The \ref p_token_node_t to check.  May be NULL.
 * @param punct The punctuation character to check.
 * @return Returns `true` only if it is.
 *
 * @sa p_token_is_punct()
 * @sa p_token_node_is_any()
 */
NODISCARD 
bool p_token_node_is_punct( p_token_node_t const *token_node, char punct );

/**
 * Gets the first node for a token whose \ref p_token::kind "kind" is _not_ one
 * of \a kinds.
 *
 * @param token_node The node to start from.
 * @param kinds The bitwise-or of kind(s) _not_ to get.
 * @return Returns said node or NULL if no such node exists.
 */
NODISCARD
p_token_node_t* p_token_node_not( p_token_node_t *token_node,
                                  p_token_kind_t kinds );

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
 *
 * @sa print_token_list_color()
 */
void print_token_list( p_token_list_t const *token_list, FILE *fout );

/**
 * Prints \a token_list in color.
 *
 * @param token_list The list of \ref p_token to print.
 * @param fout The `FILE` to print to.
 *
 * @sa print_token_list()
 */
void print_token_list_color( p_token_list_t const *token_list, FILE *fout );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether the presumed #P_IDENTIFIER token between \a prev_node and \a
 * next_node that is a presumed macro parameter is an argument for either
 * #P_CONCAT or #P_STRINGIFY.
 *
 * @remarks For function-like macros, when a parameter name is encountered in
 * the replacement list, it is substituted with the token sequence comprising
 * the corresponding macro argument.  If that token sequence is a macro, then
 * it is recursively expanded --- except if it was preceded by either #P_CONCAT
 * or #P_STRINGIFY, or followed by #P_CONCAT.
 *
 * @param prev_node The node pointing to the non-space token before the
 * parameter, if any.
 * @param next_node The node pointing to the non-space token after the
 * parameter, if any.
 * @return Returns `true` only if the macro is an argument of either #P_CONCAT
 * or #P_STRINGIFY.
 */
NODISCARD P_TOKEN_H_INLINE
bool p_is_operator_arg( p_token_node_t const *prev_node,
                        p_token_node_t const *next_node ) {
  return p_token_node_is_any( prev_node, P_ANY_OPERATOR ) ||
         p_token_node_is_any( next_node, P_CONCAT );
}

/**
 * Checks whether the #P_PUNCTUATOR \a token is _any single_ character.
 *
 * @param token The #P_PUNCTUATOR \ref p_token to check.
 * @return Returns `true` only if it is.
 *
 * @sa p_punct_token_is_char()
 * @sa p_token_is_any_char()
 */
NODISCARD P_TOKEN_H_INLINE
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
NODISCARD P_TOKEN_H_INLINE
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
NODISCARD P_TOKEN_H_INLINE
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
NODISCARD P_TOKEN_H_INLINE
bool p_token_is_punct( p_token_t const *token, char punct ) {
  return token->kind == P_PUNCTUATOR && p_punct_token_is_char( token, punct );
}

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
 * @sa p_token_dup()
 * @sa p_token_free()
 * @sa p_token_new_loc()
 */
NODISCARD P_TOKEN_H_INLINE
p_token_t* p_token_new( p_token_kind_t kind, char const *literal ) {
  return p_token_new_loc( kind, /*loc=*/NULL, literal );
}

/**
 * Convenience function that checks whether the \ref p_token_list_t starting at
 * \a token_node is "empty-ish," that is empty or contains only #P_PLACEMARKER
 * or #P_SPACE tokens.
 *
 * @param token_node The \ref p_token_node_t to start checking at.
 * @return Returns `true` only if it's "empty-ish."
 *
 * @sa p_token_list_emptyish()
 */
NODISCARD P_TOKEN_H_INLINE
bool p_token_node_emptyish( p_token_node_t const *token_node ) {
  p_token_node_t *const nonconst_node =
    CONST_CAST( p_token_node_t*, token_node );
  return p_token_node_not( nonconst_node, P_ANY_TRANSPARENT ) == NULL;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_p_token_H */
/* vim:set et sw=2 ts=2: */
