/*
**      cdecl -- C gibberish translator
**      src/parser.y
**
**      Copyright (C) 2017-2023  Paul J. Lucas, et al.
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
%expect 17

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
#include "cdecl_keyword.h"
#include "color.h"
#ifdef ENABLE_CDECL_DEBUG
#include "dump.h"
#endif /* ENABLE_CDECL_DEBUG */
#include "did_you_mean.h"
#include "english.h"
#include "gibberish.h"
#include "help.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "set_options.h"
#include "slist.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Silence these warnings for Bison-generated code.
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Wredundant-decls"
# pragma GCC diagnostic ignored "-Wunreachable-code"
#endif /* __GNUC__ */

// Developer aid for tracing when Bison %destructors are called.
#if 0
#define DTRACE                    EPRINTF( "%d: destructor\n", __LINE__ )
#else
#define DTRACE                    NO_OP
#endif

#ifdef ENABLE_CDECL_DEBUG
#define IF_DEBUG(...) \
  BLOCK( if ( opt_cdecl_debug ) { __VA_ARGS__ } )
#else
#define IF_DEBUG(...)             NO_OP
#endif /* ENABLE_CDECL_DEBUG */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup parser-group Parser
 * Helper macros, data structures, variables, functions, and the grammar for
 * C/C++ declarations.
 * @{
 */

/**
 * Calls #elaborate_error_dym() with #DYM_NONE.
 *
 * @param ... Arguments passed to fl_elaborate_error().
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
 * Calls fl_elaborate_error() followed by #PARSE_ABORT().
 *
 * @param DYM_KINDS The bitwise-or of \ref dym_kind_t things possibly meant.
 * @param ... Arguments passed to fl_elaborate_error().
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
  fl_elaborate_error( __FILE__, __LINE__, (DYM_KINDS), __VA_ARGS__ ); PARSE_ABORT(); )

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
 * @sa fl_is_nested_type_ok()
 */
#define is_nested_type_ok(TYPE_LOC) \
  fl_is_nested_type_ok( __FILE__, __LINE__, (TYPE_LOC) )

/**
 * Calls fl_keyword_expected() followed by #PARSE_ABORT().
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
  fl_keyword_expected( __FILE__, __LINE__, (KEYWORD) ); PARSE_ABORT(); )

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
 * Calls fl_punct_expected() followed by #PARSE_ABORT().
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
  fl_punct_expected( __FILE__, __LINE__, (PUNCT) ); PARSE_ABORT(); )

/**
 * Show all (as opposed to only those that are supported in the current
 * language) predefined, user, or both types.
 */
#define SHOW_ALL_TYPES            0x10u

/**
* Show only predefined types that are valid in the current language.
*/
#define SHOW_PREDEFINED_TYPES     (1u << 0)

/**
 * Show only user-defined types that were defined in the current language or
 * earlier.
 */
#define SHOW_USER_DEFINED_TYPES   (1u << 1)

/** @} */

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup parser-dump-group Debugging Macros
 * Macros that are used to dump a trace during parsing when \ref
 * opt_cdecl_debug is `true`.
 * @ingroup parser-group
 * @{
 */

/**
 * Dumps an AST.
 *
 * @param KEY The key name to print.
 * @param AST The AST to dump.
 *
 * @sa #DUMP_AST_LIST()
 */
#define DUMP_AST(KEY,AST) IF_DEBUG( \
  DUMP_COMMA; c_ast_dump( (AST), /*indent=*/1, (KEY), stdout ); )

/**
 * Dumps an s_list of AST.
 *
 * @param KEY The key name to print.
 * @param AST_LIST The \ref slist of AST to dump.
 *
 * @sa #DUMP_AST()
 */
#define DUMP_AST_LIST(KEY,AST_LIST) IF_DEBUG( \
  DUMP_KEY( KEY ": " ); c_ast_list_dump( &(AST_LIST), /*indent=*/1, stdout ); )

/**
 * Dumps a `bool`.
 *
 * @param KEY The key name to print.
 * @param BOOL The `bool` to dump.
 *
 * @sa #DUMP_KEY()
 */
#define DUMP_BOOL(KEY,BOOL)  IF_DEBUG(  \
  DUMP_KEY( KEY ": " ); bool_dump( (BOOL), stdout ); )

/**
 * Dumps a comma followed by a newline the _second_ and subsequent times it's
 * called used to separate items being dumped.
 */
#define DUMP_COMMA                fput_sep( ",\n", &dump_comma, stdout )

/**
 * Ends a dump block.
 *
 * @sa #DUMP_START()
 */
#define DUMP_END()                IF_DEBUG( PUTS( "\n}\n\n" ); )

/**
 * Possibly dumps a comma and a newline followed by the `printf()` arguments
 * &mdash; used for printing a key followed by a value.
 *
 * @param ... The `printf()` arguments.
 */
#define DUMP_KEY(...) IF_DEBUG( \
  DUMP_COMMA; PRINTF( "  " __VA_ARGS__ ); )

/**
 * Dumps an integer.
 *
 * @param KEY The key name to print.
 * @param NUM The integer to dump.
 *
 * @sa #DUMP_KEY()
 * @sa #DUMP_STR()
 */
#define DUMP_INT(KEY,NUM) \
  DUMP_KEY( KEY ": %d", STATIC_CAST( int, (NUM) ) )

/**
 * Dumps a scoped name.
 *
 * @param KEY The key name to print.
 * @param SNAME The scoped name to dump.
 *
 * @sa #DUMP_KEY()
 * @sa #DUMP_SNAME_LIST()
 * @sa #DUMP_STR()
 */
#define DUMP_SNAME(KEY,SNAME) IF_DEBUG( \
  DUMP_KEY( KEY ": " ); c_sname_dump( &(SNAME), stdout ); )

/**
 * Dumps a list of scoped names.
 *
 * @param KEY The key name to print.
 * @param LIST The list of scoped names to dump.
 *
 * @sa #DUMP_KEY()
 * @sa #DUMP_SNAME()
 */
#define DUMP_SNAME_LIST(KEY,LIST) IF_DEBUG( \
  DUMP_KEY( KEY ": " ); c_sname_list_dump( &(LIST), stdout ); )

#ifdef ENABLE_CDECL_DEBUG
/**
 * @def DUMP_START
 *
 * Starts a dump block.  The dump block _must_ end with #DUMP_END().  If a rule
 * has a result, it should be dumped as the final thing before the #DUMP_END()
 * repeating the name of the rule, e.g.:
 * ```
 *  DUMP_START( "rule",                 // <-- This rule name ...
 *              "subrule_1 name subrule_2" );
 *  DUMP_AST( "subrule_1", $1 );
 *  DUMP_STR( "name", $2 );
 *  DUMP_AST( "subrule_2", $3 );
 *  // ...
 *  DUMP_AST( "rule", $$ );             // <-- ... is repeated here.
 *  DUMP_END();
 * ```
 *
 * Whenever possible, an entire dump block should be completed before any
 * actions are taken as a result of a failed semantic check or other error
 * (typically, calling print_error() and #PARSE_ABORT()) so that the entire
 * block is dumped for debugging purposes first. If necessary, set a flag
 * within the dump block, then check it after, e.g.:
 * ```
 *  DUMP_START( "rule", "subrule_1 name subrule_2" );
 *  DUMP_AST( "subrule_1", $1 );
 *  DUMP_STR( "name", $2 );
 *  DUMP_AST( "subrule_2", $3 );
 *
 *  bool ok = true;
 *  if ( semantic_check_failed ) {
 *    // ...
 *    ok = false;
 *  }
 *
 *  DUMP_AST( "rule", $$ );
 *  DUMP_END();
 *
 *  if ( !ok ) {
 *    print_error( &@1, "...\n" );
 *    PARSE_ABORT();
 *  }
 * ```
 *
 * @param NAME The grammar production name.
 * @param RULE The grammar production rule.
 *
 * @sa #DUMP_AST
 * @sa #DUMP_AST_LIST
 * @sa #DUMP_BOOL
 * @sa #DUMP_END
 * @sa #DUMP_INT
 * @sa #DUMP_SNAME
 * @sa #DUMP_STR
 * @sa #DUMP_TID
 * @sa #DUMP_TYPE
 */
#define DUMP_START(NAME,RULE)         \
  bool dump_comma = false; IF_DEBUG(  \
  PUTS( NAME ": {\n  rule: " ); str_dump( (RULE), stdout ); PUTS( ",\n" ); )
#else
#define DUMP_START(NAME,RULE)     NO_OP
#endif

/**
 * Dumps a C string.
 *
 * @param KEY The key name to print.
 * @param STR The C string to dump.
 *
 * @sa #DUMP_INT()
 * @sa #DUMP_SNAME()
 */
#define DUMP_STR(KEY,STR) IF_DEBUG( \
  DUMP_KEY( KEY ": " ); str_dump( (STR), stdout ); )

/**
 * Dumps a \ref c_tid_t.
 *
 * @param KEY The key name to print.
 * @param TID The \ref c_tid_t to dump.
 *
 * @sa #DUMP_TYPE()
 */
#define DUMP_TID(KEY,TID) IF_DEBUG( \
  DUMP_KEY( KEY ": " ); c_tid_dump( (TID), stdout ); )

/**
 * Dumps a \ref c_type.
 *
 * @param KEY The key name to print.
 * @param TYPE The \ref c_type to dump.
 *
 * @sa #DUMP_TID()
 */
#define DUMP_TYPE(KEY,TYPE) IF_DEBUG( \
  DUMP_KEY( KEY ": " ); c_type_dump( &(TYPE), stdout ); )

/** @} */

/**
 * Shorthand for calling unsupported() with \a LANG_MACRO.
 *
 * @param LANG_MACRO A `LANG_*` macro without the `LANG_` prefix.
 *
 * @sa unsupported()
 */
#define UNSUPPORTED(LANG_MACRO)   unsupported( LANG_##LANG_MACRO )

///////////////////////////////////////////////////////////////////////////////

/**
 * @addtogroup parser-group
 * @{
 */

/**
 * Inherited attributes.
 */
struct in_attr {
  c_alignas_t   align;            ///< Alignment, if any.
  unsigned      ast_depth;        ///< Parentheses nesting depth.
  c_sname_t     current_scope;    ///< C++ only: current scope, if any.
  bool          is_implicit_int;  ///< Created implicit `int` AST?
  bool          is_typename;      ///< C++ only: `typename` specified?
  c_operator_t const *operator;   ///< C++ only: current operator, if any.
  c_ast_list_t  type_ast_stack;   ///< Type AST stack.
  c_ast_t      *typedef_ast;      ///< AST of `typedef` being declared.
  c_ast_list_t  typedef_ast_list; ///< AST nodes of `typedef` being declared.
  rb_node_t    *typedef_rb;       ///< Red-black node for temporary `typedef`.
};
typedef struct in_attr in_attr_t;

/**
 * Information for show_type_visitor().
 */
struct show_type_info {
  unsigned  show_which;                 ///< Predefined, user, or both?
  unsigned  gib_flags;                  ///< Gibberish flags.
  c_sglob_t sglob;                      ///< Scoped glob to match, if any.
};
typedef struct show_type_info show_type_info_t;

// local functions
NODISCARD
static bool c_ast_free_if_placeholder( c_ast_t* );

PJL_PRINTF_LIKE_FUNC(4)
static void fl_elaborate_error( char const*, int, dym_kind_t, char const*,
                                ... );

// local variables
static c_ast_list_t   decl_ast_list;    ///< List of ASTs being declared.
static c_ast_list_t   gc_ast_list;      ///< c_ast nodes freed after parse.
static in_attr_t      in_attr;          ///< Inherited attributes.
static c_ast_list_t   typedef_ast_list; ///< List of ASTs for `typedef`s.

////////// inline functions ///////////////////////////////////////////////////

/**
 * Garbage-collect the AST nodes on \a ast_list but does _not_ free \a ast_list
 * itself.
 *
 * @param ast_list The AST list to free the nodes of.
 *
 * @sa c_ast_list_cleanup()
 * @sa c_ast_new_gc()
 * @sa c_ast_pair_new_gc()
 */
static inline void c_ast_list_gc( c_ast_list_t *ast_list ) {
  slist_cleanup( ast_list, POINTER_CAST( slist_free_fn_t, &c_ast_free ) );
}

/**
 * Creates a new AST and adds it to \ref gc_ast_list.
 *
 * @param kind The kind of AST to create.
 * @param loc A pointer to the token location data.
 * @return Returns a pointer to a new AST.
 *
 * @sa c_ast_pair_new_gc()
 */
NODISCARD
static inline c_ast_t* c_ast_new_gc( c_ast_kind_t kind, c_loc_t const *loc ) {
  return c_ast_new( kind, in_attr.ast_depth, loc, &gc_ast_list );
}

/**
 * Set our mode to deciphering gibberish into English.
 */
static inline void gibberish_to_english( void ) {
  cdecl_mode = CDECL_GIBBERISH_TO_ENGLISH;
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
  return slist_pop_front( &in_attr.type_ast_stack );
}

/**
 * Pushes a type AST onto the
 * \ref in_attr.type_ast_stack "type AST inherited attribute  stack".
 *
 * @param ast The AST to push.
 *
 * @sa ia_type_ast_peek()
 * @sa ia_type_ast_pop()
 */
static inline void ia_type_ast_push( c_ast_t *ast ) {
  slist_push_front( &in_attr.type_ast_stack, ast );
}

/**
 * Cleans-up all memory associated with \a sti but does _not_ free \a sti
 * itself.
 *
 * @param sti The \ref show_type_info to clean up.
 *
 * @sa sti_init()
 */
static inline void sti_cleanup( show_type_info_t *sti ) {
  c_sglob_cleanup( &sti->sglob );
  MEM_ZERO( sti );
}

/**
 * Checks if the current language is _not_ among \a lang_ids.
 *
 * @param lang_ids The bitwise-or of language(s).
 * @return Returns `true` only if **cdecl** has been initialized and \ref
 * opt_lang is _not_ among \a lang_ids.
 *
 * @sa #UNSUPPORTED()
 */
NODISCARD
static inline bool unsupported( c_lang_id_t lang_ids ) {
  return !opt_lang_is_any( lang_ids ) && cdecl_initialized;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Adds a type to the global set.
 *
 * @param type_ast The AST of the type to add.
 * @param gib_flags The gibberish flags to use; must only be one of
 * #C_GIB_NONE, #C_GIB_TYPEDEF, or #C_GIB_USING.
 * @return Returns `true` either if the type was added or it's equivalent to an
 * existing type; `false` if a different type already exists having the same
 * name.
 */
NODISCARD
static bool add_type( c_ast_t const *type_ast, unsigned gib_flags ) {
  assert( type_ast != NULL );

  c_ast_t const *const leaf_ast = c_ast_leaf( type_ast );
  if ( c_ast_is_tid_any( leaf_ast, TB_AUTO ) ) {
    print_error( &leaf_ast->loc,
      "\"%s\" illegal in type definition\n",
      c_type_name_error( &leaf_ast->type )
    );
    return false;
  }

  rb_node_t const *const typedef_rb = c_typedef_add( type_ast, gib_flags );
  c_typedef_t const *const tdef = typedef_rb->data;

  if ( tdef->ast == type_ast ) {
    //
    // Type was added: we have to move the AST from the gc_ast_list so it won't
    // be garbage collected at the end of the parse to a separate
    // typedef_ast_list that's freed only at program termination.
    //
    // But first, free all orphaned placeholder AST nodes.  (For a non-type-
    // defining parse, this step isn't necessary since all nodes are freed at
    // the end of the parse anyway.)
    //
    slist_free_if(
      &gc_ast_list, POINTER_CAST( slist_pred_fn_t, &c_ast_free_if_placeholder )
    );
    slist_push_list_back( &typedef_ast_list, &gc_ast_list );
  }
  else {
    //
    // Type was NOT added because a previously declared type having the same
    // name was returned: check if the types are equal.
    //
    // In C, multiple typedef declarations having the same name are allowed
    // only if the types are equivalent:
    //
    //      typedef int T;
    //      typedef int T;                // OK
    //      typedef double T;             // error: types aren't equivalent
    //
    if ( !c_ast_equal( type_ast, tdef->ast ) ) {
      print_error( &type_ast->loc,
        "\"%s\": type redefinition with different type; original is: ",
        c_sname_full_name( &type_ast->sname )
      );
      print_type( tdef, stderr );
      return false;
    }
  }

  return true;
}

/**
 * Prints a warning that the attribute \a keyword syntax is not supported (and
 * ignored).
 *
 * @param loc The source location of \a keyword.
 * @param keyword The attribute syntax keyword, e.g., `__attribute__` or
 * `__declspec`.
 */
static void attr_syntax_not_supported( c_loc_t const *loc,
                                       char const *keyword ) {
  assert( loc != NULL );
  assert( keyword != NULL );

  print_warning( loc,
    "\"%s\" not supported by %s (ignoring)", keyword, CDECL
  );
  if ( OPT_LANG_IS( ATTRIBUTES ) )
    print_hint( "[[...]]" );
  else
    EPUTC( '\n' );
}

/**
 * A predicate function for slist_free_if() that checks whether \a ast is a
 * #K_PLACEHOLDER: if so, c_ast_free()s it.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast is a #K_PLACEHOLDER.
 */
static bool c_ast_free_if_placeholder( c_ast_t *ast ) {
  if ( ast->kind == K_PLACEHOLDER ) {
    assert( c_ast_is_orphan( ast ) );
    c_ast_free( ast );
    return true;
  }
  return false;
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
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param type_loc The location of the type declaration.
 * @return Returns `true` only if the type currently being declared is either
 * not nested or the current language is C++.
 */
NODISCARD
static bool fl_is_nested_type_ok( char const *file, int line,
                                  c_loc_t const *type_loc ) {
  assert( type_loc != NULL );
  if ( !c_sname_empty( &in_attr.current_scope ) && OPT_LANG_IS( C_ANY ) ) {
    fl_print_error( file, line, type_loc, "nested types not supported in C\n" );
    return false;
  }
  return true;
}

/**
 * A special case of fl_elaborate_error() that prevents oddly worded error
 * messages where a C/C++ keyword is expected, but that keyword isn't a keyword
 * either until a later version of the language or in a different language;
 * hence, the lexer will return the keyword as the `Y_NAME` token instead of
 * the keyword token.
 *
 * For example, if fl_elaborate_error() were used for the following \b cdecl
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
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param keyword A keyword literal.
 *
 * @sa fl_elaborate_error()
 * @sa fl_punct_expected()
 * @sa yyerror()
 */
static void fl_keyword_expected( char const *file, int line,
                                 char const *keyword ) {
  assert( keyword != NULL );

  dym_kind_t dym_kinds = DYM_NONE;

  char const *const error_token = lexer_printable_token();
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
          EPRINTF( ": \"%s\" not supported%s\n", keyword, which_lang );
          return;
        }
      }
    }

    dym_kinds = cdecl_mode == CDECL_ENGLISH_TO_GIBBERISH ?
      DYM_CDECL_KEYWORDS : DYM_C_KEYWORDS;
  }

  fl_elaborate_error( file, line, dym_kinds, "\"%s\" expected", keyword );
}

/**
 * A special case of fl_elaborate_error() that prevents oddly worded error
 * messages when a punctuation character is expected by not doing keyword look-
 * ups of the error token.

 * For example, if fl_elaborate_error() were used for the following \b cdecl
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
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param punct The punctuation character that was expected.
 *
 * @sa fl_elaborate_error()
 * @sa fl_keyword_expected()
 * @sa yyerror()
 */
static void fl_punct_expected( char const *file, int line, char punct ) {
  EPUTS( ": " );
  print_debug_file_line( file, line );

  char const *const error_token = lexer_printable_token();
  if ( error_token != NULL )
    EPRINTF( "\"%s\": ", error_token );

  EPRINTF( "'%c' expected\n", punct );
}

/**
 * Cleans-up all resources used by \ref in_attr "inherited attributes".
 */
static void ia_cleanup( void ) {
  c_sname_cleanup( &in_attr.current_scope );
  // Do _not_ pass &c_ast_free for the 2nd argument! All AST nodes were already
  // free'd from the gc_ast_list in parse_cleanup(). Just free the slist nodes.
  slist_cleanup( &in_attr.type_ast_stack, /*free_fn=*/NULL );
  c_ast_list_gc( &in_attr.typedef_ast_list );
  MEM_ZERO( &in_attr );
}

/**
 * Joins \a type_ast and \a decl_ast into a single AST.
 *
 * @param type_ast The type AST.
 * @param decl_ast The declaration AST.
 * @return Returns the joined AST on success or NULL on error.
 */
NODISCARD
c_ast_t const* join_type_decl( c_ast_t *type_ast, c_ast_t *decl_ast ) {
  assert( type_ast != NULL );
  assert( decl_ast != NULL );

  if ( in_attr.is_typename && !c_ast_is_typename_ok( type_ast ) )
    return NULL;

  c_type_t type = c_ast_take_type_any( type_ast, &T_TS_TYPEDEF );

  if ( c_tid_is_any( type.stids, TS_TYPEDEF ) && decl_ast->kind == K_TYPEDEF ) {
    //
    // This is for a case like:
    //
    //      explain typedef int int32_t;
    //
    // that is: explaining an existing typedef.  In order to do that, we have
    // to un-typedef it so we explain the type that it's a typedef for.
    //
    c_ast_t const *const raw_ast = c_ast_untypedef( decl_ast );

    //
    // However, we also have to check whether the typedef being explained is
    // not equivalent to the existing typedef.  This is for a case like:
    //
    //      explain typedef char int32_t;
    //
    if ( !c_ast_equal( type_ast, raw_ast ) ) {
      print_error( &decl_ast->loc,
        "\"%s\": type redefinition with different type; original is: ",
        c_sname_full_name( &raw_ast->sname )
      );
      // Look-up the type so we can print it how it was originally defined.
      c_typedef_t const *const tdef = c_typedef_find_sname( &raw_ast->sname );
      assert( tdef != NULL );
      print_type( tdef, stderr );
      return NULL;
    }

    //
    // Because the raw_ast for the existing type is about to be combined with
    // type_ast, duplicate raw_ast first.
    //
    decl_ast = c_ast_dup( raw_ast, &gc_ast_list );
  }

  c_ast_t *const ast = c_ast_patch_placeholder( type_ast, decl_ast );
  c_type_t const tdef_type = c_ast_take_type_any( ast, &T_TS_TYPEDEF );
  c_type_or_eq( &type, &tdef_type );
  c_type_or_eq( &ast->type, &type );

  if ( in_attr.align.kind != C_ALIGNAS_NONE ) {
    ast->align = in_attr.align;
    if ( c_tid_is_any( type.stids, TS_TYPEDEF ) ) {
      //
      // We check for illegal aligned typedef here rather than in c_ast_check.c
      // because the "typedef-ness" needed to be removed previously before the
      // eventual call to c_ast_check().
      //
      print_error( &ast->align.loc, "typedef can not be aligned\n" );
      return NULL;
    }
  }

  if ( ast->kind == K_USER_DEF_CONVERSION &&
       c_sname_local_type( &ast->sname )->btids == TB_SCOPE ) {
    //
    // User-defined conversions don't have names, but they can still have a
    // scope.  Since only classes can have them, if the scope is still
    // TB_SCOPE, change it to TB_CLASS.
    //
    c_sname_set_local_type( &ast->sname, &C_TYPE_LIT_B( TB_CLASS ) );
  }

  return ast;
}

/**
 * Cleans up individial parse data after each parse.
 *
 * @param fatal_error Must be `true` only if a fatal semantic error has
 * occurred and `YYABORT` is about to be called to bail out of parsing by
 * returning from yyparse().
 */
static void parse_cleanup( bool fatal_error ) {
  cdecl_mode = CDECL_ENGLISH_TO_GIBBERISH;

  //
  // We need to reset the lexer differently depending on whether we completed a
  // parse with a fatal error.  If so, do a "hard" reset that also resets the
  // EOF flag of the lexer.
  //
  lexer_reset( /*hard_reset=*/fatal_error );

  // Do _not_ pass &c_ast_free for the 2nd argument! All AST nodes will be
  // free'd from the gc_ast_list below. Just free the slist nodes.
  slist_cleanup( &decl_ast_list, /*free_fn=*/NULL );

  c_ast_list_gc( &gc_ast_list );
  ia_cleanup();
}

/**
 * Implements the **cdecl** `quit` command.
 *
 * @note This should be marked `noreturn` but isn't since that would generate a
 * warning that a `break` in the Bison-generated code won't be executed.
 */
static void quit( void ) {
  exit( EX_OK );
}

/**
 * Shows (prints) the definition of \a tdef.
 *
 * @param tdef The \ref c_typedef to show.
 * @param gib_flags The gibberish flags to use.
 */
static void show_type( c_typedef_t const *tdef, unsigned gib_flags ) {
  assert( tdef != NULL );
  if ( gib_flags == C_GIB_NONE ) {
    c_typedef_english( tdef, stdout );
  } else {
    if ( opt_semicolon )
      gib_flags |= C_GIB_FINAL_SEMI;
    c_typedef_gibberish( tdef, gib_flags, stdout );
  }
  PUTC( '\n' );
}

/**
 * A visitor function to show (print) \a tdef.
 *
 * @param tdef The \ref c_typedef to show.
 * @param data Optional data passed to the visitor: in this case, a \ref
 * show_type_info.
 * @return Always returns `false`.
 */
NODISCARD
static bool show_type_visitor( c_typedef_t const *tdef, void *data ) {
  assert( tdef != NULL );
  assert( data != NULL );

  show_type_info_t const *const sti = data;

  bool const show_in_lang =
    (sti->show_which & SHOW_ALL_TYPES) != 0 ||
    opt_lang_is_any( tdef->lang_ids );

  if ( show_in_lang ) {
    bool const show_it =
      ((tdef->is_predefined ?
        (sti->show_which & SHOW_PREDEFINED_TYPES  ) != 0 :
        (sti->show_which & SHOW_USER_DEFINED_TYPES) != 0)) &&
      (c_sglob_empty( &sti->sglob ) ||
       c_sname_match( &tdef->ast->sname, &sti->sglob ));

    if ( show_it )
      show_type( tdef, sti->gib_flags );
  }

  return /*stop=*/false;
}

/**
 * Initializes a show_type_info_t.
 *
 * @param sti The \ref show_type_info to initialize.
 * @param show_which Which types to show: predefined, user, or both?
 * @param glob The glob string; may be NULL.
 * @param gib_flags The gibberish flags to use.
 *
 * @sa sti_cleanup()
 */
static void sti_init( show_type_info_t *sti, unsigned show_which,
                      char const *glob, unsigned gib_flags ) {
  assert( sti != NULL );
  sti->show_which = show_which;
  sti->gib_flags = gib_flags;
  c_sglob_init( &sti->sglob );
  c_sglob_parse( glob, &sti->sglob );
}

/**
 * Called by Bison to print a parsing error message to standard error.
 *
 * @remarks A custom error printing function via `%%define parse.error custom`
 * and `yyreport_syntax_error()` is not done because printing a (perhaps long)
 * list of all the possible expected tokens isn't helpful.
 * @par
 * It's also more flexible to be able to call one of #elaborate_error(),
 * #keyword_expected(), or #punct_expected() at the point of the error rather
 * than having a single function try to figure out the best type of error
 * message to print.
 *
 * @note A newline is _not_ printed since the error message will be appended to
 * by fl_elaborate_error().  For example, the parts of an error message are
 * printed by the functions shown:
 *
 *      42: syntax error: "int": "into" expected
 *      |--||----------||----------------------|
 *      |   |           |
 *      |   yyerror()   fl_elaborate_error()
 *      |
 *      print_loc()
 *
 * @param msg The error message to print.  Bison invariably passes `syntax
 * error`.
 *
 * @sa fl_elaborate_error()
 * @sa fl_keyword_expected()
 * @sa fl_punct_expected()
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
  c_ast_t            *ast;        // for the AST being built
  c_ast_list_t        ast_list;   // for declarations and function parameters
  c_ast_pair_t        ast_pair;   // for the AST being built
  c_cast_kind_t       cast_kind;  // C/C++ cast kind
  bool                flag;       // simple flag
  unsigned            flags;      // multipurpose bitwise flags
  cdecl_help_t        help;       // type of help to print
  int                 int_val;    // signed integer value
  char const         *literal;    // token L_* literal (for new-style casts)
  char               *name;       // identifier name, cf. sname
  c_func_mbr_t        mbr;        // member, non-member, or unspecified
  c_oper_id_t         oper_id;    // overloaded operator ID
  c_sname_t           sname;      // scoped identifier name, cf. name
  slist_t             sname_list; // c_sname_t list
  char               *str_val;    // quoted string value
  c_typedef_t const  *tdef;       // typedef
  c_tid_t             tid;        // built-ins, storage classes, & qualifiers
  c_type_t            type;       // complete type
  unsigned            uint_val;   // unsigned integer value
}

                    // cdecl commands
%token              Y_cast
//                  Y_class             // covered in C++
//                  Y_const             // covered in C89
%token              Y_const_ENG         // see comment in lexer.l
%token              Y_declare
%token              Y_define
%token              Y_dynamic
//                  Y_exit              // mapped to Y_quit by lexer
%token              Y_explain
%token              Y_help
//                  Y_inline            // covered in C99
//                  Y_namespace         // covered in C++
%token              Y_no
%token              Y_quit
%token              Y_reinterpret
%token              Y_set
%token              Y_show
//                  Y_static            // covered in K&R C
//                  Y_struct            // covered in K&R C
//                  Y_typedef           // covered in K&R C
//                  Y_union             // covered in K&R C
//                  Y_using             // covered in C++

                    // Pseudo-English
%token              Y_aligned
%token              Y_all
%token              Y_array
%token              Y_as
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
%token              Y_expression
%token              Y_floating
%token              Y_function
%token              Y_initialization
%token              Y_into
%token              Y_lambda
%token              Y_length
%token              Y_linkage
%token              Y_literal
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
%left               Y_COLON2              "::"
%left               Y_COLON2_STAR         "::*"
                    // C/C++ operators: precedence 16
%token              Y_PLUS2               "++"
%token              Y_MINUS2              "--"
%left                                     '(' ')'
                                          '[' ']'
                                          '.'
                    Y_ARROW               "->"
                    // C/C++ operators: precedence 15
%right              Y_AMPER           //  '&' -- also has alt. token "bitand"
                                          '*'
                    Y_EXCLAM          //  '!' -- also has alt. token "not"
                 // Y_UMINUS          //  '-' -- covered by '-' below
                 // Y_UPLUS           //  '+' -- covered by '+' below
                    Y_sizeof
                    Y_TILDE           //  '~' -- also has alt.token "compl"
                    // C/C++ operators: precedence 14
%left               Y_DOT_STAR            ".*"
                    Y_ARROW_STAR          "->*"
                    // C/C++ operators: precedence 13
%left                                 //  '*' -- covered by '*' above
                                          '/'
                                          '%'
                    // C/C++ operators: precedence 12
%left                                     '-'
                                          '+'
                    // C/C++ operators: precedence 11
%left               Y_LESS2               "<<"
                    Y_GREATER2            ">>"
                    // C/C++ operators: precedence 10
%left               Y_LESS_EQUAL_GREATER  "<=>"
                    // C/C++ operators: precedence 9
%left                                     '<'
                                          '>'
                    Y_LESS_EQUAL          "<="
                    Y_GREATER_EQUAL       ">="
                    // C/C++ operators: precedence 8
%left               Y_EQUAL2              "=="
                    Y_EXCLAM_EQUAL    //  "!=" -- also has alt. token "not_eq"
                    // C/C++ operators: precedence 7 (covered above)
%left               Y_bit_and         //  '&' -- covered by Y_AMPER
                    // C/C++ operators: precedence 6
%left               Y_CARET           //  '^' -- also has alt. token "xor"
                    // C/C++ operators: precedence 5
%left               Y_PIPE            //  '|' -- also has alt. token "bitor"
                    // C/C++ operators: precedence 4
%left               Y_AMPER2          //  "&&" -- also has alt. token "and"
                    // C/C++ operators: precedence 3
%left               Y_PIPE2           //  "||" -- also has alt. token "or"
                    // C/C++ operators: precedence 2
%right              Y_QMARK_COLON         "?:"
                                          '='
                    Y_PERCENT_EQUAL       "%="
                    Y_AMPER_EQUAL     //  "&=" -- also has alt. token "and_eq"
                    Y_STAR_EQUAL          "*="
                    Y_PLUS_EQUAL          "+="
                    Y_MINUS_EQUAL         "-="
                    Y_SLASH_EQUAL         "/="
                    Y_LESS2_EQUAL         "<<="
                    Y_GREATER2_EQUAL      ">>="
                    Y_CARET_EQUAL     //  "^=" -- also has alt. token "xor_eq"
                    Y_PIPE_EQUAL      //  "|=" -- also has alt. token "or_eq"
                    // C/C++ operators: precedence 1
%left                                     ','

                    // K&R C
%token  <tid>       Y_auto_STORAGE      // C version of "auto"
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

                    // C89
%token              Y_asm
%token  <tid>       Y_const
%token              Y_ELLIPSIS    "..." // for varargs
%token  <tid>       Y_enum
%token  <tid>       Y_signed
%token  <tid>       Y_void
%token  <tid>       Y_volatile

                    // C95
%token  <tid>       Y_wchar_t

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
%token  <tid>       Y_false             // for noexcept(false)
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
%token  <tid>       Y_true              // for noexcept(true)
%token              Y_try
%token              Y_typeid
%token  <flag>      Y_typename
%token  <tid>       Y_using
%token  <tid>       Y_virtual

                    // C11 & C++11
%token  <tid>       Y_char16_t
%token  <tid>       Y_char32_t

                    // C23
%token  <tid>       Y__BitInt
%token  <tid>       Y_reproducible
%token              Y_typeof
%token              Y_typeof_unqual
%token  <tid>       Y_unsequenced

                    // C23 & C++11
%token              Y_ATTR_BEGIN        // First '[' of "[[" for an attribute.

                    // C++11
%token              Y_alignas
%token              Y_alignof
%token  <tid>       Y_auto_TYPE         // C23 & C++11 version of "auto"
%token              Y_carries Y_dependency
%token  <tid>       Y_carries_dependency
%token  <tid>       Y_constexpr
%token              Y_decltype
%token              Y_except
%token  <tid>       Y_final
%token  <tid>       Y_noexcept
%token              Y_nullptr
%token  <tid>       Y_override
%token              Y_QUOTE2            // for user-defined literals
%token              Y_static_assert
%token  <tid>       Y_thread_local

                    // C23 & C++14
%token  <tid>       Y_deprecated

                    // C23 & C++17
%token              Y_discard
%token  <tid>       Y_maybe_unused
%token              Y_maybe Y_unused
%token  <tid>       Y_nodiscard
%token  <tid>       Y_noreturn

                    // C23 & C++20
%token  <tid>       Y_char8_t

                    // C++20
%token              Y_address
%token              Y_concept
%token  <tid>       Y_consteval
%token  <tid>       Y_constinit
%token              Y_co_await
%token              Y_co_return
%token              Y_co_yield
%token  <tid>       Y_export
%token  <tid>       Y_no_unique_address
%token              Y_requires
%token              Y_unique

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
%token  <str_val>   Y_CHAR_LIT
%token              Y_END
%token              Y_ERROR
%token  <name>      Y_GLOB
%token  <int_val>   Y_INT_LIT
%token  <name>      Y_NAME
%token  <name>      Y_SET_OPTION
%token  <str_val>   Y_STR_LIT
%token  <tdef>      Y_TYPEDEF_NAME      // e.g., size_t
%token  <tdef>      Y_TYPEDEF_SNAME     // e.g., std::string

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
//      + <int_val>: "_int" is appended.
//      + <literal>: "_literal" is appended.
//      + <sname>: "_sname" is appended.
//      + <tid>: "_[bsa]tid" is appended.
//      + <type>: "_type" is appended.
//      + <uint_val>: "_uint" is appended.
//  4. Is expected, "_exp" is appended; is optional, "_opt" is appended.
//
// Grammar rules still use traditional positional references ($n, $$, @n, and
// @$) instead of named references because older versions of Bison don't
// support them.
//
// However, to mitigate having to use positional references, blocks of C code
// for rules having a lot of them start by assigning them to named local
// variables, for example:
//
//      array_decl_c_astp
//        : // in_attr: type_c_ast
//          decl2_c_astp array_size_c_ast gnu_attribute_specifier_list_c_opt
//          {
//            c_ast_t *const type_ast = ia_type_ast_peek();
//            c_ast_t *const decl_ast = $1.ast;
//            c_ast_t *const array_ast = $2;
//            ...
//
// However, this is not done for references of either slist_t or c_sname_t. For
// example, this is not done:
//
//            c_sname_t sname = $3;
//            ...
//            ast->sname = c_sname_move( &sname );
//
// The problem is that if a parse error occurs, $2 will still refer to the
// sname and Bison will call the %destructor for it that will result in the
// same sname being free'd twice because it's also free'd when ast is garbage
// collected.  Note that moving twice like:
//
//            c_sname_t sname = c_sname_move( &$3 );
//            ...
//            ast->sname = c_sname_move( &sname );
//
// creates a different problem: if an error occurs between the two moves, the
// %destructor will run on $3 which is now empty and so not clean-up sname.

// Sort using: sort -bdfk3

                      // Pseudo-English
%type   <ast>         alignas_or_width_decl_english_ast
%type   <align>       alignas_specifier_english
%type   <ast>         array_decl_english_ast
%type   <ast>         array_size_decl_ast
%type   <tid>         attribute_english_atid
%type   <int_val>     BitInt_english_int
%type   <ast>         block_decl_english_ast
%type   <tid>         builtin_no_BitInt_english_btid
%type   <ast>         builtin_type_english_ast
%type   <ast>         capture_decl_english_ast
%type   <ast_list>    capture_decl_list_english capture_decl_list_english_opt
%type   <ast_list>    capturing_paren_capture_decl_list_english_opt
%type   <ast>         class_struct_union_english_ast
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
%type   <mbr>         member_or_non_member_opt
%type   <cast_kind>   new_style_cast_english
%type   <sname>       of_scope_english
%type   <sname>       of_scope_list_english of_scope_list_english_opt
%type   <ast>         of_type_enum_fixed_type_english_ast_opt
%type   <ast>         oper_decl_english_ast
%type   <ast_list>    param_decl_list_english param_decl_list_english_opt
%type   <ast_list>    paren_capture_decl_list_english
%type   <ast_list>    paren_param_decl_list_english
%type   <ast_list>    paren_param_decl_list_english_opt
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

                      // C/C++ casts
%type   <ast_pair>    array_cast_c_astp
%type   <ast_pair>    block_cast_c_astp
%type   <ast_pair>    cast_c_astp cast_c_astp_opt cast2_c_astp
%type   <ast_pair>    func_cast_c_astp
%type   <ast_pair>    nested_cast_c_astp
%type   <cast_kind>   new_style_cast_c
%type   <ast_pair>    pointer_cast_c_astp
%type   <ast_pair>    pointer_to_member_cast_c_astp
%type   <ast_pair>    reference_cast_c_astp

                      // C/C++ declarations
%type   <align>       alignas_specifier_c
%type   <sname>       any_sname_c any_sname_c_exp any_sname_c_opt
%type   <ast_pair>    array_decl_c_astp
%type   <ast>         array_size_c_ast
%type   <ast>         atomic_builtin_typedef_type_c_ast
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
%type   <ast_pair>    decl_c_astp decl2_c_astp
%type   <sname>       destructor_sname
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
%type   <tid>         func_ref_qualifier_c_stid_opt
%type   <ast_list>    lambda_param_c_ast_list_opt
%type   <ast>         lambda_return_type_c_ast_opt
%type   <tid>         linkage_stid
%type   <ast_pair>    nested_decl_c_astp
%type   <tid>         noexcept_c_stid_opt
%type   <ast_pair>    oper_decl_c_astp
%type   <sname>       oper_sname_c_opt
%type   <ast>         param_c_ast param_c_ast_exp
%type   <ast_list>    param_c_ast_list param_c_ast_list_exp param_c_ast_list_opt
%type   <ast>         pc99_pointer_type_c_ast
%type   <ast_pair>    pointer_decl_c_astp
%type   <ast_pair>    pointer_to_member_decl_c_astp
%type   <ast>         pointer_to_member_type_c_ast
%type   <ast>         pointer_type_c_ast
%type   <ast_pair>    reference_decl_c_astp
%type   <ast>         reference_type_c_ast
%type   <tid>         restrict_qualifier_c_stid
%type   <tid>         rparen_func_qualifier_list_c_stid_opt
%type   <sname>       sname_c sname_c_exp sname_c_opt
%type   <ast>         sname_c_ast
%type   <type>        storage_class_c_type
%type   <sname>       sub_scope_sname_c_opt
%type   <ast>         trailing_return_type_c_ast_opt
%type   <ast>         type_c_ast
%type   <sname>       typedef_sname_c
%type   <ast>         typedef_type_c_ast
%type   <ast>         typedef_type_decl_c_ast
%type   <type>        type_modifier_c_type
%type   <type>        type_modifier_list_c_type type_modifier_list_c_type_opt
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
%type   <tdef>        any_typedef
%type   <tid>         class_struct_btid class_struct_btid_opt
%type   <tid>         class_struct_union_btid class_struct_union_btid_exp
%type   <oper_id>     c_operator
%type   <tid>         cv_qualifier_stid cv_qualifier_list_stid_opt
%type   <tid>         enum_btids
%type   <tid>         eval_expr_init_stid
%type   <name>        glob glob_opt
%type   <help>        help_what_opt
%type   <tid>         inline_stid_opt
%type   <int_val>     int_lit_exp int_lit_opt
%type   <ast>         name_ast
%type   <name>        name_exp
%type   <name>        name_opt
%type   <tid>         namespace_btid_exp
%type   <sname>       namespace_sname_c namespace_sname_c_exp
%type   <type>        namespace_type
%type   <sname>       namespace_typedef_sname_c
%type   <tid>         noexcept_bool_stid_exp
%type   <flags>       predefined_or_user_flags_opt
%type   <str_val>     set_option_value_opt
%type   <flags>       show_format show_format_exp show_format_opt
%type   <flags>       show_which_types_flags_opt
%type   <tid>         static_stid_opt
%type   <str_val>     str_lit str_lit_exp
%type   <tid>         this_stid_opt
%type   <type>        type_modifier_base_type
%type   <flag>        typename_flag_opt
%type   <tid>         virtual_stid_exp virtual_stid_opt

//
// Bison %destructors.
//
// Clean-up of AST nodes is done via garbage collection using gc_ast_list.
//
%destructor { DTRACE; c_ast_list_cleanup( &$$ ); }    <ast_list>
%destructor { DTRACE; FREE( $$ ); }                   <name>
%destructor { DTRACE; c_sname_cleanup( &$$ ); }       <sname>
%destructor { DTRACE; c_sname_list_cleanup( &$$ ); }  <sname_list>
%destructor { DTRACE; FREE( $$ ); }                   <str_val>

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
  | explain_command semi_or_end
  | help_command semi_or_end
  | quit_command semi_or_end
  | scoped_command
  | set_command semi_or_end
  | show_command semi_or_end
  | template_command semi_or_end
  | typedef_command semi_or_end
  | using_command semi_or_end
  | semi_or_end                         // allows for blank lines
  | error
    {
      if ( lexer_printable_token() != NULL )
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
  : Y_cast sname_english_opt as_into_to_exp decl_english_ast
    {
      c_ast_t *const decl_ast = $4;

      DUMP_START( "cast_command",
                  "CAST sname_english_opt as_into_to_exp decl_english_ast" );
      DUMP_SNAME( "sname_english_opt", $2 );
      DUMP_AST( "decl_english_ast", decl_ast );
      DUMP_END();

      c_ast_t *const cast_ast = c_ast_new_gc( K_CAST, &@$ );
      cast_ast->sname = c_sname_move( &$2 );
      cast_ast->cast.kind = C_CAST_C;
      cast_ast->cast.to_ast = decl_ast;
      PARSE_ASSERT( c_ast_check( cast_ast ) );
      c_ast_gibberish( cast_ast, C_GIB_CAST, stdout );
    }

    /*
     * New C++-style cast.
     */
  | new_style_cast_english sname_english_exp as_into_to_exp
    decl_english_ast
    {
      c_cast_kind_t const cast_kind = $1;
      char const   *const cast_literal = c_cast_gibberish( cast_kind );
      c_ast_t      *const decl_ast = $4;

      DUMP_START( "cast_command",
                  "new_style_cast_english CAST sname_english_exp "
                  "as_into_to_exp decl_english_ast" );
      DUMP_STR( "new_style_cast_english", cast_literal );
      DUMP_SNAME( "sname_english_exp", $2 );
      DUMP_AST( "decl_english_ast", decl_ast );
      DUMP_END();

      if ( UNSUPPORTED( CPP_ANY ) ) {
        print_error( &@1,
          "%s not supported%s\n", cast_literal,
          C_LANG_WHICH( CPP_ANY )
        );
        PARSE_ABORT();
      }

      c_ast_t *const cast_ast = c_ast_new_gc( K_CAST, &@$ );
      cast_ast->sname = c_sname_move( &$2 );
      cast_ast->cast.kind = cast_kind;
      cast_ast->cast.to_ast = decl_ast;
      PARSE_ASSERT( c_ast_check( cast_ast ) );
      c_ast_gibberish( cast_ast, C_GIB_CAST, stdout );
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
  : Y_declare sname_list_english as_exp alignas_or_width_decl_english_ast
    {
      slist_t         sname_list = slist_move( &$2 );
      c_ast_t *const  decl_ast = $4;

      if ( decl_ast->kind == K_NAME ) {
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
        assert( !c_sname_empty( &decl_ast->sname ) );
        print_error_unknown_name( &@4, &decl_ast->sname );
        c_sname_list_cleanup( &sname_list );
        PARSE_ABORT();
      }

      DUMP_START( "declare_command",
                  "DECLARE sname_list_english AS "
                  "alignas_or_width_decl_english_ast" );
      DUMP_SNAME_LIST( "sname_list_english", sname_list );
      DUMP_AST( "alignas_or_width_decl_english_ast", decl_ast );

      decl_ast->loc = @2;

      DUMP_AST( "decl_english", decl_ast );
      DUMP_END();

      // To check the declaration, it needs a name: just dup the first one.
      c_sname_t temp_sname = c_sname_dup( slist_front( &sname_list ) );
      c_sname_set( &decl_ast->sname, &temp_sname );

      bool ok = c_ast_check( decl_ast );

      if ( ok ) {
        //
        // Ensure that none of the names aren't of a previously declared type:
        //
        //      cdecl> struct S
        //      cdecl> declare S as int // error: "S": previously declared
        //
        // This check is done now in the parser rather than later in the AST
        // since similar checks are also done here in the parser.
        //
        FOREACH_SLIST_NODE( sname_node, &sname_list ) {
          c_sname_t const *const sname = sname_node->data;
          c_typedef_t const *const tdef = c_typedef_find_sname( sname );
          if ( tdef != NULL ) {
            print_error( &decl_ast->loc,
              "\"%s\": previously declared as type: ",
              c_sname_full_name( sname )
            );
            print_type( tdef, stderr );
            ok = false;
            break;
          }
        } // for
      }

      if ( ok ) {
        unsigned gib_flags = C_GIB_DECL;
        if ( slist_len( &sname_list ) > 1 )
          gib_flags |= C_GIB_MULTI_DECL;
        bool const print_as_using = c_ast_print_as_using( decl_ast );
        if ( print_as_using && opt_semicolon ) {
          //
          // When declaring multiple types via the same "declare" as "using"
          // declarations, each type needs its own "using" declaration and
          // hence its own semicolon:
          //
          //      c++decl> declare I, J as type int
          //      using I = int;
          //      using J = int;
          //
          gib_flags |= C_GIB_FINAL_SEMI;
        }

        FOREACH_SLIST_NODE( sname_node, &sname_list ) {
          c_sname_t *const cur_sname = sname_node->data;
          c_sname_set( &decl_ast->sname, cur_sname );
          bool const is_last_sname = sname_node->next == NULL;
          if ( is_last_sname && opt_semicolon )
            gib_flags |= C_GIB_FINAL_SEMI;
          c_ast_gibberish( decl_ast, gib_flags, stdout );
          if ( is_last_sname )
            continue;
          if ( print_as_using ) {
            //
            // When declaring multiple types via the same "declare" as "using"
            // declarations, they need to be separated by newlines.  (The final
            // newine is handled below.)
            //
            PUTC( '\n' );
          }
          else {
            //
            // When declaring multiple types (not as "using" declarations) or
            // objects via the same "declare", the second and subsequent types
            // or objects must not have the type name printed -- and they also
            // need to be separated by commas.  For example, when printing:
            //
            //      cdecl> declare x, y as pointer to int
            //      int *x, *y;
            //
            // the gibberish for `y` must not print the `int` again.
            //
            gib_flags |= C_GIB_OMIT_TYPE;
            PUTS( ", " );
          }
        } // for
      }

      c_sname_list_cleanup( &sname_list );
      PARSE_ASSERT( ok );
      PUTC( '\n' );
    }

    /*
     * C++ overloaded operator declaration.
     */
  | Y_declare c_operator
    { //
      // This check is done now in the parser rather than later in the AST
      // since it yields a better error message since otherwise it would warn
      // that "operator" is a keyword in C++98 which skims right past the
      // bigger error that operator overloading isn't supported in C.
      //
      if ( UNSUPPORTED( CPP_ANY ) ) {
        print_error( &@2,
          "operator overloading not supported%s\n",
          C_LANG_WHICH( CPP_ANY )
        );
        PARSE_ABORT();
      }
      in_attr.operator = c_oper_get( $2 );
    }
    of_scope_list_english_opt as_exp oper_decl_english_ast
    {
      c_ast_t *const oper_ast = $6;

      DUMP_START( "declare_command",
                  "DECLARE c_operator of_scope_list_english_opt AS "
                  "oper_decl_english_ast" );
      DUMP_STR( "c_operator", oper_ast->oper.operator->literal );
      DUMP_SNAME( "of_scope_list_english_opt", $4 );
      DUMP_AST( "oper_decl_english_ast", oper_ast );

      c_sname_set( &oper_ast->sname, &$4 );
      oper_ast->loc = @2;

      DUMP_AST( "declare_command", oper_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( oper_ast ) );
      unsigned gib_flags = C_GIB_DECL;
      if ( opt_semicolon )
        gib_flags |= C_GIB_FINAL_SEMI;
      c_ast_gibberish( oper_ast, gib_flags, stdout );
      PUTC( '\n' );
    }

  /*
   * C++ lambda.
   */
  | Y_declare storage_class_subset_english_type_opt Y_lambda
    capturing_paren_capture_decl_list_english_opt
    paren_param_decl_list_english_opt returning_english_ast_opt
    {
      DUMP_START( "declare_command",
                  "DECLARE storage_class_subset_english_type_opt "
                  "LAMBDA capturing_paren_capture_decl_list_english_opt "
                  "paren_param_decl_list_english_opt"
                  "returning_english_ast_opt" );
      DUMP_TYPE( "storage_class_subset_english_type_opt", $2 );
      DUMP_AST_LIST( "capturing_paren_capture_decl_list_english_opt", $4 );
      DUMP_AST_LIST( "paren_param_decl_list_english_opt", $5 );
      DUMP_AST( "returning_english_ast_opt", $6 );

      c_ast_t *const lambda_ast = c_ast_new_gc( K_LAMBDA, &@$ );
      lambda_ast->type = $2;
      lambda_ast->lambda.capture_ast_list = slist_move( &$4 );
      lambda_ast->lambda.param_ast_list = slist_move( &$5 );
      c_ast_set_parent( $6, lambda_ast );

      DUMP_AST( "declare_command", lambda_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( lambda_ast ) );
      c_ast_gibberish( lambda_ast, C_GIB_DECL, stdout );
      PUTC( '\n' );
    }

    /*
     * C++ user-defined conversion operator declaration.
     */
  | Y_declare storage_class_subset_english_type_opt
    cv_qualifier_list_stid_opt user_defined conversion_exp operator_opt
    of_scope_list_english_opt returning_exp decl_english_ast
    {
      c_type_t const  storage_type = $2;
      c_tid_t  const  cv_qual_stid = $3;
      c_ast_t *const  ret_ast = $9;

      DUMP_START( "declare_command",
                  "DECLARE storage_class_subset_english_type_opt "
                  "cv_qualifier_list_stid_opt "
                  "USER-DEFINED CONVERSION OPERATOR "
                  "of_scope_list_english_opt "
                  "RETURNING decl_english_ast" );
      DUMP_TYPE( "storage_class_subset_english_type_opt", storage_type );
      DUMP_TID( "cv_qualifier_list_stid_opt", cv_qual_stid );
      DUMP_SNAME( "of_scope_list_english_opt", $7 );
      DUMP_AST( "decl_english_ast", ret_ast );

      c_ast_t *const udc_ast = c_ast_new_gc( K_USER_DEF_CONVERSION, &@$ );
      c_sname_set( &udc_ast->sname, &$7 );
      udc_ast->type = c_type_or( &storage_type, &C_TYPE_LIT_S( cv_qual_stid ) );
      c_ast_set_parent( ret_ast, udc_ast );

      DUMP_AST( "declare_command", udc_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( udc_ast ) );
      unsigned gib_flags = C_GIB_DECL;
      if ( opt_semicolon )
        gib_flags |= C_GIB_FINAL_SEMI;
      c_ast_gibberish( udc_ast, gib_flags, stdout );
      PUTC( '\n' );
    }

  | Y_declare error
    {
      if ( OPT_LANG_IS( CPP_ANY ) )
        elaborate_error( "name or operator expected" );
      else
        elaborate_error( "name expected" );
    }
  ;

alignas_or_width_decl_english_ast
  : decl_english_ast

  | decl_english_ast alignas_specifier_english
    {
      $$ = $1;
      $$->align = $2;
      $$->loc = @$;
    }

  | decl_english_ast width_specifier_english_uint
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since we need to use the builtin union member now.
      //
      if ( !c_ast_is_integral( $1 ) ) {
        print_error( &@2,
          "bit-fields can be only of integral %stypes\n",
          OPT_LANG_IS( enum_BITFIELDS ) ? "or enumeration " : ""
        );
        PARSE_ABORT();
      }

      $$ = $1;
      $$->loc = @$;
      $$->bit_field.bit_width = $2;
    }
  ;

alignas_specifier_english
  : aligned_english Y_INT_LIT bytes_opt
    {
      $$.kind = C_ALIGNAS_EXPR;
      $$.loc = @1;
      $$.expr = STATIC_CAST( unsigned, $2 );
    }
  | aligned_english decl_english_ast
    {
      $$.kind = C_ALIGNAS_TYPE;
      $$.loc = @1;
      $$.type_ast = $2;
    }
  | aligned_english error
    {
      MEM_ZERO( &$$ );
      $$.loc = @1;
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
  | Y_capturing paren_capture_decl_list_english
    {
      $$ = $2;
    }
  | '[' capture_decl_list_english_opt ']'
    {
      $$ = $2;
    }
  ;

paren_capture_decl_list_english
  : '[' capture_decl_list_english_opt ']'
    {
      $$ = $2;
    }
  | '(' capture_decl_list_english_opt ')'
    {
      $$ = $2;
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
  : capture_decl_list_english comma_exp capture_decl_english_ast
    {
      DUMP_START( "capture_decl_list_english",
                  "capture_decl_list_english ',' capture_decl_english_ast" );
      DUMP_AST_LIST( "capture_decl_list_english", $1 );
      DUMP_AST( "capture_decl_english_ast", $3 );

      $$ = $1;
      slist_push_back( &$$, $3 );

      DUMP_AST_LIST( "capture_decl_list_english", $$ );
      DUMP_END();
    }

  | capture_decl_english_ast
    {
      DUMP_START( "capture_decl_list_english",
                  "capture_decl_english_ast" );
      DUMP_AST( "capture_decl_english_ast", $1 );

      slist_init( &$$ );
      slist_push_back( &$$, $1 );

      DUMP_AST_LIST( "capture_decl_list_english", $$ );
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
  | Y_reference Y_to name_exp
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      c_sname_append_name( &$$->sname, $3 );
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
  : Y_width int_lit_exp bits_opt
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since we use 0 to mean "no bit-field."
      //
      if ( $2 == 0 ) {
        print_error( &@2, "bit-field width must be > 0\n" );
        PARSE_ABORT();
      }
      $$ = STATIC_CAST( unsigned, $2 );
    }
  ;

storage_class_subset_english_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | storage_class_subset_english_type_opt
    storage_class_subset_english_type
    {
      DUMP_START( "storage_class_subset_english_type_opt",
                  "storage_class_subset_english_type_opt "
                  "storage_class_subset_english_type" );
      DUMP_TYPE( "storage_class_subset_english_type_opt", $1 );
      DUMP_TYPE( "storage_class_subset_english_type", $2 );

      $$ = $1;
      PARSE_ASSERT( c_type_add( &$$, &$2, &@2 ) );

      DUMP_TYPE( "storage_class_subset_english_type_opt", $$ );
      DUMP_END();
    }
  ;

storage_class_subset_english_type
  : attribute_english_atid        { $$ = C_TYPE_LIT_A( $1 ); }
  | storage_class_subset_english_stid
    {
      $$ = C_TYPE_LIT_S( $1 );
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
  : Y_const_ENG eval_expr_init_stid
    {
      $$ = $2;
    }
  | Y_consteval
  | Y_constexpr
  | Y_constinit
  | Y_explicit
  | Y_export
  | Y_final
  | Y_friend
  | Y_inline
  | Y_mutable
  | Y_no Y_except                 { $$ = TS_NOEXCEPT; }
  | Y_noexcept
  | Y_override
  | Y_static
  | Y_throw
  | Y_virtual
  | Y_pure virtual_stid_exp       { $$ = TS_PURE_VIRTUAL | $2; }
  ;

/// define command ////////////////////////////////////////////////////////////

define_command
  : Y_define sname_english_exp as_exp decl_english_ast
    {
      c_ast_t *const ast = $4;

      DUMP_START( "define_command",
                  "DEFINE sname_english AS decl_english_ast" );
      DUMP_SNAME( "sname", $2 );
      DUMP_AST( "decl_english_ast", ast );

      c_sname_set( &ast->sname, &$2 );

      if ( ast->kind == K_NAME ) {      // see the comment in "declare_command"
        assert( !c_sname_empty( &ast->sname ) );
        print_error_unknown_name( &@4, &ast->sname );
        PARSE_ABORT();
      }

      //
      // Explicitly add TS_TYPEDEF to prohibit cases like:
      //
      //      define eint as extern int
      //      define rint as register int
      //      define sint as static int
      //      ...
      //
      // i.e., a defined type with a storage class.  Once the semantic checks
      // pass, remove the TS_TYPEDEF.
      //
      PARSE_ASSERT( c_type_add( &ast->type, &T_TS_TYPEDEF, &@4 ) );
      PARSE_ASSERT( c_ast_check( ast ) );
      PJL_IGNORE_RV( c_ast_take_type_any( ast, &T_TS_TYPEDEF ) );

      if ( c_tid_is_any( ast->type.btids, TB_ANY_SCOPE ) )
        c_sname_set_local_type( &ast->sname, &ast->type );
      c_sname_fill_in_namespaces( &ast->sname );

      DUMP_AST( "defined.ast", ast );
      DUMP_END();

      PARSE_ASSERT( c_sname_check( &ast->sname, &@2 ) );
      PARSE_ASSERT( add_type( ast, C_GIB_NONE ) );
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
  | explain pc99_pointer_decl_list_c

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
  | explain extern_linkage_c_stid_opt using_decl_c_ast
    {
      DUMP_START( "explain_command",
                  "EXPLAIN extern_linkage_c_stid_opt using_decl_c_ast" );
      DUMP_TID( "extern_linkage_c_stid_opt", $2 );
      DUMP_AST( "using_decl_c_ast", $3 );
      DUMP_END();

      PARSE_ASSERT( c_type_add_tid( &$3->type, $2, &@2 ) );
      PARSE_ASSERT( c_ast_check( $3 ) );
      c_ast_english( $3, stdout );
    }

    /*
     * If we match an sname, it means it wasn't an sname for a type (otherwise
     * we would have matched the "Common declaration" case above).
     */
  | explain sname_c
    {
      print_error_unknown_name( &@2, &$2 );
      c_sname_cleanup( &$2 );
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
  : Y_help help_what_opt          { print_help( $2 ); }
  ;

help_what_opt
  : /* empty */                   { $$ = CDECL_HELP_COMMANDS; }
  | Y_commands                    { $$ = CDECL_HELP_COMMANDS; }
  | Y_english                     { $$ = CDECL_HELP_ENGLISH;  }
  | Y_options                     { $$ = CDECL_HELP_OPTIONS;  }
  | error
    {
      elaborate_error( "\"commands\", \"english\", or \"options\" expected" );
    }
  ;

/// quit command //////////////////////////////////////////////////////////////

quit_command
  : Y_quit                        { quit(); }
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
  : Y_SET_OPTION set_option_value_opt
    {
      bool const ok = set_option( $1, &@1, $2, &@2 );
      free( $1 );
      free( $2 );
      PARSE_ASSERT( ok );
    }
  ;

set_option_value_opt
  : /* empty */                   { $$ = NULL; }
  | '=' Y_SET_OPTION              { $$ = $2; @$ = @2; }
  | '=' error
    {
      $$ = NULL;
      elaborate_error( "option value expected" );
    }
  ;

/// show command //////////////////////////////////////////////////////////////

show_command
  : Y_show any_typedef show_format_opt
    {
      DUMP_START( "show_command", "SHOW any_typedef show_format_opt" );
      DUMP_AST( "any_typedef.ast", $2->ast );
      DUMP_INT( "show_format_opt", $3 );
      DUMP_END();

      show_type( $2, $3 );
    }

  | Y_show any_typedef Y_as show_format_exp
    {
      DUMP_START( "show_command", "SHOW any_typedef AS show_format_exp" );
      DUMP_AST( "any_typedef.ast", $2->ast );
      DUMP_INT( "show_format_exp", $4 );
      DUMP_END();

      show_type( $2, $4 );
    }

  | Y_show show_which_types_flags_opt glob_opt show_format_opt
    {
      show_type_info_t sti;
      sti_init( &sti, $2, $3, $4 );
      c_typedef_visit( &show_type_visitor, &sti );
      sti_cleanup( &sti );
      free( $3 );
    }

  | Y_show show_which_types_flags_opt glob_opt Y_as show_format_exp
    {
      show_type_info_t sti;
      sti_init( &sti, $2, $3, $5 );
      c_typedef_visit( &show_type_visitor, &sti );
      sti_cleanup( &sti );
      free( $3 );
    }

  | Y_show Y_NAME
    {
      static char const *const TYPE_COMMANDS_KNR[] = {
        L_define, L_struct, L_typedef, L_union, NULL
      };
      static char const *const TYPE_COMMANDS_C[] = {
        L_define, L_enum, L_struct, L_typedef, L_union, NULL
      };
      static char const *const TYPE_COMMANDS_CPP_WITHOUT_USING[] = {
        L_class, L_define, L_enum, L_struct, L_typedef, L_union, NULL
      };
      static char const *const TYPE_COMMANDS_CPP_WITH_USING[] = {
        L_class, L_define, L_enum, L_struct, L_typedef, L_union, L_using, NULL
      };

      char const *const *const type_commands =
        OPT_LANG_IS( C_KNR )             ? TYPE_COMMANDS_KNR :
        OPT_LANG_IS( C_ANY )             ? TYPE_COMMANDS_C :
        OPT_LANG_IS( using_DECLARATION ) ? TYPE_COMMANDS_CPP_WITH_USING :
                                           TYPE_COMMANDS_CPP_WITHOUT_USING;

      print_error( &@2, "\"%s\": not defined as type via ", $2 );
      fput_list( stderr, type_commands, /*gets=*/NULL );
      print_suggestions( DYM_C_TYPES, $2 );
      EPUTC( '\n' );
      free( $2 );
      PARSE_ABORT();
    }

  | Y_show error
    {
      elaborate_error(
        "type name or \"all\", \"predefined\", or \"user\" expected"
      );
    }
  ;

show_format
  : Y_english                     { $$ = C_GIB_NONE; }
  | Y_typedef                     { $$ = C_GIB_TYPEDEF; }
  | Y_using
    {
      if ( UNSUPPORTED( using_DECLARATION ) ) {
        print_error( &@1,
          "\"using\" not supported%s\n",
          C_LANG_WHICH( using_DECLARATION )
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
      if ( OPT_LANG_IS( using_DECLARATION ) )
        elaborate_error( "\"english\", \"typedef\", or \"using\" expected" );
      else
        elaborate_error( "\"english\" or \"typedef\" expected" );
    }
  ;

show_format_opt
  : /* empty */                   { $$ = C_GIB_NONE; }
  | show_format
  ;

show_which_types_flags_opt
  : /* empty */                   { $$ = SHOW_USER_DEFINED_TYPES; }
  | Y_all predefined_or_user_flags_opt
    {
      $$ = SHOW_ALL_TYPES |
           ($2 != 0 ? $2 : SHOW_PREDEFINED_TYPES | SHOW_USER_DEFINED_TYPES);
    }
  | Y_predefined                  { $$ = SHOW_PREDEFINED_TYPES; }
  | Y_user                        { $$ = SHOW_USER_DEFINED_TYPES; }
  ;

predefined_or_user_flags_opt
  : /* empty */                   { $$ = 0; }
  | Y_predefined                  { $$ = SHOW_PREDEFINED_TYPES; }
  | Y_user                        { $$ = SHOW_USER_DEFINED_TYPES; }
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
//  C/C++ casts                                                              //
///////////////////////////////////////////////////////////////////////////////

/// Gibberish C-style cast ////////////////////////////////////////////////////

c_style_cast_expr_c
  : '(' type_c_ast
    {
      ia_type_ast_push( $2 );
    }
    cast_c_astp_opt rparen_exp sname_c_opt
    {
      ia_type_ast_pop();

      c_ast_t *const  type_ast = $2;
      c_ast_t *const  decl_ast = $4.ast;

      DUMP_START( "explain_command",
                  "EXPLAIN '(' type_c_ast cast_c_astp_opt ')' sname_c_opt" );
      DUMP_AST( "type_c_ast", type_ast );
      DUMP_AST( "cast_c_astp_opt", decl_ast );
      DUMP_SNAME( "sname_c_opt", $6 );

      c_ast_t *const cast_ast = c_ast_new_gc( K_CAST, &@$ );
      cast_ast->sname = c_sname_move( &$6 );
      cast_ast->cast.kind = C_CAST_C;
      cast_ast->cast.to_ast = c_ast_patch_placeholder( type_ast, decl_ast );

      DUMP_AST( "explain_command", cast_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( cast_ast ) );
      c_ast_english( cast_ast, stdout );
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
  | '(' pc99_pointer_decl_list_c rparen_exp
  ;

/// Gibberish C++-style cast //////////////////////////////////////////////////

new_style_cast_expr_c
  : new_style_cast_c lt_exp type_c_ast
    {
      ia_type_ast_push( $3 );
    }
    cast_c_astp_opt gt_exp lparen_exp sname_c_exp rparen_exp
    {
      ia_type_ast_pop();

      c_cast_kind_t const cast_kind = $1;
      char const   *const cast_literal = c_cast_english( cast_kind );
      c_ast_t      *const type_ast = $3;
      c_ast_t      *const decl_ast = $5.ast;

      DUMP_START( "explain_command",
                  "EXPLAIN new_style_cast_c"
                  "'<' type_c_ast cast_c_astp_opt '>' '(' sname ')'" );
      DUMP_STR( "new_style_cast_c", cast_literal );
      DUMP_AST( "type_c_ast", type_ast );
      DUMP_AST( "cast_c_astp_opt", decl_ast );
      DUMP_SNAME( "sname", $8 );

      c_ast_t *const cast_ast = c_ast_new_gc( K_CAST, &@$ );
      cast_ast->sname = c_sname_move( &$8 );
      cast_ast->cast.kind = cast_kind;
      cast_ast->cast.to_ast = c_ast_patch_placeholder( type_ast, decl_ast );

      DUMP_AST( "explain_command", cast_ast );
      DUMP_END();

      if ( UNSUPPORTED( CPP_ANY ) ) {
        print_error( &@1,
          "%s_cast not supported%s\n",
          cast_literal, C_LANG_WHICH( CPP_ANY )
        );
        PARSE_ABORT();
      }

      PARSE_ASSERT( c_ast_check( cast_ast ) );
      c_ast_english( cast_ast, stdout );
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
  : alignas_specifier_c           { in_attr.align = $1; }
    typename_flag_opt             { in_attr.is_typename = $3; }
    typed_declaration_c
  ;

alignas_specifier_c
  : alignas lparen_exp Y_INT_LIT rparen_exp
    {
      DUMP_START( "alignas_specifier_c", "ALIGNAS '(' Y_INT_LIT ')'" );
      DUMP_INT( "INT_LIT", $3 );
      DUMP_END();

      $$.kind = C_ALIGNAS_EXPR;
      $$.loc = @1;
      $$.expr = STATIC_CAST( unsigned, $3 );
    }

  | alignas lparen_exp type_c_ast { ia_type_ast_push( $3 ); }
    cast_c_astp_opt rparen_exp
    {
      ia_type_ast_pop();

      c_ast_t *const  type_ast = $3;
      c_ast_t *const  cast_ast = $5.ast;

      DUMP_START( "alignas_specifier_c",
                  "ALIGNAS '(' type_c_ast cast_c_astp_opt ')'" );
      DUMP_AST( "type_c_ast", type_ast );
      DUMP_AST( "cast_c_astp_opt", cast_ast );
      DUMP_END();

      $$.kind = C_ALIGNAS_TYPE;
      $$.loc = @1;
      $$.type_ast = c_ast_patch_placeholder( type_ast, cast_ast );
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
  : Y_asm lparen_exp str_lit_exp rparen_exp
    {
      free( $3 );
      print_error( &@1, "asm declarations not supported by %s\n", CDECL );
      PARSE_ABORT();
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
  : class_struct_union_btid
    {
      PARSE_ASSERT( is_nested_type_ok( &@1 ) );
      gibberish_to_english();           // see the comment in "explain"
    }
    any_sname_c_exp
    {
      c_tid_t const csu_btid = $1;

      c_type_t const *const cur_type =
        c_sname_local_type( &in_attr.current_scope );
      if ( c_tid_is_any( cur_type->btids, TB_ANY_CLASS ) ) {
        char const *const cur_name =
          c_sname_local_name( &in_attr.current_scope );
        char const *const mbr_name = c_sname_local_name( &$3 );
        if ( strcmp( mbr_name, cur_name ) == 0 ) {
          print_error( &@3,
            "\"%s\": member has the same name as its enclosing %s\n",
            mbr_name, c_type_name_c( cur_type )
          );
          c_sname_cleanup( &$3 );
          PARSE_ABORT();
        }
      }

      DUMP_START( "class_struct_union_declaration_c",
                  "class_struct_union_btid sname '{' "
                  "in_scope_declaration_c_opt "
                  "'}' ';'" );
      DUMP_SNAME( "in_attr__current_scope", in_attr.current_scope );
      DUMP_TID( "class_struct_union_btid", csu_btid );
      DUMP_SNAME( "any_sname_c", $3 );

      c_sname_append_sname( &in_attr.current_scope, &$3 );
      c_sname_set_local_type(
        &in_attr.current_scope, &C_TYPE_LIT_B( csu_btid )
      );
      PARSE_ASSERT( c_sname_check( &in_attr.current_scope, &@3 ) );

      c_ast_t *const csu_ast = c_ast_new_gc( K_CLASS_STRUCT_UNION, &@3 );
      csu_ast->sname = c_sname_dup( &in_attr.current_scope );
      c_sname_append_name(
        &csu_ast->csu.csu_sname,
        check_strdup( c_sname_local_name( &in_attr.current_scope ) )
      );
      csu_ast->type.btids = c_tid_check( csu_btid, C_TPID_BASE );

      DUMP_AST( "class_struct_union_declaration_c", csu_ast );
      DUMP_END();

      PARSE_ASSERT( add_type( csu_ast, C_GIB_TYPEDEF ) );
    }
    brace_in_scope_declaration_c_opt
  ;

enum_declaration_c
    /*
     * C/C++ enum declaration, e.g.: enum E;
     */
  : enum_btids
    {
      PARSE_ASSERT( is_nested_type_ok( &@1 ) );
      gibberish_to_english();           // see the comment in "explain"
    }
    any_sname_c_exp enum_fixed_type_c_ast_opt
    {
      c_tid_t  const  enum_btids = $1;
      c_ast_t *const  fixed_type_ast = $4;

      DUMP_START( "enum_declaration_c", "enum_btids sname ';'" );
      DUMP_TID( "enum_btids", enum_btids );
      DUMP_SNAME( "any_sname_c", $3 );
      DUMP_AST( "enum_fixed_type_c_ast_opt", fixed_type_ast );

      c_sname_t enum_sname = c_sname_dup( &in_attr.current_scope );
      c_sname_append_sname( &enum_sname, &$3 );
      c_sname_set_local_type( &enum_sname, &C_TYPE_LIT_B( enum_btids ) );
      if ( !c_sname_check( &enum_sname, &@3 ) ) {
        c_sname_cleanup( &enum_sname );
        PARSE_ABORT();
      }

      c_ast_t *const enum_ast = c_ast_new_gc( K_ENUM, &@3 );
      enum_ast->sname = enum_sname;
      enum_ast->type.btids = c_tid_check( enum_btids, C_TPID_BASE );
      enum_ast->enum_.of_ast = fixed_type_ast;
      c_sname_append_name(
        &enum_ast->enum_.enum_sname,
        check_strdup( c_sname_local_name( &enum_sname ) )
      );

      DUMP_AST( "enum_declaration_c", enum_ast );
      DUMP_END();

      PARSE_ASSERT( add_type( enum_ast, C_GIB_TYPEDEF ) );
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
    namespace_sname_c_exp
    {
      c_type_t const ns_type = $1;

      DUMP_START( "namespace_declaration_c",
                  "namespace_type sname '{' "
                  "in_scope_declaration_c_opt "
                  "'}' [';']" );
      DUMP_SNAME( "in_attr__current_scope", in_attr.current_scope );
      DUMP_TYPE( "namespace_type", ns_type );
      DUMP_SNAME( "any_sname_c", $3 );
      DUMP_END();

      //
      // Nested namespace declarations are supported only in C++17 and later.
      // (However, we always allow them in configuration files.)
      //
      // This check has to be done now in the parser rather than later in the
      // AST because the AST has no "memory" of how a namespace was
      // constructed.
      //
      if ( c_sname_count( &$3 ) > 1 && UNSUPPORTED( NESTED_namespace ) ) {
        print_error( &@3,
          "nested namespace declarations not supported%s\n",
          C_LANG_WHICH( NESTED_namespace )
        );
        c_sname_cleanup( &$3 );
        PARSE_ABORT();
      }

      //
      // The namespace type (plain namespace or inline namespace) has to be
      // split across the namespace sname: only the storage classes (for
      // TS_INLINE) has to be or'd with the first scope-type of the sname ...
      //
      c_type_t const *const sname_first_type = c_sname_first_type( &$3 );
      c_sname_set_first_type(
        &$3,
        &C_TYPE_LIT(
          sname_first_type->btids,
          sname_first_type->stids | ns_type.stids,
          sname_first_type->atids
        )
      );
      //
      // ... and only the base types (for TB_NAMESPACE) has to be or'd with the
      // local scope type of the sname.
      //
      c_type_t const *const sname_local_type = c_sname_local_type( &$3 );
      c_sname_set_local_type(
        &$3,
        &C_TYPE_LIT(
          sname_local_type->btids | ns_type.btids,
          sname_local_type->stids,
          sname_local_type->atids
        )
      );

      c_sname_append_sname( &in_attr.current_scope, &$3 );
      PARSE_ASSERT( c_sname_check( &in_attr.current_scope, &@3 ) );
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
  : namespace_sname_c Y_COLON2 Y_NAME
    {
      DUMP_START( "namespace_sname_c", "sname_c '::' NAME" );
      DUMP_SNAME( "namespace_sname_c", $1 );
      DUMP_STR( "name", $3 );

      $$ = $1;
      c_sname_append_name( &$$, $3 );
      c_sname_set_local_type( &$$, &C_TYPE_LIT_B( TB_NAMESPACE ) );

      DUMP_SNAME( "namespace_sname_c", $$ );
      DUMP_END();
    }

  | namespace_sname_c Y_COLON2 any_typedef
    {
      DUMP_START( "namespace_sname_c", "namespace_sname_c '::' any_typedef" );
      DUMP_SNAME( "namespace_sname_c", $1 );
      DUMP_AST( "any_typedef.ast", $3->ast );

      $$ = $1;
      c_sname_set_local_type( &$$, &C_TYPE_LIT_B( TB_NAMESPACE ) );
      c_sname_t temp_sname = c_sname_dup( &$3->ast->sname );
      c_sname_append_sname( &$$, &temp_sname );

      DUMP_SNAME( "namespace_sname_c", $$ );
      DUMP_END();
    }

  | namespace_sname_c Y_COLON2 Y_inline name_exp
    {
      DUMP_START( "namespace_sname_c", "sname_c '::' NAME INLINE NAME" );
      DUMP_SNAME( "namespace_sname_c", $1 );
      DUMP_STR( "name", $4 );

      $$ = $1;
      c_sname_append_name( &$$, $4 );
      c_sname_set_local_type( &$$, &C_TYPE_LIT( TB_NAMESPACE, $3, TA_NONE ) );

      DUMP_SNAME( "namespace_sname_c", $$ );
      DUMP_END();
    }

  | Y_NAME
    {
      DUMP_START( "namespace_sname_c", "NAME" );
      DUMP_STR( "NAME", $1 );

      c_sname_init_name( &$$, $1 );
      c_sname_set_local_type( &$$, &C_TYPE_LIT_B( TB_NAMESPACE ) );

      DUMP_SNAME( "sname_c", $$ );
      DUMP_END();
    }
  ;

  /*
   * A version of typedef_sname_c for namespace declarations that has an extra
   * production for C++20 nested inline namespaces.  See typedef_sname_c for
   * detailed comments.
   */
namespace_typedef_sname_c
  : namespace_typedef_sname_c Y_COLON2 sname_c
    {
      DUMP_START( "namespace_typedef_sname_c",
                  "namespace_typedef_sname_c '::' sname_c" );
      DUMP_SNAME( "namespace_typedef_sname_c", $1 );
      DUMP_SNAME( "sname_c", $3 );

      $$ = $1;
      c_sname_append_sname( &$$, &$3 );

      DUMP_SNAME( "typedef_sname_c", $$ );
      DUMP_END();
    }

  | namespace_typedef_sname_c Y_COLON2 any_typedef
    {
      DUMP_START( "namespace_typedef_sname_c",
                  "namespace_typedef_sname_c '::' any_typedef" );
      DUMP_SNAME( "namespace_typedef_sname_c", $1 );
      DUMP_AST( "any_typedef", $3->ast );

      $$ = $1;
      c_sname_set_local_type( &$$, c_sname_local_type( &$3->ast->sname ) );
      c_sname_t temp_sname = c_sname_dup( &$3->ast->sname );
      c_sname_append_sname( &$$, &temp_sname );

      DUMP_SNAME( "typedef_sname_c", $$ );
      DUMP_END();
    }

  | namespace_typedef_sname_c Y_COLON2 Y_inline name_exp
    {
      DUMP_START( "namespace_typedef_sname_c",
                  "namespace_typedef_sname_c '::' INLINE NAME" );
      DUMP_SNAME( "namespace_typedef_sname_c", $1 );
      DUMP_STR( "name", $4 );

      $$ = $1;
      c_sname_append_name( &$$, $4 );
      c_sname_set_local_type( &$$, &C_TYPE_LIT( TB_NAMESPACE, $3, TA_NONE ) );

      DUMP_SNAME( "namespace_typedef_sname_c", $$ );
      DUMP_END();
    }

  | any_typedef                   { $$ = c_sname_dup( &$1->ast->sname ); }
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
  : '[' capture_decl_list_c_opt ']' lambda_param_c_ast_list_opt
    storage_class_subset_english_type_opt lambda_return_type_c_ast_opt
    {
      c_ast_t *const ret_ast = $6;
      c_type_t const type = $5;

      DUMP_START( "lambda_declaration_c",
                  "'[' capture_decl_list_c_opt ']' "
                  "lambda_param_c_ast_list_opt "
                  "lambda_return_type_c_ast_opt" );
      DUMP_AST_LIST( "capture_decl_list_c_opt", $2 );
      DUMP_AST_LIST( "lambda_param_c_ast_list_opt", $4 );
      DUMP_TYPE( "storage_class_subset_english_type_opt", type );
      DUMP_AST( "lambda_return_type_c_ast_opt", ret_ast );

      c_ast_t *const lambda_ast = c_ast_new_gc( K_LAMBDA, &@$ );
      lambda_ast->type = type;
      lambda_ast->lambda.capture_ast_list = slist_move( &$2 );
      lambda_ast->lambda.param_ast_list = slist_move( &$4 );
      c_ast_set_parent( ret_ast, lambda_ast );

      DUMP_AST( "lambda_declaration_c", lambda_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( lambda_ast ) );
      c_ast_english( lambda_ast, stdout );
    }
  ;

capture_decl_list_c_opt
  : /* empty */                   { slist_init( &$$ ); }
  | capture_decl_list_c
  ;

capture_decl_list_c
  : capture_decl_list_c comma_exp capture_decl_c_ast
    {
      DUMP_START( "capture_decl_list_c",
                  "capture_decl_list_c ',' "
                  "capture_decl_c_ast" );
      DUMP_AST_LIST( "capture_decl_list_c", $1 );
      DUMP_AST( "capture_decl_c_ast", $3 );

      $$ = $1;
      slist_push_back( &$$, $3 );

      DUMP_AST_LIST( "capture_decl_list_c", $$ );
      DUMP_END();
    }

  | capture_decl_c_ast
    {
      DUMP_START( "capture_decl_list_c",
                  "capture_decl_c_ast" );
      DUMP_AST( "capture_decl_c_ast", $1 );

      slist_init( &$$ );
      slist_push_back( &$$, $1 );

      DUMP_AST_LIST( "capture_decl_list_c", $$ );
      DUMP_END();
    }
  ;

capture_decl_c_ast
  : Y_AMPER
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      $$->capture.kind = C_CAPTURE_REFERENCE;
    }
  | Y_AMPER Y_NAME
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      c_sname_append_name( &$$->sname, $2 );
      $$->capture.kind = C_CAPTURE_REFERENCE;
    }
  | '='
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      $$->capture.kind = C_CAPTURE_COPY;
    }
  | Y_NAME
    {
      $$ = c_ast_new_gc( K_CAPTURE, &@$ );
      c_sname_append_name( &$$->sname, $1 );
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

lambda_param_c_ast_list_opt
  : /* empty */                   { slist_init( &$$ ); }
  | '(' param_c_ast_list_opt ')'  { $$ = $2; }
  ;

lambda_return_type_c_ast_opt
  : /* empty */                   { $$ = NULL; }
  | Y_ARROW type_c_ast { ia_type_ast_push( $2 ); } cast_c_astp_opt
    {
      ia_type_ast_pop();

      DUMP_START( "lambda_return_type_c_ast_opt",
                  "type_c_ast cast_c_astp_opt" );
      DUMP_AST( "type_c_ast", $2 );
      DUMP_AST( "cast_c_astp_opt", $4.ast );

      $$ = $4.ast != NULL ? $4.ast : $2;

      DUMP_AST( "lambda_return_type_c_ast_opt", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ template declaration ////////////////////////////////////////

template_declaration_c
  : Y_template
    {
      print_error( &@1, "template declarations not supported by %s\n", CDECL );
      PARSE_ABORT();
    }
  ;

/// Gibberish C/C++ typed declaration /////////////////////////////////////////

typed_declaration_c
  : type_c_ast { ia_type_ast_push( $1 ); } decl_list_c_opt
    {
      ia_type_ast_pop();
    }
  ;

/// Gibberish C/C++ typedef declaration ///////////////////////////////////////

typedef_declaration_c
  : Y_typedef typename_flag_opt
    {
      PARSE_ASSERT( is_nested_type_ok( &@1 ) );
      in_attr.is_typename = $2;
      gibberish_to_english();           // see the comment in "explain"
    }
    type_c_ast
    {
      c_ast_t *const type_ast = $4;

      PARSE_ASSERT( !in_attr.is_typename || c_ast_is_typename_ok( type_ast ) );
      // see the comment in "define_command" about TS_TYPEDEF
      PARSE_ASSERT( c_type_add_tid( &type_ast->type, TS_TYPEDEF, &@4 ) );

      //
      // We have to keep a pristine copy of the AST for the base type of the
      // typedef being declared in case multiple types are defined in the same
      // typedef.  For example, given:
      //
      //      typedef int I, *PI;
      //              ^
      // the "int" is the base type that needs to get patched twice to form two
      // types: I and PI.  Hence, we keep a pristine copy and then duplicate it
      // so every type gets a pristine copy.
      //
      assert( slist_empty( &in_attr.typedef_ast_list ) );
      slist_push_list_back( &in_attr.typedef_ast_list, &gc_ast_list );
      in_attr.typedef_ast = type_ast;
      ia_type_ast_push( c_ast_dup( in_attr.typedef_ast, &gc_ast_list ) );
    }
    typedef_decl_list_c
    {
      ia_type_ast_pop();
    }
  ;

typedef_decl_list_c
  : typedef_decl_list_c ','
    { //
      // We're defining another type in the same typedef so we need to replace
      // the current type AST inherited attribute with a new duplicate of the
      // pristine one we kept earlier.
      //
      ia_type_ast_pop();
      ia_type_ast_push( c_ast_dup( in_attr.typedef_ast, &gc_ast_list ) );
    }
    typedef_decl_c_exp

  | typedef_decl_c
  ;

typedef_decl_c
  : // in_attr: type_c_ast
    decl_c_astp
    {
      c_ast_t *const type_ast = ia_type_ast_peek();
      c_ast_t *const decl_ast = $1.ast;

      DUMP_START( "typedef_decl_c", "decl_c_astp" );
      DUMP_SNAME( "in_attr__current_scope", in_attr.current_scope );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST( "decl_c_astp", decl_ast );

      c_ast_t *typedef_ast;
      c_sname_t temp_sname;

      if ( decl_ast->kind == K_TYPEDEF ) {
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
      // see the comment in "define_command" about TS_TYPEDEF
      PJL_IGNORE_RV( c_ast_take_type_any( typedef_ast, &T_TS_TYPEDEF ) );

      if ( c_sname_count( &typedef_ast->sname ) > 1 ) {
        print_error( &@1,
          "typedef names can not be scoped; "
          "use: namespace %s { typedef ... }\n",
          c_sname_scope_name( &typedef_ast->sname )
        );
        PARSE_ABORT();
      }

      temp_sname = c_sname_dup( &in_attr.current_scope );
      c_sname_prepend_sname( &typedef_ast->sname, &temp_sname );

      DUMP_AST( "typedef_decl_c", typedef_ast );
      DUMP_END();

      PARSE_ASSERT( add_type( typedef_ast, C_GIB_TYPEDEF  ) );
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
  : user_defined_conversion_decl_c_astp
    {
      DUMP_START( "user_defined_conversion_declaration_c",
                  "user_defined_conversion_decl_c_astp" );
      DUMP_AST( "user_defined_conversion_declaration_c", $1.ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( $1.ast ) );
      c_ast_english( $1.ast, stdout );
    }
  ;

/// Gibberish C++ using declaration ///////////////////////////////////////////

using_declaration_c
  : using_decl_c_ast
    {
      // see the comment in "define_command" about TS_TYPEDEF
      PJL_IGNORE_RV( c_ast_take_type_any( $1, &T_TS_TYPEDEF ) );

      PARSE_ASSERT( add_type( $1, C_GIB_USING ) );
    }
  ;

using_decl_c_ast
  : Y_using
    {
      gibberish_to_english();           // see the comment in "explain"
    }
    any_name_exp attribute_specifier_list_c_atid_opt equals_exp type_c_ast
    {
      // see the comment in "define_command" about TS_TYPEDEF
      if ( !c_type_add_tid( &$6->type, TS_TYPEDEF, &@6 ) ) {
        free( $3 );
        PARSE_ABORT();
      }
      ia_type_ast_push( $6 );
    }
    cast_c_astp_opt
    {
      ia_type_ast_pop();

      char const *const name = $3;
      c_tid_t     const atids = $4;
      c_ast_t    *const type_ast = $6;
      c_ast_t    *const cast_ast = $8.ast;

      DUMP_START( "using_decl_c_ast",
                  "USING any_name_exp attribute_specifier_list_c_atid_opt '=' "
                  "type_c_ast cast_c_astp_opt" );
      DUMP_SNAME( "in_attr__current_scope", in_attr.current_scope );
      DUMP_STR( "any_name_exp", name );
      DUMP_TID( "attribute_specifier_list_c_atid_opt", atids );
      DUMP_AST( "type_c_ast", type_ast );
      DUMP_AST( "cast_c_astp_opt", cast_ast );

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
        FREE( name );
        PARSE_ABORT();
      }

      c_sname_t temp_sname = c_sname_dup( &in_attr.current_scope );
      c_sname_append_name( &temp_sname, name );

      $$ = c_ast_patch_placeholder( type_ast, cast_ast );
      c_sname_set( &$$->sname, &temp_sname );
      $$->type.atids = c_tid_check( atids, C_TPID_ATTR );

      DUMP_AST( "using_decl_c_ast", $$ );
      DUMP_END();

      //
      // Using declarations are supported only in C++11 and later.  (However,
      // we always allow them in configuration files.)
      //
      // This check has to be done now in the parser rather than later in the
      // AST because using declarations are treated like typedef declarations
      // and the AST has no "memory" that such a declaration was a using
      // declaration.
      //
      if ( UNSUPPORTED( using_DECLARATION ) ) {
        print_error( &@1,
          "\"using\" not supported%s\n",
          C_LANG_WHICH( using_DECLARATION )
        );
        PARSE_ABORT();
      }

      PARSE_ASSERT( c_ast_check( $$ ) );
    }
  ;

/// Gibberish C/C++ declarations //////////////////////////////////////////////

decl_list_c_opt
    /*
     * An enum, class, struct, or union (ECSU) declaration by itself, e.g.:
     *
     *      explain struct S
     *
     * without any object of that type.
     */
  : // in_attr: type_c_ast
    /* empty */
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "decl_list_c_opt", "<empty>" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST( "decl_list_c_opt", type_ast );

      if ( (type_ast->kind & K_ANY_ECSU) == 0 ) {
        //
        // The declaration is a non-ECSU type, e.g.:
        //
        //      int
        //
        c_loc_t const loc = lexer_loc();
        print_error( &loc, "declaration expected" );
        print_is_a_keyword( lexer_printable_token() );
        EPUTC( '\n' );
        PARSE_ABORT();
      }

      if ( in_attr.align.kind != C_ALIGNAS_NONE ) {
        print_error( &in_attr.align.loc, "%s invalid here\n", alignas_name() );
        PARSE_ABORT();
      }

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

      DUMP_AST( "decl_list_c_opt", type_ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( type_ast ) );
      c_typedef_english( &C_TYPEDEF_AST_LIT( type_ast ), stdout );
      PUTC( '\n' );
    }

  | decl_list_c
  ;

decl_list_c
  : decl_list_c ',' decl_c_exp
  | decl_c
  ;

decl_c
  : // in_attr: alignas_specifier_c typename_flag_opt type_c_ast
    decl_c_astp
    { //
      // The type has to be duplicated to guarantee a fresh type AST in case
      // we're doing multiple declarations, e.g.:
      //
      //    explain int *p, *q
      //
      c_ast_t *const type_ast = c_ast_dup( ia_type_ast_peek(), &gc_ast_list );
      c_ast_t *const decl_ast = $1.ast;

      DUMP_START( "decl_c", "decl_c_astp" );
      switch ( in_attr.align.kind ) {
        case C_ALIGNAS_NONE:
          break;
        case C_ALIGNAS_EXPR:
          DUMP_INT( "in_attr__alignas_specifier_c_expr", in_attr.align.expr );
          break;
        case C_ALIGNAS_TYPE:
          DUMP_AST(
            "in_attr__alignas_specifier_c_type_ast", in_attr.align.type_ast
          );
          break;
      } // switch
      DUMP_BOOL( "in_attr__typename_flag_opt", in_attr.is_typename );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST( "decl_c_astp", decl_ast );

      c_ast_t const *const ast = join_type_decl( type_ast, decl_ast );

      DUMP_AST( "decl_c", ast );
      DUMP_END();

      PARSE_ASSERT( ast != NULL );
      PARSE_ASSERT( c_ast_check( ast ) );

      if ( !c_sname_empty( &ast->sname ) ) {
        //
        // For declarations that have a name, ensure that it's not used more
        // than once in the same declaration in C++ or with different types in
        // C.  (In C, more than once with the same type are "tentative
        // definitions" and OK.)
        //
        //      int i, i;               // OK in C (same type); error in C++
        //      int j, *j;              // error (different types)
        //
        FOREACH_SLIST_NODE( node, &decl_ast_list ) {
          c_ast_t const *const prev_ast = node->data;
          if ( c_sname_cmp( &ast->sname, &prev_ast->sname ) != 0 )
            continue;
          if ( OPT_LANG_IS( CPP_ANY ) ) {
            print_error( &ast->loc,
              "\"%s\": redefinition\n",
              c_sname_full_name( &ast->sname )
            );
            PARSE_ABORT();
          }
          else if ( !c_ast_equal( ast, prev_ast ) ) {
            print_error( &ast->loc,
              "\"%s\": redefinition with different type\n",
              c_sname_full_name( &ast->sname )
            );
            PARSE_ABORT();
          }
        } // for
        slist_push_back( &decl_ast_list, CONST_CAST( void*, ast ) );
      }

      c_ast_english( ast, stdout );

      //
      // The type's AST takes on the name of the thing being declared, e.g.:
      //
      //      int x
      //
      // makes the type_ast (kind = "built-in type", type = "int") take on the
      // name of "x" so it'll be explained as:
      //
      //      declare x as int
      //
      // Once explained, the name must be cleared for the subsequent
      // declaration (if any) in the same declaration statement, e.g.:
      //
      //      int x, y
      //
      c_sname_cleanup( &type_ast->sname );
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
  | msc_calling_convention_atid msc_calling_convention_c_astp
    {
      $$ = $2;
      $$.ast->loc = @$;
      $$.ast->type.atids |= $1;
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
  | sname_c_ast gnu_attribute_specifier_list_c_opt
    {
      $$ = (c_ast_pair_t){ $1, NULL };
    }
  | typedef_type_decl_c_ast       { $$ = (c_ast_pair_t){ $1, NULL }; }
  | user_defined_conversion_decl_c_astp
  | user_defined_literal_decl_c_astp
  ;

/// Gibberish C/C++ array declaration /////////////////////////////////////////

array_decl_c_astp
  : // in_attr: type_c_ast
    decl2_c_astp array_size_c_ast gnu_attribute_specifier_list_c_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();
      c_ast_t *const decl_ast = $1.ast;
      c_ast_t *const array_ast = $2;

      DUMP_START( "array_decl_c_astp", "decl2_c_astp array_size_c_ast" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST( "decl2_c_astp", decl_ast );
      DUMP_AST( "target_ast", $1.target_ast );
      DUMP_AST( "array_size_c_ast", array_ast );

      c_ast_set_parent( c_ast_new_gc( K_PLACEHOLDER, &@1 ), array_ast );

      if ( $1.target_ast != NULL ) {    // array-of or function-like-ret type
        $$.ast = decl_ast;
        $$.target_ast = c_ast_add_array( $1.target_ast, array_ast, type_ast );
      } else {
        $$.ast = c_ast_add_array( decl_ast, array_ast, type_ast );
        $$.target_ast = NULL;
      }

      DUMP_AST( "array_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

array_size_c_ast
  : '[' rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_EMPTY_SIZE;
    }
  | '[' Y_INT_LIT rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_INT_SIZE;
      $$->array.size_int = $2;
    }
  | '[' Y_NAME rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_NAMED_SIZE;
      $$->array.size_name = $2;
    }
  | '[' type_qualifier_list_c_stid rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids = c_tid_check( $2, C_TPID_STORE );
      $$->array.kind = C_ARRAY_EMPTY_SIZE;
    }
  | '[' type_qualifier_list_c_stid static_stid_opt Y_INT_LIT rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids = c_tid_check( $2 | $3, C_TPID_STORE );
      $$->array.kind = C_ARRAY_INT_SIZE;
      $$->array.size_int = $4;
    }
  | '[' type_qualifier_list_c_stid static_stid_opt Y_NAME rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids = c_tid_check( $2 | $3, C_TPID_STORE );
      $$->array.kind = C_ARRAY_NAMED_SIZE;
      $$->array.size_name = $4;
    }
  | '[' type_qualifier_list_c_stid_opt '*' rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids = c_tid_check( $2, C_TPID_STORE );
      $$->array.kind = C_ARRAY_VLA_STAR;
    }
  | '[' Y_static type_qualifier_list_c_stid_opt Y_INT_LIT rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids = c_tid_check( TS_NON_EMPTY_ARRAY | $3, C_TPID_STORE );
      $$->array.kind = C_ARRAY_INT_SIZE;
      $$->array.size_int = $4;
    }
  | '[' Y_static type_qualifier_list_c_stid_opt Y_NAME rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->type.stids = c_tid_check( TS_NON_EMPTY_ARRAY | $3, C_TPID_STORE );
      $$->array.kind = C_ARRAY_NAMED_SIZE;
      $$->array.size_name = $4;
    }
  | '[' error ']'
    {
      elaborate_error( "array size expected" );
    }
  ;

/// Gibberish C/C++ block declaration (Apple extension) ///////////////////////

block_decl_c_astp                       // Apple extension
  : // in_attr: type_c_ast
    '(' Y_CARET
    { //
      // A block AST has to be the type inherited attribute for decl_c_astp so
      // we have to create it here.
      //
      ia_type_ast_push( c_ast_new_gc( K_APPLE_BLOCK, &@$ ) );
    }
    type_qualifier_list_c_stid_opt decl_c_astp rparen_exp
    lparen_exp param_c_ast_list_opt ')' gnu_attribute_specifier_list_c_opt
    {
      c_ast_t *const  block_ast = ia_type_ast_pop();
      c_ast_t *const  type_ast = ia_type_ast_peek();
      c_tid_t  const  type_qual_stids = $4;
      c_ast_t *const  decl_ast = $5.ast;

      DUMP_START( "block_decl_c_astp",
                  "'(' '^' type_qualifier_list_c_stid_opt decl_c_astp ')' "
                  "'(' param_c_ast_list_opt ')'" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", type_qual_stids );
      DUMP_AST( "decl_c_astp", decl_ast );
      DUMP_AST_LIST( "param_c_ast_list_opt", $8 );

      PARSE_ASSERT( c_type_add_tid( &block_ast->type, type_qual_stids, &@4 ) );
      block_ast->block.param_ast_list = slist_move( &$8 );
      $$.ast = c_ast_add_func( decl_ast, block_ast, type_ast );
      $$.target_ast = block_ast->block.ret_ast;

      DUMP_AST( "block_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish C++ in-class destructor declaration /////////////////////////////

destructor_declaration_c
    /*
     * C++ in-class destructor declaration, e.g.: ~S().
     */
  : virtual_stid_opt Y_TILDE any_name_exp
    lparen_exp rparen_func_qualifier_list_c_stid_opt noexcept_c_stid_opt
    gnu_attribute_specifier_list_c_opt func_equals_c_stid_opt
    {
      c_tid_t     const virtual_stid = $1;
      char const *const name = $3;
      c_tid_t     const func_qual_stids = $5;
      c_tid_t     const noexcept_stid = $6;
      c_tid_t     const func_equals_stid = $8;

      DUMP_START( "destructor_declaration_c",
                  "virtual_opt '~' NAME '(' ')' func_qualifier_list_c_stid_opt "
                  "noexcept_c_stid_opt gnu_attribute_specifier_list_c_opt "
                  "func_equals_c_stid_opt" );
      DUMP_TID( "virtual_stid_opt", virtual_stid );
      DUMP_STR( "any_name_exp", name );
      DUMP_TID( "func_qualifier_list_c_stid_opt", func_qual_stids );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );
      DUMP_TID( "func_equals_c_stid_opt", func_equals_stid );

      c_ast_t *const ast = c_ast_new_gc( K_DESTRUCTOR, &@$ );
      c_sname_append_name( &ast->sname, name );
      ast->type.stids = c_tid_check(
        virtual_stid | func_qual_stids | noexcept_stid | func_equals_stid,
        C_TPID_STORE
      );

      DUMP_AST( "destructor_declaration_c", ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( ast ) );
      c_ast_english( ast, stdout );
    }
  ;

/// Gibberish C++ file-scope constructor declaration //////////////////////////

file_scope_constructor_declaration_c
  : inline_stid_opt Y_CONSTRUCTOR_SNAME
    lparen_exp param_c_ast_list_opt rparen_func_qualifier_list_c_stid_opt
    noexcept_c_stid_opt gnu_attribute_specifier_list_c_opt
    {
      c_tid_t const inline_stid = $1;
      c_tid_t const func_qual_stids = $5;
      c_tid_t const noexcept_stid = $6;

      DUMP_START( "file_scope_constructor_declaration_c",
                  "inline_opt CONSTRUCTOR_SNAME '(' param_c_ast_list_opt ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt" );
      DUMP_TID( "inline_stid_opt", inline_stid );
      DUMP_SNAME( "CONSTRUCTOR_SNAME", $2 );
      DUMP_AST_LIST( "param_c_ast_list_opt", $4 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", func_qual_stids );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );

      c_sname_set_scope_type( &$2, &C_TYPE_LIT_B( TB_CLASS ) );

      c_ast_t *const ast = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      ast->sname = c_sname_move( &$2 );
      ast->type.stids = c_tid_check(
        inline_stid | func_qual_stids | noexcept_stid,
        C_TPID_STORE
      );
      ast->ctor.param_ast_list = slist_move( &$4 );

      DUMP_AST( "file_scope_constructor_declaration_c", ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( ast ) );
      c_ast_english( ast, stdout );
    }
  ;

/// Gibberish C++ file-scope destructor declaration ///////////////////////////

file_scope_destructor_declaration_c
  : inline_stid_opt destructor_sname
    lparen_exp rparen_func_qualifier_list_c_stid_opt noexcept_c_stid_opt
    gnu_attribute_specifier_list_c_opt
    {
      c_tid_t const inline_stid = $1;
      c_tid_t const func_qual_stids = $4;
      c_tid_t const noexcept_stid = $5;

      DUMP_START( "file_scope_destructor_declaration_c",
                  "inline_opt DESTRUCTOR_SNAME '(' ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt" );
      DUMP_TID( "inline_stid_opt", inline_stid );
      DUMP_SNAME( "DESTRUCTOR_SNAME", $2 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", func_qual_stids );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );

      c_sname_set_scope_type( &$2, &C_TYPE_LIT_B( TB_CLASS ) );

      c_ast_t *const ast = c_ast_new_gc( K_DESTRUCTOR, &@$ );
      ast->sname = c_sname_move( &$2 );
      ast->type.stids = c_tid_check(
        inline_stid | func_qual_stids | noexcept_stid,
        C_TPID_STORE
      );

      DUMP_AST( "file_scope_destructor_declaration_c", ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( ast ) );
      c_ast_english( ast, stdout );
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
    decl2_c_astp '(' param_c_ast_list_opt
    rparen_func_qualifier_list_c_stid_opt func_ref_qualifier_c_stid_opt
    noexcept_c_stid_opt trailing_return_type_c_ast_opt func_equals_c_stid_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();
      c_ast_t *const decl2_ast = $1.ast;
      c_tid_t  const func_qual_stid = $4;
      c_tid_t  const func_ref_qual_stid = $5;
      c_tid_t  const noexcept_stid = $6;
      c_ast_t *const trailing_ret_ast = $7;
      c_tid_t  const func_equals_stid = $8;

      DUMP_START( "func_decl_c_astp",
                  "decl2_c_astp '(' param_c_ast_list_opt ')' "
                  "func_qualifier_list_c_stid_opt "
                  "func_ref_qualifier_c_stid_opt noexcept_c_stid_opt "
                  "trailing_return_type_c_ast_opt "
                  "func_equals_c_stid_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST( "decl2_c_astp", decl2_ast );
      DUMP_AST_LIST( "param_c_ast_list_opt", $3 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", func_qual_stid );
      DUMP_TID( "func_ref_qualifier_c_stid_opt", func_ref_qual_stid );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );
      DUMP_TID( "func_equals_c_stid_opt", func_equals_stid );
      DUMP_AST( "target_ast", $1.target_ast );

      c_tid_t const func_stid =
        func_qual_stid | func_ref_qual_stid | noexcept_stid | func_equals_stid;

      //
      // Cdecl can't know for certain whether a "function" name is really a
      // constructor name because it:
      //
      //  1. Doesn't have the context of the surrounding class:
      //
      //          class T {
      //          public:
      //            T();                // <-- All cdecl usually has is this.
      //            // ...
      //          };
      //
      //  2. Can't check to see if the name is a typedef for a class, struct,
      //     or union (even though that would greatly help) since:
      //
      //          T(U);
      //
      //     could be either:
      //
      //      + A constructor of type T with a parameter of type U; or:
      //      + A variable named U of type T (with unnecessary parentheses).
      //
      // Hence, we have to infer which of a function or a constructor was
      // likely the one meant.  Assume the declaration is for a constructor
      // only if:
      //
      bool const assume_constructor =

        // + The current language is C++.
        OPT_LANG_IS( CPP_ANY ) &&

        // + The existing base type is none (because constructors don't have
        //   return types).
        type_ast->type.btids == TB_NONE &&

        // + The existing type does _not_ have any non-constructor storage
        //   classes.
        !c_tid_is_any( type_ast->type.stids, TS_FUNC_LIKE_NOT_CTOR ) &&

        ( // + The existing type has any constructor-only storage-class-like
          //   types (e.g., explicit).
          c_tid_is_any( type_ast->type.stids, TS_CONSTRUCTOR_ONLY ) ||

          // + Or the existing type only has storage-class-like types that may
          //   be applied to constructors.
          is_1n_bit_only_in_set(
            c_tid_no_tpid( type_ast->type.stids ),
            c_tid_no_tpid( TS_CONSTRUCTOR_DECL )
          )
        ) &&

        // + The new type does _not_ have any non-constructor storage classes.
        !c_tid_is_any( func_stid, TS_FUNC_LIKE_NOT_CTOR );

      c_ast_t *const func_ast =
        c_ast_new_gc( assume_constructor ? K_CONSTRUCTOR : K_FUNCTION, &@$ );
      func_ast->type.stids = c_tid_check( func_stid, C_TPID_STORE );
      func_ast->func.param_ast_list = slist_move( &$3 );

      if ( assume_constructor ) {
        assert( trailing_ret_ast == NULL );
        $$.ast = c_ast_add_func( decl2_ast, func_ast, /*ret_ast=*/NULL );
      }
      else if ( $1.target_ast != NULL ) {
        $$.ast = decl2_ast;
        PJL_IGNORE_RV( c_ast_add_func( $1.target_ast, func_ast, type_ast ) );
      }
      else {
        $$.ast = c_ast_add_func(
          decl2_ast,
          func_ast,
          trailing_ret_ast != NULL ? trailing_ret_ast : type_ast
        );
      }

      $$.target_ast = func_ast->func.ret_ast;

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
        for ( c_ast_t const *ast = $$.ast;
              (ast = c_ast_unpointer( ast )) != NULL; ) {
          if ( ast->kind == K_FUNCTION ) {
            $$.ast->type.atids &= c_tid_compl( TA_ANY_MSC_CALL );
            CONST_CAST( c_ast_t*, ast )->type.atids |= msc_call_atids;
          }
        } // for
      }

      DUMP_AST( "func_decl_c_astp", $$.ast );
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
  : Y_NAME '('
    {
      if ( OPT_LANG_IS( CPP_ANY ) ) {
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
        c_ast_t *const csu_ast = c_ast_new_gc( K_CLASS_STRUCT_UNION, &@1 );
        csu_ast->type.btids = TB_CLASS;
        c_sname_init_name( &csu_ast->csu.csu_sname, check_strdup( $1 ) );
        csu_ast->sname = c_sname_dup( &csu_ast->csu.csu_sname );

        in_attr.typedef_rb = c_typedef_add( csu_ast, C_GIB_TYPEDEF );
        MAYBE_UNUSED c_typedef_t *const csu_tdef = in_attr.typedef_rb->data;
        assert( csu_tdef->ast == csu_ast );
      }
    }
    param_c_ast_list_opt ')' noexcept_c_stid_opt func_equals_c_stid_opt
    {
      char const *const name = $1;
      c_tid_t     const noexcept_stid = $6;
      c_tid_t     const func_equals_stid = $7;

      DUMP_START( "pc99_func_or_constructor_declaration_c",
                  "NAME '(' param_c_ast_list_opt ')' noexcept_c_stid_opt "
                  "func_equals_c_stid_opt" );
      DUMP_STR( "NAME", name );
      DUMP_AST_LIST( "param_c_ast_list_opt", $4 );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );
      DUMP_TID( "func_equals_c_stid_opt", func_equals_stid );

      c_ast_t *ast;

      if ( OPT_LANG_IS( CPP_ANY ) ) {
        //
        // Free the temporary typedef for the class.
        //
        // Note that we free only the typedef and not its AST; its AST will be
        // garbage collected.
        //
        free( c_typedef_remove( in_attr.typedef_rb ) );

        //
        // In C++, encountering a name followed by '(' declares an in-class
        // constructor.
        //
        ast = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      }
      else {
        if ( UNSUPPORTED( IMPLICIT_int ) ) {
          //
          // In C99 and later, implicit int is an error.  This check has to be
          // done now in the parser rather than later in the AST since the AST
          // would have no "memory" that the return type was implicitly int.
          //
          print_error( &@1,
            "implicit \"int\" functions are illegal%s\n",
            C_LANG_WHICH( IMPLICIT_int )
          );
          FREE( name );
          PARSE_ABORT();
        }

        //
        // In C prior to C99, encountering a name followed by '(' declares a
        // function that implicitly returns int:
        //
        //      power(x, n)             /* raise x to n-th power; n > 0 */
        //
        c_ast_t *const ret_ast = c_ast_new_gc( K_BUILTIN, &@1 );
        ret_ast->type.btids = TB_INT;

        ast = c_ast_new_gc( K_FUNCTION, &@$ );
        ast->func.ret_ast = ret_ast;
      }

      c_sname_init_name( &ast->sname, name );
      ast->type.stids =
        c_tid_check( noexcept_stid | func_equals_stid, C_TPID_STORE );
      ast->func.param_ast_list = slist_move( &$4 );

      DUMP_AST( "pc99_func_or_constructor_declaration_c", ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( ast ) );
      c_ast_english( ast, stdout );
    }
  ;

rparen_func_qualifier_list_c_stid_opt
  : ')'
    {
      if ( OPT_LANG_IS( CPP_ANY ) ) {
        //
        // Both "final" and "override" are matched only within member function
        // declarations.  Now that ')' has been parsed, we're within one, so
        // set the keyword context to C_KW_CTX_MBR_FUNC.
        //
        lexer_keyword_ctx = C_KW_CTX_MBR_FUNC;
      }
    }
    func_qualifier_list_c_stid_opt
    {
      lexer_keyword_ctx = C_KW_CTX_DEFAULT;
      $$ = $3;
    }
  ;

func_qualifier_list_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | func_qualifier_list_c_stid_opt func_qualifier_c_stid
    {
      DUMP_START( "func_qualifier_list_c_stid_opt",
                  "func_qualifier_list_c_stid_opt func_qualifier_c_stid" );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $1 );
      DUMP_TID( "func_qualifier_c_stid", $2 );

      $$ = $1;
      PARSE_ASSERT( c_tid_add( &$$, $2, &@2 ) );

      DUMP_TID( "func_qualifier_list_c_stid_opt", $$ );
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

func_ref_qualifier_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_AMPER                       { $$ = TS_REFERENCE; }
  | Y_AMPER2                      { $$ = TS_RVALUE_REFERENCE; }
  ;

noexcept_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_noexcept
  | Y_noexcept '(' noexcept_bool_stid_exp rparen_exp
    {
      $$ = $3;
    }
  | Y_throw lparen_exp rparen_exp
    {
      $$ = $1;
    }
  | Y_throw lparen_exp param_c_ast_list ')'
    {
      c_ast_list_cleanup( &$3 );

      if ( OPT_LANG_IS( throw ) ) {
        print_error( &@3,
          "dynamic exception specifications not supported by %s\n", CDECL
        );
      } else {
        print_error( &@3,
          "dynamic exception specifications not supported%s\n",
          C_LANG_WHICH( throw )
        );
      }
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
    Y_ARROW type_c_ast { ia_type_ast_push( $2 ); } cast_c_astp_opt
    {
      ia_type_ast_pop();
      c_ast_t const *const ret_ast = ia_type_ast_peek();

      DUMP_START( "trailing_return_type_c_ast_opt",
                  "type_c_ast cast_c_astp_opt" );
      DUMP_AST( "in_attr__type_c_ast", ret_ast );
      DUMP_AST( "type_c_ast", $2 );
      DUMP_AST( "cast_c_astp_opt", $4.ast );

      $$ = $4.ast != NULL ? $4.ast : $2;

      DUMP_AST( "trailing_return_type_c_ast_opt", $$ );
      DUMP_END();

      //
      // The function trailing return-type syntax is supported only in C++11
      // and later.  This check has to be done now in the parser rather than
      // later in the AST because the AST has no "memory" of where the return-
      // type came from.
      //
      if ( UNSUPPORTED( TRAILING_RETURN_TYPE ) ) {
        print_error( &@1,
          "trailing return type not supported%s\n",
          C_LANG_WHICH( TRAILING_RETURN_TYPE )
        );
        PARSE_ABORT();
      }

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
      if ( ret_ast->type.btids != TB_AUTO ) {
        print_error( &ret_ast->loc,
          "function with trailing return type must only specify \"auto\"\n"
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
  | '=' Y_delete                  { $$ = $2; }
  | '=' Y_INT_LIT
    {
      if ( $2 != 0 ) {
        print_error( &@2, "'0' expected\n" );
        PARSE_ABORT();
      }
      $$ = TS_PURE_VIRTUAL;
    }
  | '=' error
    {
      if ( OPT_LANG_IS( default_delete_FUNC ) )
        elaborate_error( "'0', \"default\", or \"delete\" expected" );
      else
        elaborate_error( "'0' expected" );
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
  : param_c_ast_list comma_exp param_c_ast_exp
    {
      DUMP_START( "param_c_ast_list", "param_c_ast_list ',' param_c_ast" );
      DUMP_AST_LIST( "param_c_ast_list", $1 );
      DUMP_AST( "param_c_ast", $3 );

      $$ = $1;
      slist_push_back( &$$, $3 );

      DUMP_AST_LIST( "param_c_ast_list", $$ );
      DUMP_END();
    }

  | param_c_ast
    {
      DUMP_START( "param_c_ast_list", "param_c_ast" );
      DUMP_AST( "param_c_ast", $1 );

      slist_init( &$$ );
      slist_push_back( &$$, $1 );

      DUMP_AST_LIST( "param_c_ast_list", $$ );
      DUMP_END();
    }
  ;

param_c_ast
    /*
     * Ordinary function parameter declaration.
     */
  : this_stid_opt type_c_ast { ia_type_ast_push( $2 ); } cast_c_astp_opt
    {
      ia_type_ast_pop();

      DUMP_START( "param_c_ast", "this_stid_opt type_c_ast cast_c_astp_opt" );
      DUMP_TID( "this_stid_opt", $1 );
      DUMP_AST( "type_c_ast", $2 );
      DUMP_AST( "cast_c_astp_opt", $4.ast );

      $$ = c_ast_patch_placeholder( $2, $4.ast );

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

      PARSE_ASSERT( c_type_add_tid( &$$->type, $1, &@1 ) );

      DUMP_AST( "param_c_ast", $$ );
      DUMP_END();
    }

    /*
     * K&R C type-less function parameter declaration.
     */
  | name_ast

    /*
     * Varargs declaration.
     */
  | "..."
    {
      DUMP_START( "param_c_ast", "..." );

      $$ = c_ast_new_gc( K_VARIADIC, &@$ );

      DUMP_AST( "param_c_ast", $$ );
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

/// Gibberish C/C++ nested declaration ////////////////////////////////////////

nested_decl_c_astp
  : '('
    {
      ia_type_ast_push( c_ast_new_gc( K_PLACEHOLDER, &@$ ) );
      ++in_attr.ast_depth;
    }
    decl_c_astp rparen_exp
    {
      ia_type_ast_pop();
      --in_attr.ast_depth;

      DUMP_START( "nested_decl_c_astp", "'(' decl_c_astp ')'" );
      DUMP_AST( "decl_c_astp", $3.ast );

      $$ = $3;
      $$.ast->loc = @$;

      DUMP_AST( "nested_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish C++ operator declaration ////////////////////////////////////////

oper_decl_c_astp
  : // in_attr: type_c_ast
    oper_sname_c_opt Y_operator c_operator lparen_exp param_c_ast_list_opt
    rparen_func_qualifier_list_c_stid_opt func_ref_qualifier_c_stid_opt
    noexcept_c_stid_opt trailing_return_type_c_ast_opt func_equals_c_stid_opt
    {
      c_tid_t      const        func_qualifier_stid = $6;
      c_tid_t      const        func_ref_qualifier_stid = $7;
      c_tid_t      const        func_equals_stid = $10;
      c_tid_t      const        noexcept_stid = $8;
      c_oper_id_t  const        oper_id = $3;
      c_operator_t const *const operator = c_oper_get( oper_id );
      c_ast_t            *const trailing_ret_ast = $9;
      c_ast_t            *const type_ast = ia_type_ast_peek();

      DUMP_START( "oper_decl_c_astp",
                  "oper_sname_c_opt OPERATOR c_operator "
                  "'(' param_c_ast_list_opt ')' "
                  "func_qualifier_list_c_stid_opt "
                  "func_ref_qualifier_c_stid_opt noexcept_c_stid_opt "
                  "trailing_return_type_c_ast_opt "
                  "func_equals_c_stid_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_SNAME( "oper_sname_c_opt", $1 );
      DUMP_STR( "c_operator", operator->literal );
      DUMP_AST_LIST( "param_c_ast_list_opt", $5 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", func_qualifier_stid );
      DUMP_TID( "func_ref_qualifier_c_stid_opt", func_ref_qualifier_stid );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );
      DUMP_TID( "func_equals_c_stid_opt", func_equals_stid );

      c_tid_t const oper_stid =
        func_qualifier_stid | func_ref_qualifier_stid | noexcept_stid |
        func_equals_stid;

      c_ast_t *const oper_ast = c_ast_new_gc( K_OPERATOR, &@$ );
      oper_ast->sname = c_sname_move( &$1 );
      oper_ast->type.stids = c_tid_check( oper_stid, C_TPID_STORE );
      oper_ast->oper.param_ast_list = slist_move( &$5 );
      oper_ast->oper.operator = operator;

      c_ast_t *const ret_ast = trailing_ret_ast != NULL ?
        trailing_ret_ast : type_ast;

      $$.ast = c_ast_add_func( type_ast, oper_ast, ret_ast );
      $$.target_ast = oper_ast->oper.ret_ast;

      DUMP_AST( "oper_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ pointer declaration ///////////////////////////////////////

pointer_decl_c_astp
  : pointer_type_c_ast { ia_type_ast_push( $1 ); } decl_c_astp
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_decl_c_astp", "pointer_type_c_ast decl_c_astp" );
      DUMP_AST( "pointer_type_c_ast", $1 );
      DUMP_AST( "decl_c_astp", $3.ast );

      PJL_IGNORE_RV( c_ast_patch_placeholder( $1, $3.ast ) );
      $$ = $3;
      $$.ast->loc = @$;

      DUMP_AST( "pointer_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

pointer_type_c_ast
  : // in_attr: type_c_ast
    '*' type_qualifier_list_c_stid_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "pointer_type_c_ast", "* type_qualifier_list_c_stid_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $2 );

      $$ = c_ast_new_gc( K_POINTER, &@$ );
      $$->type.stids = c_tid_check( $2, C_TPID_STORE );
      c_ast_set_parent( type_ast, $$ );

      DUMP_AST( "pointer_type_c_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish pre-C99 implicit int pointer declaration ////////////////////////

pc99_pointer_decl_list_c
  : pc99_pointer_decl_c
  | pc99_pointer_decl_c ',' decl_list_c
  ;

pc99_pointer_decl_c
  : pc99_pointer_type_c_ast { ia_type_ast_push( $1 ); } decl_c_astp
    {
      ia_type_ast_pop();

      DUMP_START( "pc99_pointer_decl_c",
                  "pc99_pointer_type_c_ast decl_c_astp" );
      DUMP_AST( "pc99_pointer_type_c_ast", $1 );
      DUMP_AST( "decl_c_astp", $3.ast );

      PJL_IGNORE_RV( c_ast_patch_placeholder( $1, $3.ast ) );
      c_ast_t *const ast = $3.ast;
      ast->loc = @$;

      DUMP_AST( "pc99_pointer_decl_c", ast );
      DUMP_END();

      PARSE_ASSERT( c_ast_check( ast ) );
      c_ast_english( ast, stdout );
    }
  ;

pc99_pointer_type_c_ast
  : '*' type_qualifier_list_c_stid_opt
    {
      if ( OPT_LANG_IS( C_ANY ) && UNSUPPORTED( IMPLICIT_int ) ) {
        //
        // In C99 and later, implicit int is an error.  This check has to be
        // done now in the parser rather than later in the AST since the AST
        // would have no "memory" that the return type was implicitly int.
        //
        print_error( &@1,
          "implicit \"int\" is illegal%s\n",
          C_LANG_WHICH( IMPLICIT_int )
        );
        PARSE_ABORT();
      }

      DUMP_START( "pc99_pointer_type_c_ast",
                  "* type_qualifier_list_c_stid_opt" );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $2 );

      if ( false_set( &in_attr.is_implicit_int ) ) {
        c_ast_t *const int_ast = c_ast_new_gc( K_BUILTIN, &@1 );
        int_ast->type.btids = TB_INT;
        ia_type_ast_push( int_ast );
      }

      c_ast_t *const type_ast = c_ast_dup( ia_type_ast_peek(), &gc_ast_list );
      $$ = c_ast_pointer( type_ast, &gc_ast_list );
      $$->type.stids = c_tid_check( $2, C_TPID_STORE );

      DUMP_AST( "pc99_pointer_type_c_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ pointer-to-member declaration ///////////////////////////////

pointer_to_member_decl_c_astp
  : pointer_to_member_type_c_ast { ia_type_ast_push( $1 ); } decl_c_astp
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_to_member_decl_c_astp",
                  "pointer_to_member_type_c_ast decl_c_astp" );
      DUMP_AST( "pointer_to_member_type_c_ast", $1 );
      DUMP_AST( "decl_c_astp", $3.ast );

      $$ = $3;
      $$.ast->loc = @$;

      DUMP_AST( "pointer_to_member_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

pointer_to_member_type_c_ast
  : // in_attr: type_c_ast
    any_sname_c Y_COLON2_STAR cv_qualifier_list_stid_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "pointer_to_member_type_c_ast",
                  "sname '::*' cv_qualifier_list_stid_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_SNAME( "sname", $1 );
      DUMP_TID( "cv_qualifier_list_stid_opt", $3 );

      $$ = c_ast_new_gc( K_POINTER_TO_MEMBER, &@$ );

      c_type_t scope_type = *c_sname_local_type( &$1 );
      if ( !c_tid_is_any( scope_type.btids, TB_ANY_SCOPE ) ) {
        //
        // The sname has no scope-type, but we now know there's a pointer-to-
        // member of it, so it must be a class.  (It could alternatively be a
        // struct, but we have no context to know, so just pick class because
        // it's more C++-like.)
        //
        scope_type.btids = TB_CLASS;
        c_sname_set_local_type( &$1, &scope_type );
      }

      // adopt sname's scope-type for the AST
      $$->type = c_type_or( &C_TYPE_LIT_S( $3 ), &scope_type );

      $$->ptr_mbr.class_sname = c_sname_move( &$1 );
      c_ast_set_parent( type_ast, $$ );

      DUMP_AST( "pointer_to_member_type_c_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ reference declaration ///////////////////////////////////////

reference_decl_c_astp
  : reference_type_c_ast { ia_type_ast_push( $1 ); } decl_c_astp
    {
      ia_type_ast_pop();

      DUMP_START( "reference_decl_c_astp", "reference_type_c_ast decl_c_astp" );
      DUMP_AST( "reference_type_c_ast", $1 );
      DUMP_AST( "decl_c_astp", $3.ast );

      $$ = $3;
      $$.ast->loc = @$;

      DUMP_AST( "reference_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

reference_type_c_ast
  : // in_attr: type_c_ast
    Y_AMPER type_qualifier_list_c_stid_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "reference_type_c_ast", "&" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $2 );

      $$ = c_ast_new_gc( K_REFERENCE, &@$ );
      $$->type.stids = c_tid_check( $2, C_TPID_STORE );
      c_ast_set_parent( type_ast, $$ );

      DUMP_AST( "reference_type_c_ast", $$ );
      DUMP_END();
    }

  | // in_attr: type_c_ast
    Y_AMPER2 type_qualifier_list_c_stid_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "reference_type_c_ast", "&&" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $2 );

      $$ = c_ast_new_gc( K_RVALUE_REFERENCE, &@$ );
      $$->type.stids = c_tid_check( $2, C_TPID_STORE );
      c_ast_set_parent( type_ast, $$ );

      DUMP_AST( "reference_type_c_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ typedef type declaration //////////////////////////////////

typedef_type_decl_c_ast
  : // in_attr: type_c_ast
    typedef_type_c_ast
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "typedef_type_decl_c_ast", "typedef_type_c_ast" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST( "typedef_type_c_ast", $1 );

      if ( c_tid_is_any( type_ast->type.stids, TS_TYPEDEF ) ) {
        //
        // If we're defining a type, return the type as-is.
        //
        $$ = $1;
      }
      else {
        //
        // Otherwise, return the type that it's typedef'd as.
        //
        c_ast_t const *const raw_ast = c_ast_untypedef( $1 );

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
        c_typedef_t const *const tdef = c_typedef_find_sname( &raw_ast->sname );
        if ( tdef != NULL ) {
          print_error(
            &$1->loc,
            "\"%s\": previously declared as type: ",
            c_sname_full_name( &raw_ast->sname )
          );
          print_type( tdef, stderr );
          PARSE_ABORT();
        }

        //
        // We have to duplicate the type to set the current location.
        //
        $$ = c_ast_dup( raw_ast, &gc_ast_list );
        $$->loc = $1->loc;
      }

      DUMP_AST( "typedef_type_c_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ user-defined conversion operator declaration ////////////////

user_defined_conversion_decl_c_astp
  : // in_attr: type_c_ast
    oper_sname_c_opt Y_operator type_c_ast
    {
      ia_type_ast_push( $3 );
    }
    udc_decl_c_ast_opt lparen_exp rparen_func_qualifier_list_c_stid_opt
    noexcept_c_stid_opt func_equals_c_stid_opt
    {
      ia_type_ast_pop();

      c_ast_t *const  type_ast = ia_type_ast_peek();
      c_ast_t *const  to_ast = $3;
      c_ast_t *const  udc_ast = $5;
      c_tid_t  const  func_qual_stids = $7;
      c_tid_t  const  noexcept_stid = $8;
      c_tid_t  const  func_equals_stid = $9;

      DUMP_START( "user_defined_conversion_decl_c_astp",
                  "oper_sname_c_opt OPERATOR "
                  "type_c_ast udc_decl_c_ast_opt '(' ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt "
                  "func_equals_c_stid_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_SNAME( "oper_sname_c_opt", $1 );
      DUMP_AST( "type_c_ast", to_ast );
      DUMP_AST( "udc_decl_c_ast_opt", udc_ast );
      DUMP_TID( "func_qualifier_list_c_stid_opt", func_qual_stids );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );
      DUMP_TID( "func_equals_c_stid_opt", func_equals_stid );

      $$.ast = c_ast_new_gc( K_USER_DEF_CONVERSION, &@$ );
      $$.ast->sname = c_sname_move( &$1 );
      $$.ast->type.stids = c_tid_check(
        func_qual_stids | noexcept_stid | func_equals_stid,
        C_TPID_STORE
      );
      if ( type_ast != NULL )
        c_type_or_eq( &$$.ast->type, &type_ast->type );
      $$.ast->udef_conv.to_ast = udc_ast != NULL ? udc_ast : to_ast;
      $$.target_ast = $$.ast->udef_conv.to_ast;

      DUMP_AST( "user_defined_conversion_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish C++ user-defined literal declaration ////////////////////////////

user_defined_literal_decl_c_astp
  : // in_attr: type_c_ast
    oper_sname_c_opt Y_operator quote2_exp name_exp
    lparen_exp param_c_ast_list_exp ')' noexcept_c_stid_opt
    trailing_return_type_c_ast_opt
    {
      char const *const name = $4;
      c_tid_t     const noexcept_stid = $8;
      c_ast_t    *const trailing_ret_ast = $9;
      c_ast_t    *const type_ast = ia_type_ast_peek();

      DUMP_START( "user_defined_literal_decl_c_astp",
                  "oper_sname_c_opt OPERATOR \"\" "
                  "'(' param_c_ast_list_exp ')' noexcept_c_stid_opt "
                  "trailing_return_type_c_ast_opt" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_SNAME( "oper_sname_c_opt", $1 );
      DUMP_STR( "name", name );
      DUMP_AST_LIST( "param_c_ast_list_exp", $6 );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );

      c_sname_set( &type_ast->sname, &$1 );
      c_sname_append_name( &type_ast->sname, name );

      c_ast_t *const udl_ast = c_ast_new_gc( K_USER_DEF_LITERAL, &@$ );
      udl_ast->type.stids = c_tid_check( noexcept_stid, C_TPID_STORE );
      udl_ast->udef_lit.param_ast_list = slist_move( &$6 );

      $$.ast = c_ast_add_func(
        type_ast,
        udl_ast,
        trailing_ret_ast != NULL ? trailing_ret_ast : type_ast
      );

      $$.target_ast = udl_ast->udef_lit.ret_ast;

      DUMP_AST( "user_defined_literal_decl_c_astp", $$.ast );
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
  | sname_c_ast                   { $$ = (c_ast_pair_t){ $1, NULL }; }
//| typedef_type_cast_c_ast             // can't cast to a typedef
//| user_defined_conversion_cast_c_astp // can't cast to a user-defined conv.
//| user_defined_literal_cast_c_astp    // can't cast to a user-defined literal
  ;

/// Gibberish C/C++ array cast ////////////////////////////////////////////////

array_cast_c_astp
  : // in_attr: type_c_ast
    cast_c_astp_opt array_size_c_ast
    {
      c_ast_t *const type_ast = ia_type_ast_peek();
      c_ast_t *const array_ast = $2;

      DUMP_START( "array_cast_c_astp", "cast_c_astp_opt array_size_c_ast" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST( "cast_c_astp_opt", $1.ast );
      DUMP_AST( "target_ast", $1.target_ast );
      DUMP_AST( "array_size_c_ast", array_ast );

      c_ast_set_parent( c_ast_new_gc( K_PLACEHOLDER, &@1 ), array_ast );

      if ( $1.target_ast != NULL ) {    // array-of or function-like-ret type
        $$.ast = $1.ast;
        $$.target_ast = c_ast_add_array( $1.target_ast, array_ast, type_ast );
      } else {
        c_ast_t *const ast = $1.ast != NULL ? $1.ast : type_ast;
        $$.ast = c_ast_add_array( ast, array_ast, type_ast );
        $$.target_ast = NULL;
      }

      DUMP_AST( "array_cast_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ block cast (Apple extension) //////////////////////////////

block_cast_c_astp                       // Apple extension
  : // in_attr: type_c_ast
    '(' Y_CARET
    { //
      // A block AST has to be the type inherited attribute for cast_c_astp_opt
      // so we have to create it here.
      //
      ia_type_ast_push( c_ast_new_gc( K_APPLE_BLOCK, &@$ ) );
    }
    type_qualifier_list_c_stid_opt cast_c_astp_opt rparen_exp
    lparen_exp param_c_ast_list_opt ')'
    {
      c_ast_t *const  block_ast = ia_type_ast_pop();
      c_ast_t *const  type_ast = ia_type_ast_peek();
      c_tid_t  const  type_qual_stids = $4;
      c_ast_t *const  cast_ast = $5.ast;

      DUMP_START( "block_cast_c_astp",
                  "'(' '^' type_qualifier_list_c_stid_opt cast_c_astp_opt ')' "
                  "'(' param_c_ast_list_opt ')'" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", type_qual_stids );
      DUMP_AST( "cast_c_astp_opt", cast_ast );
      DUMP_AST_LIST( "param_c_ast_list_opt", $8 );

      PARSE_ASSERT( c_type_add_tid( &block_ast->type, type_qual_stids, &@4 ) );
      block_ast->block.param_ast_list = slist_move( &$8 );
      $$.ast = c_ast_add_func( cast_ast, block_ast, type_ast );
      $$.target_ast = block_ast->block.ret_ast;

      DUMP_AST( "block_cast_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ function cast /////////////////////////////////////////////

func_cast_c_astp
  : // in_attr: type_c_ast
    cast2_c_astp '(' param_c_ast_list_opt rparen_func_qualifier_list_c_stid_opt
    noexcept_c_stid_opt trailing_return_type_c_ast_opt
    {
      c_ast_t *const cast2_c_ast = $1.ast;
      c_tid_t  const func_ref_qualifier_stid = $4;
      c_tid_t  const noexcept_stid = $5;
      c_ast_t *      ret_ast = ia_type_ast_peek();
      c_ast_t *const trailing_ret_ast = $6;

      DUMP_START( "func_cast_c_astp",
                  "cast2_c_astp '(' param_c_ast_list_opt ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt "
                  "trailing_return_type_c_ast_opt" );
      DUMP_AST( "in_attr__type_c_ast", ret_ast );
      DUMP_AST( "cast2_c_ast", cast2_c_ast );
      DUMP_AST_LIST( "param_c_ast_list_opt", $3 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", func_ref_qualifier_stid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );
      DUMP_AST( "target_ast", $1.target_ast );

      if ( cast2_c_ast->kind == K_FUNCTION ) {
        //
        // This is for a case like:
        //
        //      void f( int () () )
        //              |   |  |
        //              |   |  func
        //              |   |
        //              |   cast2_c_ast (func)
        //              |
        //              ret_ast
        //
        // We replace ret_ast with cast2_c_ast:
        //
        //      void f( int() () )
        //              |     |
        //              |     func
        //              |
        //              ret_ast <- cast2_c_ast (func)
        //
        // that is, a "function returning function returning int" -- which is
        // illegal (since functions can't return functions) and will be caught
        // by c_ast_check_ret_type().
        //
        ret_ast = cast2_c_ast;
      }

      c_ast_t *const func_ast = c_ast_new_gc( K_FUNCTION, &@$ );
      c_tid_t const func_stid = func_ref_qualifier_stid | noexcept_stid;
      func_ast->type.stids = c_tid_check( func_stid, C_TPID_STORE );
      func_ast->func.param_ast_list = slist_move( &$3 );

      if ( $1.target_ast != NULL ) {
        $$.ast = cast2_c_ast;
        PJL_IGNORE_RV( c_ast_add_func( $1.target_ast, func_ast, ret_ast ) );
      }
      else {
        $$.ast = c_ast_add_func(
          cast2_c_ast,
          func_ast,
          trailing_ret_ast != NULL ? trailing_ret_ast : ret_ast
        );
      }

      $$.target_ast = func_ast->func.ret_ast;

      DUMP_AST( "func_cast_c_astp", $$.ast );
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
    cast_c_astp_opt rparen_exp
    {
      ia_type_ast_pop();
      --in_attr.ast_depth;

      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "nested_cast_c_astp", "'(' cast_c_astp_opt ')'" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST( "cast_c_astp_opt", $3.ast );

      $$ = $3;

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

      DUMP_AST( "nested_cast_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ pointer cast //////////////////////////////////////////////

pointer_cast_c_astp
  : pointer_type_c_ast { ia_type_ast_push( $1 ); } cast_c_astp_opt
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_cast_c_astp", "pointer_type_c_ast cast_c_astp_opt" );
      DUMP_AST( "pointer_type_c_ast", $1 );
      DUMP_AST( "cast_c_astp_opt", $3.ast );

      $$.ast = c_ast_patch_placeholder( $1, $3.ast );
      $$.target_ast = $3.target_ast;

      DUMP_AST( "pointer_cast_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ pointer-to-member cast ////////////////////////////////////

pointer_to_member_cast_c_astp
  : pointer_to_member_type_c_ast { ia_type_ast_push( $1 ); } cast_c_astp_opt
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_to_member_cast_c_astp",
                  "pointer_to_member_type_c_ast cast_c_astp_opt" );
      DUMP_AST( "pointer_to_member_type_c_ast", $1 );
      DUMP_AST( "cast_c_astp_opt", $3.ast );

      $$.ast = c_ast_patch_placeholder( $1, $3.ast );
      $$.target_ast = $3.target_ast;

      DUMP_AST( "pointer_to_member_cast_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ reference cast ////////////////////////////////////////////

reference_cast_c_astp
  : reference_type_c_ast { ia_type_ast_push( $1 ); } cast_c_astp_opt
    {
      ia_type_ast_pop();

      DUMP_START( "reference_cast_c_astp",
                  "reference_type_c_ast cast_c_astp_opt" );
      DUMP_AST( "reference_type_c_ast", $1 );
      DUMP_AST( "cast_c_astp_opt", $3.ast );

      $$.ast = c_ast_patch_placeholder( $1, $3.ast );
      $$.target_ast = $3.target_ast;

      DUMP_AST( "reference_cast_c_astp", $$.ast );
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
  : pointer_type_c_ast { ia_type_ast_push( $1 ); } udc_decl_c_ast_opt
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_udc_decl_c_ast",
                  "pointer_type_c_ast udc_decl_c_ast_opt" );
      DUMP_AST( "pointer_type_c_ast", $1 );
      DUMP_AST( "udc_decl_c_ast_opt", $3 );

      $$ = c_ast_patch_placeholder( $1, $3 );

      DUMP_AST( "pointer_udc_decl_c_ast", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_udc_decl_c_ast
  : pointer_to_member_type_c_ast { ia_type_ast_push( $1 ); } udc_decl_c_ast_opt
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_to_member_udc_decl_c_ast",
                  "pointer_to_member_type_c_ast udc_decl_c_ast_opt" );
      DUMP_AST( "pointer_to_member_type_c_ast", $1 );
      DUMP_AST( "udc_decl_c_ast_opt", $3 );

      $$ = c_ast_patch_placeholder( $1, $3 );

      DUMP_AST( "pointer_to_member_udc_decl_c_ast", $$ );
      DUMP_END();
    }
  ;

reference_udc_decl_c_ast
  : reference_type_c_ast { ia_type_ast_push( $1 ); } udc_decl_c_ast_opt
    {
      ia_type_ast_pop();

      DUMP_START( "reference_udc_decl_c_ast",
                  "reference_type_c_ast udc_decl_c_ast_opt" );
      DUMP_AST( "reference_type_c_ast", $1 );
      DUMP_AST( "udc_decl_c_ast_opt", $3 );

      $$ = c_ast_patch_placeholder( $1, $3 );

      DUMP_AST( "reference_udc_decl_c_ast", $$ );
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
  : type_modifier_list_c_type           // allows implicit int in pre-C99
    {
      DUMP_START( "type_c_ast", "type_modifier_list_c_type" );
      DUMP_TYPE( "type_modifier_list_c_type", $1 );

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
        C_TYPE_LIT_B( TB_INT ) : T_NONE;

      PARSE_ASSERT( c_type_add( &type, &$1, &@1 ) );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type = type;

      DUMP_AST( "type_c_ast", $$ );
      DUMP_END();
    }

    /*
     * Type-modifier type with optional trailing type-modifier(s) declarations:
     *
     *      unsigned int const i;       // uncommon, but legal
     */
  | type_modifier_list_c_type east_modified_type_c_ast
    {
      DUMP_START( "type_c_ast",
                  "type_modifier_list_c_type east_modified_type_c_ast " );
      DUMP_TYPE( "type_modifier_list_c_type", $1 );
      DUMP_AST( "east_modified_type_c_ast", $2 );

      $$ = $2;
      $$->loc = @$;
      PARSE_ASSERT( c_type_add( &$$->type, &$1, &@1 ) );

      DUMP_AST( "type_c_ast", $$ );
      DUMP_END();
    }

  | east_modified_type_c_ast
  ;

type_modifier_list_c_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | type_modifier_list_c_type
  ;

type_modifier_list_c_type
  : type_modifier_list_c_type type_modifier_c_type
    {
      DUMP_START( "type_modifier_list_c_type",
                  "type_modifier_list_c_type type_modifier_c_type" );
      DUMP_TYPE( "type_modifier_list_c_type", $1 );
      DUMP_TYPE( "type_modifier_c_type", $2 );

      $$ = $1;
      PARSE_ASSERT( c_type_add( &$$, &$2, &@2 ) );

      DUMP_TYPE( "type_modifier_list_c_type", $$ );
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
      // Y_TYPEDEF_NAME or Y_TYPEDEF_SNAME if we encounter at least one type
      // modifier (except "register" since it's is really a storage class --
      // see the comment in type_modifier_base_type about "register").
      //
      if ( $$.stids != TS_REGISTER )
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
  : atomic_builtin_typedef_type_c_ast type_modifier_list_c_type_opt
    {
      DUMP_START( "east_modified_type_c_ast",
                  "atomic_builtin_typedef_type_c_ast "
                  "type_modifier_list_c_type_opt" );
      DUMP_AST( "atomic_builtin_typedef_type_c_ast", $1 );
      DUMP_TYPE( "type_modifier_list_c_type_opt", $2 );

      $$ = $1;
      $$->loc = @$;
      PARSE_ASSERT( c_type_add( &$$->type, &$2, &@2 ) );

      DUMP_AST( "type_c_ast", $$ );
      DUMP_END();
    }

  | enum_class_struct_union_c_ast cv_qualifier_list_stid_opt
    {
      DUMP_START( "east_modified_type_c_ast",
                  "enum_class_struct_union_c_ast cv_qualifier_list_stid_opt" );
      DUMP_AST( "enum_class_struct_union_c_ast", $1 );
      DUMP_TID( "cv_qualifier_list_stid_opt", $2 );

      $$ = $1;
      $$->loc = @$;
      PARSE_ASSERT( c_type_add_tid( &$$->type, $2, &@2 ) );

      DUMP_AST( "east_modified_type_c_ast", $$ );
      DUMP_END();
    }
  ;

atomic_builtin_typedef_type_c_ast
  : atomic_specifier_type_c_ast
  | builtin_type_c_ast
  | typedef_type_c_ast
  | typeof_type_c_ast
  ;

/// Gibberish C _Atomic types /////////////////////////////////////////////////

atomic_specifier_type_c_ast
  : Y__Atomic_SPEC lparen_exp type_c_ast
    {
      ia_type_ast_push( $3 );
    }
    cast_c_astp_opt rparen_exp
    {
      ia_type_ast_pop();

      c_ast_t *const type_ast = $3;
      c_ast_t *const cast_ast = $5.ast;

      DUMP_START( "atomic_specifier_type_c_ast",
                  "ATOMIC '(' type_c_ast cast_c_astp_opt ')'" );
      DUMP_AST( "type_c_ast", type_ast );
      DUMP_AST( "cast_c_astp_opt", cast_ast );

      $$ = cast_ast != NULL ? cast_ast : type_ast;
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
        print_error( &@3,
          "_Atomic can not be of \"%s\"\n", c_type_name_c( &error_type )
        );
        PARSE_ABORT();
      }

      PARSE_ASSERT( c_type_add_tid( &$$->type, TS_ATOMIC, &@1 ) );

      DUMP_AST( "atomic_specifier_type_c_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ built-in types ////////////////////////////////////////////

builtin_type_c_ast
  : builtin_no_BitInt_c_btid
    {
      DUMP_START( "builtin_type_c_ast", "builtin_no_BitInt_c_btid" );
      DUMP_TID( "builtin_no_BitInt_c_btid", $1 );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = c_tid_check( $1, C_TPID_BASE );

      DUMP_AST( "builtin_type_c_ast", $$ );
      DUMP_END();
    }
  | Y__BitInt lparen_exp int_lit_exp rparen_exp
    {
      DUMP_START( "builtin_type_c_ast", "_BitInt '(' int_lit_exp ')'" );
      DUMP_INT( "int", $3 );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = TB_BITINT;
      $$->builtin.BitInt.width = STATIC_CAST( unsigned, $3 );

      DUMP_AST( "builtin_type_c_ast", $$ );
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
  | Y_wchar_t
  | Y_int
  | Y_float
  | Y_double
  | Y_EMC__Accum
  | Y_EMC__Fract
  ;

typeof_type_c_ast
  : typeof
    {
      print_error( &@1, "typeof declarations not supported by %s\n", CDECL );
      PARSE_ABORT();
    }
  ;

typeof
  : Y_typeof
  | Y_typeof_unqual
  ;

/// Gibberish C/C++ enum, class, struct, & union types ////////////////////////

enum_class_struct_union_c_ast
  : class_struct_union_c_ast
  | enum_c_ast
  ;

class_struct_union_c_ast
  : class_struct_union_btid attribute_specifier_list_c_atid_opt any_sname_c_exp
    {
      c_tid_t  const  csu_btid = $1;
      c_tid_t  const  atids = $2;

      DUMP_START( "enum_class_struct_union_c_ast",
                  "class_struct_union_btid "
                  "attribute_specifier_list_c_atid_opt sname" );
      DUMP_TID( "class_struct_union_btid", csu_btid );
      DUMP_TID( "attribute_specifier_list_c_atid_opt", atids );
      DUMP_SNAME( "any_sname_c", $3 );

      $$ = c_ast_new_gc( K_CLASS_STRUCT_UNION, &@$ );
      $$->type.btids = c_tid_check( csu_btid, C_TPID_BASE );
      $$->type.atids = c_tid_check( atids, C_TPID_ATTR );
      $$->csu.csu_sname = c_sname_move( &$3 );

      DUMP_AST( "class_struct_union_c_ast", $$ );
      DUMP_END();
    }

  | class_struct_union_btid attribute_specifier_list_c_atid_opt any_sname_c_opt
    '{'
    {
      print_error( &@4,
        "explaining %s declarations not supported by %s\n",
        c_tid_name_c( $1 ), CDECL
      );
      c_sname_cleanup( &$3 );
      PARSE_ABORT();
    }
  ;

enum_c_ast
  : enum_btids attribute_specifier_list_c_atid_opt any_sname_c_exp
    enum_fixed_type_c_ast_opt
    {
      c_tid_t  const  enum_btids = $1;
      c_tid_t  const  atids = $2;
      c_ast_t *const  fixed_type_ast = $4;

      DUMP_START( "enum_c_ast",
                  "enum_btids attribute_specifier_list_c_atid_opt sname "
                  "enum_fixed_type_c_ast_opt" );
      DUMP_TID( "enum_btids", enum_btids );
      DUMP_TID( "attribute_specifier_list_c_atid_opt", atids );
      DUMP_SNAME( "any_sname_c", $3 );
      DUMP_AST( "enum_fixed_type_c_ast_opt", fixed_type_ast );

      $$ = c_ast_new_gc( K_ENUM, &@$ );
      $$->type.btids = c_tid_check( enum_btids, C_TPID_BASE );
      $$->type.atids = c_tid_check( atids, C_TPID_ATTR );
      $$->enum_.of_ast = fixed_type_ast;
      $$->enum_.enum_sname = c_sname_move( &$3 );

      DUMP_AST( "enum_c_ast", $$ );
      DUMP_END();
    }

  | enum_btids attribute_specifier_list_c_atid_opt any_sname_c_opt '{'
    {
      print_error( &@4,
        "explaining %s declarations not supported by %s\n",
        c_tid_name_c( $1 ), CDECL
      );
      c_sname_cleanup( &$3 );
      PARSE_ABORT();
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
  : enum_fixed_type_modifier_list_btid
    {
      DUMP_START( "enum_fixed_type_c_ast",
                  "enum_fixed_type_modifier_list_btid" );
      DUMP_TID( "enum_fixed_type_modifier_list_btid", $1 );

      $$ = c_ast_new_gc( K_BUILTIN, &@1 );
      $$->type.btids = c_tid_check( $1, C_TPID_BASE );

      DUMP_AST( "enum_fixed_type_c_ast", $$ );
      DUMP_END();
    }

    /*
     * Type-modifier(s) type with optional training type-modifier(s):
     *
     *      enum E : short int unsigned // uncommon, but legal
     */
  | enum_fixed_type_modifier_list_btid enum_unmodified_fixed_type_c_ast
    enum_fixed_type_modifier_list_btid_opt
    {
      DUMP_START( "enum_fixed_type_c_ast",
                  "enum_fixed_type_modifier_list_btid "
                  "enum_unmodified_fixed_type_c_ast "
                  "enum_fixed_type_modifier_list_btid_opt" );
      DUMP_TID( "enum_fixed_type_modifier_list_btid", $1 );
      DUMP_AST( "enum_unmodified_fixed_type_c_ast", $2 );
      DUMP_TID( "enum_fixed_type_modifier_list_btid_opt", $3 );

      $$ = $2;
      $$->loc = @$;
      PARSE_ASSERT( c_type_add_tid( &$$->type, $1, &@1 ) );
      PARSE_ASSERT( c_type_add_tid( &$$->type, $3, &@3 ) );

      DUMP_AST( "enum_fixed_type_c_ast", $$ );
      DUMP_END();
    }

    /*
     * Type with trailing type-modifier(s):
     *
     *      enum E : int unsigned       // uncommon, but legal
     */
  | enum_unmodified_fixed_type_c_ast enum_fixed_type_modifier_list_btid_opt
    {
      DUMP_START( "enum_fixed_type_c_ast",
                  "enum_unmodified_fixed_type_c_ast "
                  "enum_fixed_type_modifier_list_btid_opt" );
      DUMP_AST( "enum_unmodified_fixed_type_c_ast", $1 );
      DUMP_TID( "enum_fixed_type_modifier_list_btid_opt", $2 );

      $$ = $1;
      $$->loc = @$;
      PARSE_ASSERT( c_type_add_tid( &$$->type, $2, &@2 ) );

      DUMP_AST( "enum_fixed_type_c_ast", $$ );
      DUMP_END();
    }
  ;

enum_fixed_type_modifier_list_btid_opt
  : /* empty */                   { $$ = TB_NONE; }
  | enum_fixed_type_modifier_list_btid
  ;

enum_fixed_type_modifier_list_btid
  : enum_fixed_type_modifier_list_btid enum_fixed_type_modifier_btid
    {
      DUMP_START( "enum_fixed_type_modifier_list_btid",
                  "enum_fixed_type_modifier_list_btid "
                  "enum_fixed_type_modifier_btid" );
      DUMP_TID( "enum_fixed_type_modifier_list_btid", $1 );
      DUMP_TID( "enum_fixed_type_modifier_btid", $2 );

      $$ = $1;
      PARSE_ASSERT( c_tid_add( &$$, $2, &@2 ) );

      DUMP_TID( "enum_fixed_type_modifier_list_btid", $$ );
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
  : type_qualifier_list_c_stid type_qualifier_c_stid
    gnu_or_msc_attribute_specifier_list_c_opt
    {
      DUMP_START( "type_qualifier_list_c_stid",
                  "type_qualifier_list_c_stid type_qualifier_c_stid" );
      DUMP_TID( "type_qualifier_list_c_stid", $1 );
      DUMP_TID( "type_qualifier_c_stid", $2 );

      $$ = $1;
      PARSE_ASSERT( c_tid_add( &$$, $2, &@2 ) );

      DUMP_TID( "type_qualifier_list_c_stid", $$ );
      DUMP_END();
    }

  | gnu_or_msc_attribute_specifier_list_c type_qualifier_c_stid
    {
      $$ = $2;
    }

  | type_qualifier_c_stid gnu_or_msc_attribute_specifier_list_c_opt
    {
      $$ = $1;
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
  | cv_qualifier_list_stid_opt cv_qualifier_stid
    {
      DUMP_START( "cv_qualifier_list_stid_opt",
                  "cv_qualifier_list_stid_opt cv_qualifier_stid" );
      DUMP_TID( "cv_qualifier_list_stid_opt", $1 );
      DUMP_TID( "cv_qualifier_stid", $2 );

      $$ = $1;
      PARSE_ASSERT( c_tid_add( &$$, $2, &@2 ) );

      DUMP_TID( "cv_qualifier_list_stid_opt", $$ );
      DUMP_END();
    }
  ;

restrict_qualifier_c_stid
  : Y_restrict                          // C only
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since both "restrict" and "__restrict" map to TS_RESTRICT and the
      // AST has no "memory" of which it was.
      //
      if ( OPT_LANG_IS( CPP_ANY ) ) {
        print_error( &@1,
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
  | '[' Y_INT_LIT rbracket_exp
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
      if ( UNSUPPORTED( _Noreturn ) ) {
        print_error( &@1,
          "\"%s\" keyword not supported%s",
          lexer_token, C_LANG_WHICH( _Noreturn )
        );
        print_hint( "[[noreturn]]" );
        PARSE_ABORT();
      }
      if ( OPT_LANG_IS( C_MIN(23)) ) {
        print_warning( &@1,
          "\"%s\" is deprecated%s",
          lexer_token, C_LANG_WHICH( C_MAX(17) )
        );
        print_hint( "[[noreturn]]" );
      }

      $$ = C_TYPE_LIT_A( $1 );
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
      if ( UNSUPPORTED( ATTRIBUTES ) ) {
        print_error( &@1,
          "\"[[\" attribute syntax not supported%s\n",
          C_LANG_WHICH( ATTRIBUTES )
        );
        PARSE_ABORT();
      }
      lexer_keyword_ctx = C_KW_CTX_ATTRIBUTE;
    }
    using_opt attribute_list_c_atid_opt ']' rbracket_exp
    {
      lexer_keyword_ctx = C_KW_CTX_DEFAULT;

      DUMP_START( "attribute_specifier_list_c_atid",
                  "'[[' using_opt attribute_list_c_atid_opt ']]'" );
      DUMP_TID( "attribute_list_c_atid_opt", $5 );

      $$ = $5;

      DUMP_END();
    }

  | gnu_or_msc_attribute_specifier_list_c
    {
      $$ = TA_NONE;
    }
  ;

using_opt
  : /* empty */
  | Y_using name_exp colon_exp
    {
      print_warning( &@1,
        "\"using\" in attributes not supported by %s (ignoring)\n", CDECL
      );
      free( $2 );
    }
  ;

attribute_list_c_atid_opt
  : /* empty */                   { $$ = TA_NONE; }
  | attribute_list_c_atid
  ;

attribute_list_c_atid
  : attribute_list_c_atid comma_exp attribute_c_atid_exp
    {
      DUMP_START( "attribute_list_c_atid",
                  "attribute_list_c_atid , attribute_c_atid" );
      DUMP_TID( "attribute_list_c_atid", $1 );
      DUMP_TID( "attribute_c_atid_exp", $3 );

      $$ = $1;
      PARSE_ASSERT( c_tid_add( &$$, $3, &@3 ) );

      DUMP_TID( "attribute_list_c_atid", $$ );
      DUMP_END();
    }

  | attribute_c_atid_exp
  ;

attribute_c_atid_exp
  : Y_carries_dependency
  | Y_deprecated attribute_str_arg_c_opt
  | Y_maybe_unused
  | Y_nodiscard attribute_str_arg_c_opt
  | Y_noreturn
  | Y_no_unique_address
  | Y_reproducible
  | Y_unsequenced
  | sname_c
    {
      if ( c_sname_count( &$1 ) > 1 ) {
        print_warning( &@1,
          "\"%s\": namespaced attributes not supported by %s\n",
          c_sname_full_name( &$1 ), CDECL
        );
      }
      else {
        char const *adj = "unknown";
        c_lang_id_t lang_ids = LANG_NONE;

        char const *const name = c_sname_local_name( &$1 );
        c_keyword_t const *const ck =
          c_keyword_find( name, c_lang_newer( opt_lang ), C_KW_CTX_ATTRIBUTE );
        if ( ck != NULL && c_tid_tpid( ck->tid ) == C_TPID_ATTR ) {
          adj = "unsupported";
          lang_ids = ck->lang_ids;
        }
        print_warning( &@1,
          "\"%s\": %s attribute%s",
          name, adj, c_lang_which( lang_ids )
        );

        print_suggestions( DYM_C_ATTRIBUTES, name );
        EPUTC( '\n' );
      }

      $$ = TA_NONE;
      c_sname_cleanup( &$1 );
    }
  | error
    {
      elaborate_error_dym( DYM_C_ATTRIBUTES, "attribute name expected" );
    }
  ;

attribute_str_arg_c_opt
  : /* empty */
  | '(' str_lit_exp rparen_exp
    {
      print_warning( &@1,
        "attribute arguments not supported by %s (ignoring)\n", CDECL
      );
      free( $2 );
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
      attr_syntax_not_supported( &@1, L_GNU___attribute__ );
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
  : Y_NAME gnu_attribute_decl_arg_list_c_opt
    {
      free( $1 );
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
  | Y_INT_LIT
  | Y_CHAR_LIT                    { free( $1 ); }
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
      attr_syntax_not_supported( &@1, L_MSC___declspec );
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
  : Y_array array_size_decl_ast of_exp decl_english_ast
    {
      c_ast_t *const  array_size_ast = $2;
      c_ast_t *const  decl_ast = $4;

      DUMP_START( "array_decl_english_ast",
                  "ARRAY array_size_num_opt OF decl_english_ast" );
      DUMP_AST( "array_size_decl_ast", array_size_ast );
      DUMP_AST( "decl_english_ast", decl_ast );

      $$ = array_size_ast;
      $$->loc = @1;
      c_ast_set_parent( decl_ast, $$ );

      DUMP_AST( "array_decl_english_ast", $$ );
      DUMP_END();
    }

  | Y_variable length_opt array_exp name_opt of_exp decl_english_ast
    {
      char     const *const name = $4;
      c_ast_t *const        decl_ast = $6;

      DUMP_START( "array_decl_english_ast",
                  "VARIABLE LENGTH ARRAY name_opt OF decl_english_ast" );
      DUMP_STR( "name_opt", name );
      DUMP_AST( "decl_english_ast", decl_ast );

      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      if ( name == NULL ) {
        $$->array.kind = C_ARRAY_VLA_STAR;
      } else {
        $$->array.kind = C_ARRAY_NAMED_SIZE;
        $$->array.size_name = name;
      }
      c_ast_set_parent( decl_ast, $$ );

      DUMP_AST( "array_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

array_size_decl_ast
  : /* empty */
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_EMPTY_SIZE;
    }
  | Y_INT_LIT
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_INT_SIZE;
      $$->array.size_int = $1;
    }
  | Y_NAME
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_NAMED_SIZE;
      $$->array.size_name = $1;
    }
  | '*'
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->array.kind = C_ARRAY_VLA_STAR;
    }
  ;

length_opt
  : /* empty */
  | Y_length
  ;

/// English block declaration (Apple extension) ///////////////////////////////

block_decl_english_ast                  // Apple extension
  : // in_attr: qualifier
    Y_Apple_block paren_param_decl_list_english_opt returning_english_ast_opt
    {
      c_ast_t *const ret_ast = $3;

      DUMP_START( "block_decl_english_ast",
                  "BLOCK paren_param_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_AST_LIST( "paren_param_decl_list_english_opt", $2 );
      DUMP_AST( "returning_english_ast_opt", ret_ast );

      $$ = c_ast_new_gc( K_APPLE_BLOCK, &@$ );
      $$->block.param_ast_list = slist_move( &$2 );
      c_ast_set_parent( ret_ast, $$ );

      DUMP_AST( "block_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

/// English C++ constructor declaration ///////////////////////////////////////

constructor_decl_english_ast
  : Y_constructor paren_param_decl_list_english_opt
    {
      DUMP_START( "constructor_decl_english_ast",
                  "CONSTRUCTOR paren_param_decl_list_english_opt" );
      DUMP_AST_LIST( "paren_param_decl_list_english_opt", $2 );

      $$ = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      $$->ctor.param_ast_list = slist_move( &$2 );

      DUMP_AST( "constructor_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

/// English C++ destructor declaration ////////////////////////////////////////

destructor_decl_english_ast
  : Y_destructor parens_opt
    {
      DUMP_START( "destructor_decl_english_ast", "DESTRUCTOR" );

      $$ = c_ast_new_gc( K_DESTRUCTOR, &@$ );

      DUMP_AST( "destructor_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

parens_opt
  : /* empty */
  | '(' rparen_exp
  ;

/// English C/C++ function declaration ////////////////////////////////////////

func_decl_english_ast
  : // in_attr: qualifier
    func_qualifier_english_type_opt member_or_non_member_opt Y_function
    paren_param_decl_list_english_opt returning_english_ast_opt
    {
      c_type_t const      func_qual_type = $1;
      c_func_mbr_t const  mbr = $2;
      c_ast_t *const      ret_ast = $5;

      DUMP_START( "func_decl_english_ast",
                  "ref_qualifier_english_stid_opt "
                  "member_or_non_member_opt "
                  "FUNCTION paren_param_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_TYPE( "func_qualifier_english_type_opt", func_qual_type );
      DUMP_INT( "member_or_non_member_opt", mbr );
      DUMP_AST_LIST( "paren_param_decl_list_english_opt", $4 );
      DUMP_AST( "returning_english_ast_opt", ret_ast );

      $$ = c_ast_new_gc( K_FUNCTION, &@$ );
      $$->type = func_qual_type;
      $$->func.param_ast_list = slist_move( &$4 );
      $$->func.mbr = mbr;
      c_ast_set_parent( ret_ast, $$ );

      DUMP_AST( "func_decl_english_ast", $$ );
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

/// English C++ operator declaration //////////////////////////////////////////

oper_decl_english_ast
  : // in_attr: operator
    type_qualifier_list_english_type_opt ref_qualifier_english_stid_opt
    member_or_non_member_opt operator_exp paren_param_decl_list_english_opt
    returning_english_ast_opt
    {
      c_type_t const      qual_type = $1;
      c_tid_t  const      ref_qual_stid = $2;
      c_func_mbr_t const  mbr = $3;
      c_ast_t *const      ret_ast = $6;

      DUMP_START( "oper_decl_english_ast",
                  "type_qualifier_list_english_type_opt "
                  "ref_qualifier_english_stid_opt "
                  "member_or_non_member_opt "
                  "OPERATOR paren_param_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_STR( "in_attr__operator", in_attr.operator->literal );
      DUMP_TYPE( "type_qualifier_list_english_type_opt", qual_type );
      DUMP_TID( "ref_qualifier_english_stid_opt", ref_qual_stid );
      DUMP_INT( "member_or_non_member_opt", mbr );
      DUMP_AST_LIST( "paren_param_decl_list_english_opt", $5 );
      DUMP_AST( "returning_english_ast_opt", ret_ast );

      $$ = c_ast_new_gc( K_OPERATOR, &@$ );
      PARSE_ASSERT( c_type_add( &$$->type, &qual_type, &@1 ) );
      PARSE_ASSERT( c_type_add_tid( &$$->type, ref_qual_stid, &@2 ) );
      $$->oper.operator = in_attr.operator;
      $$->oper.param_ast_list = slist_move( &$5 );
      $$->oper.mbr = mbr;
      c_ast_set_parent( ret_ast, $$ );

      DUMP_AST( "oper_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

/// English C/C++ parameter list declaration //////////////////////////////////

paren_param_decl_list_english_opt
  : /* empty */                   { slist_init( &$$ ); }
  | paren_param_decl_list_english
  ;

paren_param_decl_list_english
  : '(' param_decl_list_english_opt ')'
    {
      DUMP_START( "paren_param_decl_list_english",
                  "'(' param_decl_list_english_opt ')'" );
      DUMP_AST_LIST( "param_decl_list_english_opt", $2 );

      $$ = $2;

      DUMP_AST_LIST( "paren_param_decl_list_english", $$ );
      DUMP_END();
    }
  ;

param_decl_list_english_opt
  : /* empty */                   { slist_init( &$$ ); }
  | param_decl_list_english
  ;

param_decl_list_english
  : param_decl_list_english comma_exp decl_english_ast_exp
    {
      DUMP_START( "param_decl_list_english",
                  "param_decl_list_english ',' decl_english_ast" );
      DUMP_AST_LIST( "param_decl_list_english", $1 );
      DUMP_AST( "decl_english_ast", $3 );

      $$ = $1;
      slist_push_back( &$$, $3 );

      DUMP_AST_LIST( "param_decl_list_english", $$ );
      DUMP_END();
    }

  | decl_english_ast
    {
      DUMP_START( "param_decl_list_english", "decl_english_ast" );
      DUMP_AST( "decl_english_ast", $1 );

      if ( $1->kind == K_FUNCTION )     // see the comment in param_c_ast
        $1 = c_ast_pointer( $1, &gc_ast_list );

      slist_init( &$$ );
      slist_push_back( &$$, $1 );

      DUMP_AST_LIST( "param_decl_list_english", $$ );
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
      $$->type.btids = OPT_LANG_IS( IMPLICIT_int ) ? TB_INT : TB_VOID;

      DUMP_AST( "returning_english_ast_opt", $$ );
      DUMP_END();
    }

  | returning_english_ast
  ;

returning_english_ast
  : returning decl_english_ast
    {
      DUMP_START( "returning_english_ast", "RETURNING decl_english_ast" );
      DUMP_AST( "decl_english_ast", $2 );

      $$ = $2;

      DUMP_AST( "returning_english_ast", $$ );
      DUMP_END();
    }

  | returning error
    {
      elaborate_error( "English expected after \"returning\"" );
    }
  ;

/// English C/C++ qualified declaration ///////////////////////////////////////

qualified_decl_english_ast
  : type_qualifier_list_english_type_opt qualifiable_decl_english_ast
    {
      DUMP_START( "qualified_decl_english_ast",
                  "type_qualifier_list_english_type_opt "
                  "qualifiable_decl_english_ast" );
      DUMP_TYPE( "type_qualifier_list_english_type_opt", $1 );
      DUMP_AST( "qualifiable_decl_english_ast", $2 );

      $$ = $2;
      if ( !c_type_is_none( &$1 ) )
        $$->loc = @$;
      PARSE_ASSERT( c_type_add( &$$->type, &$1, &@1 ) );

      DUMP_AST( "qualified_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

type_qualifier_list_english_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | type_qualifier_list_english_type
  ;

type_qualifier_list_english_type
  : type_qualifier_list_english_type type_qualifier_english_type
    {
      DUMP_START( "type_qualifier_list_english_type",
                  "type_qualifier_list_english_type "
                  "type_qualifier_english_type" );
      DUMP_TYPE( "type_qualifier_list_english_type", $1 );
      DUMP_TYPE( "type_qualifier_english_type", $2 );

      $$ = $1;
      PARSE_ASSERT( c_type_add( &$$, &$2, &@2 ) );

      DUMP_TYPE( "type_qualifier_list_english_type", $$ );
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
  : Y_carries dependency_exp      { $$ = TA_CARRIES_DEPENDENCY; }
  | Y_carries_dependency
  | Y_deprecated
  | Y_maybe unused_exp            { $$ = TA_MAYBE_UNUSED; }
  | Y_maybe_unused
  | Y_no Y_discard                { $$ = TA_NODISCARD; }
  | Y_nodiscard
  | Y_no Y_return                 { $$ = TA_NORETURN; }
  | Y__Noreturn
  | Y_noreturn
  | Y_no Y_unique address_exp     { $$ = TA_NO_UNIQUE_ADDRESS; }
  | Y_no_unique_address
  | Y_reproducible
  | Y_unsequenced
  ;

storage_class_english_stid
  : Y_auto_STORAGE
  | Y_Apple___block
  | Y_const_ENG eval_expr_init_stid
    {
      $$ = $2;
    }
  | Y_consteval
  | Y_constexpr
  | Y_constinit
  | Y_default
  | Y_delete
  | Y_explicit
  | Y_export
  | Y_extern
  | Y_extern linkage_stid linkage_opt
    {
      $$ = $2;
    }
  | Y_final
  | Y_friend
  | Y_inline
  | Y_mutable
  | Y_no Y_except                 { $$ = TS_NOEXCEPT; }
  | Y_noexcept
  | Y_non_empty                   { $$ = TS_NON_EMPTY_ARRAY; }
  | Y_override
//| Y_register                          // in type_modifier_list_english_type
  | Y_static
  | Y_this
  | Y_thread local_exp            { $$ = TS_THREAD_LOCAL; }
  | Y__Thread_local
  | Y_thread_local
  | Y_throw
  | Y_typedef
  | Y_virtual
  | Y_pure virtual_stid_exp       { $$ = TS_PURE_VIRTUAL | $2; }
  ;

eval_expr_init_stid
  : Y_evaluation                  { $$ = TS_CONSTEVAL; }
  | Y_expression                  { $$ = TS_CONSTEXPR; }
  | Y_initialization              { $$ = TS_CONSTINIT; }
  //
  // Normally, this rule would be named eval_expr_init_stid_exp and there would
  // be "| error" as the last alternate.  However, this rule is only ever
  // preceded by the Y_const_ENG token in the grammar and that token is a
  // special case that's returned only when followed by one of these three
  // tokens so we can't possibly get something else here.
  //
  ;

linkage_stid
  : str_lit
    {
      bool ok = true;

      if ( strcmp( $1, "C" ) == 0 )
        $$ = TS_EXTERN_C;
      else if ( strcmp( $1, "C++" ) == 0 )
        $$ = TS_NONE;
      else {
        print_error( &@1, "\"%s\": unknown linkage language", $1 );
        print_hint( "\"C\" or \"C++\"" );
        ok = false;
      }

      free( $1 );
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
  : Y_INT_LIT
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
  | type_english_ast
  ;

/// English C/C++ pointer declaration /////////////////////////////////////////

pointer_decl_english_ast
    /*
     * Ordinary pointer declaration.
     */
  : // in_attr: qualifier
    Y_pointer to_exp decl_english_ast
    {
      DUMP_START( "pointer_decl_english_ast", "POINTER TO decl_english_ast" );
      DUMP_AST( "decl_english_ast", $3 );

      if ( $3->kind == K_NAME ) {       // see the comment in "declare_command"
        assert( !c_sname_empty( &$3->sname ) );
        print_error_unknown_name( &@3, &$3->sname );
        PARSE_ABORT();
      }

      $$ = c_ast_new_gc( K_POINTER, &@$ );
      c_ast_set_parent( $3, $$ );

      DUMP_AST( "pointer_decl_english_ast", $$ );
      DUMP_END();
    }

    /*
     * C++ pointer-to-member declaration.
     */
  | // in_attr: qualifier
    Y_pointer to_exp Y_member of_exp class_struct_union_btid_exp
    sname_english_exp decl_english_ast
    {
      c_tid_t  const  csu_btid = $5;
      c_ast_t *const  decl_ast = $7;

      DUMP_START( "pointer_to_member_decl_english",
                  "POINTER TO MEMBER OF "
                  "class_struct_union_btid_exp "
                  "sname_english decl_english_ast" );
      DUMP_TID( "class_struct_union_btid_exp", csu_btid );
      DUMP_SNAME( "sname_english_exp", $6 );
      DUMP_AST( "decl_english_ast", decl_ast );

      $$ = c_ast_new_gc( K_POINTER_TO_MEMBER, &@$ );
      $$->ptr_mbr.class_sname = c_sname_move( &$6 );
      c_ast_set_parent( decl_ast, $$ );
      PARSE_ASSERT( c_type_add_tid( &$$->type, csu_btid, &@5 ) );

      DUMP_AST( "pointer_to_member_decl_english", $$ );
      DUMP_END();
    }

  | Y_pointer to_exp error
    {
      if ( OPT_LANG_IS( CPP_ANY ) )
        elaborate_error( "type name or \"member\" expected" );
      else
        elaborate_error( "type name expected" );
    }
  ;

/// English C++ reference declaration /////////////////////////////////////////

reference_decl_english_ast
  : // in_attr: qualifier
    reference_english_ast to_exp decl_english_ast
    {
      DUMP_START( "reference_decl_english_ast",
                  "reference_english_ast TO decl_english_ast" );
      DUMP_AST( "reference_english_ast", $1 );
      DUMP_AST( "decl_english_ast", $3 );

      $$ = $1;
      $$->loc = @$;
      c_ast_set_parent( $3, $$ );

      DUMP_AST( "reference_decl_english_ast", $$ );
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

/// English C++ user-defined literal declaration //////////////////////////////

user_defined_literal_decl_english_ast
  : user_defined literal_exp lparen_exp param_decl_list_english_opt ')'
    returning_english_ast_opt
    { //
      // User-defined literals are supported only in C++11 and later.
      // (However, we always allow them in configuration files.)
      //
      // This check is better to do now in the parser rather than later in the
      // AST because it has to be done in fewer places in the code plus gives a
      // better error location.
      //
      if ( UNSUPPORTED( USER_DEFINED_LITERAL ) ) {
        print_error( &@1,
          "user-defined literal not supported%s\n",
          C_LANG_WHICH( USER_DEFINED_LITERAL )
        );
        PARSE_ABORT();
      }

      DUMP_START( "user_defined_literal_decl_english_ast",
                  "USER-DEFINED LITERAL '(' param_decl_list_english_opt ')' "
                  "returning_english_ast_opt" );
      DUMP_AST_LIST( "param_decl_list_english_opt", $4 );
      DUMP_AST( "returning_english_ast_opt", $6 );

      $$ = c_ast_new_gc( K_USER_DEF_LITERAL, &@$ );
      $$->udef_lit.param_ast_list = slist_move( &$4 );
      c_ast_set_parent( $6, $$ );

      DUMP_AST( "user_defined_literal_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

/// English C/C++ variable declaration ////////////////////////////////////////

var_decl_english_ast
    /*
     * Ordinary variable declaration.
     */
  : sname_c Y_as decl_english_ast
    {
      DUMP_START( "var_decl_english_ast", "NAME AS decl_english_ast" );
      DUMP_SNAME( "sname", $1 );
      DUMP_AST( "decl_english_ast", $3 );

      if ( $3->kind == K_NAME ) {       // see the comment in "declare_command"
        assert( !c_sname_empty( &$3->sname ) );
        print_error_unknown_name( &@3, &$3->sname );
        c_sname_cleanup( &$1 );
        PARSE_ABORT();
      }

      $$ = $3;
      $$->loc = @$;
      c_sname_set( &$$->sname, &$1 );

      DUMP_AST( "var_decl_english_ast", $$ );
      DUMP_END();
    }

    /*
     * K&R C type-less parameter declaration.
     */
  | sname_english_ast

    /*
     * Varargs declaration.
     */
  | "..."
    {
      DUMP_START( "var_decl_english_ast", "..." );

      $$ = c_ast_new_gc( K_VARIADIC, &@$ );

      DUMP_AST( "var_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  ENGLISH TYPES                                                            //
///////////////////////////////////////////////////////////////////////////////

type_english_ast
  : // in_attr: qualifier
    type_modifier_list_english_type_opt unmodified_type_english_ast
    {
      DUMP_START( "type_english_ast",
                  "type_modifier_list_english_type_opt "
                  "unmodified_type_english_ast" );
      DUMP_TYPE( "type_modifier_list_english_type_opt", $1 );
      DUMP_AST( "unmodified_type_english_ast", $2 );

      $$ = $2;
      if ( !c_type_is_none( &$1 ) ) {
        // Set the AST's location to the entire rule only if the leading
        // optional rule is actually present, otherwise @$ refers to a column
        // before $2.
        $$->loc = @$;
        PARSE_ASSERT( c_type_add( &$$->type, &$1, &@1 ) );
      }

      DUMP_AST( "type_english_ast", $$ );
      DUMP_END();
    }

  | // in_attr: qualifier
    type_modifier_list_english_type     // allows implicit int in pre-C99
    {
      DUMP_START( "type_english_ast", "type_modifier_list_english_type" );
      DUMP_TYPE( "type_modifier_list_english_type", $1 );

      // see the comment in "type_c_ast"
      c_type_t type =
        C_TYPE_LIT_B( OPT_LANG_IS( IMPLICIT_int ) ? TB_INT : TB_NONE );

      PARSE_ASSERT( c_type_add( &type, &$1, &@1 ) );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type = type;

      DUMP_AST( "type_english_ast", $$ );
      DUMP_END();
    }
  ;

type_modifier_list_english_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | type_modifier_list_english_type
  ;

type_modifier_list_english_type
  : type_modifier_list_english_type type_modifier_english_type
    {
      DUMP_START( "type_modifier_list_english_type",
                  "type_modifier_list_english_type "
                  "type_modifier_english_type" );
      DUMP_TYPE( "type_modifier_list_english_type", $1 );
      DUMP_TYPE( "type_modifier_english_type", $2 );

      $$ = $1;
      PARSE_ASSERT( c_type_add( &$$, &$2, &@2 ) );

      DUMP_TYPE( "type_modifier_list_english_type", $$ );
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
  | enum_english_ast
  | typedef_type_c_ast
  ;

builtin_type_english_ast
  : builtin_no_BitInt_english_btid
    {
      DUMP_START( "builtin_type_english_ast",
                  "builtin_no_BitInt_english_btid" );
      DUMP_TID( "builtin_no_BitInt_english_btid", $1 );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = c_tid_check( $1, C_TPID_BASE );

      DUMP_AST( "builtin_type_english_ast", $$ );
      DUMP_END();
    }
  | BitInt_english_int
    {
      DUMP_START( "builtin_type_english_ast", "BitInt_english_int" );
      DUMP_INT( "int", $1 );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = TB_BITINT;
      $$->builtin.BitInt.width = STATIC_CAST( unsigned, $1 );

      DUMP_AST( "builtin_type_english_ast", $$ );
      DUMP_END();
    }
  ;

builtin_no_BitInt_english_btid
  : Y_void
  | Y_auto_TYPE
  | Y__Bool
  | Y_bool
  | Y_char int_lit_opt
    {
      switch ( $2 ) {
        case  0: $$ = TB_CHAR    ; break;
        case  8: $$ = TB_CHAR8_T ; break;
        case 16: $$ = TB_CHAR16_T; break;
        case 32: $$ = TB_CHAR32_T; break;
        default:
          print_error( &@2, "bits must be one of 8, 16, or 32\n" );
          PARSE_ABORT();
      } // switch
    }
  | Y_char8_t
  | Y_char16_t
  | Y_char32_t
  | Y_wchar_t
  | Y_wide char_exp               { $$ = TB_WCHAR_T; }
  | Y_int
  | Y_float
  | Y_floating point_exp          { $$ = TB_FLOAT; }
  | Y_double precision_opt        { $$ = TB_DOUBLE; }
  | Y_EMC__Accum
  | Y_EMC__Fract
  ;

BitInt_english_int
  : BitInt_english Y_INT_LIT bits_opt
    {
      $$ = $2;
    }
  | BitInt_english '(' int_lit_exp rparen_exp
    {
      $$ = $3;
    }
  | BitInt_english Y_width int_lit_exp bits_opt
    {
      $$ = $3;
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

precise_opt
  : /* empty */
  | Y_precise
  ;

class_struct_union_english_ast
  : class_struct_union_btid any_sname_c_exp
    {
      DUMP_START( "class_struct_union_english_ast",
                  "class_struct_union_btid sname" );
      DUMP_TID( "class_struct_union_btid", $1 );
      DUMP_SNAME( "sname", $2 );

      $$ = c_ast_new_gc( $1 == TB_ENUM ? K_ENUM : K_CLASS_STRUCT_UNION, &@$ );
      $$->type.btids = c_tid_check( $1, C_TPID_BASE );
      $$->csu.csu_sname = c_sname_move( &$2 );

      DUMP_AST( "enum_class_struct_union_english_ast", $$ );
      DUMP_END();
    }
  ;

enum_english_ast
  : enum_btids any_sname_c_exp of_type_enum_fixed_type_english_ast_opt
    {
      DUMP_START( "enum_english_ast",
                  "enum_btids sname of_type_enum_fixed_type_english_ast_opt" );
      DUMP_TID( "enum_btids", $1 );
      DUMP_SNAME( "sname", $2 );
      DUMP_AST( "enum_fixed_type_english_ast", $3 );

      $$ = c_ast_new_gc(  K_ENUM, &@$ );
      $$->type.btids = c_tid_check( $1, C_TPID_BASE );
      $$->enum_.of_ast = $3;
      $$->enum_.enum_sname = c_sname_move( &$2 );

      DUMP_AST( "enum_english_ast", $$ );
      DUMP_END();
    }
  ;

of_type_enum_fixed_type_english_ast_opt
  : /* empty */                         { $$ = NULL; }
  | Y_of type_opt enum_fixed_type_english_ast
    {
      $$ = $3;
    }
  ;

enum_fixed_type_english_ast
  : enum_fixed_type_modifier_list_english_btid_opt
    enum_unmodified_fixed_type_english_ast
    {
      DUMP_START( "enum_fixed_type_english_ast",
                  "enum_fixed_type_modifier_list_stid" );
      DUMP_TID( "enum_fixed_type_modifier_list_stid", $1 );
      DUMP_AST( "enum_unmodified_fixed_type_english_ast", $2 );

      $$ = $2;
      if ( $1 != TB_NONE ) {            // See comment in type_english_ast.
        $$->loc = @$;
        PARSE_ASSERT( c_type_add_tid( &$$->type, $1, &@1 ) );
      }

      DUMP_AST( "enum_fixed_type_english_ast", $$ );
      DUMP_END();
    }

  | enum_fixed_type_modifier_list_english_btid
    {
      DUMP_START( "enum_fixed_type_english_ast",
                  "enum_fixed_type_modifier_list_english_btid" );
      DUMP_TID( "enum_fixed_type_modifier_list_english_btid", $1 );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = c_tid_check( $1, C_TPID_BASE );

      DUMP_AST( "enum_fixed_type_english_ast", $$ );
      DUMP_END();
    }
  ;

enum_fixed_type_modifier_list_english_btid_opt
  : /* empty */                   { $$ = TB_NONE; }
  | enum_fixed_type_modifier_list_english_btid
  ;

enum_fixed_type_modifier_list_english_btid
  : enum_fixed_type_modifier_list_english_btid enum_fixed_type_modifier_btid
    {
      DUMP_START( "enum_fixed_type_modifier_list_english_btid",
                  "enum_fixed_type_modifier_list_english_btid "
                  "enum_fixed_type_modifier_btid" );
      DUMP_TID( "enum_fixed_type_modifier_list_english_btid", $1 );
      DUMP_TID( "enum_fixed_type_modifier_btid", $2 );

      $$ = $1;
      PARSE_ASSERT( c_tid_add( &$$, $2, &@2 ) );

      DUMP_TID( "enum_fixed_type_modifier_list_english_btid", $$ );
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
  | Y_TYPEDEF_NAME
    {
      assert( c_sname_count( &$1->ast->sname ) == 1 );
      $$ = check_strdup( c_sname_local_name( &$1->ast->sname ) );
    }
  ;

any_name_exp
  : any_name
  | error
    {
      $$ = NULL;
      elaborate_error( "name expected" );
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

any_typedef
  : Y_TYPEDEF_NAME
  | Y_TYPEDEF_SNAME
  ;

name_ast
  : Y_NAME
    {
      DUMP_START( "name_ast", "NAME" );
      DUMP_STR( "NAME", $1 );

      $$ = c_ast_new_gc( K_NAME, &@$ );
      c_sname_append_name( &$$->sname, $1 );

      DUMP_AST( "name_ast", $$ );
      DUMP_END();
    }
  ;

name_exp
  : Y_NAME
  | error
    {
      $$ = NULL;
      elaborate_error( "name expected" );
    }
  ;

name_opt
  : /* empty */                   { $$ = NULL; }
  | Y_NAME
  ;

oper_sname_c_opt
  : /* empty */                   { c_sname_init( &$$ ); }

  | Y_OPERATOR_SNAME Y_COLON2
    {
      $$ = $1;
      if ( c_type_is_none( c_sname_local_type( &$$ ) ) )
        c_sname_set_local_type( &$$, &C_TYPE_LIT_B( TB_SCOPE ) );
    }
  ;

typedef_type_c_ast
  : any_typedef sub_scope_sname_c_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();
      c_ast_t const *type_for_ast = $1->ast;

      DUMP_START( "typedef_type_c_ast", "any_typedef" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_AST( "any_typedef.ast", type_for_ast );
      DUMP_SNAME( "sub_scope_sname_c_opt", $2 );

      if ( c_sname_empty( &$2 ) ) {
ttntd:  $$ = c_ast_new_gc( K_TYPEDEF, &@$ );
        $$->type.btids = TB_TYPEDEF;
        $$->tdef.for_ast = type_for_ast;
      }
      else {
        c_sname_t temp_name = c_sname_dup( &$1->ast->sname );
        c_sname_append_sname( &temp_name, &$2 );

        if ( type_ast == NULL ) {
          //
          // This is for a case like:
          //
          //      define S as struct S
          //      explain S::T x
          //
          // that is: a typedef'd type followed by ::T where T is an unknown
          // name used as a type. Just assume the T is a type and create a name
          // for it.
          //
          c_ast_t *const name_ast = c_ast_new_gc( K_NAME, &@2 );
          c_sname_set( &name_ast->sname, &temp_name );
          type_for_ast = name_ast;
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
        c_sname_set( &$$->sname, &temp_name );
      }

      DUMP_AST( "typedef_type_c_ast", $$ );
      DUMP_END();
    }
  ;

sub_scope_sname_c_opt
  : /* empty */                   { c_sname_init( &$$ ); }
  | Y_COLON2 any_sname_c          { $$ = $2; }
  ;

sname_c
  : sname_c Y_COLON2 Y_NAME
    {
      // see the comment in "of_scope_english"
      if ( UNSUPPORTED( CPP_ANY ) ) {
        print_error( &@2,
          "scoped names not supported%s\n",
          C_LANG_WHICH( CPP_ANY )
        );
        c_sname_cleanup( &$1 );
        free( $3 );
        PARSE_ABORT();
      }

      DUMP_START( "sname_c", "sname_c '::' NAME" );
      DUMP_SNAME( "sname_c", $1 );
      DUMP_STR( "name", $3 );

      $$ = $1;
      c_sname_set_local_type( &$$, &C_TYPE_LIT_B( TB_SCOPE ) );
      c_sname_append_name( &$$, $3 );

      DUMP_SNAME( "sname_c", $$ );
      DUMP_END();
    }

  | sname_c Y_COLON2 any_typedef
    { //
      // This is for a case like:
      //
      //      define S::int8_t as char
      //
      // that is: the type int8_t is an existing type in no scope being defined
      // as a distinct type in a new scope.
      //
      DUMP_START( "sname_c", "sname_c '::' any_typedef" );
      DUMP_SNAME( "sname_c", $1 );
      DUMP_AST( "any_typedef.ast", $3->ast );

      $$ = $1;
      c_sname_set_local_type( &$$, &C_TYPE_LIT_B( TB_SCOPE ) );
      c_sname_t temp_sname = c_sname_dup( &$3->ast->sname );
      c_sname_append_sname( &$$, &temp_sname );

      DUMP_SNAME( "sname_c", $$ );
      DUMP_END();
    }

  | Y_NAME
    {
      DUMP_START( "sname_c", "NAME" );
      DUMP_STR( "NAME", $1 );

      c_sname_init_name( &$$, $1 );

      DUMP_SNAME( "sname_c", $$ );
      DUMP_END();
    }
  ;

sname_c_ast
  : // in_attr: type_c_ast
    sname_c bit_field_c_uint_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();
      unsigned const bit_width = STATIC_CAST( unsigned, $2 );

      DUMP_START( "sname_c_ast", "sname_c" );
      DUMP_AST( "in_attr__type_c_ast", type_ast );
      DUMP_SNAME( "sname", $1 );
      DUMP_INT( "bit_field_c_uint_opt", bit_width );

      $$ = type_ast;
      c_sname_set( &$$->sname, &$1 );

      bool ok = true;
      //
      // This check has to be done now in the parser rather than later in the
      // AST since we need to use the builtin union member now.
      //
      if ( bit_width != 0 && (ok = c_ast_is_integral( $$ )) )
        $$->bit_field.bit_width = bit_width;

      DUMP_AST( "sname_c_ast", $$ );
      DUMP_END();

      if ( !ok ) {
        print_error( &@2,
          "bit-fields can be only of integral %stypes\n",
          OPT_LANG_IS( enum_BITFIELDS ) ? "or enumeration " : ""
        );
        PARSE_ABORT();
      }
    }
  ;

bit_field_c_uint_opt
  : /* empty */                   { $$ = 0; }
  | ':' int_lit_exp
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since we use 0 to mean "no bit-field."
      //
      if ( $2 == 0 ) {
        print_error( &@2, "bit-field width must be > 0\n" );
        PARSE_ABORT();
      }
      $$ = STATIC_CAST( unsigned, $2 );
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
  : any_sname_c of_scope_list_english_opt
    {
      DUMP_START( "sname_english", "any_sname_c of_scope_list_english_opt" );
      DUMP_SNAME( "any_sname_c", $1 );
      DUMP_SNAME( "of_scope_list_english_opt", $2 );

      c_type_t const *local_type = c_sname_local_type( &$2 );
      if ( c_type_is_none( local_type ) )
        local_type = c_sname_local_type( &$1 );
      $$ = $2;
      c_sname_append_sname( &$$, &$1 );
      c_sname_set_local_type( &$$, local_type );

      DUMP_SNAME( "sname_english", $$ );
      DUMP_END();
    }
  ;

sname_english_ast
  : Y_NAME of_scope_list_english_opt
    {
      DUMP_START( "sname_english_ast", "NAME of_scope_list_english_opt" );
      DUMP_STR( "NAME", $1 );
      DUMP_SNAME( "of_scope_list_english_opt", $2 );

      c_sname_t sname = c_sname_move( &$2 );
      c_sname_append_name( &sname, $1 );

      //
      // See if the full name is the name of a typedef'd type.
      //
      c_typedef_t const *const tdef = c_typedef_find_sname( &sname );
      if ( tdef != NULL ) {
        $$ = c_ast_new_gc( K_TYPEDEF, &@$ );
        $$->type.btids = TB_TYPEDEF;
        $$->tdef.for_ast = tdef->ast;
        c_sname_cleanup( &sname );
      } else {
        $$ = c_ast_new_gc( K_NAME, &@$ );
        c_sname_set( &$$->sname, &sname );
      }

      DUMP_AST( "sname_english_ast", $$ );
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

sname_list_english
  : sname_list_english ',' sname_english_exp
    {
      DUMP_START( "sname_list_english",
                  "sname_list_english ',' sname_english" );
      DUMP_SNAME_LIST( "sname_list_english", $1 );
      DUMP_SNAME( "sname_english", $3 );

      $$ = $1;
      c_sname_t *const temp_sname = MALLOC( c_sname_t, 1 );
      *temp_sname = c_sname_move( &$3 );
      slist_push_back( &$$, temp_sname );

      DUMP_SNAME_LIST( "sname_list_english", $$ );
      DUMP_END();
    }

  | sname_english
    {
      DUMP_START( "sname_list_english", "sname_english" );
      DUMP_SNAME( "sname_english", $1 );

      c_sname_t *const temp_sname = MALLOC( c_sname_t, 1 );
      *temp_sname = c_sname_move( &$1 );
      slist_init( &$$ );
      slist_push_back( &$$, temp_sname );

      DUMP_SNAME_LIST( "sname_list_english", $$ );
      DUMP_END();
    }
  ;

typedef_sname_c
  : typedef_sname_c Y_COLON2 sname_c
    {
      DUMP_START( "typedef_sname_c", "typedef_sname_c '::' sname_c" );
      DUMP_SNAME( "typedef_sname_c", $1 );
      DUMP_SNAME( "sname_c", $3 );

      //
      // This is for a case like:
      //
      //      define S as struct S
      //      define S::T as struct T
      //
      $$ = $1;
      c_sname_append_sname( &$$, &$3 );

      DUMP_SNAME( "typedef_sname_c", $$ );
      DUMP_END();
    }

  | typedef_sname_c Y_COLON2 any_typedef
    {
      DUMP_START( "typedef_sname_c", "typedef_sname_c '::' any_typedef" );
      DUMP_SNAME( "typedef_sname_c", $1 );
      DUMP_AST( "any_typedef", $3->ast );

      //
      // This is for a case like:
      //
      //      define S as struct S
      //      define T as struct T
      //      define S::T as struct S_T
      //
      $$ = $1;
      c_sname_set_local_type( &$$, c_sname_local_type( &$3->ast->sname ) );
      c_sname_t temp_sname = c_sname_dup( &$3->ast->sname );
      c_sname_append_sname( &$$, &temp_sname );

      DUMP_SNAME( "typedef_sname_c", $$ );
      DUMP_END();
    }

  | any_typedef                   { $$ = c_sname_dup( &$1->ast->sname ); }
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
      if ( OPT_LANG_IS( CPP_ANY ) ) {
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
  : Y_co_await                    { $$ = C_OP_CO_AWAIT          ; }
  | Y_new                         { $$ = C_OP_NEW               ; }
  | Y_new '[' rbracket_exp        { $$ = C_OP_NEW_ARRAY         ; }
  | Y_delete                      { $$ = C_OP_DELETE            ; }
  | Y_delete '[' rbracket_exp     { $$ = C_OP_DELETE_ARRAY      ; }
  | Y_EXCLAM                      { $$ = C_OP_EXCLAM            ; }
  | Y_EXCLAM_EQUAL                { $$ = C_OP_EXCLAM_EQUAL      ; }
  | '%'                           { $$ = C_OP_PERCENT           ; }
  | Y_PERCENT_EQUAL               { $$ = C_OP_PERCENT_EQUAL     ; }
  | Y_AMPER                       { $$ = C_OP_AMPER             ; }
  | Y_AMPER2                      { $$ = C_OP_AMPER2            ; }
  | Y_AMPER_EQUAL                 { $$ = C_OP_AMPER_EQUAL       ; }
  | '(' rparen_exp                { $$ = C_OP_PARENS            ; }
  | '*'                           { $$ = C_OP_STAR              ; }
  | Y_STAR_EQUAL                  { $$ = C_OP_STAR_EQUAL        ; }
  | '+'                           { $$ = C_OP_PLUS              ; }
  | Y_PLUS2                       { $$ = C_OP_PLUS2             ; }
  | Y_PLUS_EQUAL                  { $$ = C_OP_PLUS_EQUAL        ; }
  | ','                           { $$ = C_OP_COMMA             ; }
  | '-'                           { $$ = C_OP_MINUS             ; }
  | Y_MINUS2                      { $$ = C_OP_MINUS2            ; }
  | Y_MINUS_EQUAL                 { $$ = C_OP_MINUS_EQUAL       ; }
  | Y_ARROW                       { $$ = C_OP_ARROW             ; }
  | Y_ARROW_STAR                  { $$ = C_OP_ARROW_STAR        ; }
  | '.'                           { $$ = C_OP_DOT               ; }
  | Y_DOT_STAR                    { $$ = C_OP_DOT_STAR          ; }
  | '/'                           { $$ = C_OP_SLASH             ; }
  | Y_SLASH_EQUAL                 { $$ = C_OP_SLASH_EQUAL       ; }
  | Y_COLON2                      { $$ = C_OP_COLON2            ; }
  | '<'                           { $$ = C_OP_LESS              ; }
  | Y_LESS2                       { $$ = C_OP_LESS2             ; }
  | Y_LESS2_EQUAL                 { $$ = C_OP_LESS2_EQUAL       ; }
  | Y_LESS_EQUAL                  { $$ = C_OP_LESS_EQUAL        ; }
  | Y_LESS_EQUAL_GREATER          { $$ = C_OP_LESS_EQUAL_GREATER; }
  | '='                           { $$ = C_OP_EQUAL             ; }
  | Y_EQUAL2                      { $$ = C_OP_EQUAL2            ; }
  | '>'                           { $$ = C_OP_GREATER           ; }
  | Y_GREATER2                    { $$ = C_OP_GREATER2          ; }
  | Y_GREATER2_EQUAL              { $$ = C_OP_GREATER2_EQUAL    ; }
  | Y_GREATER_EQUAL               { $$ = C_OP_GREATER_EQUAL     ; }
  | Y_QMARK_COLON                 { $$ = C_OP_QMARK_COLON       ; }
  | '[' rbracket_exp              { $$ = C_OP_BRACKETS          ; }
  | Y_CARET                       { $$ = C_OP_CARET             ; }
  | Y_CARET_EQUAL                 { $$ = C_OP_CARET_EQUAL       ; }
  | Y_PIPE                        { $$ = C_OP_PIPE              ; }
  | Y_PIPE2                       { $$ = C_OP_PIPE2             ; }
  | Y_PIPE_EQUAL                  { $$ = C_OP_PIPE_EQUAL        ; }
  | Y_TILDE                       { $$ = C_OP_TILDE             ; }
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

equals_exp
  : '='
  | error
    {
      punct_expected( '=' );
    }
  ;

extern_linkage_c_stid
  : Y_extern linkage_stid         { $$ = $2; }
  | Y_extern linkage_stid '{'
    {
      print_error( &@3,
        "scoped linkage declarations not supported by %s\n", CDECL
      );
      PARSE_ABORT();
    }
  ;

extern_linkage_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | extern_linkage_c_stid
  ;

glob
  : Y_GLOB
    {
      if ( OPT_LANG_IS( C_ANY ) && strchr( $1, ':' ) != NULL ) {
        print_error( &@1, "scoped names not supported in C\n" );
        free( $1 );
        PARSE_ABORT();
      }
      $$ = $1;
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
      elaborate_error( "int[eger] expected" );
    }
  ;

int_lit_exp
  : Y_INT_LIT
  | error
    {
      elaborate_error( "integer literal expected" );
    }
  ;

int_lit_opt
  : /* empty */                   { $$ = 0; }
  | Y_INT_LIT
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
  : Y_of scope_english_type_exp any_sname_c_exp
    { //
      // Scoped names are supported only in C++.  (However, we always allow
      // them in configuration files.)
      //
      // This check is better to do now in the parser rather than later in the
      // AST because it has to be done in fewer places in the code plus gives a
      // better error location.
      //
      if ( UNSUPPORTED( CPP_ANY ) ) {
        print_error( &@2,
          "scoped names not supported%s\n",
          C_LANG_WHICH( CPP_ANY )
        );
        c_sname_cleanup( &$3 );
        PARSE_ABORT();
      }
      $$ = $3;
      c_sname_set_local_type( &$$, &$2 );
    }
  ;

of_scope_list_english
  : of_scope_list_english of_scope_english
    {
      $$ = $2;                          // "of scope X of scope Y" means Y::X
      c_sname_append_sname( &$$, &$1 );
      c_sname_fill_in_namespaces( &$$ );
      if ( !c_sname_check( &$$, &@1 ) ) {
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

quote2_exp
  : Y_QUOTE2
  | error
    {
      elaborate_error( "\"\" expected" );
    }
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
      $$ = NULL;
      elaborate_error( "string literal expected" );
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
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif /* __GNUC__ */

/**
 * @addtogroup parser-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints an additional parsing error message including a newline to standard
 * error that continues from where yyerror() left off.  Additionally:
 *
 * + If the lexer_printable_token() isn't NULL:
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
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param dym_kinds The bitwise-or of the kind(s) of things possibly meant.
 * @param format A `printf()` style format string.  It _must not_ end in a
 * newline since this function prints its own newline.
 * @param ... Arguments to print.
 *
 * @sa #elaborate_error()
 * @sa #elaborate_error_dym()
 * @sa fl_keyword_expected()
 * @sa fl_punct_expected()
 * @sa yyerror()
 */
PJL_PRINTF_LIKE_FUNC(4)
static void fl_elaborate_error( char const *file, int line,
                                dym_kind_t dym_kinds, char const *format,
                                ... ) {
  assert( format != NULL );

  EPUTS( ": " );
  print_debug_file_line( file, line );

  char const *const error_token = lexer_printable_token();
  if ( error_token != NULL ) {
    EPRINTF( "\"%s\"", error_token );
#ifdef ENABLE_CDECL_DEBUG
    if ( opt_cdecl_debug ) {
      switch ( yychar ) {
        case YYEMPTY:
          EPUTS( " [<empty>]" );
          break;
        case YYEOF:
          EPUTS( " [<EOF>]" );
          break;
        default:
          EPRINTF( " [%d]", yychar );
      } // switch
    }
#endif /* ENABLE_CDECL_DEBUG */
    EPUTS( ": " );
  }

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );

  if ( error_token != NULL ) {
    print_is_a_keyword( error_token );
    print_suggestions( dym_kinds, error_token );
  }

  EPUTC( '\n' );
}

////////// extern functions ///////////////////////////////////////////////////

void parser_cleanup( void ) {
  c_ast_list_gc( &typedef_ast_list );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
