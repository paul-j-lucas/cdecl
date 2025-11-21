/*
**      cdecl -- C gibberish translator
**      src/parser.y
**
**      Copyright (C) 2017-2025  Paul J. Lucas, et al.
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

/**
 * @file
 * Defines helper macros, data structures, variables, functions, and the
 * grammar for C/C++ declarations.
 */

/** @cond DOXYGEN_IGNORE */

%define api.header.include { "parser.h" }
%expect 23

%{
/** @endcond */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_ast.h"
#include "c_ast_check.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_operator.h"
#include "c_sglob.h"
#include "c_sname.h"
#include "c_type.h"
#include "c_typedef.h"
#include "cdecl.h"
#include "cdecl_dym.h"
#include "cdecl_keyword.h"
#include "color.h"
#include "dump.h"
#include "english.h"
#include "gibberish.h"
#include "help.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "p_macro.h"
#include "p_token.h"
#include "print.h"
#include "red_black.h"
#include "set_options.h"
#include "show.h"
#include "slist.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <stdio.h>
#include <stdlib.h>

// Silence these warnings for Bison-generated code.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wunreachable-code"

// Developer aid for tracing when Bison %destructors are called.
#if 0
#define DTRACE                    EPRINTF( "%d: destructor\n", __LINE__ )
#else
#define DTRACE                    NO_OP
#endif

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Checks whether \a AST is a #K_NAME and therefore an error (if \ref
 * opt_permissive_types is `false`) or merely a warning (if \ref
 * opt_permissive_types is `true`).
 *
 * @param AST The AST to check.
 * @return Returns `true` only if \a ast is a #K_NAME and therefore an error.
 */
#define c_ast_is_name_error(AST)  l_c_ast_is_name_error( __LINE__, (AST) )

/**
 * Checks whether \a SNAME is a type.
 *
 * @param SNAME The scoped name to check.
 * @param LOC The location of \a SNAME.
 * @return Returns `true` only if \a SNAME is a type (and prints an error
 * message); otherwise `false` (and does nothing).
 */
#define c_sname_is_type(SNAME,LOC) \
  l_c_sname_is_type( __LINE__, (SNAME), (LOC) )

/**
 * @defgroup parser-group Parser
 * Helper macros, data structures, variables, functions, and the grammar for
 * C/C++ declarations.
 * @{
 */

/**
 * Calls #elaborate_error_dym() with #DYM_NONE.
 *
 * @param ... Arguments passed to l_elaborate_error().
 *
 * @note This must be used _only_ after an `error` token, e.g.:
 * ```
 *  | Y_define error
 *    {
 *      elaborate_error( "name expected" );
 *    }
 * ```
 * @note A newline _is_ printed.
 *
 * @sa elaborate_error_dym()
 * @sa keyword_expected()
 * @sa punct_expected()
 */
#define elaborate_error(...) \
  elaborate_error_dym( DYM_NONE, __VA_ARGS__ )

/**
 * Calls l_elaborate_error() followed by #PARSE_ABORT().
 *
 * @param DYM_KINDS The bitwise-or of \ref dym_kind_t things possibly meant.
 * @param ... Arguments passed to l_elaborate_error().
 *
 * @note This must be used _only_ after an `error` token, e.g.:
 * ```
 *  | error
 *    {
 *      elaborate_error_dym( DYM_COMMANDS, "unexpected token" );
 *    }
 * ```
 * @note A newline _is_ printed.
 *
 * @sa elaborate_error()
 * @sa keyword_expected()
 * @sa punct_expected()
 */
#define elaborate_error_dym(DYM_KINDS,...) BLOCK( \
  l_elaborate_error( __LINE__, (DYM_KINDS), __VA_ARGS__ ); PARSE_ABORT(); )

/**
 * Executes the given statements only if \ref opt_cdecl_debug `!=`
 * #CDECL_DEBUG_NO.
 *
 * @param ... The statement(s) to execute.
 */
#define IF_CDECL_DEBUG(...) \
  BLOCK( if ( opt_cdecl_debug != CDECL_DEBUG_NO ) { __VA_ARGS__ } )

/**
 * Prints that a particular language feature is not supported by **cdecl** and
 * will be ignored.
 *
 * @param LOC The location of what's being ignored.
 * @param WHAT The string literal of what is being ignored.  It may contain a
 * `printf()` format string.
 * @param ... The `printf()` arguments, if any.
 *
 * @sa #UNSUPPORTED()
 */
#define IGNORING(LOC,WHAT,...)                      \
  print_warning( (LOC),                             \
    WHAT " not supported by " CDECL " (ignoring)\n" \
    VA_OPT( (,), __VA_ARGS__ ) __VA_ARGS__          \
  )

/**
 * Checks whether the type currently being declared (`enum`, `struct`,
 * `typedef`, or `union`) is nested within some other type (`struct` or
 * `union`) and whether the current language is C: if not, prints an error
 * message.
 *
 * @remarks In debug mode, also includes the file & line where the function was
 * called from in the error message.
 *
 * @param TYPE_LOC The location of the type declaration.
 * @return Returns `true` only if the type currently being declared is either
 * not nested or the current language is C++.
 *
 * @sa l_is_nested_type_ok()
 */
#define is_nested_type_ok(TYPE_LOC) \
  l_is_nested_type_ok( __LINE__, (TYPE_LOC) )

/**
 * Calls l_keyword_expected() followed by #PARSE_ABORT().
 *
 * @param KEYWORD A keyword literal.
 *
 * @note This must be used _only_ after an `error` token, e.g.:
 * ```
 *  : Y_virtual
 *  | error
 *    {
 *      keyword_expected( L_virtual );
 *    }
 * ```
 *
 * @sa elaborate_error()
 * @sa elaborate_error_dym()
 * @sa punct_expected()
 */
#define keyword_expected(KEYWORD) BLOCK ( \
  l_keyword_expected( __LINE__, (KEYWORD) ); PARSE_ABORT(); )

/**
 * Aborts the current parse after an error message has been printed.
 *
 * @sa #PARSE_ASSERT()
 */
#define PARSE_ABORT() \
  BLOCK( parse_cleanup( /*fatal_error=*/true ); YYABORT; )

/**
 * Evaluates \a EXPR: if `false`, calls #PARSE_ABORT().
 *
 * @param EXPR The expression to evalulate.
 */
#define PARSE_ASSERT(EXPR)        BLOCK( if ( !(EXPR) ) PARSE_ABORT(); )

/**
 * Calls l_punct_expected() followed by #PARSE_ABORT().
 *
 * @param PUNCT The punctuation character that was expected.
 *
 * @note This must be used _only_ after an `error` token, e.g.:
 * ```
 *  : ','
 *  | error
 *    {
 *      punct_expected( ',' );
 *    }
 * ```
 *
 * @sa elaborate_error()
 * @sa elaborate_error_dym()
 * @sa keyword_expected()
 */
#define punct_expected(PUNCT) BLOCK( \
  l_punct_expected( __LINE__, (PUNCT) ); PARSE_ABORT(); )

/**
 * Prints that a particular language feature is not supported by **cdecl** and
 * therefore an error followed by #PARSE_ABORT().
 *
 * @param LOC The location of what's not supported.
 * @param WHAT The string literal of what is being ignored.  It may contain a
 * `printf()` format string.
 * @param ... The `printf()` arguments, if any.
 *
 * @sa #IGNORING()
 */
#define UNSUPPORTED(LOC,WHAT,...) BLOCK(    \
  print_error( (LOC),                       \
    WHAT " not supported by " CDECL "\n"    \
    VA_OPT( (,), __VA_ARGS__ ) __VA_ARGS__  \
  );                                        \
  PARSE_ABORT(); )

/** @} */

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup parser-dump-group Debugging Macros
 * Macros that are used to dump a trace during parsing when \ref
 * opt_cdecl_debug is set.
 * @ingroup parser-group
 * @{
 */

/**
 * Dumps \a ALIGN.
 *
 * @param KEY The key name to print.
 * @param ALIGN The \ref c_alignas to dump.
 */
#define DUMP_ALIGN(KEY,ALIGN) IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": " ); c_alignas_dump( &(ALIGN), dump.fout ); )

/**
 * Dumps an AST.
 *
 * @param KEY The key name to print.
 * @param AST The AST to dump.
 *
 * @sa #DUMP_AST_LIST()
 * @sa #DUMP_AST_PAIR()
 */
#define DUMP_AST(KEY,AST) IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": " ); c_ast_dump( (AST), dump.fout ); )

/**
 * Dumps an s_list of AST.
 *
 * @param KEY The key name to print.
 * @param AST_LIST The \ref slist of AST to dump.
 *
 * @sa #DUMP_AST()
 * @sa #DUMP_AST_PAIR()
 */
#define DUMP_AST_LIST(KEY,AST_LIST) IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": " ); c_ast_list_dump( &(AST_LIST), dump.fout ); )

/**
 * Dump a \ref c_ast_pair.
 *
 * @param KEY The key name to print.
 * @param ASTP The \ref c_ast_pair to dump.
 *
 * @sa #DUMP_AST()
 */
#define DUMP_AST_PAIR(KEY,ASTP) IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": " ); c_ast_pair_dump( &(ASTP), dump.fout ); )

/**
 * Dumps a `bool`.
 *
 * @param KEY The key name to print.
 * @param BOOL The `bool` to dump.
 */
#define DUMP_BOOL(KEY,BOOL)  IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": " ); bool_dump( (BOOL), dump.fout ); )

/**
 * Ends a dump block.
 *
 * @sa #DUMP_START()
 */
#define DUMP_END() \
  IF_CDECL_DEBUG( FPUTS( "\n}\n\n", dump.fout ); )

/**
 * Possibly dumps a comma and a newline followed by the `printf()` arguments
 * --- used for printing a key followed by a value.
 *
 * @param ... The `printf()` arguments.
 *
 * @warning This _must_ only be called inside #IF_CDECL_DEBUG().
 */
#define DUMP_KEY(...) BLOCK(                  \
  fput_sep( ",\n", &dump.comma, dump.fout );  \
  FPRINTF( dump.fout, "  " __VA_ARGS__ ); )

/**
 * Dumps an integer.
 *
 * @param KEY The key name to print.
 * @param NUM The integer to dump.
 *
 * @sa #DUMP_STR()
 */
#define DUMP_INT(KEY,NUM) IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": %d", STATIC_CAST( int, (NUM) ) ); )

/**
 * Dumps a \ref p_macro.
 *
 * @param KEY The key name to print.
 * @param MACRO The \ref p_macro to dump.
 */
#define DUMP_MACRO(KEY,MACRO) IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": " ); p_macro_dump( (MACRO), dump.fout ); )

/**
 * Dumps a list of macro arguments.
 *
 * @param KEY The key name to print.
 * @param ARG_LIST The list of arguments to dump.
 */
#define DUMP_MACRO_ARG_LIST(KEY,ARG_LIST) IF_CDECL_DEBUG(       \
  if ( (ARG_LIST) != NULL ) {                                   \
    DUMP_KEY( KEY ": " );                                       \
    p_arg_list_dump( (ARG_LIST), dump.indent + 1, dump.fout );  \
  } )

/**
 * Dumps a list of \ref p_param.
 *
 * @param KEY The key name to print.
 * @param PARAM_LIST The list of \ref p_param to dump.
 */
#define DUMP_MACRO_PARAM_LIST(KEY,PARAM_LIST) IF_CDECL_DEBUG(       \
  if ( (PARAM_LIST) != NULL ) {                                     \
    DUMP_KEY( KEY ": " );                                           \
    p_param_list_dump( (PARAM_LIST), dump.indent + 1, dump.fout );  \
  } )

/**
 * Dumps a list of \ref p_token.
 *
 * @param KEY The key name to print.
 * @param TOKEN_LIST The list of \ref p_token to dump.
 */
#define DUMP_MACRO_TOKEN_LIST(KEY,TOKEN_LIST) IF_CDECL_DEBUG(       \
  if ( (TOKEN_LIST) != NULL ) {                                     \
    DUMP_KEY( KEY ": " );                                           \
    p_token_list_dump( (TOKEN_LIST), dump.indent + 1, dump.fout );  \
  } )

/**
 * Starts a dump block.
 *
 * @remarks If a production has a result, it should be dumped as the final
 * thing before the #DUMP_END() with the KEY of `$$_` followed by a suffix
 * denoting the type, e.g., `ast`.  For example:
 * ```
 *  DUMP_START( "rule",
 *              "subrule_1 subrule_2 ..." );
 *  DUMP_AST( "subrule_1", $1 );
 *  DUMP_STR( "name", $2 );
 *  DUMP_AST( "subrule_2", $3 );
 *  // ...
 *  DUMP_AST( "$$_ast", $$ );
 *  DUMP_END();
 * ```
 *
 * @param NAME The grammar production name.
 * @param RULE The grammar production rule.
 *
 * @note The dump block _must_ end with #DUMP_END().
 *
 * @sa #DUMP_END()
 */
#define DUMP_START(NAME,RULE)     \
  dump_state_t dump;              \
  dump_init( &dump, 0, stdout );  \
  IF_CDECL_DEBUG( PUTS( "{\n  rule: {\n    lhs: \"" NAME "\",\n    rhs: \"" RULE "\"\n  },\n" ); )

/**
 * Dumps a scoped name.
 *
 * @param KEY The key name to print.
 * @param SNAME The scoped name to dump.
 *
 * @sa #DUMP_SNAME_LIST()
 * @sa #DUMP_STR()
 */
#define DUMP_SNAME(KEY,SNAME) IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": " ); c_sname_dump( &(SNAME), dump.fout ); )

/**
 * Dumps a list of scoped names.
 *
 * @param KEY The key name to print.
 * @param LIST The list of scoped names to dump.
 *
 * @sa #DUMP_SNAME()
 */
#define DUMP_SNAME_LIST(KEY,LIST) IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": " ); c_sname_list_dump( &(LIST), dump.fout ); )

/**
 * Dumps a C string.
 *
 * @param KEY The key name to print.
 * @param STR The C string to dump.
 *
 * @sa #DUMP_INT()
 * @sa #DUMP_SNAME()
 */
#define DUMP_STR(KEY,STR) IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": " ); fputs_quoted( (STR), '"', dump.fout ); )

/**
 * Dumps a \ref c_tid_t.
 *
 * @param KEY The key name to print.
 * @param TID The \ref c_tid_t to dump.
 *
 * @sa #DUMP_TYPE()
 */
#define DUMP_TID(KEY,TID) IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": " ); c_tid_dump( (TID), dump.fout ); )

/**
 * Dumps a \ref c_type.
 *
 * @param KEY The key name to print.
 * @param TYPE The \ref c_type to dump.
 *
 * @sa #DUMP_TID()
 */
#define DUMP_TYPE(KEY,TYPE) IF_CDECL_DEBUG( \
  DUMP_KEY( KEY ": " ); c_type_dump( &(TYPE), dump.fout ); )

/** @} */

///////////////////////////////////////////////////////////////////////////////

/**
 * @addtogroup parser-group
 * @{
 */

/**
 * Inherited attributes.
 *
 * @remarks These are grouped into a `struct` (rather than having them as
 * separate global variables) so that they can all be reset (mostly) via a
 * single assignment from `{0}`.
 *
 * @sa ia_cleanup()
 */
struct in_attr {
  c_alignas_t     align;            ///< Alignment, if any.
  unsigned        ast_depth;        ///< Parentheses nesting depth.
  bool            is_typename;      ///< C++ only: is `typename` specified?
  c_sname_t       scope_sname;      ///< C++ only: current scope name, if any.

  /**
   * Type AST stack.
   *
   * @sa ia_type_ast_peek()
   * @sa ia_type_ast_pop()
   * @sa ia_type_ast_push()
   */
  c_ast_list_t    type_ast_stack;

  /**
   * Declaration type specifier AST.
   *
   * @remarks A C/C++ declaration is of the form:
   *
   *      type-specifier declarator+
   *
   * where one or more declarators all have the same base type, e.g.:
   *
   *      int i, *p, a[2], f(double);
   *
   * This is a pointer to the base type AST (above, `int`) that is duplicated
   * for each declarator.  It's set by the first call to ia_type_ast_push().
   *
   * @sa ia_type_spec_ast()
   */
  c_ast_t const  *type_spec_ast;

  /**
   * Red-black node for temporary `typedef`.
   *
   * @remarks This is used only in the `pc99_func_or_constructor_declaration_c`
   * grammar production. It's needed because a temporary `typedef` needs to be
   * created in a mid-rule action then removed in the end-rule action and
   * there's no way to pass local variables between Bison actions, so it's
   * passed via an inherited attribute.
   */
  rb_node_t      *tdef_rb;
};
typedef struct in_attr in_attr_t;

// local functions
NODISCARD
static bool l_c_sname_is_type( int, c_sname_t const*, c_loc_t const* );

PJL_PRINTF_LIKE_FUNC(3)
static void l_elaborate_error( int, dym_kind_t, char const*, ... );

PJL_DISCARD
static bool print_error_token( char const* );

// local variables
static c_ast_list_t   gc_ast_list;      ///< c_ast nodes freed after parse.
static in_attr_t      in_attr;          ///< Inherited attributes.
static c_ast_list_t   typedef_ast_list; ///< List of ASTs for `typedef`s.

////////// inline functions ///////////////////////////////////////////////////

/**
 * Duplicates \a ast and adds it to \ref gc_ast_list.
 *
 * @param ast The AST to duplicate.
 * @return Returns the duplicated AST.
 *
 * @sa c_ast_new_gc()
 */
static inline c_ast_t* c_ast_dup_gc( c_ast_t const *ast ) {
  return c_ast_dup( ast, &gc_ast_list );
}

/**
 * Garbage-collects the AST nodes on \a ast_list but does _not_ free \a
 * ast_list itself.
 *
 * @param ast_list The AST list to free the nodes of.
 *
 * @sa c_ast_list_cleanup()
 * @sa c_ast_dup_gc()
 * @sa c_ast_new_gc()
 * @sa c_ast_pair_new_gc()
 */
static inline void c_ast_list_cleanup_gc( c_ast_list_t *ast_list ) {
  slist_cleanup( ast_list, POINTER_CAST( slist_free_fn_t, &c_ast_free ) );
}

/**
 * Creates a new AST and adds it to \ref gc_ast_list.
 *
 * @param kind The kind of AST to create.
 * @param loc A pointer to the token location data.
 * @return Returns a pointer to a new AST.
 *
 * @sa c_ast_dup_gc()
 * @sa c_ast_pair_new_gc()
 */
NODISCARD
static inline c_ast_t* c_ast_new_gc( c_ast_kind_t kind, c_loc_t const *loc ) {
  return c_ast_new( kind, in_attr.ast_depth, loc, &gc_ast_list );
}

/**
 * Set our mode to deciphering gibberish into English.
 *
 * @sa is_english_to_gibberish()
 * @sa is_gibberish_to_english()
 */
static inline void gibberish_to_english( void ) {
  lexer_find &= ~LEXER_FIND_CDECL_KEYWORDS;
}

/**
 * Peeks at the type AST at the top of the
 * \ref in_attr.type_ast_stack "type AST inherited attribute stack".
 *
 * @return Returns said AST.
 *
 * @sa ia_type_ast_pop()
 * @sa ia_type_ast_push()
 */
NODISCARD
static inline c_ast_t* ia_type_ast_peek( void ) {
  return slist_front( &in_attr.type_ast_stack );
}

/**
 * Pops a type AST from the
 * \ref in_attr.type_ast_stack "type AST inherited attribute stack".
 *
 * @return Returns said AST.
 *
 * @sa ia_type_ast_peek()
 * @sa ia_type_ast_push()
 */
PJL_DISCARD
static inline c_ast_t* ia_type_ast_pop( void ) {
  c_ast_t *const ast = slist_pop_front( &in_attr.type_ast_stack );
  assert( ast != NULL );
  return ast;
}

/**
 * Given an AST for a type that is the "of type", return type, or "to type" of
 * a declaration, returns a duplicate of \a type_ast if necessary.
 *
 * @param type_ast The AST of a type.
 * @return If \a type_ast `==` \ref in_attr::type_spec_ast "type_spec_ast",
 * returns a duplicate of \a type_ast; otherwise returns \a type_ast.
 *
 * @sa \ref in_attr::type_spec_ast
 */
NODISCARD
static inline c_ast_t* ia_type_spec_ast( c_ast_t *type_ast ) {
  assert( type_ast != NULL );
  // Yes, == is correct here: we mean the same AST node.
  return type_ast == in_attr.type_spec_ast ?
    c_ast_dup_gc( type_ast ) : type_ast;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints a warning that the attribute \a keyword syntax is not supported (and
 * ignored).
 *
 * @param keyword The attribute syntax keyword, e.g., `__attribute__` or
 * `__declspec`.
 * @param keyword_loc The source location of \a keyword.
 */
static void attr_syntax_not_supported( char const *keyword,
                                       c_loc_t const *keyword_loc ) {
  assert( keyword != NULL );
  assert( keyword_loc != NULL );

  print_warning( keyword_loc,
    "\"%s\" not supported by " CDECL " (ignoring)", keyword
  );
  if ( OPT_LANG_IS( ATTRIBUTES ) )
    print_hint( "%s...%s", other_token_c( "[[" ), other_token_c( "]]" ) );
  else
    EPUTC( '\n' );
}

/**
 * A predicate function for slist_free_if() that checks whether \a ast is one
 * that can be garbage collected: if so, c_ast_free()s it.
 *
 * @param ast_node The \ref slist_node pointing to the AST to check.
 * @param data Contains the AST of the type being declared.
 * @return Returns `true` only if \a ast should be removed from the list.
 */
NODISCARD
static bool c_ast_free_if_garbage( slist_node_t *ast_node, void *data ) {
  assert( ast_node != NULL );
  c_ast_t *const ast = ast_node->data;
  assert( ast != NULL );
  c_ast_t const *const type_ast = data;
  assert( type_ast != NULL );

  if ( ast == type_ast || !c_ast_is_orphan( ast ) ) {
    assert( ast->kind != K_PLACEHOLDER );
    return false;
  }

  // We also need to ensure the AST isn't in the type_ast_stack.
  FOREACH_SLIST_NODE( type_node, &in_attr.type_ast_stack ) {
    if ( ast == type_node->data )
      return false;
  } // for

  c_ast_free( ast );
  return true;
}

/**
 * Checks whether `typename` is OK since the type's name is a qualified name.
 *
 * @param ast The AST to check.
 * @return Returns `true` only upon success.
 */
NODISCARD
bool c_ast_is_typename_ok( c_ast_t const *ast ) {
  c_ast_t const *const raw_ast = c_ast_untypedef( ast );
  if ( c_sname_count( &raw_ast->sname ) < 2 ) {
    print_error( &ast->loc, "qualified name expected after \"typename\"\n" );
    return false;
  }
  return true;
}

/**
 * If \a bit_width is &gt; 0 and \a ast is an integral type, sets \a ast's \ref
 * c_bit_field_ast::bit_width "bit_width" to \a bit_width.
 *
 * @remarks This check has to be done now in the parser rather than later in
 * the AST since we need to use the \ref c_ast::bit_field `union` member now.
 *
 * @param ast The AST whose \ref c_bit_field_ast::bit_width "bit_width" to
 * possibly set.
 * @param bit_width The bit-field width.
 * @return Returns `true` only if \a bit_width is &gt; 0 and \a ast if an
 * integral type.
 */
NODISCARD
static bool c_ast_set_bit_field_width( c_ast_t *ast, unsigned bit_width ) {
  assert( ast != NULL );

  if ( bit_width > 0 ) {
    if ( !c_ast_is_integral( ast ) ) {
      print_error( &ast->loc, "invalid bit-field type " );
      print_ast_type_aka( ast, stderr );
      EPRINTF( "; must be an integral %stype\n",
        OPT_LANG_IS( enum_BITFIELDS ) ? "or enumeration " : ""
      );
      return false;
    }
    ast->bit_field.bit_width = bit_width;
  }

  return true;
}

/**
 * Defines a type by adding it to the global set.
 *
 * @param type_ast The AST of the type to define.
 * @param decl_flags The declaration flags to use; must only be one of
 * #C_ENG_DECL, #C_GIB_TYPEDEF, or #C_GIB_USING.
 * @return Returns `true` either if the type was defined or it's equivalent to
 * an existing type; `false` if a different type already exists having the same
 * name.
 */
NODISCARD
static bool define_type( c_ast_t const *type_ast, decl_flags_t decl_flags ) {
  assert( type_ast != NULL );
  assert( is_1_bit_only_in_set( decl_flags, C_TYPE_DECL_ANY ) );

  if ( !c_type_ast_check( type_ast ) )
    return false;

  c_typedef_t *const tdef = RB_DINT( c_typedef_add( type_ast, decl_flags ) );

  if ( tdef->ast == type_ast ) {
    //
    // Type was added: we have to move the AST from the gc_ast_list so it won't
    // be garbage collected at the end of the parse to a separate
    // typedef_ast_list that's freed only at program termination.
    //
    // But first, garbage collect all orphaned AST nodes.  (For a non-type-
    // defining parse, this step isn't necessary since all nodes are freed at
    // the end of the parse anyway.)
    //
    slist_free_if(
      &gc_ast_list,
      &c_ast_free_if_garbage,
      CONST_CAST( void*, type_ast )
    );
    slist_push_list_back( &typedef_ast_list, &gc_ast_list );
    return true;
  }

  //
  // Type was NOT added because a previously declared type having the same name
  // was returned: check if the types are equal.
  //
  // In C, multiple typedef declarations having the same name are allowed only
  // if the types are equivalent:
  //
  //      typedef int T;
  //      typedef int T;                // OK
  //      typedef double T;             // error: types aren't equivalent
  //
  if ( c_ast_equal( type_ast, tdef->ast ) ) {
    // Update the language(s) the type is available in to include opt_lang_id.
    if ( opt_lang_id < c_lang_oldest( tdef->lang_ids ) )
      tdef->lang_ids = c_lang_and_newer( opt_lang_id );
    return true;
  }

  if ( tdef->is_predefined ) {
    print_error( &type_ast->loc,
      "\"%s\" is a predefined type starting in %s\n",
      c_sname_gibberish( &type_ast->sname ),
      c_lang_name( c_lang_oldest( tdef->lang_ids ) )
    );
  } else {
    print_error( &type_ast->loc, "type " );
    print_ast_type_aka( type_ast, stderr );
    EPUTS( " redefinition incompatible with original type \"" );
    print_type_ast( tdef, stderr );
    EPUTS( "\"\n" );
  }

  return false;
}

/**
 * Cleans-up all resources used by \ref in_attr "inherited attributes".
 */
static void ia_cleanup( void ) {
  c_sname_cleanup( &in_attr.scope_sname );
  // Do _not_ pass &c_ast_free for the 2nd argument! All AST nodes were already
  // free'd from the gc_ast_list in parse_cleanup(). Just free the slist nodes.
  slist_cleanup( &in_attr.type_ast_stack, /*free_fn=*/NULL );
  in_attr = (in_attr_t){ 0 };
}

/**
 * Pushes a type AST onto the
 * \ref in_attr.type_ast_stack "type AST inherited attribute  stack".
 *
 * @remarks Additionally, if \a ast is #K_ANY_TYPE_SPECIFIER and \ref
 * in_attr::type_spec_ast "type_spec_ast" is NULL, sets \ref
 * in_attr::type_spec_ast "type_spec_ast" to \a ast.
 *
 * @param ast The AST to push.
 *
 * @sa ia_type_ast_peek()
 * @sa ia_type_ast_pop()
 */
static void ia_type_ast_push( c_ast_t *ast ) {
  assert( ast != NULL );
  slist_push_front( &in_attr.type_ast_stack, ast );
  if ( in_attr.type_spec_ast != NULL )
    return;
  if ( (ast->kind & K_ANY_TYPE_SPECIFIER) != 0 )
    in_attr.type_spec_ast = ast;
}

/**
 * Joins \a type_ast and \a decl_ast into a single AST.
 *
 * @param type_ast The type AST.
 * @param decl_ast The declaration AST.
 * @return Returns the joined AST on success or NULL on error.
 */
NODISCARD
c_ast_t* join_type_decl( c_ast_t *type_ast, c_ast_t *decl_ast ) {
  assert( type_ast != NULL );
  assert( decl_ast != NULL );

  if ( in_attr.is_typename && !c_ast_is_typename_ok( type_ast ) )
    return NULL;

  type_ast = ia_type_spec_ast( type_ast );
  c_type_t type = c_ast_take_type_any( type_ast, &T_TS_typedef );

  //
  // This is for a case like:
  //
  //      explain typedef ...
  //
  bool const is_explain_typedef = c_tid_is_any( type.stids, TS_typedef );

  if ( !is_explain_typedef &&
       type_ast->kind == K_BUILTIN && decl_ast->kind == K_BUILTIN &&
       c_sname_is_type( &decl_ast->sname, &decl_ast->loc ) ) {
    //
    // This checks for a case like:
    //
    //      typedef int T
    //      explain unsigned T          // error
    //
    // This check has to be done now in the parser rather than later in the
    // AST because once type_ast and decl_ast are joined, the fact that
    // decl_ast was a typedef is lost.
    //
    return NULL;
  }

  if ( is_explain_typedef && decl_ast->kind == K_TYPEDEF ) {
    //
    // This is for a case like:
    //
    //      explain typedef int int32_t;
    //
    // that is: explaining an existing typedef.  In order to do that, we have
    // to un-typedef it so we explain the type that it's a typedef for.
    //
    c_ast_t const *const raw_decl_ast = c_ast_untypedef( decl_ast );

    //
    // However, we also have to check whether the typedef being explained is
    // not equivalent to the existing typedef.  This is for a case like:
    //
    //      explain typedef char int32_t;
    //
    if ( !c_ast_equal( type_ast, raw_decl_ast ) ) {
      print_error( &decl_ast->loc, "type " );
      print_ast_type_aka( type_ast, stderr );
      EPUTS( " redefinition incompatible with original type " );
      print_ast_type_aka( decl_ast, stderr );
      EPUTC( '\n' );
      return NULL;
    }

    //
    // Because the raw_decl_ast for the existing type is about to be combined
    // with type_ast, duplicate raw_decl_ast first.
    //
    c_loc_t const *const orig_loc = &decl_ast->loc;
    decl_ast = c_ast_dup_gc( raw_decl_ast );
    decl_ast->loc = *orig_loc;
  }

  c_ast_t *const ast = c_ast_patch_placeholder( type_ast, decl_ast );
  c_type_t const tdef_type = c_ast_take_type_any( ast, &T_TS_typedef );
  c_type_or_eq( &type, &tdef_type );
  c_type_or_eq( &ast->type, &type );

  if ( in_attr.align.kind != C_ALIGNAS_NONE ) {
    ast->align = in_attr.align;
    if ( c_tid_is_any( type.stids, TS_typedef ) ) {
      //
      // We check for illegal aligned typedef here rather than in c_ast_check.c
      // because the "typedef-ness" needed to be removed previously before the
      // eventual call to c_ast_check().
      //
      print_error( &ast->align.loc, "typedef can not be aligned\n" );
      return NULL;
    }
  }

  if ( ast->kind == K_UDEF_CONV &&
       c_sname_local_type( &ast->sname )->btids == TB_SCOPE ) {
    //
    // User-defined conversions don't have names, but they can still have a
    // scope.  Since only classes can have them, if the scope is still
    // TB_SCOPE, change it to TB_class.
    //
    c_sname_local_data( &ast->sname )->type = C_TYPE_LIT_B( TB_class );
  }

  return ast;
}

/**
 * Checks whether \a ast is of kind #K_NAME and an error if:
 *
 *  + \ref opt_permissive_types is `false`; and:
 *  + \ref c_ast::sname is a C or C++ keyword.
 *
 * @note This function isn't normally called directly; use the
 * #c_ast_is_name_error() macro instead.
 *
 * @param line The line number within this file where this function was called
 * from.
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast is a #K_NAME, \ref
 * opt_permissive_types is `false`, and \ref c_ast::sname is a C or C++
 * keyword.
 */
NODISCARD
static bool l_c_ast_is_name_error( int line, c_ast_t const *ast ) {
  assert( ast != NULL );
  if ( ast->kind != K_NAME || opt_permissive_types )
    return false;

  c_keyword_t const *const ck = c_keyword_find(
    c_sname_gibberish( &ast->sname ), LANG_ANY, C_KW_CTX_DEFAULT
  );
  if ( ck == NULL )
    return false;

  assert( !c_sname_empty( &ast->sname ) );
  fl_print_error_unknown_name( __FILE__, line, &ast->loc, &ast->sname );
  return true;
}

/**
 * Checks whether \a sname is a type.
 *
 * @param line The line number within \a file where this function was called
 * from.
 * @param sname The scoped name to check.
 * @param loc The location of \a sname.
 * @return Returns `true` only if \a sname is a type (and prints an error
 * message); otherwise `false` (and does nothing).
 */
NODISCARD
static bool l_c_sname_is_type( int line, c_sname_t const *sname,
                               c_loc_t const *loc ) {
  assert( sname != NULL );
  assert( loc != NULL );

  c_typedef_t const *const tdef = c_typedef_find_sname( sname );
  if ( tdef == NULL )
    return false;

  if ( tdef->is_predefined ) {
    fl_print_error( __FILE__, line, loc,
      "\"%s\" is a predefined type starting in %s\n",
      c_sname_gibberish( sname ),
      c_lang_name( c_lang_oldest( tdef->lang_ids ) )
    );
  } else {
    fl_print_error( __FILE__, line, loc,
      "\"%s\": previously declared as type \"",
      c_sname_gibberish( sname )
    );
    print_type_ast( tdef, stderr );
    EPUTS( "\"\n" );
  }

  return true;
}

/**
 * A special case of l_elaborate_error() that prevents oddly worded error
 * messages where a C/C++ keyword is expected, but that keyword isn't a keyword
 * either until a later version of the language or in a different language;
 * hence, the lexer will return the keyword as the `Y_NAME` token instead of
 * the keyword token.
 *
 * For example, if l_elaborate_error() were used for the following \b cdecl
 * command when the current language is C, you'd get the following:
 * ```
 * declare f as virtual function returning void
 *              ^
 * 14: syntax error: "virtual": "virtual" expected; not a keyword until C++98
 * ```
 * because it's really this:
 * ```
 * ... "virtual" [the name]": "virtual" [the token] expected ...
 * ```
 * and that looks odd.
 *
 * @note This function isn't normally called directly; use the
 * #keyword_expected() macro instead.
 *
 * @param line The line number within this file where this function was called
 * from.
 * @param keyword A keyword literal.
 *
 * @sa l_elaborate_error()
 * @sa l_punct_expected()
 * @sa yyerror()
 */
static void l_keyword_expected( int line, char const *keyword ) {
  assert( keyword != NULL );

  dym_kind_t dym_kinds = DYM_NONE;

  char const *const error_token = printable_yytext();
  if ( error_token != NULL ) {
    if ( strcmp( error_token, keyword ) == 0 ) {
      //
      // The error token is the expected keyword which means the lexer returned
      // it as a name and not the token which likely means it's not a C/C++
      // keyword until a later version of the current language.
      //
      c_keyword_t const *const ck =
        c_keyword_find( keyword, LANG_ANY, C_KW_CTX_DEFAULT );
      if ( ck != NULL ) {
        char const *const which_lang = c_lang_which( ck->lang_ids );
        if ( which_lang[0] != '\0' ) {
          EPUTS( ": " );
          print_debug_file_line( __FILE__, line );
          EPRINTF( "\"%s\" not supported%s\n", keyword, which_lang );
          return;
        }
      }
    }

    dym_kinds = is_english_to_gibberish() ? DYM_CDECL_KEYWORDS : DYM_C_KEYWORDS;
  }

  l_elaborate_error( line, dym_kinds, "\"%s\" expected", keyword );
}

/**
 * Checks whether the type currently being declared (`enum`, `struct`,
 * `typedef`, or `union`) is nested within some other type (`struct` or
 * `union`) and whether the current language is C: if not, prints an error
 * message.
 *
 * @note It is unnecessary to check either when a `class` is being declared or
 * when a type is being declared within a `namespace` since those are not legal
 * in C anyway.
 *
 * @note This function isn't normally called directly; use the
 * #is_nested_type_ok() macro instead.
 *
 * @param line The line number within this file where this function was called
 * from.
 * @param type_loc The location of the type declaration.
 * @return Returns `true` only if the type currently being declared is either
 * not nested or the current language is C++.
 */
NODISCARD
static bool l_is_nested_type_ok( int line, c_loc_t const *type_loc ) {
  assert( type_loc != NULL );
  if ( !c_sname_empty( &in_attr.scope_sname ) &&
       !OPT_LANG_IS( NESTED_TYPES ) ) {
    fl_print_error( __FILE__, line, type_loc,
      "nested types not supported%s\n",
      C_LANG_WHICH( NESTED_TYPES )
    );
    return false;
  }
  return true;
}

/**
 * A special case of l_elaborate_error() that prevents oddly worded error
 * messages when a punctuation character is expected by not doing keyword look-
 * ups of the error token.
 *
 * For example, if l_elaborate_error() were used for the following \b cdecl
 * command, you'd get the following:
 * ```
 * explain void f(int g const)
 *                      ^
 * 29: syntax error: "const": ',' expected ("const" is a keyword)
 * ```
 * and that looks odd since, if a punctuation character was expected, it seems
 * silly to point out that the encountered token is a keyword.
 *
 * @note This function isn't normally called directly; use the
 * #punct_expected() macro instead.
 *
 * @param line The line number within this file where this function was called
 * from.
 * @param punct The punctuation character that was expected.
 *
 * @sa l_elaborate_error()
 * @sa l_keyword_expected()
 * @sa yyerror()
 */
static void l_punct_expected( int line, char punct ) {
  EPUTS( ": " );
  print_debug_file_line( __FILE__, line );
  if ( print_error_token( printable_yytext() ) )
    EPUTS( ": " );
  EPRINTF( "'%c' expected\n", punct );
}

/**
 * Cleans up individial parse data after each parse.
 *
 * @param fatal_error Must be `true` only if a fatal semantic error has
 * occurred and `YYABORT` is about to be called to bail out of parsing by
 * returning from yyparse().
 */
static void parse_cleanup( bool fatal_error ) {
  //
  // We need to reset the lexer differently depending on whether we completed a
  // parse with a fatal error.  If so, do a "hard" reset that also resets the
  // EOF flag of the lexer.
  //
  lexer_reset( /*hard_reset=*/fatal_error );

  if ( fatal_error && yytext[0] != '\n' ) {
    //
    // Generally, PARSE_ABORT() was called before getting to the '\n' at the
    // end of the line, so Flex will not have incremented yylineno: manually
    // increment yylineno here to compensate.
    //
    ++yylineno;
  }

  c_ast_list_cleanup_gc( &gc_ast_list );
  ia_cleanup();
}

/**
 * Called by Bison to print a parsing error message to standard error.
 *
 * @remarks A custom error printing function via `%%define parse.error custom`
 * and
 * [`yyreport_syntax_error()`](https://www.gnu.org/software/bison/manual/html_node/Syntax-Error-Reporting-Function.html)
 * is not done because printing a (perhaps long) list of all the possible
 * expected tokens isn't helpful.
 * @par
 * It's also more flexible to be able to call one of #elaborate_error(),
 * #keyword_expected(), or #punct_expected() at the point of the error rather
 * than having a single function try to figure out the best type of error
 * message to print.
 *
 * @note A newline is _not_ printed since the error message will be appended to
 * by l_elaborate_error().  For example, the parts of an error message are
 * printed by the functions shown:
 *
 *      42: syntax error: "int": "into" expected
 *      |--||----------||----------------------|
 *      |   |           |
 *      |   yyerror()   l_elaborate_error()
 *      |
 *      print_loc()
 *
 * @param msg The error message to print.  Bison invariably passes `syntax
 * error`.
 *
 * @sa l_elaborate_error()
 * @sa l_keyword_expected()
 * @sa l_punct_expected()
 * @sa print_loc()
 */
static void yyerror( char const *msg ) {
  assert( msg != NULL );

  c_loc_t const loc = lexer_loc();
  print_loc( &loc );

  color_start( stderr, sgr_error );
  EPUTS( msg );                         // no newline
  color_end( stderr, sgr_error );

  //
  // A syntax error has occurred, but syntax errors aren't fatal since Bison
  // tries to recover.  We'll clean-up the current parse, but YYABORT won't be
  // called so we won't bail out of parsing by returning from yyparse(); hence,
  // parsing will continue.
  //
  parse_cleanup( /*fatal_error=*/false );
}

/** @} */

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

%}

//
// The difference between "literal" and either "name" or "str_val" is that
// "literal" points to a statically allocated `L_` string literal constant that
// MUST NOT be free'd whereas both "name" and "str_val" point to a malloc'd
// string that MUST be free'd.
//
%union {
  c_alignas_t         align;
  c_ast_t            *ast;            // for the AST being built
  c_ast_list_t        ast_list;       // for declarations & function parameters
  c_ast_pair_t        ast_pair;       // for the AST being built
  c_cast_kind_t       cast_kind;      // C/C++ cast kind
  bool                flag;           // simple flag
  unsigned            flags;          // multipurpose bitwise flags
  char const         *literal;        // token L_* literal for new-style casts
  int                 int_val;        // signed integer value, cf. uint_val
  char               *name;           // identifier name, cf. sname
  c_func_member_t     member;         // member, non-member, or unspecified
  c_op_id_t           op_id;          // overloaded operator ID
  p_arg_list_t       *p_arg_list;     // preprocessor macro argument list
  p_param_t          *p_param;        // preprocessor macro parameter
  p_param_list_t     *p_param_list;   // preprocessor macro parameter list
  p_token_t          *p_token;        // preprocessor token
  p_token_list_t     *p_token_list;   // preprocessor token list
  void               *ptrs[2];        // pair of pointers
  cdecl_show_t        show;           // which types to show
  c_sname_t           sname;          // scoped identifier name, cf. name
  slist_t             sname_list;     // c_sname_t list
  char               *str_val;        // quoted string value
  c_typedef_t const  *tdef;           // typedef
  c_tid_t             tid;            // built-ins, storage classes, qualifiers
  c_type_t            type;           // complete type
  unsigned            uint_val;       // unsigned integer value
}

                    // cdecl commands
%token              Y_cast
//                  Y_class           // covered in C++
//                  Y_const           // covered in C89
%token              Y_constant        // see comment in lexer.l
%token              Y_declare
%token              Y_define
%token              Y_dynamic         // cast
%token              Y_explain
//                  Y_inline          // covered in C99
//                  Y_namespace       // covered in C++
%token              Y_no              // discard, except, return, unique address
%token              Y_quit
%token              Y_reinterpret     // cast
%token              Y_set
%token              Y_show
//                  Y_static          // covered in K&R C
//                  Y_struct          // covered in K&R C
//                  Y_typedef         // covered in K&R C
//                  Y_union           // covered in K&R C
//                  Y_using           // covered in C++

                    // Pseudo-English
%token              Y_aligned
%token              Y_all
%token              Y_array
%token              Y_as
%token              Y_binding
%token              Y_bit
%token              Y_bit_precise
%token              Y_bits
%token              Y_by
%token              Y_bytes
%token              Y_capturing
%token              Y_commands
%token              Y_constructor
%token              Y_conversion
%token              Y_copy
%token              Y_defined
%token              Y_destructor
%token              Y_english
%token              Y_evaluation
%token              Y_expand
%token              Y_expression
%token              Y_floating
%token              Y_function
%token              Y_initialization
%token              Y_into
%token              Y_lambda
%token              Y_length
%token              Y_linkage
%token              Y_literal
%token              Y_macros
%token              Y_member
%token              Y_non_empty
%token              Y_non_member
%token              Y_of
%token              Y_options
%token              Y_point
%token              Y_pointer
%token              Y_precise
%token              Y_precision
%token              Y_predefined
%token              Y_pure
%token              Y_reference
%token              Y_returning
%token              Y_rvalue
%token              Y_scope
%token              Y_structured
%token              Y_to
%token              Y_user
%token              Y_user_defined
%token              Y_variable
%token              Y_wide
%token              Y_width

                    // Precedence
%nonassoc           Y_PREC_LESS_THAN_upc_layout_qualifier

                    //
                    // C and C++ operators that are single-character and have
                    // no alternative token are represented by themselves
                    // directly.  All multi-character operators or those that
                    // have an alternative token are given Y_ names.
                    //

                    // C/C++ operators: precedence 17
%left               Y_COLON_COLON         "::"
                    Y_COLON_COLON_STAR // "::" followed by '*'
                    // C/C++ operators: precedence 16
                    %token              Y_PLUS_PLUS           "++"
%token              Y_MINUS_MINUS         "--"
%left                                     '(' ')'
                                          '[' ']'
                                          '.'
                    Y_MINUS_GREATER       "->"
                    // C/C++ operators: precedence 15
%right                                    '&' // also has alt. token "bitand"
                                          '*'
                                          '!' // also has alt. token "not"
                 // Y_UMINUS          //  '-' -- covered by '-' below
                 // Y_UPLUS           //  '+' -- covered by '+' below
                    Y_sizeof
                                          '~' // also has alt. token "compl"
                    // C/C++ operators: precedence 14
%left               Y_DOT_STAR            ".*"
                    Y_MINUS_GREATER_STAR  "->*"
                    // C/C++ operators: precedence 13
%left                                 //  '*' -- covered by '*' above
                                          '/'
                                          '%'
                    // C/C++ operators: precedence 12
%left                                     '-'
                                          '+'
                    // C/C++ operators: precedence 11
%left               Y_LESS_LESS           "<<"
                    Y_GREATER_GREATER     ">>"
                    // C/C++ operators: precedence 10
%left               Y_LESS_EQUAL_GREATER  "<=>"
                    // C/C++ operators: precedence 9
%left                                     '<'
                                          '>'
                    Y_LESS_EQUAL          "<="
                    Y_GREATER_EQUAL       ">="
                    // C/C++ operators: precedence 8
%left               Y_EQUAL_EQUAL         "=="
                    Y_EXCLAM_EQUAL    //  "!=" -- also has alt. token "not_eq"
                    // C/C++ operators: precedence 7 (covered above)
%left               Y_bit_and         //  '&'  -- covered by '&' above
                    // C/C++ operators: precedence 6
%left                                     '^'  // also has alt. token "xor"
                    // C/C++ operators: precedence 5
%left                                     '|'  // also has alt. token "bitor"
                    // C/C++ operators: precedence 4
%left               Y_AMPER_AMPER     //  "&&" -- also has alt. token "and"
                    // C/C++ operators: precedence 3
%left               Y_PIPE_PIPE       //  "||" -- also has alt. token "or"
                    // C/C++ operators: precedence 2
%right              Y_QMARK_COLON         "?:"
                                          '='
                    Y_PERCENT_EQUAL       "%="
                    Y_AMPER_EQUAL     //  "&=" -- also has alt. token "and_eq"
                    Y_STAR_EQUAL          "*="
                    Y_PLUS_EQUAL          "+="
                    Y_MINUS_EQUAL         "-="
                    Y_SLASH_EQUAL         "/="
                    Y_LESS_LESS_EQUAL     "<<="
                    Y_GREATER_GREATER_EQUAL ">>="
                    Y_CARET_EQUAL     //  "^=" -- also has alt. token "xor_eq"
                    Y_PIPE_EQUAL      //  "|=" -- also has alt. token "or_eq"
                    // C/C++ operators: precedence 1
%left                                     ','

                    // K&R C
%token  <tid>       Y_auto_STORAGE      // pre-C23/C++11 version of "auto"
%token              Y_break
%token              Y_case
%token  <tid>       Y_char
%token              Y_continue
%token  <tid>       Y_default
%token              Y_do
%token  <tid>       Y_double
%token              Y_else
%token  <tid>       Y_extern
%token  <tid>       Y_float
%token              Y_for
%token              Y_goto
%token              Y_if
%token  <tid>       Y_int
%token  <tid>       Y_long
%token  <tid>       Y_register
%token              Y_return
%token  <tid>       Y_short
%token  <tid>       Y_static
%token  <tid>       Y_struct
%token              Y_switch
%token  <tid>       Y_typedef
%token  <tid>       Y_union
%token  <tid>       Y_unsigned
%token              Y_while

                    // C Preprocessor
%token              /* stringify */       '#'
%token              Y_PRE_CONCAT          "##"
%token              Y_PRE_SPACE         // whitespace
%token              Y_PRE_define
%token              Y_PRE_elif
%token              Y_PRE_else
%token              Y_PRE_error
%token              Y_PRE_if
%token              Y_PRE_ifdef
%token              Y_PRE_ifndef
                 // Y_PRE_include       // handled within the lexer
%token              Y_PRE_line
%token              Y_PRE_undef
%token              Y_PRE___VA_ARGS__
%token              Y_PRE___VA_OPT__

                    // C99 preprocessor
%token              Y_PRE_pragma

                    // C23 preprocessor
%token              Y_PRE_elifdef
%token              Y_PRE_elifndef
%token              Y_PRE_embed
%token              Y_PRE_warning

                    // C89
%token              Y_asm
%token  <tid>       Y_const
%token              Y_ELLIPSIS            "..."
%token  <tid>       Y_enum
%token  <tid>       Y_signed
%token  <tid>       Y_void
%token  <tid>       Y_volatile

                    // C95
%token  <tid>       Y_wchar_t           // see comment for TB_wchar_t

                    // C99
%token  <tid>       Y__Bool
%token  <tid>       Y__Complex
%token  <tid>       Y__Imaginary
%token  <tid>       Y_inline
%token  <tid>       Y_restrict

                    // C11
%token              Y__Alignas
%token              Y__Alignof
%token  <tid>       Y__Atomic_QUAL      // qualifier: _Atomic type
%token  <tid>       Y__Atomic_SPEC      // specifier: _Atomic (type)
%token              Y__Generic
%token  <tid>       Y__Noreturn
%token              Y__Static_assert
%token  <tid>       Y__Thread_local
%token              Y_thread Y_local

                    // C++
%token  <tid>       Y_bool
%token              Y_catch
%token  <tid>       Y_class
%token  <literal>   Y_const_cast
%token  <sname>     Y_CONSTRUCTOR_SNAME // e.g., S::T::T
%token  <tid>       Y_delete
%token  <sname>     Y_DESTRUCTOR_SNAME  // e.g., S::T::~T
%token  <literal>   Y_dynamic_cast
%token  <tid>       Y_explicit
%token  <tid>       Y_false             // noexcept(false)
%token  <tid>       Y_friend
%token  <tid>       Y_mutable
%token  <tid>       Y_namespace
%token              Y_new
%token              Y_operator
%token  <sname>     Y_OPERATOR_SNAME    // e.g., S::T::operator
%token              Y_private
%token              Y_protected
%token              Y_public
%token  <literal>   Y_reinterpret_cast
%token  <literal>   Y_static_cast
%token              Y_template
%token  <tid>       Y_this
%token  <tid>       Y_throw
%token  <tid>       Y_true              // noexcept(true)
%token              Y_try
%token              Y_typeid
%token  <flag>      Y_typename
%token  <tid>       Y_using
%token  <tid>       Y_virtual

                    // C11 & C++11
%token  <tid>       Y_char16_t          // see comment for TB_char16_t
%token  <tid>       Y_char32_t          // see comment for TB_char32_t

                    // C23
%token  <tid>       Y__BitInt
%token  <tid>       Y_reproducible
%token              Y_typeof
%token              Y_typeof_unqual
%token  <tid>       Y_unsequenced

                    // C23 & C++11
%token              Y_ATTR_BEGIN        // first '[' of "[[" for an attribute
%token  <tid>       Y_auto_TYPE         // C23/C++11 version of "auto"

                    // C++11
%token              Y_alignas
%token              Y_alignof
%token              Y_carries Y_dependency
%token  <tid>       Y_carries_dependency
%token  <tid>       Y_constexpr
%token              Y_decltype
%token              Y_except
%token  <tid>       Y_final
%token  <tid>       Y_noexcept
%token              Y_nullptr
%token  <tid>       Y_override
%token              Y_static_assert
%token  <tid>       Y_thread_local

                    // C23 & C++14
%token  <tid>       Y_deprecated

                    // C++17
%token              Y_auto_STRUCTURED_BINDING

                    // C23 & C++17
%token              Y_discard           // no discard
%token  <tid>       Y_maybe_unused
%token              Y_maybe Y_unused
%token  <tid>       Y_nodiscard
%token  <tid>       Y_noreturn

                    // C23 & C++20
%token  <tid>       Y_char8_t           // see comment for TB_char8_t

                    // C++20
%token              Y_concept
%token  <tid>       Y_consteval
%token  <tid>       Y_constinit
%token              Y_co_await
%token              Y_co_return
%token              Y_co_yield
%token  <tid>       Y_export
%token  <tid>       Y_no_unique_address
%token              Y_parameter Y_pack
%token              Y_requires
%token              Y_unique Y_address  // no unique address

                    // C++26
%token  <tid>       Y_indeterminate

                    // Embedded C extensions
%token  <tid>       Y_EMC__Accum
%token  <tid>       Y_EMC__Fract
%token  <tid>       Y_EMC__Sat

                    // Unified Parallel C extensions
%token  <tid>       Y_UPC_relaxed
%token  <tid>       Y_UPC_shared
%token  <tid>       Y_UPC_strict

                    // GNU extensions
%token              Y_GNU___attribute__
%token  <tid>       Y_GNU___restrict

                    // Apple extensions
%token  <tid>       Y_Apple___block     // __block storage class
%token              Y_Apple_block       // English for '^'

                    // Microsoft extensions
%token  <tid>       Y_MSC___cdecl
%token  <tid>       Y_MSC___clrcall
%token              Y_MSC___declspec
%token  <tid>       Y_MSC___fastcall
%token  <tid>       Y_MSC___stdcall
%token  <tid>       Y_MSC___thiscall
%token  <tid>       Y_MSC___vectorcall

                    // Miscellaneous
%token              ':'
%token              ';'
%token              '{' '}'
%token  <str_val>   Y_CHAR_LIT          // must be free'd
%token  <sname>     Y_CONCEPT_SNAME
%token              Y_END
%token              Y_ERROR
%token              Y_FLOAT_LIT
%token  <name>      Y_GLOB              // must be free'd
%token  <int_val>   Y_INT_LIT
%token  <name>      Y_NAME              // must be free'd
%token  <name>      Y_SET_OPTION        // must be free'd
%token  <str_val>   Y_STR_LIT           // must be free'd
%token  <tdef>      Y_TYPEDEF_NAME_TDEF // e.g., size_t
%token  <tdef>      Y_TYPEDEF_SNAME_TDEF// e.g., std::string

                    //
                    // When the lexer returns Y_LEXER_ERROR, it means that
                    // there was a lexical error and that it's already printed
                    // an error message so the parser should NOT print an error
                    // message and just call PARSE_ABORT().
                    ///
%token              Y_LEXER_ERROR

//
// Grammar rules are named according to the following conventions.  In order,
// if a rule:
//
//  1. Is a list, "_list" is appended.
//  2. Is specific to C/C++, "_c" is appended; is specific to pseudo-English,
//     "_english" is appended.
//  3. Is of type:
//      + <ast>: "_ast" is appended.
//      + <ast_list>: (See #1.)
//      + <ast_pair>: "_astp" is appended.
//      + <flag>: "_flag" is appended.
//      + <flags>: "_flags" is appended.
//      + <name>: "_name" is appended.
//      + <literal>: "_literal" is appended.
//      + <sname>: "_sname" is appended.
//      + <tdef>: "_tdef" is appended.
//      + <tid>: "_[bsa]tid" is appended.
//      + <type>: "_type" is appended.
//      + <uint_val>: "_uint" is appended.
//  4. Is expected, "_exp" is appended; is optional, "_opt" is appended.
//

// Sort using: sort -bdfk3

                      // Pseudo-English
%type   <ast>         alignas_or_width_decl_english_ast
%type   <align>       alignas_specifier_english
%type   <ast>         array_decl_english_ast
%type   <ast>         array_size_decl_ast
%type   <tid>         attribute_english_atid
%type   <uint_val>    BitInt_english_int
%type   <ast>         block_decl_english_ast
%type   <tid>         builtin_no_BitInt_english_btid
%type   <ast>         builtin_type_english_ast
%type   <ast>         capture_decl_english_ast
%type   <ast_list>    capture_decl_list_english capture_decl_list_english_opt
%type   <ast_list>    capturing_paren_capture_decl_list_english_opt
%type   <ast>         class_struct_union_english_ast
%type   <ast>         concept_type_english_ast
%type   <ast>         constructor_decl_english_ast
%type   <ast>         decl_english_ast decl_english_ast_exp
%type   <ast>         destructor_decl_english_ast
%type   <ast>         enum_english_ast
%type   <ast>         enum_fixed_type_english_ast
%type   <tid>         enum_fixed_type_modifier_list_english_btid
%type   <tid>         enum_fixed_type_modifier_list_english_btid_opt
%type   <ast>         enum_unmodified_fixed_type_english_ast
%type   <ast>         func_decl_english_ast
%type   <type>        func_qualifier_english_type_opt
%type   <member>      member_or_non_member_opt
%type   <cast_kind>   new_style_cast_english
%type   <sname>       of_scope_english
%type   <sname>       of_scope_list_english of_scope_list_english_opt
%type   <ast>         of_type_enum_fixed_type_english_ast_opt
%type   <ast_list>    param_decl_list_english param_decl_list_english_opt
%type   <ast>         parameter_pack_english_ast
%type   <ast_list>    paren_capture_decl_list_english
%type   <ast_list>    paren_param_decl_list_english
%type   <ast_list>    paren_param_decl_list_english_opt
%type   <ast>         pc99_pointer_decl_c
%type   <ast_list>    pc99_pointer_decl_list_c
%type   <ast>         pointer_decl_english_ast
%type   <ast>         qualifiable_decl_english_ast
%type   <ast>         qualified_decl_english_ast
%type   <ast>         reference_decl_english_ast
%type   <ast>         reference_english_ast
%type   <tid>         ref_qualifier_english_stid_opt
%type   <ast>         returning_english_ast returning_english_ast_opt
%type   <type>        scope_english_type scope_english_type_exp
%type   <sname>       sname_english sname_english_exp sname_english_opt
%type   <ast>         sname_english_ast
%type   <sname_list>  sname_list_english
%type   <tid>         storage_class_english_stid
%type   <tid>         storage_class_subset_english_stid
%type   <type>        storage_class_subset_english_type
%type   <type>        storage_class_subset_english_type_opt
%type   <ast>         structured_binding_decl_english_ast
%type   <ast>         type_english_ast
%type   <type>        type_modifier_english_type
%type   <type>        type_modifier_list_english_type
%type   <type>        type_modifier_list_english_type_opt
%type   <tid>         type_qualifier_english_stid
%type   <type>        type_qualifier_english_type
%type   <type>        type_qualifier_list_english_type
%type   <type>        type_qualifier_list_english_type_opt
%type   <ast>         unmodified_type_english_ast
%type   <ast>         user_defined_literal_decl_english_ast
%type   <ast>         var_decl_english_ast
%type   <uint_val>    width_specifier_english_uint

                      // C Preprocessor
%type  <ptrs>         expand_command2
%type  <p_arg_list>   p_arg_list p_arg_list_opt
%type  <p_token>      p_arg_token
%type  <p_token_list> p_arg_token_as_list
%type  <p_token>      p_arg_token_with_comma
%type  <p_token_list> p_arg_token_with_comma_as_list
%type  <p_token_list> p_arg_tokens
%type  <p_token_list> p_arg_tokens_with_comma p_arg_tokens_with_comma_opt
%type  <p_arg_list>   p_comma_arg_list
%type  <p_token>      p_extra_token
%type  <p_token>      p_extra_token_except_lparen
%type  <p_token_list> p_extra_tokens p_extra_tokens_opt
%type  <p_param>      p_param
%type  <p_param_list> p_param_list p_param_list_opt
%type  <p_param_list> p_paren_param_list_opt
%type  <p_token_list> p_replace_list p_replace_list_opt
%type  <p_token>      p_replace_token

                      // C/C++ casts
%type   <ast_pair>    array_cast_c_astp
%type   <ast_pair>    block_cast_c_astp
%type   <ast_pair>    cast_c_astp cast_c_astp_opt cast2_c_astp
%type   <ast_pair>    func_cast_c_astp
%type   <ast_pair>    nested_cast_c_astp
%type   <cast_kind>   new_style_cast_c
%type   <ast>         param_pack_cast_c_ast
%type   <ast_pair>    pointer_cast_c_astp
%type   <ast_pair>    pointer_to_member_cast_c_astp
%type   <ast_pair>    reference_cast_c_astp

                      // C/C++ declarations
%type   <align>       alignas_specifier_c
%type   <sname>       any_sname_c any_sname_c_exp any_sname_c_opt
%type   <ast_pair>    array_decl_c_astp
%type   <ast>         array_size_c_ast
%type   <ast>         atomic_specifier_type_c_ast
%type   <tid>         attribute_c_atid_exp
%type   <tid>         attribute_list_c_atid attribute_list_c_atid_opt
%type   <tid>         attribute_specifier_list_c_atid
%type   <tid>         attribute_specifier_list_c_atid_opt
%type   <uint_val>    bit_field_c_uint_opt
%type   <ast_pair>    block_decl_c_astp
%type   <tid>         builtin_no_BitInt_c_btid
%type   <ast>         builtin_type_c_ast
%type   <ast>         capture_decl_c_ast
%type   <ast_list>    capture_decl_list_c capture_decl_list_c_opt
%type   <ast>         class_struct_union_c_ast
%type   <ast>         concept_type_c_ast
%type   <ast>         decl_c decl_c_exp
%type   <ast_pair>    decl_c_astp decl2_c_astp
%type   <ast_list>    decl_list_c
%type   <sname>       destructor_sname
%type   <ast>         east_modifiable_type_c_ast
%type   <ast>         east_modified_type_c_ast
%type   <ast>         enum_c_ast
%type   <ast>         enum_class_struct_union_c_ast
%type   <ast>         enum_fixed_type_c_ast enum_fixed_type_c_ast_opt
%type   <tid>         enum_fixed_type_modifier_btid
%type   <tid>         enum_fixed_type_modifier_list_btid
%type   <tid>         enum_fixed_type_modifier_list_btid_opt
%type   <ast>         enum_unmodified_fixed_type_c_ast
%type   <tid>         extern_linkage_c_stid extern_linkage_c_stid_opt
%type   <ast_pair>    func_decl_c_astp
%type   <tid>         func_equals_c_stid_opt
%type   <tid>         func_qualifier_c_stid
%type   <tid>         func_qualifier_list_c_stid_opt
%type   <ast>         lambda_return_type_c_ast_opt
%type   <tid>         linkage_stid
%type   <ast_pair>    nested_decl_c_astp
%type   <tid>         noexcept_c_stid_opt
%type   <ast_pair>    oper_decl_c_astp
%type   <sname>       oper_sname_c_opt
%type   <ast>         param_c_ast param_c_ast_exp
%type   <ast_list>    param_c_ast_list param_c_ast_list_exp param_c_ast_list_opt
%type   <ast>         param_pack_decl_c_ast
%type   <ast_list>    paren_param_c_ast_list_opt
%type   <ast>         pc99_pointer_type_c_ast
%type   <ast_pair>    pointer_decl_c_astp
%type   <ast_pair>    pointer_to_member_decl_c_astp
%type   <ast>         pointer_to_member_type_c_ast
%type   <ast>         pointer_type_c_ast
%type   <tid>         ref_qualifier_c_stid_opt
%type   <ast_pair>    reference_decl_c_astp
%type   <ast>         reference_type_c_ast
%type   <tid>         restrict_qualifier_c_stid
%type   <tid>         param_list_rparen_func_qualifier_list_c_stid_opt
%type   <sname>       sname_c sname_c_exp sname_c_opt
%type   <ast>         sname_c_ast
%type   <type>        storage_class_c_type
%type   <ast>         structured_binding_type_c_ast
%type   <sname>       sub_scope_sname_c_opt
%type   <ast>         trailing_return_type_c_ast_opt
%type   <ast>         type_c_ast
%type   <sname>       typedef_sname_c
%type   <ast>         typedef_type_c_ast
%type   <ast>         typedef_type_decl_c_ast
%type   <type>        type_modifier_c_type
%type   <type>        type_modifier_list_c_type type_modifier_list_c_type_opt
%type   <flag>        typeof
%type   <ast>         typeof_type_c_ast
%type   <tid>         type_qualifier_c_stid
%type   <tid>         type_qualifier_list_c_stid type_qualifier_list_c_stid_opt
%type   <ast_pair>    user_defined_conversion_decl_c_astp
%type   <ast_pair>    user_defined_literal_decl_c_astp
%type   <ast>         using_decl_c_ast

                      // C++ user-defined conversions
%type   <ast>         pointer_to_member_udc_decl_c_ast
%type   <ast>         pointer_udc_decl_c_ast
%type   <ast>         reference_udc_decl_c_ast
%type   <ast>         udc_decl_c_ast udc_decl_c_ast_opt

                      // Microsoft extensions
%type   <tid>         msc_calling_convention_atid
%type   <ast_pair>    msc_calling_convention_c_astp

                      // Miscellaneous
%type   <tid>         _Noreturn_atid
%type   <name>        any_name any_name_exp
%type   <tdef>        any_typedef_tdef
%type   <tid>         class_struct_btid class_struct_btid_opt
%type   <tid>         class_struct_union_btid class_struct_union_btid_exp
%type   <op_id>       c_operator
%type   <tid>         cv_qualifier_stid cv_qualifier_list_stid_opt
%type   <tid>         enum_btids
%type   <tid>         eval_expr_init_stid
%type   <name>        glob glob_opt
%type   <name>        help_what_opt
%type   <tid>         inline_stid_opt
%type   <ast>         name_ast
%type   <name>        name_cat name_exp name_opt
%type   <tid>         namespace_btid_exp
%type   <sname>       namespace_sname_c namespace_sname_c_exp
%type   <type>        namespace_type
%type   <sname>       namespace_typedef_sname_c
%type   <tid>         noexcept_bool_stid_exp
%type   <show>        predefined_or_user_types_opt
%type   <str_val>     set_option_value_opt
%type   <flags>       show_format show_format_exp show_format_opt
%type   <show>        show_which_opt
%type   <sname_list>  sname_list_c
%type   <tid>         static_stid_opt
%type   <str_val>     str_lit str_lit_exp
%type   <tid>         this_stid_opt
%type   <type>        type_modifier_base_type
%type   <flag>        typename_flag_opt
%type   <uint_val>    uint_lit uint_lit_exp uint_lit_opt
%type   <tid>         virtual_stid_exp virtual_stid_opt

//
// Bison %destructors.
//
// Clean-up of AST nodes is done via garbage collection using gc_ast_list.
//
%destructor { DTRACE; c_ast_list_cleanup( &$$ );              } <ast_list>
%destructor { DTRACE; FREE( $$ );                             } <name>
%destructor { DTRACE; p_arg_list_cleanup( $$ );   FREE( $$ ); } <p_arg_list>
%destructor { DTRACE; p_param_free( $$ );                     } <p_param>
%destructor { DTRACE; p_param_list_cleanup( $$ ); FREE( $$ ); } <p_param_list>
%destructor { DTRACE; p_token_free( $$ );                     } <p_token>
%destructor { DTRACE; p_token_list_cleanup( $$ ); FREE( $$ ); } <p_token_list>
%destructor { DTRACE; c_sname_cleanup( &$$ );                 } <sname>
%destructor { DTRACE; c_sname_list_cleanup( &$$ );            } <sname_list>
%destructor { DTRACE; FREE( $$ );                             } <str_val>

///////////////////////////////////////////////////////////////////////////////
%%

command_list
  : /* empty */
  | command_list command
    { //
      // We get here only after a successful parse.
      //
      parse_cleanup( /*fatal_error=*/false );
    }
  ;

command
  : cast_command semi_or_end
  | declare_command semi_or_end
  | define_command semi_or_end
  | expand_command semi_or_end
  | explain_command semi_or_end
  | help_command semi_or_end
  | preprocessor_command Y_END
  | quit_command semi_or_end
  | scoped_command
  | set_command semi_or_end
  | show_command semi_or_end
  | template_command semi_or_end
  | typedef_command semi_or_end
  | using_command semi_or_end
  | semi_or_end                         // allows for blank lines
  | Y_LEXER_ERROR                 { PARSE_ABORT(); }
  | error
    {
      if ( printable_yytext() != NULL )
        elaborate_error_dym( DYM_COMMANDS, "unexpected token" );
      else
        elaborate_error( "unexpected end of command" );
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  COMMANDS                                                                 //
///////////////////////////////////////////////////////////////////////////////

/// cast command //////////////////////////////////////////////////////////////

cast_command
    /*
     * C-style cast.
     */
  : Y_cast sname_english_opt[sname] as_into_to_exp decl_english_ast[decl_ast]
    {
      DUMP_START( "cast_command",
                  "CAST sname_english_opt {AS|[IN]TO} decl_english_ast" );
      DUMP_SNAME( "sname_english_opt", $sname );
      DUMP_AST( "decl_english_ast", $decl_ast );

      c_ast_t *const cast_ast = c_ast_new_gc( K_CAST, &@sname );
      cast_ast->sname = c_sname_move( &$sname );
      cast_ast->cast.kind = C_CAST_C;
      cast_ast->cast.to_ast = $decl_ast;

      DUMP_AST( "$$_ast", cast_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( cast_ast ) );
      c_ast_gibberish( cast_ast, C_GIB_PRINT_CAST, stdout );
    }

    /*
     * New C++-style cast.
     */
  | new_style_cast_english[cast_kind] sname_english_exp[sname] as_into_to_exp
    decl_english_ast[decl_ast]
    {
      DUMP_START( "cast_command",
                  "new_style_cast_english sname_english_exp {AS|[IN]TO} "
                  "decl_english_ast" );
      DUMP_STR( "new_style_cast_english", c_cast_gibberish( $cast_kind ) );
      DUMP_SNAME( "sname_english_exp", $sname );
      DUMP_AST( "decl_english_ast", $decl_ast );

      c_ast_t *const cast_ast = c_ast_new_gc( K_CAST, &@$ );
      cast_ast->sname = c_sname_move( &$sname );
      cast_ast->cast.kind = $cast_kind;
      cast_ast->cast.to_ast = $decl_ast;

      DUMP_AST( "$$_ast", cast_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( cast_ast ) );
      c_ast_gibberish( cast_ast, C_GIB_PRINT_CAST, stdout );
    }
  ;

new_style_cast_english
  : Y_const cast_exp              { $$ = C_CAST_CONST;        }
  | Y_dynamic cast_exp            { $$ = C_CAST_DYNAMIC;      }
  | Y_reinterpret cast_exp        { $$ = C_CAST_REINTERPRET;  }
  | Y_static cast_exp             { $$ = C_CAST_STATIC;       }
  | new_style_cast_c
  ;

/// declare command ///////////////////////////////////////////////////////////

declare_command
    /*
     * Common declaration, e.g.: declare x as int.
     */
  : Y_declare sname_list_english[sname_list] as_exp
    alignas_or_width_decl_english_ast[decl_ast]
    {
      if ( c_ast_is_name_error( $decl_ast ) ) {
        //
        // This checks for a case like:
        //
        //      declare x as y
        //
        // i.e., declaring a variable name as another name (unknown type).
        // This can get this far due to the nature of the C/C++ grammar.
        //
        // This check has to be done now in the parser rather than later in the
        // AST because the name of the AST node needs to be set to the variable
        // name, but the AST node is itself a name and overwriting it would
        // lose information.
        //
        c_sname_list_cleanup( &$sname_list );
        PARSE_ABORT();
      }

      DUMP_START( "declare_command",
                  "DECLARE sname_list_english AS "
                  "alignas_or_width_decl_english_ast" );
      DUMP_SNAME_LIST( "sname_list_english", $sname_list );
      DUMP_AST( "alignas_or_width_decl_english_ast", $decl_ast );

      slist_t *sname_list = &$sname_list;
      c_ast_t *const struct_bind_ast = c_ast_unreference_any( $decl_ast );

      if ( struct_bind_ast->kind == K_STRUCTURED_BINDING ) {
        //
        // For ref-qualified structured bindings, it's much simpler to
        // retroactively change $decl_ast to the structured binding AST
        // directly eliding the reference AST and fold in the reference type.
        //
        struct_bind_ast->type.stids |= $decl_ast->type.stids;
        switch ( $decl_ast->kind ) {
          case K_REFERENCE:
            struct_bind_ast->type.stids |= TS_REFERENCE;
            break;
          case K_RVALUE_REFERENCE:
            struct_bind_ast->type.stids |= TS_RVALUE_REFERENCE;
            break;
          default:
            /* suppress warning */;
        } // switch
        $decl_ast = struct_bind_ast;
        c_ast_set_parent( $decl_ast, /*parent_ast=*/NULL );

        //
        // A structured binding is inverted: instead of having one structured
        // binding AST for each name, we have just one structured binding AST
        // with a list of name(s).
        //
        $decl_ast->struct_bind.sname_list = slist_move( sname_list );
        sname_list = &$decl_ast->struct_bind.sname_list;
      }

      DUMP_AST( "$$_ast", $decl_ast );
      DUMP_END();

      $decl_ast->loc = @sname_list;

      bool ok = true;
      //
      // Ensure that none of the names aren't of a previously declared type:
      //
      //      cdecl> struct S
      //      cdecl> declare S as int // error: "S": previously declared
      //
      // This check is done now in the parser rather than later in the AST
      // since similar checks are also done here in the parser.
      //
      FOREACH_SLIST_NODE( sname_node, sname_list ) {
        c_sname_t const *const sname = sname_node->data;
        if ( c_sname_is_type( sname, &$decl_ast->loc ) ) {
          ok = false;
          break;
        }
      } // for

      if ( ok ) {
        // To check the declaration, it needs a name: just dup the first one.
        c_sname_t temp_sname = c_sname_dup( slist_front( sname_list ) );
        c_sname_set( &$decl_ast->sname, &temp_sname );
        ok = c_ast_check( $decl_ast );
      }

      if ( ok ) {
        if ( $decl_ast->kind == K_STRUCTURED_BINDING ) {
          decl_flags_t decl_flags = C_GIB_PRINT_DECL;
          if ( opt_semicolon )
            decl_flags |= C_GIB_OPT_SEMICOLON;
          c_ast_gibberish( $decl_ast, decl_flags, stdout );
        }
        else {
          c_ast_sname_list_gibberish( $decl_ast, &$sname_list, stdout );
        }
      }

      c_sname_list_cleanup( &$sname_list );
      PARSE_ASSERT( ok );
      PUTC( '\n' );
    }

    /*
     * C++ overloaded operator declaration.
     */
  | Y_declare c_operator[op_id]
    { //
      // This check is done now in the parser right here mid-rule rather than
      // later in the end-rule action or in the AST since it yields a better
      // error message since otherwise it would warn that "operator" is a
      // keyword in C++98 which skims right past the bigger error that operator
      // overloading isn't supported in C.
      //
      if ( !OPT_LANG_IS( operator ) ) {
        print_error( &@op_id,
          "operator overloading not supported%s\n",
          C_LANG_WHICH( operator )
        );
        PARSE_ABORT();
      }
    }
    of_scope_list_english_opt[scope_sname] as_exp
    type_qualifier_list_english_type_opt[qual_type]
    ref_qualifier_english_stid_opt[ref_qual_stid]
    member_or_non_member_opt[member] operator_exp
    paren_param_decl_list_english_opt[param_ast_list]
    returning_english_ast_opt[ret_ast]
    {
      c_operator_t const *const operator = c_op_get( $op_id );

      DUMP_START( "declare_command",
                  "DECLARE c_operator of_scope_list_english_opt AS "
                  "type_qualifier_list_english_type_opt "
                  "ref_qualifier_english_stid_opt "
                  "member_or_non_member_opt "
                  "OPERATOR paren_param_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_STR( "c_operator", operator->literal );
      DUMP_SNAME( "of_scope_list_english_opt", $scope_sname );
      DUMP_TYPE( "type_qualifier_list_english_type_opt", $qual_type );
      DUMP_TID( "ref_qualifier_english_stid_opt", $ref_qual_stid );
      DUMP_INT( "member_or_non_member_opt", $member );
      DUMP_AST_LIST( "paren_param_decl_list_english_opt", $param_ast_list );
      DUMP_AST( "returning_english_ast_opt", $ret_ast );
      DUMP_END();

      c_ast_t *const oper_ast = c_ast_new_gc( K_OPERATOR, &@op_id );
      c_sname_set( &oper_ast->sname, &$scope_sname );
      PARSE_ASSERT( c_type_add( &oper_ast->type, &$qual_type, &@qual_type ) );
      PARSE_ASSERT(
        c_type_add_tid( &oper_ast->type, $ref_qual_stid, &@ref_qual_stid )
      );
      oper_ast->oper.operator = operator;
      c_ast_list_set_param_of( &$param_ast_list, oper_ast );
      oper_ast->oper.param_ast_list = slist_move( &$param_ast_list );
      oper_ast->oper.member = $member;
      c_ast_set_parent( $ret_ast, oper_ast );

      PARSE_ASSERT( c_ast_check( oper_ast ) );
      decl_flags_t decl_flags = C_GIB_PRINT_DECL;
      if ( opt_semicolon )
        decl_flags |= C_GIB_OPT_SEMICOLON;
      c_ast_gibberish( oper_ast, decl_flags, stdout );
      PUTC( '\n' );
    }

  /*
   * C++ lambda.
   */
  | Y_declare storage_class_subset_english_type_opt[store_type] Y_lambda
    capturing_paren_capture_decl_list_english_opt[capture_ast_list]
    paren_param_decl_list_english_opt[param_ast_list]
    returning_english_ast_opt[ret_ast]
    {
      DUMP_START( "declare_command",
                  "DECLARE storage_class_subset_english_type_opt "
                  "LAMBDA capturing_paren_capture_decl_list_english_opt "
                  "paren_param_decl_list_english_opt"
                  "returning_english_ast_opt" );
      DUMP_TYPE( "storage_class_subset_english_type_opt", $store_type );
      DUMP_AST_LIST( "capturing_paren_capture_decl_list_english_opt",
                     $capture_ast_list );
      DUMP_AST_LIST( "paren_param_decl_list_english_opt", $param_ast_list );
      DUMP_AST( "returning_english_ast_opt", $ret_ast );

      c_ast_t *const lambda_ast = c_ast_new_gc( K_LAMBDA, &@Y_lambda );
      lambda_ast->type = $store_type;
      c_ast_list_set_param_of( &$capture_ast_list, lambda_ast );
      lambda_ast->lambda.capture_ast_list = slist_move( &$capture_ast_list );
      c_ast_list_set_param_of( &$param_ast_list, lambda_ast );
      lambda_ast->lambda.param_ast_list = slist_move( &$param_ast_list );
      c_ast_set_parent( $ret_ast, lambda_ast );

      DUMP_AST( "$$_ast", lambda_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( lambda_ast ) );
      c_ast_gibberish( lambda_ast, C_GIB_PRINT_DECL, stdout );
      PUTC( '\n' );
    }

    /*
     * C++ user-defined conversion operator declaration.
     */
  | Y_declare storage_class_subset_english_type_opt[store_type]
    cv_qualifier_list_stid_opt[cv_qual_stid] user_defined conversion_exp
    operator_opt of_scope_list_english_opt[scope_sname] returning_exp
    decl_english_ast[ret_ast]
    {
      DUMP_START( "declare_command",
                  "DECLARE storage_class_subset_english_type_opt "
                  "cv_qualifier_list_stid_opt "
                  "USER-DEFINED CONVERSION [OPERATOR] "
                  "of_scope_list_english_opt "
                  "RETURNING decl_english_ast" );
      DUMP_TYPE( "storage_class_subset_english_type_opt", $store_type );
      DUMP_TID( "cv_qualifier_list_stid_opt", $cv_qual_stid );
      DUMP_SNAME( "of_scope_list_english_opt", $scope_sname );
      DUMP_AST( "decl_english_ast", $ret_ast );

      c_ast_t *const udc_ast = c_ast_new_gc( K_UDEF_CONV, &@$ );
      c_sname_set( &udc_ast->sname, &$scope_sname );
      udc_ast->type = c_type_or( &$store_type, &C_TYPE_LIT_S( $cv_qual_stid ) );
      c_ast_set_parent( $ret_ast, udc_ast );

      DUMP_AST( "$$_ast", udc_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( udc_ast ) );
      decl_flags_t decl_flags = C_GIB_PRINT_DECL;
      if ( opt_semicolon )
        decl_flags |= C_GIB_OPT_SEMICOLON;
      c_ast_gibberish( udc_ast, decl_flags, stdout );
      PUTC( '\n' );
    }

  | Y_declare error
    {
      if ( OPT_LANG_IS( operator ) )
        elaborate_error( "name or operator expected" );
      else
        elaborate_error( "name expected" );
    }
  ;

alignas_or_width_decl_english_ast
  : decl_english_ast

  | decl_english_ast[decl_ast] alignas_specifier_english[align]
    {
      DUMP_START( "alignas_or_width_decl_english_ast",
                  "decl_english_ast alignas_specifier_english" );
      DUMP_AST( "decl_english_ast", $decl_ast );
      DUMP_ALIGN( "alignas_specifier_english", $align );

      $$ = $decl_ast;
      $$->align = $align;
      $$->loc = @$;

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | decl_english_ast[decl_ast] width_specifier_english_uint[bit_width]
    {
      DUMP_START( "alignas_or_width_decl_english_ast",
                  "decl_english_ast width_specifier_english_uint" );
      DUMP_AST( "decl_english_ast", $decl_ast );
      DUMP_INT( "width_specifier_english_uint", $bit_width );

      //
      // This check has to be done now in the parser rather than later in the
      // AST since we need to use the builtin union member now.
      //
      if ( !c_ast_is_integral( $decl_ast ) ) {
        print_error( &@bit_width, "invalid bit-field type " );
        print_ast_type_aka( $decl_ast, stderr );
        EPRINTF( "; must be an integral %stype\n",
          OPT_LANG_IS( enum_BITFIELDS ) ? "or enumeration " : ""
        );
        PARSE_ABORT();
      }

      $$ = $decl_ast;
      $$->loc = @$;
      $$->bit_field.bit_width = $bit_width;

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

alignas_specifier_english
  : aligned_english uint_lit[bytes] bytes_opt
    {
      DUMP_START( "alignas_specifier_english", "ALIGNAS uint_lit [BYTES]" );
      DUMP_INT( "uint_lit", $bytes );

      $$.kind = C_ALIGNAS_BYTES;
      $$.loc = @$;
      $$.bytes = $bytes;

      DUMP_ALIGN( "$$_align", $$ );
      DUMP_END();
    }
  | aligned_english decl_english_ast[decl_ast] bytes_opt
    {
      DUMP_START( "alignas_specifier_english",
                  "ALIGNAS decl_english_ast [BYTES]" );
      DUMP_AST( "decl_english_ast", $decl_ast );

      if ( $decl_ast->kind == K_NAME ) {
        $$.kind = C_ALIGNAS_SNAME;
        $$.sname = c_sname_move( &$decl_ast->sname );
      } else {
        $$.kind = C_ALIGNAS_TYPE;
        $$.type_ast = $decl_ast;
      }
      $$.loc = @$;

      DUMP_ALIGN( "$$_align", $$ );
      DUMP_END();
    }
  | aligned_english error
    {
      $$ = (c_alignas_t){ 0 };
      $$.loc = @$;
      elaborate_error( "integer or type expected" );
    }
  ;

aligned_english
  : Y_aligned as_or_to_opt
  | Y__Alignas
  | Y_alignas
  ;

capturing_paren_capture_decl_list_english_opt
  : /* empty */                   { slist_init( &$$ ); }
  | Y_capturing paren_capture_decl_list_english[capture_decl_ast_list]
    {
      $$ = $capture_decl_ast_list;
    }
  | '[' capture_decl_list_english_opt[capture_decl_ast_list] ']'
    {
      $$ = $capture_decl_ast_list;
    }
  ;

paren_capture_decl_list_english
  : '[' capture_decl_list_english_opt[capture_decl_ast_list] ']'
    {
      $$ = $capture_decl_ast_list;
    }
  | '(' capture_decl_list_english_opt[capture_decl_ast_list] ')'
    {
      $$ = $capture_decl_ast_list;
    }
  | error
    {
      slist_init( &$$ );
      elaborate_error( "'[' or '(' expected\n" );
    }
  ;

capture_decl_list_english_opt
  : /* empty */                   { slist_init( &$$ ); }
  | capture_decl_list_english
  ;

capture_decl_list_english
  : capture_decl_list_english[capture_decl_ast_list] comma_exp
    capture_decl_english_ast[capture_decl_ast]
    {
      DUMP_START( "capture_decl_list_english",
                  "capture_decl_list_english ',' capture_decl_english_ast" );
      DUMP_AST_LIST( "capture_decl_list_english", $capture_decl_ast_list );
      DUMP_AST( "capture_decl_english_ast", $capture_decl_ast );

      $$ = $capture_decl_ast_list;
      slist_push_back( &$$, $capture_decl_ast );

      DUMP_AST_LIST( "$$_ast_list", $$ );
      DUMP_END();
    }

  | capture_decl_english_ast[capture_decl_ast]
    {
      DUMP_START( "capture_decl_list_english",
                  "capture_decl_english_ast" );
      DUMP_AST( "capture_decl_english_ast", $capture_decl_ast );

      slist_init( &$$ );
      slist_push_back( &$$, $capture_decl_ast );

      DUMP_AST_LIST( "$$_ast_list", $$ );
      DUMP_END();
    }
  ;

capture_decl_english_ast
  : Y_copy capture_default_opt
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      $$->capture.kind = C_CAPTURE_COPY;
    }
  | Y_reference capture_default_opt
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      $$->capture.kind = C_CAPTURE_REFERENCE;
    }
  | Y_reference Y_to name_exp[name]
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      c_sname_init_name( &$$->sname, $name );
      $$->capture.kind = C_CAPTURE_REFERENCE;
    }
  | capture_decl_c_ast
  ;

capture_default_opt
  : /* empty */
  | Y_by default_exp
  | Y_default
  ;

width_specifier_english_uint
  : Y_width uint_lit_exp[bit_width] bits_opt
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since we use 0 to mean "no bit-field."
      //
      if ( $bit_width == 0 ) {
        print_error( &@bit_width, "bit-field width must be > 0\n" );
        PARSE_ABORT();
      }
      $$ = $bit_width;
    }
  ;

storage_class_subset_english_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | storage_class_subset_english_type_opt[left_store_type]
    storage_class_subset_english_type[right_store_type]
    {
      DUMP_START( "storage_class_subset_english_type_opt",
                  "storage_class_subset_english_type_opt "
                  "storage_class_subset_english_type" );
      DUMP_TYPE( "storage_class_subset_english_type_opt", $left_store_type );
      DUMP_TYPE( "storage_class_subset_english_type", $right_store_type );

      $$ = $left_store_type;
      PARSE_ASSERT( c_type_add( &$$, &$right_store_type, &@right_store_type ) );

      DUMP_TYPE( "$$_type", $$ );
      DUMP_END();
    }
  ;

storage_class_subset_english_type
  : attribute_english_atid        { $$ = C_TYPE_LIT_A( $1 ); }
  | storage_class_subset_english_stid[stid]
    {
      $$ = C_TYPE_LIT_S( $stid );
    }
  ;

  /*
   * We need a separate storage class set for both lambdas and user-defined
   * conversion operators without "delete" to eliminiate a shift/reduce
   * conflict; shift:
   *
   *      declare delete as ...
   *
   * where "delete" is the operator; and reduce:
   *
   *      declare delete[d] user-defined conversion operator ...
   *
   * where "delete" is storage-class-like.  The "delete" can safely be removed
   * since only special members can be deleted anyway.
   */
storage_class_subset_english_stid
  : Y_auto_TYPE                   { $$ = TS_auto; }
  | Y_Apple___block
  | Y_constant eval_expr_init_stid[stid]
    {
      $$ = $stid;
    }
  | Y_consteval
  | Y_constexpr
  | Y_constinit
  | Y_explicit
  | Y_export
  | Y_extern
  | Y_extern linkage_stid[stid] linkage_opt
    {
      $$ = $stid;
    }
  | Y_final
  | Y_friend
  | Y_inline
  | Y_mutable
  | Y_no Y_except                 { $$ = TS_noexcept; }
  | Y_noexcept
  | Y_non_empty                   { $$ = TS_NON_EMPTY_ARRAY; }
  | Y_override
  | Y_static
  | Y_this
  | Y_thread local_exp            { $$ = TS_thread_local; }
  | Y__Thread_local
  | Y_thread_local
  | Y_throw
  | Y_typedef
  | Y_virtual
  | Y_pure virtual_stid_exp       { $$ = TS_PURE_virtual | $2; }
  ;

/// define command ////////////////////////////////////////////////////////////

define_command
  : Y_define sname_english_exp[sname] as_exp decl_english_ast[decl_ast]
    {
      DUMP_START( "define_command",
                  "DEFINE sname_english AS decl_english_ast" );
      DUMP_SNAME( "sname_english_exp", $sname );
      DUMP_AST( "decl_english_ast", $decl_ast );

      // see the comment in "declare_command"
      if ( c_ast_is_name_error( $decl_ast ) ) {
        c_sname_cleanup( &$sname );
        PARSE_ABORT();
      }

      if ( !c_tid_is_any( $decl_ast->type.stids, TS_typedef ) ) {
        //
        // Explicitly add TS_typedef (if it doesn't have it already) to
        // prohibit cases like:
        //
        //      define eint as extern int
        //      define rint as register int
        //      define sint as static int
        //      ...
        //
        // i.e., a defined type with a storage class.  Once the semantic checks
        // pass, remove the TS_typedef.
        //
        PARSE_ASSERT(
          c_type_add( &$decl_ast->type, &T_TS_typedef, &@decl_ast )
        );
      }
      PARSE_ASSERT( c_ast_check( $decl_ast ) );
      PJL_DISCARD_RV( c_ast_take_type_any( $decl_ast, &T_TS_typedef ) );

      c_sname_set( &$decl_ast->sname, &$sname );
      if ( c_tid_is_any( $decl_ast->type.btids, TB_ANY_SCOPE ) )
        c_sname_local_data( &$decl_ast->sname )->type = $decl_ast->type;
      c_sname_set_all_types( &$decl_ast->sname );

      DUMP_AST( "$$_ast", $decl_ast );
      DUMP_END();

      PARSE_ASSERT( define_type( $decl_ast, C_ENG_DECL ) );
    }
  ;

/// expand command ////////////////////////////////////////////////////////////

expand_command
  : Y_expand Y_NAME[name] expand_command2[expand2]
    {
      p_arg_list_t   *const arg_list   = $expand2[0];
      p_token_list_t *const extra_list = $expand2[1];

      DUMP_START( "expand_command", "EXPAND NAME arg_list_opt extra_list_opt" );
      DUMP_STR( "name", $name );
      DUMP_MACRO_ARG_LIST( "arg_list", arg_list );
      DUMP_MACRO_TOKEN_LIST( "extra_list", extra_list );
      DUMP_END();

      bool const ok =
        p_macro_expand( $name, &@name, arg_list, extra_list, stdout );
      p_arg_list_cleanup( arg_list );
      free( arg_list );
      p_token_list_cleanup( extra_list );
      free( extra_list );
      free( $name );
      PARSE_ASSERT( ok );
    }

  | Y_expand error
    {
      elaborate_error( "macro name expected" );
    }
  ;

expand_command2
  : /* empty */                   { $$[0] = $$[1] = NULL; }

    /*
     * This is for the case where there are no macro arguments, but perhaps
     * additional tokens that do _not_ start with a '(' past the name of the
     * macro.  These should be appended (and expaned if necessary):
     *
     *      cdecl> #define M   x
     *      cdecl> expand M y
     *      M => x
     *      M => x y
     *
     * If the additional tokens started with a '(', then it would be the next
     * case.
     */
  | p_extra_token_except_lparen[token] p_extra_tokens_opt[extra_list]
    {
      if ( $extra_list == NULL ) {
        $extra_list = MALLOC( p_token_list_t, 1 );
        slist_init( $extra_list );
      }
      slist_push_front( $extra_list, $token );

      $$[0] = NULL;
      $$[1] = $extra_list;
    }

  | '(' p_arg_list_opt[arg_list] ')' p_extra_tokens_opt[extra_list]
    {
      $$[0] = $arg_list;
      $$[1] = $extra_list;
    }
  ;

p_extra_token_except_lparen
  : p_arg_token_with_comma
  | ')'
    {
      $$ = p_token_new_loc( P_PUNCTUATOR, &@1, ")" );
    }
  ;

p_extra_tokens_opt
  : /* empty */                   { $$ = NULL; }
  | p_extra_tokens
  ;

p_extra_tokens
  : p_extra_tokens[extra_list] p_extra_token[token]
    {
      $$ = $extra_list;
      slist_push_back( $$, $token );
    }
  | p_extra_token[token]
    {
      $$ = MALLOC( p_token_list_t, 1 );
      slist_init( $$ );
      slist_push_back( $$, $token );
    }
  ;

p_extra_token
  : p_extra_token_except_lparen
  | '('
    {
      $$ = p_token_new_loc( P_PUNCTUATOR, &@1, "(" );
    }
  ;

p_arg_list_opt
  : /* empty */
    {
      $$ = MALLOC( p_arg_list_t, 1 );
      slist_init( $$ );
    }

  | p_arg_list

    /*
     * Handles cases where there are only empty macro arguments separated only
     * by commas (no spaces), e.g.:
     *
     *      M(,)
     *      M(,,)
     *
     * and so on.  Note that N commas returns N-1 arguments, so we have to add
     * two placemarker arguments, one for each side of the commas.
     */
  | p_comma_arg_list[comma_arg_list]
    {
      slist_push_back( $comma_arg_list, p_token_list_new_placemarker() );
      slist_push_back( $comma_arg_list, p_token_list_new_placemarker() );

      $$ = MALLOC( p_arg_list_t, 1 );
      slist_init( $$ );
      slist_push_list_back( $$, $comma_arg_list );
    }

    /*
     * Handles cases where there are empty macro arguments separated only by
     * commas at the beginning, e.g.:
     *
     *      M(,x)
     *      M(,,x)
     *
     * and so on.  Note than N commas returns N-1 arguments, so we have to add
     * one placemarker argument for the left of the commas.
     */
  | p_comma_arg_list[comma_arg_list] p_arg_list[arg_list]
    {
      slist_push_back( $comma_arg_list, p_token_list_new_placemarker() );
      $$ = $arg_list;
      slist_push_list_front( $$, $comma_arg_list );
    }

    /*
     * Handles cases where there are empty macro arguments separated only by
     * commas at the beginning and end, e.g.:
     *
     *      M(,x,)
     *      M(,x,,)
     *      M(,,x,)
     *
     * and so on.  Note than N commas returns N-1 arguments, so we have to add
     * two placemarker arguments, one for the left of the left commas and one
     * for the right of the right commas.
     */
  | p_comma_arg_list[lcomma_arg_list]
    p_arg_list[arg_list]
    p_comma_arg_list[rcomma_arg_list]
    {
      slist_push_back( $lcomma_arg_list, p_token_list_new_placemarker() );
      slist_push_back( $rcomma_arg_list, p_token_list_new_placemarker() );

      $$ = $arg_list;
      slist_push_list_front( $$, $lcomma_arg_list );
      slist_push_list_back ( $$, $rcomma_arg_list );
    }

    /*
     * Handles cases where there are empty macro arguments separated only by
     * commas at the end, e.g.:
     *
     *      M(x,)
     *      M(x,,)
     *
     * and so on.  Note than N commas returns N-1 arguments, so we have to add
     * one placemarker argument for the right of the commas.
     */
  | p_arg_list[arg_list] p_comma_arg_list[comma_arg_list]
    {
      slist_push_back( $comma_arg_list, p_token_list_new_placemarker() );
      $$ = $arg_list;
      slist_push_list_back( $$, $comma_arg_list );
    }

  | error
    {
      $$ = NULL;
      elaborate_error( "macro argument expected" );
    }
  ;

p_comma_arg_list
  : p_comma_arg_list[arg_list] ','
    {
      $$ = $arg_list;
      slist_push_back( $$, p_token_list_new_placemarker() );
    }
  | ','
    {
      $$ = MALLOC( p_arg_list_t, 1 );
      slist_init( $$ );
    }
  ;

p_arg_list
    /*
     * Also handles cases where there are empty macro arguments separated only
     * by commas in the middle, e.g.:
     *
     *      M(x,y)                      // normal case
     *      M(x,,y)                     // one empty argument
     *      M(x,,,y)                    // two empty arguments
     *
     * and so on.  Note that N commas returns N-1 arguments, so for the normal
     * case of 1 comma, comma_arg_list will be empty and we do nothing special.
     */
  : p_arg_list[arg_list]
    p_comma_arg_list[comma_arg_list]
    p_arg_tokens[arg_tokens]
    {
      $$ = $arg_list;

      if ( slist_empty( $comma_arg_list ) )
        free( $comma_arg_list );
      else
        slist_push_list_back( $$, $comma_arg_list );

      slist_push_back( $$, $arg_tokens );
    }

  | p_arg_tokens[arg_tokens]
    {
      $$ = MALLOC( p_arg_list_t, 1 );
      slist_init( $$ );
      slist_push_back( $$, $arg_tokens );
    }
  ;

p_arg_tokens
  : p_arg_tokens[arg_tokens] p_arg_token_as_list[arg_token_list]
    {
      $$ = $arg_tokens;
      slist_push_list_back( $$, $arg_token_list );
    }
  | p_arg_token_as_list
  ;

p_arg_token_as_list
  : p_arg_token[arg_token]
    {
      $$ = MALLOC( p_token_list_t, 1 );
      slist_init( $$ );
      slist_push_back( $$, $arg_token );
    }
  | '('[lparen] p_arg_tokens_with_comma_opt[arg_tokens] ')'[rparen]
    {
      $$ = $arg_tokens;
      slist_push_front( $$, p_token_new_loc( P_PUNCTUATOR, &@lparen, "(" ) );
      slist_push_back ( $$, p_token_new_loc( P_PUNCTUATOR, &@rparen, ")" ) );
    }
  ;

p_arg_token
  : Y_CHAR_LIT[char]
    {
      $$ = p_token_new_loc( P_CHAR_LIT, &@char, $char );
    }
  | Y_NAME[name]
    {
      $$ = p_token_new_loc( P_IDENTIFIER, &@name, $name );
    }
  | p_num_lit[num]
    {
      $$ = p_token_new_loc( P_NUM_LIT, &@num, check_strdup( yytext ) );
    }
  | Y_PRE_SPACE[space]
    {
      $$ = p_token_new_loc( P_SPACE, &@space, /*literal=*/NULL );
    }
  | Y_STR_LIT[str]
    {
      $$ = p_token_new_loc( P_STR_LIT, &@str, $str );
    }
  | p_other[other]
    {
      $$ = p_token_new_loc( P_OTHER, &@other, yytext );
    }
  | p_punctuator[punct]
    {
      $$ = p_token_new_loc( P_PUNCTUATOR, &@punct, yytext );
    }
  ;

p_num_lit
  : Y_FLOAT_LIT
  | Y_INT_LIT
  ;

p_other
  : '$'
  | '@'
  | '`'
  ;

p_punctuator
  : '!'
  | '#'
  | '%'
  | '&'
/*| '(' -- handled separately for parenthesized arguments */
/*| ')' -- handled separately for parenthesized arguments */
  | '*'
  | '+'
/*| ',' -- handled separately to separate arguments */
  | '-'
  | '.'
  | '/'
  | ':'
  | ';'
  | '<'
  | '='
  | '>'
  | '?'
  | '['
  | ']'
  | '^'
  | '{'
  | '|'
  | '}'
  | '~'
  | Y_AMPER_AMPER
  | Y_AMPER_EQUAL
  | Y_CARET_EQUAL
  | Y_COLON_COLON
  | Y_DOT_STAR
  | Y_ELLIPSIS
  | Y_EQUAL_EQUAL
  | Y_EXCLAM_EQUAL
  | Y_GREATER_EQUAL
  | Y_GREATER_GREATER
  | Y_GREATER_GREATER_EQUAL
  | Y_LESS_EQUAL
  | Y_LESS_EQUAL_GREATER
  | Y_LESS_LESS
  | Y_LESS_LESS_EQUAL
  | Y_MINUS_EQUAL
  | Y_MINUS_GREATER
  | Y_MINUS_GREATER_STAR
  | Y_MINUS_MINUS
  | Y_PERCENT_EQUAL
  | Y_PIPE_EQUAL
  | Y_PIPE_PIPE
  | Y_PLUS_EQUAL
  | Y_PLUS_PLUS
  | Y_SLASH_EQUAL
  | Y_STAR_EQUAL
  ;

p_arg_tokens_with_comma_opt
  : /* empty */
    {
      $$ = MALLOC( p_token_list_t, 1 );
      slist_init( $$ );
    }
  | p_arg_tokens_with_comma
  ;

  /*
   * We need a version of p_arg_tokens, p_arg_token_as_list, and p_arg_token
   * where p_arg_token includes ',' because the original set uses ',' to
   * separate arguments whereas this set (that is within parentheses) treats
   * ',' as just another token that's part of the same argument, e.g.:
   *
   *      expand M(x, (a,b), y)
   *
   * The ',' in the middle is part of the second argument of (a,b).
   */
p_arg_tokens_with_comma
  : p_arg_tokens_with_comma[arg_tokens]
    p_arg_token_with_comma_as_list[arg_token_list]
    {
      $$ = $arg_tokens;
      slist_push_list_back( $$, $arg_token_list );
    }
  | p_arg_token_with_comma_as_list
  ;

p_arg_token_with_comma_as_list
  : p_arg_token_with_comma[arg_token]
    {
      $$ = MALLOC( p_token_list_t, 1 );
      slist_init( $$ );
      slist_push_back( $$, $arg_token );
    }
  | '('[lparen] p_arg_tokens_with_comma_opt[arg_tokens] ')'[rparen]
    {
      $$ = $arg_tokens;
      slist_push_front( $$, p_token_new_loc( P_PUNCTUATOR, &@lparen, "(" ) );
      slist_push_back ( $$, p_token_new_loc( P_PUNCTUATOR, &@rparen, ")" ) );
    }
  ;

p_arg_token_with_comma
  : p_arg_token
  | ','
    {
      $$ = p_token_new_loc( P_PUNCTUATOR, &@1, "," );
    }
  ;

/// explain command ///////////////////////////////////////////////////////////

explain_command
    /*
     * C-style cast.
     */
  : explain c_style_cast_expr_c

    /*
     * New C++-style cast.
     */
  | explain new_style_cast_expr_c

    /*
     * Common typed declaration, e.g.: T x.
     */
  | explain typed_declaration_c

    /*
     * Common declaration with alignas, e.g.: alignas(8) T x.
     */
  | explain aligned_declaration_c

    /*
     * asm declaration -- not supported.
     */
  | explain asm_declaration_c

    /*
     * C++ file-scope constructor definition, e.g.: S::S([params]).
     */
  | explain file_scope_constructor_declaration_c

    /*
     * C++ in-class destructor declaration, e.g.: ~S().
     */
  | explain destructor_declaration_c

    /*
     * C++ file scope destructor definition, e.g.: S::~S().
     */
  | explain file_scope_destructor_declaration_c

    /*
     * C++ lambda declaration.
     */
  | explain lambda_declaration_c

    /*
     * Pre-C99 implicit int function and C++ in-class constructor declaration.
     */
  | explain pc99_func_or_constructor_declaration_c

    /*
     * Pre-C99 pointer to implicit int declaration:
     *
     *      *p;               // pointer to int
     *      *p, i;            // pointer to int, int
     *      *a[4];            // array 4 of pointer to int
     *      *f();             // function returning pointer to int
     */
  | explain pc99_pointer_decl_list_c[decl_ast_list]
    {
      PARSE_ASSERT( c_ast_list_check( &$decl_ast_list ) );
      c_ast_list_english( &$decl_ast_list, stdout );
    }

    /*
     * Template declaration -- not supported.
     */
  | explain template_declaration_c

    /*
     * Common C++ declaration with typename (without alignas), e.g.:
     *
     *      explain typename T::U x
     *
     * (We can't use typename_flag_opt because it would introduce more
     * shift/reduce conflicts.)
     */
  | explain Y_typename { in_attr.is_typename = true; } typed_declaration_c

    /*
     * User-defined conversion operator declaration without a storage-class-
     * like part.  This is for a case like:
     *
     *      explain operator int()
     *
     * User-defined conversion operator declarations with a storage-class-like
     * part, e.g.:
     *
     *      explain explicit operator int()
     *
     * are handled by the common declaration rule.
     */
  | explain user_defined_conversion_declaration_c

    /*
     * C++ using declaration.
     */
  | explain extern_linkage_c_stid_opt[extern_stid] using_decl_c_ast[decl_ast]
    {
      DUMP_START( "explain_command",
                  "EXPLAIN extern_linkage_c_stid_opt using_decl_c_ast" );
      DUMP_TID( "extern_linkage_c_stid_opt", $extern_stid );
      DUMP_AST( "using_decl_c_ast", $decl_ast );

      PARSE_ASSERT(
        c_type_add_tid( &$decl_ast->type, $extern_stid, &@extern_stid )
      );

      DUMP_AST( "$$_ast", $decl_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( $decl_ast ) );
      c_ast_english( $decl_ast, C_ENG_DECL, stdout );
      PUTC( '\n' );
    }

    /*
     * If we match an sname, it means it wasn't an sname for a type (otherwise
     * we would have matched the "Common declaration" case above).
     */
  | explain sname_c[sname]
    {
      print_error_unknown_name( &@sname, &$sname );
      c_sname_cleanup( &$sname );
      PARSE_ABORT();
    }

  | explain error
    {
      elaborate_error( "cast or declaration expected" );
    }
  ;

explain
  : Y_explain
    { //
      // Set our mode to deciphering gibberish into English and specifically
      // tell the lexer to return cdecl keywords (e.g., "func") as ordinary
      // names, otherwise gibberish like:
      //
      //      int func(void);
      //
      // would result in a parser error.
      //
      gibberish_to_english();
    }
  ;

/// help command //////////////////////////////////////////////////////////////

help_command
  : '?'
    { //
      // We want cdecl commands (like "declare", etc.), other cdecl keywords
      // (like "cast", "commands", "english", "options", etc.), and C/C++
      // keywords (like "const", "static", "typedef", etc.) all to be returned
      // as just strings so we don't have to enumerate all the possible tokens
      // in the grammar.
      //
      lexer_find &= ~(LEXER_FIND_C_KEYWORDS | LEXER_FIND_CDECL_KEYWORDS);
    }
    help_what_opt[what]
    {
      bool const ok = print_help( $what, &@what );
      free( $what );
      PARSE_ASSERT( ok );
    }
  ;

help_what_opt
  : /* empty */                   { $$ = NULL; }
  | name_cat
  | '#' Y_NAME[name]              { $$ = str_realloc_pcat( "#", "", $name ); }
  | '#' error
    {
      elaborate_error( "\"#define\", \"#include\", or \"#undef\" expected" );
      $$ = NULL;
    }
  | error
    {
      elaborate_error(
        "<command>, \"commands\", \"english\", or \"options\" expected"
      );
      $$ = NULL;
    }
  ;

/// preprocessor command //////////////////////////////////////////////////////

preprocessor_command
  : '#'                                 /* ignore null directive */
  | '#' p_define
/*| '#' include -- handled within the lexer */
  | '#' p_undef

  | '#' Y_PRE_elif
    {
      UNSUPPORTED( &@Y_PRE_elif, "\"#elif\"" );
    }
  | '#' Y_PRE_elifdef
    {
      UNSUPPORTED( &@Y_PRE_elifdef, "\"#elifdef\"" );
    }
  | '#' Y_PRE_elifndef
    {
      UNSUPPORTED( &@Y_PRE_elifndef, "\"#elifdef\"" );
    }
  | '#' Y_PRE_else
    {
      UNSUPPORTED( &@Y_PRE_else, "\"#else\"" );
    }
  | '#' Y_PRE_embed
    {
      UNSUPPORTED( &@Y_PRE_embed, "\"#embed\"" );
    }
  | '#' Y_PRE_error
    {
      UNSUPPORTED( &@Y_PRE_error, "\"#error\"" );
    }
  | '#' Y_PRE_if
    {
      UNSUPPORTED( &@Y_PRE_if, "\"#if\"" );
    }
  | '#' Y_PRE_ifdef
    {
      UNSUPPORTED( &@Y_PRE_ifdef, "\"#ifdef\"" );
    }
  | '#' Y_PRE_ifndef
    {
      UNSUPPORTED( &@Y_PRE_ifndef, "\"#ifndef\"" );
    }
  | '#' Y_PRE_line
    {
      UNSUPPORTED( &@Y_PRE_line, "\"#line\"" );
    }
  | '#' Y_PRE_pragma
    {
      UNSUPPORTED( &@Y_PRE_pragma, "\"#pragma\"" );
    }
  | '#' Y_PRE_warning
    {
      UNSUPPORTED( &@Y_PRE_warning, "\"#warning\"" );
    }
  | '#' error
    {
      elaborate_error( "\"#define\", \"#include\", or \"#undef\" expected" );
    }
  ;

p_define
  : Y_PRE_define name_exp[name] p_paren_param_list_opt[param_list]
    p_replace_list_opt[replace_list]
    {
      DUMP_START( "p_define",
                  "#define NAME p_paren_param_list_opt p_replace_list_opt" );
      DUMP_STR( "name", $name );
      DUMP_MACRO_PARAM_LIST( "p_paren_param_list_opt", $param_list );
      DUMP_MACRO_TOKEN_LIST( "p_replace_list_opt", $replace_list );

      c_typedef_t const *const tdef = c_typedef_find_name( $name );
      if ( tdef != NULL ) {
        if ( tdef->is_predefined ) {
          print_warning( &@name,
            "\"%s\" is a predefined type since %s\n",
            $name,
            c_lang_name( c_lang_oldest( tdef->lang_ids ) )
          );
        }
        else {
          print_warning( &@name,
            "\"%s\" previously defined as type (\"",
            $name
          );
          print_type_decl( tdef, tdef->decl_flags, stderr );
          EPUTS( "\")\n" );
        }
      }

      p_macro_t *const macro =
        p_macro_define( $name, &@name, $param_list, $replace_list );
      p_param_list_cleanup( $param_list );
      free( $param_list );
      p_token_list_cleanup( $replace_list );
      free( $replace_list );
      PARSE_ASSERT( macro != NULL );

      DUMP_MACRO( "macro", macro );
      DUMP_END();
    }

p_paren_param_list_opt
  : /* empty */                   { $$ = NULL; }
  | '(' p_param_list_opt ')'      { $$ = $2; }

p_param_list_opt
  : /* empty */
    { //
      // Use an empty list to distinguish a macro with an empty parameter list
      // from one with no parameter list.
      //
      //      #define M   X       // no parameter list: use NULL
      //      #define F() #X      // empty parameter list: use empty list
      //
      $$ = MALLOC( p_param_list_t, 1 );
      slist_init( $$ );
    }
  | p_param_list
  ;

p_param_list
  : p_param_list[param_list] comma_exp p_param[param]
    {
      DUMP_START( "p_param_list", "p_param" );
      DUMP_MACRO_PARAM_LIST( "p_param_list", $param_list );
      DUMP_STR( "p_param", $param->name );

      $$ = $param_list;
      slist_push_back( $$, $param );

      DUMP_MACRO_PARAM_LIST( "$$_list", $$ );
      DUMP_END();
    }

  | p_param[param]
    {
      DUMP_START( "p_param_list", "p_param" );
      DUMP_STR( "p_param", $param->name );

      $$ = MALLOC( p_param_list_t, 1 );
      slist_init( $$ );
      slist_push_back( $$, $param );

      DUMP_MACRO_PARAM_LIST( "$$_list", $$ );
      DUMP_END();
    }
  ;

p_param
  : Y_NAME[name]
    {
      $$ = MALLOC( p_param_t, 1 );
      *$$ = (p_param_t){ .name = $name, .loc = @name };
    }
  | Y_ELLIPSIS
    {
      $$ = MALLOC( p_param_t, 1 );
      *$$ = (p_param_t){ .name = check_strdup( L_ELLIPSIS ), .loc = @1 };
    }
  | error
    {
      if ( OPT_LANG_IS( VARIADIC_MACROS ) )
        elaborate_error( "parameter name or \"...\" expected" );
      else
        elaborate_error( "parameter name expected" );
      $$ = NULL;
    }
  ;

p_replace_list_opt
  : /* empty */                   { $$ = NULL; }
  | p_replace_list
  ;

p_replace_list
  : p_replace_list[replace_list] p_replace_token[token]
    {
      $$ = $replace_list;
      slist_push_back( $$, $token );
    }

  | p_replace_token[token]
    {
      $$ = MALLOC( p_token_list_t, 1 );
      slist_init( $$ );
      slist_push_back( $$, $token );
    }
  ;

p_replace_token
  : p_arg_token[token]
    {
      if ( p_token_is_punct( $token, '#' ) ) {
        //
        // A '#' macro argument token is an ordinary '#', but as a replacement
        // token, retroactively make it P_STRINGIFY.
        //
        $token->kind = P_STRINGIFY;
      }
      $$ = $token;
    }
  | '('
    {
      $$ = p_token_new_loc( P_PUNCTUATOR, &@1, "(" );
    }
  | ')'
    {
      $$ = p_token_new_loc( P_PUNCTUATOR, &@1, ")" );
    }
  | ','
    {
      $$ = p_token_new_loc( P_PUNCTUATOR, &@1, "," );
    }
  | Y_PRE_CONCAT
    {
      $$ = p_token_new_loc( P_CONCAT, &@1, /*literal=*/NULL );
    }
  | Y_PRE___VA_ARGS__
    {
      $$ = p_token_new_loc( P___VA_ARGS__, &@1, /*literal=*/NULL );
    }
  | Y_PRE___VA_OPT__
    {
      $$ = p_token_new_loc( P___VA_OPT__, &@1, /*literal=*/NULL );
    }
  ;

p_undef
  : Y_PRE_undef name_exp[name]
    {
      PARSE_ASSERT( p_macro_undef( $name, &@name ) );
    }
  ;

/// quit command //////////////////////////////////////////////////////////////

quit_command
  : Y_quit                        { cdecl_quit(); }
  ;

/// scope (enum, class, struct, union, namespace) command /////////////////////

scoped_command
  : scoped_declaration_c
  ;

/// set command ///////////////////////////////////////////////////////////////

set_command
  : Y_set
    {
      PARSE_ASSERT( set_option( NULL, NULL, NULL, NULL ) );
    }
  | Y_set set_option_list
  ;

set_option_list
  : set_option_list set_option
  | set_option
  ;

set_option
  : Y_SET_OPTION[option] set_option_value_opt[value]
    {
      bool const ok = set_option( $option, &@option, $value, &@value );
      free( $option );
      free( $value );
      PARSE_ASSERT( ok );
    }
  ;

set_option_value_opt
  : /* empty */                   { $$ = NULL; }
  | '=' Y_SET_OPTION[option]      { $$ = $option; @$ = @option; }
  | '=' error
    {
      elaborate_error( "option value expected" );
      $$ = NULL;
    }
  ;

/// show command //////////////////////////////////////////////////////////////

show_command
  : Y_show any_typedef_tdef[tdef] show_format_opt[format]
    {
      DUMP_START( "show_command", "SHOW any_typedef_tdef show_format_opt" );
      DUMP_AST( "any_typedef__ast", $tdef->ast );
      DUMP_INT( "show_format_opt", $format );
      DUMP_END();

      show_type( $tdef, $format, stdout );
    }

  | Y_show any_typedef_tdef[tdef] Y_as show_format_exp[format]
    {
      DUMP_START( "show_command", "SHOW any_typedef_tdef AS show_format_exp" );
      DUMP_AST( "any_typedef__ast", $tdef->ast );
      DUMP_INT( "show_format_exp", $format );
      DUMP_END();

      show_type( $tdef, $format, stdout );
    }

  | Y_show show_which_opt[show] glob_opt[name] show_format_opt[format]
    {
      bool const ok = show_types( $show, $name, $format, stdout ) ||
                      $name == NULL;
      if ( !ok ) {
        print_error( &@name, "\"%s\": no such type or macro defined", $name );
        print_suggestions( DYM_C_MACROS | DYM_C_TYPES, $name );
        EPUTC( '\n' );
      }
      free( $name );
      PARSE_ASSERT( ok );
    }

  | Y_show show_which_opt[show] glob_opt[name] Y_as show_format_exp[format]
    {
      bool const ok = show_types( $show, $name, $format, stdout ) ||
                      $name == NULL;
      if ( !ok ) {
        print_error( &@name, "\"%s\": no such type or macro defined", $name );
        print_suggestions( DYM_C_MACROS | DYM_C_TYPES, $name );
        EPUTC( '\n' );
      }
      free( $name );
      PARSE_ASSERT( ok );
    }

  | Y_show show_which_opt[show] Y_macros
    {
      show_macros( $show, stdout );
    }

  | Y_show Y_NAME[name]
    {
      bool ok = false;

      //
      // Check to see if $name is a macro here rather than in the lexer since
      // this is the only place in the cdecl grammar where a name can be a
      // macro since cdecl doesn't support using macros everywhere.
      //
      p_macro_t const *const macro = p_macro_find( $name );
      if ( macro == NULL ) {
        print_error( &@name, "\"%s\": no such type or macro defined", $name );
        print_suggestions( DYM_C_MACROS | DYM_C_TYPES, $name );
        EPUTC( '\n' );
      }
      else if ( !(ok = show_macro( macro, stdout )) ) {
        c_lang_id_t const lang_ids = (*macro->dyn_fn)( /*ptoken=*/NULL );
        print_error( &@name,
          "\"%s\" not supported%s\n",
          $name, c_lang_which( lang_ids )
        );
      }

      free( $name );
      if ( !ok )
        PARSE_ABORT();
    }

  | Y_show error
    {
      elaborate_error(
        "type or macro name, or \"all\", \"predefined\", or \"user\" expected"
      );
    }
  ;

show_format
  : Y_english                     { $$ = C_ENG_DECL; }
  | Y_typedef                     { $$ = C_GIB_TYPEDEF; }
  | Y_using
    {
      if ( !OPT_LANG_IS( using_DECLS ) ) {
        print_error( &@Y_using,
          "\"using\" not supported%s\n",
          C_LANG_WHICH( using_DECLS )
        );
        PARSE_ABORT();
      }
      $$ = C_GIB_USING;
    }
  ;

show_format_exp
  : show_format
  | error
    {
      if ( OPT_LANG_IS( using_DECLS ) )
        elaborate_error( "\"english\", \"typedef\", or \"using\" expected" );
      else
        elaborate_error( "\"english\" or \"typedef\" expected" );
    }
  ;

show_format_opt
  : /* empty */                   { $$ = 0/*as declared*/; }
  | show_format
  ;

show_which_opt
  : /* empty */                   { $$ = CDECL_SHOW_USER_DEFINED; }
  | Y_all predefined_or_user_types_opt[show]
    {
      $$ = $show | CDECL_SHOW_OPT_IGNORE_LANG;
    }
  | Y_predefined                  { $$ = CDECL_SHOW_PREDEFINED; }
  | Y_user                        { $$ = CDECL_SHOW_USER_DEFINED; }
  ;

predefined_or_user_types_opt
  : /* empty */
    {
      $$ = CDECL_SHOW_PREDEFINED | CDECL_SHOW_USER_DEFINED;
    }
  | Y_predefined                  { $$ = CDECL_SHOW_PREDEFINED; }
  | Y_user                        { $$ = CDECL_SHOW_USER_DEFINED; }
  ;

/// template command //////////////////////////////////////////////////////////

template_command
  : template_declaration_c
  ;

/// typedef command ///////////////////////////////////////////////////////////

typedef_command
  : typedef_declaration_c
  ;

/// using command /////////////////////////////////////////////////////////////

using_command
  : using_declaration_c
  ;

///////////////////////////////////////////////////////////////////////////////
//  C/C++ CASTS                                                              //
///////////////////////////////////////////////////////////////////////////////

/// Gibberish C-style cast ////////////////////////////////////////////////////

c_style_cast_expr_c
  : '(' type_c_ast[type_ast]
    {
      ia_type_ast_push( $type_ast );
    }
    cast_c_astp_opt[cast_astp] rparen_exp sname_c_opt[sname]
    {
      ia_type_ast_pop();

      DUMP_START( "explain_command",
                  "EXPLAIN '(' type_c_ast cast_c_astp_opt ')' sname_c_opt" );
      DUMP_AST( "type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );
      DUMP_SNAME( "sname_c_opt", $sname );

      c_ast_t *const cast_ast = c_ast_new_gc( K_CAST, &@$ );
      cast_ast->sname = c_sname_move( &$sname );
      cast_ast->cast.kind = C_CAST_C;
      cast_ast->cast.to_ast =
        c_ast_patch_placeholder( $type_ast, $cast_astp.ast );

      DUMP_AST( "$$_ast", cast_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( cast_ast ) );
      c_ast_english( cast_ast, C_ENG_DECL, stdout );
      PUTC( '\n' );
    }

    /*
     * Pre-C99 pointer to implicit int declaration (with unnecessary
     * parentheses):
     *
     *      (*p);             // pointer to int
     *      (*p, i);          // pointer to int, int
     *      (*a[4]);          // array 4 of pointer to int
     *      (*f());           // function returning pointer to int
     */
  | '(' pc99_pointer_decl_list_c[decl_ast_list] rparen_exp
    {
      PARSE_ASSERT( c_ast_list_check( &$decl_ast_list ) );
      c_ast_list_english( &$decl_ast_list, stdout );
    }
  ;

/// Gibberish C++-style cast //////////////////////////////////////////////////

new_style_cast_expr_c
  : new_style_cast_c[cast_kind] lt_exp type_c_ast[type_ast]
    {
      ia_type_ast_push( $type_ast );
    }
    cast_c_astp_opt[cast_astp] gt_exp lparen_exp sname_c_exp[sname] rparen_exp
    {
      ia_type_ast_pop();

      DUMP_START( "explain_command",
                  "EXPLAIN new_style_cast_c"
                  "'<' type_c_ast cast_c_astp_opt '>' '(' sname ')'" );
      DUMP_STR( "new_style_cast_c", c_cast_english( $cast_kind ) );
      DUMP_AST( "type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );
      DUMP_SNAME( "sname_c_exp", $sname );

      c_ast_t *const cast_ast = c_ast_new_gc( K_CAST, &@$ );
      cast_ast->sname = c_sname_move( &$sname );
      cast_ast->cast.kind = $cast_kind;
      cast_ast->cast.to_ast =
        c_ast_patch_placeholder( $type_ast, $cast_astp.ast );

      DUMP_AST( "$$_ast", cast_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( cast_ast ) );
      c_ast_english( cast_ast, C_ENG_DECL, stdout );
      PUTC( '\n' );
    }
  ;

new_style_cast_c
  : Y_const_cast                  { $$ = C_CAST_CONST;        }
  | Y_dynamic_cast                { $$ = C_CAST_DYNAMIC;      }
  | Y_reinterpret_cast            { $$ = C_CAST_REINTERPRET;  }
  | Y_static_cast                 { $$ = C_CAST_STATIC;       }
  ;

///////////////////////////////////////////////////////////////////////////////
//  C/C++ DECLARATIONS                                                       //
///////////////////////////////////////////////////////////////////////////////

/// Gibberish C/C++ aligned declaration ///////////////////////////////////////

aligned_declaration_c
  : alignas_specifier_c[align]    { in_attr.align = $align; }
    typename_flag_opt[flag]       { in_attr.is_typename = $flag; }
    typed_declaration_c
  ;

alignas_specifier_c
  : alignas lparen_exp uint_lit[bytes] rparen_exp
    {
      DUMP_START( "alignas_specifier_c", "ALIGNAS '(' uint_lit ')'" );
      DUMP_INT( "uint_lit", $bytes );

      $$.kind = C_ALIGNAS_BYTES;
      $$.loc = @$;
      $$.bytes = $bytes;

      DUMP_ALIGN( "$$_align", $$ );
      DUMP_END();
    }

  | alignas lparen_exp sname_c[sname] rparen_exp
    {
      DUMP_START( "alignas_specifier_c", "ALIGNAS '(' sname_c ')'" );
      DUMP_SNAME( "sname", $sname );

      $$.kind = C_ALIGNAS_SNAME;
      $$.loc = @$;
      $$.sname = c_sname_move( &$sname );

      DUMP_ALIGN( "$$_align", $$ );
      DUMP_END();
    }

  | alignas lparen_exp type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    cast_c_astp_opt[cast_astp] rparen_exp
    {
      ia_type_ast_pop();

      DUMP_START( "alignas_specifier_c",
                  "ALIGNAS '(' type_c_ast cast_c_astp_opt ')'" );
      DUMP_AST( "type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );

      $$.kind = C_ALIGNAS_TYPE;
      $$.loc = @$;
      $$.type_ast = c_ast_patch_placeholder( $type_ast, $cast_astp.ast );

      DUMP_ALIGN( "$$_align", $$ );
      DUMP_END();
    }

  | alignas lparen_exp error ')'
    {
      elaborate_error( "integer or type expected" );
    }
  ;

alignas
  : Y__Alignas
  | Y_alignas
  ;

/// Gibberish C/C++ asm declaration ///////////////////////////////////////////

asm_declaration_c
  : Y_asm lparen_exp str_lit_exp[str] rparen_exp
    {
      free( $str );
      UNSUPPORTED( &@Y_asm, "asm declarations" );
    }
  ;

/// Gibberish C/C++ scoped declarations ///////////////////////////////////////

scoped_declaration_c
  : class_struct_union_declaration_c
  | enum_declaration_c
  | namespace_declaration_c
  ;

class_struct_union_declaration_c
    /*
     * C++ scoped declaration, e.g.: class C { typedef int I; };
     */
  : class_struct_union_btid[csu_btid]
    {
      PARSE_ASSERT( is_nested_type_ok( &@csu_btid ) );
      gibberish_to_english();           // see the comment in "explain"
    }
    any_sname_c_exp[sname]
    {
      c_type_t const *const cur_type =
        c_sname_local_type( &in_attr.scope_sname );
      if ( c_tid_is_any( cur_type->btids, TB_ANY_CLASS ) ) {
        char const *const cur_name = c_sname_local_name( &in_attr.scope_sname );
        char const *const mbr_name = c_sname_local_name( &$sname );
        if ( strcmp( mbr_name, cur_name ) == 0 ) {
          print_error( &@sname,
            "\"%s\": member has the same name as its enclosing %s\n",
            mbr_name, c_type_gibberish( cur_type )
          );
          c_sname_cleanup( &$sname );
          PARSE_ABORT();
        }
      }

      DUMP_START( "class_struct_union_declaration_c",
                  "class_struct_union_btid any_sname_c_exp" );
      DUMP_SNAME( "in_attr__scope_sname", in_attr.scope_sname );
      DUMP_TID( "class_struct_union_btid", $csu_btid );
      DUMP_SNAME( "any_sname_c_exp", $sname );

      c_sname_push_back_sname( &in_attr.scope_sname, &$sname );

      c_sname_local_data( &in_attr.scope_sname )->type =
        C_TYPE_LIT_B( $csu_btid );
      c_sname_set_all_types( &in_attr.scope_sname );

      c_sname_t csu_sname = c_sname_dup( &in_attr.scope_sname );
      c_sname_push_back_sname( &csu_sname, &$sname );

      c_ast_t *const csu_ast = c_ast_new_gc( K_CLASS_STRUCT_UNION, &@sname );
      csu_ast->sname = csu_sname;
      csu_ast->type.btids = c_tid_check( $csu_btid, C_TPID_BASE );
      c_sname_init_name(
        &csu_ast->csu.csu_sname,
        check_strdup( c_sname_local_name( &csu_ast->sname ) )
      );

      DUMP_AST( "$$_ast", csu_ast );
      DUMP_END();

      PARSE_ASSERT( define_type( csu_ast, C_GIB_TYPEDEF ) );
    }
    brace_in_scope_declaration_c_opt
  ;

enum_declaration_c
    /*
     * C/C++ enum declaration, e.g.: enum E;
     */
  : enum_btids
    {
      PARSE_ASSERT( is_nested_type_ok( &@enum_btids ) );
      gibberish_to_english();           // see the comment in "explain"
    }
    any_sname_c_exp[sname] enum_fixed_type_c_ast_opt[fixed_type_ast]
    {
      DUMP_START( "enum_declaration_c",
                  "enum_btids any_sname_c_exp enum_fixed_type_c_ast_opt" );
      DUMP_TID( "enum_btids", $enum_btids );
      DUMP_SNAME( "any_sname_c_exp", $sname );
      DUMP_AST( "enum_fixed_type_c_ast_opt", $fixed_type_ast );

      c_sname_t enum_sname = c_sname_dup( &in_attr.scope_sname );
      c_sname_push_back_sname( &enum_sname, &$sname );

      c_sname_local_data( &enum_sname )->type = C_TYPE_LIT_B( $enum_btids );
      c_sname_set_all_types( &enum_sname );

      c_ast_t *const enum_ast = c_ast_new_gc( K_ENUM, &@sname );
      enum_ast->sname = enum_sname;
      enum_ast->type.btids = c_tid_check( $enum_btids, C_TPID_BASE );
      c_ast_set_parent( $fixed_type_ast, enum_ast );
      c_sname_init_name(
        &enum_ast->enum_.enum_sname,
        check_strdup( c_sname_local_name( &enum_ast->sname ) )
      );

      DUMP_AST( "$$_ast", enum_ast );
      DUMP_END();

      PARSE_ASSERT( define_type( enum_ast, C_GIB_TYPEDEF ) );
    }
  ;

namespace_declaration_c
    /*
     * C++ namespace declaration, e.g.: namespace NS { typedef int I; }
     */
  : namespace_type
    {
      gibberish_to_english();           // see the comment in "explain"
    }
    namespace_sname_c_exp[sname]
    {
      DUMP_START( "namespace_declaration_c",
                  "namespace_type namespace_sname_c_exp" );
      DUMP_SNAME( "in_attr__scope_sname", in_attr.scope_sname );
      DUMP_TYPE( "namespace_type", $namespace_type );
      DUMP_SNAME( "namespace_sname_c_exp", $sname );

      //
      // Nested namespace declarations are supported only in C++17 and later.
      // (However, we always allow them in configuration files.)
      //
      // This check has to be done now in the parser rather than later in the
      // AST because the AST has no "memory" of how a namespace was
      // constructed.
      //
      if ( c_sname_count( &$sname ) > 1 && !OPT_LANG_IS( NESTED_namespace ) ) {
        print_error( &@sname,
          "nested namespace declarations not supported%s\n",
          C_LANG_WHICH( NESTED_namespace )
        );
        c_sname_cleanup( &$sname );
        PARSE_ABORT();
      }

      //
      // The namespace type (plain namespace or inline namespace) has to be
      // split across the namespace sname: only the storage classes (for
      // TS_inline) has to be or'd with the first scope-type of the sname ...
      //
      c_type_t *const sname_global_type = &c_sname_global_data( &$sname )->type;
      *sname_global_type =
        C_TYPE_LIT(
          sname_global_type->btids,
          sname_global_type->stids | $namespace_type.stids,
          sname_global_type->atids
        );
      //
      // ... and only the base types (for TB_namespace) has to be or'd with the
      // local scope type of the sname.
      //
      c_type_t *const sname_local_type = &c_sname_local_data( &$sname )->type;
      *sname_local_type =
        C_TYPE_LIT(
          sname_local_type->btids | $namespace_type.btids,
          sname_local_type->stids,
          sname_local_type->atids
        );

      c_sname_push_back_sname( &in_attr.scope_sname, &$sname );
      c_sname_set_all_types( &in_attr.scope_sname );

      DUMP_SNAME( "$$_sname", $sname );
      DUMP_END();

      //
      // These checks are better to do now in the parser rather than later in
      // the AST because they give a better error location.
      //
      if ( c_sname_is_inline_nested_namespace( &in_attr.scope_sname ) ) {
        print_error( &@sname,
          "nested namespace can not be %s\n",
          c_tid_error( TS_inline )
        );
        PARSE_ABORT();
      }
      PARSE_ASSERT( c_sname_check( &in_attr.scope_sname, &@sname ) );
    }
    brace_in_scope_declaration_c_exp
  ;

namespace_sname_c_exp
  : namespace_sname_c
  | namespace_typedef_sname_c
  | error
    {
      c_sname_init( &$$ );
      elaborate_error( "name expected" );
    }
  ;

  /*
   * A version of sname_c for namespace declarations that has an extra
   * production for C++20 nested inline namespaces.  See sname_c for detailed
   * comments.
   */
namespace_sname_c
  : namespace_sname_c[sname] Y_COLON_COLON Y_NAME[name]
    {
      DUMP_START( "namespace_sname_c", "sname_c '::' NAME" );
      DUMP_SNAME( "namespace_sname_c", $sname );
      DUMP_STR( "name", $name );

      $$ = $sname;
      c_sname_push_back_name( &$$, $name );
      c_sname_local_data( &$$ )->type = C_TYPE_LIT_B( TB_namespace );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }

  | namespace_sname_c[sname] Y_COLON_COLON any_typedef_tdef[tdef]
    {
      DUMP_START( "namespace_sname_c",
                  "namespace_sname_c '::' any_typedef_tdef" );
      DUMP_SNAME( "namespace_sname_c", $sname );
      DUMP_AST( "any_typedef__ast", $tdef->ast );

      $$ = $sname;
      c_sname_local_data( &$$ )->type = C_TYPE_LIT_B( TB_namespace );
      c_sname_t temp_sname = c_sname_dup( &$tdef->ast->sname );
      c_sname_push_back_sname( &$$, &temp_sname );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }

  | namespace_sname_c[sname] Y_COLON_COLON Y_inline[inline_stid] name_exp[name]
    {
      DUMP_START( "namespace_sname_c", "sname_c '::' NAME INLINE NAME" );
      DUMP_SNAME( "namespace_sname_c", $sname );
      DUMP_STR( "name", $name );

      $$ = $sname;
      c_sname_push_back_name( &$$, $name );
      c_sname_local_data( &$$ )->type =
        C_TYPE_LIT( TB_namespace, $inline_stid, TA_NONE );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }

  | Y_NAME[name]
    {
      DUMP_START( "namespace_sname_c", "NAME" );
      DUMP_STR( "NAME", $name );

      c_sname_init_name( &$$, $name );
      c_sname_local_data( &$$ )->type = C_TYPE_LIT_B( TB_namespace );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }
  ;

  /*
   * A version of typedef_sname_c for namespace declarations that has an extra
   * production for C++20 nested inline namespaces.  See typedef_sname_c for
   * detailed comments.
   */
namespace_typedef_sname_c
  : namespace_typedef_sname_c[ns_sname] Y_COLON_COLON sname_c[sname]
    {
      DUMP_START( "namespace_typedef_sname_c",
                  "namespace_typedef_sname_c '::' sname_c" );
      DUMP_SNAME( "namespace_typedef_sname_c", $ns_sname );
      DUMP_SNAME( "sname_c", $sname );

      $$ = $ns_sname;
      c_sname_push_back_sname( &$$, &$sname );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }

  | namespace_typedef_sname_c[ns_sname] Y_COLON_COLON any_typedef_tdef[tdef]
    {
      DUMP_START( "namespace_typedef_sname_c",
                  "namespace_typedef_sname_c '::' any_typedef_tdef" );
      DUMP_SNAME( "namespace_typedef_sname_c", $ns_sname );
      DUMP_AST( "any_typedef__ast", $tdef->ast );

      $$ = $ns_sname;
      c_sname_local_data( &$$ )->type =
        *c_sname_local_type( &$tdef->ast->sname );
      c_sname_t temp_sname = c_sname_dup( &$tdef->ast->sname );
      c_sname_push_back_sname( &$$, &temp_sname );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }

  | namespace_typedef_sname_c[ns_sname] Y_COLON_COLON Y_inline[inline_stid]
    name_exp[name]
    {
      DUMP_START( "namespace_typedef_sname_c",
                  "namespace_typedef_sname_c '::' INLINE NAME" );
      DUMP_SNAME( "namespace_typedef_sname_c", $ns_sname );
      DUMP_STR( "name", $name );

      $$ = $ns_sname;
      c_sname_push_back_name( &$$, $name );
      c_sname_local_data( &$$ )->type =
        C_TYPE_LIT( TB_namespace, $inline_stid, TA_NONE );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }

  | any_typedef_tdef[tdef]        { $$ = c_sname_dup( &$tdef->ast->sname ); }
  ;

brace_in_scope_declaration_c_exp
  : brace_in_scope_declaration_c
  | error
    {
      punct_expected( '{' );
    }
  ;

brace_in_scope_declaration_c_opt
  : /* empty */
  | brace_in_scope_declaration_c
  ;

brace_in_scope_declaration_c
  : '{' '}'
  | '{' in_scope_declaration_c_exp semi_opt rbrace_exp
  ;

in_scope_declaration_c_exp
  : scoped_declaration_c
  | typedef_declaration_c semi_exp
  | using_declaration_c semi_exp
  | error
    {
      elaborate_error( "declaration expected" );
    }
  ;

/// Gibberish C++ lambda declaration //////////////////////////////////////////

lambda_declaration_c
  : '[' capture_decl_list_c_opt[capture_ast_list] ']'
    paren_param_c_ast_list_opt[param_ast_list]
    storage_class_subset_english_type_opt[type]
    lambda_return_type_c_ast_opt[ret_ast]
    {
      DUMP_START( "lambda_declaration_c",
                  "'[' capture_decl_list_c_opt ']' "
                  "paren_param_c_ast_list_opt "
                  "lambda_return_type_c_ast_opt" );
      DUMP_AST_LIST( "capture_decl_list_c_opt", $capture_ast_list );
      DUMP_AST_LIST( "paren_param_c_ast_list_opt", $param_ast_list );
      DUMP_TYPE( "storage_class_subset_english_type_opt", $type );
      DUMP_AST( "lambda_return_type_c_ast_opt", $ret_ast );

      c_ast_t *const lambda_ast = c_ast_new_gc( K_LAMBDA, &@$ );
      lambda_ast->type = $type;
      c_ast_list_set_param_of( &$capture_ast_list, lambda_ast );
      lambda_ast->lambda.capture_ast_list = slist_move( &$capture_ast_list );
      c_ast_list_set_param_of( &$param_ast_list, lambda_ast );
      lambda_ast->lambda.param_ast_list = slist_move( &$param_ast_list );
      c_ast_set_parent( $ret_ast, lambda_ast );

      DUMP_AST( "$$_ast", lambda_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( lambda_ast ) );
      c_ast_english( lambda_ast, C_ENG_DECL, stdout );
      PUTC( '\n' );
    }
  ;

capture_decl_list_c_opt
  : /* empty */                   { slist_init( &$$ ); }
  | capture_decl_list_c
  ;

capture_decl_list_c
  : capture_decl_list_c[capture_ast_list] comma_exp
    capture_decl_c_ast[capture_ast]
    {
      DUMP_START( "capture_decl_list_c",
                  "capture_decl_list_c ',' "
                  "capture_decl_c_ast" );
      DUMP_AST_LIST( "capture_decl_list_c", $capture_ast_list );
      DUMP_AST( "capture_decl_c_ast", $capture_ast );

      $$ = $capture_ast_list;
      slist_push_back( &$$, $capture_ast );

      DUMP_AST_LIST( "$$_ast_list", $$ );
      DUMP_END();
    }

  | capture_decl_c_ast[capture_decl_ast]
    {
      DUMP_START( "capture_decl_list_c",
                  "capture_decl_c_ast" );
      DUMP_AST( "capture_decl_c_ast", $capture_decl_ast );

      slist_init( &$$ );
      slist_push_back( &$$, $capture_decl_ast );

      DUMP_AST_LIST( "$$_ast_list", $$ );
      DUMP_END();
    }
  ;

capture_decl_c_ast
  : '&'
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      $$->capture.kind = C_CAPTURE_REFERENCE;
    }
  | '&' Y_NAME[name]
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      c_sname_init_name( &$$->sname, $name );
      $$->capture.kind = C_CAPTURE_REFERENCE;
    }
  | '='
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      $$->capture.kind = C_CAPTURE_COPY;
    }
  | Y_NAME[name]
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      c_sname_init_name( &$$->sname, $name );
    }
  | Y_this
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      $$->capture.kind = C_CAPTURE_THIS;
    }
  | '*' this_exp
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      $$->capture.kind = C_CAPTURE_STAR_THIS;
    }
  ;

lambda_return_type_c_ast_opt
  : /* empty */                   { $$ = NULL; }
  | Y_MINUS_GREATER type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    cast_c_astp_opt[cast_astp]
    {
      ia_type_ast_pop();

      DUMP_START( "lambda_return_type_c_ast_opt",
                  "'->' type_c_ast cast_c_astp_opt" );
      DUMP_AST( "type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );

      $$ = IF_ELSE_EXPR( $cast_astp.ast, $type_ast );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ template declaration ////////////////////////////////////////

template_declaration_c
  : Y_template
    {
      UNSUPPORTED( &@Y_template, "template declarations" );
    }
  ;

/// Gibberish C/C++ typed declaration /////////////////////////////////////////

typed_declaration_c
  : type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); } decl_list_c_opt
    {
      ia_type_ast_pop();
    }
  ;

/// Gibberish C/C++ typedef declaration ///////////////////////////////////////

typedef_declaration_c
  : Y_typedef typename_flag_opt[flag]
    {
      PARSE_ASSERT( is_nested_type_ok( &@Y_typedef ) );
      in_attr.is_typename = $flag;
      gibberish_to_english();           // see the comment in "explain"
    }
    type_c_ast[type_ast]
    {
      PARSE_ASSERT( !in_attr.is_typename || c_ast_is_typename_ok( $type_ast ) );
      // see the comment in "define_command" about TS_typedef
      PARSE_ASSERT(
        c_type_add_tid( &$type_ast->type, TS_typedef, &@type_ast )
      );
      ia_type_ast_push( $type_ast );
    }
    typedef_decl_list_c
    {
      ia_type_ast_pop();
    }
  ;

typedef_decl_list_c
  : typedef_decl_list_c ',' typedef_decl_c_exp
  | typedef_decl_c
  ;

typedef_decl_c
  : // in_attr: type_c_ast
    decl_c_astp[decl_astp]
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "typedef_decl_c", "decl_c_astp" );
      DUMP_SNAME( "in_attr__scope_sname", in_attr.scope_sname );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST_PAIR( "decl_c_astp", $decl_astp );

      c_ast_t *const decl_ast = $decl_astp.ast;
      c_ast_t *typedef_ast;
      c_sname_t temp_sname;

      if ( decl_ast->kind == K_TYPEDEF && c_sname_empty( &decl_ast->sname ) ) {
        //
        // This is for either of the following cases:
        //
        //      typedef int int32_t;
        //      typedef int32_t int_least32_t;
        //
        // that is: any type name followed by an existing typedef name.
        //
        typedef_ast = type_ast;
        if ( c_sname_empty( &typedef_ast->sname ) )
          typedef_ast->sname = c_sname_dup( &decl_ast->tdef.for_ast->sname );
      }
      else {
        //
        // This is for either of the following cases:
        //
        //      typedef int foo;
        //      typedef int32_t foo;
        //
        // that is: any type name followed by a new name.
        //
        typedef_ast = c_ast_patch_placeholder( type_ast, decl_ast );
        temp_sname = c_ast_move_sname( decl_ast );
        c_sname_set( &typedef_ast->sname, &temp_sname );
      }

      PARSE_ASSERT( c_ast_check( typedef_ast ) );
      // see the comment in "define_command" about TS_typedef
      PJL_DISCARD_RV( c_ast_take_type_any( typedef_ast, &T_TS_typedef ) );

      if ( c_sname_count( &typedef_ast->sname ) > 1 ) {
        print_error( &@decl_astp,
          "typedef names can not be scoped; "
          "use: namespace %s { typedef ... }\n",
          c_sname_scope_gibberish( &typedef_ast->sname )
        );
        PARSE_ABORT();
      }

      temp_sname = c_sname_dup( &in_attr.scope_sname );
      c_sname_push_front_sname( &typedef_ast->sname, &temp_sname );

      DUMP_AST( "$$_ast", typedef_ast );
      DUMP_END();

      PARSE_ASSERT( define_type( typedef_ast, C_GIB_TYPEDEF  ) );
    }
  ;

typedef_decl_c_exp
  : typedef_decl_c
  | error
    {
      elaborate_error( "type expected" );
    }
  ;

/// Gibberish C++ user-defined conversion operator declaration ////////////////

user_defined_conversion_declaration_c
  : user_defined_conversion_decl_c_astp[decl_astp]
    {
      DUMP_START( "user_defined_conversion_declaration_c",
                  "user_defined_conversion_decl_c_astp" );
      DUMP_AST_PAIR( "user_defined_conversion_declaration_c", $decl_astp );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( $decl_astp.ast ) );
      c_ast_english( $decl_astp.ast, C_ENG_DECL, stdout );
      PUTC( '\n' );
    }
  ;

/// Gibberish C++ using declaration ///////////////////////////////////////////

using_declaration_c
  : using_decl_c_ast[decl_ast]
    {
      // see the comment in "define_command" about TS_typedef
      PJL_DISCARD_RV( c_ast_take_type_any( $decl_ast, &T_TS_typedef ) );

      PARSE_ASSERT( define_type( $decl_ast, C_GIB_USING ) );
    }
  ;

using_decl_c_ast
  : Y_using
    { //
      // Using declarations are supported only in C++11 and later.  (However,
      // we always allow them in configuration files.)
      //
      // This check has to be done now in the parser rather than later in the
      // AST because using declarations are treated like typedef declarations
      // and the AST has no "memory" that such a declaration was a using
      // declaration.
      //
      if ( !OPT_LANG_IS( using_DECLS ) ) {
        print_error( &@Y_using,
          "\"using\" not supported%s\n",
          C_LANG_WHICH( using_DECLS )
        );
        PARSE_ABORT();
      }

      gibberish_to_english();           // see the comment in "explain"
    }
    any_name_exp[name] attribute_specifier_list_c_atid_opt[atids] equals_exp
    type_c_ast[type_ast]
    {
      // see the comment in "define_command" about TS_typedef
      if ( !c_type_add_tid( &$type_ast->type, TS_typedef, &@type_ast ) ) {
        free( $name );
        PARSE_ABORT();
      }
      ia_type_ast_push( $type_ast );
    }
    cast_c_astp_opt[cast_astp]
    {
      ia_type_ast_pop();

      DUMP_START( "using_decl_c_ast",
                  "USING any_name_exp attribute_specifier_list_c_atid_opt '=' "
                  "type_c_ast cast_c_astp_opt" );
      DUMP_SNAME( "in_attr__scope_sname", in_attr.scope_sname );
      DUMP_STR( "any_name_exp", $name );
      DUMP_TID( "attribute_specifier_list_c_atid_opt", $atids );
      DUMP_AST( "type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );

      c_ast_t *const cast_ast = $cast_astp.ast;

      //
      // Ensure the type on the right-hand side doesn't have a name, e.g.:
      //
      //      using U = void (*F)();    // error
      //
      // This check has to be done now in the parser rather than later in the
      // AST because the patched AST loses the name.
      //
      c_sname_t const *const sname = c_ast_find_name( cast_ast, C_VISIT_DOWN );
      if ( sname != NULL ) {
        print_error( &cast_ast->loc, "\"using\" type can not have a name\n" );
        free( $name );
        PARSE_ABORT();
      }

      c_sname_t temp_sname = c_sname_dup( &in_attr.scope_sname );
      c_sname_push_back_name( &temp_sname, $name );

      $$ = c_ast_patch_placeholder( $type_ast, cast_ast );
      c_sname_set( &$$->sname, &temp_sname );
      $$->type.atids = c_tid_check( $atids, C_TPID_ATTR );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( $$ ) );
    }
  ;

/// Gibberish C/C++ declarations //////////////////////////////////////////////

decl_list_c_opt
    /*
     * Either:
     *
     * + An enum, class, struct, or union (ECSU) declaration by itself, e.g.:
     *
     *          explain struct S
     *
     *   without any object of that type.
     *
     * + A structured binding:
     *
     *          auto [x, y]
     */
  : // in_attr: type_c_ast
    /* empty */
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "decl_list_c_opt", "<empty>" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );

      if ( in_attr.align.kind != C_ALIGNAS_NONE ) {
        print_error( &in_attr.align.loc,
          "\"%s\" invalid here\n",
          alignas_name()
        );
        PARSE_ABORT();
      }

      bool const is_structured_binding =
        c_ast_unreference_any( type_ast )->kind == K_STRUCTURED_BINDING;

      if ( is_structured_binding ) {
        // do nothing
      }
      else if ( (type_ast->kind & K_ANY_ECSU) == 0 ) {
        //
        // The declaration is a non-ECSU type, e.g.:
        //
        //      int
        //
        c_loc_t const loc = lexer_loc();
        print_error( &loc, "declaration expected" );
        print_error_token_is_a( printable_yytext() );
        EPUTC( '\n' );
        PARSE_ABORT();
      }
      else {
        c_sname_t const *const ecsu_sname = &type_ast->csu.csu_sname;
        assert( !c_sname_empty( ecsu_sname ) );

        if ( c_sname_count( ecsu_sname ) > 1 ) {
          print_error( &type_ast->loc,
            "forward declaration can not have a scoped name\n"
          );
          PARSE_ABORT();
        }

        c_sname_t temp_sname = c_sname_dup( ecsu_sname );
        c_sname_set( &type_ast->sname, &temp_sname );
      }

      DUMP_AST( "$$_ast", type_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( type_ast ) );
      if ( is_structured_binding )
        c_ast_english( type_ast, C_ENG_DECL, stdout );
      else
        c_typedef_english( &C_TYPEDEF_LIT( type_ast, C_ENG_DECL ), stdout );
      PUTC( '\n' );
    }

  | decl_list_c[decl_ast_list]
    {
      DUMP_START( "decl_list_c_opt", "decl_list_c" );
      DUMP_AST_LIST( "decl_list_c", $decl_ast_list );
      DUMP_END();

      PARSE_ASSERT( c_ast_list_check( &$decl_ast_list ) );
      c_ast_list_english( &$decl_ast_list, stdout );
    }
  ;

decl_list_c
  : decl_list_c[decl_ast_list] ',' decl_c_exp[decl_ast]
    {
      $$ = $decl_ast_list;
      slist_push_back( &$$, $decl_ast );
    }
  | decl_c[decl_ast]
    {
      slist_init( &$$ );
      slist_push_back( &$$, $decl_ast );
    }
  ;

decl_c
  : // in_attr: alignas_specifier_c typename_flag_opt type_c_ast
    decl_c_astp[decl_astp]
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "decl_c", "decl_c_astp" );
      switch ( in_attr.align.kind ) {
        case C_ALIGNAS_NONE:
          break;
        case C_ALIGNAS_BYTES:
          DUMP_INT( "in_attr__alignas_bytes", in_attr.align.bytes );
          break;
        case C_ALIGNAS_SNAME:
          DUMP_SNAME( "in_attr__alignas_sname", in_attr.align.sname );
          break;
        case C_ALIGNAS_TYPE:
          DUMP_AST( "in_attr__alignas_type_ast", in_attr.align.type_ast );
          break;
      } // switch
      DUMP_BOOL( "in_attr__typename_flag_opt", in_attr.is_typename );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST_PAIR( "decl_c_astp", $decl_astp );

      $$ = join_type_decl( type_ast, $decl_astp.ast );
      PARSE_ASSERT( $$ != NULL );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

decl_c_exp
  : decl_c
  | error
    {
      elaborate_error( "declaration expected" );
    }
  ;

decl_c_astp
  : decl2_c_astp
  | pointer_decl_c_astp
  | pointer_to_member_decl_c_astp
  | reference_decl_c_astp
  | msc_calling_convention_atid[atid] msc_calling_convention_c_astp[astp]
    {
      $$ = $astp;
      $$.ast->loc = @$;
      $$.ast->type.atids |= $atid;
    }
  ;

msc_calling_convention_c_astp
  : func_decl_c_astp
  | pointer_decl_c_astp
  ;

decl2_c_astp
  : array_decl_c_astp
  | block_decl_c_astp
  | func_decl_c_astp
  | nested_decl_c_astp
  | oper_decl_c_astp
  | param_pack_decl_c_ast[ast]     { $$ = (c_ast_pair_t){ $ast, NULL }; }
  | sname_c_ast[ast] gnu_attribute_specifier_list_c_opt
    {
      $$ = (c_ast_pair_t){ $ast, .target_ast = NULL };
    }
  | typedef_type_decl_c_ast[ast]
    {
      $$ = (c_ast_pair_t){ $ast, .target_ast = NULL };
    }
  | user_defined_conversion_decl_c_astp
  | user_defined_literal_decl_c_astp
  ;

param_pack_decl_c_ast
  : Y_ELLIPSIS sname_c_ast[ast]
    {
      DUMP_START( "param_pack_decl_c_ast", "... sname_c_ast" );\
      DUMP_AST( "sname_c_ast", $ast );

      c_ast_set_parameter_pack( $ast );
      $$ = $ast;

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  | Y_ELLIPSIS error
    {
      elaborate_error( "name expected" );
      $$ = NULL;
    }
  ;

/// Gibberish C/C++ array declaration /////////////////////////////////////////

array_decl_c_astp
  : // in_attr: type_c_ast
    decl2_c_astp[decl_astp] array_size_c_ast[array_ast]
    gnu_attribute_specifier_list_c_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "array_decl_c_astp", "decl2_c_astp array_size_c_ast" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST_PAIR( "decl2_c_astp", $decl_astp );
      DUMP_AST( "array_size_c_ast", $array_ast );

      c_ast_set_parent(
        c_ast_new_gc( K_PLACEHOLDER, &@decl_astp ), $array_ast
      );

      c_ast_t *const of_ast = ia_type_spec_ast( type_ast );
      if ( $decl_astp.target_ast != NULL ) {
        // array-of or function-like-ret type
        $$ = (c_ast_pair_t){
          $decl_astp.ast,
          c_ast_add_array( $decl_astp.target_ast, $array_ast, of_ast )
        };
      }
      else {
        $$ = (c_ast_pair_t){
          c_ast_add_array( $decl_astp.ast, $array_ast, of_ast ),
          .target_ast = NULL
        };
      }

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

array_size_c_ast
  : '[' rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_SIZE_NONE;
    }
  | '[' uint_lit[size] rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_SIZE_INT;
      $$->array.size_int = $size;
    }
  | '[' Y_NAME[name] rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_SIZE_NAME;
      $$->array.size_name = $name;
    }
  | '[' type_qualifier_list_c_stid[qual_stids] rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids = c_tid_check( $qual_stids, C_TPID_STORE );
      $$->array.kind = C_ARRAY_SIZE_NONE;
    }
  | '[' type_qualifier_list_c_stid[qual_stids] static_stid_opt[static_stid]
    uint_lit[size] rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids = c_tid_check( $qual_stids | $static_stid, C_TPID_STORE );
      $$->array.kind = C_ARRAY_SIZE_INT;
      $$->array.size_int = $size;
    }
  | '[' type_qualifier_list_c_stid[qual_stids] static_stid_opt[static_stid]
    Y_NAME[name] rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids = c_tid_check( $qual_stids | $static_stid, C_TPID_STORE );
      $$->array.kind = C_ARRAY_SIZE_NAME;
      $$->array.size_name = $name;
    }
  | '[' type_qualifier_list_c_stid_opt[qual_stids] '*' rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids = c_tid_check( $qual_stids, C_TPID_STORE );
      $$->array.kind = C_ARRAY_SIZE_VLA;
    }
  | '[' Y_static type_qualifier_list_c_stid_opt[qual_stids] uint_lit[size]
    rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids =
        c_tid_check( TS_NON_EMPTY_ARRAY | $qual_stids, C_TPID_STORE );
      $$->array.kind = C_ARRAY_SIZE_INT;
      $$->array.size_int = $size;
    }
  | '[' Y_static type_qualifier_list_c_stid_opt[qual_stids] Y_NAME[name]
    rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids =
        c_tid_check( TS_NON_EMPTY_ARRAY | $qual_stids, C_TPID_STORE );
      $$->array.kind = C_ARRAY_SIZE_NAME;
      $$->array.size_name = $name;
    }
  | '[' error ']'
    {
      elaborate_error( "array size expected" );
    }
  ;

/// Gibberish C/C++ block declaration (Apple extension) ///////////////////////

block_decl_c_astp                       // Apple extension
  : // in_attr: type_c_ast
    '(' '^'
    { //
      // A block AST has to be the type inherited attribute for decl_c_astp so
      // we have to create it here.
      //
      ia_type_ast_push( c_ast_new_gc( K_APPLE_BLOCK, &@$ ) );
    }
    type_qualifier_list_c_stid_opt[qual_stids] decl_c_astp[decl_astp] rparen_exp
    lparen_exp param_c_ast_list_opt[param_ast_list] ')'
    gnu_attribute_specifier_list_c_opt
    {
      c_ast_t *const block_ast = ia_type_ast_pop();
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "block_decl_c_astp",
                  "'(' '^' type_qualifier_list_c_stid_opt decl_c_astp ')' "
                  "'(' param_c_ast_list_opt ')'" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $qual_stids );
      DUMP_AST_PAIR( "decl_c_astp", $decl_astp );
      DUMP_AST_LIST( "param_c_ast_list_opt", $param_ast_list );

      PARSE_ASSERT(
        c_type_add_tid( &block_ast->type, $qual_stids, &@qual_stids )
      );
      c_ast_list_set_param_of( &$param_ast_list, block_ast );
      block_ast->block.param_ast_list = slist_move( &$param_ast_list );

      c_ast_t *const ret_ast = ia_type_spec_ast( type_ast );
      if ( $decl_astp.target_ast != NULL ) {
        $$ = (c_ast_pair_t){
          $decl_astp.ast,
          c_ast_add_func( $decl_astp.target_ast, block_ast, ret_ast )
        };
      }
      else {
        $$ = (c_ast_pair_t){
          c_ast_add_func( $decl_astp.ast, block_ast, ret_ast ),
          block_ast->block.ret_ast
        };
      }

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ in-class destructor declaration /////////////////////////////

destructor_declaration_c
    /*
     * C++ in-class destructor declaration, e.g.: ~S().
     */
  : virtual_stid_opt[virtual_stid] '~' any_name_exp[name]
    lparen_exp no_destructor_params
    param_list_rparen_func_qualifier_list_c_stid_opt[qual_stids]
    noexcept_c_stid_opt[noexcept_stid] gnu_attribute_specifier_list_c_opt
    func_equals_c_stid_opt[equals_stid]
    {
      DUMP_START( "destructor_declaration_c",
                  "virtual_stid_opt '~' any_name_exp '(' ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt "
                  "gnu_attribute_specifier_list_c_opt func_equals_c_stid_opt" );
      DUMP_TID( "virtual_stid_opt", $virtual_stid );
      DUMP_STR( "any_name_exp", $name );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $qual_stids );
      DUMP_TID( "noexcept_c_stid_opt", $noexcept_stid );
      DUMP_TID( "func_equals_c_stid_opt", $equals_stid );

      c_ast_t *const dtor_ast = c_ast_new_gc( K_DESTRUCTOR, &@$ );
      c_sname_init_name( &dtor_ast->sname, $name );
      dtor_ast->type.stids = c_tid_check(
        $virtual_stid | $qual_stids | $noexcept_stid | $equals_stid,
        C_TPID_STORE
      );

      DUMP_AST( "$$_ast", dtor_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( dtor_ast ) );
      c_ast_english( dtor_ast, C_ENG_DECL, stdout );
      PUTC( '\n' );
    }
  ;

no_destructor_params
    /*
     * Ensure destructors do _not_ have parameters.
     *
     * This rule isn't necessary.  The grammar could expect '(' immediately
     * followed by ')', but then we'd get an error message like:
     *
     *      c++decl> explain ~C(int)
     *                          ^
     *      12: syntax error: "int": ')' expected
     *
     * that isn't helpful.  Better would be to say "destructors may not have
     * parameters" explicity, hence this rule.
     *
     * This check is done now in the parser rather than later in the AST since
     * it would be silly to parse and store a parameter list for a destructor
     * in the AST when destructors don't have parameters.
     */
  : /* empty */
  | error
    { //
      // Ordinarily, we'd get an error message like:
      //
      //      c++decl> explain ~C(int)
      //                          ^
      //      12: syntax error: "int": destructors may not have parameters
      //      ("int" is a keyword)
      //
      // where the <<("int" is a keyword)>> part seems wrong since the user
      // didn't try using "int" as an ordinary identifier.  To suppress that
      // part, set yytext to the empty string so elaborate_error() won't print
      // it nor call print_error_token_is_a() and and we'll instead get an
      // error message like:
      //
      //      c++decl> explain ~C(int)
      //                          ^
      //      12: syntax error: destructors may not have parameters
      //
      set_yytext( "" );
      elaborate_error( "destructors may not have parameters" );
    }
  ;

/// Gibberish C++ file-scope constructor declaration //////////////////////////

file_scope_constructor_declaration_c
  : inline_stid_opt[inline_stid] Y_CONSTRUCTOR_SNAME[sname]
    lparen_exp param_c_ast_list_opt[param_ast_list]
    param_list_rparen_func_qualifier_list_c_stid_opt[qual_stids]
    noexcept_c_stid_opt[noexcept_stid] gnu_attribute_specifier_list_c_opt
    {
      DUMP_START( "file_scope_constructor_declaration_c",
                  "inline_opt CONSTRUCTOR_SNAME '(' param_c_ast_list_opt ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt" );
      DUMP_TID( "inline_stid_opt", $inline_stid );
      DUMP_SNAME( "CONSTRUCTOR_SNAME", $sname );
      DUMP_AST_LIST( "param_c_ast_list_opt", $param_ast_list );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $qual_stids );
      DUMP_TID( "noexcept_c_stid_opt", $noexcept_stid );

      c_sname_set_scope_type( &$sname, &C_TYPE_LIT_B( TB_class ) );

      c_ast_t *const ctor_ast = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      ctor_ast->sname = c_sname_move( &$sname );
      ctor_ast->type.stids = c_tid_check(
        $inline_stid | $qual_stids | $noexcept_stid,
        C_TPID_STORE
      );
      c_ast_list_set_param_of( &$param_ast_list, ctor_ast );
      ctor_ast->ctor.param_ast_list = slist_move( &$param_ast_list );

      DUMP_AST( "$$_ast", ctor_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( ctor_ast ) );
      c_ast_english( ctor_ast, C_ENG_DECL, stdout );
      PUTC( '\n' );
    }
  ;

/// Gibberish C++ file-scope destructor declaration ///////////////////////////

file_scope_destructor_declaration_c
  : inline_stid_opt[inline_stid] destructor_sname[sname]
    lparen_exp no_destructor_params
    param_list_rparen_func_qualifier_list_c_stid_opt[qual_stids]
    noexcept_c_stid_opt[noexcept_stid] gnu_attribute_specifier_list_c_opt
    {
      DUMP_START( "file_scope_destructor_declaration_c",
                  "inline_opt destructor_sname '(' ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt" );
      DUMP_TID( "inline_stid_opt", $inline_stid );
      DUMP_SNAME( "destructor_sname", $sname );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $qual_stids );
      DUMP_TID( "noexcept_c_stid_opt", $noexcept_stid );

      c_sname_set_scope_type( &$sname, &C_TYPE_LIT_B( TB_class ) );

      c_ast_t *const dtor_ast = c_ast_new_gc( K_DESTRUCTOR, &@$ );
      dtor_ast->sname = c_sname_move( &$sname );
      dtor_ast->type.stids = c_tid_check(
        $inline_stid | $qual_stids | $noexcept_stid,
        C_TPID_STORE
      );

      DUMP_AST( "$$_ast", dtor_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( dtor_ast ) );
      c_ast_english( dtor_ast, C_ENG_DECL, stdout );
      PUTC( '\n' );
    }
  ;

/// Gibberish C/C++ function declaration //////////////////////////////////////

func_decl_c_astp
    /*
     * Function and C++ constructor declaration.
     *
     * This grammar rule handles both since they're so similar (and so would
     * cause grammar conflicts if they were separate rules in an LALR(1)
     * parser).
     */
  : // in_attr: type_c_ast
    decl2_c_astp[decl_astp]
    param_list_c_lparen param_c_ast_list_opt[param_ast_list]
    param_list_rparen_func_qualifier_list_c_stid_opt[qual_stids]
    ref_qualifier_c_stid_opt[ref_qual_stid]
    noexcept_c_stid_opt[noexcept_stid]
    trailing_return_type_c_ast_opt[trailing_ret_ast]
    func_equals_c_stid_opt[equals_stid]
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "func_decl_c_astp",
                  "decl2_c_astp '(' param_c_ast_list_opt ')' "
                  "func_qualifier_list_c_stid_opt "
                  "ref_qualifier_c_stid_opt noexcept_c_stid_opt "
                  "trailing_return_type_c_ast_opt "
                  "func_equals_c_stid_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST_PAIR( "decl2_c_astp", $decl_astp );
      DUMP_AST_LIST( "param_c_ast_list_opt", $param_ast_list );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $qual_stids );
      DUMP_TID( "ref_qualifier_c_stid_opt", $ref_qual_stid );
      DUMP_TID( "noexcept_c_stid_opt", $noexcept_stid );
      DUMP_AST( "trailing_return_type_c_ast_opt", $trailing_ret_ast );
      DUMP_TID( "func_equals_c_stid_opt", $equals_stid );

      c_tid_t const func_stids = c_tid_check(
        $qual_stids | $ref_qual_stid | $noexcept_stid | $equals_stid,
        C_TPID_STORE
      );

      bool const assume_constructor = c_ast_is_likely_ctor( type_ast );

      c_ast_t *const func_ast =
        c_ast_new_gc( assume_constructor ? K_CONSTRUCTOR : K_FUNCTION, &@$ );
      func_ast->type.stids = func_stids;
      c_ast_list_set_param_of( &$param_ast_list, func_ast );
      func_ast->func.param_ast_list = slist_move( &$param_ast_list );

      c_ast_t *const decl_ast = $decl_astp.ast;

      if ( assume_constructor ) {
        assert( $trailing_ret_ast == NULL );
        $$ = (c_ast_pair_t){
          c_ast_add_func( decl_ast, func_ast, /*ret_ast=*/NULL ),
          .target_ast = NULL
        };
      }
      else {
        c_ast_t *ret_ast;
        c_ast_t *target_ast = $decl_astp.target_ast;

        if ( target_ast != NULL && target_ast->kind != K_POINTER ) {
          //
          // This is for a case like:
          //
          //      int f() ()
          //       |    |  |
          //       |    |  func
          //       |    |
          //       |    decl_ast (func)
          //       |
          //       ret_ast
          //
          // We replace ret_ast with decl_ast:
          //
          //      int f() ()
          //       |    |  |
          //       |    |  func
          //       X    |
          //       |    decl_ast (func)
          //       |
          //       ret_ast <- decl_ast (func)
          //
          // that is, a "function returning function" -- which is illegal
          // (since functions can't return functions) and will be caught by
          // c_ast_check_ret_type().
          //
          ret_ast = decl_ast;
          target_ast = NULL;
        }
        else {
          ret_ast = IF_ELSE_EXPR(
            $trailing_ret_ast, ia_type_spec_ast( type_ast )
          );
        }

        if ( target_ast != NULL ) {
          $$ = (c_ast_pair_t){
            decl_ast,
            c_ast_add_func( target_ast, func_ast, ret_ast )
          };
        }
        else {
          $$ = (c_ast_pair_t){
            c_ast_add_func( decl_ast, func_ast, ret_ast ),
            func_ast->func.ret_ast
          };
        }
      }

      c_tid_t const msc_call_atids = $$.ast->type.atids & TA_ANY_MSC_CALL;
      if ( msc_call_atids != TA_NONE ) {
        //
        // Microsoft calling conventions need to be moved from the pointer to
        // the function, e.g., change:
        //
        //      declare f as cdecl pointer to function returning void
        //
        // to:
        //
        //      declare f as pointer to cdecl function returning void
        //
        for ( c_ast_t *ast = $$.ast; (ast = c_ast_unpointer( ast )) != NULL; ) {
          if ( ast->kind == K_FUNCTION ) {
            $$.ast->type.atids &= c_tid_compl( TA_ANY_MSC_CALL );
            ast->type.atids |= msc_call_atids;
          }
        } // for
      }

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

pc99_func_or_constructor_declaration_c
    /*
     * Pre-C99 implicit int function and C++ in-class constructor declaration.
     *
     * This grammar rule handles both since they're so similar (and so would
     * cause grammar conflicts if they were separate rules in an LALR(1)
     * parser).
     */
  : Y_NAME[name] param_list_c_lparen
    {
      if ( OPT_LANG_IS( CONSTRUCTORS ) ) {
        //
        // In C++, encountering a name followed by '(' declares an in-class
        // constructor.  That means NAME is the name of a class, so we have to
        // create a temporary class type for it so if it's used as the type of
        // a parameter, e.g.:
        //
        //      C(C const&)
        //
        // it'll be recognized as such.
        //
        c_ast_t *const csu_ast = c_ast_new_gc( K_CLASS_STRUCT_UNION, &@name );
        csu_ast->type.btids = TB_class;
        c_sname_init_name( &csu_ast->csu.csu_sname, check_strdup( $name ) );
        csu_ast->sname = c_sname_dup( &csu_ast->csu.csu_sname );

        in_attr.tdef_rb = c_typedef_add( csu_ast, C_GIB_TYPEDEF );
        MAYBE_UNUSED c_typedef_t const *const csu_tdef =
          RB_DINT( in_attr.tdef_rb );
        assert( csu_tdef->ast == csu_ast );
      }
    }
    param_c_ast_list_opt[param_ast_list]
    param_list_rparen_func_qualifier_list_c_stid_opt[qual_stids]
    noexcept_c_stid_opt[noexcept_stid] func_equals_c_stid_opt[equals_stid]
    {
      DUMP_START( "pc99_func_or_constructor_declaration_c",
                  "NAME '(' param_c_ast_list_opt ')' noexcept_c_stid_opt "
                  "func_equals_c_stid_opt" );
      DUMP_STR( "NAME", $name );
      DUMP_AST_LIST( "param_c_ast_list_opt", $param_ast_list );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $qual_stids );
      DUMP_TID( "noexcept_c_stid_opt", $noexcept_stid );
      DUMP_TID( "func_equals_c_stid_opt", $equals_stid );

      c_ast_t *ast;

      if ( OPT_LANG_IS( CONSTRUCTORS ) ) {
        //
        // Remove the temporary typedef for the class.
        //
        c_typedef_remove( in_attr.tdef_rb );

        //
        // In C++, encountering a name followed by '(' declares an in-class
        // constructor.
        //
        ast = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      }
      else {
        if ( !OPT_LANG_IS( IMPLICIT_int ) ) {
          //
          // In C99 and later, implicit int is an error.  This check has to be
          // done now in the parser rather than later in the AST since the AST
          // would have no "memory" that the return type was implicitly int.
          //
          print_error( &@name,
            "implicit \"%s\" functions are illegal%s",
            c_tid_error( TB_int ),
            C_LANG_WHICH( IMPLICIT_int )
          );
          print_error_token_is_a( $name );
          if ( strcmp( $name, L_typeof ) == 0 ||
               strcmp( $name, L_typeof_unqual ) == 0 ) {
            EPRINTF( "; use \"%s\" instead", L_GNU___typeof__ );
          }
          else {
            print_suggestions(
              DYM_C_KEYWORDS | DYM_C_MACROS | DYM_C_TYPES,
              $name
            );
          }
          EPUTC( '\n' );
          free( $name );
          PARSE_ABORT();
        }

        //
        // In C prior to C99, encountering a name followed by '(' declares a
        // function that implicitly returns int:
        //
        //      power(x, n)             /* raise x to n-th power; n > 0 */
        //
        c_ast_t *const ret_ast = c_ast_new_gc( K_BUILTIN, &@name );
        ret_ast->type.btids = TB_int;

        ast = c_ast_new_gc( K_FUNCTION, &@$ );
        c_ast_set_parent( ret_ast, ast );
      }

      c_sname_init_name( &ast->sname, $name );
      ast->type.stids = c_tid_check(
        $qual_stids | $noexcept_stid | $equals_stid,
        C_TPID_STORE
      );
      c_ast_list_set_param_of( &$param_ast_list, ast );
      ast->func.param_ast_list = slist_move( &$param_ast_list );

      DUMP_AST( "$$_ast", ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( ast ) );
      c_ast_english( ast, C_ENG_DECL, stdout );
      PUTC( '\n' );
    }
  ;

param_list_rparen_func_qualifier_list_c_stid_opt
  : param_list_rparen
    {
      if ( OPT_LANG_IS( MEMBER_FUNCTIONS ) ) {
        //
        // Both "final" and "override" are matched only within member function
        // declarations.  Now that ')' has been parsed, we're within one, so
        // set the keyword context to C_KW_CTX_MBR_FUNC.
        //
        lexer_keyword_ctx = C_KW_CTX_MBR_FUNC;
      }
    }
    func_qualifier_list_c_stid_opt[stids]
    {
      lexer_keyword_ctx = C_KW_CTX_DEFAULT;
      $$ = $stids;
    }
  ;

func_qualifier_list_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | func_qualifier_list_c_stid_opt[stids] func_qualifier_c_stid[stid]
    {
      DUMP_START( "func_qualifier_list_c_stid_opt",
                  "func_qualifier_list_c_stid_opt func_qualifier_c_stid" );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $stids );
      DUMP_TID( "func_qualifier_c_stid", $stid );

      $$ = $stids;
      PARSE_ASSERT( c_tid_add( &$$, $stid, &@stid ) );

      DUMP_TID( "$$_stids", $$ );
      DUMP_END();
    }
  ;

func_qualifier_c_stid
  : cv_qualifier_stid
  | Y_final
  | Y_override
    /*
     * GNU C++ allows restricted-this-pointer member functions:
     *
     *      void S::f() __restrict;
     *
     * <https://gcc.gnu.org/onlinedocs/gcc/Restricted-Pointers.html>
     */
  | Y_GNU___restrict                    // GNU C++ extension
  ;

noexcept_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_noexcept
  | Y_noexcept '(' noexcept_bool_stid_exp[stid] rparen_exp
    {
      $$ = $stid;
    }
  | Y_throw lparen_exp rparen_exp
    {
      $$ = $Y_throw;
    }
  | Y_throw lparen_exp param_c_ast_list[ast_list] ')'
    {
      c_ast_list_cleanup( &$ast_list );

      if ( OPT_LANG_IS( throw ) )
        UNSUPPORTED( &@ast_list, "dynamic exception specifications" );

      print_error( &@ast_list,
        "dynamic exception specifications not supported%s\n",
        C_LANG_WHICH( throw )
      );
      PARSE_ABORT();
    }
  ;

noexcept_bool_stid_exp
  : Y_false
  | Y_true
  | error
    {
      elaborate_error( "\"true\" or \"false\" expected" );
    }
  ;

trailing_return_type_c_ast_opt
  : /* empty */                   { $$ = NULL; }
  | // in_attr: type_c_ast
    Y_MINUS_GREATER type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    cast_c_astp_opt[cast_astp]
    { //
      // The function trailing return-type syntax is supported only in C++11
      // and later.  This check has to be done now in the parser rather than
      // later in the AST because the AST has no "memory" of where the return-
      // type came from.
      //
      if ( !OPT_LANG_IS( TRAILING_RETURN_TYPES ) ) {
        print_error( &@Y_MINUS_GREATER,
          "trailing return type not supported%s\n",
          C_LANG_WHICH( TRAILING_RETURN_TYPES )
        );
        PARSE_ABORT();
      }

      ia_type_ast_pop();
      c_ast_t const *const ret_ast = ia_type_ast_peek();

      DUMP_START( "trailing_return_type_c_ast_opt",
                  "'->' type_c_ast cast_c_astp_opt" );
      DUMP_AST( "in_attr__type_c_ast", ret_ast );
      DUMP_AST( "type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );

      $$ = IF_ELSE_EXPR( $cast_astp.ast, $type_ast );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();

      //
      // Ensure that a function using the C++11 trailing return type syntax
      // starts with "auto":
      //
      //      void f() -> int
      //      ^
      //      error: must be "auto"
      //
      // This check has to be done now in the parser rather than later in the
      // AST because the "auto" is just a syntactic return-type placeholder in
      // C++11 and the AST node for the placeholder is discarded and never made
      // part of the AST.
      //
      if ( ret_ast->type.btids != TB_auto ) {
        print_error( &ret_ast->loc,
          "function with trailing return type must only specify \"%s\"\n",
          c_tid_error( TB_auto )
        );
        PARSE_ABORT();
      }
    }

  | gnu_attribute_specifier_list_c
    {
      $$ = NULL;
    }
  ;

func_equals_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | '=' Y_default                 { $$ = $2; }
  | '=' Y_delete delete_reason_opt
    {
      $$ = $2;
    }
  | '=' uint_lit
    {
      if ( $2 != 0 ) {
        print_error( &@2, "'0' expected\n" );
        PARSE_ABORT();
      }
      $$ = TS_PURE_virtual;
    }
  | '=' error
    {
      if ( OPT_LANG_IS( default_delete_FUNCS ) )
        elaborate_error( "'0', \"default\", or \"delete\" expected" );
      else
        elaborate_error( "'0' expected" );
    }
  ;

delete_reason_opt
  : /* empty */
  | '(' str_lit_exp[reason] rparen_exp
    {
      FREE( $reason );
      //
      // This check has to be done now in the parser rather than later in the
      // AST since we ignore the reason string.
      //
      if ( !OPT_LANG_IS( delete_REASON ) ) {
        print_error( &@reason,
          "\"delete\" with reason not supported%s\n",
          C_LANG_WHICH( delete_REASON )
        );
        PARSE_ABORT();
      }
    }
  ;

/// Gibberish C/C++ function(like) parameter declaration //////////////////////

param_c_ast_list_exp
  : param_c_ast_list
  | error
    {
      slist_init( &$$ );
      elaborate_error( "parameter list expected" );
    }
  ;

param_c_ast_list_opt
  : /* empty */                   { slist_init( &$$ ); }
  | param_c_ast_list
  ;

param_c_ast_list
  : param_c_ast_list[ast_list] comma_exp param_c_ast_exp[ast]
    {
      DUMP_START( "param_c_ast_list", "param_c_ast_list ',' param_c_ast" );
      DUMP_AST_LIST( "param_c_ast_list", $ast_list );
      DUMP_AST( "param_c_ast", $ast );

      $$ = $ast_list;
      slist_push_back( &$$, $ast );

      DUMP_AST_LIST( "$$_ast_list", $$ );
      DUMP_END();
    }

  | param_c_ast[param_ast]
    {
      DUMP_START( "param_c_ast_list", "param_c_ast" );
      DUMP_AST( "param_c_ast", $param_ast );

      slist_init( &$$ );
      slist_push_back( &$$, $param_ast );

      DUMP_AST_LIST( "$$_ast_list", $$ );
      DUMP_END();
    }
  ;

param_c_ast
    /*
     * Ordinary function parameter declaration.
     */
  : this_stid_opt[this_stid] type_c_ast[type_ast]
    {
      ia_type_ast_push( $type_ast );
    }
    cast_c_astp_opt[cast_astp]
    {
      ia_type_ast_pop();

      DUMP_START( "param_c_ast", "this_stid_opt type_c_ast cast_c_astp_opt" );
      DUMP_TID( "this_stid_opt", $this_stid );
      DUMP_AST( "type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );

      $$ = c_ast_patch_placeholder( $type_ast, $cast_astp.ast );

      if ( $$->kind == K_FUNCTION ) {
        //
        // From the C11 standard, section 6.3.2.1(4):
        //
        //    [A] function designator with type "function returning type" is
        //    converted to an expression that has type "pointer to function
        //    returning type."
        //
        $$ = c_ast_pointer( $$, &gc_ast_list );
      }

      PARSE_ASSERT( c_type_add_tid( &$$->type, $this_stid, &@this_stid ) );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

    /*
     * K&R C type-less function parameter declaration.
     */
  | name_ast

    /*
     * Varargs declaration.
     */
  | Y_ELLIPSIS
    {
      DUMP_START( "param_c_ast", "..." );

      $$ = c_ast_new_gc( K_VARIADIC, &@$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

param_c_ast_exp
  : param_c_ast
  | error
    {
      elaborate_error( "parameter declaration expected" );
    }
  ;

paren_param_c_ast_list_opt
  : /* empty */                   { slist_init( &$$ ); }
  | param_list_c_lparen param_c_ast_list_opt param_list_rparen
    {
      $$ = $2;
    }
  ;

param_list_c_lparen
  : '('                           { lexer_is_param_list_decl = true; }
  ;

param_list_rparen
  : ')'                           { lexer_is_param_list_decl = false; }
  ;

/// Gibberish C/C++ nested declaration ////////////////////////////////////////

nested_decl_c_astp
  : '('
    {
      ia_type_ast_push( c_ast_new_gc( K_PLACEHOLDER, &@$ ) );
      ++in_attr.ast_depth;
    }
    decl_c_astp[decl_astp] rparen_exp
    {
      ia_type_ast_pop();
      --in_attr.ast_depth;

      DUMP_START( "nested_decl_c_astp", "'(' decl_c_astp ')'" );
      DUMP_AST_PAIR( "decl_c_astp", $decl_astp );

      $$ = $decl_astp;
      $$.ast->loc = @$;

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ operator declaration ////////////////////////////////////////

oper_decl_c_astp
  : // in_attr: type_c_ast
    oper_sname_c_opt[sname] Y_operator c_operator[op_id] param_list_c_lparen
    param_c_ast_list_opt[param_ast_list]
    param_list_rparen_func_qualifier_list_c_stid_opt[qual_stids]
    ref_qualifier_c_stid_opt[ref_qual_stid]
    noexcept_c_stid_opt[noexcept_stid]
    trailing_return_type_c_ast_opt[trailing_ret_ast]
    func_equals_c_stid_opt[equals_stid]
    {
      c_operator_t const *const operator = c_op_get( $op_id );
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "oper_decl_c_astp",
                  "oper_sname_c_opt OPERATOR c_operator "
                  "'(' param_c_ast_list_opt ')' "
                  "func_qualifier_list_c_stid_opt "
                  "ref_qualifier_c_stid_opt noexcept_c_stid_opt "
                  "trailing_return_type_c_ast_opt "
                  "func_equals_c_stid_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_SNAME( "oper_sname_c_opt", $sname );
      DUMP_STR( "c_operator", operator->literal );
      DUMP_AST_LIST( "param_c_ast_list_opt", $param_ast_list );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $qual_stids );
      DUMP_TID( "ref_qualifier_c_stid_opt", $ref_qual_stid );
      DUMP_TID( "noexcept_c_stid_opt", $noexcept_stid );
      DUMP_AST( "trailing_return_type_c_ast_opt", $trailing_ret_ast );
      DUMP_TID( "func_equals_c_stid_opt", $equals_stid );

      c_tid_t const oper_stids = c_tid_check(
        $qual_stids | $ref_qual_stid | $noexcept_stid | $equals_stid,
        C_TPID_STORE
      );

      c_ast_t *const oper_ast = c_ast_new_gc( K_OPERATOR, &@$ );
      oper_ast->sname = c_sname_move( &$sname );
      oper_ast->type.stids = oper_stids;
      c_ast_list_set_param_of( &$param_ast_list, oper_ast );
      oper_ast->oper.param_ast_list = slist_move( &$param_ast_list );
      oper_ast->oper.operator = operator;

      c_ast_t *const ret_ast =
        IF_ELSE_EXPR( $trailing_ret_ast, ia_type_spec_ast( type_ast ) );

      $$ = (c_ast_pair_t){
        c_ast_add_func( type_ast, oper_ast, ret_ast ),
        oper_ast->oper.ret_ast
      };

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ pointer declaration ///////////////////////////////////////

pointer_decl_c_astp
  : pointer_type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    decl_c_astp[decl_astp]
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_decl_c_astp", "pointer_type_c_ast decl_c_astp" );
      DUMP_AST( "pointer_type_c_ast", $type_ast );
      DUMP_AST_PAIR( "decl_c_astp", $decl_astp );

      PJL_DISCARD_RV( c_ast_patch_placeholder( $type_ast, $decl_astp.ast ) );
      $$ = $decl_astp;
      $$.ast->loc = @$;

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

pointer_type_c_ast
  : // in_attr: type_c_ast
    '*' type_qualifier_list_c_stid_opt[qual_stids]
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "pointer_type_c_ast", "'*' type_qualifier_list_c_stid_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $qual_stids );

      $$ = c_ast_new_gc( K_POINTER, &@$ );
      $$->type.stids = c_tid_check( $qual_stids, C_TPID_STORE );
      c_ast_set_parent( ia_type_spec_ast( type_ast ), $$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish pre-C99 implicit int pointer declaration ////////////////////////

pc99_pointer_decl_list_c
  : pc99_pointer_decl_c[decl_ast] ',' decl_list_c[decl_ast_list]
    {
      $$ = $decl_ast_list;
      slist_push_front( &$$, $decl_ast );
    }
  | pc99_pointer_decl_c[decl_ast]
    {
      slist_init( &$$ );
      slist_push_back( &$$, $decl_ast );
    }
  ;

pc99_pointer_decl_c
  : pc99_pointer_type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    decl_c_astp[decl_astp]
    {
      ia_type_ast_pop();

      DUMP_START( "pc99_pointer_decl_c",
                  "pc99_pointer_type_c_ast decl_c_astp" );
      DUMP_AST( "pc99_pointer_type_c_ast", $type_ast );
      DUMP_AST_PAIR( "decl_c_astp", $decl_astp );

      $$ = c_ast_patch_placeholder( $type_ast, $decl_astp.ast );
      $$->loc = @$;

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

pc99_pointer_type_c_ast
  : // in_attr: type_c_ast
    '*'[star] type_qualifier_list_c_stid_opt[qual_stids]
    {
      if ( !OPT_LANG_IS( IMPLICIT_int ) ) {
        //
        // In C99 and later, implicit int is an error.  This check has to be
        // done now in the parser rather than later in the AST since the AST
        // would have no "memory" that the return type was implicitly int.
        //
        print_error( &@star,
          "implicit \"%s\" is illegal%s\n",
          c_tid_error( TB_int ),
          C_LANG_WHICH( IMPLICIT_int )
        );
        PARSE_ABORT();
      }

      c_ast_t *type_ast = ia_type_ast_peek();

      DUMP_START( "pc99_pointer_type_c_ast",
                  "'*' type_qualifier_list_c_stid_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $qual_stids );

      if ( type_ast == NULL ) {
        type_ast = c_ast_new_gc( K_BUILTIN, &@$ );
        type_ast->type.btids = TB_int;
        ia_type_ast_push( type_ast );
      }

      $$ = c_ast_pointer( ia_type_spec_ast( type_ast ), &gc_ast_list );
      $$->type.stids = c_tid_check( $qual_stids, C_TPID_STORE );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ pointer-to-member declaration ///////////////////////////////

pointer_to_member_decl_c_astp
  : pointer_to_member_type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    decl_c_astp[decl_astp]
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_to_member_decl_c_astp",
                  "pointer_to_member_type_c_ast decl_c_astp" );
      DUMP_AST( "pointer_to_member_type_c_ast", $type_ast );
      DUMP_AST_PAIR( "decl_c_astp", $decl_astp );

      $$ = $decl_astp;
      $$.ast->loc = @$;

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_type_c_ast
  : // in_attr: type_c_ast
    any_sname_c[sname] Y_COLON_COLON_STAR '*'
    cv_qualifier_list_stid_opt[qual_stids]
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "pointer_to_member_type_c_ast",
                  "any_sname_c '::*' cv_qualifier_list_stid_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_SNAME( "any_sname_c", $sname );
      DUMP_TID( "cv_qualifier_list_stid_opt", $qual_stids );

      $$ = c_ast_new_gc( K_POINTER_TO_MEMBER, &@$ );

      c_type_t scope_type = *c_sname_local_type( &$sname );
      if ( !c_tid_is_any( scope_type.btids, TB_ANY_SCOPE ) ) {
        //
        // The sname has no scope-type, but we now know there's a pointer-to-
        // member of it, so it must be a class.  (It could alternatively be a
        // struct, but we have no context to know, so just pick class because
        // it's more C++-like.)
        //
        scope_type.btids = TB_class;
        c_sname_local_data( &$sname )->type = scope_type;
      }

      // adopt sname's scope-type for the AST
      $$->type = c_type_or( &C_TYPE_LIT_S( $qual_stids ), &scope_type );

      $$->ptr_mbr.class_sname = c_sname_move( &$sname );
      c_ast_set_parent( c_ast_dup_gc( type_ast ), $$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ reference declaration ///////////////////////////////////////

reference_decl_c_astp
  : reference_type_c_ast[type_ast] type_qualifier_list_c_stid_opt[qual_stids]
    {
      $type_ast->type.stids = c_tid_check( $qual_stids, C_TPID_STORE );
      ia_type_ast_push( $type_ast );
    }
    decl_c_astp[decl_astp]
    {
      ia_type_ast_pop();

      DUMP_START( "reference_decl_c_astp", "reference_type_c_ast decl_c_astp" );
      DUMP_AST( "reference_type_c_ast", $type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $qual_stids );
      DUMP_AST_PAIR( "decl_c_astp", $decl_astp );

      PJL_DISCARD_RV( c_ast_patch_placeholder( $type_ast, $decl_astp.ast ) );
      $$ = $decl_astp;
      $$.ast->loc = @$;

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

reference_type_c_ast
  : // in_attr: type_c_ast
    '&'
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "reference_type_c_ast", "'&'" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );

      $$ = c_ast_new_gc( K_REFERENCE, &@$ );
      c_ast_set_parent( ia_type_spec_ast( type_ast ), $$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | // in_attr: type_c_ast
    Y_AMPER_AMPER
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "reference_type_c_ast", "'&&'" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );

      $$ = c_ast_new_gc( K_RVALUE_REFERENCE, &@$ );
      c_ast_set_parent( ia_type_spec_ast( type_ast ), $$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ typedef type declaration //////////////////////////////////

typedef_type_decl_c_ast
  : // in_attr: type_c_ast
    typedef_type_c_ast[tdef_ast] bit_field_c_uint_opt[bit_width]
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "typedef_type_decl_c_ast",
                  "typedef_type_c_ast bit_field_c_uint_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST( "typedef_type_c_ast", $tdef_ast );
      DUMP_INT( "bit_field_c_uint_opt", $bit_width );

      if ( c_tid_is_any( type_ast->type.stids, TS_typedef ) ) {
        //
        // If we're defining a type, return the type as-is.
        //
        $$ = $tdef_ast;
      }
      else {
        //
        // Otherwise, return the type that it's typedef'd as.
        //
        c_ast_t const *const raw_tdef_ast = c_ast_untypedef( $tdef_ast );

        //
        // But first ensure the name isn't of a previously declared type:
        //
        //      cdecl> struct S
        //      cdecl> explain int S    // error: "S": previously declared
        //
        // Note that typedef_type_c_ast is like:
        //
        //      typedef_type_c_ast: {
        //        sname: { string: "" },
        //        kind: { value: 0x20, string: "typedef" },
        //        ...
        //        type: { btid: 0x10000001, ..., string: "none" },
        //        tdef: {
        //          for_ast: {
        //            sname: { string: "S", scopes: "struct" },
        //            kind: { value: 0x8, string: "struct or union" },
        //            ...
        //            type: { btid: 0x800001, ..., string: "struct" },
        //            csu: {
        //              csu_sname: { string: "S", scopes: "none" }
        //            }
        //          }
        //        },
        //        ...
        //      }
        //
        // That is, typedef_type_c_ast has no name itself (at this point), but
        // the raw type, of course, does, so it's that name we have to check.
        //
        // This check has to be done now in the parser rather than later in the
        // AST since if this declaration were joined with a type (like `int`
        // above), the type information would be lost and we'd get this from
        // above:
        //
        //      declare S as structure S          // wrong
        //
        // Additionally, we also get here when the typedef name is used as part
        // of longer name, e.g., `S::x`:
        //
        //      c++decl> explain int S::x
        //      declare x of structure S as int   // correct
        //
        // but that name isn't of a previously declared type, so it's OK.
        //
        PARSE_ASSERT(
          !c_sname_is_type( &raw_tdef_ast->sname, &$tdef_ast->loc )
        );

        //
        // We have to duplicate the type to set the current location.
        //
        $$ = c_ast_dup_gc( raw_tdef_ast );
        $$->loc = $tdef_ast->loc;
      }

      PARSE_ASSERT( c_ast_set_bit_field_width( $$, $bit_width ) );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ user-defined conversion operator declaration ////////////////

user_defined_conversion_decl_c_astp
  : // in_attr: type_c_ast
    oper_sname_c_opt[sname] Y_operator type_c_ast[to_ast]
    {
      ia_type_ast_push( $to_ast );
    }
    udc_decl_c_ast_opt[decl_ast] lparen_exp
    param_list_rparen_func_qualifier_list_c_stid_opt[qual_stids]
    noexcept_c_stid_opt[noexcept_stid] func_equals_c_stid_opt[equals_stid]
    {
      ia_type_ast_pop();

      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "user_defined_conversion_decl_c_astp",
                  "oper_sname_c_opt OPERATOR "
                  "type_c_ast udc_decl_c_ast_opt '(' ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt "
                  "func_equals_c_stid_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_SNAME( "oper_sname_c_opt", $sname );
      DUMP_AST( "type_c_ast", $to_ast );
      DUMP_AST( "udc_decl_c_ast_opt", $decl_ast );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $qual_stids );
      DUMP_TID( "noexcept_c_stid_opt", $noexcept_stid );
      DUMP_TID( "func_equals_c_stid_opt", $equals_stid );

      c_ast_t *const udc_ast = c_ast_new_gc( K_UDEF_CONV, &@$ );
      udc_ast->sname = c_sname_move( &$sname );
      udc_ast->type.stids = c_tid_check(
        $qual_stids | $noexcept_stid | $equals_stid,
        C_TPID_STORE
      );
      if ( type_ast != NULL )
        c_type_or_eq( &udc_ast->type, &type_ast->type );
      udc_ast->udef_conv.to_ast = IF_ELSE_EXPR( $decl_ast, $to_ast );

      $$ = (c_ast_pair_t){ udc_ast, udc_ast->udef_conv.to_ast };

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ user-defined literal declaration ////////////////////////////

user_defined_literal_decl_c_astp
  : // in_attr: type_c_ast
    oper_sname_c_opt[sname] Y_operator empty_str_lit_exp name_exp[name]
    lparen_exp param_c_ast_list_exp[param_ast_list] ')'
    noexcept_c_stid_opt[noexcept_stid]
    trailing_return_type_c_ast_opt[trailing_ret_ast]
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "user_defined_literal_decl_c_astp",
                  "oper_sname_c_opt OPERATOR '\"\"' "
                  "'(' param_c_ast_list_exp ')' noexcept_c_stid_opt "
                  "trailing_return_type_c_ast_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_SNAME( "oper_sname_c_opt", $sname );
      DUMP_STR( "name", $name );
      DUMP_AST_LIST( "param_c_ast_list_exp", $param_ast_list );
      DUMP_TID( "noexcept_c_stid_opt", $noexcept_stid );
      DUMP_AST( "trailing_return_type_c_ast_opt", $trailing_ret_ast );

      c_sname_set( &type_ast->sname, &$sname );
      c_sname_push_back_name( &type_ast->sname, $name );

      c_ast_t *const udl_ast = c_ast_new_gc( K_UDEF_LIT, &@$ );
      udl_ast->type.stids = c_tid_check( $noexcept_stid, C_TPID_STORE );
      c_ast_list_set_param_of( &$param_ast_list, udl_ast );
      udl_ast->udef_lit.param_ast_list = slist_move( &$param_ast_list );

      $$ = (c_ast_pair_t){
        c_ast_add_func(
          type_ast,
          udl_ast,
          IF_ELSE_EXPR( $trailing_ret_ast, type_ast )
        ),
        udl_ast->udef_lit.ret_ast
      };

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  C/C++ CASTS
///////////////////////////////////////////////////////////////////////////////

cast_c_astp_opt
  : /* empty */                   { $$ = (c_ast_pair_t){ NULL, NULL }; }
  | cast_c_astp
  ;

cast_c_astp
  : cast2_c_astp
  | pointer_cast_c_astp
  | pointer_to_member_cast_c_astp
  | reference_cast_c_astp
  ;

cast2_c_astp
  : array_cast_c_astp
  | block_cast_c_astp
  | func_cast_c_astp
  | nested_cast_c_astp
//| oper_cast_c_astp                    // can't cast to an operator
  | param_pack_cast_c_ast[ast]    { $$ = (c_ast_pair_t){ $ast, NULL }; }
  | sname_c_ast[ast]              { $$ = (c_ast_pair_t){ $ast, NULL }; }
//| typedef_type_cast_c_ast             // can't cast to a typedef
//| user_defined_conversion_cast_c_astp // can't cast to a user-defined conv.
//| user_defined_literal_cast_c_astp    // can't cast to a user-defined literal
  ;

param_pack_cast_c_ast
  : // in_attr: type_c_ast
    Y_ELLIPSIS
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "param_pack_cast_c_ast", "..." );\
      DUMP_AST( "in_attr__type_c_ast", type_ast );

      c_ast_set_parameter_pack( type_ast );
      $$ = NULL;

      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_END();
    }
  | Y_ELLIPSIS sname_c_ast[ast]
    {
      DUMP_START( "param_pack_cast_c_ast", "... sname_c_ast" );\
      DUMP_AST( "sname_c_ast", $ast );

      c_ast_set_parameter_pack( $ast );
      $$ = $ast;

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ array cast ////////////////////////////////////////////////

array_cast_c_astp
  : // in_attr: type_c_ast
    cast_c_astp_opt[cast_astp] array_size_c_ast[array_ast]
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "array_cast_c_astp", "cast_c_astp_opt array_size_c_ast" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );
      DUMP_AST( "array_size_c_ast", $array_ast );

      c_ast_set_parent(
        c_ast_new_gc( K_PLACEHOLDER, &@cast_astp ), $array_ast
      );

      c_ast_t *const of_ast = ia_type_spec_ast( type_ast );
      if ( $cast_astp.target_ast != NULL ) {
        // array-of or function-like-ret type
        $$ = (c_ast_pair_t){
          $cast_astp.ast,
          c_ast_add_array( $cast_astp.target_ast, $array_ast, of_ast )
        };
      } else {
        c_ast_t *const ast = IF_ELSE_EXPR( $cast_astp.ast, of_ast );
        $$ = (c_ast_pair_t){
          c_ast_add_array( ast, $array_ast, of_ast ),
          .target_ast = NULL
        };
      }

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ block cast (Apple extension) //////////////////////////////

block_cast_c_astp                       // Apple extension
  : // in_attr: type_c_ast
    '(' '^'
    { //
      // A block AST has to be the type inherited attribute for cast_c_astp_opt
      // so we have to create it here.
      //
      ia_type_ast_push( c_ast_new_gc( K_APPLE_BLOCK, &@$ ) );
    }
    type_qualifier_list_c_stid_opt[qual_stids] cast_c_astp_opt[cast_astp]
    rparen_exp
    param_list_c_lparen param_c_ast_list_opt[param_ast_list] param_list_rparen
    {
      c_ast_t *const block_ast = ia_type_ast_pop();
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "block_cast_c_astp",
                  "'(' '^' type_qualifier_list_c_stid_opt cast_c_astp_opt ')' "
                  "'(' param_c_ast_list_opt ')'" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $qual_stids );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );
      DUMP_AST_LIST( "param_c_ast_list_opt", $param_ast_list );

      PARSE_ASSERT(
        c_type_add_tid( &block_ast->type, $qual_stids, &@qual_stids )
      );
      c_ast_list_set_param_of( &$param_ast_list, block_ast );
      block_ast->block.param_ast_list = slist_move( &$param_ast_list );

      c_ast_t *const ret_ast = ia_type_spec_ast( type_ast );
      if ( $cast_astp.target_ast != NULL ) {
        $$ = (c_ast_pair_t){
          $cast_astp.ast,
          c_ast_add_func( $cast_astp.target_ast, block_ast, ret_ast )
        };
      }
      else {
        $$ = (c_ast_pair_t){
          c_ast_add_func( $cast_astp.ast, block_ast, ret_ast ),
          block_ast->block.ret_ast
        };
      }

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ function cast /////////////////////////////////////////////

func_cast_c_astp
  : // in_attr: type_c_ast
    cast2_c_astp[cast_astp]
    param_list_c_lparen param_c_ast_list_opt[param_ast_list]
    param_list_rparen_func_qualifier_list_c_stid_opt[ref_qual_stids]
    noexcept_c_stid_opt[noexcept_stid]
    trailing_return_type_c_ast_opt[trailing_ret_ast]
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "func_cast_c_astp",
                  "cast2_c_astp '(' param_c_ast_list_opt ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt "
                  "trailing_return_type_c_ast_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST_PAIR( "cast2_c_astp", $cast_astp );
      DUMP_AST_LIST( "param_c_ast_list_opt", $param_ast_list );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $ref_qual_stids );
      DUMP_AST( "trailing_return_type_c_ast_opt", $trailing_ret_ast );
      DUMP_TID( "noexcept_c_stid_opt", $noexcept_stid );

      c_ast_t *const func_ast = c_ast_new_gc( K_FUNCTION, &@$ );
      func_ast->type.stids =
        c_tid_check( $ref_qual_stids | $noexcept_stid, C_TPID_STORE );
      c_ast_list_set_param_of( &$param_ast_list, func_ast );
      func_ast->func.param_ast_list = slist_move( &$param_ast_list );

      c_ast_t *const cast_ast = $cast_astp.ast;
      //
      // This is for a case similar to the one in func_decl_c_astp like:
      //
      //     void f( int () () )
      //              |   |  |
      //              |   |  func
      //              |   |
      //              |   cast_ast (func)
      //              |
      //              ret_ast
      //
      // We replace ret_ast with cast_ast:
      //
      //      void f( int() () )
      //              |  |  |
      //              |  |  func
      //              X  |
      //              |  cast_ast (func)
      //              |
      //              ret_ast <- cast_ast (func)
      //
      // that is, a "function returning function returning int" -- which is
      // illegal (since functions can't return functions) and will be caught by
      // c_ast_check_ret_type().
      //
      c_ast_t *const ret_ast = cast_ast->kind == K_FUNCTION ?
        cast_ast :
        IF_ELSE_EXPR( $trailing_ret_ast, ia_type_spec_ast( type_ast ) );

      if ( $cast_astp.target_ast != NULL ) {
        $$ = (c_ast_pair_t){
          cast_ast,
          c_ast_add_func( $cast_astp.target_ast, func_ast, ret_ast )
        };
      }
      else {
        $$ = (c_ast_pair_t){
          c_ast_add_func( cast_ast, func_ast, ret_ast ),
          func_ast->func.ret_ast
        };
      }

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ nested cast ///////////////////////////////////////////////

nested_cast_c_astp
  : '('
    {
      ia_type_ast_push( c_ast_new_gc( K_PLACEHOLDER, &@$ ) );
      ++in_attr.ast_depth;
    }
    cast_c_astp_opt[cast_astp] rparen_exp
    {
      ia_type_ast_pop();
      --in_attr.ast_depth;

      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "nested_cast_c_astp", "'(' cast_c_astp_opt ')'" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );

      $$ = $cast_astp;

      if ( $$.ast == NULL ) {
        //
        // This is for a case like:
        //
        //      void f( int() )
        //                 ^^
        //
        // where the unnamed parameter is a "function returning int".  (In
        // param_c_ast, this will be converted into a "pointer to function
        // returning int".)
        //
        $$.ast = c_ast_new_gc( K_FUNCTION, &@$ );
        c_ast_set_parent( type_ast, $$.ast );
      } else {
        $$.ast->loc = @$;
      }

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ pointer cast //////////////////////////////////////////////

pointer_cast_c_astp
  : pointer_type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    cast_c_astp_opt[cast_astp]
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_cast_c_astp", "pointer_type_c_ast cast_c_astp_opt" );
      DUMP_AST( "pointer_type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );

      $$ = (c_ast_pair_t){
        c_ast_patch_placeholder( $type_ast, $cast_astp.ast ),
        $cast_astp.target_ast
      };

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ pointer-to-member cast ////////////////////////////////////

pointer_to_member_cast_c_astp
  : pointer_to_member_type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    cast_c_astp_opt[cast_astp]
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_to_member_cast_c_astp",
                  "pointer_to_member_type_c_ast cast_c_astp_opt" );
      DUMP_AST( "pointer_to_member_type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );

      $$ = (c_ast_pair_t){
        c_ast_patch_placeholder( $type_ast, $cast_astp.ast ),
        $cast_astp.target_ast
      };

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ reference cast ////////////////////////////////////////////

reference_cast_c_astp
  : reference_type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    cast_c_astp_opt[cast_astp]
    {
      ia_type_ast_pop();

      DUMP_START( "reference_cast_c_astp",
                  "reference_type_c_ast cast_c_astp_opt" );
      DUMP_AST( "reference_type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );

      $$ = (c_ast_pair_t){
        c_ast_patch_placeholder( $type_ast, $cast_astp.ast ),
        $cast_astp.target_ast
      };

      DUMP_AST_PAIR( "$$_astp", $$ );
      DUMP_END();
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  C++ USER-DEFINED CONVERSIONS                                             //
//                                                                           //
//  These productions are a subset of C/C++ cast productions, specifically   //
//  without arrays, blocks, functions, or nested declarations, all of which  //
//  are either illegal or ambiguous.                                         //
///////////////////////////////////////////////////////////////////////////////

udc_decl_c_ast_opt
  : /* empty */                   { $$ = NULL; }
  | udc_decl_c_ast
  ;

udc_decl_c_ast
  : pointer_udc_decl_c_ast
  | pointer_to_member_udc_decl_c_ast
  | reference_udc_decl_c_ast
  | sname_c_ast
  ;

pointer_udc_decl_c_ast
  : pointer_type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    udc_decl_c_ast_opt[decl_ast]
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_udc_decl_c_ast",
                  "pointer_type_c_ast udc_decl_c_ast_opt" );
      DUMP_AST( "pointer_type_c_ast", $type_ast );
      DUMP_AST( "udc_decl_c_ast_opt", $decl_ast );

      $$ = c_ast_patch_placeholder( $type_ast, $decl_ast );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_udc_decl_c_ast
  : pointer_to_member_type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    udc_decl_c_ast_opt[decl_ast]
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_to_member_udc_decl_c_ast",
                  "pointer_to_member_type_c_ast udc_decl_c_ast_opt" );
      DUMP_AST( "pointer_to_member_type_c_ast", $type_ast );
      DUMP_AST( "udc_decl_c_ast_opt", $decl_ast );

      $$ = c_ast_patch_placeholder( $type_ast, $decl_ast );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

reference_udc_decl_c_ast
  : reference_type_c_ast[type_ast] { ia_type_ast_push( $type_ast ); }
    udc_decl_c_ast_opt[decl_ast]
    {
      ia_type_ast_pop();

      DUMP_START( "reference_udc_decl_c_ast",
                  "reference_type_c_ast udc_decl_c_ast_opt" );
      DUMP_AST( "reference_type_c_ast", $type_ast );
      DUMP_AST( "udc_decl_c_ast_opt", $decl_ast );

      $$ = c_ast_patch_placeholder( $type_ast, $decl_ast );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  C/C++ TYPES                                                              //
///////////////////////////////////////////////////////////////////////////////

type_c_ast
    /*
     * Type-modifier-only (implicit int) declarations:
     *
     *      unsigned i;
     */
  : type_modifier_list_c_type[mod_list_type]
    {
      DUMP_START( "type_c_ast", "type_modifier_list_c_type" );
      DUMP_TYPE( "type_modifier_list_c_type", $mod_list_type );

      //
      // Prior to C99, typeless declarations are implicitly int, so we set it
      // here.  In C99 and later, however, implicit int is an error, so we
      // don't set it here and c_ast_check() will catch the error later.
      //
      // Note that type modifiers, e.g., unsigned, count as a type since that
      // means unsigned int; however, neither qualifiers, e.g., const, nor
      // storage classes, e.g., register, by themselves count as a type:
      //
      //      unsigned i;   // legal in C99
      //      const    j;   // illegal in C99
      //      register k;   // illegal in C99
      //
      c_type_t type = OPT_LANG_IS( IMPLICIT_int ) ?
        C_TYPE_LIT_B( TB_int ) : T_NONE;

      PARSE_ASSERT( c_type_add( &type, &$mod_list_type, &@mod_list_type ) );

      if ( type.btids == TB_int ) {     // type is still the TB_int we added
        //
        // This check has to be done now in the parser rather than later in the
        // AST since the AST has no memory that the "int" is implicit.
        //
        // Alternatively, we could _not_ set TB_int here, but then we'd need a
        // new separate AST "fix-up" pass that would change TB_NONE to TB_int.
        // Objectively, that's probably the better solution, but it's a lot of
        // extra code that would currently only be needed to fix-up implicit
        // int.
        //
        print_warning( &@mod_list_type,
          "missing type specifier; \"%s\" assumed\n",
          c_tid_error( TB_int )
        );
      }

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type = type;

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

    /*
     * Type-modifier type with optional trailing type-modifier(s) declarations:
     *
     *      unsigned int const i;       // uncommon, but legal
     */
  | type_modifier_list_c_type[mod_list_type] east_modified_type_c_ast[type_ast]
    {
      DUMP_START( "type_c_ast",
                  "type_modifier_list_c_type east_modified_type_c_ast " );
      DUMP_TYPE( "type_modifier_list_c_type", $mod_list_type );
      DUMP_AST( "east_modified_type_c_ast", $type_ast );

      $$ = $type_ast;
      $$->loc = @$;
      PARSE_ASSERT( c_type_add( &$$->type, &$mod_list_type, &@mod_list_type ) );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | east_modified_type_c_ast
  ;

type_modifier_list_c_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | type_modifier_list_c_type
  ;

type_modifier_list_c_type
  : type_modifier_list_c_type[mod_list_type] type_modifier_c_type[mod_type]
    {
      DUMP_START( "type_modifier_list_c_type",
                  "type_modifier_list_c_type type_modifier_c_type" );
      DUMP_TYPE( "type_modifier_list_c_type", $mod_list_type );
      DUMP_TYPE( "type_modifier_c_type", $mod_type );

      $$ = $mod_list_type;
      PARSE_ASSERT( c_type_add( &$$, &$mod_type, &@mod_type ) );

      DUMP_TYPE( "$$_type", $$ );
      DUMP_END();
    }

  | type_modifier_c_type
  ;

type_modifier_c_type
  : type_modifier_base_type
    {
      $$ = $1;
      //
      // This is for a case like:
      //
      //      explain typedef unsigned long size_t
      //
      // that is: explain a redefinition of a typedef'd type with the same type
      // that contains only one or more type_modifier_base_type.  The problem
      // is that, without an east_modified_type_c_ast (like int), the parser
      // would ordinarily take the typedef'd type (here, the size_t) as part of
      // the type_c_ast and then be out of tokens for the decl_c_astp -- at
      // which time it'll complain.
      //
      // Since type modifiers can't apply to a typedef'd type (e.g., "long
      // size_t" is illegal), we tell the lexer not to return either
      // Y_TYPEDEF_NAME_TDEF or Y_TYPEDEF_SNAME_TDEF if we encounter at least
      // one type modifier (except "register" since it's is really a storage
      // class -- see the comment in type_modifier_base_type about "register").
      //
      if ( $$.stids != TS_register )
        lexer_find &= ~LEXER_FIND_TYPES;
    }
  | type_qualifier_c_stid         { $$ = C_TYPE_LIT_S( $1 ); }
  | storage_class_c_type
  | attribute_specifier_list_c_atid
    {
      $$ = C_TYPE_LIT_A( $1 );
    }
  ;

type_modifier_base_type
  : Y__Complex                    { $$ = C_TYPE_LIT_B( $1 ); }
  | Y__Imaginary                  { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_long                        { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_short                       { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_signed                      { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_unsigned                    { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_EMC__Sat                    { $$ = C_TYPE_LIT_B( $1 ); }
    /*
     * Register is here (rather than in storage_class_c_type) because it's the
     * only storage class that can be specified for function parameters.
     * Therefore, it's simpler to treat it as any other type modifier.
     */
  | Y_register                    { $$ = C_TYPE_LIT_S( $1 ); }
  ;

east_modified_type_c_ast
  : east_modifiable_type_c_ast[type_ast]
    type_modifier_list_c_type_opt[mod_list_type]
    {
      DUMP_START( "east_modified_type_c_ast",
                  "east_modifiable_type_c_ast type_modifier_list_c_type_opt" );
      DUMP_AST( "east_modifiable_type_c_ast", $type_ast );
      DUMP_TYPE( "type_modifier_list_c_type_opt", $mod_list_type );

      $$ = $type_ast;
      $$->loc = @$;
      PARSE_ASSERT( c_type_add( &$$->type, &$mod_list_type, &@mod_list_type ) );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | enum_class_struct_union_c_ast[ecsu_ast]
    cv_qualifier_list_stid_opt[qual_stids]
    {
      DUMP_START( "east_modified_type_c_ast",
                  "enum_class_struct_union_c_ast cv_qualifier_list_stid_opt" );
      DUMP_AST( "enum_class_struct_union_c_ast", $ecsu_ast );
      DUMP_TID( "cv_qualifier_list_stid_opt", $qual_stids );

      $$ = $ecsu_ast;
      $$->loc = @$;
      PARSE_ASSERT( c_type_add_tid( &$$->type, $qual_stids, &@qual_stids ) );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

east_modifiable_type_c_ast
  : atomic_specifier_type_c_ast
  | builtin_type_c_ast
  | concept_type_c_ast
  | structured_binding_type_c_ast
  | typedef_type_c_ast
  | typeof_type_c_ast
  ;

/// Gibberish C _Atomic types /////////////////////////////////////////////////

atomic_specifier_type_c_ast
  : Y__Atomic_SPEC[atomic] lparen_exp type_c_ast[type_ast]
    {
      ia_type_ast_push( $type_ast );
    }
    cast_c_astp_opt[cast_astp] rparen_exp
    {
      ia_type_ast_pop();

      DUMP_START( "atomic_specifier_type_c_ast",
                  "ATOMIC '(' type_c_ast cast_c_astp_opt ')'" );
      DUMP_AST( "type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );

      $$ = IF_ELSE_EXPR( $cast_astp.ast, $type_ast );
      $$->loc = @$;

      //
      // Ensure the _Atomic() specifier type doesn't have either a storage
      // class or attributes:
      //
      //      const _Atomic(int) x;     // OK
      //      _Atomic(const int) y;     // error
      //
      // This check has to be done now in the parser rather than later in the
      // AST since the type would be stored as "atomic const int" either way so
      // the AST has no "memory" of which it was.
      //
      if ( $$->type.stids != TS_NONE || $$->type.atids != TA_NONE ) {
        static c_type_t const TSA_ANY = { TB_NONE, TS_ANY, TA_ANY };
        c_type_t const error_type = c_type_and( &$$->type, &TSA_ANY );
        print_error( &@type_ast,
          "\"%s\" can not be of \"%s\"\n",
          c_tid_error( TS__Atomic ),
          c_type_gibberish( &error_type )
        );
        PARSE_ABORT();
      }

      PARSE_ASSERT( c_type_add_tid( &$$->type, TS__Atomic, &@atomic ) );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ built-in types ////////////////////////////////////////////

builtin_type_c_ast
  : builtin_no_BitInt_c_btid[btid]
    {
      DUMP_START( "builtin_type_c_ast", "builtin_no_BitInt_c_btid" );
      DUMP_TID( "builtin_no_BitInt_c_btid", $btid );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = c_tid_check( $btid, C_TPID_BASE );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  | Y__BitInt lparen_exp uint_lit_exp[width] rparen_exp
    {
      DUMP_START( "builtin_type_c_ast", "_BitInt '(' uint_lit_exp ')'" );
      DUMP_INT( "uint_lit_exp", $width );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = TB__BitInt;
      $$->builtin.BitInt.width = $width;

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

builtin_no_BitInt_c_btid
  : Y_void
  | Y_auto_TYPE
  | Y__Bool
  | Y_bool
  | Y_char
  | Y_char8_t
  | Y_char16_t
  | Y_char32_t
  | Y_decltype
    {
      UNSUPPORTED( &@Y_decltype, "decltype declarations" );
    }
  | Y_wchar_t
  | Y_int
  | Y_float
  | Y_double
  | Y_EMC__Accum
  | Y_EMC__Fract
  ;

/// Gibberish C++ concept (constrained) auto types ////////////////////////////

concept_type_c_ast
  : Y_CONCEPT_SNAME[sname] auto_TYPE_exp
    {
      DUMP_START( "concept_type_c_ast", "Y_CONCEPT_SNAME AUTO" );
      DUMP_SNAME( "Y_CONCEPT_SNAME", $sname );

      c_sname_set_all_types( &$sname );

      $$ = c_ast_new_gc( K_CONCEPT, &@sname );
      $$->concept.concept_sname = c_sname_move( &$sname );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ structured binding types ////////////////////////////////////

structured_binding_type_c_ast
  : Y_auto_STRUCTURED_BINDING cv_qualifier_list_stid_opt[cv_qual_stids]
    ref_qualifier_c_stid_opt[ref_stid]
    lbracket_exp sname_list_c[sname_list] rbracket_exp
    {
      DUMP_START( "structured_binding_type_c_ast",
                  "AUTO cv_qualifier_list_stid_opt ref_qualifier_c_stid_opt "
                  "'[' sname_list_c ']'" );
      DUMP_TID( "cv_qualifier_list_stid_opt", $cv_qual_stids );
      DUMP_TID( "ref_qualifier_c_stid_opt", $ref_stid );

      $$ = c_ast_new_gc( K_STRUCTURED_BINDING, &@$ );
      $$->type.stids = c_tid_check( $cv_qual_stids | $ref_stid, C_TPID_STORE );
      $$->struct_bind.sname_list = slist_move( &$sname_list );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ typedef types /////////////////////////////////////////////

typedef_type_c_ast
  : any_typedef_tdef[tdef] sub_scope_sname_c_opt[sname]
    {
      c_ast_t *type_ast = ia_type_ast_peek();
      c_ast_t const *type_for_ast = $tdef->ast;

      DUMP_START( "typedef_type_c_ast",
                  "any_typedef_tdef sub_scope_sname_c_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST( "any_typedef__ast", type_for_ast );
      DUMP_SNAME( "sub_scope_sname_c_opt", $sname );

      if ( c_sname_empty( &$sname ) ) {
ttntd:  $$ = c_ast_new_gc( K_TYPEDEF, &@$ );
        $$->type.btids = TB_typedef;
        $$->tdef.for_ast = type_for_ast;
      }
      else {
        c_sname_t temp_sname = c_sname_dup( &$tdef->ast->sname );
        c_sname_push_back_sname( &temp_sname, &$sname );

        if ( type_ast == NULL ) {
          //
          // This is for a case like:
          //
          //      define S as struct S
          //      explain S::T x
          //
          // that is: a typedef'd type followed by ::T where T is an unknown
          // name used as a type. Just assume the T is some type, say int, and
          // create a name for it.
          //
          type_ast = c_ast_new_gc( K_BUILTIN, &@$ );
          type_ast->type.btids = TB_int;
          c_sname_set( &type_ast->sname, &temp_sname );
          type_for_ast = type_ast;
          goto ttntd;
        }

        //
        // Otherwise, this is for cases like:
        //
        //  1. A typedef'd type used for a scope:
        //
        //          define S as struct S
        //          explain int S::x
        //
        //  2. A typedef'd type used for an intermediate scope:
        //
        //          define S as struct S
        //          define T as struct T
        //          explain int S::T::x
        //
        $$ = type_ast;
        c_sname_set( &$$->sname, &temp_sname );
      }

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

sub_scope_sname_c_opt
  : /* empty */                   { c_sname_init( &$$ ); }
  | Y_COLON_COLON any_sname_c[sname]
    {
      $$ = $sname;
    }
  ;

/// Gibberish C/C++ typeof types //////////////////////////////////////////////

typeof_type_c_ast
  : typeof[is_unqual] lparen_exp type_c_ast[type_ast]
    {
      ia_type_ast_push( $type_ast );
    }
    cast_c_astp_opt[cast_astp] rparen_exp
    {
      ia_type_ast_pop();

      DUMP_START( "typeof_type_c_ast",
                  "TYPEOF '(' type_c_ast cast_c_astp_opt ')'" );
      DUMP_BOOL( "is_unqual", $is_unqual );
      DUMP_AST( "type_c_ast", $type_ast );
      DUMP_AST_PAIR( "cast_c_astp_opt", $cast_astp );

      if ( $is_unqual ) {
        c_ast_t const *const raw_ast = c_ast_untypedef( $type_ast );
        if ( raw_ast != $type_ast &&
             c_tid_is_any( raw_ast->type.stids, TS_CVRA ) ) {
          //
          // The type is a typedef and the type that it's for is ACVR-qualified
          // so we need to dup _that_ type before un-ACVR-qualifying it because
          // we don't want to modify it.
          //
          c_loc_t const *const orig_loc = &$type_ast->loc;
          $type_ast = c_ast_dup_gc( raw_ast );
          $type_ast->loc = *orig_loc;
        }
        $type_ast->type.stids &= c_tid_compl( TS_CVRA );
      }

      $$ = c_ast_patch_placeholder( $type_ast, $cast_astp.ast );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();

      //
      // Overwrite type_spec_ast to be the typeof type since _that_ is the base
      // type for one or more declarators, e.g.:
      //
      //      typeof(int*) p, a[2], f(char)
      //
      in_attr.type_spec_ast = $$;
    }

  | typeof lparen_exp error
    {
      elaborate_error(
        "typeof(expression) declarations not supported by " CDECL
      );
    }
  ;

typeof
  : Y_typeof                      { $$ = /*is_unqual=*/false; }
  | Y_typeof_unqual               { $$ = /*is_unqual=*/true;  }
  ;

/// Gibberish C/C++ enum, class, struct, & union types ////////////////////////

enum_class_struct_union_c_ast
  : class_struct_union_c_ast
  | enum_c_ast
  ;

class_struct_union_c_ast
  : class_struct_union_btid[csu_btid]
    attribute_specifier_list_c_atid_opt[atids] any_sname_c_exp[sname]
    {
      DUMP_START( "enum_class_struct_union_c_ast",
                  "class_struct_union_btid "
                  "attribute_specifier_list_c_atid_opt any_sname_c_exp" );
      DUMP_TID( "class_struct_union_btid", $csu_btid );
      DUMP_TID( "attribute_specifier_list_c_atid_opt", $atids );
      DUMP_SNAME( "any_sname_c_exp", $sname );

      $$ = c_ast_new_gc( K_CLASS_STRUCT_UNION, &@$ );
      $$->type.btids = c_tid_check( $csu_btid, C_TPID_BASE );
      $$->type.atids = c_tid_check( $atids, C_TPID_ATTR );
      $$->csu.csu_sname = c_sname_move( &$sname );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | class_struct_union_btid[csu_btid] attribute_specifier_list_c_atid_opt
    any_sname_c_opt[sname] '{'[brace]
    {
      c_sname_cleanup( &$sname );
      UNSUPPORTED( &@brace,
        "explaining %s declarations", c_tid_gibberish( $csu_btid )
      );
    }
  ;

enum_c_ast
  : enum_btids attribute_specifier_list_c_atid_opt[atids]
    any_sname_c_exp[sname] enum_fixed_type_c_ast_opt[fixed_type_ast]
    {
      DUMP_START( "enum_c_ast",
                  "enum_btids attribute_specifier_list_c_atid_opt "
                  "any_sname_c_exp enum_fixed_type_c_ast_opt" );
      DUMP_TID( "enum_btids", $enum_btids );
      DUMP_TID( "attribute_specifier_list_c_atid_opt", $atids );
      DUMP_SNAME( "any_sname_c_exp", $sname );
      DUMP_AST( "enum_fixed_type_c_ast_opt", $fixed_type_ast );

      $$ = c_ast_new_gc( K_ENUM, &@$ );
      $$->type.btids = c_tid_check( $enum_btids, C_TPID_BASE );
      $$->type.atids = c_tid_check( $atids, C_TPID_ATTR );
      c_ast_set_parent( $fixed_type_ast, $$ );
      $$->enum_.enum_sname = c_sname_move( &$sname );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | enum_btids attribute_specifier_list_c_atid_opt any_sname_c_opt[sname]
    '{'[brace]
    {
      c_sname_cleanup( &$sname );
      UNSUPPORTED( &@brace,
        "explaining %s declarations", c_tid_gibberish( $enum_btids )
      );
    }
  ;

/// Gibberish C/C++ enum type /////////////////////////////////////////////////

enum_btids
  : Y_enum class_struct_btid_opt  { $$ = $1 | $2; }
  ;

enum_fixed_type_c_ast_opt
  : /* empty */                   { $$ = NULL; }
  | ':' enum_fixed_type_c_ast     { $$ = $2; }
  | ':' error
    {
      elaborate_error( "type name expected" );
    }
  ;

    /*
     * These rules are a subset of type_c_ast for an enum's fixed type to
     * decrease the number of shift/reduce conflicts.
     */
enum_fixed_type_c_ast
    /*
     * Type-modifier-only (implicit int):
     *
     *      enum E : unsigned
     */
  : enum_fixed_type_modifier_list_btid[btids]
    {
      DUMP_START( "enum_fixed_type_c_ast",
                  "enum_fixed_type_modifier_list_btid" );
      DUMP_TID( "enum_fixed_type_modifier_list_btid", $btids );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = c_tid_check( $btids, C_TPID_BASE );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

    /*
     * Type-modifier(s) type with optional training type-modifier(s):
     *
     *      enum E : short int unsigned // uncommon, but legal
     */
  | enum_fixed_type_modifier_list_btid[left_btids]
    enum_unmodified_fixed_type_c_ast[fixed_type_ast]
    enum_fixed_type_modifier_list_btid_opt[right_btids]
    {
      DUMP_START( "enum_fixed_type_c_ast",
                  "enum_fixed_type_modifier_list_btid "
                  "enum_unmodified_fixed_type_c_ast "
                  "enum_fixed_type_modifier_list_btid_opt" );
      DUMP_TID( "enum_fixed_type_modifier_list_btid", $left_btids );
      DUMP_AST( "enum_unmodified_fixed_type_c_ast", $fixed_type_ast );
      DUMP_TID( "enum_fixed_type_modifier_list_btid_opt", $right_btids );

      $$ = $fixed_type_ast;
      $$->loc = @$;
      PARSE_ASSERT( c_type_add_tid( &$$->type, $left_btids, &@left_btids ) );
      PARSE_ASSERT( c_type_add_tid( &$$->type, $right_btids, &@right_btids ) );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

    /*
     * Type with trailing type-modifier(s):
     *
     *      enum E : int unsigned       // uncommon, but legal
     */
  | enum_unmodified_fixed_type_c_ast[fixed_type_ast]
    enum_fixed_type_modifier_list_btid_opt[btids]
    {
      DUMP_START( "enum_fixed_type_c_ast",
                  "enum_unmodified_fixed_type_c_ast "
                  "enum_fixed_type_modifier_list_btid_opt" );
      DUMP_AST( "enum_unmodified_fixed_type_c_ast", $fixed_type_ast );
      DUMP_TID( "enum_fixed_type_modifier_list_btid_opt", $btids );

      $$ = $fixed_type_ast;
      $$->loc = @$;
      PARSE_ASSERT( c_type_add_tid( &$$->type, $btids, &@btids ) );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

enum_fixed_type_modifier_list_btid_opt
  : /* empty */                   { $$ = TB_NONE; }
  | enum_fixed_type_modifier_list_btid
  ;

enum_fixed_type_modifier_list_btid
  : enum_fixed_type_modifier_list_btid[btids]
    enum_fixed_type_modifier_btid[btid]
    {
      DUMP_START( "enum_fixed_type_modifier_list_btid",
                  "enum_fixed_type_modifier_list_btid "
                  "enum_fixed_type_modifier_btid" );
      DUMP_TID( "enum_fixed_type_modifier_list_btid", $btids );
      DUMP_TID( "enum_fixed_type_modifier_btid", $btid );

      $$ = $btids;
      PARSE_ASSERT( c_tid_add( &$$, $btid, &@btid ) );

      DUMP_TID( "$$_btids", $$ );
      DUMP_END();
    }

  | enum_fixed_type_modifier_btid
  ;

enum_fixed_type_modifier_btid
  : Y_long
  | Y_short
  | Y_signed
  | Y_unsigned
  ;

enum_unmodified_fixed_type_c_ast
  : builtin_type_c_ast
  | typedef_type_c_ast
  | typeof_type_c_ast
  ;

/// Gibberish C/C++ class, struct, & union types //////////////////////////////

class_struct_btid_opt
  : /* empty */                   { $$ = TB_NONE; }
  | class_struct_btid
  ;

class_struct_btid
  : Y_class
  | Y_struct
  ;

class_struct_union_btid
  : class_struct_btid
  | Y_union
  ;

/// Gibberish C/C++ type qualifiers ///////////////////////////////////////////

type_qualifier_list_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | type_qualifier_list_c_stid
  ;

type_qualifier_list_c_stid
  : type_qualifier_list_c_stid[stids] type_qualifier_c_stid[stid]
    gnu_or_msc_attribute_specifier_list_c_opt
    {
      DUMP_START( "type_qualifier_list_c_stid",
                  "type_qualifier_list_c_stid type_qualifier_c_stid" );
      DUMP_TID( "type_qualifier_list_c_stid", $stids );
      DUMP_TID( "type_qualifier_c_stid", $stid );

      $$ = $stids;
      PARSE_ASSERT( c_tid_add( &$$, $stid, &@stid ) );

      DUMP_TID( "$$_stids", $$ );
      DUMP_END();
    }

  | gnu_or_msc_attribute_specifier_list_c type_qualifier_c_stid[stid]
    {
      $$ = $stid;
    }

  | type_qualifier_c_stid[stid] gnu_or_msc_attribute_specifier_list_c_opt
    {
      $$ = $stid;
    }
  ;

type_qualifier_c_stid
  : Y__Atomic_QUAL
  | cv_qualifier_stid
  | restrict_qualifier_c_stid
  | Y_UPC_relaxed
  | Y_UPC_shared                  %prec Y_PREC_LESS_THAN_upc_layout_qualifier
  | Y_UPC_shared upc_layout_qualifier_c
  | Y_UPC_strict
  ;

cv_qualifier_stid
  : Y_const
  | Y_volatile
  ;

cv_qualifier_list_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | cv_qualifier_list_stid_opt[stids] cv_qualifier_stid[stid]
    {
      DUMP_START( "cv_qualifier_list_stid_opt",
                  "cv_qualifier_list_stid_opt cv_qualifier_stid" );
      DUMP_TID( "cv_qualifier_list_stid_opt", $stids );
      DUMP_TID( "cv_qualifier_stid", $stid );

      $$ = $stids;
      PARSE_ASSERT( c_tid_add( &$$, $stid, &@stid ) );

      DUMP_TID( "$$_stids", $$ );
      DUMP_END();
    }
  ;

restrict_qualifier_c_stid
  : Y_restrict                          // C only
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since both "restrict" and "__restrict" map to TS_restrict and the
      // AST has no "memory" of which it was.
      //
      if ( OPT_LANG_IS( CPP_ANY ) ) {
        print_error( &@Y_restrict,
          "\"restrict\" not supported in C++; use \"%s\" instead\n",
          L_GNU___restrict
        );
        PARSE_ABORT();
      }
    }
  | Y_GNU___restrict                    // GNU C/C++ extension
  ;

upc_layout_qualifier_c
  : '[' ']'
  | '[' uint_lit rbracket_exp
  | '[' '*' rbracket_exp
  | '[' error ']'
    {
      elaborate_error( "one of nothing, integer, or '*' expected" );
    }
  ;

/// Gibberish C/C++ storage classes ///////////////////////////////////////////

storage_class_c_type
  : Y_auto_STORAGE                { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_Apple___block               { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_consteval                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_constexpr                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_constinit                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_explicit                    { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_export                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_extern                      { $$ = C_TYPE_LIT_S( $1 ); }
  | extern_linkage_c_stid         { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_final                       { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_friend                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_inline                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_mutable                     { $$ = C_TYPE_LIT_S( $1 ); }
  | _Noreturn_atid
    {
      //
      // These checks have to be done now in the parser rather than later in
      // the AST because the _Noreturn keyword is mapped to the [[noreturn]]
      // attribute and the AST has no "memory" that it was _Noreturn.
      //
      if ( !OPT_LANG_IS( _Noreturn ) ) {
        print_error( &@_Noreturn_atid,
          "\"%s\" keyword not supported%s",
          yytext, C_LANG_WHICH( _Noreturn )
        );
        print_hint(
          "%snoreturn%s", other_token_c( "[[" ), other_token_c( "]]" )
        );
        PARSE_ABORT();
      }
      if ( !OPT_LANG_IS( _Noreturn_NOT_DEPRECATED ) ) {
        print_warning( &@_Noreturn_atid,
          "\"%s\" is deprecated%s",
          yytext, C_LANG_WHICH( _Noreturn_NOT_DEPRECATED )
        );
        print_hint(
          "%snoreturn%s", other_token_c( "[[" ), other_token_c( "]]" )
        );
      }

      $$ = C_TYPE_LIT_A( $_Noreturn_atid );
    }
  | Y_override                    { $$ = C_TYPE_LIT_S( $1 ); }
//| Y_register                          // in type_modifier_base_type
  | Y_static                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_typedef                     { $$ = C_TYPE_LIT_S( $1 ); }
  | Y__Thread_local               { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_thread_local                { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_virtual                     { $$ = C_TYPE_LIT_S( $1 ); }
  ;

_Noreturn_atid
  : Y__Noreturn
  | Y_noreturn
  ;

/// Gibberish C/C++ attributes ////////////////////////////////////////////////

attribute_specifier_list_c_atid_opt
  : /* empty */                   { $$ = TA_NONE; }
  | attribute_specifier_list_c_atid
  ;

attribute_specifier_list_c_atid
  : Y_ATTR_BEGIN '['
    {
      if ( !OPT_LANG_IS( ATTRIBUTES ) ) {
        print_error( &@Y_ATTR_BEGIN,
          "\"%s\" attribute syntax not supported%s\n",
          other_token_c( "[[" ),
          C_LANG_WHICH( ATTRIBUTES )
        );
        PARSE_ABORT();
      }
      lexer_keyword_ctx = C_KW_CTX_ATTRIBUTE;
    }
    using_opt attribute_list_c_atid_opt[atids] ']' rbracket_exp
    {
      lexer_keyword_ctx = C_KW_CTX_DEFAULT;

      DUMP_START( "attribute_specifier_list_c_atid",
                  "'[[' using_opt attribute_list_c_atid_opt ']]'" );
      DUMP_TID( "attribute_list_c_atid_opt", $atids );

      $$ = $atids;

      DUMP_END();
    }

  | gnu_or_msc_attribute_specifier_list_c
    {
      $$ = TA_NONE;
    }
  ;

using_opt
  : /* empty */
  | Y_using name_exp[name] colon_exp
    {
      IGNORING( &@Y_using, "\"using\" in attributes" );
      free( $name );
    }
  ;

attribute_list_c_atid_opt
  : /* empty */                   { $$ = TA_NONE; }
  | attribute_list_c_atid
  ;

attribute_list_c_atid
  : attribute_list_c_atid[atids] comma_exp attribute_c_atid_exp[atid]
    {
      DUMP_START( "attribute_list_c_atid",
                  "attribute_list_c_atid ',' attribute_c_atid" );
      DUMP_TID( "attribute_list_c_atid", $atids );
      DUMP_TID( "attribute_c_atid_exp", $atid );

      $$ = $atids;
      PARSE_ASSERT( c_tid_add( &$$, $atid, &@atid ) );

      DUMP_TID( "$$_atids", $$ );
      DUMP_END();
    }

  | attribute_c_atid_exp
  ;

attribute_c_atid_exp
  : Y_carries_dependency
  | Y_deprecated attribute_str_arg_c_opt
  | Y_indeterminate
  | Y_maybe_unused
  | Y_nodiscard attribute_str_arg_c_opt
  | Y_noreturn
  | Y_no_unique_address
  | Y_reproducible
  | Y_unsequenced
  | sname_c[sname]
    {
      if ( c_sname_count( &$sname ) > 1 ) {
        IGNORING( &@sname,
          "\"%s\": namespaced attributes", c_sname_gibberish( &$sname )
        );
      }
      else {
        char const *adj = "unknown";
        c_lang_id_t lang_ids = LANG_NONE;

        char const *const name = c_sname_local_name( &$sname );
        c_keyword_t const *const ck = c_keyword_find(
          name, c_lang_newer( opt_lang_id ), C_KW_CTX_ATTRIBUTE
        );
        if ( ck != NULL && c_tid_tpid( ck->tid ) == C_TPID_ATTR ) {
          adj = "unsupported";
          lang_ids = ck->lang_ids;
        }
        print_warning( &@sname,
          "\"%s\": %s attribute%s",
          name, adj, c_lang_which( lang_ids )
        );

        print_suggestions( DYM_C_ATTRIBUTES, name );
        EPUTC( '\n' );
      }

      $$ = TA_NONE;
      c_sname_cleanup( &$sname );
    }
  | error
    {
      elaborate_error_dym( DYM_C_ATTRIBUTES, "attribute name expected" );
    }
  ;

attribute_str_arg_c_opt
  : /* empty */
  | '('[paren] str_lit_exp[str] rparen_exp
    {
      IGNORING( &@paren, "attribute arguments" );
      free( $str );
    }
  ;

gnu_or_msc_attribute_specifier_list_c_opt
  : /* empty */
  | gnu_or_msc_attribute_specifier_list_c
  ;

gnu_or_msc_attribute_specifier_list_c
  : gnu_attribute_specifier_list_c
  | msc_attribute_specifier_list_c
  ;

gnu_attribute_specifier_list_c_opt
  : /* empty */
  | gnu_attribute_specifier_list_c
  ;

gnu_attribute_specifier_list_c
  : gnu_attribute_specifier_list_c gnu_attribute_specifier_c
  | gnu_attribute_specifier_c
  ;

gnu_attribute_specifier_c
  : Y_GNU___attribute__
    {
      attr_syntax_not_supported( L_GNU___attribute__, &@1 );
      //
      // Temporariy disabling finding keywords allows GNU attributes that are C
      // keywords (e.g., const) to be found as ordinary string literals.
      //
      lexer_find &= ~LEXER_FIND_C_KEYWORDS;
    }
    lparen_exp lparen_exp gnu_attribute_list_c_opt ')' rparen_exp
    {
      lexer_find |= LEXER_FIND_C_KEYWORDS;
    }
  ;

gnu_attribute_list_c_opt
  : /* empty */
  | gnu_attribuet_list_c
  ;

gnu_attribuet_list_c
  : gnu_attribuet_list_c comma_exp gnu_attribute_c_exp
  | gnu_attribute_c_exp
  ;

gnu_attribute_c_exp
  : Y_NAME[name] gnu_attribute_decl_arg_list_c_opt
    {
      free( $name );
    }
  | error
    {
      elaborate_error( "attribute name expected" );
    }
  ;

gnu_attribute_decl_arg_list_c_opt
  : /* empty */
  | '(' gnu_attribute_arg_list_c_opt ')'
  ;

gnu_attribute_arg_list_c_opt
  : /* empty */
  | gnu_attribute_arg_list_c
  ;

gnu_attribute_arg_list_c
  : gnu_attribute_arg_list_c comma_exp gnu_attribute_arg_c
  | gnu_attribute_arg_c
  ;

gnu_attribute_arg_c
  : Y_NAME                        { free( $1 ); }
  | Y_CHAR_LIT                    { free( $1 ); }
  | Y_INT_LIT
  | Y_STR_LIT                     { free( $1 ); }
  | '(' gnu_attribute_arg_list_c rparen_exp
  | Y_LEXER_ERROR
    {
      PARSE_ABORT();
    }
  ;

msc_attribute_specifier_list_c
  : msc_attribute_specifier_list_c msc_attribute_specifier_c
  | msc_attribute_specifier_c
  ;

msc_attribute_specifier_c
  : Y_MSC___declspec
    {
      attr_syntax_not_supported( L_MSC___declspec, &@1 );
      // See comment in gnu_attribute_specifier_c.
      lexer_find &= ~LEXER_FIND_C_KEYWORDS;
    }
    lparen_exp msc_attribute_list_c_opt ')'
    {
      lexer_find |= LEXER_FIND_C_KEYWORDS;
    }
  ;

msc_attribute_list_c_opt
  : /* empty */
  | msc_attribuet_list_c
  ;

msc_attribuet_list_c
    /*
     * Microsoft's syntax for individual attributes is the same as GNU's, so we
     * can just use gnu_attribute_c_exp here.
     *
     * Note that Microsoft attributes are separated by whitespace, not commas
     * as in GNU's, so that's why msc_attribuet_list_c is needed.
     */
  : msc_attribuet_list_c gnu_attribute_c_exp
  | gnu_attribute_c_exp
  ;

///////////////////////////////////////////////////////////////////////////////
//  ENGLISH DECLARATIONS                                                     //
///////////////////////////////////////////////////////////////////////////////

decl_english_ast
  : qualified_decl_english_ast
  | user_defined_literal_decl_english_ast
  | var_decl_english_ast
  ;

/// English C/C++ array declaration ///////////////////////////////////////////

array_decl_english_ast
  : Y_array array_size_decl_ast[array_ast] of_exp decl_english_ast[decl_ast]
    {
      DUMP_START( "array_decl_english_ast",
                  "ARRAY array_size_num_opt OF decl_english_ast" );
      DUMP_AST( "array_size_decl_ast", $array_ast );
      DUMP_AST( "decl_english_ast", $decl_ast );

      $$ = $array_ast;
      $$->loc = @Y_array;
      c_ast_set_parent( $decl_ast, $$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | Y_variable length_opt array_exp name_opt[name] of_exp
    decl_english_ast[decl_ast]
    {
      DUMP_START( "array_decl_english_ast",
                  "VARIABLE LENGTH ARRAY name_opt OF decl_english_ast" );
      DUMP_STR( "name_opt", $name );
      DUMP_AST( "decl_english_ast", $decl_ast );

      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      if ( $name == NULL ) {
        $$->array.kind = C_ARRAY_SIZE_VLA;
      } else {
        $$->array.kind = C_ARRAY_SIZE_NAME;
        $$->array.size_name = $name;
      }
      c_ast_set_parent( $decl_ast, $$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

array_size_decl_ast
  : /* empty */
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_SIZE_NONE;
    }
  | uint_lit[size]
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_SIZE_INT;
      $$->array.size_int = $size;
    }
  | Y_NAME[name]
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_SIZE_NAME;
      $$->array.size_name = $name;
    }
  | '*'
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_SIZE_VLA;
    }
  ;

length_opt
  : /* empty */
  | Y_length
  ;

/// English block declaration (Apple extension) ///////////////////////////////

block_decl_english_ast                  // Apple extension
  : // in_attr: qualifier
    Y_Apple_block paren_param_decl_list_english_opt[param_ast_list]
    returning_english_ast_opt[ret_ast]
    {
      DUMP_START( "block_decl_english_ast",
                  "BLOCK paren_param_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_AST_LIST( "paren_param_decl_list_english_opt", $param_ast_list );
      DUMP_AST( "returning_english_ast_opt", $ret_ast );

      $$ = c_ast_new_gc( K_APPLE_BLOCK, &@$ );
      c_ast_list_set_param_of( &$param_ast_list, $$ );
      $$->block.param_ast_list = slist_move( &$param_ast_list );
      c_ast_set_parent( $ret_ast, $$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// English C++ constructor declaration ///////////////////////////////////////

constructor_decl_english_ast
  : Y_constructor paren_param_decl_list_english_opt[param_ast_list]
    {
      DUMP_START( "constructor_decl_english_ast",
                  "CONSTRUCTOR paren_param_decl_list_english_opt" );
      DUMP_AST_LIST( "paren_param_decl_list_english_opt", $param_ast_list );

      $$ = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      c_ast_list_set_param_of( &$param_ast_list, $$ );
      $$->ctor.param_ast_list = slist_move( &$param_ast_list );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// English C++ destructor declaration ////////////////////////////////////////

destructor_decl_english_ast
  : Y_destructor destructor_parens_opt
    {
      DUMP_START( "destructor_decl_english_ast", "DESTRUCTOR ['(' ')']" );

      $$ = c_ast_new_gc( K_DESTRUCTOR, &@$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

destructor_parens_opt
  : /* empty */
  | '(' no_destructor_params ')'
  ;

/// English C/C++ function declaration ////////////////////////////////////////

func_decl_english_ast
  : // in_attr: qualifier
    func_qualifier_english_type_opt[qual_type] member_or_non_member_opt[member]
    Y_function paren_param_decl_list_english_opt[param_ast_list]
    returning_english_ast_opt[ret_ast]
    {
      DUMP_START( "func_decl_english_ast",
                  "ref_qualifier_english_stid_opt "
                  "member_or_non_member_opt "
                  "FUNCTION paren_param_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_TYPE( "func_qualifier_english_type_opt", $qual_type );
      DUMP_INT( "member_or_non_member_opt", $member );
      DUMP_AST_LIST( "paren_param_decl_list_english_opt", $param_ast_list );
      DUMP_AST( "returning_english_ast_opt", $ret_ast );

      $$ = c_ast_new_gc( K_FUNCTION, &@$ );
      $$->type = $qual_type;
      c_ast_list_set_param_of( &$param_ast_list, $$ );
      $$->func.param_ast_list = slist_move( &$param_ast_list );
      $$->func.member = $member;
      c_ast_set_parent( $ret_ast, $$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

func_qualifier_english_type_opt
  : ref_qualifier_english_stid_opt
    {
      $$ = C_TYPE_LIT_S( $1 );
    }
  | msc_calling_convention_atid
    {
      $$ = C_TYPE_LIT_A( $1 );
    }
  ;

msc_calling_convention_atid
  : Y_MSC___cdecl
  | Y_MSC___clrcall
  | Y_MSC___fastcall
  | Y_MSC___stdcall
  | Y_MSC___thiscall
  | Y_MSC___vectorcall
  ;

/// English C/C++ parameter list declaration //////////////////////////////////

paren_param_decl_list_english_opt
  : /* empty */                   { slist_init( &$$ ); }
  | paren_param_decl_list_english
  ;

paren_param_decl_list_english
  : '(' param_decl_list_english_opt[param_ast_list] ')'
    {
      DUMP_START( "paren_param_decl_list_english",
                  "'(' param_decl_list_english_opt ')'" );
      DUMP_AST_LIST( "param_decl_list_english_opt", $param_ast_list );

      $$ = $param_ast_list;

      DUMP_AST_LIST( "$$_ast_list", $$ );
      DUMP_END();
    }
  ;

param_decl_list_english_opt
  : /* empty */                   { slist_init( &$$ ); }
  | param_decl_list_english
  ;

param_decl_list_english
  : param_decl_list_english[param_ast_list] comma_exp
    decl_english_ast_exp[decl_ast]
    {
      DUMP_START( "param_decl_list_english",
                  "param_decl_list_english ',' decl_english_ast" );
      DUMP_AST_LIST( "param_decl_list_english", $param_ast_list );
      DUMP_AST( "decl_english_ast", $decl_ast );

      $$ = $param_ast_list;
      slist_push_back( &$$, $decl_ast );

      DUMP_AST_LIST( "$$_ast_list", $$ );
      DUMP_END();
    }

  | decl_english_ast[decl_ast]
    {
      DUMP_START( "param_decl_list_english", "decl_english_ast" );
      DUMP_AST( "decl_english_ast", $decl_ast );

      if ( $decl_ast->kind == K_FUNCTION ) // see the comment in param_c_ast
        $decl_ast = c_ast_pointer( $decl_ast, &gc_ast_list );

      slist_init( &$$ );
      slist_push_back( &$$, $decl_ast );

      DUMP_AST_LIST( "$$_ast_list", $$ );
      DUMP_END();
    }
  ;

decl_english_ast_exp
  : decl_english_ast
  | error
    {
      elaborate_error( "declaration expected" );
    }
  ;

/// English C++ reference qualifier declaration ///////////////////////////////

ref_qualifier_english_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_reference                   { $$ = TS_REFERENCE; }
  | Y_rvalue reference_exp        { $$ = TS_RVALUE_REFERENCE; }
  ;

/// English C/C++ function(like) returning declaration ////////////////////////

returning_english_ast_opt
  : /* empty */
    {
      DUMP_START( "returning_english_ast_opt", "<empty>" );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      // see the comment in "type_c_ast"
      $$->type.btids = OPT_LANG_IS( IMPLICIT_int ) ? TB_int : TB_void;

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | returning_english_ast
  ;

returning_english_ast
  : returning decl_english_ast[decl_ast]
    {
      DUMP_START( "returning_english_ast", "RETURNING decl_english_ast" );
      DUMP_AST( "decl_english_ast", $decl_ast );

      $$ = $decl_ast;

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | returning error
    {
      elaborate_error( "English expected after \"returning\"" );
    }
  ;

/// English C/C++ qualified declaration ///////////////////////////////////////

qualified_decl_english_ast
  : type_qualifier_list_english_type_opt[type]
    qualifiable_decl_english_ast[decl_ast]
    {
      DUMP_START( "qualified_decl_english_ast",
                  "type_qualifier_list_english_type_opt "
                  "qualifiable_decl_english_ast" );
      DUMP_TYPE( "type_qualifier_list_english_type_opt", $type );
      DUMP_AST( "qualifiable_decl_english_ast", $decl_ast );

      $$ = $decl_ast;
      if ( !c_type_is_none( &$type ) )
        $$->loc = @$;
      PARSE_ASSERT( c_type_add( &$$->type, &$type, &@type ) );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

type_qualifier_list_english_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | type_qualifier_list_english_type
  ;

type_qualifier_list_english_type
  : type_qualifier_list_english_type[left_type]
    type_qualifier_english_type[right_type]
    {
      DUMP_START( "type_qualifier_list_english_type",
                  "type_qualifier_list_english_type "
                  "type_qualifier_english_type" );
      DUMP_TYPE( "type_qualifier_list_english_type", $left_type );
      DUMP_TYPE( "type_qualifier_english_type", $right_type );

      $$ = $left_type;
      PARSE_ASSERT( c_type_add( &$$, &$right_type, &@right_type ) );

      DUMP_TYPE( "$$_type", $$ );
      DUMP_END();
    }

  | type_qualifier_english_type
  ;

type_qualifier_english_type
  : attribute_english_atid        { $$ = C_TYPE_LIT_A( $1 ); }
  | storage_class_english_stid    { $$ = C_TYPE_LIT_S( $1 ); }
  | type_qualifier_english_stid   { $$ = C_TYPE_LIT_S( $1 ); }
  ;

attribute_english_atid
  : Y_carries dependency_exp      { $$ = TA_carries_dependency; }
  | Y_carries_dependency
  | Y_deprecated
  | Y_indeterminate
  | Y_maybe unused_exp            { $$ = TA_maybe_unused; }
  | Y_maybe_unused
  | Y_no Y_discard                { $$ = TA_nodiscard; }
  | Y_nodiscard
  | Y_no Y_return                 { $$ = TA_noreturn; }
  | Y__Noreturn
  | Y_noreturn
  | Y_no Y_unique address_exp     { $$ = TA_no_unique_address; }
  | Y_no_unique_address
  | Y_reproducible
  | Y_unsequenced
  ;

storage_class_english_stid
  : Y_auto_STORAGE
  | Y_Apple___block
  | Y_constant eval_expr_init_stid[stid]
    {
      $$ = $stid;
    }
  | Y_consteval
  | Y_constexpr
  | Y_constinit
  | Y_default
  | Y_delete
  | Y_explicit
  | Y_export
  | Y_extern
  | Y_extern linkage_stid[stid] linkage_opt
    {
      $$ = $stid;
    }
  | Y_final
  | Y_friend
  | Y_inline
  | Y_mutable
  | Y_no Y_except                 { $$ = TS_noexcept; }
  | Y_noexcept
  | Y_non_empty                   { $$ = TS_NON_EMPTY_ARRAY; }
  | Y_override
//| Y_register                          // in type_modifier_list_english_type
  | Y_static
  | Y_this
  | Y_thread local_exp            { $$ = TS_thread_local; }
  | Y__Thread_local
  | Y_thread_local
  | Y_throw
  | Y_typedef
  | Y_virtual
  | Y_pure virtual_stid_exp       { $$ = TS_PURE_virtual | $2; }
  ;

eval_expr_init_stid
  : Y_evaluation                  { $$ = TS_consteval; }
  | Y_expression                  { $$ = TS_constexpr; }
  | Y_initialization              { $$ = TS_constinit; }
  //
  // Normally, this rule would be named eval_expr_init_stid_exp and there would
  // be "| error" as the last alternate.  However, this rule is only ever
  // preceded by the Y_constant token in the grammar and that token is a
  // special case that's returned only when followed by one of these three
  // tokens so we can't possibly get something else here.
  //
  ;

linkage_stid
  : str_lit[str]
    {
      bool ok = true;

      if ( strcmp( $str, "C" ) == 0 )
        $$ = TS_extern_C;
      else if ( strcmp( $str, "C++" ) == 0 )
        $$ = TS_NONE;
      else {
        print_error( &@str, "\"%s\": unknown linkage language", $str );
        print_hint( "\"C\" or \"C++\"" );
        ok = false;
      }

      free( $str );
      PARSE_ASSERT( ok );
    }
  ;

linkage_opt
  : /* empty */
  | Y_linkage
  ;

type_qualifier_english_stid
  : Y__Atomic_QUAL
  | cv_qualifier_stid
  | restrict_qualifier_c_stid
  | Y_UPC_relaxed
  | Y_UPC_shared                  %prec Y_PREC_LESS_THAN_upc_layout_qualifier
  | Y_UPC_shared upc_layout_qualifier_english
  | Y_UPC_strict
  ;

upc_layout_qualifier_english
  : uint_lit
  | '*'
  ;

qualifiable_decl_english_ast
  : array_decl_english_ast
  | block_decl_english_ast
  | constructor_decl_english_ast
  | destructor_decl_english_ast
  | func_decl_english_ast
  | pointer_decl_english_ast
  | reference_decl_english_ast
  | structured_binding_decl_english_ast
  | type_english_ast
  ;

/// English C/C++ pointer declaration /////////////////////////////////////////

pointer_decl_english_ast
    /*
     * Ordinary pointer declaration.
     */
  : // in_attr: qualifier
    Y_pointer to_exp decl_english_ast[decl_ast]
    {
      DUMP_START( "pointer_decl_english_ast", "POINTER TO decl_english_ast" );
      DUMP_AST( "decl_english_ast", $decl_ast );

      // see the comment in "declare_command"
      PARSE_ASSERT( !c_ast_is_name_error( $decl_ast ) );

      $$ = c_ast_new_gc( K_POINTER, &@$ );
      c_ast_set_parent( $decl_ast, $$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

    /*
     * C++ pointer-to-member declaration.
     */
  | // in_attr: qualifier
    Y_pointer to_exp Y_member of_exp class_struct_union_btid_exp[csu_btid]
    sname_english_exp[sname] decl_english_ast[decl_ast]
    {
      DUMP_START( "pointer_to_member_decl_english",
                  "POINTER TO MEMBER OF "
                  "class_struct_union_btid_exp "
                  "sname_english decl_english_ast" );
      DUMP_TID( "class_struct_union_btid_exp", $csu_btid );
      DUMP_SNAME( "sname_english_exp", $sname );
      DUMP_AST( "decl_english_ast", $decl_ast );

      // see the comment in "declare_command"
      if ( c_ast_is_name_error( $decl_ast ) ) {
        c_sname_cleanup( &$sname );
        PARSE_ABORT();
      }

      $$ = c_ast_new_gc( K_POINTER_TO_MEMBER, &@$ );
      $$->ptr_mbr.class_sname = c_sname_move( &$sname );
      c_ast_set_parent( $decl_ast, $$ );
      PARSE_ASSERT( c_type_add_tid( &$$->type, $csu_btid, &@csu_btid ) );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | Y_pointer to_exp error
    {
      if ( OPT_LANG_IS( POINTERS_TO_MEMBER ) )
        elaborate_error( "type name or \"member\" expected" );
      else
        elaborate_error( "type name expected" );
    }
  ;

/// English C++ reference declaration /////////////////////////////////////////

reference_decl_english_ast
  : // in_attr: qualifier
    reference_english_ast[ref_ast] to_exp decl_english_ast[decl_ast]
    {
      DUMP_START( "reference_decl_english_ast",
                  "reference_english_ast TO decl_english_ast" );
      DUMP_AST( "reference_english_ast", $ref_ast );
      DUMP_AST( "decl_english_ast", $decl_ast );

      // see the comment in "declare_command"
      PARSE_ASSERT( !c_ast_is_name_error( $decl_ast ) );

      $$ = $ref_ast;
      $$->loc = @$;
      c_ast_set_parent( $decl_ast, $$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

reference_english_ast
  : Y_reference
    {
      $$ = c_ast_new_gc( K_REFERENCE, &@$ );
    }

  | Y_rvalue reference_exp
    {
      $$ = c_ast_new_gc( K_RVALUE_REFERENCE, &@$ );
    }
  ;

/// English C++ structured binding declaration ////////////////////////////////

structured_binding_decl_english_ast
  : Y_structured binding_exp
    {
      $$ = c_ast_new_gc( K_STRUCTURED_BINDING, &@$ );
    }
  ;

/// English C++ user-defined literal declaration //////////////////////////////

user_defined_literal_decl_english_ast
  : user_defined literal_exp lparen_exp
    param_decl_list_english_opt[param_ast_list] ')'
    returning_english_ast_opt[ret_ast]
    { //
      // User-defined literals are supported only in C++11 and later.
      // (However, we always allow them in configuration files.)
      //
      // This check is better to do now in the parser rather than later in the
      // AST because it has to be done in fewer places in the code plus gives a
      // better error location.
      //
      if ( !OPT_LANG_IS( USER_DEF_LITS ) ) {
        print_error( &@user_defined,
          "user-defined literals not supported%s\n",
          C_LANG_WHICH( USER_DEF_LITS )
        );
        PARSE_ABORT();
      }

      DUMP_START( "user_defined_literal_decl_english_ast",
                  "USER-DEFINED LITERAL '(' param_decl_list_english_opt ')' "
                  "returning_english_ast_opt" );
      DUMP_AST_LIST( "param_decl_list_english_opt", $param_ast_list );
      DUMP_AST( "returning_english_ast_opt", $ret_ast );

      $$ = c_ast_new_gc( K_UDEF_LIT, &@$ );
      c_ast_list_set_param_of( &$param_ast_list, $$ );
      $$->udef_lit.param_ast_list = slist_move( &$param_ast_list );
      c_ast_set_parent( $ret_ast, $$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

/// English C/C++ variable declaration ////////////////////////////////////////

var_decl_english_ast
    /*
     * Ordinary variable declaration.
     */
  : sname_c[sname] Y_as decl_english_ast[decl_ast]
    {
      DUMP_START( "var_decl_english_ast", "sname_c AS decl_english_ast" );
      DUMP_SNAME( "sname_c", $sname );
      DUMP_AST( "decl_english_ast", $decl_ast );

      // see the comment in "declare_command"
      if ( c_ast_is_name_error( $decl_ast ) ) {
        c_sname_cleanup( &$sname );
        PARSE_ABORT();
      }

      $$ = $decl_ast;
      $$->loc = @$;
      c_sname_set( &$$->sname, &$sname );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

    /*
     * K&R C type-less parameter declaration.
     */
  | sname_english_ast

    /*
     * Varargs declaration.
     */
  | Y_ELLIPSIS
    {
      DUMP_START( "var_decl_english_ast", "..." );

      $$ = c_ast_new_gc( K_VARIADIC, &@$ );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  ENGLISH TYPES                                                            //
///////////////////////////////////////////////////////////////////////////////

type_english_ast
  : // in_attr: qualifier
    type_modifier_list_english_type_opt[type] unmodified_type_english_ast[ast]
    {
      DUMP_START( "type_english_ast",
                  "type_modifier_list_english_type_opt "
                  "unmodified_type_english_ast" );
      DUMP_TYPE( "type_modifier_list_english_type_opt", $type );
      DUMP_AST( "unmodified_type_english_ast", $ast );

      $$ = $ast;
      if ( !c_type_is_none( &$type ) ) {
        // Set the AST's location to the entire rule only if the leading
        // optional rule is actually present, otherwise @$ refers to a column
        // before $ast.
        $$->loc = @$;
        PARSE_ASSERT( c_type_add( &$$->type, &$type, &@type ) );
      }

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | // in_attr: qualifier
    type_modifier_list_english_type[type] // allows implicit int in pre-C99
    {
      DUMP_START( "type_english_ast", "type_modifier_list_english_type" );
      DUMP_TYPE( "type_modifier_list_english_type", $type );

      // see the comment in "type_c_ast"
      c_type_t new_type =
        C_TYPE_LIT_B( OPT_LANG_IS( IMPLICIT_int ) ? TB_int : TB_NONE );

      PARSE_ASSERT( c_type_add( &new_type, &$type, &@type ) );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type = new_type;

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

type_modifier_list_english_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | type_modifier_list_english_type
  ;

type_modifier_list_english_type
  : type_modifier_list_english_type[left_type]
    type_modifier_english_type[right_type]
    {
      DUMP_START( "type_modifier_list_english_type",
                  "type_modifier_list_english_type "
                  "type_modifier_english_type" );
      DUMP_TYPE( "type_modifier_list_english_type", $left_type );
      DUMP_TYPE( "type_modifier_english_type", $right_type );

      $$ = $left_type;
      PARSE_ASSERT( c_type_add( &$$, &$right_type, &@right_type ) );

      DUMP_TYPE( "$$_type", $$ );
      DUMP_END();
    }

  | type_modifier_english_type
  ;

type_modifier_english_type
  : type_modifier_base_type
  ;

unmodified_type_english_ast
  : builtin_type_english_ast
  | class_struct_union_english_ast
  | concept_type_english_ast
  | enum_english_ast
  | parameter_pack_english_ast
  | typedef_type_c_ast
  ;

builtin_type_english_ast
  : builtin_no_BitInt_english_btid[btid]
    {
      DUMP_START( "builtin_type_english_ast",
                  "builtin_no_BitInt_english_btid" );
      DUMP_TID( "builtin_no_BitInt_english_btid", $btid );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = c_tid_check( $btid, C_TPID_BASE );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  | BitInt_english_int[width]
    {
      DUMP_START( "builtin_type_english_ast", "BitInt_english_int" );
      DUMP_INT( "BitInt_english_int", $width );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = TB__BitInt;
      $$->builtin.BitInt.width = $width;

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

builtin_no_BitInt_english_btid
  : Y_void
  | Y_auto_TYPE
  | Y__Bool
  | Y_bool
  | Y_char uint_lit_opt[bits]
    {
      switch ( $bits ) {
        case  0: $$ = TB_char    ; break;
        case  8: $$ = TB_char8_t ; break;
        case 16: $$ = TB_char16_t; break;
        case 32: $$ = TB_char32_t; break;
        default:
          print_error( &@bits, "bits must be one of 8, 16, or 32\n" );
          PARSE_ABORT();
      } // switch
    }
  | Y_char8_t
  | Y_char16_t
  | Y_char32_t
  | Y_decltype
    {
      UNSUPPORTED( &@Y_decltype, "decltype() declarations" );
    }
  | Y_wchar_t
  | Y_wide char_exp               { $$ = TB_wchar_t; }
  | Y_int
  | Y_float
  | Y_floating point_exp          { $$ = TB_float; }
  | Y_double precision_opt        { $$ = TB_double; }
  | Y_EMC__Accum
  | Y_EMC__Fract
  ;

BitInt_english_int
  : BitInt_english uint_lit[width] bits_opt
    {
      $$ = $width;
    }
  | BitInt_english '(' uint_lit_exp[width] rparen_exp
    {
      $$ = $width;
    }
  | BitInt_english Y_width uint_lit_exp[width] bits_opt
    {
      $$ = $width;
    }
  | BitInt_english error
    {
      elaborate_error( "integer literal, '(', or \"width\" expected" );
    }
  ;

BitInt_english
  : Y__BitInt
  | Y_bit_precise int_exp
  | Y_bit precise_opt int_exp
  ;

concept_type_english_ast
  : Y_concept sname_english_exp[sname]
    {
      DUMP_START( "concept_type_english_ast", "CONCEPT sname_english_exp" );
      DUMP_SNAME( "sname_english_exp", $sname );

      c_sname_set_all_types( &$sname );

      $$ = c_ast_new_gc( K_CONCEPT, &@$ );
      $$->concept.concept_sname = c_sname_move( &$sname );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | Y_concept sname_english_exp[sname] Y_parameter pack_exp
    {
      DUMP_START( "concept_type_english_ast",
                  "CONCEPT sname_english_exp PARAMTER PACK" );
      DUMP_SNAME( "sname_english_exp", $sname );

      c_sname_set_all_types( &$sname );

      $$ = c_ast_new_gc( K_CONCEPT, &@$ );
      $$->is_param_pack = true;
      $$->concept.concept_sname = c_sname_move( &$sname );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

parameter_pack_english_ast
  : Y_parameter pack_exp
    {
      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->is_param_pack = true;
      $$->type.btids = TB_auto;         // for consistency with C++ case
    }
  ;

pack_exp
  : Y_pack
  | error
    {
      keyword_expected( L_pack );
    }
  ;

precise_opt
  : /* empty */
  | Y_precise
  ;

class_struct_union_english_ast
  : class_struct_union_btid[csu_btid] any_sname_c_exp[sname]
    {
      DUMP_START( "class_struct_union_english_ast",
                  "class_struct_union_btid any_sname_c_exp" );
      DUMP_TID( "class_struct_union_btid", $csu_btid );
      DUMP_SNAME( "any_sname_c_exp", $sname );

      $$ = c_ast_new_gc( K_CLASS_STRUCT_UNION, &@$ );
      $$->type.btids = c_tid_check( $csu_btid, C_TPID_BASE );
      $$->csu.csu_sname = c_sname_move( &$sname );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

enum_english_ast
  : enum_btids any_sname_c_exp[sname]
    of_type_enum_fixed_type_english_ast_opt[fixed_type_ast]
    {
      DUMP_START( "enum_english_ast",
                  "enum_btids any_sname_c_exp "
                  "of_type_enum_fixed_type_english_ast_opt" );
      DUMP_TID( "enum_btids", $enum_btids );
      DUMP_SNAME( "any_sname_c_exp", $sname );
      DUMP_AST( "enum_fixed_type_english_ast", $fixed_type_ast );

      $$ = c_ast_new_gc( K_ENUM, &@$ );
      $$->type.btids = c_tid_check( $enum_btids, C_TPID_BASE );
      c_ast_set_parent( $fixed_type_ast, $$ );
      $$->enum_.enum_sname = c_sname_move( &$sname );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

of_type_enum_fixed_type_english_ast_opt
  : /* empty */                         { $$ = NULL; }
  | Y_of type_opt enum_fixed_type_english_ast[fixed_type_ast]
    {
      $$ = $fixed_type_ast;
    }
  ;

enum_fixed_type_english_ast
  : enum_fixed_type_modifier_list_english_btid_opt[btids]
    enum_unmodified_fixed_type_english_ast[fixed_type_ast]
    {
      DUMP_START( "enum_fixed_type_english_ast",
                  "enum_fixed_type_modifier_list_stid" );
      DUMP_TID( "enum_fixed_type_modifier_list_stid", $btids );
      DUMP_AST( "enum_unmodified_fixed_type_english_ast", $fixed_type_ast );

      $$ = $fixed_type_ast;
      if ( $btids != TB_NONE ) {        // See comment in type_english_ast.
        $$->loc = @$;
        PARSE_ASSERT( c_type_add_tid( &$$->type, $btids, &@btids ) );
      }

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }

  | enum_fixed_type_modifier_list_english_btid[btids]
    {
      DUMP_START( "enum_fixed_type_english_ast",
                  "enum_fixed_type_modifier_list_english_btid" );
      DUMP_TID( "enum_fixed_type_modifier_list_english_btid", $btids );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = c_tid_check( $btids, C_TPID_BASE );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

enum_fixed_type_modifier_list_english_btid_opt
  : /* empty */                   { $$ = TB_NONE; }
  | enum_fixed_type_modifier_list_english_btid
  ;

enum_fixed_type_modifier_list_english_btid
  : enum_fixed_type_modifier_list_english_btid[btids]
    enum_fixed_type_modifier_btid[btid]
    {
      DUMP_START( "enum_fixed_type_modifier_list_english_btid",
                  "enum_fixed_type_modifier_list_english_btid "
                  "enum_fixed_type_modifier_btid" );
      DUMP_TID( "enum_fixed_type_modifier_list_english_btid", $btids );
      DUMP_TID( "enum_fixed_type_modifier_btid", $btid );

      $$ = $btids;
      PARSE_ASSERT( c_tid_add( &$$, $btid, &@btid ) );

      DUMP_TID( "$$_btids", $$ );
      DUMP_END();
    }

  | enum_fixed_type_modifier_btid
  ;

enum_unmodified_fixed_type_english_ast
  : builtin_type_english_ast
  | sname_english_ast
  ;

///////////////////////////////////////////////////////////////////////////////
//  NAMES                                                                    //
///////////////////////////////////////////////////////////////////////////////

any_name
  : Y_NAME
  | Y_TYPEDEF_NAME_TDEF[tdef]
    {
      assert( c_sname_count( &$tdef->ast->sname ) == 1 );
      $$ = check_strdup( c_sname_local_name( &$tdef->ast->sname ) );
    }
  ;

any_name_exp
  : any_name
  | error
    {
      elaborate_error( "name expected" );
      $$ = NULL;
    }
  ;

any_sname_c
  : sname_c
  | typedef_sname_c
  ;

any_sname_c_exp
  : any_sname_c
  | error
    {
      c_sname_init( &$$ );
      elaborate_error( "name expected" );
    }
  ;

any_sname_c_opt
  : /* empty */                   { c_sname_init( &$$ ); }
  | any_sname_c
  ;

any_typedef_tdef
  : Y_TYPEDEF_NAME_TDEF
  | Y_TYPEDEF_SNAME_TDEF
  ;

name_ast
  : Y_NAME[name]
    {
      DUMP_START( "name_ast", "NAME" );
      DUMP_STR( "NAME", $name );

      $$ = c_ast_new_gc( K_NAME, &@$ );
      c_sname_init_name( &$$->sname, $name );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

name_exp
  : Y_NAME
  | error
    {
      elaborate_error( "name expected" );
      $$ = NULL;
    }
  ;

name_cat
  : name_cat[dst] Y_NAME[src]
    {
      $$ = str_realloc_cat( $dst, " ", $src );
      free( $src );
    }
  | Y_NAME
  ;

name_opt
  : /* empty */                   { $$ = NULL; }
  | Y_NAME
  ;

oper_sname_c_opt
  : /* empty */                   { c_sname_init( &$$ ); }

  | Y_OPERATOR_SNAME[sname] Y_COLON_COLON
    {
      $$ = $sname;
      if ( c_type_is_none( c_sname_local_type( &$$ ) ) )
        c_sname_local_data( &$$ )->type = C_TYPE_LIT_B( TB_SCOPE );
    }
  ;

sname_c
  : sname_c[sname] Y_COLON_COLON Y_NAME[name]
    {
      // see the comment in "of_scope_english"
      if ( !OPT_LANG_IS( SCOPED_NAMES ) ) {
        print_error( &@2,
          "scoped names not supported%s\n",
          C_LANG_WHICH( SCOPED_NAMES )
        );
        c_sname_cleanup( &$sname );
        free( $name );
        PARSE_ABORT();
      }

      DUMP_START( "sname_c", "sname_c '::' NAME" );
      DUMP_SNAME( "sname_c", $sname );
      DUMP_STR( "name", $name );

      $$ = $sname;
      c_sname_local_data( &$$ )->type = C_TYPE_LIT_B( TB_SCOPE );
      c_sname_push_back_name( &$$, $name );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }

  | sname_c[sname] Y_COLON_COLON any_typedef_tdef[tdef]
    { //
      // This is for a case like:
      //
      //      define S::int8_t as char
      //
      // that is: the type int8_t is an existing type in no scope being defined
      // as a distinct type in a new scope.
      //
      DUMP_START( "sname_c", "sname_c '::' any_typedef_tdef" );
      DUMP_SNAME( "sname_c", $sname );
      DUMP_AST( "any_typedef__ast", $tdef->ast );

      $$ = $sname;
      c_sname_local_data( &$$ )->type = C_TYPE_LIT_B( TB_SCOPE );
      c_sname_t temp_sname = c_sname_dup( &$tdef->ast->sname );
      c_sname_push_back_sname( &$$, &temp_sname );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }

  | Y_NAME[name]
    {
      DUMP_START( "sname_c", "NAME" );
      DUMP_STR( "NAME", $name );

      c_sname_init_name( &$$, $name );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }
  ;

sname_c_ast
  : // in_attr: type_c_ast
    sname_c[sname] bit_field_c_uint_opt[bit_width]
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "sname_c_ast", "sname_c" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_SNAME( "sname_c", $sname );
      DUMP_INT( "bit_field_c_uint_opt", $bit_width );

      $$ = ia_type_spec_ast( type_ast );
      c_sname_set( &$$->sname, &$sname );
      PARSE_ASSERT( c_ast_set_bit_field_width( $$, $bit_width ) );

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

bit_field_c_uint_opt
  : /* empty */                   { $$ = 0; }
  | ':' uint_lit_exp[bit_width]
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since we use 0 to mean "no bit-field."
      //
      if ( $bit_width == 0 ) {
        print_error( &@bit_width, "bit-field width must be > 0\n" );
        PARSE_ABORT();
      }
      $$ = $bit_width;
    }
  ;

sname_c_exp
  : sname_c
  | error
    {
      c_sname_init( &$$ );
      elaborate_error( "name expected" );
    }
  ;

sname_c_opt
  : /* empty */                   { c_sname_init( &$$ ); }
  | sname_c
  ;

sname_english
  : any_sname_c[sname] of_scope_list_english_opt[scope_sname]
    {
      DUMP_START( "sname_english", "any_sname_c of_scope_list_english_opt" );
      DUMP_SNAME( "any_sname_c", $sname );
      DUMP_SNAME( "of_scope_list_english_opt", $scope_sname );

      c_type_t const *local_type = c_sname_local_type( &$scope_sname );
      if ( c_type_is_none( local_type ) )
        local_type = c_sname_local_type( &$sname );
      $$ = $scope_sname;
      c_sname_push_back_sname( &$$, &$sname );
      c_sname_local_data( &$$ )->type = *local_type;

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }
  ;

sname_english_ast
  : Y_NAME[name] of_scope_list_english_opt[scope_sname]
    {
      DUMP_START( "sname_english_ast", "NAME of_scope_list_english_opt" );
      DUMP_STR( "NAME", $name );
      DUMP_SNAME( "of_scope_list_english_opt", $scope_sname );

      c_sname_t sname = c_sname_move( &$scope_sname );
      c_sname_push_back_name( &sname, $name );

      c_typedef_t const *const tdef = c_typedef_find_sname( &sname );
      if ( tdef != NULL ) {
        //
        // The sname is the name of a typedef'd type: create a new K_TYPEDEF
        // node and point it at the typedef's AST.
        //
        $$ = c_ast_new_gc( K_TYPEDEF, &@$ );
        $$->type.btids = TB_typedef;
        $$->tdef.for_ast = tdef->ast;
        c_sname_cleanup( &sname );
      }
      else {
        $$ = c_ast_new_gc( K_NAME, &@$ );
        $$->sname = sname;
        //
        // Set the c_name_ast's sname also in case it's used as a type name.
        //
        c_sname_t temp_sname = c_sname_dup( &$$->sname );
        c_sname_set( &$$->name.sname, &temp_sname );
      }

      DUMP_AST( "$$_ast", $$ );
      DUMP_END();
    }
  ;

sname_english_exp
  : sname_english
  | error
    {
      c_sname_init( &$$ );
      elaborate_error( "name expected" );
    }
  ;

sname_english_opt
  : /* empty */                   { c_sname_init( &$$ ); }
  | sname_english
  ;

sname_list_c
  : sname_list_c[sname_list] ',' sname_c[sname]
    {
      DUMP_START( "sname_list_c",
                  "sname_list_c ',' sname_c" );
      DUMP_SNAME_LIST( "sname_list_c", $sname_list );
      DUMP_SNAME( "sname_c", $sname );

      $$ = $sname_list;
      c_sname_t *const temp_sname = MALLOC( c_sname_t, 1 );
      *temp_sname = c_sname_move( &$sname );
      slist_push_back( &$$, temp_sname );

      DUMP_SNAME_LIST( "$$_sname_list", $$ );
      DUMP_END();
    }
  | sname_c[sname]
    {
      DUMP_START( "sname_list_c", "sname_c" );
      DUMP_SNAME( "sname_c", $sname );

      c_sname_t *const temp_sname = MALLOC( c_sname_t, 1 );
      *temp_sname = c_sname_move( &$sname );
      slist_init( &$$ );
      slist_push_back( &$$, temp_sname );

      DUMP_SNAME_LIST( "$$_sname_list", $$ );
      DUMP_END();
    }
  ;

sname_list_english
  : sname_list_english[left_sname] ',' sname_english_exp[right_sname]
    {
      DUMP_START( "sname_list_english",
                  "sname_list_english ',' sname_english" );
      DUMP_SNAME_LIST( "sname_list_english", $left_sname );
      DUMP_SNAME( "sname_english_exp", $right_sname );

      $$ = $left_sname;
      c_sname_t *const temp_sname = MALLOC( c_sname_t, 1 );
      *temp_sname = c_sname_move( &$right_sname );
      slist_push_back( &$$, temp_sname );

      DUMP_SNAME_LIST( "$$_sname_list", $$ );
      DUMP_END();
    }

  | sname_english[sname]
    {
      DUMP_START( "sname_list_english", "sname_english" );
      DUMP_SNAME( "sname_english", $sname );

      c_sname_t *const temp_sname = MALLOC( c_sname_t, 1 );
      *temp_sname = c_sname_move( &$sname );
      slist_init( &$$ );
      slist_push_back( &$$, temp_sname );

      DUMP_SNAME_LIST( "$$_sname_list", $$ );
      DUMP_END();
    }
  ;

typedef_sname_c
  : typedef_sname_c[tdef_sname] Y_COLON_COLON sname_c[sname]
    {
      DUMP_START( "typedef_sname_c", "typedef_sname_c '::' sname_c" );
      DUMP_SNAME( "typedef_sname_c", $tdef_sname );
      DUMP_SNAME( "sname_c", $sname );

      //
      // This is for a case like:
      //
      //      define S as struct S
      //      define S::T as struct T
      //
      $$ = c_sname_move( &$tdef_sname );
      c_sname_push_back_sname( &$$, &$sname );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }

  | typedef_sname_c[sname] Y_COLON_COLON any_typedef_tdef[tdef]
    {
      DUMP_START( "typedef_sname_c", "typedef_sname_c '::' any_typedef_tdef" );
      DUMP_SNAME( "typedef_sname_c", $sname );
      DUMP_AST( "any_typedef__ast", $tdef->ast );

      //
      // This is for a case like:
      //
      //      define S as struct S
      //      define T as struct T
      //      define S::T as struct S_T
      //
      $$ = c_sname_move( &$sname );
      c_sname_local_data( &$$ )->type =
        *c_sname_local_type( &$tdef->ast->sname );
      c_sname_t temp_sname = c_sname_dup( &$tdef->ast->sname );
      c_sname_push_back_sname( &$$, &temp_sname );

      DUMP_SNAME( "$$_sname", $$ );
      DUMP_END();
    }

  | any_typedef_tdef[tdef]        { $$ = c_sname_dup( &$tdef->ast->sname ); }
  ;

///////////////////////////////////////////////////////////////////////////////
//  MISCELLANEOUS                                                            //
///////////////////////////////////////////////////////////////////////////////

address_exp
  : Y_address
  | error
    {
      keyword_expected( L_address );
    }
  ;

array_exp
  : Y_array
  | error
    {
      keyword_expected( L_array );
    }
  ;

as_exp
  : Y_as
    {
      if ( OPT_LANG_IS( MEMBER_FUNCTIONS ) ) {
        //
        // For either "declare" or "define", neither "override" nor "final"
        // must be matched initially to allow for cases like:
        //
        //      c++decl> declare final as int
        //      int final;
        //
        // (which is legal).  However, in C++, after parsing "as", the keyword
        // context has to be set to C_KW_CTX_MBR_FUNC to be able to match
        // "override" and "final", e.g.:
        //
        //      c++decl> declare f as final function
        //      void f() final;
        //
        lexer_keyword_ctx = C_KW_CTX_MBR_FUNC;
      }
    }
  | error
    {
      keyword_expected( L_as );
    }
  ;

as_into_to_exp
  : Y_as
  | Y_into
  | Y_to
  | error
    {
      elaborate_error( "\"as\", \"into\", or \"to\" expected" );
    }
  ;

as_or_to_opt
  : /* empty */
  | Y_as
  | Y_to
  ;

auto_TYPE_exp
  : Y_auto_TYPE
  | error
    {
      keyword_expected( L_auto );
    }
  ;

binding_exp
  : Y_binding
  | error
    {
      keyword_expected( L_binding );
    }
  ;

bits_opt
  : /* empty */
  | Y_bit
  | Y_bits
  ;

bytes_opt
  : /* empty */
  | Y_bytes
  ;

cast_exp
  : Y_cast
  | error
    {
      keyword_expected( L_cast );
    }
  ;

char_exp
  : Y_char
  | error
    {
      keyword_expected( L_char );
    }
  ;

class_struct_union_btid_exp
  : class_struct_union_btid
  | error
    {
      elaborate_error( "\"class\", \"struct\", or \"union\" expected" );
    }
  ;

colon_exp
  : ':'
  | error
    {
      punct_expected( ':' );
    }
  ;

comma_exp
  : ','
  | error
    {
      punct_expected( ',' );
    }
  ;

conversion_exp
  : Y_conversion
  | error
    {
      keyword_expected( L_conversion );
    }
  ;

c_operator
  : Y_co_await                    { $$ = C_OP_CO_AWAIT              ; }
  | Y_new                         { $$ = C_OP_NEW                   ; }
  | Y_new '[' rbracket_exp        { $$ = C_OP_NEW_ARRAY             ; }
  | Y_delete                      { $$ = C_OP_DELETE                ; }
  | Y_delete '[' rbracket_exp     { $$ = C_OP_DELETE_ARRAY          ; }
  | '!'                           { $$ = C_OP_EXCLAMATION           ; }
  | Y_EXCLAM_EQUAL                { $$ = C_OP_EXCLAMATION_EQUAL     ; }
  | '%'                           { $$ = C_OP_PERCENT               ; }
  | Y_PERCENT_EQUAL               { $$ = C_OP_PERCENT_EQUAL         ; }
  | '&'                           { $$ = C_OP_AMPERSAND             ; }
  | Y_AMPER_AMPER                 { $$ = C_OP_AMPERSAND_AMPERSAND   ; }
  | Y_AMPER_EQUAL                 { $$ = C_OP_AMPERSAND_EQUAL       ; }
  | '(' rparen_exp                { $$ = C_OP_PARENTHESES           ; }
  | '*'                           { $$ = C_OP_STAR                  ; }
  | Y_STAR_EQUAL                  { $$ = C_OP_STAR_EQUAL            ; }
  | '+'                           { $$ = C_OP_PLUS                  ; }
  | Y_PLUS_PLUS                   { $$ = C_OP_PLUS_PLUS             ; }
  | Y_PLUS_EQUAL                  { $$ = C_OP_PLUS_EQUAL            ; }
  | ','                           { $$ = C_OP_COMMA                 ; }
  | '-'                           { $$ = C_OP_MINUS                 ; }
  | Y_MINUS_MINUS                 { $$ = C_OP_MINUS_MINUS           ; }
  | Y_MINUS_EQUAL                 { $$ = C_OP_MINUS_EQUAL           ; }
  | Y_MINUS_GREATER               { $$ = C_OP_MINUS_GREATER         ; }
  | Y_MINUS_GREATER_STAR          { $$ = C_OP_MINUS_GREATER_STAR    ; }
  | '.'                           { $$ = C_OP_DOT                   ; }
  | Y_DOT_STAR                    { $$ = C_OP_DOT_STAR              ; }
  | '/'                           { $$ = C_OP_SLASH                 ; }
  | Y_SLASH_EQUAL                 { $$ = C_OP_SLASH_EQUAL           ; }
  | Y_COLON_COLON                 { $$ = C_OP_COLON_COLON           ; }
  | '<'                           { $$ = C_OP_LESS                  ; }
  | Y_LESS_LESS                   { $$ = C_OP_LESS_LESS             ; }
  | Y_LESS_LESS_EQUAL             { $$ = C_OP_LESS_LESS_EQUAL       ; }
  | Y_LESS_EQUAL                  { $$ = C_OP_LESS_EQUAL            ; }
  | Y_LESS_EQUAL_GREATER          { $$ = C_OP_LESS_EQUAL_GREATER    ; }
  | '='                           { $$ = C_OP_EQUAL                 ; }
  | Y_EQUAL_EQUAL                 { $$ = C_OP_EQUAL_EQUAL           ; }
  | '>'                           { $$ = C_OP_GREATER               ; }
  | Y_GREATER_GREATER             { $$ = C_OP_GREATER_GREATER       ; }
  | Y_GREATER_GREATER_EQUAL       { $$ = C_OP_GREATER_GREATER_EQUAL ; }
  | Y_GREATER_EQUAL               { $$ = C_OP_GREATER_EQUAL         ; }
  | Y_QMARK_COLON                 { $$ = C_OP_QUESTION_MARK_COLON   ; }
  | '[' rbracket_exp              { $$ = C_OP_BRACKETS              ; }
  | '^'                           { $$ = C_OP_CARET                 ; }
  | Y_CARET_EQUAL                 { $$ = C_OP_CARET_EQUAL           ; }
  | '|'                           { $$ = C_OP_PIPE                  ; }
  | Y_PIPE_PIPE                   { $$ = C_OP_PIPE_PIPE             ; }
  | Y_PIPE_EQUAL                  { $$ = C_OP_PIPE_EQUAL            ; }
  | '~'                           { $$ = C_OP_TILDE                 ; }
  ;

default_exp
  : Y_default
  | error
    {
      keyword_expected( L_default );
    }
  ;

defined_exp
  : Y_defined
  | error
    {
      keyword_expected( L_defined );
    }
  ;

dependency_exp
  : Y_dependency
  | error
    {
      keyword_expected( L_dependency );
    }
  ;

destructor_sname
  : Y_DESTRUCTOR_SNAME
  | Y_LEXER_ERROR
    {
      c_sname_init( &$$ );
      PARSE_ABORT();
    }
  ;

empty_str_lit_exp
  : str_lit_exp[str]
    { //
      // This check is done now in the parser rather than later in the AST so
      // we don't have to keep the string literal around.
      //
      bool const is_empty = $str == NULL || $str[0] == '\0';
      free( $str );
      if ( !is_empty ) {
        print_error( &@str, "empty string literal expected\n" );
        PARSE_ABORT();
      }
    }
  ;

equals_exp
  : '='
  | error
    {
      punct_expected( '=' );
    }
  ;

extern_linkage_c_stid
  : Y_extern linkage_stid[stid]   { $$ = $stid; }
  | Y_extern linkage_stid '{'[brace]
    {
      UNSUPPORTED( &@brace, "scoped linkage declarations" );
    }
  ;

extern_linkage_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | extern_linkage_c_stid
  ;

glob
  : Y_GLOB[glob_name]
    {
      if ( !OPT_LANG_IS( SCOPED_NAMES ) && strchr( $glob_name, ':' ) != NULL ) {
        print_error( &@glob_name,
          "scoped names not supported%s\n",
          C_LANG_WHICH( SCOPED_NAMES )
        );
        free( $glob_name );
        PARSE_ABORT();
      }
      $$ = $glob_name;
    }
  ;

glob_opt
  : /* empty */                   { $$ = NULL; }
  | glob
  ;

gt_exp
  : '>'
  | error
    {
      punct_expected( '>' );
    }
  ;

inline_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_inline
  ;

int_exp
  : Y_int
  | error
    {
      elaborate_error( "\"int[eger]\" expected" );
    }
  ;

lbracket_exp
  : '['
  | error
    {
      punct_expected( '[' );
    }
  ;

literal_exp
  : Y_literal
  | error
    {
      keyword_expected( L_literal );
    }
  ;

local_exp
  : Y_local
  | error
    {
      keyword_expected( L_local );
    }
  ;

lparen_exp
  : '('
  | error
    {
      punct_expected( '(' );
    }
  ;

lt_exp
  : '<'
  | error
    {
      punct_expected( '<' );
    }
  ;

member_or_non_member_opt
  : /* empty */                   { $$ = C_FUNC_UNSPECIFIED; }
  | Y_member                      { $$ = C_FUNC_MEMBER     ; }
  | Y_non_member                  { $$ = C_FUNC_NON_MEMBER ; }
  ;

namespace_btid_exp
  : Y_namespace
  | error
    {
      keyword_expected( L_namespace );
    }
  ;

namespace_type
  : Y_namespace                   { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_inline namespace_btid_exp   { $$ = C_TYPE_LIT( $2, $1, TA_NONE ); }
  ;

of_exp
  : Y_of
  | error
    {
      keyword_expected( L_of );
    }
  ;

of_scope_english
  : Y_of scope_english_type_exp[scope_type] any_sname_c_exp[sname]
    { //
      // Scoped names are supported only in C++.  (However, we always allow
      // them in configuration files.)
      //
      // This check is better to do now in the parser rather than later in the
      // AST because it has to be done in fewer places in the code plus gives a
      // better error location.
      //
      if ( !OPT_LANG_IS( SCOPED_NAMES ) ) {
        print_error( &@scope_type,
          "scoped names not supported%s\n",
          C_LANG_WHICH( SCOPED_NAMES )
        );
        c_sname_cleanup( &$sname );
        PARSE_ABORT();
      }
      $$ = c_sname_move( &$sname );
      c_sname_local_data( &$$ )->type = $scope_type;
    }
  ;

of_scope_list_english
  : of_scope_list_english[left_sname] of_scope_english[right_sname]
    {
      // "of scope X of scope Y" means Y::X
      $$ = c_sname_move( &$right_sname );
      c_sname_push_back_sname( &$$, &$left_sname );
      c_sname_set_all_types( &$$ );
      if ( !c_sname_check( &$$, &@left_sname ) ) {
        c_sname_cleanup( &$$ );
        PARSE_ABORT();
      }
    }
  | of_scope_english
  ;

of_scope_list_english_opt
  : /* empty */                   { c_sname_init( &$$ ); }
  | of_scope_list_english
  ;

operator_exp
  : Y_operator
  | error
    {
      keyword_expected( L_operator );
    }
  ;

operator_opt
  : /* empty */
  | Y_operator
  ;

point_exp
  : Y_point
  | error
    {
      keyword_expected( L_point );
    }

precision_opt
  : /* empty */
  | Y_precision
  ;

rbrace_exp
  : '}'
  | error
    {
      punct_expected( '}' );
    }
  ;

rbracket_exp
  : ']'
  | error
    {
      punct_expected( ']' );
    }
  ;

ref_qualifier_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | '&'                           { $$ = TS_REFERENCE; }
  | Y_AMPER_AMPER                 { $$ = TS_RVALUE_REFERENCE; }
  ;

reference_exp
  : Y_reference
  | error
    {
      keyword_expected( L_reference );
    }
  ;

returning
  : Y_returning
  | Y_return
  ;

returning_exp
  : returning
  | error
    {
      keyword_expected( L_returning );
    }
  ;

rparen_exp
  : ')'
  | error
    {
      punct_expected( ')' );
    }
  ;

scope_english_type
  : class_struct_union_btid       { $$ = C_TYPE_LIT_B( $1 ); }
  | namespace_type
  | Y_scope                       { $$ = C_TYPE_LIT_B( TB_SCOPE ); }
  ;

scope_english_type_exp
  : scope_english_type
  | error
    {
      elaborate_error(
        "\"class\", \"namespace\", \"scope\", \"struct\", or \"union\" expected"
      );
    }
  ;

semi_exp
  : ';'
  | error
    {
      punct_expected( ';' );
    }
  ;

semi_opt
  : /* empty */
  | ';'
  ;

semi_or_end
  : ';'
  | Y_END
  ;

static_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_static                      { $$ = TS_NON_EMPTY_ARRAY; }
  ;

str_lit
  : Y_STR_LIT
  | Y_LEXER_ERROR
    {
      $$ = NULL;
      PARSE_ABORT();
    }
  ;

str_lit_exp
  : str_lit
  | error
    {
      elaborate_error( "string literal expected" );
      $$ = NULL;
    }
  ;

this_exp
  : Y_this
  | error
    {
      keyword_expected( L_this );
    }
  ;

this_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_this
  ;

to_exp
  : Y_to
  | error
    {
      keyword_expected( L_to );
    }
  ;

type_opt
  : /* empty */
  | Y_typedef
  ;

typename_flag_opt
  : /* empty */                   { $$ = false; }
  | Y_typename                    { $$ = true; }
  ;

uint_lit
  : Y_INT_LIT[int_val]
    {
      if ( $int_val < 0 ) {
        //
        // This check is better to do now in the parser rather than later in
        // the AST because we can store the value as unsigned plus gives a
        // better error location.
        //
        print_error( &@int_val, "non-negative integer expected\n" );
        PARSE_ABORT();
      }
      $$ = STATIC_CAST( unsigned, $int_val );
    }
  ;

uint_lit_exp
  : uint_lit
  | error
    {
      elaborate_error( "integer literal expected" );
    }
  ;

uint_lit_opt
  : /* empty */                   { $$ = 0; }
  | uint_lit
  ;

unused_exp
  : Y_unused
  | error
    {
      keyword_expected( L_unused );
    }
  ;

user_defined
  : Y_user_defined
  | Y_user defined_exp
  ;

virtual_stid_exp
  : Y_virtual
  | error
    {
      keyword_expected( L_virtual );
    }
  ;

virtual_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_virtual
  ;

%%

/// @endcond

// Re-enable warnings.
#pragma GCC diagnostic pop

/**
 * @addtogroup parser-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints an additional parsing error message including a newline to standard
 * error that continues from where yyerror() left off.  Additionally:
 *
 * + If the printable_yytext() isn't NULL:
 *     + Checks to see if it's a keyword: if it is, mentions that it's a
 *       keyword in the error message.
 *     + May print "did you mean ...?" \a dym_kinds suggestions.
 *
 * + In debug mode, also prints the file & line where the function was called
 *   from as well as the ID of the lookahead token, if any.
 *
 * @note A newline _is_ printed.
 * @note This function isn't normally called directly; use the
 * #elaborate_error() or #elaborate_error_dym() macros instead.
 *
 * @param line The line number within this file where this function was called
 * from.
 * @param dym_kinds The bitwise-or of the kind(s) of things possibly meant.
 * @param format A `printf()` style format string.  It _must not_ end in a
 * newline since this function prints its own newline.
 * @param ... Arguments to print.
 *
 * @sa #elaborate_error()
 * @sa #elaborate_error_dym()
 * @sa l_keyword_expected()
 * @sa l_punct_expected()
 * @sa yyerror()
 */
PJL_PRINTF_LIKE_FUNC(3)
static void l_elaborate_error( int line, dym_kind_t dym_kinds,
                               char const *format, ... ) {
  assert( format != NULL );

  EPUTS( ": " );
  print_debug_file_line( __FILE__, line );

  char const *const error_token = printable_yytext();
  if ( print_error_token( error_token ) )
    EPUTS( ": " );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );

  if ( error_token != NULL ) {
    print_error_token_is_a( error_token );
    print_suggestions( dym_kinds, error_token );
  }

  EPUTC( '\n' );
}

/**
 * Cleans up all global parser data at program termination.
 *
 * @note This function is called only via **atexit**(3).
 *
 * @sa parser_init()
 */
static void parser_cleanup( void ) {
  c_ast_list_cleanup_gc( &typedef_ast_list );
}

/**
 * Prints \a token, quoted; if \ref opt_cdecl_debug `!=` #CDECL_DEBUG_NO, also
 * prints the look-ahead character within `[]`.
 *
 * @param token The error token to print, if any.
 * @return Returns `true` only if anything was printed.
 */
static bool print_error_token( char const *token ) {
  if ( token == NULL )
    return false;
  EPRINTF( "\"%s\"", token );
  if ( opt_cdecl_debug != CDECL_DEBUG_NO ) {
    // LCOV_EXCL_START
    switch ( yychar ) {
      case YYEMPTY:
        EPUTS( " [<EMPTY>]" );
        break;
      case YYEOF:
        EPUTS( " [<EOF>]" );
        break;
      case YYerror:
        EPUTS( " [<error>]" );
        break;
      case YYUNDEF:
        EPUTS( " [<UNDEF>]" );
        break;
      default:
        EPRINTF( isprint( yychar ) ? " ['%c']" : " [%d]", yychar );
    } // switch
    // LCOV_EXCL_STOP
  }
  return true;
}

////////// extern functions ///////////////////////////////////////////////////

void parser_init( void ) {
  ASSERT_RUN_ONCE();
  ATEXIT( &parser_cleanup );
}

bool yyparse_sn( char const *s, size_t s_len ) {
  assert( s != NULL );

  FILE *const s_file = fmemopen( CONST_CAST( void*, s ), s_len, "r" );
  PERROR_EXIT_IF( s_file == NULL, EX_IOERR );
  yyrestart( s_file );

  int const rv_bison = yyparse();
  fclose( s_file );
  if ( unlikely( rv_bison == 2 ) ) {
    //
    // Bison has already printed "memory exhausted" via yyerror() that doesn't
    // print a newline, so print one now.
    //
    // LCOV_EXCL_START
    EPUTC( '\n' );
    _Exit( EX_SOFTWARE );
    // LCOV_EXCL_STOP
  }

  return rv_bison == 0;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
