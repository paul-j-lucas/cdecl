/*
**      cdecl -- C gibberish translator
**      src/p_macro.c
**
**      Copyright (C) 2023-2025  Paul J. Lucas
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
#include "p_macro.h"
#include "cdecl.h"
#include "c_lang.h"
#include "color.h"
#include "dump.h"
#include "gibberish.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "p_token.h"
#include "print.h"
#include "red_black.h"
#include "show.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// @endcond

/**
 * @addtogroup p-macro-group
 * @{
 */

/**
 * Ends a dump block.
 *
 * @sa #DUMP_START()
 */
#define DUMP_END() \
  FPRINTF( dump.fout, "\n%*s}\n", STATIC_CAST( int, dump.indent * CDECL_DUMP_INDENT ), "" )

/**
 * Possibly dumps a comma and a newline followed by the `printf()` arguments
 * --- used for printing a key followed by a value.
 *
 * @param ... The `printf()` arguments.
 */
#define DUMP_KEY(...) BLOCK(                  \
  fput_sep( ",\n", &dump.comma, dump.fout );  \
  DUMP_PRINTF( "  " __VA_ARGS__ ); )

/**
 * Indents the current number of spaces and prints the given arguments.
 *
 * @param ... The `printf()` arguments.
 */
#define DUMP_PRINTF(...) BLOCK(                           \
  FPUTNSP( dump.indent * CDECL_DUMP_INDENT, dump.fout );  \
  FPRINTF( dump.fout, __VA_ARGS__ ); )

/**
 * Starts a dump block.
 *
 * @param INDENT The indentation to use.
 * @param FOUT The `FILE` to dump to.
 *
 * @note The dump block _must_ end with #DUMP_END().
 *
 * @sa #DUMP_END()
 */
#define DUMP_START(INDENT,FOUT)         \
  dump_state_t dump;                    \
  dump_init( &dump, (INDENT), (FOUT) ); \
  DUMP_PRINTF( "{\n" )

/**
 * Dumps a C string.
 *
 * @param KEY The key name to print.
 * @param STR The C string to dump.
 */
#define DUMP_STR(KEY,STR) BLOCK( \
  DUMP_KEY( KEY ": " ); fputs_quoted( (STR), '"', dump.fout ); )

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
 * Initializes \a MEX to have the \ref mex_state::replace_list "replace_list"
 * of \a PARENT_MEX's \ref mex_state::va_args_token_list.
 *
 * @note This is a macro instead of a function so the compound literal used to
 * initialize \ref mex_state::macro "macro" remains in scope.
 *
 * @param MEX The mex_state to initialize.
 * @param PARENT_MEX The parent mex_state to use.
 *
 * @sa mex_init()
 */
#define VA_ARGS_MEX_INIT(MEX,PARENT_MEX)                \
  mex_init( (MEX),                                      \
    (PARENT_MEX),                                       \
    &(p_macro_t){ .name = L_PRE___VA_ARGS__ },          \
    &(PARENT_MEX)->name_loc,                            \
    /*arg_list=*/NULL,                                  \
    /*replace_list=*/&(PARENT_MEX)->va_args_token_list, \
    (PARENT_MEX)->fout                                  \
  )

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
   * Macro was _not_ expanded because:
   *
   *  1. A token can not be expanded, so treat it as ordinary text; or:
   *  2. An entire expansion pass can not be performed, so skip it; or if it
   *     already started, abort it and pretend it didn't happen; or:
   *  3. An expansion pass was completed, but no tokens from from \ref
   *     mex_state::replace_list "replace_list" were expanded onto \ref
   *     mex_state::expand_list "expand_list", i.e., the latter is an exact
   *     copy of the former.
   */
  MEX_NOT_EXPANDED,

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
  void               *visit_data;       ///< Caller's optional data.
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

  /**
   * Macros undergoing expansion.
   *
   * @sa mex_expanding_set_key()
   */
  rb_tree_t            *expanding_set;

  /**
   * The set of macros that won't expand we've warned about that is one of:
   *
   *  + Dynamic and not supported in the current language; or:
   *  + A function-like macro not followed by `(`.
   *
   * @sa mex_no_expand_set_key()
   */
  rb_tree_t            *no_expand_set;

  /**
   * Substituted, but not expanded, `__VA_ARGS__` tokens.
   *
   * @sa mex_init_va_args_token_list()
   */
  p_token_list_t        va_args_token_list;

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
   * When set, tokens are _not_ trimmed via p_token_list_trim() after each
   * expansion function is called.
   *
   * @remarks This is set _only_ when appending "arguments" via
   * mex_append_args() to the expansion of a non-function-like macro so they
   * are appended verbatim.
   */
  bool                  expand_opt_no_trim_tokens;

  /**
   * When set, \ref arg_list is _neither_ printed via mex_print_macro() _nor_
   * used in token location calculations via mex_relocate_expand_list().
   *
   * @remarks This is set _only_ when expanding #P___VA_OPT__ tokens since it
   * needs access to \ref arg_list, but they should _not_ be printed.
   */
  bool                  print_opt_omit_args;

  /**
   * Flag to keep track of whether \ref va_args_token_list has been printed.
   */
  bool                  printed___VA_ARGS__;

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
static bool             mex_check( mex_state_t *mex ),
                        mex_check_concat( mex_state_t*,
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

static void             mex_check_identifier( mex_state_t*,
                                              p_token_node_t const* );
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
                        mex_expand_identifier( mex_state_t*, p_token_node_t** );

NODISCARD
static bool             mex_expand_stringify( mex_state_t*, p_token_node_t** ),
                        mex_expand___VA_ARGS__( mex_state_t*, p_token_list_t*,
                                                p_token_node_t const*,
                                                p_token_node_t const* );

NODISCARD
static p_token_node_t*  mex_expand___VA_OPT__( mex_state_t*,
                                               p_token_node_t const*,
                                               p_token_list_t* );

NODISCARD
static strbuf_t const*  mex_expanding_set_key( mex_state_t const* );

static void             mex_init( mex_state_t*, mex_state_t*, p_macro_t const*,
                                  c_loc_t const*, p_arg_list_t*,
                                  p_token_list_t const*, FILE* );
static void             mex_init_va_args_token_list( mex_state_t* );

NODISCARD
static strbuf_t const*  mex_no_expand_set_key( mex_state_t const*,
                                               p_macro_t const* );

NODISCARD
static p_token_list_t*  mex_param_arg( mex_state_t const*, char const* );

NODISCARD
static bool             mex_prep_args( mex_state_t* );

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

static void             p_arg_list_trim( p_arg_list_t* );
static void             p_macro_cleanup( p_macro_t* );

NODISCARD
static bool             p_macro_is_variadic( p_macro_t const* ),
                        p_macro_check_params( p_macro_t const* );

NODISCARD
static int              param_expand_cmp( param_expand_t const*,
                                          param_expand_t const* );

NODISCARD
static p_token_node_t*  parse_args( p_token_node_t*, p_arg_list_t* );

PJL_DISCARD
static p_token_node_t*  push_back_dup_tokens( p_token_list_t*,
                                              p_token_list_t const* );

static void             set_substituted( p_token_node_t* );
static void             va_args_mex_print_macro( mex_state_t* );

// local constants
static char const ARROW[] = "=>";       ///< Separates macro name from tokens.

// local variables
static rb_tree_t  macro_set;            ///< Global set of macros.

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether \a name is either `__VA_ARGS__` or `__VA_OPT__`.
 *
 * @param name The name to check.
 * @return Returns `true` only if it is.
 *
 * @sa macro_name_check()
 * @sa macro_name_is_predefined()
 */
NODISCARD
static inline bool macro_name_is__VA_( char const *name ) {
  return  strcmp( name, L_PRE___VA_ARGS__ ) == 0 ||
          strcmp( name, L_PRE___VA_OPT__  ) == 0;
}

////////// local functions ////////////////////////////////////////////////////

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
    if ( strcmp( param->name, L_ELLIPSIS ) == 0 ) {
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
 * Checks \a name for validity.
 *
 * @param name The name of the macro to check.
 * @param name_loc The source location of \a name.
 * @return Returns `true` only if all checks passed.
 *
 * @sa macro_name_is_predefined()
 * @sa macro_name_is__VA_()
 */
NODISCARD
static bool macro_name_check( char const *name, c_loc_t const *name_loc ) {
  assert( name != NULL );
  assert( name_loc != NULL );

  if ( !cdecl_is_initialized )
    return true;

  if ( macro_name_is_predefined( name ) ) {
    print_error( name_loc,
      "\"%s\": predefined macro may not be redefined\n", name
    );
    return false;
  }

  if ( OPT_LANG_IS( C_ANY ) && str_is_prefix( "__STDC_", name ) ) {
    print_warning( name_loc,
      "\"%s\": macro names beginning with \"__STDC_\" are reserved\n", name
    );
  }

  return true;
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

  unsigned arg_index = 0;
  FOREACH_SLIST_NODE( arg_node, mex->arg_list ) {
    char arg_name[ ARRAY_SIZE( "arg_NNN" ) ];
    check_snprintf( arg_name, sizeof arg_name, "arg_%u", ++arg_index );

    mex_state_t arg_mex;
    mex_init( &arg_mex,
      /*parent_mex=*/mex,
      &(p_macro_t){ .name = arg_name },
      &mex->name_loc,
      /*arg_list=*/NULL,
      /*replace_list=*/arg_node->data,
      mex->fout
    );
    arg_mex.expand_opt_no_trim_tokens = true;

    static mex_expand_all_fn_t const EXPAND_FNS[] = {
      // We need only mex_expand_all_macros() that mex_expand_all_fns() does
      // implicitly.
      NULL
    };

    mex_print_macro( &arg_mex, arg_mex.replace_list );
    bool const ok = mex_expand_all_fns( &arg_mex, EXPAND_FNS );
    if ( ok ) {
      push_back_dup_tokens( mex->expand_list, arg_mex.expand_list );
      if ( arg_node->next != NULL )
        slist_push_back( mex->expand_list, p_token_new( P_PUNCTUATOR, "," ) );
    }
    mex_cleanup( &arg_mex );
    if ( !ok )
      return false;
  } // for

  slist_push_back( mex->expand_list, p_token_new( P_PUNCTUATOR, ")" ) );
  return true;
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
        mex_check_identifier( mex, token_node );
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
      "\"%s\" not supported%s; treated as text\n",
      other_token_c( "##" ), C_LANG_WHICH( P_CONCAT )
    );
  }

  if ( token_node == p_token_node_not( mex->replace_list->head, P_SPACE ) ) {
    print_error( &concat_token->loc,
      "\"%s\" can not be first\n", other_token_c( "##" )
    );
    return false;
  }

  token_node = p_token_node_not( token_node->next, P_SPACE );
  if ( token_node == NULL ) {
    print_error( &concat_token->loc,
      "\"%s\" can not be last\n", other_token_c( "##" )
    );
    return false;
  }

  return true;
}

/**
 * Checks a #P_IDENTIFIER macro for semantic warnings.
 *
 * @param mex The mex_state to use.
 * @param token_node The node pointing to the #P_IDENTIFIER token.
 */
static void mex_check_identifier( mex_state_t *mex,
                                  p_token_node_t const *token_node ) {
  assert( mex != NULL );
  assert( token_node != NULL );
  p_token_t *const identifier_token = token_node->data;
  assert( identifier_token->kind == P_IDENTIFIER );

  if ( identifier_token->ident.ineligible )
    return;

  p_macro_t const *const found_macro =
    p_macro_find( identifier_token->ident.name );
  if ( found_macro == NULL )            // identifier is not a macro
    return;

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
      rb_insert_rv_t const rv_rbi = rb_tree_insert(
        mex->no_expand_set, CONST_CAST( char*, found_macro->name ),
        strlen( found_macro->name ) + 1
      );
      if ( rv_rbi.inserted ) {
        //
        // Now that we know the macro has been inserted, replace macro's name
        // used to test for insertion with a copy.  Doing it this way means we
        // do the look-up only once and the strdup() only if inserted.
        //
        print_warning( &identifier_token->loc,
          "\"%s\" not supported%s; will not expand\n",
          identifier_token->ident.name,
          c_lang_which( lang_ids )
        );
      }
    }
    return;
  }

  if ( !p_macro_is_func_like( found_macro ) )
    return;

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
        return;

      case P_IDENTIFIER:
        if ( p_macro_find( next_token->ident.name ) != NULL ) {
          //
          // The macro could expand into tokens starting with '('.
          //
          return;
        }

        if ( p_macro_find_param( mex->macro,
                                 next_token->ident.name ) != NO_PARAM ) {
          //
          // The parent's macro parameter could expand into tokens starting
          // with '('.
          //
          return;
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
          return;
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
        return;
    } // switch
  }

  strbuf_t const *const mnes_key = mex_no_expand_set_key( mex, found_macro );
  rb_insert_rv_t const rv_rbi = rb_tree_insert(
    mex->no_expand_set, CONST_CAST( char*, mnes_key->str ), mnes_key->len + 1
  );

  if ( next_token != NULL && next_token->is_substituted ) {
    //
    // The next token has already been substituted so this macro without
    // arguments can _never_ expand, so mark it ineligible so we won't warn
    // about it more than once.
    //
    identifier_token->ident.ineligible = true;
  }

  if ( rv_rbi.inserted ) {
    print_warning( &identifier_token->loc,
      "\"%s\": function-like macro without arguments will not expand\n",
      identifier_token->ident.name
    );
  }
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

  if ( n_args == 0 && n_req_params == 1 ) {
    //
    // Function-like macros taking multiple parameters will accept zero tokens
    // for an argument, e.g.:
    //
    //      cdecl> #define M2(A,B)  [A ## B]
    //      cdecl> expand M2(X,)
    //      M2(X,) => [A ## B]
    //      M2(X,) => [X ## ]
    //      M2(X,) => [X]
    //
    // This is also true for macros taking a single parameter:
    //
    //      cdecl> #define M1(A)    [A]
    //      cdecl> expand M1()
    //      M1() => [A]
    //      | A =>
    //      M1() => []
    //
    // Hence, retroactively create a single placemarker token for an argument.
    //
    // Even though the p_arg_list_opt parser rule inserts placemarkers for
    // empty arguments for multiple parameters, it doesn't do it for no
    // parameters because it can't distinguish between a function-like macro
    // that takes zero parameters from one that takes one parameter.  Hence, we
    // do it here.
    //
    slist_push_back( mex->arg_list, p_token_list_new_placemarker() );
    return true;
  }

  if ( n_args >= n_req_params && (n_args <= n_req_params || is_variadic) )
    return true;

  c_loc_t loc;
  if ( mex->indent == 0 ) {
    loc = mex->name_loc;
  } else {
    mex_print_macro( mex, mex->replace_list );
    loc = (c_loc_t){ .first_column = C_LOC_NUM_T( mex->indent * 2 ) };
  }

  if ( n_args < n_req_params ) {
    print_error( &loc,
      "too few arguments (%zu) for function-like macro (need %s%zu)\n",
      n_args, is_variadic ? "at least " : "", n_req_params
    );
  }
  else {
    print_error( &loc,
      "too many arguments (%zu) for function-like macro (need %zu)\n",
      n_args, n_req_params
    );
  }

  return false;
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
        "'%s' in non-function-like macro treated as text\n",
        other_token_c( "#" )
      );
    }
    return true;
  }

  if ( !OPT_LANG_IS( P_STRINGIFY ) ) {
    if ( false_set( &mex->warned_stringify_not_supported ) ) {
      print_warning( &stringify_token->loc,
        "'%s' not supported%s; treated as text\n",
        other_token_c( "#" ),
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
  print_error( &stringify_token->loc,
    "'%s' not followed by macro parameter", other_token_c( "#" )
  );
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
    print_error(
      &(c_loc_t){
        .first_line = __VA_OPT___token->loc.first_line,
        .first_column = __VA_OPT___token->loc.last_column + 1,
        .last_line = __VA_OPT___token->loc.last_line,
        .last_column = __VA_OPT___token->loc.last_column + 1
      },
      "'(' expected\n"
    );
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
            "\"%s\" can not be first within \"__VA_OPT__\"\n",
            other_token_c( "##" )
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
                  "\"%s\" can not be last within \"__VA_OPT__\"\n",
                  other_token_c( "##" )
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
  p_token_list_cleanup( &mex->va_args_token_list );
  p_token_list_cleanup( &mex->work_lists[0] );
  p_token_list_cleanup( &mex->work_lists[1] );

  if ( mex->parent_mex == NULL ) {
    rb_tree_cleanup( mex->expanding_set, /*free_fn=*/NULL );
    free( mex->expanding_set );
    rb_tree_cleanup( mex->no_expand_set, /*free_fn=*/NULL );
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

  mex_rv_t prev_rv = MEX_NOT_EXPANDED;

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
      case MEX_EXPANDED:
        break;
      case MEX_NOT_EXPANDED:
        return true;
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

  mex_rv_t rv = MEX_NOT_EXPANDED;

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
      case MEX_EXPANDED:
        if ( !mex->expand_opt_no_trim_tokens )
          p_token_list_trim( mex->expand_list );
        mex_relocate_expand_list( mex );
        mex_print_macro( mex, mex->expand_list );
        break;
      case MEX_NOT_EXPANDED:
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
    if ( opt_lang_is_any( (*mex->macro->dyn_fn)( &token ) ) ) {
      token->is_substituted = true;
      p_token_list_push_back( mex->expand_list, token );
      mex_print_macro( mex, mex->expand_list );
      return MEX_EXPANDED;
    }

    identifier_token->ident.ineligible = true;
    return MEX_NOT_EXPANDED;
  }

  if ( !p_macro_check_params( mex->macro ) )
    return MEX_NOT_EXPANDED;

  if ( mex->arg_list == NULL && p_macro_is_func_like( mex->macro ) )
    return MEX_NOT_EXPANDED;

  strbuf_t const *const mes_key = mex_expanding_set_key( mex );
  rb_insert_rv_t const rv_rbi =
    rb_tree_insert( mex->expanding_set, mes_key->str, mes_key->len + 1 );
  if ( !rv_rbi.inserted ) {
    identifier_token->ident.ineligible = true;
    print_warning( &identifier_token->loc,
      "recursive macro \"%s\" will not expand\n",
      mex->macro->name
    );
    return MEX_NOT_EXPANDED;
  }

  mex_print_macro( mex, mex->replace_list );
  mex_pre_filter___VA_OPT__( mex );
  mex_init_va_args_token_list( mex );

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

  bool const ok = mex_expand_all_fns( mex, EXPAND_FNS );
  rb_tree_delete( mex->expanding_set, rv_rbi.node );
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
 * Above, only `b` and `c` are concatenated even though `a b` was substituted
 * for `X` and `c d` was substituted for `Y`.
 * @endparblock
 *
 * @param mex The mex_state to use.
 * @return Returns a \ref mex_rv.
 */
NODISCARD
static mex_rv_t mex_expand_all_concat( mex_state_t *mex ) {
  assert( mex != NULL );

  if ( !OPT_LANG_IS( P_CONCAT ) )
    return MEX_NOT_EXPANDED;

  mex_rv_t rv = MEX_NOT_EXPANDED;

  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t const *const token = token_node->data;
    p_token_node_t const *next_node =
      p_token_node_not( token_node->next, P_SPACE );
    if ( next_node == NULL )
      goto skip;
    p_token_t const *next_token = next_node->data;
    if ( next_token->kind != P_CONCAT )
      goto skip;

    if ( p_token_is_macro( token ) ) {
      print_warning( &token->loc,
        "\"%s\" doesn't expand macro arguments; \"%s\" will not expand\n",
        other_token_c( "##" ), token->ident.name
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
          "\"%s\" doesn't expand macro arguments; \"%s\" will not expand\n",
          other_token_c( "##" ), next_token->ident.name
        );
      }

      strbuf_puts( &sbuf, p_token_str( next_token ) );
      next_node = p_token_node_not( token_node->next, P_SPACE );
      if ( next_node == NULL )
        break;
      next_token = next_node->data;
    } while ( next_token->kind == P_CONCAT );

    p_token_t *const concatted_token = p_token_lex( &token->loc, &sbuf );
    strbuf_cleanup( &sbuf );
    if ( concatted_token == NULL )
      return MEX_ERROR;
    concatted_token->is_substituted = true;
    p_token_list_push_back( mex->expand_list, concatted_token );
    rv = MEX_EXPANDED;
    continue;

skip:
    p_token_list_push_back( mex->expand_list, p_token_dup( token ) );
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

  mex_rv_t rv = MEX_NOT_EXPANDED;

  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t const *const token = token_node->data;
    if ( token->kind == P_IDENTIFIER ) {
      switch ( mex_expand_identifier( mex, &token_node ) ) {
        case MEX_EXPANDED:
          rv = MEX_EXPANDED;
          continue;
        case MEX_NOT_EXPANDED:
          break;
        case MEX_ERROR:
          return MEX_ERROR;
      } // switch
    }

    p_token_list_push_back( mex->expand_list, p_token_dup( token ) );
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

  if ( mex->arg_list == NULL || !p_macro_is_func_like( mex->macro ) )
    return MEX_NOT_EXPANDED;

  mex_rv_t rv = MEX_NOT_EXPANDED;

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
  rb_tree_init(
    &param_cache, RB_DINT, POINTER_CAST( rb_cmp_fn_t, &param_expand_cmp )
  );

  p_token_node_t const *prev_node = NULL;
  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t const *const token = token_node->data;
    if ( token->kind != P_IDENTIFIER )
      goto skip;
    p_token_list_t *arg_tokens = mex_param_arg( mex, token->ident.name );
    if ( arg_tokens == NULL )           // identifier isn't a parameter
      goto skip;

    p_token_node_t const *const next_node =
      p_token_node_not( token_node->next, P_SPACE );
    if ( p_is_operator_arg( prev_node, next_node ) )
      goto append;

    //
    // There's no need to strdup() the name because it will outlive this
    // param_expand_t node.
    //
    param_expand_t ins_pe = { .name = token->ident.name };
    rb_insert_rv_t const rv_rbi =
      rb_tree_insert( &param_cache, &ins_pe, sizeof ins_pe );

    if ( !rv_rbi.inserted ) {
      param_expand_t const *const found_pe = RB_DINT( rv_rbi.node );
      arg_tokens = found_pe->expand_list;
      goto append;
    }

    param_expand_t *const new_pe = RB_DINT( rv_rbi.node );

    mex_state_t param_mex;
    mex_init( &param_mex,
      /*parent_mex=*/mex,
      &(p_macro_t){ .name = token->ident.name },
      &mex->name_loc,
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
      // There's no need to duplicate the tokens because they will outlive this
      // param_expand_t node.
      //
      new_pe->expand_list = arg_tokens;
    }

    mex_cleanup( &param_mex );
    if ( !ok ) {
      rv = MEX_ERROR;
      goto done;
    }

append:
    set_substituted( push_back_dup_tokens( mex->expand_list, arg_tokens ) );
    rv = MEX_EXPANDED;
    goto next;

skip:
    p_token_list_push_back( mex->expand_list, p_token_dup( token ) );
next:
    if ( token->kind != P_SPACE )
      prev_node = token_node;
  } // for

done:
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
    return MEX_NOT_EXPANDED;

  mex_rv_t rv = MEX_NOT_EXPANDED;

  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t *const token = token_node->data;
    if ( token->kind != P_STRINGIFY )
      goto skip;
    if ( mex_expand_stringify( mex, &token_node ) ) {
      rv = MEX_EXPANDED;
      continue;
    }

skip:
    p_token_list_push_back( mex->expand_list, p_token_dup( token ) );
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
    return MEX_NOT_EXPANDED;

  bool expanded___VA_ARGS__ = false;
  mex_rv_t rv = MEX_NOT_EXPANDED;

  p_token_list_t expanded_va_args_token_list;
  slist_init( &expanded_va_args_token_list );

  p_token_node_t const *prev_node = NULL;
  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t const *const token = token_node->data;
    if ( token->kind != P___VA_ARGS__ )
      goto skip;

    p_token_node_t const *const next_node =
      p_token_node_not( token_node->next, P_ANY_TRANSPARENT );

    if ( false_set( &expanded___VA_ARGS__ ) &&
         !mex_expand___VA_ARGS__( mex, &expanded_va_args_token_list,
                                  prev_node, next_node ) ) {
      return MEX_ERROR;
    }

    set_substituted(
      push_back_dup_tokens( mex->expand_list, &expanded_va_args_token_list )
    );
    rv = MEX_EXPANDED;
    goto next;

skip:
    p_token_list_push_back( mex->expand_list, p_token_dup( token ) );
next:
    if ( token->kind != P_SPACE )
      prev_node = token_node;
  } // for

  p_token_list_cleanup( &expanded_va_args_token_list );
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
    return MEX_NOT_EXPANDED;

  mex_rv_t rv = MEX_NOT_EXPANDED;

  FOREACH_SLIST_NODE( token_node, mex->replace_list ) {
    p_token_t const *const token = token_node->data;
    if ( token->kind == P___VA_OPT__ ) {
      va_args_mex_print_macro( mex );
      token_node = mex_expand___VA_OPT__( mex, token_node, mex->expand_list );
      if ( token_node == NULL )
        return MEX_ERROR;
      rv = MEX_EXPANDED;
      continue;
    }
    p_token_list_push_back( mex->expand_list, p_token_dup( token ) );
  } // for

  return rv;
}

/**
 * Expands a #P_IDENTIFIER if it's a macro.
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
    return MEX_NOT_EXPANDED;

  p_macro_t const *const found_macro =
    p_macro_find( identifier_token->ident.name );
  if ( found_macro == NULL )            // identifier is not a macro
    return MEX_NOT_EXPANDED;

  p_arg_list_t arg_list;
  slist_init( &arg_list );
  bool looks_func_like = false;
  mex_rv_t rv = MEX_ERROR;

  p_token_node_t *const next_node =
    p_token_node_not( token_node->next, P_SPACE );
  if ( p_token_node_is_punct( next_node, '(' ) ) {
    token_node = parse_args( next_node, &arg_list );
    if ( token_node == NULL )
      goto done;
    looks_func_like = true;
  }

  mex_state_t macro_mex;
  mex_init( &macro_mex,
    /*parent_mex=*/mex,
    found_macro,
    &mex->name_loc,
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
      case MEX_NOT_EXPANDED:
      case MEX_ERROR:
        break;
    } // switch
  }

  mex_cleanup( &macro_mex );

done:
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
 * @return Returns `true` only if an expansion occurred.
 *
 * @sa mex_expand_all_stringify()
 */
NODISCARD
static bool mex_expand_stringify( mex_state_t *mex,
                                  p_token_node_t **ptoken_node ) {
  assert( OPT_LANG_IS( P_STRINGIFY ) );
  assert( mex != NULL );
  assert( ptoken_node != NULL );
  p_token_node_t const *const stringify_node = *ptoken_node;
  assert( p_token_node_is_any( stringify_node, P_STRINGIFY ) );

  if ( !p_macro_is_func_like( mex->macro ) ) {
    //
    // When # appears in the replacement list of a non-function-like macro, it
    // is treated as an ordinary character.
    //
    return false;
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
  return true;
}

/**
 * Expands the #P___VA_ARGS__ token.
 *
 * @param mex The mex_state to use.
 * @param va_args_token_list The \ref p_token_list_t to expand into.
 * @param prev_node The non-space \ref p_token_node_t just before
 * #P___VA_ARGS__.
 * @param next_node The non-space \ref p_token_node_t just after
 * #P___VA_ARGS__.
 * @return Returns a \ref mex_rv.
 *
 * @sa mex_expand_all___VA_ARGS__()
 */
NODISCARD
static bool mex_expand___VA_ARGS__( mex_state_t *mex,
                                    p_token_list_t *va_args_token_list,
                                    p_token_node_t const *prev_node,
                                    p_token_node_t const *next_node ) {
  assert( mex != NULL );
  assert( va_args_token_list != NULL );
  assert( slist_empty( va_args_token_list ) );

  mex_state_t va_args_mex;
  VA_ARGS_MEX_INIT( &va_args_mex, mex );
  if ( false_set( &mex->printed___VA_ARGS__ ) )
    mex_print_macro( &va_args_mex, va_args_mex.replace_list );

  bool ok;

  if ( p_is_operator_arg( prev_node, next_node ) ) {
    push_back_dup_tokens( va_args_mex.expand_list, va_args_mex.replace_list );
    ok = true;
  }
  else {
    static mex_expand_all_fn_t const EXPAND_FNS[] = {
      &mex_expand_all_stringify,
      &mex_expand_all_params,
      &mex_expand_all_concat,
      NULL
    };
    ok = mex_expand_all_fns( &va_args_mex, EXPAND_FNS );
  }

  if ( ok )
    *va_args_token_list = slist_move( va_args_mex.expand_list );
  mex_cleanup( &va_args_mex );
  return ok;
}

/**
 * Expands the #P___VA_OPT__ token.
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
 * @sa mex_init_va_args_token_list()
 */
NODISCARD
static p_token_node_t* mex_expand___VA_OPT__( mex_state_t *mex,
                                              p_token_node_t const
                                                *__VA_OPT___node,
                                              p_token_list_t *dst_list ) {
  assert( mex != NULL );
  assert( p_macro_is_variadic( mex->macro ) );
  assert( p_token_node_is_any( __VA_OPT___node, P___VA_OPT__ ) );
  assert( dst_list != NULL );
  assert( OPT_LANG_IS( P___VA_OPT__ ) );

  p_token_node_t *token_node =
    p_token_node_not( __VA_OPT___node->next, P_SPACE );
  assert( p_token_node_is_punct( token_node, '(' ) );

  bool const is_va_args_empty = slist_empty( &mex->va_args_token_list );

  p_token_list_t va_opt_token_list;
  slist_init( &va_opt_token_list );

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
      p_token_list_push_back( &va_opt_token_list, p_token_dup( token ) );
  } // for

  p_token_list_trim( &va_opt_token_list );

  if ( slist_empty( &va_opt_token_list ) ) {
    slist_push_back( dst_list, p_token_new( P_PLACEMARKER, /*literal=*/NULL ) );
  }
  else {
    mex_state_t va_opt_mex;
    mex_init( &va_opt_mex,
      /*parent_mex=*/mex,
      &(p_macro_t){
        .name = L_PRE___VA_OPT__,
        .param_list = mex->macro->param_list
      },
      &mex->name_loc,
      mex->arg_list,
      /*replace_list=*/&va_opt_token_list,
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

  p_token_list_cleanup( &va_opt_token_list );
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
static strbuf_t const* mex_expanding_set_key( mex_state_t const *mex ) {
  assert( mex != NULL );

  static strbuf_t sbuf;
  strbuf_reset( &sbuf );

  strbuf_puts( &sbuf, mex->macro->name );
  if ( mex->arg_list != NULL ) {
    strbuf_putc( &sbuf, '(' );
    FOREACH_SLIST_NODE( arg_node, mex->arg_list ) {
      strbuf_puts( &sbuf, p_token_list_str( arg_node->data ) );
      if ( arg_node->next != NULL )
        strbuf_putc( &sbuf, ',' );
    } // for
    strbuf_putc( &sbuf, ')' );
  }

  return &sbuf;
}

/**
 * Initializes \a mex.
 *
 * @param mex The mex_state to initialize.
 * @param parent_mex The parent mex_state, if any.
 * @param macro The macro to use.
 * @param name_loc The source location of \a macro's name.
 * @param arg_list The argument list, if any.
 * @param replace_list The replacement token list.
 * @param fout The `FILE` to print to.
 *
 * @sa mex_cleanup()
 * @sa #VA_ARGS_MEX_INIT()
 */
static void mex_init( mex_state_t *mex, mex_state_t *parent_mex,
                      p_macro_t const *macro, c_loc_t const *name_loc,
                      p_arg_list_t *arg_list,
                      p_token_list_t const *replace_list, FILE *fout ) {
  assert( mex != NULL );
  assert( macro != NULL );
  assert( macro->name != NULL );
  assert( name_loc != NULL );
  assert( replace_list != NULL );
  assert( fout != NULL );

  rb_tree_t *expanding_set;
  unsigned   indent;
  rb_tree_t *no_expand_set;

  if ( parent_mex == NULL ) {
    expanding_set = MALLOC( rb_tree_t, 1 );
    rb_tree_init(
      expanding_set, RB_DINT, POINTER_CAST( rb_cmp_fn_t, &strcmp )
    );
    indent = 0;
    no_expand_set = MALLOC( rb_tree_t, 1 );
    rb_tree_init(
      no_expand_set, RB_DINT, POINTER_CAST( rb_cmp_fn_t, &strcmp )
    );
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
    .name_loc = *name_loc,
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
 * Expands the #P___VA_ARGS__ token.
 *
 * @param mex The mex_state to use.
 *
 * @sa mex_check___VA_ARGS__()
 * @sa mex_expand___VA_OPT__()
 * @sa mex_stringify___VA_ARGS__()
 */
static void mex_init_va_args_token_list( mex_state_t *mex ) {
  assert( mex != NULL );

  if ( !slist_empty( &mex->va_args_token_list ) )
    return;
  if ( !p_macro_is_variadic( mex->macro ) )
    return;

  assert( OPT_LANG_IS( VARIADIC_MACROS ) );

  size_t curr_index = 0;
  size_t const ellipsis_index = slist_len( mex->macro->param_list ) - 1;

  FOREACH_SLIST_NODE( arg_node, mex->arg_list ) {
    if ( curr_index++ < ellipsis_index )
      continue;
    push_back_dup_tokens( &mex->va_args_token_list, arg_node->data );
    if ( arg_node->next != NULL ) {
      slist_push_back(
        &mex->va_args_token_list, p_token_new( P_PUNCTUATOR, "," )
      );
    }
  } // for
}

/**
 * Generates a key for function-like macros that won't expand for the \ref
 * mex_state::no_expand_set "no_expand_set".
 *
 * @param mex The mex_state to use.
 * @param warn_macro The function-like \ref p_macro to potentially warn about.
 * @return Returns said key.
 *
 * @warning The pointer returned is to a static buffer.
 */
NODISCARD
static strbuf_t const* mex_no_expand_set_key( mex_state_t const *mex,
                                              p_macro_t const *warn_macro ) {
  assert( mex != NULL );
  assert( warn_macro != NULL );

  static strbuf_t sbuf;
  strbuf_reset( &sbuf );
  strbuf_printf( &sbuf, "%s-%s", mex->macro->name, warn_macro->name );
  return &sbuf;
}

/**
 * Given \a param_name, gets the tokens comprising the corresponding macro
 * argument.
 *
 * @param mex The mex_state to use.
 * @param param_name The name of a macro parameter.
 * @return Returns the tokens comprising said argument or NULL if \a param_name
 * isn't a macro parameter.
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
  p_token_list_t *const arg_tokens = slist_at( mex->arg_list, param_index );
  assert( arg_tokens != NULL );
  return arg_tokens;
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
    token->ident.name = check_strdup( L_PRE___VA_OPT__ );
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
    FOREACH_SLIST_NODE( arg_node, mex->arg_list ) {
      push_back_dup_tokens( &replace_list, arg_node->data );
      if ( arg_node->next != NULL )
        slist_push_back( &replace_list, p_token_new( P_PUNCTUATOR, "," ) );
    } // for
    slist_push_back( &replace_list, p_token_new( P_PUNCTUATOR, ")" ) );
  }

  mex_state_t check_mex;
  mex_init( &check_mex,
    /*parent_mex=*/NULL,
    &(p_macro_t){ .name = "preliminary_check" },
    &mex->name_loc,
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
 * Only before the first expansion pass, for every token comprising the current
 * \ref mex_state::replace_list "replace_list":
 *
 *  + Adjusts the \ref c_loc::first_column "first_column" and \ref
 *    c_loc::last_column "last_column" of \ref p_token::loc "loc".
 *
 *  + Sets the \ref c_loc::first_line "first_line" and \ref c_loc::last_line
 *    "last_line" of \ref p_token::loc "loc" to \ref c_loc::first_line
 *    "first_line" of \ref mex_state::name_loc.
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

    FOREACH_SLIST_NODE( token_node, mex->expand_list ) {
      p_token_t *const token = token_node->data;
      token->loc.first_line = token->loc.last_line = mex->name_loc.first_line;
    } // for

    mex_swap_lists( mex );
  }
}

/**
 * Prepares macro arguments and checks their number.
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
  p_arg_list_trim( mex->arg_list );
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
 * @sa mex_print_arg_list()
 * @sa print_token_list_color()
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
    print_token_list_color( token_list, mex->fout );
  }
  FPUTC( '\n', mex->fout );

  if ( opt_cdecl_debug == CDECL_DEBUG_NO )
    return;

  DUMP_START( mex->indent, mex->fout );
  DUMP_STR( "macro", mex->macro->name );
  if ( print_arg_list ) {
    DUMP_KEY( "arg_list: " );
    p_arg_list_dump( mex->arg_list, mex->indent + 1, mex->fout );
  }
  if ( print_token_list ) {
    DUMP_KEY( "token_list: " );
    p_token_list_dump( token_list, mex->indent + 1, mex->fout );
  }
  DUMP_END();
}

/**
 * Adjusts the \ref c_loc::first_column "first_column" and \ref
 * c_loc::last_column "last_column" of \ref p_token::loc "loc" for every token
 * comprising the current \ref mex_state::expand_list "expand_list".
 *
 * @param mex The mex_state to use.
 *
 * @note The column calculations _must_ match how mex_print_macro() prints.
 *
 * @sa mex_preliminary_relocate_replace_list()
 * @sa p_token_list_relocate()
 */
static void mex_relocate_expand_list( mex_state_t *mex ) {
  assert( mex != NULL );

  size_t column = STRLITLEN( "| " ) * mex->indent + strlen( mex->macro->name );

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

  column += STRLITLEN( " " ) + STRLITLEN( ARROW ) + STRLITLEN( " " );
  PJL_DISCARD_RV( p_token_list_relocate( mex->expand_list, column ) );
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
        "'%s' doesn't expand macro arguments; \"%s\" will not expand\n",
        other_token_c( "#" ),
        arg_token->ident.name
      );
    }
  }

  p_token_t *const stringified_token =
    p_token_new( P_STR_LIT, check_strdup( p_token_list_str( arg_tokens ) ) );
  stringified_token->is_substituted = true;
  p_token_list_push_back( mex->expand_list, stringified_token );
}

/**
 * Stringifies the #P___VA_ARGS__ token.
 *
 * @param mex The mex_state to use.
 *
 * @sa mex_init_va_args_token_list()
 * @sa mex_stringify___VA_OPT__()
 */
static void mex_stringify___VA_ARGS__( mex_state_t *mex ) {
  assert( mex != NULL );

  va_args_mex_print_macro( mex );

  p_token_t *const stringified_token = p_token_new(
    P_STR_LIT, check_strdup( p_token_list_str( &mex->va_args_token_list ) )
  );

  stringified_token->is_substituted = true;
  p_token_list_push_back( mex->expand_list, stringified_token );
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

  va_args_mex_print_macro( mex );

  p_token_list_t va_opt_token_list;
  slist_init( &va_opt_token_list );

  p_token_node_t *const rv_node =
    mex_expand___VA_OPT__( mex, __VA_OPT___node, &va_opt_token_list );
  p_token_t *const stringified_token = p_token_new(
    P_STR_LIT, check_strdup( p_token_list_str( &va_opt_token_list ) )
  );
  p_token_list_cleanup( &va_opt_token_list );
  stringified_token->is_substituted = true;
  p_token_list_push_back( mex->expand_list, stringified_token );

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
 * Trims leading #P_SPACE tokens from the first argument's tokens and trailing
 * #P_SPACE tokens from the last argument's tokens.
 *
 * @param arg_list The macro argument list to trim.
 *
 * @sa p_token_list_trim()
 */
static void p_arg_list_trim( p_arg_list_t *arg_list ) {
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
static void p_macro_cleanup( p_macro_t *macro ) {
  if ( macro == NULL )
    return;                             // LCOV_EXCL_LINE
  if ( !macro->is_dynamic ) {
    p_param_list_cleanup( macro->param_list );
    free( macro->param_list );
    p_token_list_cleanup( &macro->replace_list );
  }
  FREE( macro->name );
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
 * @note The column calculations _must_ match how show_macro() prints.
 */
static void p_macro_relocate_params( p_macro_t *macro ) {
  assert( macro != NULL );
  assert( p_macro_is_func_like( macro ) );

  size_t column = strlen( other_token_c( "#" ) ) + STRLITLEN( "define " )
    + strlen( macro->name ) + STRLITLEN( "(" );

  FOREACH_SLIST_NODE( param_node, macro->param_list ) {
    p_param_t *const param = param_node->data;
    param->loc.first_column = C_LOC_NUM_T( column );
    column += strlen( param->name );
    param->loc.last_column = C_LOC_NUM_T( column - 1 );
    if ( param_node->next != NULL )
      column += STRLITLEN( ", " );
  } // for
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
  assert( i_pe != NULL );
  assert( j_pe != NULL );
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
static p_token_node_t* parse_args( p_token_node_t *token_node,
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
      p_token_list_push_back( arg_tokens, p_token_dup( token ) );
  } // for

  return token_node;
}

/**
 * Cleans up C preprocessor macro data at program termination.
 *
 * @note This function is called only via **atexit**(3).
 *
 * @sa p_macros_init()
 */
static void p_macros_cleanup( void ) {
  rb_tree_cleanup( &macro_set, POINTER_CAST( rb_free_fn_t, &p_macro_cleanup ) );
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

  p_token_node_t *const orig_dst_tail = dst_list->tail;

  FOREACH_SLIST_NODE( src_node, src_list )
    p_token_list_push_back( dst_list, p_token_dup( src_node->data ) );

  return orig_dst_tail != NULL ? orig_dst_tail->next : dst_list->head;
}

/**
 * Red-black tree visitor function that forwards to the \ref p_macro_visit_fn_t
 * function.
 *
 * @param node_data A pointer to the node's data.
 * @param visit_data Data passed to to the visitor.
 * @return Returning `true` will cause traversal to stop and the current node
 * to be returned to the caller of rb_tree_visit().
 */
NODISCARD
static bool rb_visitor( void *node_data, void *visit_data ) {
  assert( node_data != NULL );
  assert( visit_data != NULL );

  p_macro_t const *const macro = node_data;
  macro_rb_visit_data_t const *const mrvd = visit_data;

  return (*mrvd->visit_fn)( macro, mrvd->visit_data );
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
 * Prints a macro's `__VA_ARGS__` tokens, if any.
 *
 * @param mex The mex_state to use.
 */
static void va_args_mex_print_macro( mex_state_t *mex ) {
  assert( mex != NULL );

  if ( false_set( &mex->printed___VA_ARGS__ ) ) {
    mex_state_t va_args_mex;
    VA_ARGS_MEX_INIT( &va_args_mex, mex );
    mex_print_macro( &va_args_mex, va_args_mex.replace_list );
    mex_cleanup( &va_args_mex );
  }
}

////////// extern functions ///////////////////////////////////////////////////

bool macro_name_is_predefined( char const *name ) {
  assert( name != NULL );
  if ( macro_name_is__VA_( name ) )
    return true;
  p_macro_t const *const macro = p_macro_find( name );
  return macro != NULL && macro->is_dynamic;
}


void p_arg_list_cleanup( p_arg_list_t *arg_list ) {
  if ( arg_list == NULL )
    return;                             // LCOV_EXCL_LINE
  //
  // Using only the free_fn parameter of slist_cleanup() would also require a
  // p_token_list_free() function in addition to p_token_list_cleanup(). It's
  // not worth it since this is the only place where p_token_list_free() would
  // be needed.  Instead, just iterate and call p_token_list_cleanup() first
  // followed by free_fn = free().
  //
  FOREACH_SLIST_NODE( arg_node, arg_list )
    p_token_list_cleanup( arg_node->data );
  slist_cleanup( arg_list, &free );
}

p_macro_t* p_macro_define( char *name, c_loc_t const *name_loc,
                           p_param_list_t *param_list,
                           p_token_list_t *replace_list ) {
  assert( name != NULL );
  assert( name_loc != NULL );

  if ( !macro_name_check( name, name_loc ) )
    goto error;
  if ( param_list != NULL && !check_macro_params( param_list ) )
    goto error;

  p_macro_t new_macro = {
    .name = name,
    .replace_list = slist_move( replace_list )
  };
  if ( param_list != NULL ) {
    new_macro.param_list = MALLOC( p_param_list_t, 1 );
    *new_macro.param_list = slist_move( param_list );
  }

  p_token_list_trim( &new_macro.replace_list );

  mex_state_t check_mex;
  mex_init( &check_mex,
    /*parent_mex=*/NULL,
    &new_macro,
    name_loc,
    /*arg_list=*/NULL,
    &new_macro.replace_list,
    stdout
  );

  bool const ok = mex_check( &check_mex );
  mex_cleanup( &check_mex );
  if ( !ok ) {
    p_macro_cleanup( &new_macro );
    return NULL;
  }

  if ( p_macro_is_func_like( &new_macro ) )
    p_macro_relocate_params( &new_macro );

  rb_insert_rv_t const rv_rbi =
    rb_tree_insert( &macro_set, &new_macro, sizeof new_macro );
  if ( !rv_rbi.inserted ) {
    p_macro_t *const old_macro = RB_DINT( rv_rbi.node );
    assert( !old_macro->is_dynamic );
    p_macro_cleanup( old_macro );
    memcpy( rv_rbi.node->data, &new_macro, sizeof new_macro );
    print_warning( name_loc, "\"%s\" already exists; redefined\n", name );
  }

  return RB_DINT( rv_rbi.node );

error:
  free( name );
  return NULL;
}

bool p_macro_expand( char const *name, c_loc_t const *name_loc,
                     p_arg_list_t *arg_list, p_token_list_t *extra_list,
                     FILE *fout ) {
  assert( name != NULL );
  assert( name_loc != NULL );
  assert( fout != NULL );

  if ( macro_name_is__VA_( name ) ) {
    print_error( name_loc, "\"%s\" only valid in macro definition\n", name );
    return false;
  }

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
  bool const orig_no_print_input_line = print_params.opt_no_print_input_line;

  mex_state_t mex;
  mex_init( &mex,
    /*parent_mex=*/NULL,
    macro,
    name_loc,
    arg_list,
    &macro->replace_list,
    fout
  );

  bool ok = false;

  if ( !mex_prep_args( &mex ) || !mex_preliminary_check( &mex ) )
    goto done;

  //
  // For non-dynamic macros, we have to relocate the tokens comprising the
  // replacement list.  The correct locations are based on the supplied
  // arguments, so they'll always be different for each expansion.
  //
  mex_preliminary_relocate_replace_list( &mex );

  //
  // Instruct print_error() and print_warning() _not_ to print the input line
  // before printing ^ since we need to print the macro expansion lines
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
    goto done;

  if ( extra_list != NULL && !p_token_list_emptyish( extra_list ) ) {
    //
    // There were extra tokens given at the end of the "expand" command: append
    // them and do another expansion pass.
    //
    slist_push_list_back( mex.expand_list, extra_list );
    mex_relocate_expand_list( &mex );
    mex_swap_lists( &mex );
    if ( mex_expand( &mex, &token ) == MEX_ERROR )
      goto done;
  }

  ok = true;

done:
  print_params.opt_no_print_input_line = orig_no_print_input_line;
  mex_cleanup( &mex );
  return ok;
}

p_macro_t const* p_macro_find( char const *name ) {
  assert( name != NULL );
  p_macro_t const find_macro = { .name = name };
  rb_node_t const *const found_rb = rb_tree_find( &macro_set, &find_macro );
  return found_rb != NULL ? RB_DINT( found_rb ) : NULL;
}

bool p_macro_undef( char const *name, c_loc_t const *name_loc ) {
  assert( name != NULL );
  assert( name_loc != NULL );

  if ( macro_name_is__VA_( name ) )
    goto predef_macro;

  p_macro_t const find_macro = { .name = name };
  rb_node_t *const found_rb = rb_tree_find( &macro_set, &find_macro );
  if ( found_rb == NULL ) {
    print_error( name_loc, "\"%s\": no such macro\n", name );
    return false;
  }

  p_macro_t *const macro = RB_DINT( found_rb );
  if ( macro->is_dynamic )
    goto predef_macro;

  p_macro_cleanup( macro );
  rb_tree_delete( &macro_set, found_rb );
  return true;

predef_macro:
  print_error( name_loc,
    "\"%s\": predefined macro may not be undefined\n", name
  );
  return false;
}

void p_macro_visit( p_macro_visit_fn_t visit_fn, void *visit_data ) {
  assert( visit_fn != NULL );
  macro_rb_visit_data_t mrvd = { visit_fn, visit_data };
  rb_tree_visit( &macro_set, &rb_visitor, &mrvd );
}

void p_macros_init( void ) {
  ASSERT_RUN_ONCE();

  rb_tree_init(
    &macro_set, RB_DINT, POINTER_CAST( rb_cmp_fn_t, &p_macro_cmp )
  );
  ATEXIT( &p_macros_cleanup );

  extern void p_predefine_macros( void );
  p_predefine_macros();
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

///////////////////////////////////////////////////////////////////////////////

/** @} */

extern inline bool p_macro_is_func_like( p_macro_t const* );

/* vim:set et sw=2 ts=2: */
