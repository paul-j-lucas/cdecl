/*
**      cdecl -- C gibberish translator
**      src/p_macro.c
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

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define P_MACRO_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "p_macro.h"
#include "c_lang.h"
#include "c_operator.h"
#include "color.h"
#include "dump.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "parser.h"
#include "print.h"
#include "prompt.h"
#include "red_black.h"
#include "show.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/// @endcond

/**
 * @addtogroup p-macro-group
 * @{
 */

/**
 * Convenience macro for specifying a \ref c_loc having \a COL as its \ref
 * c_loc::first_column "first_column".
 *
 * @param COL The first column.
 * @return Returns said \ref c_loc
 *
 * @sa C_LOC_NEXT_COL()
 */
#define C_LOC_COL(COL) \
  (c_loc_t){ .first_column = STATIC_CAST( c_loc_num_t, (COL) ) }

/**
 * Gets a \ref c_loc literal whose \ref c_loc::first_column "first_column" is
 * \a LOC's \ref c_loc::last_column "last_column" + 1.
 *
 * @param LOC The location to get the \ref c_loc::last_column "last_column" of.
 * @return Returns said \ref c_loc.
 *
 * @sa C_LOC_COL()
 */
#define C_LOC_NEXT_COL(LOC)       C_LOC_COL( (LOC).last_column + 1 )

/**
 * Ends a dump block.
 *
 * @param INDENT The indentation to use.
 *
 * @sa #DUMP_START()
 */
#define DUMP_END(INDENT) \
  FPRINTF( stdout, "\n%*s}\n", STATIC_CAST( int, 2 * (INDENT) ), "" )

/**
 * Possibly dumps a comma and a newline followed by the `printf()` arguments
 * --- used for printing a key followed by a value.
 *
 * @param INDENT The indentation to use.
 * @param ... The `printf()` arguments.
 */
#define DUMP_KEY(INDENT, ...) BLOCK(      \
  fput_sep( ",\n", &dump_comma, stdout ); \
  FPUTNSP( 2 * (INDENT), stdout );        \
  PRINTF( "  " __VA_ARGS__ ); )

/**
 * Starts a dump block.
 *
 * @param INDENT The indentation to use.
 *
 * @note The dump block _must_ end with #DUMP_END().
 *
 * @sa #DUMP_END()
 */
#define DUMP_START(INDENT)  \
  bool dump_comma = false;  \
  FPRINTF( stdout, "%*s{\n", STATIC_CAST( int, 2 * (INDENT) ), "" )

/**
 * Dumps a C string.
 *
 * @param INDENT The indentation to use.
 * @param KEY The key name to print.
 * @param STR The C string to dump.
 */
#define DUMP_STR(INDENT,KEY,STR) BLOCK( \
  DUMP_KEY( (INDENT), KEY ": " ); fputs_quoted( (STR), '"', stdout ); )

/**
 * The maximum indentation for printing macros.
 *
 * @remarks If the indentation ever gets larger than this, it's likely due to
 * an infinite recursion bug.
 */
#define INDENT_MAX                50

/**
 * A return value of p_macro_find_param() to indicate that a parameter having a
 * given name does not exist.
 */
#define NO_PARAM                  SIZE_MAX

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

////////// enumerations ///////////////////////////////////////////////////////

/**
 * Macro EXpansion function Return Value.
 *
 * @note These are in priority order from lowest to highest.  When performing
 * multiple expansion passes, the end result of the set of passes _must_ be the
 * highest value to have occurred.
 */
enum mex_rv {
  /**
   * Either:
   *
   *  1. A token can not be expanded, so treat it as ordinary text; or:
   *  2. An entire expansion pass can not be performed, so skip it; or if it
   *     already started, abort it and pretend it didn't happen.
   */
  MEX_CAN_NOT_EXPAND,

  /**
   * An expansion pass was completed, but no tokens from from \ref
   * mex_state::replace_list "replace_list" were expanded onto \ref
   * mex_state::expand_list "expand_list", i.e., the latter is an exact copy of
   * the former.
   */
  MEX_DID_NOT_EXPAND,

  /**
   * Macro was expanded, that is at least one token from \ref
   * mex_state::replace_list "replace_list" was expanded onto \ref
   * mex_state::expand_list "expand_list".
   */
  MEX_EXPANDED,

  /**
   * An error occurred; abort expansion.
   */
  MEX_ERROR
};

////////// typedefs ///////////////////////////////////////////////////////////

typedef struct macro_rb_visit_data  macro_rb_visit_data_t;
typedef enum   mex_rv               mex_rv_t;
typedef struct mex_state            mex_state_t;
typedef struct param_expand         param_expand_t;

/**
 * The signature for functions passed to mex_expand_all_fns().
 *
 * @param mex The mex_state to use.
 * @return Returns a \ref mex_rv.
 */
typedef mex_rv_t (*mex_expand_all_fn_t)( mex_state_t *mex );

////////// structs ////////////////////////////////////////////////////////////

/**
 * Data passed to our red-black tree visitor function.
 */
struct macro_rb_visit_data {
  p_macro_visit_fn_t  visit_fn;         ///< Caller's visitor function.
  void               *v_data;           ///< Caller's optional data.
};

/**
 * State maintained during Macro EXpansion.
 */
struct mex_state {
  mex_state_t const    *parent_mex;     ///< The parent mex_state, if any.

  p_macro_t const      *macro;          ///< The \ref p_macro to expand.
  c_loc_t               name_loc;       ///< Macro name source location.
  p_arg_list_t         *arg_list;       ///< Macro arguments, if any.
  p_token_list_t const *replace_list;   ///< Current replacement tokens.
  p_token_list_t       *expand_list;    ///< Current expansion tokens.

  rb_tree_t            *expanding_set;  ///< Macros undergoing expansion.

  /**
   * The set of macro names that won't expand we've warned about that is one
   * of:
   *
   *  + Dynamic and not supported in the current language; or:
   *  + A function-like macro not followed by `(`.
   */
  rb_tree_t            *no_expand_set;

  p_token_list_t        va_args_list;   ///< Expanded `__VA_ARGS__` tokens.
  char const           *va_args_str;    ///< \ref va_args_list as a string.

  /**
   * Token lists used during expansion.
   *
   * @remarks Initially, \ref replace_list points to \ref p_macro::replace_list
   * and \ref expand_list points to `work_lists[0]` so that the tokens in the
   * macro's original replacement list are expanded into `work_lists[0]`.
   * @par
   * If any tokens were expaned, \ref replace_list and \ref expand_list are
   * "swapped" such that \ref replace_list points to `work_lists[0]` and \ref
   * expand_list points to `work_lists[1]` so that the next expansion will
   * expand the result of the previous expansion.  Subsequent expansions swap
   * between `work_lists` 0 and 1.
   *
   * @sa mex_swap_lists()
   */
  p_token_list_t        work_lists[2];

  /**
   * When not NULL, this <code>%mex_state</code> is for a macro parameter and
   * this points to the parameter's #P_IDENTIFIER node in the macro's \ref
   * replace_list.
   */
  p_token_node_t       *param_node;

  FILE                 *fout;           ///< File to print to.
  unsigned              indent;         ///< Current indentation.

  /**
   * When set, tokens are _not_ trimmed via trim_tokens() after each expansion
   * function is called.
   *
   * @remarks This is set _only_ when appending "arguments" via
   * mex_append_args() to the expansion of a non-function-like macro so they
   * are appended verbatim.
   */
  bool                  expand_opt_no_trim_tokens;

  /**
   * When set, \ref mex_state::arg_list "arg_list" is _neither_ printed via
   * mex_print_macro() _nor_ used in token location calculations via
   * mex_relocate_expand_list().
   *
   * @remarks This is set _only_ when expanding #P___VA_OPT__ tokens since it
   * needs access to \ref mex_state::arg_list "arg_list", but they should _not_
   * be printed.
   */
  bool                  print_opt_omit_args;

  /**
   * Flag to keep track of whether this warning has already been printed so as
   * not to print it more than once per expansion pass.
   * @{
   */
  bool                  warned_concat_not_supported;
  bool                  warned_stringify_in_non_func_like_macro;
  bool                  warned_stringify_not_supported;
  bool                  warned___VA_ARGS___not_supported;
  bool                  warned___VA_OPT___not_supported;
  /** @} */
};

/**
 * Macro parameter expansion cache entry used by mex_expand_all_params().
 */
struct param_expand {
  char const     *name;                 ///< Parameter name.
  p_token_list_t *expand_list;          ///< Tokens it expands into.
};

////////// local functions ////////////////////////////////////////////////////

NODISCARD
static bool             ident_will_not_expand( p_token_t const*,
                                               p_token_node_t const*,
                                               p_token_node_t const* ),
                        is_multi_char_punctuator( char const* ),
                        is_operator_arg( p_token_node_t const*,
                                         p_token_node_t const* ),
                        mex_check( mex_state_t *mex ),
                        mex_check_concat( mex_state_t*,
                                          p_token_node_t const* ),
                        mex_check_identifier( mex_state_t*,
                                              p_token_node_t const* ),
                        mex_check_stringify( mex_state_t*,
                                             p_token_node_t const* ),
                        mex_check___VA_ARGS__( mex_state_t*,
                                               p_token_node_t const* ),
                        mex_check___VA_OPT__( mex_state_t const*,
                                              p_token_node_t const* ),
                        mex_expand_all_fns( mex_state_t*,
                                            mex_expand_all_fn_t
                                              const[static 1] );

static void             mex_cleanup( mex_state_t* );

NODISCARD
static mex_rv_t         mex_expand_all_concat( mex_state_t* ),
                        mex_expand_all_fns_impl( mex_state_t*,
                                                 mex_expand_all_fn_t
                                                   const[static 1], mex_rv_t* ),
                        mex_expand_all_macros( mex_state_t* ),
                        mex_expand_all_params( mex_state_t* ),
                        mex_expand_all_stringify( mex_state_t* ),
                        mex_expand_all___VA_ARGS__( mex_state_t* ),
                        mex_expand_all___VA_OPT__( mex_state_t* ),
                        mex_expand_identifier( mex_state_t*, p_token_node_t** ),
                        mex_expand_stringify( mex_state_t*, p_token_node_t** );

NODISCARD
static p_token_node_t*  mex_expand___VA_OPT__( mex_state_t*, p_token_node_t*,
                                               p_token_list_t* );

NODISCARD
static char const*      mex_expanding_set_key( mex_state_t const* );

static void             mex_init( mex_state_t*, mex_state_t*, p_macro_t const*,
                                  p_arg_list_t*, p_token_list_t const*, FILE* );

NODISCARD
static p_token_list_t*  mex_param_arg( mex_state_t const*, char const* );

NODISCARD
static bool             mex_pre_expand___VA_ARGS__( mex_state_t* ),
                        mex_prep_args( mex_state_t* );

static void             mex_pre_filter___VA_OPT__( mex_state_t* );
static void             mex_preliminary_relocate_replace_list( mex_state_t* );
static void             mex_print_macro( mex_state_t const*,
                                         p_token_list_t const* );
static void             mex_relocate_expand_list( mex_state_t* );
static void             mex_stringify_identifier( mex_state_t*,
                                                  p_token_t const* );
static void             mex_stringify___VA_ARGS__( mex_state_t* );

NODISCARD
static p_token_node_t*  mex_stringify___VA_OPT__( mex_state_t*,
                                                  p_token_node_t* );

static void             mex_swap_lists( mex_state_t* );

NODISCARD
static size_t           p_arg_list_count( p_arg_list_t const* ),
                        p_macro_find_param( p_macro_t const*, char const* );

static void             p_macro_free( p_macro_t* );

NODISCARD
static bool             p_macro_is_variadic( p_macro_t const* ),
                        p_macro_check_params( p_macro_t const* );

NODISCARD
static p_token_t*       p_token_dup( p_token_t const* );

NODISCARD
static size_t           p_token_list_relocate( p_token_list_t*, size_t );

NODISCARD
static char const*      p_token_list_str( p_token_list_t const* );

NODISCARD
static p_token_node_t*  p_token_node_not( p_token_node_t*, p_token_kind_t );

NODISCARD
static int              param_expand_cmp( param_expand_t const*,
                                          param_expand_t const* );

NODISCARD
static p_token_node_t*  parse_args( p_token_node_t const*, p_arg_list_t* );

static void             push_back_dup_token( p_token_list_t*,
                                             p_token_t const* );

PJL_DISCARD
static p_token_node_t*  push_back_dup_tokens( p_token_list_t*,
                                              p_token_list_t const* );

static void             push_back_substituted_token( p_token_list_t*,
                                                     p_token_t* );

static void             set_substituted( p_token_node_t* );
static void             trim_args( p_arg_list_t* );
static void             trim_tokens( p_token_list_t* );

// local constants
static char const ARROW[] = "=>";       ///< Separates macro name from tokens.

// local variables
static rb_tree_t  macro_set;            ///< Global set of macros.

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether \a macro is a function-like macro.
 *
 * @param macro The \ref p_macro to check.
 * @return Returns `true` only if it is.
 */
NODISCARD
static inline bool p_macro_is_func_like( p_macro_t const *macro ) {
  return !macro->is_dynamic && macro->param_list != NULL;
}

/**
 * Checks whether \a token is an eligible #P_IDENTIFIER and a macro exists
 * having the identifier's name.
 *
 * @param token The \ref p_token to use.
 * @return Returns `true` only if it is.
 *
 * @sa is_predefined_macro_name()
 */
NODISCARD
static inline bool p_token_is_macro( p_token_t const *token ) {
  return  token->kind == P_IDENTIFIER && !token->ident.ineligible &&
          p_macro_find( token->ident.name ) != NULL;
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
NODISCARD
static inline bool p_token_node_emptyish( p_token_node_t const *token_node ) {
  p_token_node_t *const nonconst_node =
    CONST_CAST( p_token_node_t*, token_node );
  return p_token_node_not( nonconst_node, P_ANY_TRANSPARENT ) == NULL;
}

/**
 * Convenience function that checks whether \a token_list is "empty-ish," that
 * is empty or contains only #P_PLACEMARKER or #P_SPACE tokens.
 *
 * @param token_list The \ref p_token_list_t to check.
 * @return Returns `true` only if it's "empty-ish."
 *
 * @sa p_token_node_emptyish()
 */
NODISCARD
static inline bool p_token_list_emptyish( p_token_list_t const *token_list ) {
  return p_token_node_emptyish( token_list->head );
}

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
static inline bool p_token_node_is_any( p_token_node_t const *token_node,
                                        p_token_kind_t kinds ) {
  if ( token_node == NULL )
    return false;
  p_token_t const *const token = token_node->data;
  return (token->kind & kinds) != 0;
}

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
static inline bool p_token_node_is_punct( p_token_node_t const *token_node,
                                          char punct ) {
  return token_node != NULL && p_token_is_punct( token_node->data, punct );
}

/**
 * Creates a \ref c_loc where \ref c_loc::first_column "first_column" is 0 and
 * \ref c_loc::last_column "last_column" is the length of \a s.
 *
 * @param s The string to use the length of.
 * @returns Returns said location.
 */
NODISCARD
static c_loc_t str_loc( char const *s ) {
  return (c_loc_t){
    .first_column = 0,
    .last_column = s[0] == '\0' ?
      0 : (STATIC_CAST( c_loc_num_t, strlen( s ) ) - 1)
  };
}

////////// local functions ////////////////////////////////////////////////////

/**
 * If the last non-#P_PLACEMARKER token of \a token_list, if any, and \a token
 * are both #P_PUNCTUATOR tokens and pasting (concatenating) them together
 * would form a different valid #P_PUNCTUATOR token, appends a #P_SPACE token
 * onto \a token_list to avoid this.
 *
 * @param token_list The \ref p_token_list_t whose last token, if any, to avoid
 * pasting with.
 * @param token The \ref p_token to avoid pasting.  It is _not_ appended to \a
 * token_list.
 *
 * @sa is_multi_char_punctuator()
 * @sa [Token Spacing](https://gcc.gnu.org/onlinedocs/gcc-4.9.3/cppinternals/Token-Spacing.html#Token-Spacing).
 */
static void avoid_paste( p_token_list_t *token_list, p_token_t const *token ) {
  assert( token != NULL );
  assert( token_list != NULL );

  if ( token->kind != P_PUNCTUATOR )
    return;

  //
  // Get the last token of token_list that is not a P_PLACEMARKER, if any.  If
  // said token is not a P_PUNCTUATOR, return.
  //
  p_token_t const *last_token;
  for ( size_t roffset = 0; true; ++roffset ) {
    last_token = slist_atr( token_list, roffset );
    if ( last_token == NULL )
      return;
    if ( last_token->kind == P_PUNCTUATOR )
      break;
    if ( last_token->kind != P_PLACEMARKER )
      return;
  } // for

  char const *const s1 = p_token_str( last_token );
  char const *const s2 = p_token_str( token );

  //
  // It's large enough to hold two of the longest operators of `->*`, `<<=`,
  // `<=>`, or `>>=`, consecutively, plus a terminating `\0`.
  //
  char paste_buf[7];                    // 1112220

  check_snprintf( paste_buf, sizeof paste_buf, "%s%s", s1, s2 );
  if ( is_multi_char_punctuator( paste_buf ) )
    goto append;

  if ( s2[1] != '\0' ) {
    //
    // We also have to check for cases where a partial paste of the token would
    // form a different valid punctuator, e.g.:
    //
    //      cdecl> #define P(X)  -X
    //      cdecl> expand P(->)
    //      P(->) => -X
    //      | X => ->
    //      P(->) => - ->                 // not: -->
    //
    // That would later be parsed as -- > which is wrong.
    //
    check_snprintf( paste_buf, sizeof paste_buf, "%s%c", s1, s2[0] );
    if ( is_multi_char_punctuator( paste_buf ) )
      goto append;
  }

  return;

append:
  slist_push_back( token_list, p_token_new( P_SPACE, /*literal=*/NULL ) );
}

/**
 * Checks macro parameters, if any, for semantic errors.
 *
 * @param param_list The list of \ref p_param to check.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool check_macro_params( p_param_list_t const *param_list ) {
  assert( param_list != NULL );

  FOREACH_SLIST_NODE( param_node, param_list ) {
    p_param_t const *const param = param_node->data;
    if ( strcmp( param->name, L_ellipsis ) == 0 ) {
      if ( !OPT_LANG_IS( VARIADIC_MACROS ) ) {
        print_error( &param->loc,
          "variadic macros not supported%s\n",
          C_LANG_WHICH( VARIADIC_MACROS )
        );
        return false;
      }
      if ( param_node->next != NULL ) {
        print_error( &param->loc, "\"...\" must be last parameter\n" );
        return false;
      }
    }

    FOREACH_SLIST_NODE_UNTIL( prev_param_node, param_list, param_node ) {
      p_param_t const *const prev_param = prev_param_node->data;
      if ( strcmp( param->name, prev_param->name ) == 0 ) {
        print_error( &param->loc,
          "\"%s\": duplicate macro parameter\n",
          param->name
        );
        return false;
      }
    } // for
  } // for

  return true;
}

/**
 * Prints \a token_list in color.
 *
 * @param token_list The list of \ref p_token to print.
 * @param fout The `FILE` to print to.
 *
 * @sa print_token_list()
 */
static void color_print_token_list( p_token_list_t const *token_list,
                                    FILE *fout ) {
  assert( token_list != NULL );
  assert( fout != NULL );

  bool printed_opaque = false;

  p_token_node_t const *prev_node = NULL;
  FOREACH_SLIST_NODE( token_node, token_list ) {
    char const *color = NULL;
    p_token_t const *const token = token_node->data;

    p_token_node_t const *const next_node =
      p_token_node_not( token_node->next, P_ANY_TRANSPARENT );

    switch ( token->kind ) {
      case P_IDENTIFIER:
        if ( ident_will_not_expand( token, prev_node, next_node ) ) {
          color = sgr_macro_no_expand;
          break;
        }
        FALLTHROUGH;
      default:
        if ( token->is_substituted )
          color = sgr_macro_subst;
        break;
      case P_PLACEMARKER:
        continue;
      case P_SPACE:
        if ( !printed_opaque )
          continue;                     // don't print leading spaces
        if ( next_node == NULL )
          return;                       // don't print trailing spaces either
        break;
    } // switch

    color_start( fout, color );
    FPUTS( p_token_str( token ), fout );
    color_end( fout, color );
    printed_opaque = true;

    if ( token->kind != P_SPACE )
      prev_node = token_node;
  } // for
}

/**
 * Gets the current value of the `__DATE__` macro.
 *
 * @return Returns said value.
 *
 * @warning The pointer returned is to a static buffer.
 *
 * @sa #get___TIME___str()
 */
static char const* get___DATE___str( void ) {
  static char buf[ sizeof( "MMM DD YYYY" ) ];
  time_t const now = time( /*tloc=*/NULL );
  check_strftime( buf, sizeof buf, "%b %e %Y", localtime( &now ) );
  return buf;
}

/**
 * Gets the current value of the `__TIME__` macro.
 *
 * @return Returns said value.
 *
 * @warning The pointer returned is to a static buffer.
 *
 * @sa #get___DATE___str()
 */
static char const* get___TIME___str( void ) {
  static char buf[ sizeof( "hh:mm:ss" ) ];
  time_t const now = time( /*tloc=*/NULL );
  check_strftime( buf, sizeof buf, "%H:%M:%S", localtime( &now ) );
  return buf;
}

/**
 * Checks whether \a identifier_token will not expand.
 *
 * @remarks
 * @parblock
 * An identifier token will not expand if it's a macro and it's one of:
 *
 *  + Ineligible; or:
 *  + An argument of either #P_CONCAT or #P_STRINGIFY; or:
 *  + Dynamic and not supported in the current language; or:
 *  + A function-like macro not followed by `(`.
 * @endparblock
 *
 * @param identifier_token The #P_IDENTIFIER \ref p_token to check.
 * @param prev_node The non-space \ref p_token_node_t just before \a
 * identifier_token.
 * @param next_node The non-space \ref p_token_node_t just after \a
 * identifier_token.
 * @return Returns `true` only if \a identifier_token will not expand.
 *
 * @note This is a helper function for color_print_token_list() to know whether
 * to print a #P_IDENTIFIER token in the \ref sgr_macro_no_expand color.
 */
NODISCARD
static bool ident_will_not_expand( p_token_t const *identifier_token,
                                   p_token_node_t const *prev_node,
                                   p_token_node_t const *next_node ) {
  assert( identifier_token != NULL );
  assert( identifier_token->kind == P_IDENTIFIER );

  if ( identifier_token->ident.ineligible )
    return true;

  p_macro_t const *const macro = p_macro_find( identifier_token->ident.name );
  if ( macro == NULL )
    return false;

  if ( is_operator_arg( prev_node, next_node ) )
    return true;
  if ( macro->is_dynamic &&
       !opt_lang_is_any( (*macro->dyn_fn)( /*ptoken=*/NULL ) ) ) {
    return true;
  }
  if ( !p_macro_is_func_like( macro ) )
    return false;
  if ( !p_token_node_is_punct( next_node, '(' ) )
    return true;

  return false;
}

/**
 * Checks whether \a s is a multi-character punctuator.
 *
 * @param s The literal to check.
 * @return Returns `true` only if \a s is a multi-character punctuator.
 *
 * @sa avoid_paste()
 */
NODISCARD
static bool is_multi_char_punctuator( char const *s ) {
  static c_lang_lit_t const MULTI_CHAR_PUNCTUATORS[] = {
    { LANG_ANY,                 "!="  },
    { LANG_ANY,                 "%="  },
    { LANG_ANY,                 "&&"  },
    { LANG_ANY,                 "&="  },
    { LANG_ANY,                 "*="  },
    { LANG_ANY,                 "++"  },
    { LANG_ANY,                 "+="  },
    { LANG_ANY,                 "--"  },
    { LANG_ANY,                 "-="  },
    { LANG_ANY,                 "->"  },
    { LANG_CPP_ANY,             "->*" },
    { LANG_CPP_ANY,             ".*"  },
    { LANG_ANY,                 "/*"  },
    { LANG_SLASH_SLASH_COMMENT, "//"  },
    { LANG_ANY,                 "/="  },
    { LANG_CPP_ANY,             "::"  },
    { LANG_ANY,                 "<<"  },
    { LANG_ANY,                 "<<=" },
    { LANG_ANY,                 "<="  },
    { LANG_LESS_EQUAL_GREATER,  "<=>" },
    { LANG_ANY,                 "=="  },
    { LANG_ANY,                 ">="  },
    { LANG_ANY,                 ">>=" },
    { LANG_ANY,                 "^="  },
    { LANG_ANY,                 "|="  },
    { LANG_ANY,                 "||"  },
  };

  FOREACH_ARRAY_ELEMENT( c_lang_lit_t, punct, MULTI_CHAR_PUNCTUATORS ) {
    if ( !opt_lang_is_any( punct->lang_ids ) )
      continue;
    if ( strcmp( s, punct->literal ) == 0 )
      return true;
  } // for

  return false;
}

/**
 * Checks whether a macro is an argument for either #P_CONCAT or #P_STRINGIFY.
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
NODISCARD
static bool is_operator_arg( p_token_node_t const *prev_node,
                             p_token_node_t const *next_node ) {
  return p_token_node_is_any( prev_node, P_ANY_OPERATOR ) ||
         p_token_node_is_any( next_node, P_CONCAT );
}

/**
 * Checks whether \a name is a predefined macro or `__VA_ARGS__` or
 * `__VA_OPT__`.
 *
 * @param name The name to check.
 * @return Returns `true` only if it is.
 *
 * @sa p_token_is_macro()
 */
NODISCARD
static bool is_predefined_macro_name( char const *name ) {
  assert( name != NULL );
  if ( strcmp( name, L___VA_ARGS__ ) == 0 ||
       strcmp( name, L___VA_OPT__  ) == 0 ) {
    return true;
  }
  p_macro_t const *const macro = p_macro_find( name );
  return macro != NULL && macro->is_dynamic;
}

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
 */
NODISCARD
static p_token_t* lex_token( c_loc_t const *loc, strbuf_t *sbuf ) {
  assert( loc != NULL );
  assert( sbuf != NULL );

  if ( sbuf->len == 0 )
    return p_token_new_loc( P_PLACEMARKER, loc, /*literal=*/NULL );

  // Preprocessor lines must end in a newline.
  strbuf_putc( sbuf, '\n' );
  lexer_push_string( sbuf->str, sbuf->len );

  p_token_t *token = NULL;
  int y_token_id = yylex();

  switch ( y_token_id ) {
    case '!':
    case '#':                           // ordinary '#', not P_STRINGIFY
    case '%':
    case '&':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case ':':
    case ';':
    case '<':
    case '=':
    case '>':
    case '?':
    case '[':
    case ']':
    case '^':
    case '{':
    case '|':
    case '}':
    case '~':
    case Y_AMPER2:
    case Y_AMPER_EQUAL:
    case Y_CARET_EQUAL:
    case Y_COLON2_STAR:
    case Y_DOT3:
    case Y_EQUAL2:
    case Y_EXCLAM_EQUAL:
    case Y_GREATER2:
    case Y_GREATER2_EQUAL:
    case Y_GREATER_EQUAL:
    case Y_LESS2:
    case Y_LESS2_EQUAL:
    case Y_LESS_EQUAL:
    case Y_MINUS2:
    case Y_MINUS_EQUAL:
    case Y_MINUS_GREATER:
    case Y_PERCENT_EQUAL:
    case Y_PIPE2:
    case Y_PIPE_EQUAL:
    case Y_PLUS2:
    case Y_PLUS_EQUAL:
    case Y_SLASH_EQUAL:
    case Y_STAR_EQUAL:
      token = p_token_new_loc( P_PUNCTUATOR, &yylloc, lexer_token );
      break;

    case Y_COLON2:
    case Y_DOT_STAR:
    case Y_MINUS_GREATER_STAR:
      //
      // Special case: the lexer isn't language-sensitive (which would be hard
      // to do) so these tokens are always recognized.  But if the current
      // language isn't C++, consider them as two tokens (which is a
      // concatenation error).
      //
      if ( !OPT_LANG_IS( CPP_ANY ) )
        goto done;
      token = p_token_new_loc( P_PUNCTUATOR, &yylloc, lexer_token );
      break;

    case Y_LESS_EQUAL_GREATER:
      //
      // Special case: same as above tokens.
      //
      if ( !OPT_LANG_IS( LESS_EQUAL_GREATER ) )
        goto done;
      token = p_token_new_loc( P_PUNCTUATOR, &yylloc, lexer_token );
      break;

    case Y_CHAR_LIT:
      token = p_token_new_loc( P_CHAR_LIT, &yylloc, yylval.str_val );
      break;

    case Y_FLOAT_LIT:
    case Y_INT_LIT:
      token =
        p_token_new_loc( P_NUM_LIT, &yylloc, check_strdup( lexer_token ) );
      break;

    case Y_NAME:
      token = p_token_new_loc( P_IDENTIFIER, &yylloc, yylval.name );
      break;

    case Y_STR_LIT:
      token = p_token_new_loc( P_STR_LIT, &yylloc, yylval.str_val );
      break;

    case Y_P_CONCAT:
      //
      // Given:
      //
      //      #define hash_hash # ## #
      //
      // when expanding hash_hash, the concat operator produces a new token
      // consisting of two adjacent sharp signs, but this new token is NOT the
      // concat operator.
      //
      token = p_token_new_loc( P_PUNCTUATOR, &yylloc, "##" );
      break;

    case Y_P_SPACE:                     // can't result from concatenation
      UNEXPECTED_INT_VALUE( y_token_id );

    case Y_P___VA_ARGS__:
      //
      // Given:
      //
      //      cdecl> #define M(...)   __VA ## _ARGS__
      //      cdecl> expand M(x)
      //      M(x) => __VA_ARGS__
      //
      // when expanding M, the concat operator produces a new __VA_ARGS__
      // token, but this new token is NOT the normal __VA_ARGS__.
      //
      token = p_token_new_loc(
        P_IDENTIFIER, &yylloc, check_strdup( L___VA_ARGS__ )
      );
      token->ident.ineligible = true;
      break;

    case Y_P___VA_OPT__:
      //
      // Given:
      //
      //      cdecl> #define M(...)   __VA_ARGS__ __VA ## _OPT__(y)
      //      cdecl> expand M(x)
      //      M(x) => x __VA_OPT__(y)
      //
      // when expanding M, the concat operator produces a new __VA_OPT__ token,
      // but this new token is NOT the normal __VA_OPT__.
      //
      token = p_token_new_loc(
        P_IDENTIFIER, &yylloc, check_strdup( L___VA_OPT__ )
      );
      token->ident.ineligible = true;
      break;

    case '$':
    case '@':
    case '`':
      token = p_token_new_loc( P_OTHER, &yylloc, lexer_token );
      break;

    case Y_LEXER_ERROR:
      goto done;

    default:
      UNEXPECTED_INT_VALUE( y_token_id );
  } // switch

  //
  // We've successfully lex'd a token: now try to lex another one to see if
  // there is another one.
  //
  y_token_id = yylex();

done:
  lexer_pop_string();
  sbuf->str[ --sbuf->len ] = '\0';      // remove newline

  switch ( y_token_id ) {
    case Y_END:                         // exactly one token: success
      return token;
    case Y_LEXER_ERROR:
      break;
    default:                            // more than one token: failure
      print_error( loc,
        "\"%s\": concatenation formed invalid token\n", sbuf->str
      );
  } // switch

  p_token_free( token );
  return NULL;
}

/**
 * Generates a key for function-like macros that won't expand for the \ref
 * mex_state::no_expand_set "no_expand_set".
 *
 * @param curr_macro The current \ref p_macro.
 * @param warn_macro The function-like \ref p_macro to potentially warn about.
 * @return Returns said key.
 *
 * @warning The pointer returned is to a static buffer.
 */
NODISCARD
static char const* macro_flmwa_key( p_macro_t const *curr_macro,
                                    p_macro_t const *warn_macro ) {
  assert( curr_macro != NULL );
  assert( warn_macro != NULL );

  static strbuf_t sbuf;
  strbuf_reset( &sbuf );
  strbuf_printf( &sbuf, "%s-%s", curr_macro->name, warn_macro->name );
  return sbuf.str;
}

/**
 * Appends supplied "arguments" to a non-function-like macro.
 *
 * @param mex The mex_state to use.
 * @return Returns `true` only if successful.
 */
NODISCARD
static bool mex_append_args( mex_state_t *mex ) {
  assert( mex != NULL );
  assert( !p_macro_is_func_like( mex->macro ) );
  assert( mex->arg_list != NULL );

  slist_push_back( mex->expand_list, p_token_new( P_PUNCTUATOR, "(" ) );

  static mex_expand_all_fn_t const EXPAND_FNS[] = {
    // We need only mex_expand_all_macros() that mex_expand_all_fns() does
    // implicitly.
    NULL
  };

  unsigned arg_index = 0;
  bool comma = false;
  FOREACH_SLIST_NODE( arg_node, mex->arg_list ) {
    char arg_name[8];                   // arg_NNN0
    check_snprintf( arg_name, sizeof arg_name, "arg_%u", ++arg_index );

    mex_state_t arg_mex;
    mex_init( &arg_mex,
      /*parent_mex=*/mex,
      &(p_macro_t){ .name = arg_name },
      /*arg_list=*/NULL,
      /*replace_list=*/arg_node->data,
      mex->fout
    );
    arg_mex.expand_opt_no_trim_tokens = true;

    mex_print_macro( &arg_mex, arg_mex.replace_list );
    bool const ok = mex_expand_all_fns( &arg_mex, EXPAND_FNS );
    if ( ok ) {
      if ( true_or_set( &comma ) )
        slist_push_back( mex->expand_list, p_token_new( P_PUNCTUATOR, "," ) );
      push_back_dup_tokens( mex->expand_list, arg_mex.expand_list );
    }
    mex_cleanup( &arg_mex );
    if ( !ok )
      return false;
  } // for

  slist_push_back( mex->expand_list, p_token_new( P_PUNCTUATOR, ")" ) );
  return true;
}

/**
 * Checks whether the `__cplusplus` macro has a value in the current language
 * and possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG_CPP_ANY.
 *
 * @sa macro_dyn___STDC_VERSION__()
 */
static c_lang_id_t macro_dyn___cplusplus( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    char const *const value = c_lang___cplusplus();
    *ptoken = value == NULL ? NULL :
      p_token_new( P_NUM_LIT, check_strdup( value ) );
  }
  return LANG_CPP_ANY;
}

/**
 * Checks whether the `__DATE__` macro has a value in the current language and
 * possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___DATE__.
 *
 * @sa get___DATE___str()
 * @sa macro_dyn___TIME__()
 */
static c_lang_id_t macro_dyn___DATE__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    *ptoken = !OPT_LANG_IS( __DATE__ ) ? NULL :
      p_token_new( P_STR_LIT, check_strdup( get___DATE___str() ) );
  }
  return LANG___DATE__;
}

/**
 * Checks whether the `__FILE__` macro has a value in the current language and
 * possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___FILE__.
 *
 * @sa macro_dyn___LINE__()
 */
static c_lang_id_t macro_dyn___FILE__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    static char const *const VALUE[] = { "example.c", "example.cpp" };
    *ptoken = !OPT_LANG_IS( __FILE__ ) ? NULL :
      p_token_new( P_STR_LIT, check_strdup( VALUE[ OPT_LANG_IS( CPP_ANY ) ] ) );
  }
  return LANG___FILE__;
}

/**
 * Checks whether the `__LINE__` macro has a value in the current language and
 * possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___LINE__.
 *
 * @sa macro_dyn___FILE__()
 */
static c_lang_id_t macro_dyn___LINE__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    static char const VALUE[] = "42";
    *ptoken = !OPT_LANG_IS( __LINE__ ) ? NULL :
      p_token_new( P_NUM_LIT, check_strdup( VALUE ) );
  }
  return LANG___LINE__;
}

/**
 * Checks whether the `__STDC__` macro has a value in the current language and
 * possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___STDC__.
 *
 * @sa macro_dyn___STDC_VERSION__()
 */
static c_lang_id_t macro_dyn___STDC__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    char const *const value = c_lang___STDC__();
    *ptoken = value == NULL ? NULL :
      p_token_new( P_NUM_LIT, check_strdup( value ) );
  }
  return LANG___STDC__;
}

/**
 * Checks whether the `__STDC_VERSION__` macro has a value in the current
 * language and possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___STDC_VERSION__.
 *
 * @sa macro_dyn___cplusplus()
 * @sa macro_dyn___STDC__
 */
static c_lang_id_t macro_dyn___STDC_VERSION__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    char const *const value = c_lang___STDC_VERSION__();
    *ptoken = value == NULL ? NULL :
      p_token_new( P_NUM_LIT, check_strdup( value ) );
  }
  return LANG___STDC_VERSION__;
}

/**
 * Checks whether the `__TIME__` macro has a value in the current language and
 * possibly creates a \ref p_token having said value.
 *
 * @param ptoken A pointer to receive a pointer to a new \ref p_token, if any.
 * The caller is responsible for freeing it.
 * @return Always returns #LANG___TIME__.
 *
 * @sa get___TIME___str()
 * @sa macro_dyn___DATE__()
 */
static c_lang_id_t macro_dyn___TIME__( p_token_t **ptoken ) {
  if ( ptoken != NULL ) {
    *ptoken = !OPT_LANG_IS( __TIME__ ) ? NULL :
      p_token_new( P_STR_LIT, check_strdup( get___TIME___str() ) );
  }
  return LANG___TIME__;
}

/**
 * Cleans up C preprocessor macro data.
 *
 * @sa p_macro_init()
 */
static void macros_cleanup( void ) {
  rb_tree_cleanup( &macro_set, POINTER_CAST( rb_free_fn_t, &p_macro_free ) );
}

/**
 * Checks \a macro for syntactic & semantic errors.
 *
 * @param mex The mex_state to use.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool mex_check( mex_state_t *mex ) {
  assert( mex != NULL );

  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t const *const token = token_node->data;
    switch ( token->kind ) {
      case P_CONCAT:
        if ( !mex_check_concat( mex, token_node ) )
          return false;
        break;
      case P_IDENTIFIER:
        if ( !mex_check_identifier( mex, token_node ) )
          return false;
        break;
      case P_STRINGIFY:
        if ( !mex_check_stringify( mex, token_node ) )
          return false;
        break;
      case P___VA_ARGS__:
        if ( !mex_check___VA_ARGS__( mex, token_node ) )
          return false;
        break;
      case P___VA_OPT__:
        if ( !mex_check___VA_OPT__( mex, token_node ) )
          return false;
        break;
      case P_CHAR_LIT:
      case P_NUM_LIT:
      case P_OTHER:
      case P_PLACEMARKER:
      case P_PUNCTUATOR:
      case P_SPACE:
      case P_STR_LIT:
        // nothing to check
        break;
    } // switch
  } // for

  return true;
}

/**
 * Checks a #P_CONCAT macro for syntactic & semantic errors.
 *
 * @param mex The mex_state to use.
 * @param token_node The node pointing to the #P_CONCAT token.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool mex_check_concat( mex_state_t *mex,
                              p_token_node_t const *token_node ) {
  assert( mex != NULL );
  assert( token_node != NULL );
  p_token_t const *const concat_token = token_node->data;
  assert( concat_token->kind == P_CONCAT );

  if ( !OPT_LANG_IS( P_CONCAT ) &&
       false_set( &mex->warned_concat_not_supported ) ) {
    print_warning( &concat_token->loc,
      "\"##\" not supported%s; treated as text\n",
      C_LANG_WHICH( P_CONCAT )
    );
  }

  if ( token_node == p_token_node_not( mex->replace_list->head, P_SPACE ) ) {
    print_error( &concat_token->loc, "\"##\" can not be first\n" );
    return false;
  }

  token_node = p_token_node_not( token_node->next, P_SPACE );
  if ( token_node == NULL ) {
    print_error( &concat_token->loc, "\"##\" can not be last\n" );
    return false;
  }

  return true;
}

/**
 * Checks a #P_IDENTIFIER macro for syntactic & semantic errors.
 *
 * @param mex The mex_state to use.
 * @param token_node The node pointing to the #P_IDENTIFIER token.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool mex_check_identifier( mex_state_t *mex,
                                  p_token_node_t const *token_node ) {
  assert( mex != NULL );
  assert( token_node != NULL );
  p_token_t *const identifier_token = token_node->data;
  assert( identifier_token->kind == P_IDENTIFIER );

  if ( identifier_token->ident.ineligible )
    return true;

  p_macro_t const *const found_macro =
    p_macro_find( identifier_token->ident.name );
  if ( found_macro == NULL )            // identifier is not a macro
    return true;

  if ( found_macro->is_dynamic ) {
    c_lang_id_t const lang_ids = (*found_macro->dyn_fn)( /*ptoken=*/NULL );
    if ( !opt_lang_is_any( lang_ids ) ) {
      //
      // We don't simply mark the token ineligible because, if we're currently
      // doing a macro definition, then the warning would be suppressed during
      // subsequent expansion, e.g., the second warning below:
      //
      //      cdecl> set knrc
      //      #define OLD_DATE    __DATE__
      //                          ^
      //      21: warning: "__DATE__" not supported until C89; will not expand
      //      cdecl> expand OLD_DATE
      //      OLD_DATE => __DATE__
      //                  ^
      //      13: warning: "__DATE__" not supported until C89; will not expand
      //
      rb_insert_rv_t rbi = rb_tree_insert(
        mex->no_expand_set, CONST_CAST( char*, found_macro->name )
      );
      if ( rbi.inserted ) {
        rbi.node->data = check_strdup( rbi.node->data );
        print_warning( &identifier_token->loc,
          "\"%s\" not supported%s; will not expand\n",
          identifier_token->ident.name,
          c_lang_which( lang_ids )
        );
      }
    }
    return true;
  }

  if ( !p_macro_is_func_like( found_macro ) )
    return true;

  token_node = p_token_node_not( token_node->next, P_ANY_TRANSPARENT );
  if ( token_node == NULL && mex->param_node != NULL ) {
    //
    // There is no non-space token following the parameter macro in its
    // expansion: check for a non-space token following the parameter in its
    // parent's expansion.  For example, given:
    //
    //      #define F(X)    #X
    //      #define G(A,B)  A(B)
    //      expand G(F,a)
    //      G(F, a) => A(B)
    //      | A => F
    //
    // When the argument 'A' expands into 'F', there are no more tokens
    // following 'F' in 'A', but '(' follows 'A' in the replacement list of 'G'
    // which means this it _not_ a case where 'F' is a macro without arguments
    // so we shouldn't warn about it.
    //
    token_node = p_token_node_not( mex->param_node->next, P_ANY_TRANSPARENT );
  }

  p_token_t const *next_token = NULL;
  if ( token_node != NULL ) {
    next_token = token_node->data;
    switch ( next_token->kind ) {
      case P_CONCAT:
        //
        // "##" doesn't expand macro arguments so the fact that the macro isn't
        // followed by '(' is irrelevant.
        //
        return true;

      case P_IDENTIFIER:
        if ( p_macro_find( next_token->ident.name ) != NULL ) {
          //
          // The macro could expand into tokens starting with '('.
          //
          return true;
        }

        if ( p_macro_find_param( mex->macro,
                                 next_token->ident.name ) != NO_PARAM ) {
          //
          // The parent's macro parameter could expand into tokens starting
          // with '('.
          //
          return true;
        }
        break;

      case P_CHAR_LIT:
      case P_NUM_LIT:
      case P_OTHER:
      case P_STRINGIFY:
      case P_STR_LIT:
        break;

      case P_PUNCTUATOR:
        if ( p_punct_token_is_char( next_token, '(' ) )
          return true;
        break;

      case P_PLACEMARKER:
      case P_SPACE:
        unreachable();

      case P___VA_ARGS__:
      case P___VA_OPT__:
        //
        // Either __VA_ARGS__ or __VA_OPT__ could expand into tokens starting
        // with '('.
        //
        return true;
    } // switch
  }

  char const *const macro_key = macro_flmwa_key( mex->macro, found_macro );
  rb_insert_rv_t rbi = rb_tree_insert(
    mex->no_expand_set, CONST_CAST( char*, macro_key )
  );

  if ( next_token != NULL && next_token->is_substituted ) {
    //
    // The next token has already been substituted so this macro without
    // arguments can _never_ expand, so mark it ineligible so we won't warn
    // about it more than once.
    //
    identifier_token->ident.ineligible = true;
  }

  if ( !rbi.inserted )
    return true;

  //
  // Now that we know the macro has been inserted, replace the static key used
  // to test for insertion with a dynamically allocated copy.  Doing it this
  // way means we do the look-up only once and the strdup() only if inserted.
  //
  rbi.node->data = check_strdup( rbi.node->data );

  print_warning( &identifier_token->loc,
    "\"%s\": function-like macro without arguments will not expand\n",
    identifier_token->ident.name
  );

  return true;
}

/**
 * Checks whether \a macro can accept the given number of arguments.
 *
 * @param mex The \ref mex_state to use.
 * @return Returns `true` only if it can.
 */
NODISCARD
static bool mex_check_num_args( mex_state_t const *mex ) {
  assert( mex != NULL );
  assert( p_macro_is_func_like( mex->macro ) );
  assert( mex->arg_list != NULL );

  bool const is_variadic = p_macro_is_variadic( mex->macro );
  size_t const n_req_params = slist_len( mex->macro->param_list ) - is_variadic;
  size_t const n_args = p_arg_list_count( mex->arg_list );

  if ( n_args < n_req_params ) {
    print_error( &mex->name_loc,
      "too few arguments (%zu) for function-like macro (need %s%zu)\n",
      n_args, is_variadic ? "at least " : "", n_req_params
    );
    return false;
  }
  if ( n_args > n_req_params && !is_variadic ) {
    print_error( &mex->name_loc,
      "too many arguments (%zu) for function-like macro (need %zu)\n",
      n_args, n_req_params
    );
    return false;
  }

  return true;
}

/**
 * Checks a #P_STRINGIFY macro for syntactic & semantic errors.
 *
 * @param mex The mex_state to use.
 * @param token_node The node pointing to the #P_STRINGIFY token.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool mex_check_stringify( mex_state_t *mex,
                                 p_token_node_t const *token_node ) {
  assert( mex != NULL );
  assert( token_node != NULL );
  p_token_t const *const stringify_token = token_node->data;
  assert( stringify_token->kind == P_STRINGIFY );

  if ( !p_macro_is_func_like( mex->macro ) ) {
    if ( false_set( &mex->warned_stringify_in_non_func_like_macro ) ) {
      print_warning( &stringify_token->loc,
        "'#' in non-function-like macro treated as text\n"
      );
    }
    return true;
  }

  if ( !OPT_LANG_IS( P_STRINGIFY ) ) {
    if ( false_set( &mex->warned_stringify_not_supported ) ) {
      print_warning( &stringify_token->loc,
        "'#' not supported%s; treated as text\n",
        C_LANG_WHICH( P_STRINGIFY )
      );
    }
    return true;
  }

  token_node = p_token_node_not( token_node->next, P_SPACE );
  if ( token_node == NULL )
    goto error;

  p_token_t const *const token = token_node->data;
  switch ( token->kind ) {
    case P_IDENTIFIER:
      if ( p_macro_find_param( mex->macro, token->ident.name ) == NO_PARAM )
        goto error;
      break;
    case P___VA_ARGS__:
    case P___VA_OPT__:
      break;
    default:
      goto error;
  } // switch

  return true;

error:
  print_error( &stringify_token->loc, "'#' not followed by macro parameter" );
  if ( OPT_LANG_IS( VARIADIC_MACROS ) ) {
    if ( OPT_LANG_IS( P___VA_OPT__ ) )
      EPUTS( ", \"__VA_ARGS__\", or \"__VA_OPT__\"" );
    else
      EPUTS( " or \"__VA_ARGS__\"" );
  }
  EPUTC( '\n' );
  return false;
}

/**
 * Checks a #P___VA_ARGS__ macro for syntactic & semantic errors.
 *
 * @param mex The mex_state to use.
 * @param token_node The node pointing to the #P___VA_ARGS__ token.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool mex_check___VA_ARGS__( mex_state_t *mex,
                                   p_token_node_t const *token_node ) {
  assert( mex != NULL );
  assert( token_node != NULL );
  p_token_t const *const __VA_ARGS___token = token_node->data;
  assert( __VA_ARGS___token->kind == P___VA_ARGS__ );

  if ( !OPT_LANG_IS( VARIADIC_MACROS ) ) {
    if ( false_set( &mex->warned___VA_ARGS___not_supported ) ) {
      print_warning( &__VA_ARGS___token->loc,
        "\"__VA_ARGS__\" not supported%s; treated as text\n",
        C_LANG_WHICH( VARIADIC_MACROS )
      );
    }
    return true;
  }

  if ( !p_macro_is_variadic( mex->macro ) ) {
    print_error( &__VA_ARGS___token->loc,
      "\"__VA_ARGS__\" not allowed in non-variadic macro\n"
    );
    return false;
  }

  return true;
}

/**
 * Checks a #P___VA_OPT__ macro for syntactic & semantic errors.
 *
 * @param mex The mex_state to use.
 * @param token_node The node pointing to the #P___VA_OPT__ token.
 * @return Returns `true` only if all checks passed.
 */
NODISCARD
static bool mex_check___VA_OPT__( mex_state_t const *mex,
                                  p_token_node_t const *token_node ) {
  assert( mex != NULL );
  assert( token_node != NULL );
  p_token_t const *const __VA_OPT___token = token_node->data;
  assert( __VA_OPT___token->kind == P___VA_OPT__ );

  if ( !OPT_LANG_IS( P___VA_OPT__ ) ) {
    print_warning( &__VA_OPT___token->loc,
      "\"__VA_OPT__\" not supported%s; treated as text\n",
      C_LANG_WHICH( P___VA_OPT__ )
    );
    return true;
  }

  if ( !p_macro_is_variadic( mex->macro ) ) {
    print_error( &__VA_OPT___token->loc,
      "\"__VA_OPT__\" not allowed in non-variadic macro\n"
    );
    return false;
  }

  token_node = p_token_node_not( token_node->next, P_SPACE );
  if ( token_node == NULL ) {
    print_error( &C_LOC_NEXT_COL( __VA_OPT___token->loc ), "'(' expected\n" );
    return false;
  }

  p_token_t const *token = token_node->data;
  if ( !p_token_is_punct( token, '(' ) ) {
    print_error( &token->loc,
      "\"%s\": '(' expected\n",
      p_token_str( token )
    );
    return false;
  }

  p_token_t const *prev_token = NULL;
  for ( unsigned paren_count = 1; paren_count > 0; prev_token = token ) {
    token_node = p_token_node_not( token_node->next, P_SPACE );
    if ( token_node == NULL ) {
      print_error( &__VA_OPT___token->loc,
        "unterminated \"__VA_OPT__\" macro\n"
      );
      return false;
    }

    token = token_node->data;

    switch ( token->kind ) {
      case P_CONCAT:
        if ( prev_token == NULL ) {
          print_error( &token->loc,
            "\"##\" can not be first within \"__VA_OPT__\"\n"
          );
          return false;
        }
        break;

      case P_PUNCTUATOR:
        if ( p_punct_token_is_any_char( token ) ) {
          switch ( token->punct.value[0] ) {
            case '(':
              ++paren_count;
              break;
            case ')':
              if ( --paren_count > 0 )
                break;
              if ( prev_token != NULL && prev_token->kind == P_CONCAT ) {
                print_error( &prev_token->loc,
                  "\"##\" can not be last within \"__VA_OPT__\"\n"
                );
                return false;
              }
              continue;
          } // switch
        }
        break;

      case P___VA_OPT__:
        print_error( &token->loc, "\"__VA_OPT__\" can not nest\n" );
        return false;

      case P_CHAR_LIT:
      case P_IDENTIFIER:
      case P_NUM_LIT:
      case P_OTHER:
      case P_PLACEMARKER:
      case P_SPACE:
      case P_STRINGIFY:
      case P_STR_LIT:
      case P___VA_ARGS__:
        // nothing to do
        break;
    } // switch
  } // for

  return true;
}

/**
 * Cleans-up all memory associated with \a mex but _not_ \a mex itself.
 *
 * @param mex The mex_state to clean-up.  If NULL, does nothing.
 *
 * @sa mex_init()
 */
static void mex_cleanup( mex_state_t *mex ) {
  if ( mex == NULL )
    return;                             // LCOV_EXCL_LINE
  // arg_list is the caller's responsibility
  // replace_list is the caller's responsibility
  // expand_list only points to one of work_lists
  p_token_list_cleanup( &mex->va_args_list );
  FREE( mex->va_args_str );
  p_token_list_cleanup( &mex->work_lists[0] );
  p_token_list_cleanup( &mex->work_lists[1] );

  if ( mex->parent_mex == NULL ) {
    rb_tree_cleanup( mex->expanding_set, &free );
    free( mex->expanding_set );
    rb_tree_cleanup( mex->no_expand_set, &free );
    free( mex->no_expand_set );
  }
}

/**
 * Performs the set of expansion functions given by \a fns once followed by
 * mex_expand_all_macros() repeatedly as long as expansions happen.
 *
 * @param mex The mex_state to use.
 * @param fns An array of \ref mex_expand_all_fn_t to be executed in the given
 * order.  The array _must_ be NULL terminated.
 * @return Returns `true` only if successful.
 *
 * @note mex_expand_all_macros() is implicitly done last and _must not_ be
 * included in \a fns.
 *
 * @sa mex_expand_all_fns_impl()
 */
NODISCARD
static bool mex_expand_all_fns( mex_state_t *mex,
                                mex_expand_all_fn_t const fns[static 1] ) {
  assert( mex != NULL );

  mex_rv_t prev_rv = MEX_DID_NOT_EXPAND;

  //
  // Perform the given expansion functions, if any, once.
  //
  if ( mex_expand_all_fns_impl( mex, fns, &prev_rv ) == MEX_ERROR )
    return false;

  //
  // Now call mex_expand_all_macros() repeatedly as long as expansions happen.
  // We could just call it directly, but it's simpler to call it via
  // mex_expand_all_fns_impl() that already handles all the token relocation,
  // list swapping, list clean-up, and printing.
  //
  for (;;) {
    static mex_expand_all_fn_t const EXPAND_FNS[] = {
      &mex_expand_all_macros,
      NULL
    };
    switch ( mex_expand_all_fns_impl( mex, EXPAND_FNS, &prev_rv ) ) {
      case MEX_CAN_NOT_EXPAND:
      case MEX_DID_NOT_EXPAND:
        return true;
      case MEX_EXPANDED:
        break;
      case MEX_ERROR:
        return false;
    } // switch
  } // for
}

/**
 * Performs the set of expansion functions given by \a fns once.
 *
 * @param mex The mex_state to use.
 * @param fns An array of \ref mex_expand_all_fn_t to be executed in the given
 * order.  The array _must_ be NULL terminated.
 * @param prev_rv A pointer to the previous return value.
 * @return Returns a \ref mex_rv.
 *
 * @sa mex_expand_all_fns()
 */
NODISCARD
static mex_rv_t mex_expand_all_fns_impl( mex_state_t *mex,
                                         mex_expand_all_fn_t const
                                           fns[static 1],
                                         mex_rv_t *prev_rv ) {
  assert( mex != NULL );
  assert( prev_rv != NULL );

  mex_rv_t rv = MEX_CAN_NOT_EXPAND;

  for ( unsigned i = 0; fns[i] != NULL; ++i ) {
    if ( *prev_rv == MEX_EXPANDED ) {
      //
      // Make the expand_list from the previous pass become the replace_list
      // for the next pass --- and check it before attempting to expand it.
      //
      mex_swap_lists( mex );
      if ( !mex_check( mex ) )
        return MEX_ERROR;
    }
    else if ( i == 0 ) {
      //
      // Before the first pass, we also want to check the replace_list before
      // attempting to expand it.
      //
      mex_preliminary_relocate_replace_list( mex );
      if ( !mex_check( mex ) )
        return MEX_ERROR;
    }

    p_token_list_cleanup( mex->expand_list );
    *prev_rv = (*fns[i])( mex );

    switch ( *prev_rv ) {
      case MEX_CAN_NOT_EXPAND:
      case MEX_DID_NOT_EXPAND:
        break;
      case MEX_EXPANDED:
        if ( !mex->expand_opt_no_trim_tokens )
          trim_tokens( mex->expand_list );
        mex_relocate_expand_list( mex );
        mex_print_macro( mex, mex->expand_list );
        break;
      case MEX_ERROR:
        return MEX_ERROR;
    } // switch

    if ( *prev_rv > rv )
      rv = *prev_rv;
  } // for

  return rv;
}

/**
 * Performs macro expansion.
 *
 * @param mex The mex_state to use.
 * @param identifier_token A pointer to the #P_IDENTIFIER \ref p_token of the
 * macro.
 * @return Returns a \ref mex_rv.
 *
 * @sa [Macro Expansion Algorithm](https://gcc.gnu.org/onlinedocs/cppinternals/Macro-Expansion.html)
 */
NODISCARD
static mex_rv_t mex_expand( mex_state_t *mex, p_token_t *identifier_token ) {
  assert( mex != NULL );
  assert( identifier_token != NULL );
  assert( identifier_token->kind == P_IDENTIFIER );

  if ( mex->macro->is_dynamic ) {
    p_token_t *token;
    if ( opt_lang_is_any( (*mex->macro->dyn_fn)( &token ) ) ) {;
      push_back_substituted_token( mex->expand_list, token );
      mex_print_macro( mex, mex->expand_list );
      return MEX_EXPANDED;
    }

    identifier_token->ident.ineligible = true;
    return MEX_CAN_NOT_EXPAND;
  }

  if ( !p_macro_check_params( mex->macro ) )
    return MEX_CAN_NOT_EXPAND;

  if ( mex->arg_list == NULL && p_macro_is_func_like( mex->macro ) )
    return MEX_CAN_NOT_EXPAND;

  char const *macro_key = mex_expanding_set_key( mex );
  rb_insert_rv_t const rbi = rb_tree_insert(
    mex->expanding_set,
    CONST_CAST( char*, macro_key )
  );
  if ( !rbi.inserted ) {
    identifier_token->ident.ineligible = true;
    print_warning( &identifier_token->loc,
      "recursive macro \"%s\" will not expand\n",
      mex->macro->name
    );
    return MEX_CAN_NOT_EXPAND;
  }

  //
  // Now that we know the macro has been inserted, replace the static key used
  // to test for insertion with a dynamically allocated copy.  Doing it this
  // way means we do the look-up only once and the strdup() only if inserted.
  //
  rbi.node->data = check_strdup( rbi.node->data );

  mex_print_macro( mex, mex->replace_list );
  mex_pre_filter___VA_OPT__( mex );

  static mex_expand_all_fn_t const EXPAND_FNS[] = {
    //
    // Stringification must occur before general parameter expansion because
    // a parameter that expands into multiple tokens must be stringified as a
    // single string:
    //
    //      cdecl> #define Q(X)   #X
    //      cdecl> expand Q(a b)
    //      Q(a b) => #X
    //      Q(a b) => "a b"             // not: "a" b
    //
    // Note that mex_expand_all_stringify() does its own specific parameter
    // expansion so it knows all the token(s) comprising the parameter.
    //
    &mex_expand_all_stringify,
    &mex_expand_all_params,

    //
    // Next, these.
    //
    &mex_expand_all___VA_ARGS__,
    &mex_expand_all___VA_OPT__,

    //
    // Finally after everything has been expanded, concatenation (if any),
    // can be done.
    //
    &mex_expand_all_concat,

    //
    // Note that mex_expand_all_fns() does macro expansion implicitly.
    //
    NULL
  };

  bool const ok = mex_pre_expand___VA_ARGS__( mex ) &&
                  mex_expand_all_fns( mex, EXPAND_FNS );

  macro_key = rb_tree_delete( mex->expanding_set, rbi.node );
  assert( macro_key != NULL );
  FREE( macro_key );

  if ( !ok )
    return MEX_ERROR;

  if ( mex->arg_list != NULL && !p_macro_is_func_like( mex->macro ) ) {
    //
    // "Arguments" following a non-function-like macro are simply appended.
    //
    if ( !mex_append_args( mex ) )
      return MEX_ERROR;
    mex_print_macro( mex, mex->expand_list );
  }

  return MEX_EXPANDED;
}

/**
 * Concatenates tokens separated by #P_CONCAT, if any, together.
 *
 * @remarks
 * @parblock
 * Unlike #P_STRINGIFY where multi-token arguments are stringified as a single
 * string:
 *
 *      cdecl> #define Q(X)       #X
 *      cdecl> expand Q(a b)
 *      Q(a b) => #X
 *      Q(a b) => "a b"
 *
 * #P_CONCAT concatenates only adjacent tokens after parameter substitution
 * even if any argument was multi-token:
 *
 *      cdecl> #define C(X,Y)     X ## Y
 *      cdecl> expand C(a b, c d)
 *      C(a b, c d) => X ## Y
 *      C(a b, c d) => a b ## c d
 *      C(a b, c d) => a bc d
 *
 * Above, only `b` and `c` are concatenated even though `X` was substituted for
 * `a b` and `Y` was substituted for `c d`.
 * @endparblock
 *
 * @param mex The mex_state to use.
 * @return Returns a \ref mex_rv.
 */
NODISCARD
static mex_rv_t mex_expand_all_concat( mex_state_t *mex ) {
  assert( mex != NULL );

  if ( !OPT_LANG_IS( P_CONCAT ) )
    return MEX_CAN_NOT_EXPAND;

  mex_rv_t rv = MEX_DID_NOT_EXPAND;

  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t const *const token = token_node->data;
    p_token_node_t *next_node = p_token_node_not( token_node->next, P_SPACE );
    if ( next_node == NULL )
      goto skip;
    p_token_t const *next_token = next_node->data;
    if ( next_token->kind != P_CONCAT )
      goto skip;

    if ( p_token_is_macro( token ) ) {
      print_warning( &token->loc,
        "\"##\" doesn't expand macro arguments; \"%s\" will not expand\n",
        token->ident.name
      );
    }

    strbuf_t sbuf;
    strbuf_init( &sbuf );
    strbuf_puts( &sbuf, p_token_str( token ) );

    do {
      token_node = p_token_node_not( next_node->next, P_SPACE );
      assert( token_node != NULL );
      next_token = token_node->data;

      if ( p_token_is_macro( next_token ) ) {
        print_warning( &next_token->loc,
          "\"##\" doesn't expand macro arguments; \"%s\" will not expand\n",
          next_token->ident.name
        );
      }

      strbuf_puts( &sbuf, p_token_str( next_token ) );
      next_node = p_token_node_not( token_node->next, P_SPACE );
      if ( next_node == NULL )
        break;
      next_token = next_node->data;
    } while ( next_token->kind == P_CONCAT );

    p_token_t *const concatted_token = lex_token( &token->loc, &sbuf );
    strbuf_cleanup( &sbuf );
    if ( concatted_token == NULL )
      return MEX_ERROR;
    push_back_substituted_token( mex->expand_list, concatted_token );
    rv = MEX_EXPANDED;
    continue;

skip:
    push_back_dup_token( mex->expand_list, token );
  } // for

  return rv;
}

/**
 * Expands macros recursively.
 *
 * @param mex The mex_state to use.
 * @return Returns a \ref mex_rv.
 */
NODISCARD
static mex_rv_t mex_expand_all_macros( mex_state_t *mex ) {
  assert( mex != NULL );

  mex_rv_t rv = MEX_DID_NOT_EXPAND;

  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t const *const token = token_node->data;
    if ( token->kind == P_IDENTIFIER ) {
      switch ( mex_expand_identifier( mex, &token_node ) ) {
        case MEX_CAN_NOT_EXPAND:
        case MEX_DID_NOT_EXPAND:
          break;
        case MEX_EXPANDED:
          rv = MEX_EXPANDED;
          continue;
        case MEX_ERROR:
          return MEX_ERROR;
      } // switch
    }

    push_back_dup_token( mex->expand_list, token );
  } // for

  return rv;
}

/**
 * Expands all macro parameters, if any, into their respective arguments.
 *
 * @param mex The mex_state to use.
 * @return Returns a \ref mex_rv.
 */
NODISCARD
static mex_rv_t mex_expand_all_params( mex_state_t *mex ) {
  assert( mex != NULL );

  mex_rv_t rv = MEX_DID_NOT_EXPAND;

  //
  // Keep track of parameters we've expanded so we neither do redundant work of
  // expanding them more than once nor print their expansions more than once.
  // For example, given:
  //
  //      #define QUOTE(X)      #X
  //      #define CHAR_PTR(X)   char const *X = QUOTE(X)
  //      expand CHAR_PTR(p)
  //
  // we will NOT get:
  //
  //      CHAR_PTR(p) => char const *X = QUOTE(X)
  //      | X => p
  //      | X => p
  //
  // even though the parameter "X" occurs twice in the replacement list.
  //
  rb_tree_t param_cache;
  rb_tree_init( &param_cache, POINTER_CAST( rb_cmp_fn_t, &param_expand_cmp ) );

  p_token_node_t const *prev_node = NULL;
  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t const *const token = token_node->data;
    if ( token->kind != P_IDENTIFIER )
      goto skip;
    p_token_list_t *arg_tokens = mex_param_arg( mex, token->ident.name );
    if ( arg_tokens == NULL )           // identifier isn't a parameter
      goto skip;

    p_token_node_t *const next_node =
      p_token_node_not( token_node->next, P_SPACE );
    if ( is_operator_arg( prev_node, next_node ) )
      goto append;

    param_expand_t find_pe = { .name = token->ident.name };
    rb_insert_rv_t rbi = rb_tree_insert( &param_cache, &find_pe );

    if ( !rbi.inserted ) {
      param_expand_t const *const found_pe = rbi.node->data;
      arg_tokens = found_pe->expand_list;
      goto append;
    }

    mex_state_t param_mex;
    mex_init( &param_mex,
      /*parent_mex=*/mex,
      &(p_macro_t){ .name = token->ident.name },
      /*arg_list=*/NULL,
      /*replace_list=*/arg_tokens,
      mex->fout
    );
    param_mex.param_node = token_node;

    mex_print_macro( &param_mex, param_mex.replace_list );

    //
    // Since the argument tokens were plucked for the corresponding macro
    // parameter, we first have to make their locations correct before
    // expanding them.
    //
    push_back_dup_tokens( param_mex.expand_list, param_mex.replace_list );
    mex_relocate_expand_list( &param_mex );
    mex_swap_lists( &param_mex );

    static mex_expand_all_fn_t const EXPAND_FNS[] = {
      //
      // Parameters in replace_list, after having their corresponding arguments
      // substituted for them, need only stringification and concatenation.
      // Note that mex_expand_all_fns() does macro expansion implicitly.
      //
      &mex_expand_all_stringify,
      &mex_expand_all_concat,
      NULL
    };

    bool const ok = mex_expand_all_fns( &param_mex, EXPAND_FNS );
    if ( ok ) {
      p_token_list_cleanup( arg_tokens );
      *arg_tokens = slist_move( param_mex.expand_list );
      if ( slist_empty( arg_tokens ) ) {
        //
        // If a parameter expands into nothing, push a P_PLACEMARKER to be an
        // argument for either P_CONCAT or P_STRINGIFY.
        //
        slist_push_back(
          arg_tokens, p_token_new( P_PLACEMARKER, /*literal=*/NULL )
        );
      }

      //
      // Now that we know the parameter is newly and successfully expanded,
      // replace the find_pe key used to test for insertion with a dynamically
      // allocated copy.  Doing it this way means we do the look-up only once
      // and the dynamic allocation only if inserted.
      //
      param_expand_t *const new_pe = MALLOC( param_expand_t, 1 );
      *new_pe = (param_expand_t){
        //
        // There's no need either to strdup() the name or duplicate the tokens
        // here because the both will outlive this param_expand_t node.
        //
        .name = token->ident.name,
        .expand_list = arg_tokens
      };
      rbi.node->data = new_pe;
    }

    mex_cleanup( &param_mex );
    if ( !ok ) {
      rv = MEX_ERROR;
      goto error;
    }

append:
    set_substituted( push_back_dup_tokens( mex->expand_list, arg_tokens ) );
    rv = MEX_EXPANDED;
    goto next;

skip:
    push_back_dup_token( mex->expand_list, token );
next:
    if ( token->kind != P_SPACE )
      prev_node = token_node;
  } // for

error:
  rb_tree_cleanup( &param_cache, &free );
  return rv;
}

/**
 * Expands all #P_STRINGIFY tokens.
 *
 * @param mex The mex_state to use.
 * @return Returns a \ref mex_rv.
 *
 * @sa mex_expand_stringify()
 */
NODISCARD
static mex_rv_t mex_expand_all_stringify( mex_state_t *mex ) {
  assert( mex != NULL );

  if ( !OPT_LANG_IS( P_STRINGIFY ) )
    return MEX_CAN_NOT_EXPAND;

  mex_rv_t rv = MEX_DID_NOT_EXPAND;

  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t *const token = token_node->data;
    if ( token->kind == P_STRINGIFY ) {
      switch ( mex_expand_stringify( mex, &token_node ) ) {
        case MEX_CAN_NOT_EXPAND:
        case MEX_DID_NOT_EXPAND:
          break;
        case MEX_EXPANDED:
          rv = MEX_EXPANDED;
          continue;
        case MEX_ERROR:
          return MEX_ERROR;
      } // switch
    }

    push_back_dup_token( mex->expand_list, token );
  } // for

  return rv;
}

/**
 * Expands all #P___VA_ARGS__ tokens, if any.
 *
 * @param mex The mex_state to use.
 * @return Returns a \ref mex_rv.
 *
 * @sa mex_expand_all___VA_OPT__()
 */
NODISCARD
static mex_rv_t mex_expand_all___VA_ARGS__( mex_state_t *mex ) {
  assert( mex != NULL );

  if ( !OPT_LANG_IS( VARIADIC_MACROS ) )
    return MEX_CAN_NOT_EXPAND;

  mex_rv_t rv = MEX_DID_NOT_EXPAND;

  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t *const token = token_node->data;
    if ( token->kind == P___VA_ARGS__ ) {
      set_substituted(
        push_back_dup_tokens( mex->expand_list, &mex->va_args_list )
      );
      rv = MEX_EXPANDED;
      continue;
    }
    push_back_dup_token( mex->expand_list, token );
  } // for

  return rv;
}

/**
 * Expands all #P___VA_OPT__ tokens, if any.
 *
 * @param mex The mex_state to use.
 * @return Returns the last node of the expansion.
 */
NODISCARD
static mex_rv_t mex_expand_all___VA_OPT__( mex_state_t *mex ) {
  assert( mex != NULL );

  if ( !OPT_LANG_IS( P___VA_OPT__ ) )
    return MEX_CAN_NOT_EXPAND;

  mex_rv_t rv = MEX_DID_NOT_EXPAND;

  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t const *const token = token_node->data;
    if ( token->kind == P___VA_OPT__ ) {
      token_node = mex_expand___VA_OPT__( mex, token_node, mex->expand_list );
      if ( token_node == NULL )
        return MEX_ERROR;
      rv = MEX_EXPANDED;
      continue;
    }
    push_back_dup_token( mex->expand_list, token );
  } // for

  return rv;
}

/**
 * Expands an #P_IDENTIFIER if it's a macro.
 *
 * @param mex The mex_state to use.
 * @param ptoken_node A pointer to the token node whose token is a
 * #P_IDENTIFIER.  If the identifier is a macro with arguments, on return \a
 * *ptoken_node is set to point to \ref p_token of the `)`; or NULL upon error.
 * @return Returns a \ref mex_rv.
 */
static mex_rv_t mex_expand_identifier( mex_state_t *mex,
                                       p_token_node_t **ptoken_node ) {
  assert( mex != NULL );
  assert( ptoken_node != NULL );
  p_token_node_t *token_node = *ptoken_node;
  assert( token_node != NULL );
  p_token_t *const identifier_token = token_node->data;
  assert( identifier_token->kind == P_IDENTIFIER );

  if ( identifier_token->ident.ineligible )
    return MEX_CAN_NOT_EXPAND;

  p_macro_t const *const found_macro =
    p_macro_find( identifier_token->ident.name );
  if ( found_macro == NULL )            // identifier is not a macro
    return MEX_CAN_NOT_EXPAND;

  p_arg_list_t arg_list;
  slist_init( &arg_list );
  bool looks_func_like = false;
  mex_rv_t rv = MEX_ERROR;

  p_token_node_t *const next_node =
    p_token_node_not( token_node->next, P_SPACE );
  if ( p_token_node_is_punct( next_node, '(' ) ) {
    token_node = parse_args( next_node, &arg_list );
    if ( token_node == NULL )
      goto parse_args_error;
    looks_func_like = true;
  }

  mex_state_t macro_mex;
  mex_init( &macro_mex,
    /*parent_mex=*/mex,
    found_macro,
    looks_func_like ? &arg_list : NULL,
    &found_macro->replace_list,
    mex->fout
  );

  if ( mex_prep_args( &macro_mex ) ) {
    rv = mex_expand( &macro_mex, identifier_token );
    switch ( rv ) {
      case MEX_EXPANDED:
        *ptoken_node = token_node;
        set_substituted(
          push_back_dup_tokens( mex->expand_list, macro_mex.expand_list )
        );
        break;
      case MEX_CAN_NOT_EXPAND:
      case MEX_DID_NOT_EXPAND:
      case MEX_ERROR:
        break;
    } // switch
  }

  mex_cleanup( &macro_mex );

parse_args_error:
  p_arg_list_cleanup( &arg_list );
  return rv;
}

/**
 * Expands a single #P_STRINGIFY token.
 *
 * @param mex The mex_state to use.
 * @param ptoken_node A pointer to the token node whose token is a
 * #P_STRINGIFY.  On return \a *ptoken_node is set to point to \ref p_token of
 * macro parameter that was stringified `)`; or NULL upon error.
 * @return Returns a \ref mex_rv.
 *
 * @sa mex_expand_all_stringify()
 */
NODISCARD
static mex_rv_t mex_expand_stringify( mex_state_t *mex,
                                      p_token_node_t **ptoken_node ) {
  assert( OPT_LANG_IS( P_STRINGIFY ) );
  assert( mex != NULL );
  assert( ptoken_node != NULL );
  p_token_node_t *const stringify_node = *ptoken_node;
  assert( p_token_node_is_any( stringify_node, P_STRINGIFY ) );

  if ( !p_macro_is_func_like( mex->macro ) ) {
    //
    // When # appears in the replacement list of a non-function-like macro, it
    // is treated as an ordinary character.
    //
    return MEX_CAN_NOT_EXPAND;
  }

  p_token_node_t *next_node = p_token_node_not( stringify_node->next, P_SPACE );
  assert( next_node != NULL );
  p_token_t const *const next_token = next_node->data;
  switch ( next_token->kind ) {
    case P_IDENTIFIER:
      mex_stringify_identifier( mex, next_token );
      break;
    case P___VA_ARGS__:
      mex_stringify___VA_ARGS__( mex );
      break;
    case P___VA_OPT__:
      next_node = mex_stringify___VA_OPT__( mex, next_node );
      break;
    default:
      UNEXPECTED_INT_VALUE( next_token->kind );
  } // switch

  *ptoken_node = next_node;
  return MEX_EXPANDED;
}

/**
 * Expands the `__VA_OPT__` token.
 *
 * @param mex The mex_state to use.
 * @param __VA_OPT___node The \ref p_token_node_t of `__VA_OPT__`.
 * @param dst_list The \ref p_token_list_t to append onto.
 * @return Returns the last node of the expansion.
 *
 * @note This function is called via both mex_expand_all___VA_OPT__() and
 * mex_stringify___VA_OPT__().  When `__VA_OPT__` is not supported, it is _not_
 * called via the former but _is_ called via the latter.
 *
 * @sa mex_check___VA_OPT__()
 * @sa mex_pre_expand___VA_ARGS__()
 */
NODISCARD
static p_token_node_t* mex_expand___VA_OPT__( mex_state_t *mex,
                                              p_token_node_t *__VA_OPT___node,
                                              p_token_list_t *dst_list ) {
  assert( mex != NULL );
  assert( p_macro_is_variadic( mex->macro ) );
  assert( p_token_node_is_any( __VA_OPT___node, P___VA_OPT__ ) );
  assert( dst_list != NULL );
  assert( OPT_LANG_IS( P___VA_OPT__ ) );

  p_token_node_t *token_node =
    p_token_node_not( __VA_OPT___node->next, P_SPACE );
  assert( p_token_node_is_punct( token_node, '(' ) );

  bool const is_va_args_empty =
    str_is_empty( empty_if_null( mex->va_args_str ) );

  p_token_list_t va_opt_list;
  slist_init( &va_opt_list );

  for ( unsigned paren_count = 1; paren_count > 0; ) {
    token_node = token_node->next;
    p_token_t const *const token = token_node->data;
    if ( p_token_is_any_char( token ) ) {
      switch ( token->punct.value[0] ) {
        case '(':
          ++paren_count;
          break;
        case ')':
          if ( --paren_count == 0 )
            continue;
          break;
      } // switch
    }
    if ( !is_va_args_empty )
      push_back_dup_token( &va_opt_list, token );
  } // for

  if ( !is_va_args_empty ) {
    mex_state_t va_opt_mex;
    mex_init( &va_opt_mex,
      /*parent_mex=*/mex,
      &(p_macro_t){
        .name = L___VA_OPT__,
        .param_list = mex->macro->param_list
      },
      mex->arg_list,
      /*replace_list=*/&va_opt_list,
      mex->fout
    );
    va_opt_mex.print_opt_omit_args = true;
    mex_print_macro( &va_opt_mex, va_opt_mex.replace_list );

    static mex_expand_all_fn_t const EXPAND_FNS[] = {
      &mex_expand_all_stringify,
      &mex_expand_all_params,
      &mex_expand_all_concat,
      NULL
    };

    if ( mex_expand_all_fns( &va_opt_mex, EXPAND_FNS ) )
      push_back_dup_tokens( dst_list, va_opt_mex.expand_list );
    else
      token_node = NULL;
    mex_cleanup( &va_opt_mex );
  }

  p_token_list_cleanup( &va_opt_list );
  return token_node;
}

/**
 * Generates a distinct key for a macro for use with \ref
 * mex_state::expanding_set "expanding_set".
 *
 * @param mex The mex_state to use.
 * @return Returns said key.
 *
 * @warning The pointer returned is to a static buffer.
 */
NODISCARD
static char const* mex_expanding_set_key( mex_state_t const *mex ) {
  assert( mex != NULL );

  static strbuf_t sbuf;
  strbuf_reset( &sbuf );

  strbuf_puts( &sbuf, mex->macro->name );
  if ( mex->arg_list != NULL ) {
    bool comma = false;
    strbuf_putc( &sbuf, '(' );
    FOREACH_SLIST_NODE( arg_node, mex->arg_list ) {
      strbuf_sepc_puts(
        &sbuf, ',', &comma, p_token_list_str( arg_node->data )
      );
    } // for
    strbuf_putc( &sbuf, ')' );
  }

  return sbuf.str;
}

/**
 * Initializes \a mex.
 *
 * @param mex The mex_state to initialize.
 * @param parent_mex The parent mex_state, if any.
 * @param macro The macro to use.
 * @param arg_list The argument list, if any.
 * @param replace_list The replacement token list.
 * @param fout The `FILE` to print to.
 *
 * @sa mex_cleanup()
 */
static void mex_init( mex_state_t *mex, mex_state_t *parent_mex,
                      p_macro_t const *macro, p_arg_list_t *arg_list,
                      p_token_list_t const *replace_list, FILE *fout ) {
  assert( mex != NULL );
  assert( macro != NULL );
  assert( macro->name != NULL );
  assert( replace_list != NULL );
  assert( fout != NULL );

  rb_tree_t *expanding_set;
  unsigned   indent;
  rb_tree_t *no_expand_set;

  if ( parent_mex == NULL ) {
    expanding_set = MALLOC( rb_tree_t, 1 );
    rb_tree_init( expanding_set, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );
    indent = 0;
    no_expand_set = MALLOC( rb_tree_t, 1 );
    rb_tree_init( no_expand_set, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );
  }
  else {
    expanding_set = parent_mex->expanding_set;
    indent = parent_mex->indent + 1;
    assert( indent <= INDENT_MAX && "large indentation: infinite recursion?" );
    no_expand_set = parent_mex->no_expand_set;
  }

  *mex = (mex_state_t){
    .parent_mex = parent_mex,
    .macro = macro,
    .name_loc = str_loc( macro->name ),
    .arg_list = arg_list,
    .replace_list = replace_list,
    .expand_list = &mex->work_lists[0],
    .expanding_set = expanding_set,
    .no_expand_set = no_expand_set,
    .fout = fout,
    .indent = indent
  };
}

/**
 * Given \a param_name, gets the corresponding macro argument.
 *
 * @param mex The mex_state to use.
 * @param param_name The name of a macro parameter.
 * @return Returns said argument or NULL if \a param_name isn't a macro
 * parameter.
 */
NODISCARD
static p_token_list_t* mex_param_arg( mex_state_t const *mex,
                                      char const *param_name ) {
  assert( mex != NULL );
  assert( param_name != NULL );

  if ( mex->arg_list == NULL || !p_macro_is_func_like( mex->macro ) )
    return NULL;
  size_t const param_index = p_macro_find_param( mex->macro, param_name );
  if ( param_index == NO_PARAM )
    return NULL;
  return slist_at( mex->arg_list, param_index );
}

/**
 * Expands the #P___VA_ARGS__ token.
 *
 * @param mex The mex_state to use.
 * @return Returns `true` only if expansion succeeded.
 *
 * @sa mex_check___VA_ARGS__()
 * @sa mex_expand___VA_OPT__()
 * @sa mex_stringify___VA_ARGS__()
 */
static bool mex_pre_expand___VA_ARGS__( mex_state_t *mex ) {
  assert( mex != NULL );

  if ( !p_macro_is_variadic( mex->macro ) )
    return true;

  assert( OPT_LANG_IS( VARIADIC_MACROS ) );

  bool comma = false;
  size_t curr_index = 0;
  size_t const ellipsis_index = slist_len( mex->macro->param_list ) - 1;

  p_token_list_t va_args_list;
  slist_init( &va_args_list );

  FOREACH_SLIST_NODE( arg_node, mex->arg_list ) {
    if ( curr_index++ < ellipsis_index )
      continue;
    if ( true_or_set( &comma ) )
      slist_push_back( &va_args_list, p_token_new( P_PUNCTUATOR, "," ) );
    push_back_dup_tokens( &va_args_list, arg_node->data );
  } // for

  mex_state_t va_args_mex;
  mex_init( &va_args_mex,
    /*parent_mex=*/mex,
    &(p_macro_t){ .name = L___VA_ARGS__ },
    /*arg_list=*/NULL,
    /*replace_list=*/&va_args_list,
    mex->fout
  );
  mex_print_macro( &va_args_mex, va_args_mex.replace_list );

  static mex_expand_all_fn_t const EXPAND_FNS[] = {
    &mex_expand_all_stringify,
    &mex_expand_all_params,
    &mex_expand_all_concat,
    NULL
  };

  bool const ok = mex_expand_all_fns( &va_args_mex, EXPAND_FNS );
  if ( ok ) {
    mex->va_args_list = slist_move( va_args_mex.expand_list );
    mex->va_args_str = check_strdup( p_token_list_str( &mex->va_args_list ) );
  }
  mex_cleanup( &va_args_mex );
  p_token_list_cleanup( &va_args_list );
  return ok;
}

/**
 * Pre-filter \ref mex_state::replace_list "replace_list" retroactively
 * replacing #P___VA_OPT__ tokens with #P_IDENTIFIER tokens if #P___VA_OPT__ is
 * not supported in the current language.
 *
 * @param mex The mex_state to use.
 */
static void mex_pre_filter___VA_OPT__( mex_state_t *mex ) {
  assert( mex != NULL );
  if ( OPT_LANG_IS( P___VA_OPT__ ) )
    return;

  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t *const token = token_node->data;
    if ( token->kind != P___VA_OPT__ )
      continue;
    if ( false_set( &mex->warned___VA_OPT___not_supported ) ) {
      print_warning( &token->loc,
        "\"__VA_OPT__\" not supported%s; will not expand\n",
        C_LANG_WHICH( P___VA_OPT__ )
      );
    }
    token->kind = P_IDENTIFIER;
    token->ident.name = check_strdup( L___VA_OPT__ );
    token->ident.ineligible = true;
  } // for
}

/**
 * Performs preliminary checks just prior to macro expansion.
 *
 * @param mex The mex_state to use.
 */
NODISCARD
static bool mex_preliminary_check( mex_state_t const *mex ) {
  assert( mex != NULL );

  //
  // In order to call mex_check() on the argument of the "expand" command, it
  // has to be in a replace_list, so create a dummy "preliminary_check" macro
  // whose replace_list is just that.  For example, given:
  //
  //      expand M(42)
  //
  // create:
  //
  //      preliminary_check => M(42)
  //
  // then call mex_check() on the right-hand side.
  //

  p_token_list_t replace_list;
  slist_init( &replace_list );
  slist_push_back( &replace_list,
    p_token_new_loc(
      P_IDENTIFIER,
      &mex->name_loc,
      check_strdup( mex->macro->name )
    )
  );

  if ( mex->arg_list != NULL ) {
    slist_push_back( &replace_list, p_token_new( P_PUNCTUATOR, "(" ) );
    bool comma = false;
    FOREACH_SLIST_NODE( arg_node, mex->arg_list ) {
      if ( true_or_set( &comma ) )
        slist_push_back( &replace_list, p_token_new( P_PUNCTUATOR, "," ) );
      push_back_dup_tokens( &replace_list, arg_node->data );
    } // for
    slist_push_back( &replace_list, p_token_new( P_PUNCTUATOR, ")" ) );
  }

  mex_state_t check_mex;
  mex_init( &check_mex,
    /*parent_mex=*/NULL,
    &(p_macro_t){ .name = "preliminary_check" },
    /*arg_list=*/NULL,
    &replace_list,
    mex->fout
  );

  bool const ok = mex_check( &check_mex );
  mex_cleanup( &check_mex );
  p_token_list_cleanup( &replace_list );
  return ok;
}

/**
 * Only before the first expansion pass, adjusts the \ref c_loc::first_column
 * "first_column" and \ref c_loc::last_column "last_column" of \ref
 * p_token::loc "loc" for every token comprising the current \ref
 * mex_state::replace_list "replace_list".
 *
 * @param mex The mex_state to use.
 *
 * @sa mex_relocate_expand_list()
 */
static void mex_preliminary_relocate_replace_list( mex_state_t *mex ) {
  assert( mex != NULL );

  if ( mex->macro->is_dynamic )
    return;

  if ( mex->replace_list == &mex->macro->replace_list ) {
    assert( slist_empty( mex->expand_list ) );
    push_back_dup_tokens( mex->expand_list, mex->replace_list );
    mex_relocate_expand_list( mex );
    mex_swap_lists( mex );
  }
}

/**
 * Prepares macro arguments and expands them.
 *
 * @param mex The mex_state to use.
 * @return Returns `true` only if the macro is not function-like, there are no
 * arguments, or preparation succeeded.
 */
NODISCARD
static bool mex_prep_args( mex_state_t *mex ) {
  assert( mex != NULL );

  if ( !p_macro_is_func_like( mex->macro ) || mex->arg_list == NULL )
    return true;
  trim_args( mex->arg_list );
  return mex_check_num_args( mex );
}

/**
 * Prints \a mex's \ref mex_state::arg_list "arg_list", if any, between
 * parentheses and separated by commas.
 *
 * @param mex The mex_state to use.
 *
 * @sa mex_print_macro()
 * @sa print_token_list()
 */
static void mex_print_arg_list( mex_state_t const *mex ) {
  assert( mex != NULL );
  assert( p_macro_is_func_like( mex->macro ) );
  assert( mex->arg_list != NULL );

  FPUTC( '(', mex->fout );
  bool comma = false;
  FOREACH_SLIST_NODE( arg_node, mex->arg_list ) {
    p_token_list_t const *const arg_tokens = arg_node->data;
    bool const emptyish = p_token_list_emptyish( arg_tokens );
    if ( true_or_set( &comma ) ) {
      FPUTC( ',', mex->fout );
      if ( !emptyish )
        FPUTC( ' ', mex->fout );
    }
    if ( !emptyish )
      print_token_list( arg_tokens, mex->fout );
  } // for
  FPUTC( ')', mex->fout );
}

/**
 * Prints a macro's name, arguments (if any), and \a token_list.
 *
 * @param mex The mex_state to use.
 * @param token_list The token list to print.
 *
 * @sa color_print_token_list()
 * @sa mex_print_arg_list()
 */
static void mex_print_macro( mex_state_t const *mex,
                             p_token_list_t const *token_list ) {
  assert( mex != NULL );

  FOR_N_TIMES( mex->indent ) {
    color_start( mex->fout, sgr_macro_punct );
    FPUTC( '|', mex->fout );
    color_end( mex->fout, sgr_macro_punct );
    FPUTC( ' ', mex->fout );
  } // for

  FPUTS( mex->macro->name, mex->fout );

  bool const print_arg_list = mex->arg_list != NULL &&
    !mex->print_opt_omit_args &&
    p_macro_is_func_like( mex->macro );

  if ( print_arg_list )
    mex_print_arg_list( mex );

  FPUTC( ' ', mex->fout );
  color_start( mex->fout, sgr_macro_punct );
  FPUTS( ARROW, mex->fout );
  color_end( mex->fout, sgr_macro_punct );

  bool const print_token_list = !p_token_list_emptyish( token_list );

  if ( print_token_list ) {
    FPUTC( ' ', mex->fout );
    color_print_token_list( token_list, mex->fout );
  }
  FPUTC( '\n', mex->fout );

  if ( opt_cdecl_debug == CDECL_DEBUG_NO )
    return;

  DUMP_START( mex->indent );
  DUMP_STR( mex->indent, "macro", mex->macro->name );
  if ( print_arg_list ) {
    DUMP_KEY( mex->indent, "arg_list: " );
    p_arg_list_dump( mex->arg_list, mex->indent + 1, stdout );
  }
  if ( print_token_list ) {
    DUMP_KEY( mex->indent, "token_list: " );
    p_token_list_dump( token_list, mex->indent + 1, mex->fout );
  }
  DUMP_END( mex->indent );
}

/**
 * Adjusts the \ref c_loc::first_column "first_column" and \ref
 * c_loc::last_column "last_column" of \ref p_token::loc "loc" for every token
 * comprising the current \ref mex_state::expand_list "expand_list".
 *
 * @param mex The mex_state to use.
 *
 * @note The column caclulations _must_ match how mex_print_macro() prints.
 *
 * @sa mex_preliminary_relocate_replace_list()
 * @sa p_token_list_relocate()
 */
static void mex_relocate_expand_list( mex_state_t *mex ) {
  assert( mex != NULL );

  size_t column = 2/*"| "*/ * mex->indent + strlen( mex->macro->name );

  if ( !mex->print_opt_omit_args && p_macro_is_func_like( mex->macro ) &&
       mex->arg_list != NULL ) {
    ++column;                           // '('
    bool comma = false;
    FOREACH_SLIST_NODE( arg_node, mex->arg_list ) {
      p_token_list_t *const arg_tokens = arg_node->data;
      bool const emptyish = p_token_list_emptyish( arg_tokens );
      if ( true_or_set( &comma ) ) {
        ++column;                       // ','
        if ( emptyish )
          continue;
        ++column;                       // ' '
      }
      else if ( emptyish ) {
        continue;
      }
      column = p_token_list_relocate( arg_tokens, column );
    } // for
    ++column;                           // ')'
  }

  column += 1/*space*/ + STRLITLEN( ARROW ) + 1/*space*/;
  PJL_IGNORE_RV( p_token_list_relocate( mex->expand_list, column ) );
}

/**
 * Stringifies a #P_IDENTIFIER token.
 *
 * @param mex The mex_state to use.
 * @param identifier_token The \ref p_token of an identifier.
 */
static void mex_stringify_identifier( mex_state_t *mex,
                                      p_token_t const *identifier_token ) {
  assert( mex != NULL );
  assert( p_macro_is_func_like( mex->macro ) );
  assert( identifier_token != NULL );
  assert( identifier_token->kind == P_IDENTIFIER );

  p_token_list_t const *const arg_tokens =
    mex_param_arg( mex, identifier_token->ident.name );
  assert( arg_tokens != NULL );

  if ( slist_len( arg_tokens ) == 1 ) {
    p_token_t const *const arg_token = slist_front( arg_tokens );
    if ( p_token_is_macro( arg_token ) ) {
      print_warning( &identifier_token->loc,
        "'#' doesn't expand macro arguments; \"%s\" will not expand\n",
        arg_token->ident.name
      );
    }
  }

  p_token_t *const stringified_token =
    p_token_new( P_STR_LIT, check_strdup( p_token_list_str( arg_tokens ) ) );
  push_back_substituted_token( mex->expand_list, stringified_token );
}

/**
 * Stringifies the #P___VA_ARGS__ token.
 *
 * @param mex The mex_state to use.
 *
 * @sa mex_pre_expand___VA_ARGS__()
 * @sa mex_stringify___VA_OPT__()
 */
static void mex_stringify___VA_ARGS__( mex_state_t *mex ) {
  assert( mex != NULL );
  assert( mex->va_args_str != NULL );

  p_token_t *const stringified_token =
    p_token_new( P_STR_LIT, check_strdup( mex->va_args_str ) );
  push_back_substituted_token( mex->expand_list, stringified_token );
}

/**
 * Stringifies the #P___VA_OPT__ token.
 *
 * @param mex The mex_state to use.
 * @param __VA_OPT___node The \ref p_token_node_t pointing to the #P___VA_OPT__
 * token.
 * @return Returns the last node of the expansion.
 *
 * @sa mex_expand___VA_OPT__()
 * @sa mex_stringify___VA_ARGS__()
 */
NODISCARD
static p_token_node_t* mex_stringify___VA_OPT__( mex_state_t *mex,
                                                 p_token_node_t
                                                  *__VA_OPT___node ) {
  assert( mex != NULL );
  assert( p_token_node_is_any( __VA_OPT___node, P___VA_OPT__ ) );

  p_token_list_t va_opt_list;
  slist_init( &va_opt_list );

  p_token_node_t *const rv_node =
    mex_expand___VA_OPT__( mex, __VA_OPT___node, &va_opt_list );
  p_token_t *const stringified_token =
    p_token_new( P_STR_LIT, check_strdup( p_token_list_str( &va_opt_list ) ) );
  p_token_list_cleanup( &va_opt_list );
  push_back_substituted_token( mex->expand_list, stringified_token );

  return rv_node;
}

/**
 * Swaps the pointers for \ref mex_state::replace_list "replace_list" and \ref
 * mex_state::expand_list "expand_list".
 *
 * @param mex The mex_state to use.
 */
static void mex_swap_lists( mex_state_t *mex ) {
  assert( mex != NULL );
  bool const expand_is_work_list_1 = mex->expand_list == &mex->work_lists[1];
  mex->replace_list = &mex->work_lists[  expand_is_work_list_1 ];
  mex->expand_list  = &mex->work_lists[ !expand_is_work_list_1 ];
}

/**
 * Gets the number of _actual_ arguments of \a arg_list.
 *
 * @remarks
 * @parblock
 * There can be three cases:
 *
 *  1. If the length of \a arg_list is _not_ 1, then that is the number of
 *     actual arguments.
 *
 *  2. Otherwise, if the number of tokens of the lone argument is &gt; 0, then
 *     the number of actual arguments is 1.
 *
 *  3. Otherwise it is 0.
 *
 * For example, given `M(,)`, the length of \a arg_list is 2 and the number of
 * actual arguments _must also_ be 2 because the presence of the `,` compels it
 * to be. Whether or not the arguments have any tokens is irrelevant.
 *
 * However, given `M( )`, this _could_ have originally been `M(X)` and `X`
 * expanded to nothing --- so _now_ there's no actual argument.
 * @endparblock
 *
 * @param arg_list The \ref p_arg_list_t to count.
 * @return Returns said number.
 */
NODISCARD
static size_t p_arg_list_count( p_arg_list_t const *arg_list ) {
  assert( arg_list != NULL );

  size_t const n_args = slist_len( arg_list );
  if ( n_args != 1 )
    return n_args;

  p_token_list_t *const arg_tokens = slist_front( arg_list );
  return slist_empty( arg_tokens ) ? 0 : 1;
}

/**
 * Compares two \ref p_macro objects.
 *
 * @param i_macro A pointer to the first \a ref p_macro.
 * @param j_macro A pointer to the second \a ref p_macro.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the macro name pointed to by \a i_macro is less than, equal to,
 * or greater than the macro name pointed to by \a j_macro.
 */
NODISCARD
static int p_macro_cmp( p_macro_t const *i_macro, p_macro_t const *j_macro ) {
  assert( i_macro != NULL );
  assert( j_macro != NULL );
  return strcmp( i_macro->name, j_macro->name );
}

/**
 * Finds a parameter of \a macro having \a name, if any.
 *
 * @param macro The \ref p_macro to check.
 * @param name The name of the parameter to find.
 * @return Returns the zero-based index of the parameter having \a name or
 * #NO_PARAM if none.
 */
NODISCARD
static size_t p_macro_find_param( p_macro_t const *macro, char const *name ) {
  assert( macro != NULL );
  assert( p_macro_is_func_like( macro ) );
  assert( name != NULL );

  size_t param_index = 0;
  FOREACH_SLIST_NODE( param_node, macro->param_list ) {
    p_param_t const *const param = param_node->data;
    if ( strcmp( name, param->name ) == 0 )
      return param_index;
    ++param_index;
  } // for

  return NO_PARAM;
}

/**
 * Frees all memory used by \a macro _including_ \a macro itself.
 *
 * @param macro The \ref p_macro to free.  If NULL, does nothing.
 *
 * @sa p_macro_define()
 */
static void p_macro_free( p_macro_t *macro ) {
  if ( macro == NULL )
    return;                             // LCOV_EXCL_LINE
  if ( !macro->is_dynamic ) {
    p_param_list_cleanup( macro->param_list );
    free( macro->param_list );
    p_token_list_cleanup( &macro->replace_list );
  }
  FREE( macro->name );
  free( macro );
}

/**
 * Checks whether \a macro's last parameter is `...`.
 *
 * @param macro The \ref p_macro to check.
 * @return Returns `true` only if \a macro's last parameter is `...`.
 */
NODISCARD
static bool p_macro_is_variadic( p_macro_t const *macro ) {
  assert( macro != NULL );
  if ( !p_macro_is_func_like( macro ) )
    return false;
  p_param_t const *const last_param = slist_back( macro->param_list );
  return last_param != NULL && last_param->name[0] == '.';
}

/**
 * Checks the parameters, if any, of \a macro just prior to expansion.
 *
 * @param macro The \ref p_macro to check the parameters of.
 * @return Returns `true` only if all checks passed.
 */
static bool p_macro_check_params( p_macro_t const *macro ) {
  assert( macro != NULL );

  if ( !p_macro_is_func_like( macro ) || OPT_LANG_IS( VARIADIC_MACROS ) )
    return true;
  p_param_t const *const last_param = slist_back( macro->param_list );
  if ( last_param == NULL || last_param->name[0] != '.' )
    return true;

  show_macro( macro, stderr );
  print_error( &last_param->loc,
    "variadic macros not supported%s\n",
    C_LANG_WHICH( VARIADIC_MACROS )
  );
  return false;
}

/**
 * Adjusts the \ref c_loc::first_column "first_column" and \ref
 * c_loc::last_column "last_column" of \ref p_param::loc "loc" for every
 * parameter of \a macro.
 *
 * @param macro The \ref p_macro whose parameters to relocate.
 *
 * @note The column caclulations _must_ match how show_macro() prints.
 */
static void p_macro_relocate_params( p_macro_t *macro ) {
  assert( macro != NULL );
  assert( p_macro_is_func_like( macro ) );

  size_t column = STRLITLEN( "#define " ) + strlen( macro->name ) + 1/*'('*/;

  bool comma = false;
  FOREACH_SLIST_NODE( param_node, macro->param_list ) {
    p_param_t *const param = param_node->data;
    if ( true_or_set( &comma ) )
      column += 2;                      // ", "
    param->loc.first_column = STATIC_CAST( c_loc_num_t, column );
    column += strlen( param->name );
    param->loc.last_column = STATIC_CAST( c_loc_num_t, column - 1 );
  } // for
}

/**
 * Duplicates \a token.
 *
 * @param token The p_token to duplicate; may be NULL.
 * @return Returns the duplicated token or NULL only if \a token is NULL.
 */
NODISCARD
static p_token_t* p_token_dup( p_token_t const *token ) {
  if ( token == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  p_token_t *const dup_token = MALLOC( p_token_t, 1 );
  dup_token->kind = token->kind;
  dup_token->loc = token->loc;
  dup_token->is_substituted = token->is_substituted;
  switch ( token->kind ) {
    case P_CHAR_LIT:
    case P_NUM_LIT:
    case P_STR_LIT:
      dup_token->lit.value = check_strdup( token->lit.value );
      break;
    case P_IDENTIFIER:
      dup_token->ident.ineligible = token->ident.ineligible;
      dup_token->ident.name = check_strdup( token->ident.name );
      break;
    case P_OTHER:
      dup_token->other.value = token->other.value;
      break;
    case P_PUNCTUATOR:
      strcpy( dup_token->punct.value, token->punct.value );
      break;
    case P_CONCAT:
    case P_PLACEMARKER:
    case P_SPACE:
    case P_STRINGIFY:
    case P___VA_ARGS__:
    case P___VA_OPT__:
      // nothing to do
      break;
  } // switch
  return dup_token;
}

/**
 * A predicate function for slist_free_if() that checks whether \a token_node
 * is a #P_SPACE token and precedes another: if so, frees it.
 *
 * @param token_node A pointer to the \ref p_token to possibly free.
 * @param user_data Not used.
 * @return Returns `true` only if \a token_node was freed.
 */
static bool p_token_free_if_consec_space( p_token_node_t *token_node,
                                          user_data_t user_data ) {
  assert( token_node != NULL );
  (void)user_data;
  p_token_t *const token = token_node->data;
  assert( token != NULL );

  if ( token->kind != P_SPACE )
    return false;

  p_token_node_t const *const next_node =
    p_token_node_not( token_node->next, P_PLACEMARKER );
  if ( p_token_node_is_any( next_node, P_ANY_OPAQUE ) )
    return false;

  p_token_free( token );
  return true;
}

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
static size_t p_token_list_relocate( p_token_list_t *token_list,
                                     size_t first_column ) {
  assert( token_list != NULL );

  bool relocated_opaque = false;

  FOREACH_SLIST_NODE( token_node, token_list ) {
    p_token_t *const token = token_node->data;
    switch ( token->kind ) {
      case P_PLACEMARKER:
        continue;
      case P_SPACE:
        if ( !relocated_opaque )
          continue;                     // don't do leading spaces
        if ( p_token_node_emptyish( token_node->next ) )
          goto done;                    // don't do trailing spaces either
        FALLTHROUGH;
      default:
        token->loc.first_column = STATIC_CAST( c_loc_num_t, first_column );
        first_column += strlen( p_token_str( token ) );
        token->loc.last_column = STATIC_CAST( c_loc_num_t, first_column - 1 );
        relocated_opaque = true;
        break;
    } // switch
  } // for

done:
  return first_column;
}

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
static char const* p_token_list_str( p_token_list_t const *token_list ) {
  assert( token_list != NULL );

  static strbuf_t sbuf;
  strbuf_reset( &sbuf );

  bool stringified_opaque = false;

  FOREACH_SLIST_NODE( token_node, token_list ) {
    p_token_t const *const token = token_node->data;
    switch ( token->kind ) {
      case P_PLACEMARKER:
        continue;
      case P_SPACE:
        if ( !stringified_opaque )
          continue;                     // don't do leading spaces
        if ( p_token_node_emptyish( token_node->next ) )
          goto done;                    // don't do trailing spaces either
        FALLTHROUGH;
      default:
        strbuf_puts( &sbuf, p_token_str( token ) );
        stringified_opaque = true;
        break;
    } // switch
  } // for

done:
  return empty_if_null( sbuf.str );
}

/**
 * Gets the first node for a token whose \ref p_token::kind "kind" is _not_ one
 * of \a kinds.
 *
 * @param token_node The node to start from.
 * @param kinds The bitwise-or of kind(s) _not_ to get.
 * @return Returns said node or NULL if no such node exists.
 */
NODISCARD
static p_token_node_t* p_token_node_not( p_token_node_t *token_node,
                                         p_token_kind_t kinds ) {
  for ( ; token_node != NULL; token_node = token_node->next ) {
    p_token_t const *const token = token_node->data;
    if ( (token->kind & kinds) == 0 )
      break;
  } // for
  return token_node;
}

/**
 * Comparison function for two \ref param_expand.
 *
 * @param i_pe The first \ref param_expand.
 * @param j_pe The second \ref param_expand.
 * @return Returns an integer less than, equal to, or greater than 0, according
 * to whether the \ref param_expand::name "name" pointed to by \a i_pe is less
 * than, equal to, or greater than the \ref param_expand::name "name" name
 * pointed to by \a j_pe.
 */
static int param_expand_cmp( param_expand_t const *i_pe,
                             param_expand_t const *j_pe ) {
  return strcmp( i_pe->name, j_pe->name );
}

/**
 * Parses macro arguments between `(` and `)` tokens and appends them onto \a
 * arg_list.
 *
 * @param token_node The node whose data is the \ref p_token of the `(`.
 * @param arg_list The list to append arguments onto.  If NULL, only a parse is
 * done: no tokens are appended.
 * @return Returns the node whose data is the \ref p_token of the `)` or NULL
 * upon error.
 */
NODISCARD
static p_token_node_t* parse_args( p_token_node_t const *token_node,
                                   p_arg_list_t *arg_list ) {
  assert( p_token_node_is_punct( token_node, '(' ) );

  p_token_list_t *arg_tokens = NULL;
  if ( arg_list != NULL ) {
    arg_tokens = MALLOC( p_token_list_t, 1 );
    slist_init( arg_tokens );
  }

  p_token_t *token = token_node->data;
  for ( unsigned paren_count = 1; paren_count > 0; ) {
    token_node = token_node->next;
    if ( token_node == NULL ) {
      print_error( &token->loc, "unterminated function-like macro\n" );
      p_token_list_cleanup( arg_tokens );
      free( arg_tokens );
      return NULL;
    }
    token = token_node->data;
    if ( p_token_is_any_char( token ) ) {
      switch ( token->punct.value[0] ) {
        case '(':
          ++paren_count;
          break;
        case ')':
          if ( --paren_count == 0 ) {
            if ( arg_list != NULL )
              slist_push_back( arg_list, arg_tokens );
            continue;
          }
          break;
        case ',':
          if ( paren_count == 1 ) {     // separates arguments
            if ( arg_list != NULL ) {
              slist_push_back( arg_list, arg_tokens );
              arg_tokens = MALLOC( p_token_list_t, 1 );
              slist_init( arg_tokens );
            }
            continue;
          }
          break;
      } // switch
    }
    if ( arg_tokens != NULL )
      push_back_dup_token( arg_tokens, token );
  } // for

  return CONST_CAST( p_token_node_t*, token_node );
}

/**
 * Predefines a macro.
 *
 * @param name The name of the macro to predefine.
 * @param dyn_fn A \ref p_macro_dyn_fn_t.
 */
static void predefine_macro( char const *name, p_macro_dyn_fn_t dyn_fn ) {
  assert( name != NULL );
  assert( dyn_fn != NULL );

  p_macro_t *const macro = p_macro_define(
    check_strdup( name ),
    &C_LOC_COL( STRLITLEN( "#define " ) ),
    /*param_list=*/NULL,
    /*replace_list=*/NULL
  );
  assert( macro != NULL );

  macro->dyn_fn = dyn_fn;
  macro->is_dynamic = true;
}

/**
 * Possibly appends a duplicate of \a token onto the end of \a token_list;
 * however, if \a token is #P_SPACE and \a token_list is either empty or its
 * last token is #P_SPACE, does nothing.
 *
 * @param token_list The \ref p_token_list_t to append onto.
 * @param token The \ref p_token to possibly duplicate.
 */
static void push_back_dup_token( p_token_list_t *token_list,
                                 p_token_t const *token ) {
  avoid_paste( token_list, token );
  slist_push_back( token_list, p_token_dup( token ) );
}

/**
 * Appends the tokens comprising \a src_list onto the end of \a dst_list.
 *
 * @param dst_list The \ref p_token_list_t to append onto.
 * @param src_list The list of tokens to append.
 * @return Returns the node pointing to the first duplicated token, if any.
 */
PJL_DISCARD
static p_token_node_t* push_back_dup_tokens( p_token_list_t *dst_list,
                                             p_token_list_t const *src_list ) {
  assert( dst_list != NULL );
  assert( src_list != NULL );

  p_token_node_t *const dst_tail_orig = dst_list->tail;

  FOREACH_SLIST_NODE( src_node, src_list )
    push_back_dup_token( dst_list, src_node->data );

  return dst_tail_orig != NULL ? dst_tail_orig->next : dst_list->head;
}

/**
 * Convenience function to set \a token's \ref p_token::is_substituted
 * "is_substituted" then push it onto \a token_list.
 *
 * @param token_list The p_token_list to push onto.
 * @param token The \ref p_token to push back.
 */
static void push_back_substituted_token( p_token_list_t *token_list,
                                         p_token_t *token ) {
  avoid_paste( token_list, token );
  token->is_substituted = true;
  slist_push_back( token_list, token );
}

/**
 * Red-black tree visitor function that forwards to the \ref p_macro_visit_fn_t
 * function.
 *
 * @param node_data A pointer to the node's data.
 * @param v_data Data passed to to the visitor.
 * @return Returning `true` will cause traversal to stop and the current node
 * to be returned to the caller of rb_tree_visit().
 */
NODISCARD
static bool rb_visitor( void *node_data, void *v_data ) {
  assert( node_data != NULL );
  assert( v_data != NULL );

  p_macro_t const *const macro = node_data;
  macro_rb_visit_data_t const *const mrvd = v_data;

  return (*mrvd->visit_fn)( macro, mrvd->v_data );
}

/**
 * Sets \ref p_token::is_substituted "is_substituted" of all the tokens
 * starting with \a token_node to `true`.
 *
 * @param token_node The first \ref p_token_node_t to start at.
 */
static void set_substituted( p_token_node_t *token_node ) {
  for ( ; token_node != NULL; token_node = token_node->next ) {
    p_token_t *const token = token_node->data;
    token->is_substituted = true;
  } // for
}

/**
 * Trims leading #P_SPACE tokens from the first argument's tokens and trailing
 * #P_SPACE tokens from the last argument's tokens.
 *
 * @param arg_list The macro argument list to trim.
 *
 * @sa trim_tokens()
 */
static void trim_args( p_arg_list_t *arg_list ) {
  p_token_list_t *const first_arg_tokens = slist_front( arg_list );
  if ( first_arg_tokens != NULL ) {
    while ( !slist_empty( first_arg_tokens ) ) {
      p_token_t *const token = slist_front( first_arg_tokens );
      if ( token->kind != P_SPACE )
        break;
      p_token_free( slist_pop_front( first_arg_tokens ) );
    } // while
  }

  p_token_list_t *const last_arg_tokens = slist_back( arg_list );
  if ( last_arg_tokens != NULL ) {
    while ( !slist_empty( last_arg_tokens ) ) {
      p_token_t *const token = slist_back( last_arg_tokens );
      if ( token->kind != P_SPACE )
        break;
      p_token_free( slist_pop_back( last_arg_tokens ) );
    } // while
  }
}

/**
 * Trims both leading and trailing #P_SPACE tokens from \a token_list as well
 * as squashes multiple consecutive intervening #P_SPACE to a single #P_SPACE
 * within \a token_list.
 *
 * @param token_list The list of \ref p_token to trim.
 *
 * @sa trim_args()
 */
static void trim_tokens( p_token_list_t *token_list ) {
  assert( token_list != NULL );

  while ( !slist_empty( token_list ) ) {
    p_token_t *const token = slist_front( token_list );
    if ( token->kind != P_SPACE )
      break;
    p_token_free( slist_pop_front( token_list ) );
  } // while

  while ( !slist_empty( token_list ) ) {
    p_token_t *const token = slist_back( token_list );
    if ( token->kind != P_SPACE )
      break;
    p_token_free( slist_pop_back( token_list ) );
  } // while

  slist_free_if( token_list, &p_token_free_if_consec_space, USER_DATA_ZERO );
}

////////// extern functions ///////////////////////////////////////////////////

void p_arg_list_cleanup( p_arg_list_t *arg_list ) {
  if ( arg_list == NULL )
    return;                             // LCOV_EXCL_LINE
  FOREACH_SLIST_NODE( arg_node, arg_list ) {
    p_token_list_t *const arg_tokens = arg_node->data;
    p_token_list_cleanup( arg_tokens );
    free( arg_tokens );
  } // for
}

char const* p_kind_name( p_token_kind_t kind ) {
  switch ( kind ) {
    case P_CHAR_LIT   : return "char_lit";
    case P_CONCAT     : return "##";
    case P_IDENTIFIER : return "identifier";
    case P_NUM_LIT    : return "num_lit";
    case P_OTHER      : return "other";
    case P_PLACEMARKER: return "placemarker";
    case P_PUNCTUATOR : return "punctuator";
    case P_SPACE      : return " ";
    case P_STRINGIFY  : return "#";
    case P_STR_LIT    : return "str_lit";
    case P___VA_ARGS__: return L___VA_ARGS__;
    case P___VA_OPT__ : return L___VA_OPT__;
  } // switch
  UNEXPECTED_INT_VALUE( kind );
}

p_macro_t* p_macro_define( char *name, c_loc_t const *name_loc,
                           p_param_list_t *param_list,
                           p_token_list_t *replace_list ) {
  assert( name != NULL );
  assert( name_loc != NULL );

  if ( is_predefined_macro_name( name ) ) {
    print_error( name_loc,
      "\"%s\": predefined macro may not be redefined\n", name
    );
    goto error;
  }

  if ( param_list != NULL && !check_macro_params( param_list ) )
    goto error;

  p_macro_t *const new_macro = MALLOC( p_macro_t, 1 );
  *new_macro = (p_macro_t){
    .name = name,
    .replace_list = slist_move( replace_list )
  };
  if ( param_list != NULL ) {
    new_macro->param_list = MALLOC( p_param_list_t, 1 );
    *new_macro->param_list = slist_move( param_list );
  }

  trim_tokens( &new_macro->replace_list );

  mex_state_t check_mex;
  mex_init( &check_mex,
    /*parent_mex=*/NULL,
    new_macro,
    /*arg_list=*/NULL,
    &new_macro->replace_list,
    stdout
  );

  bool const ok = mex_check( &check_mex );
  mex_cleanup( &check_mex );
  if ( !ok ) {
    p_macro_free( new_macro );
    return NULL;
  }

  if ( p_macro_is_func_like( new_macro ) )
    p_macro_relocate_params( new_macro );

  rb_insert_rv_t const rbi = rb_tree_insert( &macro_set, new_macro );
  if ( !rbi.inserted ) {
    p_macro_t *const old_macro = rbi.node->data;
    assert( !old_macro->is_dynamic );
    p_macro_free( old_macro );
    rbi.node->data = new_macro;
    print_warning( name_loc, "\"%s\" already exists; redefined\n", name );
  }

  return new_macro;

error:
  FREE( name );
  return NULL;
}

bool p_macro_expand( char const *name, c_loc_t const *name_loc,
                     p_arg_list_t *arg_list, p_token_list_t *extra_list,
                     FILE *fout ) {
  assert( name != NULL );
  assert( name_loc != NULL );
  assert( fout != NULL );

  p_macro_t const *const macro = p_macro_find( name );
  if ( macro == NULL ) {
    print_error( name_loc, "\"%s\": no such macro\n", name );
    return false;
  }

  //
  // Ordinarily, print_error() and print_warning() given a location will
  // print the line containing the error then print the ^ under that.  We
  // need to suppress printing the line because lines containing errors or
  // warnings are from macros previously #define'd, not lines the user just
  // typed; so we have to print the ^ relative to macro expansion lines we've
  // been printing.
  //
  bool const opt_no_print_input_line_orig =
    print_params.opt_no_print_input_line;

  mex_state_t mex;
  mex_init( &mex,
    /*parent_mex=*/NULL,
    macro,
    arg_list,
    &macro->replace_list,
    fout
  );
  mex.name_loc = *name_loc;

  bool ok = false;

  if ( !mex_prep_args( &mex ) || !mex_preliminary_check( &mex ) )
    goto error;

  //
  // For non-dynamic macros, we have to relocate the tokens comprising the
  // replacement list.  The correct locations are based on the supplied
  // arguments, so they'll always be different for each expansion.
  //
  mex_preliminary_relocate_replace_list( &mex );

  //
  // Instruct print_error() and print_warning() _not_ to print the input line
  // before printing '^' since we need to print the macro expansion lines
  // ourselves.
  //
  print_params.opt_no_print_input_line = true;

  //
  // We need a dummy token to pass to p_expand() initially.
  //
  p_token_t token = {
    .kind = P_IDENTIFIER,
    .loc = *name_loc,
    .ident = { .name = macro->name }
  };

  //
  // Do the primary expansion.
  //
  if ( mex_expand( &mex, &token ) == MEX_ERROR )
    goto error;

  if ( extra_list != NULL && !p_token_list_emptyish( extra_list ) ) {
    //
    // There were extra tokens given at the end of the "expand" command: append
    // them and do another expansion pass.
    //
    slist_push_list_back( mex.expand_list, extra_list );
    mex_relocate_expand_list( &mex );
    mex_swap_lists( &mex );
    if ( mex_expand( &mex, &token ) == MEX_ERROR )
      goto error;
  }

  ok = true;

error:
  print_params.opt_no_print_input_line = opt_no_print_input_line_orig;
  mex_cleanup( &mex );
  return ok;
}

p_macro_t const* p_macro_find( char const *name ) {
  assert( name != NULL );
  p_macro_t const find_macro = { .name = name };
  rb_node_t const *const found_rb = rb_tree_find( &macro_set, &find_macro );
  return found_rb != NULL ? found_rb->data : NULL;
}

bool p_macro_undef( char const *name, c_loc_t const *name_loc ) {
  assert( name != NULL );
  assert( name_loc != NULL );

  p_macro_t const find_macro = { .name = name };
  rb_node_t *const found_rb = rb_tree_find( &macro_set, &find_macro );
  if ( found_rb == NULL ) {
    print_error( name_loc, "\"%s\": no such macro\n", name );
    return false;
  }
  p_macro_t *const macro = found_rb->data;
  if ( macro->is_dynamic ) {
    print_error( name_loc,
      "\"%s\": predefined macro may not be undefined\n", name
    );
    return false;
  }

  p_macro_free( rb_tree_delete( &macro_set, found_rb ) );
  return true;
}

p_macro_t const* p_macro_visit( p_macro_visit_fn_t visit_fn, void *v_data ) {
  assert( visit_fn != NULL );
  macro_rb_visit_data_t mrvd = { visit_fn, v_data };
  rb_node_t const *const rb = rb_tree_visit( &macro_set, &rb_visitor, &mrvd );
  return rb != NULL ? rb->data : NULL;
}

void p_macros_init( void ) {
  ASSERT_RUN_ONCE();

  rb_tree_init( &macro_set, POINTER_CAST( rb_cmp_fn_t, &p_macro_cmp ) );
  ATEXIT( &macros_cleanup );

  predefine_macro( "__cplusplus",       &macro_dyn___cplusplus      );
  predefine_macro( "__DATE__",          &macro_dyn___DATE__         );
  predefine_macro( "__FILE__",          &macro_dyn___FILE__         );
  predefine_macro( "__LINE__",          &macro_dyn___LINE__         );
  predefine_macro( "__STDC__",          &macro_dyn___STDC__         );
  predefine_macro( "__STDC_VERSION__",  &macro_dyn___STDC_VERSION__ );
  predefine_macro( "__TIME__",          &macro_dyn___TIME__         );
}

void p_param_free( p_param_t *param ) {
  if ( param == NULL )
    return;                             // LCOV_EXCL_LINE
  FREE( param->name );
  free( param );
}

void p_param_list_cleanup( p_param_list_t *list ) {
  slist_cleanup( list, POINTER_CAST( slist_free_fn_t, &p_param_free ) );
}

void p_token_free( p_token_t *token ) {
  if ( token == NULL )
    return;                             // LCOV_EXCL_LINE
  switch ( token->kind ) {
    case P_CHAR_LIT:
    case P_NUM_LIT:
    case P_STR_LIT:
      FREE( token->lit.value );
      break;
    case P_IDENTIFIER:
      FREE( token->ident.name );
      break;
    case P_CONCAT:
    case P_OTHER:
    case P_PLACEMARKER:
    case P_PUNCTUATOR:
    case P_SPACE:
    case P_STRINGIFY:
    case P___VA_ARGS__:
    case P___VA_OPT__:
      // nothing to do
      break;
  } // switch
  free( token );
}

void p_token_list_cleanup( p_token_list_t *list ) {
  slist_cleanup( list, POINTER_CAST( slist_free_fn_t, &p_token_free ) );
}

p_token_t* p_token_new_loc( p_token_kind_t kind, c_loc_t const *loc,
                            char const *literal ) {
  p_token_t *const token = MALLOC( p_token_t, 1 );
  *token = (p_token_t){ .kind = kind };
  if ( loc != NULL )
    token->loc = *loc;
  switch ( kind ) {
    case P_CHAR_LIT:
    case P_NUM_LIT:
    case P_STR_LIT:
      assert( literal != NULL );
      token->lit.value = literal;
      break;
    case P_IDENTIFIER:
      assert( literal != NULL );
      token->ident.name = literal;
      break;
    case P_OTHER:
      assert( literal != NULL );
      assert( literal[0] != '\0' );
      assert( literal[1] == '\0' );
      token->other.value = literal[0];
      break;
    case P_PUNCTUATOR:
      assert( literal != NULL );
      assert( literal[0] != '\0' );
      assert( literal[1] == '\0' || literal[2] == '\0' || literal[3] == '\0' );
      strcpy( token->punct.value, literal );
      break;
    case P_CONCAT:
    case P_PLACEMARKER:
    case P_SPACE:
    case P_STRINGIFY:
    case P___VA_ARGS__:
    case P___VA_OPT__:
      assert( literal == NULL );
      break;
  } // switch
  return token;
}

char const* p_token_str( p_token_t const *token ) {
  assert( token != NULL );

  static char other_str[2];
  static strbuf_t sbuf;

  switch ( token->kind ) {
    case P_CHAR_LIT:
      strbuf_reset( &sbuf );
      strbuf_puts_quoted( &sbuf, '\'', token->lit.value );
      return sbuf.str;
    case P_CONCAT:
      return "##";
    case P_IDENTIFIER:
      return token->ident.name;
    case P_NUM_LIT:
      return token->lit.value;
    case P_OTHER:
      other_str[0] = token->other.value;
      return other_str;
    case P_PLACEMARKER:
      return "";
    case P_PUNCTUATOR:
      return token->punct.value;
    case P_SPACE:
      return " ";
    case P_STRINGIFY:
      return "#";
    case P_STR_LIT:
      strbuf_reset( &sbuf );
      strbuf_puts_quoted( &sbuf, '"', token->lit.value );
      return sbuf.str;
    case P___VA_ARGS__:
      return L___VA_ARGS__;
    case P___VA_OPT__:
      return L___VA_OPT__;
  } // switch

  UNEXPECTED_INT_VALUE( token->kind );
}

void print_token_list( p_token_list_t const *token_list, FILE *fout ) {
  assert( token_list != NULL );
  assert( fout != NULL );

  bool printed_opaque = false;

  FOREACH_SLIST_NODE( token_node, token_list ) {
    p_token_t const *const token = token_node->data;
    switch ( token->kind ) {
      case P_PLACEMARKER:
        continue;
      case P_SPACE:
        if ( !printed_opaque )
          continue;                     // don't print leading spaces
        if ( p_token_node_emptyish( token_node->next ) )
          return;                       // don't print trailing spaces either
        break;
      default:
        break;
    } // switch

    FPUTS( p_token_str( token ), fout );
    printed_opaque = true;
  } // for
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
