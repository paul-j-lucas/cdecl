/*
**      cdecl -- C gibberish translator
**      src/parser.y
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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

%expect 17

%{
/** @endcond */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_ast.h"
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
#include "check.h"
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

#ifdef __GNUC__
// Silence these warnings for Bison-generated code.
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunreachable-code"
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
#define IF_DEBUG(...)             /* nothing */
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
 * Calls c_ast_check(): if the check fails, calls #PARSE_ABORT().
 *
 * @param AST The AST to check.
 */
#define C_AST_CHECK(AST) \
  BLOCK( if ( !c_ast_check( AST ) ) PARSE_ABORT(); )

/**
 * Calls c_type_add(): if adding the type fails, calls #PARSE_ABORT().
 *
 * @param DST_TYPE The \ref c_type to add to.
 * @param NEW_TYPE The \ref c_type to add.
 * @param NEW_LOC The source location of \a NEW_TYPE.
 *
 * @sa #C_TID_ADD()
 * @sa #C_TYPE_ADD_TID()
 */
#define C_TYPE_ADD(DST_TYPE,NEW_TYPE,NEW_LOC) BLOCK( \
  if ( !c_type_add( (DST_TYPE), (NEW_TYPE), &(NEW_LOC) ) ) PARSE_ABORT(); )

/**
 * Calls c_type_add_tid(): if adding the type fails, calls #PARSE_ABORT().
 *
 * @param DST_TYPE The \ref c_type to add to.
 * @param NEW_TID The \ref c_tid_t to add.
 * @param NEW_LOC The source location of \a NEW_TID.
 *
 * @sa #C_TID_ADD()
 * @sa #C_TYPE_ADD()
 */
#define C_TYPE_ADD_TID(DST_TYPE,NEW_TID,NEW_LOC) BLOCK( \
  if ( !c_type_add_tid( (DST_TYPE), (NEW_TID), &(NEW_LOC) ) ) PARSE_ABORT(); )

/**
 * Calls c_tid_add(): if adding the type fails, calls #PARSE_ABORT().
 *
 * @param DST_TID The \ref c_tid_t to add to.
 * @param NEW_TID The \ref c_tid_t to add.
 * @param NEW_LOC The source location of \a NEW_TID.
 *
 * @sa #C_TYPE_ADD()
 * @sa #C_TYPE_ADD_TID()
 */
#define C_TID_ADD(DST_TID,NEW_TID,NEW_LOC) BLOCK( \
  if ( !c_tid_add( (DST_TID), (NEW_TID), &(NEW_LOC) ) ) PARSE_ABORT(); )

/**
 * Calls fl_is_nested_type_ok(): if it returns `false`, calls #PARSE_ABORT().
 *
 * @param TYPE_LOC The location of the type declaration.
 */
#define CHECK_NESTED_TYPE_OK(TYPE_LOC) BLOCK( \
  if ( !fl_is_nested_type_ok( __FILE__, __LINE__, TYPE_LOC ) ) PARSE_ABORT(); )

/**
 * Calls #elaborate_error_dym() with a \ref dym_kind_t of #DYM_NONE.
 *
 * @param ... Arguments passed to fl_elaborate_error().
 *
 * @note This must be used _only_ after an `error` token, e.g.:
 * @code
 *  | Y_DEFINE error
 *    {
 *      elaborate_error( "name expected" );
 *    }
 * @endcode
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
 * @param DYM_KINDS The bitwise-or of the kind(s) of things possibly meant.
 * @param ... Arguments passed to fl_elaborate_error().
 *
 * @note
 * This must be used _only_ after an `error` token, e.g.:
 * @code
 *  | error
 *    {
 *      elaborate_error_dym( DYM_COMMANDS, "unexpected token" );
 *    }
 * @endcode
 *
 * @sa elaborate_error()
 * @sa keyword_expected()
 * @sa punct_expected()
 */
#define elaborate_error_dym(DYM_KINDS,...) BLOCK( \
  fl_elaborate_error( __FILE__, __LINE__, (DYM_KINDS), __VA_ARGS__ ); PARSE_ABORT(); )

/**
 * Calls fl_keyword_expected() followed by #PARSE_ABORT().
 *
 * @param KEYWORD A keyword literal.
 *
 * @note
 * This must be used _only_ after an `error` token, e.g.:
 * @code
 *  : Y_VIRTUAL
 *  | error
 *    {
 *      keyword_expected( L_VIRTUAL );
 *    }
 * @endcode
 *
 * @sa elaborate_error()
 * @sa elaborate_error_dym()
 * @sa punct_expected()
 */
#define keyword_expected(KEYWORD) BLOCK ( \
  fl_keyword_expected( __FILE__, __LINE__, (KEYWORD) ); PARSE_ABORT(); )

/**
 * Aborts the current parse (presumably after an error message has been
 * printed).
 */
#define PARSE_ABORT()             BLOCK( parse_cleanup( true ); YYABORT; )

/**
 * Calls fl_punct_expected() followed by #PARSE_ABORT().
 *
 * @param PUNCT The punctuation character that was expected.
 *
 * @note
 * This must be used _only_ after an `error` token, e.g.:
 * @code
 *  : ','
 *  | error
 *    {
 *      punct_expected( ',' );
 *    }
 * @endcode
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
#define SHOW_USER_TYPES           (1u << 1)

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
 * Dumps a comma followed by a newline the _second_ and subsequent times it's
 * called.  It's used to separate items being dumped.
 */
#define DUMP_COMMA \
  BLOCK( if ( true_or_set( &dump_comma ) ) PUTS( ",\n" ); )

/**
 * Dumps an AST.
 *
 * @param KEY The key name to print.
 * @param AST The AST to dump.
 *
 * @sa #DUMP_AST_LIST()
 */
#define DUMP_AST(KEY,AST) IF_DEBUG( \
  if ( (AST) != NULL ) { DUMP_COMMA; c_ast_dump( (AST), 1, (KEY), stdout ); } )

/**
 * Dumps an s_list of AST.
 *
 * @param KEY The key name to print.
 * @param AST_LIST The \ref slist of AST to dump.
 *
 * @sa #DUMP_AST()
 */
#define DUMP_AST_LIST(KEY,AST_LIST) IF_DEBUG( \
  DUMP_COMMA; PUTS( "  " KEY " = " );         \
  c_ast_list_dump( &(AST_LIST), 1, stdout ); )

/**
 * Dumps a `bool`.
 *
 * @param KEY The key name to print.
 * @param BOOL The `bool` to dump.
 */
#define DUMP_BOOL(KEY,BOOL)  IF_DEBUG(  \
  DUMP_COMMA; PUTS( "  " KEY " = " );   \
  bool_dump( BOOL, stdout ); )

/**
 * Ends a dump block.
 *
 * @sa #DUMP_START()
 */
#define DUMP_END()                IF_DEBUG( PUTS( "\n}\n" ); )

/**
 * Dumps an integer.
 *
 * @param KEY The key name to print.
 * @param NUM The integer to dump.
 *
 * @sa #DUMP_STR()
 */
#define DUMP_INT(KEY,NUM) \
  IF_DEBUG( DUMP_COMMA; FPRINTF( stdout, "  " KEY " = %d", (int)(NUM) ); )

/**
 * Dumps a scoped name.
 *
 * @param KEY The key name to print.
 * @param SNAME The scoped name to dump.
 *
 * @sa #DUMP_SNAME_LIST()
 * @sa #DUMP_STR()
 */
#define DUMP_SNAME(KEY,SNAME) IF_DEBUG( \
  DUMP_COMMA; PUTS( "  " KEY " = " );   \
  c_sname_dump( &(SNAME), stdout ); )

/**
 * Dumps a list of scoped names.
 *
 * @param KEY The key name to print.
 * @param LIST The list of scoped names to dump.
 *
 * @sa #DUMP_SNAME()
 */
#define DUMP_SNAME_LIST(KEY,LIST) IF_DEBUG( \
  DUMP_COMMA; PUTS( "  " KEY " = " );       \
  c_sname_list_dump( &(LIST), stdout ); )

#ifdef ENABLE_CDECL_DEBUG
/**
 * @def DUMP_START
 *
 * Starts a dump block.  The dump block _must_ end with #DUMP_END().  If a rule
 * has a result, it should be dumped as the final thing before the #DUMP_END()
 * repeating the name of the rule, e.g.:
 * @code
 *  DUMP_START( "rule",                 // <-- This rule name ...
 *              "subrule_1 name subrule_2" );
 *  DUMP_AST( "subrule_1", $1 );
 *  DUMP_STR( "name", $2 );
 *  DUMP_AST( "subrule_2", $3 );
 *  // ...
 *  DUMP_AST( "rule", $$ );             // <-- ... is repeated here.
 *  DUMP_END();
 * @endcode
 *
 * Whenever possible, an entire dump block should be completed before any
 * actions are taken as a result of a failed semantic check or other error
 * (typically, calling print_error() and #PARSE_ABORT()) so that the entire
 * block is dumped for debugging purposes first. If necessary, set a flag
 * within the dump block, then check it after, e.g.:
 * @code
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
 * @endcode
 *
 * @param NAME The grammar production name.
 * @param PROD The grammar production rule.
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
#define DUMP_START(NAME,PROD)                           \
  bool dump_comma = false;                              \
  IF_DEBUG( PUTS( "\n" NAME " ::= " PROD " = {\n" ); )
#else
#define DUMP_START(NAME,PROD)     /* nothing */
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
  DUMP_COMMA; PUTS( "  " KEY " = " ); str_dump( (STR), stdout ); )

/**
 * Dumps a \ref c_tid_t.
 *
 * @param KEY The key name to print.
 * @param TID The \ref c_tid_t to dump.
 *
 * @sa #DUMP_TYPE()
 */
#define DUMP_TID(KEY,TID) IF_DEBUG( \
  DUMP_COMMA; PUTS( "  " KEY " = " ); c_tid_dump( (TID), stdout ); )

/**
 * Dumps a \ref c_type.
 *
 * @param KEY The key name to print.
 * @param TYPE The \ref c_type to dump.
 *
 * @sa #DUMP_TID()
 */
#define DUMP_TYPE(KEY,TYPE) IF_DEBUG( \
  DUMP_COMMA; PUTS( "  " KEY " = " ); c_type_dump( &(TYPE), stdout ); )

/** @} */

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
  c_sname_t     current_scope;    ///< C++ only: current scope, if any.
  c_ast_list_t  type_ast_stack;   ///< Type AST stack.
  c_ast_list_t  typedef_ast_list; ///< AST nodes of `typedef` being declared.
  c_ast_t      *typedef_type_ast; ///< AST of `typedef` being declared.
  bool          typename;         ///< C++ only: `typename` specified?
};
typedef struct in_attr in_attr_t;

/**
 * Qualifier and its source location.
 */
struct c_qualifier {
  c_tid_t qual_stid;                    ///< E.g., #TS_CONST or #TS_VOLATILE.
  c_loc_t loc;                          ///< Qualifier source location.
};
typedef struct c_qualifier c_qualifier_t;

/**
 * Information for show_type_visitor().
 */
struct show_type_info {
  unsigned      show_which;             ///< Predefined, user, or both?
  c_gib_flags_t gib_flags;              ///< Gibberish flags.
  c_sglob_t     sglob;                  ///< Scoped glob to match, if any.
};
typedef struct show_type_info show_type_info_t;

// local functions
PJL_WARN_UNUSED_RESULT
static bool slist_node_is_ast_placeholder( void* );

// local variables
static c_ast_depth_t  ast_depth;        ///< Parentheses nesting depth.
static slist_t        decl_ast_list;    ///< List of ASTs being declared.
static c_ast_list_t   gc_ast_list;      ///< c_ast nodes freed after parse.
static in_attr_t      in_attr;          ///< Inherited attributes.
static c_ast_list_t   typedef_ast_list; ///< c_ast nodes for `typedef`s.

////////// inline functions ///////////////////////////////////////////////////

/**
 * Garbage-collect the AST nodes on \a ast_list.
 *
 * @param ast_list The AST list to free.
 */
static inline void c_ast_list_gc( c_ast_list_t *ast_list ) {
  slist_cleanup( ast_list, (slist_free_fn_t)&c_ast_free );
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
PJL_WARN_UNUSED_RESULT
static inline c_ast_t* c_ast_new_gc( c_ast_kind_t kind, c_loc_t const *loc ) {
  return c_ast_new( kind, ast_depth, loc, &gc_ast_list );
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
PJL_WARN_UNUSED_RESULT
static inline c_ast_t* ia_type_ast_peek( void ) {
  return slist_peek_head( &in_attr.type_ast_stack );
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
PJL_NOWARN_UNUSED_RESULT
static inline c_ast_t* ia_type_ast_pop( void ) {
  return slist_pop_head( &in_attr.type_ast_stack );
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
  slist_push_head( &in_attr.type_ast_stack, ast );
}

/**
 * Gets a printable string of \ref lexer_token.
 *
 * @return Returns said string or NULL if \ref lexer_token is the empty string.
 */
PJL_WARN_UNUSED_RESULT
static inline char const* printable_token( void ) {
  switch ( lexer_token[0] ) {
    case '\0': return NULL;
    case '\n': return "\\n";
    default  : return lexer_token;
  } // switch
}

/**
 * Cleans-up all memory associated with \a sti but _not_ \a sti itself.
 *
 * @param sti The \ref show_type_info to free.
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
 * @return Returns `true` only if cdecl has been initialized and \ref opt_lang
 * is _not_ among \a lang_ids.
 */
PJL_WARN_UNUSED_RESULT
static inline bool unsupported( c_lang_id_t lang_ids ) {
  return cdecl_initialized && !opt_lang_is_any( lang_ids );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Adds a type to the global set.
 *
 * @param decl_keyword The keyword used for the declaration.
 * @param type_ast The AST of the type to add.
 * @param type_loc The location of the offending type declaration.
 * @return Returns `true` either if the type was added or it's equivalent to
 * the existing type; `false` if a different type already exists having the
 * same name.
 */
PJL_WARN_UNUSED_RESULT
static bool add_type( char const *decl_keyword, c_ast_t const *type_ast,
                      c_loc_t const *type_loc ) {
  assert( decl_keyword != NULL );
  assert( type_ast != NULL );
  assert( type_loc != NULL );

  c_typedef_t const *const old_tdef = c_typedef_add( type_ast );
  if ( old_tdef == NULL ) {             // type was added
    //
    // We have to move the AST from the gc_ast_list so it won't be garbage
    // collected at the end of the parse to a separate typedef_ast_list that's
    // freed only at program termination.
    //
    // But first, free all orphaned placeholder AST nodes.  (For a normal, non-
    // type-defining parse, this step isn't necessary since all nodes are freed
    // at the end of the parse anyway.)
    //
    slist_free_if( &gc_ast_list, &slist_node_is_ast_placeholder );
    slist_push_list_tail( &typedef_ast_list, &gc_ast_list );
  }
  else if ( old_tdef->ast != NULL ) {   // type exists and isn't equivalent
    print_error( type_loc,
      "\"%s\": \"%s\" redefinition with different type; original is: ",
      c_sname_full_name( &type_ast->sname ), decl_keyword
    );

    // The == works because this function is called with L_DEFINE.
    if ( decl_keyword == L_DEFINE ) {
      c_ast_explain_type( old_tdef->ast, stderr );
    } else {
      //
      // When printing the existing type in C/C++ as part of an error message,
      // we always want to omit the trailing semicolon.
      //
      bool const orig_semicolon = opt_semicolon;
      opt_semicolon = false;

      c_typedef_gibberish(
        // The == works because this function is called with L_USING.
        old_tdef, decl_keyword == L_USING ? C_GIB_USING : C_GIB_TYPEDEF, stderr
      );

      opt_semicolon = orig_semicolon;
    }

    return false;
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
  if ( OPT_LANG_IS(C_CPP_MIN(2X,11)) )
    print_hint( "[[...]]" );
  else
    EPUTC( '\n' );
}

/**
 * Prints an additional parsing error message including a newline to standard
 * error that continues from where yyerror() left off.  Additionally:
 *
 * + If the printable_token() isn't NULL:
 *     + Checks to see if it's a keyword: if it is, mentions that it's a
 *       keyword in the error message.
 *     + May print "did you mean ...?" \a dym_kinds suggestions.
 *
 * + In debug mode, also prints the file & line where the function was called
 *   from.
 *
 * @note This function isn't normally called directly; use the
 * #elaborate_error() macro instead.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param dym_kinds The bitwise-or of the kind(s) of things possibly meant.
 * @param format A `printf()` style format string.
 * @param ... Arguments to print.
 *
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

  char const *const error_token = printable_token();
  if ( error_token != NULL )
    EPRINTF( "\"%s\": ", error_token );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );

  if ( error_token != NULL ) {
    c_keyword_t const *const k =
      c_keyword_find( error_token, LANG_ANY, C_KW_CTX_DEFAULT );
    if ( k != NULL ) {
      c_lang_id_t const oldest_lang = c_lang_oldest( k->lang_ids );
      if ( oldest_lang > opt_lang )
        EPRINTF( "; not a keyword until %s", c_lang_name( oldest_lang ) );
      else
        EPRINTF( " (\"%s\" is a keyword)", error_token );
    }
    else if ( cdecl_mode == CDECL_ENGLISH_TO_GIBBERISH ) {
      cdecl_keyword_t const *const ck = cdecl_keyword_find( error_token );
      if ( ck != NULL )
        EPRINTF( " (\"%s\" is a cdecl keyword)", error_token );
    }

    print_suggestions( dym_kinds, error_token );
  }

  EPUTC( '\n' );
}

/**
 * Checks whether the type currently being declared (`enum`, `struct`,
 * `typedef`, or `union`) is nested within some other type (`struct` or
 * `union`) and whether the current language is C.
 *
 * @note It is unnecessary to check either when a `class` is being declared or
 * when a type is being declared within a `namespace` since those are not legal
 * in C anyway.
 *
 * @note This function isn't normally called directly; use the
 * #CHECK_NESTED_TYPE_OK() macro instead.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param type_loc The location of the type declaration.
 * @return Returns `true` only if the type currently being declared is either
 * not nested or the current language is C++.
 */
PJL_WARN_UNUSED_RESULT
static bool fl_is_nested_type_ok( char const *file, int line,
                                  c_loc_t const *type_loc ) {
  assert( type_loc != NULL );
  if ( !c_sname_empty( &in_attr.current_scope ) && OPT_LANG_IS(C_ANY) ) {
    fl_print_error( file, line, type_loc, "nested types not supported in C\n" );
    return false;
  }
  return true;
}

/**
 * A special case of fl_elaborate_error() that prevents oddly worded error
 * messages where a C/C++ keyword is expected, but that keyword isn't a keyword
 * either until a later version of the language or in a different language;
 * hence, the lexer will return the keyword as the Y_NAME token instead of the
 * keyword token.
 *
 * For example, if fl_elaborate_error() were used for the following \b cdecl
 * command when the current language is C, you'd get the following:
 * @code
 * declare f as virtual function returning void
 *              ^
 * 14: syntax error: "virtual": "virtual" expected; not a keyword until C++98
 * @endcode
 * because it's really this:
 * @code
 * ... "virtual" [the name]": "virtual" [the token] expected ...
 * @endcode
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

  char const *const error_token = printable_token();
  if ( error_token != NULL && strcmp( error_token, keyword ) == 0 ) {
    c_keyword_t const *const k =
      c_keyword_find( keyword, LANG_ANY, C_KW_CTX_DEFAULT );
    if ( k != NULL ) {
      char const *const which_lang = c_lang_which( k->lang_ids );
      if ( which_lang[0] != '\0' ) {
        EPRINTF( ": \"%s\" not supported%s\n", keyword, which_lang );
        return;
      }
    }
  }

  fl_elaborate_error( file, line, DYM_NONE, "\"%s\" expected", keyword );
}

/**
 * A special case of fl_elaborate_error() that prevents oddly worded error
 * messages when a punctuation character is expected by not doing keyword look-
 * ups of the error token.

 * For example, if fl_elaborate_error() were used for the following \b cdecl
 * command, you'd get the following:
 * @code
 * explain void f(int g const)
 *                      ^
 * 29: syntax error: "const": ',' expected ("const" is a keyword)
 * @endcode
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

  char const *const error_token = printable_token();
  if ( error_token != NULL )
    EPRINTF( "\"%s\": ", error_token );

  EPRINTF( "'%c' expected\n", punct );
}

/**
 * Frees all resources used by \ref in_attr "inherited attributes".
 */
static void ia_free( void ) {
  c_sname_cleanup( &in_attr.current_scope );
  // Do _not_ pass &c_ast_free for the 3rd argument! All AST nodes were already
  // free'd from the gc_ast_list in parse_cleanup(). Just free the slist nodes.
  slist_cleanup( &in_attr.type_ast_stack, /*free_fn=*/NULL );
  c_ast_list_gc( &in_attr.typedef_ast_list );
  MEM_ZERO( &in_attr );
}

/**
 * Cleans up individial parse data after each parse.
 *
 * @param hard_reset If `true`, does a "hard" reset that currently resets the
 * EOF flag of the lexer.  This should be `true` if an error occurs and
 * `YYABORT` is called.
 *
 * @sa parse_init()
 */
static void parse_cleanup( bool hard_reset ) {
  lexer_reset( hard_reset );
  slist_cleanup( &decl_ast_list, /*free_fn=*/NULL );
  c_ast_list_gc( &gc_ast_list );
  ia_free();
}

/**
 * Gets ready to parse a command.
 *
 * @sa parse_cleanup()
 */
static void parse_init( void ) {
  ast_depth = 0;
  cdecl_mode = CDECL_ENGLISH_TO_GIBBERISH;
}

/**
 * Implements the cdecl `quit` command.
 *
 * @note
 * This should be marked `noreturn` but isn't since that would generate a
 * warning that a `break` in the Bison-generated code won't be executed.
 */
static void quit( void ) {
  exit( EX_OK );
}

/**
 * Prints the definition of a `typedef`.
 *
 * @param tdef The \ref c_typedef to print.
 * @param data Optional data passed to the visitor: in this case, the bitmask
 * of which `typedef`s to print.
 * @return Always returns `false`.
 */
PJL_WARN_UNUSED_RESULT
static bool show_type_visitor( c_typedef_t const *tdef, void *data ) {
  assert( tdef != NULL );
  assert( data != NULL );

  show_type_info_t const *const sti = data;

  bool const show_in_lang =
    (sti->show_which & SHOW_ALL_TYPES) != 0 ||
    opt_lang_is_any( tdef->lang_ids );

  if ( show_in_lang ) {
    bool const show_type =
      ((tdef->user_defined ?
        (sti->show_which & SHOW_USER_TYPES) != 0 :
        (sti->show_which & SHOW_PREDEFINED_TYPES) != 0)) &&
      (sti->sglob.count == 0 ||
       c_sname_match( &tdef->ast->sname, &sti->sglob ));

    if ( show_type ) {
      if ( sti->gib_flags == C_GIB_NONE )
        c_ast_explain_type( tdef->ast, cdecl_fout );
      else
        c_typedef_gibberish( tdef, sti->gib_flags, cdecl_fout );
    }
  }

  return false;
}

/**
 * A predicate function for slist_free_if() that checks whether \a data (cast
 * to an AST) is a #K_PLACEHOLDER: if so, c_ast_free()s it.
 *
 * @param data The AST to check.
 * @return Returns `true` only if \a data (cast to an AST) is a #K_PLACEHOLDER.
 */
static bool slist_node_is_ast_placeholder( void *data ) {
  c_ast_t *const ast = data;
  if ( ast->kind == K_PLACEHOLDER ) {
    assert( c_ast_is_orphan( ast ) );
    c_ast_free( ast );
    return true;
  }
  return false;
}

/**
 * Initializes a show_type_info_t.
 *
 * @param sti The \ref show_type_info to initialize.
 * @param show_which Which types to show: predefined, user, or both?
 * @param glob The glob string.  May be NULL.
 * @param gib_flags The \ref c_gib_flags_t to use.
 *
 * @sa sti_cleanup()
 */
static void sti_init( show_type_info_t *sti, unsigned show_which,
                      char const *glob, c_gib_flags_t gib_flags ) {
  assert( sti != NULL );
  sti->show_which = show_which;
  sti->gib_flags = gib_flags;
  c_sglob_init( &sti->sglob );
  c_sglob_parse( glob, &sti->sglob );
}

/**
 * Prints a parsing error message to standard error.  This function is called
 * directly by Bison to print just `syntax error` (usually).
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
 * @param msg The error message to print.
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

  SGR_START_COLOR( stderr, error );
  EPUTS( msg );                         // no newline
  SGR_END_COLOR( stderr );

  parse_cleanup( false );
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
  unsigned            bitmask;    // multipurpose bitmask (used by show)
  c_cast_kind_t       cast_kind;  // C/C++ cast kind
  bool                flag;       // simple flag
  c_gib_flags_t       gib_flags;  // gibberish flags
  cdecl_help_t        help;       // type of help to print
  int                 int_val;    // integer value
  slist_t             list;       // multipurpose list
  char const         *literal;    // token L_* literal (for new-style casts)
  char               *name;       // identifier name, cf. sname
  c_oper_id_t         oper_id;    // overloaded operator ID
  c_sname_t           sname;      // scoped identifier name, cf. name
  char               *str_val;    // quoted string value
  c_typedef_t const  *tdef;       // typedef
  c_tid_t             tid;        // built-ins, storage classes, & qualifiers
  c_type_t            type;       // complete type
}

                    // cdecl commands
%token              Y_CAST
//                  Y_CLASS             // covered in C++
//                  Y_CONST             // covered in C89
%token              Y_DECLARE
%token              Y_DEFINE
%token              Y_DYNAMIC
//                  Y_EXIT              // mapped to Y_QUIT by lexer
%token              Y_EXPLAIN
%token              Y_HELP
//                  Y_INLINE            // covered in C99
//                  Y_NAMESPACE         // covered in C++
%token              Y_NO
%token              Y_QUIT
%token              Y_REINTERPRET
%token              Y_SET
%token              Y_SHOW
//                  Y_STATIC            // covered in K&R C
//                  Y_STRUCT            // covered in K&R C
//                  Y_TYPEDEF           // covered in K&R C
//                  Y_UNION             // covered in K&R C
//                  Y_USING             // covered in C++

                    // Pseudo-English
%token              Y_ALIGNED
%token              Y_ALL
%token              Y_ARRAY
%token              Y_AS
%token              Y_BITS
%token              Y_BYTES
%token              Y_COMMANDS
%token              Y_CONSTRUCTOR
%token              Y_CONVERSION
%token              Y_DESTRUCTOR
%token              Y_ENGLISH
%token              Y_EVALUATION
%token              Y_EXPRESSION
%token              Y_FUNCTION
%token              Y_INITIALIZATION
%token              Y_INTO
%token              Y_LENGTH
%token              Y_LINKAGE
%token              Y_LITERAL
%token              Y_MEMBER
%token              Y_NON_MEMBER
%token              Y_OF
%token              Y_OPTIONS
%token              Y_POINTER
%token              Y_PREDEFINED
%token              Y_PURE
%token              Y_REFERENCE
%token              Y_RETURNING
%token              Y_RVALUE
%token              Y_SCOPE
%token              Y_TO
%token              Y_USER
%token              Y_USER_DEFINED
%token              Y_VARIABLE
%token              Y_WIDTH

                    // Precedence
%nonassoc           Y_PREC_LESS_THAN_upc_layout_qualifier

                    //
                    // C and C++ operators that are single-character and have
                    // no alternative token are represented by themselves
                    // directly.  All multi-character operators or those that
                    // have an alternative token are given Y_ names.
                    //

                    // C/C++ operators: precedence 17
%left               Y_COLON2            "::"
%left               Y_COLON2_STAR       "::*"
                    // C/C++ operators: precedence 16
%token              Y_PLUS2             "++"
%token              Y_MINUS2            "--"
%left                                   '(' ')'
                                        '[' ']'
                                        '.'
                    Y_ARROW             "->"
                    // C/C++ operators: precedence 15
%right              Y_AMPER          // '&' -- also has alt. token "bitand"
                                        '*'
                    Y_EXCLAM         // '!' -- also has alt. token "not"
                 // Y_UMINUS         // '-' -- covered by '-' below
                 // Y_UPLUS          // '+' -- covered by '+' below
                    Y_SIZEOF
                    Y_TILDE          // '~' -- also has alt.token "compl"
                    // C/C++ operators: precedence 14
%left               Y_DOT_STAR          ".*"
                    Y_ARROW_STAR        "->*"
                    // C/C++ operators: precedence 13
%left                                // '*' -- covered by '*' above
                                        '/'
                                        '%'
                    // C/C++ operators: precedence 12
%left                                   '-'
                                        '+'
                    // C/C++ operators: precedence 11
%left               Y_LESS2             "<<"
                    Y_GREATER2          ">>"
                    // C/C++ operators: precedence 10
%left               Y_LESS_EQ_GREATER   "<=>"
                    // C/C++ operators: precedence 9
%left                                   '<'
                                        '>'
                    Y_LESS_EQ           "<="
                    Y_GREATER_EQ        ">="
                    // C/C++ operators: precedence 8
%left               Y_EQ2               "=="
                    Y_EXCLAM_EQ      // "!=" -- also has alt. token "not_eq"
                    // C/C++ operators: precedence 7 (covered above)
%left               Y_BIT_AND        // '&' -- covered by Y_AMPER
                    // C/C++ operators: precedence 6
%left               Y_CIRC           // '^' -- also has alt. token "xor"
                    // C/C++ operators: precedence 5
%left               Y_PIPE           // '|' -- also has alt. token "bitor"
                    // C/C++ operators: precedence 4
%left               Y_AMPER2         // "&&" -- also has alt. token "and"
                    // C/C++ operators: precedence 3
%left               Y_PIPE2          // "||" -- also has alt. token "or"
                    // C/C++ operators: precedence 2
%right              Y_QMARK_COLON       "?:"
                                        '='
                    Y_PERCENT_EQ        "%="
                    Y_AMPER_EQ       // "&=" -- also has alt. token "and_eq"
                    Y_STAR_EQ           "*="
                    Y_PLUS_EQ           "+="
                    Y_MINUS_EQ          "-="
                    Y_SLASH_EQ          "/="
                    Y_LESS2_EQ          "<<="
                    Y_GREATER2_EQ       ">>="
                    Y_CIRC_EQ        // "^=" -- also has alt. token "xor_eq"
                    Y_PIPE_EQ        // "|=" -- also has alt. token "or_eq"
                    // C/C++ operators: precedence 1
%left                                   ','

                    // K&R C
%token  <tid>       Y_AUTO_STORAGE      // C version of "auto"
%token              Y_BREAK
%token              Y_CASE
%token  <tid>       Y_CHAR
%token              Y_CONTINUE
%token  <tid>       Y_DEFAULT
%token              Y_DO
%token  <tid>       Y_DOUBLE
%token              Y_ELSE
%token  <tid>       Y_EXTERN
%token  <tid>       Y_FLOAT
%token              Y_FOR
%token              Y_GOTO
%token              Y_IF
%token  <tid>       Y_INT
%token  <tid>       Y_LONG
%token  <tid>       Y_REGISTER
%token              Y_RETURN
%token  <tid>       Y_SHORT
%token  <tid>       Y_STATIC
%token  <tid>       Y_STRUCT
%token              Y_SWITCH
%token  <tid>       Y_TYPEDEF
%token  <tid>       Y_UNION
%token  <tid>       Y_UNSIGNED
%token              Y_WHILE

                    // C89
%token              Y_ASM
%token  <tid>       Y_CONST
%token              Y_ELLIPSIS    "..." // for varargs
%token  <tid>       Y_ENUM
%token  <tid>       Y_SIGNED
%token  <tid>       Y_VOID
%token  <tid>       Y_VOLATILE

                    // C95
%token  <tid>       Y_WCHAR_T

                    // C99
%token  <tid>       Y__BOOL
%token  <tid>       Y__COMPLEX
%token  <tid>       Y__IMAGINARY
%token  <tid>       Y_INLINE
%token  <tid>       Y_RESTRICT

                    // C11
%token              Y__ALIGNAS
%token              Y__ALIGNOF
%token  <tid>       Y__ATOMIC_QUAL      // qualifier: _Atomic type
%token  <tid>       Y__ATOMIC_SPEC      // specifier: _Atomic (type)
%token              Y__GENERIC
%token  <tid>       Y__NORETURN
%token              Y__STATIC_ASSERT
%token  <tid>       Y__THREAD_LOCAL
%token              Y_THREAD Y_LOCAL

                    // C++
%token  <tid>       Y_BOOL
%token              Y_CATCH
%token  <tid>       Y_CLASS
%token  <literal>   Y_CONST_CAST
%token  <sname>     Y_CONSTRUCTOR_SNAME // e.g., S::T::T
%token  <tid>       Y_DELETE
%token  <sname>     Y_DESTRUCTOR_SNAME  // e.g., S::T::~T
%token  <literal>   Y_DYNAMIC_CAST
%token  <tid>       Y_EXPLICIT
%token  <tid>       Y_FALSE             // for noexcept(false)
%token  <tid>       Y_FRIEND
%token  <tid>       Y_MUTABLE
%token  <tid>       Y_NAMESPACE
%token              Y_NEW
%token              Y_OPERATOR
%token              Y_PRIVATE
%token              Y_PROTECTED
%token              Y_PUBLIC
%token  <literal>   Y_REINTERPRET_CAST
%token  <literal>   Y_STATIC_CAST
%token              Y_TEMPLATE
%token              Y_THIS
%token  <tid>       Y_THROW
%token  <tid>       Y_TRUE              // for noexcept(true)
%token              Y_TRY
%token              Y_TYPEID
%token  <flag>      Y_TYPENAME
%token  <tid>       Y_USING
%token  <tid>       Y_VIRTUAL

                    // C11 & C++11
%token  <tid>       Y_CHAR16_T
%token  <tid>       Y_CHAR32_T

                    // C2X & C++11
%token              Y_ATTR_BEGIN        // First '[' of "[[" for an attribute.

                    // C++11
%token              Y_ALIGNAS
%token              Y_ALIGNOF
%token  <tid>       Y_AUTO_TYPE         // C++11 version of "auto"
%token              Y_CARRIES Y_DEPENDENCY
%token  <tid>       Y_CARRIES_DEPENDENCY
%token  <tid>       Y_CONSTEXPR
%token              Y_DECLTYPE
%token              Y_EXCEPT
%token  <tid>       Y_FINAL
%token  <tid>       Y_NOEXCEPT
%token              Y_NULLPTR
%token  <tid>       Y_OVERRIDE
%token              Y_QUOTE2            // for user-defined literals
%token              Y_STATIC_ASSERT
%token  <tid>       Y_THREAD_LOCAL

                    // C2X & C++14
%token  <tid>       Y_DEPRECATED

                    // C2X & C++17
%token              Y_DISCARD
%token  <tid>       Y_MAYBE_UNUSED
%token              Y_MAYBE Y_UNUSED
%token  <tid>       Y_NODISCARD

                    // C++17
%token  <tid>       Y_NORETURN

                    // C2X & C++20
%token  <tid>       Y_CHAR8_T

                    // C++20
%token              Y_ADDRESS
%token              Y_CONCEPT
%token  <tid>       Y_CONSTEVAL
%token  <tid>       Y_CONSTINIT
%token              Y_CO_AWAIT
%token              Y_CO_RETURN
%token              Y_CO_YIELD
%token  <tid>       Y_EXPORT
%token  <tid>       Y_NO_UNIQUE_ADDRESS
%token              Y_REQUIRES
%token              Y_UNIQUE

                    // Embedded C extensions
%token  <tid>       Y_EMC__ACCUM
%token  <tid>       Y_EMC__FRACT
%token  <tid>       Y_EMC__SAT

                    // Unified Parallel C extensions
%token  <tid>       Y_UPC_RELAXED
%token  <tid>       Y_UPC_SHARED
%token  <tid>       Y_UPC_STRICT

                    // GNU extensions
%token              Y_GNU___ATTRIBUTE__
%token  <tid>       Y_GNU___RESTRICT

                    // Apple extensions
%token  <tid>       Y_APPLE___BLOCK     // __block storage class
%token              Y_APPLE_BLOCK       // English for '^'

                    // Microsoft extensions
%token  <tid>       Y_MSC___CDECL
%token  <tid>       Y_MSC___CLRCALL
%token              Y_MSC___DECLSPEC
%token  <tid>       Y_MSC___FASTCALL
%token  <tid>       Y_MSC___STDCALL
%token  <tid>       Y_MSC___THISCALL
%token  <tid>       Y_MSC___VECTORCALL

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
//      + <bitmask>: "_mask" is appended.
//      + <flag>: "_flag" is appended.
//      + <name>: "_name" is appended.
//      + <int_val>: "_int" is appended.
//      + <literal>: "_literal" is appended.
//      + <sname>: "_sname" is appended.
//      + <tid>: "_[bsa]tid" is appended.
//      + <type>: "_type" is appended.
//  4. Is expected, "_exp" is appended; is optional, "_opt" is appended.
//
// Sort using: sort -bdk3

                    // Pseudo-English
%type   <ast>       alignas_or_width_decl_english_ast
%type   <align>     alignas_specifier_english
%type   <ast>       array_decl_english_ast
%type   <tid>       array_qualifier_list_english_stid
%type   <tid>       array_qualifier_list_english_stid_opt
%type   <int_val>   array_size_int_opt
%type   <tid>       attribute_english_atid
%type   <ast>       block_decl_english_ast
%type   <ast>       constructor_decl_english_ast
%type   <ast>       decl_english_ast
%type   <ast_list>  decl_list_english decl_list_english_opt
%type   <ast>       destructor_decl_english_ast
%type   <ast>       enum_class_struct_union_english_ast
%type   <ast>       enum_fixed_type_english_ast
%type   <tid>       enum_fixed_type_modifier_list_english_btid
%type   <tid>       enum_fixed_type_modifier_list_english_btid_opt
%type   <ast>       enum_unmodified_fixed_type_english_ast
%type   <ast>       func_decl_english_ast
%type   <type>      func_qualifier_english_type_opt
%type   <bitmask>   member_or_non_member_mask_opt
%type   <cast_kind> new_style_cast_english
%type   <sname>     of_scope_english
%type   <sname>     of_scope_list_english of_scope_list_english_opt
%type   <ast>       of_type_enum_fixed_type_english_ast_opt
%type   <ast>       oper_decl_english_ast
%type   <ast_list>  paren_decl_list_english paren_decl_list_english_opt
%type   <ast>       pointer_decl_english_ast
%type   <ast>       qualifiable_decl_english_ast
%type   <ast>       qualified_decl_english_ast
%type   <ast>       reference_decl_english_ast
%type   <ast>       reference_english_ast
%type   <tid>       ref_qualifier_english_stid_opt
%type   <ast>       returning_english_ast returning_english_ast_opt
%type   <type>      scope_english_type scope_english_type_exp
%type   <sname>     sname_english sname_english_exp
%type   <ast>       sname_english_ast
%type   <list>      sname_list_english
%type   <tid>       storage_class_english_stid
%type   <tid>       storage_class_list_english_stid_opt
%type   <ast>       type_english_ast
%type   <type>      type_modifier_english_type
%type   <type>      type_modifier_list_english_type
%type   <type>      type_modifier_list_english_type_opt
%type   <tid>       type_qualifier_english_stid
%type   <type>      type_qualifier_english_type
%type   <type>      type_qualifier_list_english_type
%type   <type>      type_qualifier_list_english_type_opt
%type   <tid>       udc_storage_class_english_stid
%type   <type>      udc_storage_class_english_type
%type   <type>      udc_storage_class_list_english_type_opt
%type   <ast>       unmodified_type_english_ast
%type   <ast>       user_defined_literal_decl_english_ast
%type   <ast>       var_decl_english_ast
%type   <int_val>   width_specifier_english

                    // C/C++ casts
%type   <ast_pair>  array_cast_c_astp
%type   <ast_pair>  block_cast_c_astp
%type   <ast_pair>  cast_c_astp cast_c_astp_opt cast2_c_astp
%type   <ast_pair>  func_cast_c_astp
%type   <ast_pair>  nested_cast_c_astp
%type   <cast_kind> new_style_cast_c
%type   <ast_pair>  pointer_cast_c_astp
%type   <ast_pair>  pointer_to_member_cast_c_astp
%type   <ast_pair>  reference_cast_c_astp

                    // C/C++ declarations
%type   <align>     alignas_specifier_c
%type   <sname>     any_sname_c any_sname_c_exp any_sname_c_opt
%type   <ast_pair>  array_decl_c_astp
%type   <ast>       array_size_c_ast
%type   <int_val>   array_size_c_int
%type   <ast>       atomic_builtin_typedef_type_c_ast
%type   <ast>       atomic_specifier_type_c_ast
%type   <tid>       attribute_c_atid
%type   <tid>       attribute_list_c_atid attribute_list_c_atid_opt
%type   <tid>       attribute_specifier_list_c_atid
%type   <tid>       attribute_specifier_list_c_atid_opt
%type   <int_val>   bit_field_c_int_opt
%type   <ast_pair>  block_decl_c_astp
%type   <ast_pair>  decl_c_astp decl2_c_astp
%type   <ast>       east_modified_type_c_ast
%type   <ast>       enum_class_struct_union_c_ast
%type   <ast>       enum_fixed_type_c_ast enum_fixed_type_c_ast_opt
%type   <tid>       enum_fixed_type_modifier_list_btid
%type   <tid>       enum_fixed_type_modifier_list_btid_opt
%type   <tid>       enum_fixed_type_modifier_btid
%type   <ast>       enum_unmodified_fixed_type_c_ast
%type   <tid>       extern_linkage_c_stid extern_linkage_c_stid_opt
%type   <ast_pair>  func_decl_c_astp
%type   <tid>       func_equals_c_stid_opt
%type   <tid>       func_qualifier_c_stid
%type   <tid>       func_qualifier_list_c_stid_opt
%type   <tid>       func_ref_qualifier_c_stid_opt
%type   <tid>       linkage_stid
%type   <ast_pair>  nested_decl_c_astp
%type   <tid>       noexcept_c_stid_opt
%type   <ast>       oper_c_ast
%type   <ast_pair>  oper_decl_c_astp
%type   <ast>       param_c_ast
%type   <ast_list>  param_list_c_ast param_list_c_ast_exp param_list_c_ast_opt
%type   <ast_pair>  pointer_decl_c_astp
%type   <ast_pair>  pointer_to_member_decl_c_astp
%type   <ast>       pointer_to_member_type_c_ast
%type   <ast>       pointer_type_c_ast
%type   <ast_pair>  reference_decl_c_astp
%type   <ast>       reference_type_c_ast
%type   <tid>       restrict_qualifier_c_stid
%type   <tid>       rparen_func_qualifier_list_c_stid_opt
%type   <sname>     scope_sname_c_opt sub_scope_sname_c_opt
%type   <sname>     sname_c sname_c_exp sname_c_opt
%type   <ast>       sname_c_ast
%type   <type>      storage_class_c_type
%type   <ast>       trailing_return_type_c_ast_opt
%type   <ast>       type_c_ast
%type   <sname>     typedef_sname_c
%type   <ast>       typedef_type_c_ast
%type   <ast>       typedef_type_decl_c_ast
%type   <type>      type_modifier_c_type
%type   <type>      type_modifier_list_c_type type_modifier_list_c_type_opt
%type   <tid>       type_qualifier_c_stid
%type   <tid>       type_qualifier_list_c_stid type_qualifier_list_c_stid_opt
%type   <ast_pair>  user_defined_conversion_decl_c_astp
%type   <ast>       user_defined_literal_c_ast
%type   <ast_pair>  user_defined_literal_decl_c_astp
%type   <ast>       using_decl_c_ast

                    // C++ user-defined conversions
%type   <ast>       pointer_to_member_udc_decl_c_ast
%type   <ast>       pointer_udc_decl_c_ast
%type   <ast>       reference_udc_decl_c_ast
%type   <ast>       udc_decl_c_ast udc_decl_c_ast_opt

                    // Microsoft extensions
%type   <tid>       msc_calling_convention_atid
%type   <ast_pair>  msc_calling_convention_c_astp

                    // Miscellaneous
%type   <name>      any_name any_name_exp
%type   <tdef>      any_typedef
%type   <tid>       builtin_btid
%type   <ast>       builtin_type_ast
%type   <tid>       class_struct_btid class_struct_btid_opt
%type   <tid>       class_struct_union_btid class_struct_union_btid_exp
%type   <oper_id>   c_operator
%type   <tid>       cv_qualifier_stid cv_qualifier_list_stid_opt
%type   <tid>       enum_btid enum_class_struct_union_c_btid
%type   <name>      glob glob_opt
%type   <help>      help_what_opt
%type   <tid>       inline_stid_opt
%type   <int_val>   int_exp
%type   <ast>       name_ast
%type   <name>      name_exp
%type   <tid>       namespace_btid_exp
%type   <sname>     namespace_sname_c namespace_sname_c_exp
%type   <type>      namespace_type
%type   <sname>     namespace_typedef_sname_c
%type   <tid>       no_except_bool_stid_exp
%type   <bitmask>   predefined_or_user_mask_opt
%type   <name>      set_option_value_opt
%type   <gib_flags> show_format show_format_exp show_format_opt
%type   <bitmask>   show_which_types_mask_opt
%type   <tid>       static_stid_opt
%type   <str_val>   str_lit str_lit_exp
%type   <type>      type_modifier_base_type
%type   <flag>      typename_flag_opt
%type   <tid>       virtual_stid_exp virtual_stid_opt

//
// Bison %destructors.  We don't use the <identifier> syntax because older
// versions of Bison don't support it.
//
// Clean-up of AST nodes is done via garbage collection using gc_ast_list.
//

// c_ast_list_t
%destructor { DTRACE; c_ast_list_cleanup( &$$ ); } decl_list_english
%destructor { DTRACE; c_ast_list_cleanup( &$$ ); } decl_list_english_opt
%destructor { DTRACE; c_ast_list_cleanup( &$$ ); } param_list_c_ast
%destructor { DTRACE; c_ast_list_cleanup( &$$ ); } param_list_c_ast_exp
%destructor { DTRACE; c_ast_list_cleanup( &$$ ); } param_list_c_ast_opt
%destructor { DTRACE; c_ast_list_cleanup( &$$ ); } paren_decl_list_english
%destructor { DTRACE; c_ast_list_cleanup( &$$ ); } paren_decl_list_english_opt

// name
%destructor { DTRACE; FREE( $$ ); } any_name
%destructor { DTRACE; FREE( $$ ); } any_name_exp
%destructor { DTRACE; FREE( $$ ); } glob_opt
%destructor { DTRACE; FREE( $$ ); } name_exp
%destructor { DTRACE; FREE( $$ ); } set_option_value_opt
%destructor { DTRACE; FREE( $$ ); } str_lit
%destructor { DTRACE; FREE( $$ ); } str_lit_exp
%destructor { DTRACE; FREE( $$ ); } Y_CHAR_LIT
%destructor { DTRACE; FREE( $$ ); } Y_GLOB
%destructor { DTRACE; FREE( $$ ); } Y_NAME
%destructor { DTRACE; FREE( $$ ); } Y_SET_OPTION
%destructor { DTRACE; FREE( $$ ); } Y_STR_LIT

// sname
%destructor { DTRACE; c_sname_cleanup( &$$ ); } any_sname_c
%destructor { DTRACE; c_sname_cleanup( &$$ ); } any_sname_c_exp
%destructor { DTRACE; c_sname_cleanup( &$$ ); } any_sname_c_opt
%destructor { DTRACE; c_sname_cleanup( &$$ ); } of_scope_english
%destructor { DTRACE; c_sname_cleanup( &$$ ); } of_scope_list_english
%destructor { DTRACE; c_sname_cleanup( &$$ ); } of_scope_list_english_opt
%destructor { DTRACE; c_sname_cleanup( &$$ ); } scope_sname_c_opt
%destructor { DTRACE; c_sname_cleanup( &$$ ); } sname_c
%destructor { DTRACE; c_sname_cleanup( &$$ ); } sname_c_exp
%destructor { DTRACE; c_sname_cleanup( &$$ ); } sname_c_opt
%destructor { DTRACE; c_sname_cleanup( &$$ ); } sname_english
%destructor { DTRACE; c_sname_cleanup( &$$ ); } sname_english_exp
%destructor { DTRACE; c_sname_cleanup( &$$ ); } sub_scope_sname_c_opt
%destructor { DTRACE; c_sname_cleanup( &$$ ); } typedef_sname_c
%destructor { DTRACE; c_sname_cleanup( &$$ ); } Y_CONSTRUCTOR_SNAME
%destructor { DTRACE; c_sname_cleanup( &$$ ); } Y_DESTRUCTOR_SNAME

// sname_list
%destructor { DTRACE; c_sname_list_cleanup( &$$ ); } sname_list_english

///////////////////////////////////////////////////////////////////////////////
%%

command_list
  : /* empty */
  | command_list { parse_init(); } command
    { //
      // We get here only after a successful parse, so a hard reset is not
      // needed.
      //
      parse_cleanup( false );
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
      if ( printable_token() != NULL )
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
  : Y_CAST sname_english_exp as_into_to_exp decl_english_ast
    {
      DUMP_START( "cast_command",
                  "CAST sname_english_exp as_into_to_exp decl_english_ast" );
      DUMP_SNAME( "sname_english_exp", $2 );
      DUMP_AST( "decl_english_ast", $4 );
      DUMP_END();

      $4->cast_kind = C_CAST_C;
      bool const ok = c_ast_check( $4 );
      if ( ok ) {
        FPUTC( '(', cdecl_fout );
        c_ast_gibberish( $4, C_GIB_CAST, cdecl_fout );
        FPRINTF( cdecl_fout, ")%s\n", c_sname_full_name( &$2 ) );
      }

      c_sname_cleanup( &$2 );
      if ( !ok )
        PARSE_ABORT();
    }

    /*
     * New C++-style cast.
     */
  | new_style_cast_english cast_exp sname_english_exp as_into_to_exp
    decl_english_ast
    {
      char const *const cast_literal = c_cast_gibberish( $1 );

      DUMP_START( "cast_command",
                  "new_style_cast_english CAST sname_english_exp "
                  "as_into_to_exp decl_english_ast" );
      DUMP_STR( "new_style_cast_english", cast_literal );
      DUMP_SNAME( "sname_english_exp", $3 );
      DUMP_AST( "decl_english_ast", $5 );
      DUMP_END();

      bool ok = false;

      if ( unsupported( LANG_CPP_ANY ) ) {
        print_error( &@1,
          "%s not supported%s\n",
          cast_literal, c_lang_which( LANG_CPP_ANY )
        );
      }
      else {
        $5->cast_kind = $1;
        if ( (ok = c_ast_check( $5 )) ) {
          FPRINTF( cdecl_fout, "%s<", cast_literal );
          c_ast_gibberish( $5, C_GIB_CAST, cdecl_fout );
          FPRINTF( cdecl_fout, ">(%s)\n", c_sname_full_name( &$3 ) );
        }
      }

      c_sname_cleanup( &$3 );
      if ( !ok )
        PARSE_ABORT();
    }
  ;

new_style_cast_english
  : Y_CONST                       { $$ = C_CAST_CONST;        }
  | Y_DYNAMIC                     { $$ = C_CAST_DYNAMIC;      }
  | Y_REINTERPRET                 { $$ = C_CAST_REINTERPRET;  }
  | Y_STATIC                      { $$ = C_CAST_STATIC;       }
  ;

/// declare command ///////////////////////////////////////////////////////////

declare_command
    /*
     * Common declaration, e.g.: declare x as int.
     */
  : Y_DECLARE sname_list_english as_exp storage_class_list_english_stid_opt
    alignas_or_width_decl_english_ast
    {
      if ( $5->kind == K_NAME ) {
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
        assert( !c_sname_empty( &$5->sname ) );
        print_error_unknown_name( &@5, &$5->sname );
        c_sname_list_cleanup( &$2 );
        PARSE_ABORT();
      }

      DUMP_START( "declare_command",
                  "DECLARE sname_list_english AS "
                  "storage_class_list_english_stid_opt "
                  "alignas_or_width_decl_english_ast" );
      DUMP_SNAME_LIST( "sname_list_english", $2 );
      DUMP_TID( "storage_class_list_english_stid_opt", $4 );
      DUMP_AST( "alignas_or_width_decl_english_ast", $5 );

      $5->loc = @2;

      bool ok = c_type_add_tid( &$5->type, $4, &@4 );

      DUMP_AST( "decl_english", $5 );
      DUMP_END();

      if ( ok ) {
        c_gib_flags_t gib_flags = C_GIB_DECL;
        if ( slist_len( &$2 ) > 1 )
          gib_flags |= C_GIB_MULTI_DECL;
        FOREACH_SLIST_NODE( sname_node, &$2 ) {
          c_sname_set( &$5->sname, sname_node->data );
          if ( !(ok = c_ast_check( $5 )) )
            break;
          c_ast_gibberish( $5, gib_flags, cdecl_fout );
          if ( sname_node->next != NULL ) {
            gib_flags |= C_GIB_OMIT_TYPE;
            FPUTS( ", ", cdecl_fout );
          }
        } // for
      }

      c_sname_list_cleanup( &$2 );
      if ( !ok )
        PARSE_ABORT();
      if ( opt_semicolon )
        FPUTC( ';', cdecl_fout );
      FPUTC( '\n', cdecl_fout );
    }

    /*
     * C++ overloaded operator declaration.
     */
  | Y_DECLARE c_operator
    { //
      // This check is done now in the parser rather than later in the AST
      // since it yields a better error message since otherwise it would warn
      // that "operator" is a keyword in C++98 which skims right past the
      // bigger error that operator overloading isn't supported in C.
      //
      if ( unsupported( LANG_CPP_ANY ) ) {
        print_error( &@2, "operator overloading not supported in C\n" );
        PARSE_ABORT();
      }
    }
    of_scope_list_english_opt as_exp storage_class_list_english_stid_opt
    oper_decl_english_ast
    {
      DUMP_START( "declare_command",
                  "DECLARE c_operator of_scope_list_english_opt AS "
                  "storage_class_list_english_stid_opt "
                  "oper_decl_english_ast" );
      DUMP_STR( "c_operator", c_oper_get( $2 )->name );
      DUMP_SNAME( "of_scope_list_english_opt", $4 );
      DUMP_TID( "storage_class_list_english_stid_opt", $6 );
      DUMP_AST( "oper_decl_english_ast", $7 );

      c_sname_set( &$7->sname, &$4 );
      $7->loc = @2;
      $7->as.oper.oper_id = $2;
      C_TYPE_ADD_TID( &$7->type, $6, @6 );

      DUMP_AST( "declare_command", $7 );
      DUMP_END();

      C_AST_CHECK( $7 );
      c_ast_gibberish( $7, C_GIB_DECL, cdecl_fout );
      if ( opt_semicolon )
        FPUTC( ';', cdecl_fout );
      FPUTC( '\n', cdecl_fout );
    }

    /*
     * C++ user-defined conversion operator declaration.
     */
  | Y_DECLARE udc_storage_class_list_english_type_opt cv_qualifier_list_stid_opt
    Y_USER_DEFINED conversion_exp operator_opt of_scope_list_english_opt
    returning_exp decl_english_ast
    {
      DUMP_START( "declare_command",
                  "DECLARE udc_storage_class_list_english_type_opt "
                  "cv_qualifier_list_stid_opt "
                  "USER-DEFINED CONVERSION OPERATOR "
                  "of_scope_list_english_opt "
                  "RETURNING decl_english_ast" );
      DUMP_TYPE( "udc_storage_class_list_english_type_opt", $2 );
      DUMP_TID( "cv_qualifier_list_stid_opt", $3 );
      DUMP_SNAME( "of_scope_list_english_opt", $7 );
      DUMP_AST( "decl_english_ast", $9 );

      c_ast_t *const conv_ast = c_ast_new_gc( K_USER_DEF_CONVERSION, &@$ );
      c_sname_set( &conv_ast->sname, &$7 );
      conv_ast->type = c_type_or( &$2, &C_TYPE_LIT_S( $3 ) );
      c_ast_set_parent( $9, conv_ast );

      DUMP_AST( "declare_command", conv_ast );
      DUMP_END();

      C_AST_CHECK( conv_ast );
      c_ast_gibberish( conv_ast, C_GIB_DECL, cdecl_fout );
      if ( opt_semicolon )
        FPUTC( ';', cdecl_fout );
      FPUTC( '\n', cdecl_fout );
    }

  | Y_DECLARE error
    {
      if ( OPT_LANG_IS(CPP_ANY) )
        elaborate_error( "name or %s expected", L_OPERATOR );
      else
        elaborate_error( "name expected" );
    }
  ;

storage_class_list_english_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | storage_class_list_english_stid_opt storage_class_english_stid
    {
      DUMP_START( "storage_class_list_english_stid_opt",
                  "storage_class_list_english_stid_opt "
                  "storage_class_english_stid" );
      DUMP_TID( "storage_class_list_english_stid_opt", $1 );
      DUMP_TID( "storage_class_english_stid", $2 );

      $$ = $1;
      C_TID_ADD( &$$, $2, @2 );

      DUMP_TID( "storage_class_list_english_stid_opt", $$ );
      DUMP_END();
    }
  ;

storage_class_english_stid
  : Y_AUTO_STORAGE
  | Y_APPLE___BLOCK
  | Y_CONST Y_EVALUATION          { $$ = TS_CONSTEVAL; }
  | Y_CONST Y_EXPRESSION          { $$ = TS_CONSTEXPR; }
  | Y_CONST Y_INITIALIZATION      { $$ = TS_CONSTINIT; }
  | Y_CONSTEVAL
  | Y_CONSTEXPR
  | Y_CONSTINIT
  | Y_DEFAULT
  | Y_DELETE
  | Y_EXPLICIT
  | Y_EXPORT
  | Y_EXTERN
  | Y_EXTERN linkage_stid linkage_opt
    {
      $$ = $2;
    }
  | Y_FINAL
  | Y_FRIEND
  | Y_INLINE
  | Y_MUTABLE
  | Y_NO Y_EXCEPT                 { $$ = TS_NOEXCEPT; }
  | Y_NOEXCEPT
  | Y_OVERRIDE
//| Y_REGISTER                          // in type_modifier_list_english_type
  | Y_STATIC
  | Y_THREAD local_exp            { $$ = TS_THREAD_LOCAL; }
  | Y__THREAD_LOCAL
  | Y_THREAD_LOCAL
  | Y_THROW
  | Y_TYPEDEF
  | Y_VIRTUAL
  | Y_PURE virtual_stid_exp       { $$ = TS_PURE_VIRTUAL | $2; }
  ;

attribute_english_atid
  : Y_CARRIES dependency_exp      { $$ = TA_CARRIES_DEPENDENCY; }
  | Y_CARRIES_DEPENDENCY
  | Y_DEPRECATED
  | Y_MAYBE unused_exp            { $$ = TA_MAYBE_UNUSED; }
  | Y_MAYBE_UNUSED
  | Y_NO Y_DISCARD                { $$ = TA_NODISCARD; }
  | Y_NODISCARD
  | Y_NO Y_RETURN                 { $$ = TA_NORETURN; }
  | Y__NORETURN
  | Y_NORETURN
  | Y_NO Y_UNIQUE address_exp     { $$ = TA_NO_UNIQUE_ADDRESS; }
  | Y_NO_UNIQUE_ADDRESS
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
      if ( !ok )
        PARSE_ABORT();
    }
  ;

linkage_opt
  : /* empty */
  | Y_LINKAGE
  ;

udc_storage_class_list_english_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | udc_storage_class_list_english_type_opt udc_storage_class_english_type
    {
      DUMP_START( "udc_storage_class_list_english_type_opt",
                  "udc_storage_class_list_english_type_opt "
                  "udc_storage_class_english_type" );
      DUMP_TYPE( "udc_storage_class_list_english_type_opt", $1 );
      DUMP_TYPE( "udc_storage_class_english_type", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$, &$2, @2 );

      DUMP_TYPE( "udc_storage_class_list_english_type_opt", $$ );
      DUMP_END();
    }
  ;

udc_storage_class_english_type
  : attribute_english_atid        { $$ = C_TYPE_LIT_A( $1 ); }
  | udc_storage_class_english_stid
    {
      $$ = C_TYPE_LIT_S( $1 );
    }
  ;

  /*
   * We need a seperate storage class set for user-defined conversion operators
   * without "delete" to eliminiate a shift/reduce conflict; shift:
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
udc_storage_class_english_stid
  : Y_CONSTEVAL
  | Y_CONSTEXPR
  | Y_CONSTINIT
  | Y_EXPLICIT
  | Y_EXPORT
  | Y_FINAL
  | Y_FRIEND
  | Y_INLINE
  | Y_NOEXCEPT
  | Y_OVERRIDE
  | Y_THROW
  | Y_VIRTUAL
  | Y_PURE virtual_stid_exp       { $$ = TS_PURE_VIRTUAL | $2; }
  ;

alignas_or_width_decl_english_ast
  : decl_english_ast              { $$ = $1; }

  | decl_english_ast alignas_specifier_english
    {
      $$ = $1;
      $$->align = $2;
      $$->loc = @$;
    }

  | decl_english_ast width_specifier_english
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since we need to use the builtin union member now.
      //
      if ( !c_ast_is_builtin_any( $1, TB_ANY_INTEGRAL ) ) {
        print_error( &@2, "bit-field type must be integral\n" );
        PARSE_ABORT();
      }

      $$ = $1;
      $$->loc = @$;
      $$->as.builtin.bit_width = (c_bit_width_t)$2;
    }
  ;

alignas_specifier_english
  : Y_ALIGNED as_or_to_opt Y_INT_LIT bytes_opt
    {
      $$.kind = C_ALIGNAS_EXPR;
      $$.loc = @1;
      $$.as.expr = (unsigned)$3;
    }
  | Y_ALIGNED as_or_to_opt decl_english_ast
    {
      $$.kind = C_ALIGNAS_TYPE;
      $$.loc = @1;
      $$.as.type_ast = $3;
    }
  | Y_ALIGNED as_or_to_opt error
    {
      MEM_ZERO( &$$ );
      $$.loc = @1;
      elaborate_error( "integer or type expected" );
    }
  ;

as_or_to_opt
  : /* empty */
  | Y_AS
  | Y_TO
  ;

bytes_opt
  : /* empty */
  | Y_BYTES
  ;

width_specifier_english
  : Y_WIDTH int_exp bits_opt
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since we use 0 to mean "no bit-field."
      //
      if ( $2 == 0 ) {
        print_error( &@2, "bit-field width must be > 0\n" );
        PARSE_ABORT();
      }
      $$ = $2;
    }
  ;

bits_opt
  : /* empty */
  | Y_BITS
  ;

/// define command ////////////////////////////////////////////////////////////

define_command
  : Y_DEFINE sname_english_exp as_exp storage_class_list_english_stid_opt
    decl_english_ast
    {
      DUMP_START( "define_command",
                  "DEFINE sname_english AS "
                  "storage_class_list_english_stid_opt decl_english_ast" );
      DUMP_SNAME( "sname", $2 );
      DUMP_TID( "storage_class_list_english_stid_opt", $4 );
      DUMP_AST( "decl_english_ast", $5 );

      c_sname_set( &$5->sname, &$2 );

      if ( $5->kind == K_NAME ) {       // see the comment in "declare_command"
        assert( !c_sname_empty( &$5->sname ) );
        print_error_unknown_name( &@5, &$5->sname );
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
      //  i.e., a defined type with a storage class.  Once the semantic checks
      // pass, remove the TS_TYPEDEF.
      //
      if ( !c_type_add( &$5->type, &T_TS_TYPEDEF, &@4 ) ||
           !c_type_add_tid( &$5->type, $4, &@4 ) ||
           !c_ast_check( $5 ) ) {
        PARSE_ABORT();
      }
      PJL_IGNORE_RV( c_ast_take_type_any( $5, &T_TS_TYPEDEF ) );

      if ( c_type_is_tid_any( &$5->type, TB_ANY_SCOPE ) )
        c_sname_set_local_type( &$5->sname, &$5->type );
      c_sname_fill_in_namespaces( &$5->sname );

      DUMP_AST( "defined.ast", $5 );

      if ( !c_sname_check( &$5->sname, &@2 ) )
        PARSE_ABORT();
      if ( !add_type( L_DEFINE, $5, &@5 ) )
        PARSE_ABORT();

      DUMP_END();
    }
  ;

/// explain command ///////////////////////////////////////////////////////////

explain_command
    /*
     * C-style cast.
     */
  : explain c_style_cast_c

    /*
     * New C++-style cast.
     */
  | explain new_style_cast_c

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
  | explain file_scope_constructor_decl_c

    /*
     * C++ in-class destructor declaration, e.g.: ~S().
     */
  | explain destructor_decl_c

    /*
     * C++ file scope destructor definition, e.g.: S::~S().
     */
  | explain file_scope_destructor_decl_c

    /*
     * K&R C implicit int function and C++ in-class constructor declaration.
     */
  | explain knr_func_or_constructor_decl_c

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
  | explain Y_TYPENAME { in_attr.typename = true; } typed_declaration_c

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
  | explain user_defined_conversion_decl_c_astp
    {
      DUMP_START( "explain_command",
                  "EXPLAIN user_defined_conversion_decl_c_astp" );
      DUMP_AST( "user_defined_conversion_decl_c_astp", $2.ast );
      DUMP_END();

      C_AST_CHECK( $2.ast );
      c_ast_explain_declaration( $2.ast, cdecl_fout );
    }

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

      C_TYPE_ADD_TID( &$3->type, $2, @2 );

      C_AST_CHECK( $3 );
      c_ast_explain_declaration( $3, cdecl_fout );
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
  : Y_EXPLAIN
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
  : Y_HELP help_what_opt          { print_help( $2 ); }
  ;

help_what_opt
  : /* empty */                   { $$ = CDECL_HELP_COMMANDS; }
  | Y_COMMANDS                    { $$ = CDECL_HELP_COMMANDS; }
  | Y_ENGLISH                     { $$ = CDECL_HELP_ENGLISH;  }
  | Y_OPTIONS                     { $$ = CDECL_HELP_OPTIONS;  }
  | error
    {
      elaborate_error(
        "\"%s\", \"%s\", or \"%s\" expected",
        L_COMMANDS, L_ENGLISH, L_OPTIONS
      );
    }
  ;

/// quit command //////////////////////////////////////////////////////////////

quit_command
  : Y_QUIT                        { quit(); }
  ;

/// scope (enum, class, struct, union, namespace) command /////////////////////

scoped_command
  : scoped_declaration_c
  ;

/// set command ///////////////////////////////////////////////////////////////

set_command
  : Y_SET
    {
      if ( !option_set( NULL, NULL, NULL, NULL ) )
        PARSE_ABORT();
    }
  | Y_SET set_option_list
  ;

set_option_list
  : set_option_list set_option
  | set_option
  ;

set_option
  : Y_SET_OPTION set_option_value_opt
    {
      bool const ok = option_set( $1, &@1, $2, &@2 );
      free( $1 );
      free( $2 );
      if ( !ok )
        PARSE_ABORT();
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
  : Y_SHOW any_typedef show_format_opt
    {
      DUMP_START( "show_command", "SHOW any_typedef show_format_opt" );
      DUMP_AST( "any_typedef.ast", $2->ast );
      DUMP_INT( "show_format_opt", $3 );
      DUMP_END();

      if ( $3 == C_GIB_NONE )
        c_ast_explain_type( $2->ast, cdecl_fout );
      else
        c_typedef_gibberish( $2, $3, cdecl_fout );
    }

  | Y_SHOW any_typedef Y_AS show_format_exp
    {
      DUMP_START( "show_command", "SHOW any_typedef AS show_format_exp" );
      DUMP_AST( "any_typedef.ast", $2->ast );
      DUMP_INT( "show_format_exp", $4 );
      DUMP_END();

      c_typedef_gibberish( $2, $4, cdecl_fout );
    }

  | Y_SHOW show_which_types_mask_opt glob_opt show_format_opt
    {
      show_type_info_t sti;
      sti_init( &sti, $2, $3, $4 );
      c_typedef_visit( &show_type_visitor, &sti );
      sti_cleanup( &sti );
      free( $3 );
    }

  | Y_SHOW show_which_types_mask_opt glob_opt Y_AS show_format_exp
    {
      show_type_info_t sti;
      sti_init( &sti, $2, $3, $5 );
      c_typedef_visit( &show_type_visitor, &sti );
      sti_cleanup( &sti );
      free( $3 );
    }

  | Y_SHOW Y_NAME
    {
      static char const *const type_commands_knr[] = {
        L_DEFINE, L_STRUCT, L_TYPEDEF, L_UNION, NULL
      };
      static char const *const type_commands_c[] = {
        L_DEFINE, L_ENUM, L_STRUCT, L_TYPEDEF, L_UNION, NULL
      };
      static char const *const type_commands_cpp_pre11[] = {
        L_CLASS, L_DEFINE, L_ENUM, L_STRUCT, L_TYPEDEF, L_UNION, NULL
      };
      static char const *const type_commands_cpp11[] = {
        L_CLASS, L_DEFINE, L_ENUM, L_STRUCT, L_TYPEDEF, L_UNION, L_USING, NULL
      };

      char const *const *const type_commands =
        OPT_LANG_IS(C_KNR)     ? type_commands_knr :
        OPT_LANG_IS(C_ANY)     ? type_commands_c :
        opt_lang < LANG_CPP_11 ? type_commands_cpp_pre11 :
                                 type_commands_cpp11;

      print_error( &@2, "\"%s\": not defined as type via ", $2 );
      fprint_list( stderr, type_commands, /*elt_size=*/0, /*gets=*/NULL );
      print_suggestions( DYM_C_TYPES, $2 );
      EPUTC( '\n' );
      free( $2 );
      PARSE_ABORT();
    }

  | Y_SHOW error
    {
      elaborate_error(
        "type name or \"%s\", \"%s\", or \"%s\" expected",
        L_ALL, L_PREDEFINED, L_USER
      );
    }
  ;

show_format
  : Y_ENGLISH                     { $$ = C_GIB_NONE; }
  | Y_TYPEDEF                     { $$ = C_GIB_TYPEDEF; }
  | Y_USING
    {
      if ( opt_lang < LANG_CPP_11 ) {
        print_error( &@1,
          "\"%s\" not supported%s\n",
          L_USING, c_lang_which( LANG_CPP_MIN(11) )
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
      if ( opt_lang < LANG_CPP_11 )
        elaborate_error( "\"%s\" or \"%s\" expected", L_ENGLISH, L_TYPEDEF );
      else
        elaborate_error(
          "\"%s\", \"%s\", or \"%s\" expected", L_ENGLISH, L_TYPEDEF, L_USING
        );
    }
  ;

show_format_opt
  : /* empty */                   { $$ = C_GIB_NONE; }
  | show_format
  ;

show_which_types_mask_opt
  : /* empty */                   { $$ = SHOW_USER_TYPES; }
  | Y_ALL predefined_or_user_mask_opt
    {
      $$ = SHOW_ALL_TYPES |
           ($2 != 0 ? $2 : SHOW_PREDEFINED_TYPES | SHOW_USER_TYPES);
    }
  | Y_PREDEFINED                  { $$ = SHOW_PREDEFINED_TYPES; }
  | Y_USER                        { $$ = SHOW_USER_TYPES; }
  ;

predefined_or_user_mask_opt
  : /* empty */                   { $$ = 0; }
  | Y_PREDEFINED                  { $$ = SHOW_PREDEFINED_TYPES; }
  | Y_USER                        { $$ = SHOW_USER_TYPES; }
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

c_style_cast_c
  : '(' type_c_ast
    {
      ia_type_ast_push( $2 );
    }
    cast_c_astp_opt rparen_exp sname_c_opt
    {
      ia_type_ast_pop();

      DUMP_START( "explain_command",
                  "EXPLAIN '(' type_c_ast cast_c_astp_opt ')' sname_c_opt" );
      DUMP_AST( "type_c_ast", $2 );
      DUMP_AST( "cast_c_astp_opt", $4.ast );
      DUMP_SNAME( "sname_c_opt", $6 );

      c_ast_t *const cast_ast = c_ast_patch_placeholder( $2, $4.ast );
      cast_ast->cast_kind = C_CAST_C;

      DUMP_AST( "explain_command", cast_ast );
      DUMP_END();

      bool const ok = c_ast_check( cast_ast );
      if ( ok )
        c_ast_explain_cast( &$6, cast_ast, cdecl_fout );

      c_sname_cleanup( &$6 );
      if ( !ok )
        PARSE_ABORT();
    }
  ;

/// Gibberish C++-style cast //////////////////////////////////////////////////

new_style_cast_c
  : new_style_cast_c lt_exp type_c_ast
    {
      ia_type_ast_push( $3 );
    }
    cast_c_astp_opt gt_exp lparen_exp sname_c_exp rparen_exp
    {
      ia_type_ast_pop();

      char const *const cast_literal = c_cast_english( $1 );

      DUMP_START( "explain_command",
                  "EXPLAIN new_style_cast_c"
                  "'<' type_c_ast cast_c_astp_opt '>' '(' sname ')'" );
      DUMP_STR( "new_style_cast_c", cast_literal );
      DUMP_AST( "type_c_ast", $3 );
      DUMP_AST( "cast_c_astp_opt", $5.ast );
      DUMP_SNAME( "sname", $8 );

      c_ast_t *const cast_ast = c_ast_patch_placeholder( $3, $5.ast );
      cast_ast->cast_kind = $1;

      DUMP_AST( "explain_command", cast_ast );
      DUMP_END();

      bool ok = false;

      if ( unsupported( LANG_CPP_ANY ) )
        print_error( &@1, "%s_cast not supported in C\n", cast_literal );
      else if ( (ok = c_ast_check( cast_ast )) )
        c_ast_explain_cast( &$8, cast_ast, cdecl_fout );

      c_sname_cleanup( &$8 );
      if ( !ok )
        PARSE_ABORT();
    }
  ;

new_style_cast_c
  : Y_CONST_CAST                  { $$ = C_CAST_CONST;        }
  | Y_DYNAMIC_CAST                { $$ = C_CAST_DYNAMIC;      }
  | Y_REINTERPRET_CAST            { $$ = C_CAST_REINTERPRET;  }
  | Y_STATIC_CAST                 { $$ = C_CAST_STATIC;       }
  ;

///////////////////////////////////////////////////////////////////////////////
//  C/C++ DECLARATIONS                                                       //
///////////////////////////////////////////////////////////////////////////////

/// Gibberish C/C++ aligned declaration ///////////////////////////////////////

aligned_declaration_c
  : alignas_specifier_c { in_attr.align = $1; }
    typename_flag_opt { in_attr.typename = $3; }
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
      $$.as.expr = (unsigned)$3;
    }

  | alignas lparen_exp type_c_ast { ia_type_ast_push( $3 ); }
    cast_c_astp_opt rparen_exp
    {
      ia_type_ast_pop();

      DUMP_START( "alignas_specifier_c",
                  "ALIGNAS '(' type_c_ast cast_c_astp_opt ')'" );
      DUMP_AST( "type_c_ast", $3 );
      DUMP_AST( "cast_c_astp_opt", $5.ast );
      DUMP_END();

      c_ast_t *const type_ast = c_ast_patch_placeholder( $3, $5.ast );

      $$.kind = C_ALIGNAS_TYPE;
      $$.loc = @1;
      $$.as.type_ast = type_ast;
    }

  | alignas lparen_exp error ')'
    {
      elaborate_error( "integer or type expected" );
    }
  ;

alignas
  : Y__ALIGNAS
  | Y_ALIGNAS
  ;

/// Gibberish C/C++ asm declaration ///////////////////////////////////////////

asm_declaration_c
  : Y_ASM lparen_exp str_lit_exp rparen_exp
    {
      free( $3 );
      print_error( &@1, "%s declarations not supported by %s\n", L_ASM, CDECL );
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
      CHECK_NESTED_TYPE_OK( &@1 );
      gibberish_to_english();           // see the comment in "explain"
    }
    any_sname_c_exp
    {
      c_type_t const *const cur_type =
        c_sname_local_type( &in_attr.current_scope );
      if ( c_type_is_tid_any( cur_type, TB_ANY_CLASS ) ) {
        char const *const cur_name =
          c_sname_local_name( &in_attr.current_scope );
        char const *const mbr_name = c_sname_local_name( &$3 );
        if ( strcmp( mbr_name, cur_name ) == 0 ) {
          print_error( &@3,
            "\"%s\": %s has the same name as its enclosing %s\n",
            mbr_name, L_MEMBER, c_type_name_c( cur_type )
          );
          c_sname_cleanup( &$3 );
          PARSE_ABORT();
        }
      }

      DUMP_START( "class_struct_union_declaration_c",
                  "class_struct_union_btid sname '{' "
                  "in_scope_declaration_c_opt "
                  "'}' ';'" );
      DUMP_SNAME( "(current_scope)", in_attr.current_scope );
      DUMP_TID( "class_struct_union_btid", $1 );
      DUMP_SNAME( "any_sname_c", $3 );

      c_sname_append_sname( &in_attr.current_scope, &$3 );
      c_sname_set_local_type( &in_attr.current_scope, &C_TYPE_LIT_B( $1 ) );
      if ( !c_sname_check( &in_attr.current_scope, &@3 ) )
        PARSE_ABORT();

      c_ast_t *const csu_ast = c_ast_new_gc( K_ENUM_CLASS_STRUCT_UNION, &@3 );
      csu_ast->sname = c_sname_dup( &in_attr.current_scope );
      c_sname_append_name(
        &csu_ast->as.ecsu.ecsu_sname,
        check_strdup( c_sname_local_name( &in_attr.current_scope ) )
      );
      csu_ast->type.btids = c_tid_check( $1, C_TPID_BASE );

      DUMP_AST( "class_struct_union_declaration_c", csu_ast );
      DUMP_END();

      if ( !add_type( c_tid_name_c( $1 ), csu_ast, &@1 ) )
        PARSE_ABORT();
    }
    brace_in_scope_declaration_c_opt
  ;

enum_declaration_c
    /*
     * C/C++ enum declaration, e.g.: enum E;
     */
  : enum_btid
    {
      CHECK_NESTED_TYPE_OK( &@1 );
      gibberish_to_english();           // see the comment in "explain"
    }
    any_sname_c_exp enum_fixed_type_c_ast_opt
    {
      DUMP_START( "enum_declaration_c", "enum_btid sname ;" );
      DUMP_TID( "enum_btid", $1 );
      DUMP_SNAME( "any_sname_c", $3 );
      DUMP_AST( "enum_fixed_type_c_ast_opt", $4 );

      c_sname_t enum_sname = c_sname_dup( &in_attr.current_scope );
      c_sname_append_sname( &enum_sname, &$3 );
      c_sname_set_local_type( &enum_sname, &C_TYPE_LIT_B( $1 ) );
      if ( !c_sname_check( &enum_sname, &@3 ) ) {
        c_sname_cleanup( &enum_sname );
        PARSE_ABORT();
      }

      c_ast_t *const enum_ast = c_ast_new_gc( K_ENUM_CLASS_STRUCT_UNION, &@3 );
      enum_ast->sname = enum_sname;
      enum_ast->type.btids = c_tid_check( $1, C_TPID_BASE );
      enum_ast->as.ecsu.of_ast = $4;
      c_sname_append_name(
        &enum_ast->as.ecsu.ecsu_sname,
        check_strdup( c_sname_local_name( &enum_sname ) )
      );

      DUMP_AST( "enum_declaration_c", enum_ast );
      DUMP_END();

      if ( !add_type( c_tid_name_c( $1 ), enum_ast, &@1 ) )
        PARSE_ABORT();
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
      DUMP_START( "namespace_declaration_c",
                  "namespace_type sname '{' "
                  "in_scope_declaration_c_opt "
                  "'}' [';']" );
      DUMP_SNAME( "(current_scope)", in_attr.current_scope );
      DUMP_TYPE( "namespace_type", $1 );
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
      if ( c_sname_count( &$3 ) > 1 && unsupported( LANG_CPP_MIN(17) ) ) {
        print_error( &@3,
          "nested %s declarations not supported%s\n",
          L_NAMESPACE, c_lang_which( LANG_CPP_MIN(17) )
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
          sname_first_type->stids | $1.stids,
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
          sname_local_type->btids | $1.btids,
          sname_local_type->stids,
          sname_local_type->atids
        )
      );

      c_sname_append_sname( &in_attr.current_scope, &$3 );
      if ( !c_sname_check( &in_attr.current_scope, &@3 ) )
        PARSE_ABORT();
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

  | namespace_sname_c Y_COLON2 Y_INLINE name_exp
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

  | namespace_typedef_sname_c Y_COLON2 Y_INLINE name_exp
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

/// Gibberish C++ template declaration ////////////////////////////////////////

template_declaration_c
  : Y_TEMPLATE
    {
      print_error( &@1,
        "%s declarations not supported by %s\n", L_TEMPLATE, CDECL
      );
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
  : Y_TYPEDEF typename_flag_opt
    {
      CHECK_NESTED_TYPE_OK( &@1 );
      in_attr.typename = $2;
      gibberish_to_english();           // see the comment in "explain"
    }
    type_c_ast
    {
      if ( $2 && !c_ast_is_typename_ok( $4 ) )
        PARSE_ABORT();

      // see the comment in "define_command" about TS_TYPEDEF
      C_TYPE_ADD_TID( &$4->type, TS_TYPEDEF, @4 );

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
      slist_push_list_tail( &in_attr.typedef_ast_list, &gc_ast_list );
      in_attr.typedef_type_ast = $4;
      ia_type_ast_push( c_ast_dup( in_attr.typedef_type_ast, &gc_ast_list ) );
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
      ia_type_ast_push( c_ast_dup( in_attr.typedef_type_ast, &gc_ast_list ) );
    }
    typedef_decl_c

  | typedef_decl_c
  ;

typedef_decl_c
  : // in_attr: type_c_ast
    decl_c_astp
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "typedef_decl_c", "decl_c_astp" );
      DUMP_SNAME( "(current_scope)", in_attr.current_scope );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "decl_c_astp", $1.ast );

      c_ast_t *typedef_ast;
      c_sname_t temp_sname;

      if ( $1.ast->kind == K_TYPEDEF ) {
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
          typedef_ast->sname = c_sname_dup( &$1.ast->as.tdef.for_ast->sname );
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
        typedef_ast = c_ast_patch_placeholder( type_ast, $1.ast );
        temp_sname = c_ast_take_name( $1.ast );
        c_sname_set( &typedef_ast->sname, &temp_sname );
      }

      C_AST_CHECK( typedef_ast );
      // see the comment in "define_command" about TS_TYPEDEF
      PJL_IGNORE_RV( c_ast_take_type_any( typedef_ast, &T_TS_TYPEDEF ) );

      if ( c_sname_count( &typedef_ast->sname ) > 1 ) {
        print_error( &@1,
          "%s names can not be scoped; use: %s %s { %s ... }\n",
          L_TYPEDEF, L_NAMESPACE, c_sname_scope_name( &typedef_ast->sname ),
          L_TYPEDEF
        );
        PARSE_ABORT();
      }

      temp_sname = c_sname_dup( &in_attr.current_scope );
      c_sname_prepend_sname( &typedef_ast->sname, &temp_sname );

      DUMP_AST( "typedef_decl_c", typedef_ast );
      DUMP_END();

      if ( !add_type( L_TYPEDEF, typedef_ast, &@1 ) )
        PARSE_ABORT();
    }
  ;

/// Gibberish C++ using declaration ///////////////////////////////////////////

using_declaration_c
  : using_decl_c_ast
    {
      // see the comment in "define_command" about TS_TYPEDEF
      PJL_IGNORE_RV( c_ast_take_type_any( $1, &T_TS_TYPEDEF ) );

      if ( !add_type( L_USING, $1, &@1 ) )
        PARSE_ABORT();
    }
  ;

using_decl_c_ast
  : Y_USING
    {
      gibberish_to_english();           // see the comment in "explain"
    }
    any_name_exp equals_exp type_c_ast
    {
      // see the comment in "define_command" about TS_TYPEDEF
      if ( !c_type_add_tid( &$5->type, TS_TYPEDEF, &@5 ) ) {
        free( $3 );
        PARSE_ABORT();
      }
      ia_type_ast_push( $5 );
    }
    cast_c_astp_opt
    {
      ia_type_ast_pop();

      DUMP_START( "using_decl_c_ast",
                  "USING any_name_exp '=' type_c_ast cast_c_astp_opt" );
      DUMP_SNAME( "(current_scope)", in_attr.current_scope );
      DUMP_STR( "any_name_exp", $3 );
      DUMP_AST( "type_c_ast", $5 );
      DUMP_AST( "cast_c_astp_opt", $7.ast );

      //
      // Ensure the type on the right-hand side doesn't have a name, e.g.:
      //
      //      using U = void (*F)();    // error
      //
      // This check has to be done now in the parser rather than later in the
      // AST because the patched AST loses the name.
      //
      c_sname_t const *const sname = c_ast_find_name( $7.ast, C_VISIT_DOWN );
      if ( sname != NULL ) {
        print_error( &$7.ast->loc,
          "\"%s\" type can not have a name\n", L_USING
        );
        free( $3 );
        PARSE_ABORT();
      }

      c_sname_t temp_sname = c_sname_dup( &in_attr.current_scope );
      c_sname_append_name( &temp_sname, $3 );

      $$ = c_ast_patch_placeholder( $5, $7.ast );
      c_sname_set( &$$->sname, &temp_sname );

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
      if ( unsupported( LANG_CPP_MIN(11) ) ) {
        print_error( &@1,
          "\"%s\" not supported%s\n",
          L_USING, c_lang_which( LANG_CPP_MIN(11) )
        );
        PARSE_ABORT();
      }

      C_AST_CHECK( $$ );
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
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "decl_list_c_opt", type_ast );

      if ( type_ast->kind != K_ENUM_CLASS_STRUCT_UNION ) {
        //
        // The declaration is a non-ECSU type, e.g.:
        //
        //      int
        //
        c_loc_t const loc = lexer_loc();
        print_error( &loc, "declaration expected\n" );
        PARSE_ABORT();
      }

      c_sname_t const *const ecsu_sname = &type_ast->as.ecsu.ecsu_sname;
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

      C_AST_CHECK( type_ast );
      c_ast_explain_type( type_ast, cdecl_fout );
    }

  | decl_list_c
  ;

decl_list_c
  : decl_list_c ',' decl_c
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

      DUMP_START( "decl_c", "decl_c_astp" );
      switch ( in_attr.align.kind ) {
        case C_ALIGNAS_NONE:
          break;
        case C_ALIGNAS_EXPR:
          DUMP_INT( "(alignas_specifier_c.as.expr)", in_attr.align.as.expr );
          break;
        case C_ALIGNAS_TYPE:
          DUMP_AST(
            "(alignas_specifier_c.as.type_ast)", in_attr.align.as.type_ast
          );
          break;
      } // switch
      DUMP_BOOL( "(typename_flag_opt)", in_attr.typename );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "decl_c_astp", $1.ast );

      c_ast_t const *const decl_ast = c_ast_join_type_decl(
        in_attr.typename, &in_attr.align, type_ast, $1.ast
      );

      DUMP_AST( "decl_c", decl_ast );
      DUMP_END();

      if ( decl_ast == NULL )
        PARSE_ABORT();
      C_AST_CHECK( decl_ast );

      if ( !c_sname_empty( &decl_ast->sname ) ) {
        //
        // For declarations that have a name, ensure that it's not used more
        // than once in the same declaration with different types.  (More than
        // once with the same type are "tentative definitions" and OK.)
        //
        //      int i, i;               // ok (same type)
        //      int j, *j;              // error (different types)
        //
        FOREACH_SLIST_NODE( node, &decl_ast_list ) {
          c_ast_t const *const prev_ast = node->data;
          if ( c_sname_cmp( &decl_ast->sname, &prev_ast->sname ) == 0 &&
              !c_ast_equal( decl_ast, prev_ast ) ) {
            print_error( &decl_ast->loc,
              "\"%s\": redefinition with different type\n",
              c_sname_full_name( &decl_ast->sname )
            );
            PARSE_ABORT();
          }
        } // for
        slist_push_tail( &decl_ast_list, CONST_CAST( void*, decl_ast ) );
      }

      c_ast_explain_declaration( decl_ast, cdecl_fout );

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
    decl2_c_astp array_size_c_int gnu_attribute_specifier_list_c_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "array_decl_c_astp", "decl2_c_astp array_size_c_int" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "decl2_c_astp", $1.ast );
      DUMP_AST( "target_ast", $1.target_ast );
      DUMP_INT( "array_size_c_int", $2 );

      c_ast_t *const array_ast = c_ast_new_gc( K_ARRAY, &@$ );
      array_ast->as.array.size = $2;
      c_ast_set_parent( c_ast_new_gc( K_PLACEHOLDER, &@1 ), array_ast );

      if ( $1.target_ast != NULL ) {    // array-of or function-like-ret type
        $$.ast = $1.ast;
        $$.target_ast = c_ast_add_array( $1.target_ast, array_ast, type_ast );
      } else {
        $$.ast = c_ast_add_array( $1.ast, array_ast, type_ast );
        $$.target_ast = NULL;
      }

      DUMP_AST( "array_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

array_size_c_int
  : '[' rbracket_exp              { $$ = C_ARRAY_SIZE_NONE; }
  | '[' Y_INT_LIT rbracket_exp    { $$ = $2; }
  | '[' error ']'
    {
      elaborate_error( "integer expected for array size" );
    }
  ;

/// Gibberish C/C++ block declaration (Apple extension) ///////////////////////

block_decl_c_astp                       // Apple extension
  : // in_attr: type_c_ast
    '(' Y_CIRC
    { //
      // A block AST has to be the type inherited attribute for decl_c_astp so
      // we have to create it here.
      //
      ia_type_ast_push( c_ast_new_gc( K_APPLE_BLOCK, &@$ ) );
    }
    type_qualifier_list_c_stid_opt decl_c_astp rparen_exp
    lparen_exp param_list_c_ast_opt ')' gnu_attribute_specifier_list_c_opt
    {
      c_ast_t *const block_ast = ia_type_ast_pop();
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "block_decl_c_astp",
                  "'(' '^' type_qualifier_list_c_stid_opt decl_c_astp ')' "
                  "'(' param_list_c_ast_opt ')'" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $4 );
      DUMP_AST( "decl_c_astp", $5.ast );
      DUMP_AST_LIST( "param_list_c_ast_opt", $8 );

      C_TYPE_ADD_TID( &block_ast->type, $4, @4 );
      block_ast->as.block.param_ast_list = $8;
      $$.ast = c_ast_add_func( $5.ast, block_ast, type_ast );
      $$.target_ast = block_ast->as.block.ret_ast;

      DUMP_AST( "block_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish in-class destructor declaration /////////////////////////////////

destructor_decl_c
    /*
     * C++ in-class destructor declaration, e.g.: ~S().
     */
  : virtual_stid_opt Y_TILDE any_name_exp
    lparen_exp rparen_func_qualifier_list_c_stid_opt noexcept_c_stid_opt
    gnu_attribute_specifier_list_c_opt func_equals_c_stid_opt
    {
      DUMP_START( "destructor_decl_c",
                  "virtual_opt '~' NAME '(' ')' func_qualifier_list_c_stid_opt "
                  "noexcept_c_stid_opt gnu_attribute_specifier_list_c_opt "
                  "func_equals_c_stid_opt" );
      DUMP_TID( "virtual_stid_opt", $1 );
      DUMP_STR( "any_name_exp", $3 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $5 );
      DUMP_TID( "noexcept_c_stid_opt", $6 );
      DUMP_TID( "func_equals_c_stid_opt", $8 );

      c_ast_t *const ast = c_ast_new_gc( K_DESTRUCTOR, &@$ );
      c_sname_append_name( &ast->sname, $3 );
      ast->type.stids = c_tid_check( $1 | $5 | $6 | $8, C_TPID_STORE );

      DUMP_AST( "destructor_decl_c", ast );
      DUMP_END();

      C_AST_CHECK( ast );
      c_ast_explain_declaration( ast, cdecl_fout );
    }
  ;

/// Gibberish file-scope constructor declaration //////////////////////////////

file_scope_constructor_decl_c
  : inline_stid_opt Y_CONSTRUCTOR_SNAME
    lparen_exp param_list_c_ast_opt rparen_func_qualifier_list_c_stid_opt
    noexcept_c_stid_opt gnu_attribute_specifier_list_c_opt
    {
      DUMP_START( "file_scope_constructor_decl_c",
                  "inline_opt CONSTRUCTOR_SNAME '(' param_list_c_ast_opt ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt" );
      DUMP_TID( "inline_stid_opt", $1 );
      DUMP_SNAME( "CONSTRUCTOR_SNAME", $2 );
      DUMP_AST_LIST( "param_list_c_ast_opt", $4 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $5 );
      DUMP_TID( "noexcept_c_stid_opt", $6 );

      c_sname_set_scope_type( &$2, &C_TYPE_LIT_B( TB_CLASS ) );

      c_ast_t *const ast = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      ast->sname = $2;
      ast->type.stids = c_tid_check( $1 | $5 | $6, C_TPID_STORE );
      ast->as.constructor.param_ast_list = $4;

      DUMP_AST( "file_scope_constructor_decl_c", ast );
      DUMP_END();

      C_AST_CHECK( ast );
      c_ast_explain_declaration( ast, cdecl_fout );
    }
  ;

/// Gibberish file-scope destructor declaration ///////////////////////////////

file_scope_destructor_decl_c
  : inline_stid_opt Y_DESTRUCTOR_SNAME
    lparen_exp rparen_func_qualifier_list_c_stid_opt noexcept_c_stid_opt
    gnu_attribute_specifier_list_c_opt
    {
      DUMP_START( "file_scope_destructor_decl_c",
                  "inline_opt DESTRUCTOR_SNAME '(' ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt" );
      DUMP_TID( "inline_stid_opt", $1 );
      DUMP_SNAME( "DESTRUCTOR_SNAME", $2 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $4 );
      DUMP_TID( "noexcept_c_stid_opt", $5 );

      c_sname_set_scope_type( &$2, &C_TYPE_LIT_B( TB_CLASS ) );

      c_ast_t *const ast = c_ast_new_gc( K_DESTRUCTOR, &@$ );
      ast->sname = $2;
      ast->type.stids = c_tid_check( $1 | $4 | $5, C_TPID_STORE );

      DUMP_AST( "file_scope_destructor_decl_c", ast );
      DUMP_END();

      C_AST_CHECK( ast );
      c_ast_explain_declaration( ast, cdecl_fout );
    }
  ;

/// Gibberish function declaration ////////////////////////////////////////////

func_decl_c_astp
    /*
     * Function and C++ constructor declaration.
     *
     * This grammar rule handles both since they're so similar (and so would
     * cause grammar conflicts if they were separate rules in an LALR(1)
     * parser).
     */
  : // in_attr: type_c_ast
    decl2_c_astp '(' param_list_c_ast_opt
    rparen_func_qualifier_list_c_stid_opt func_ref_qualifier_c_stid_opt
    noexcept_c_stid_opt trailing_return_type_c_ast_opt func_equals_c_stid_opt
    {
      c_ast_t *const decl2_ast = $1.ast;
      c_tid_t  const func_qualifier_stid = $4;
      c_tid_t  const func_ref_qualifier_stid = $5;
      c_tid_t  const noexcept_stid = $6;
      c_tid_t  const func_equals_stid = $8;
      c_ast_t *const trailing_ret_ast = $7;
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "func_decl_c_astp",
                  "decl2_c_astp '(' param_list_c_ast_opt ')' "
                  "func_qualifier_list_c_stid_opt "
                  "func_ref_qualifier_c_stid_opt noexcept_c_stid_opt "
                  "trailing_return_type_c_ast_opt "
                  "func_equals_c_stid_opt" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "decl2_c_astp", decl2_ast );
      DUMP_AST_LIST( "param_list_c_ast_opt", $3 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", func_qualifier_stid );
      DUMP_TID( "func_ref_qualifier_c_stid_opt", func_ref_qualifier_stid );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );
      DUMP_TID( "func_equals_c_stid_opt", func_equals_stid );
      DUMP_AST( "target_ast", $1.target_ast );

      c_tid_t const func_stid =
        func_qualifier_stid | func_ref_qualifier_stid | noexcept_stid |
        func_equals_stid;

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
        OPT_LANG_IS(CPP_ANY) &&

        // + The existing base type is none (because constructors don't have
        //   return types).
        type_ast->type.btids == TB_NONE &&

        // + The existing type does _not_ have any non-constructor storage
        //   classes.
        !c_type_is_tid_any( &type_ast->type, TS_NOT_CONSTRUCTOR ) &&

        ( // + The existing type has any constructor-only storage-class-like
          //   types (e.g., explicit).
          c_type_is_tid_any( &type_ast->type, TS_CONSTRUCTOR_ONLY ) ||

          // + Or the existing type only has storage-class-like types that may
          //   be applied to constructors.
          only_bits_set(
            c_tid_no_tpid( type_ast->type.stids ),
            c_tid_no_tpid( TS_CONSTRUCTOR_DECL )
          )
        ) &&

        // + The new type does _not_ have any non-constructor storage classes.
        !c_tid_is_any( func_stid, TS_NOT_CONSTRUCTOR );

      c_ast_t *const func_ast =
        c_ast_new_gc( assume_constructor ? K_CONSTRUCTOR : K_FUNCTION, &@$ );
      func_ast->type.stids = c_tid_check( func_stid, C_TPID_STORE );
      func_ast->as.func.param_ast_list = $3;

      if ( assume_constructor ) {
        assert( trailing_ret_ast == NULL );
        c_type_or_eq( &func_ast->type, &type_ast->type );
        $$.ast = func_ast;
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

      $$.target_ast = func_ast->as.func.ret_ast;

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

knr_func_or_constructor_decl_c
    /*
     * K&R C implicit int function and C++ in-class constructor declaration.
     *
     * This grammar rule handles both since they're so similar (and so would
     * cause grammar conflicts if they were separate rules in an LALR(1)
     * parser).
     */
  : Y_NAME '(' param_list_c_ast_opt ')' noexcept_c_stid_opt
    func_equals_c_stid_opt
    {
      if ( OPT_LANG_IS(C_MIN(99)) ) {
        //
        // In C99 and later, an implicit int function is an error.  This check
        // has to be done now in the parser rather than later in the AST since
        // the AST would have no "memory" that the return type was implicitly
        // int.
        //
        print_error( &@1,
          "implicit \"%s\" functions are illegal in %s and later\n",
          L_INT, c_lang_name( LANG_C_99 )
        );
        PARSE_ABORT();
      }

      DUMP_START( "knr_func_or_constructor_decl_c",
                  "NAME '(' param_list_c_ast_opt ')' noexcept_c_stid_opt "
                  "func_equals_c_stid_opt" );
      DUMP_STR( "NAME", $1 );
      DUMP_AST_LIST( "param_list_c_ast_opt", $3 );
      DUMP_TID( "noexcept_c_stid_opt", $5 );
      DUMP_TID( "func_equals_c_stid_opt", $6 );

      c_ast_t *ast;
      c_sname_t sname;
      c_sname_init_name( &sname, $1 );

      if ( OPT_LANG_IS(C_ANY) ) {
        //
        // In C prior to C99, encountering a name followed by '(' declares a
        // function that implicitly returns int:
        //
        //      power(x, n)             /* raise x to n-th power; n > 0 */
        //
        c_ast_t *const ret_ast = c_ast_new_gc( K_BUILTIN, &@1 );
        ret_ast->type.btids = TB_INT;

        ast = c_ast_new_gc( K_FUNCTION, &@$ );
        ast->as.func.ret_ast = ret_ast;
        ast->type.stids = c_tid_check( $5 | $6, C_TPID_STORE );
      }
      else {
        //
        // In C++, encountering a name followed by '(' declares an in-class
        // constructor.
        //
        ast = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
        ast->type.stids = c_tid_check( $5 | $6, C_TPID_STORE );
      }

      c_sname_set( &ast->sname, &sname );
      ast->as.func.param_ast_list = $3;

      DUMP_AST( "knr_func_or_constructor_decl_c", ast );
      DUMP_END();

      C_AST_CHECK( ast );
      c_ast_explain_declaration( ast, cdecl_fout );
    }
  ;

rparen_func_qualifier_list_c_stid_opt
  : ')'
    {
      if ( OPT_LANG_IS(CPP_ANY) ) {
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
      C_TID_ADD( &$$, $2, @2 );

      DUMP_TID( "func_qualifier_list_c_stid_opt", $$ );
      DUMP_END();
    }
  ;

func_qualifier_c_stid
  : cv_qualifier_stid
  | Y_FINAL
  | Y_OVERRIDE
    /*
     * GNU C++ allows restricted-this-pointer member functions:
     *
     *      void S::f() __restrict;
     *
     * <https://gcc.gnu.org/onlinedocs/gcc/Restricted-Pointers.html>
     */
  | Y_GNU___RESTRICT                    // GNU C++ extension
  ;

func_ref_qualifier_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_AMPER                       { $$ = TS_REFERENCE; }
  | Y_AMPER2                      { $$ = TS_RVALUE_REFERENCE; }
  ;

noexcept_c_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_NOEXCEPT
  | Y_NOEXCEPT '(' no_except_bool_stid_exp rparen_exp
    {
      $$ = $3;
    }
  | Y_THROW lparen_exp rparen_exp
  ;

no_except_bool_stid_exp
  : Y_FALSE
  | Y_TRUE
  | error
    {
      elaborate_error( "\"%s\" or \"%s\" expected", L_TRUE, L_FALSE );
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
      DUMP_AST( "(type_c_ast)", ret_ast );
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
      if ( unsupported( LANG_CPP_MIN(11) ) ) {
        print_error( &@1,
          "trailing return type not supported%s\n",
          c_lang_which( LANG_CPP_MIN(11) )
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
          "function with trailing return type must only specify \"%s\"\n",
          L_AUTO
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
  | '=' Y_DEFAULT                 { $$ = $2; }
  | '=' Y_DELETE                  { $$ = $2; }
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
      if ( opt_lang < LANG_CPP_11 )
        elaborate_error( "'0' expected" );
      else
        elaborate_error(
          "'0', \"%s\", or \"%s\" expected", L_DEFAULT, L_DELETE
        );
    }
  ;

/// Gibberish C/C++ function(like) parameter declaration //////////////////////

param_list_c_ast_exp
  : param_list_c_ast
  | error
    {
      slist_init( &$$ );
      elaborate_error( "parameter list expected" );
    }
  ;

param_list_c_ast_opt
  : /* empty */                   { slist_init( &$$ ); }
  | param_list_c_ast
  ;

param_list_c_ast
  : param_list_c_ast comma_exp param_c_ast
    {
      DUMP_START( "param_list_c_ast", "param_list_c_ast ',' param_c_ast" );
      DUMP_AST_LIST( "param_list_c_ast", $1 );
      DUMP_AST( "param_c_ast", $3 );

      $$ = $1;
      slist_push_tail( &$$, $3 );

      DUMP_AST_LIST( "param_list_c_ast", $$ );
      DUMP_END();
    }

  | param_c_ast
    {
      DUMP_START( "param_list_c_ast", "param_c_ast" );
      DUMP_AST( "param_c_ast", $1 );

      slist_init( &$$ );
      slist_push_tail( &$$, $1 );

      DUMP_AST_LIST( "param_list_c_ast", $$ );
      DUMP_END();
    }
  ;

param_c_ast
    /*
     * Ordinary function parameter declaration.
     */
  : type_c_ast { ia_type_ast_push( $1 ); } cast_c_astp_opt
    {
      ia_type_ast_pop();

      DUMP_START( "param_c_ast", "type_c_ast cast_c_astp_opt" );
      DUMP_AST( "type_c_ast", $1 );
      DUMP_AST( "cast_c_astp_opt", $3.ast );

      $$ = c_ast_patch_placeholder( $1, $3.ast );

      if ( $$->kind == K_FUNCTION )     // see the comment in decl_english_ast
        $$ = c_ast_pointer( $$, &gc_ast_list );

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


/// Gibberish C/C++ nested declaration ////////////////////////////////////////

nested_decl_c_astp
  : '('
    {
      ia_type_ast_push( c_ast_new_gc( K_PLACEHOLDER, &@$ ) );
      ++ast_depth;
    }
    decl_c_astp rparen_exp
    {
      ia_type_ast_pop();
      --ast_depth;

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
    oper_c_ast lparen_exp param_list_c_ast_opt
    rparen_func_qualifier_list_c_stid_opt func_ref_qualifier_c_stid_opt
    noexcept_c_stid_opt trailing_return_type_c_ast_opt func_equals_c_stid_opt
    {
      c_ast_t *const oper_c_ast = $1;
      c_tid_t  const func_qualifier_stid = $4;
      c_tid_t  const func_ref_qualifier_stid = $5;
      c_tid_t  const func_equals_stid = $8;
      c_tid_t  const noexcept_stid = $6;
      c_ast_t *const trailing_ret_ast = $7;
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "oper_decl_c_astp",
                  "oper_c_ast '(' param_list_c_ast_opt ')' "
                  "func_qualifier_list_c_stid_opt "
                  "func_ref_qualifier_c_stid_opt noexcept_c_stid_opt "
                  "trailing_return_type_c_ast_opt "
                  "func_equals_c_stid_opt" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "oper_c_ast", oper_c_ast );
      DUMP_AST_LIST( "param_list_c_ast_opt", $3 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", func_qualifier_stid );
      DUMP_TID( "func_ref_qualifier_c_stid_opt", func_ref_qualifier_stid );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );
      DUMP_TID( "func_equals_c_stid_opt", func_equals_stid );

      c_tid_t const oper_stid =
        func_qualifier_stid | func_ref_qualifier_stid | noexcept_stid |
        func_equals_stid;

      c_ast_t *const oper_ast = c_ast_new_gc( K_OPERATOR, &@$ );
      oper_ast->type.stids = c_tid_check( oper_stid, C_TPID_STORE );
      oper_ast->as.oper.param_ast_list = $3;
      oper_ast->as.oper.oper_id = oper_c_ast->as.oper.oper_id;

      $$.ast = c_ast_add_func(
        oper_c_ast,
        oper_ast,
        trailing_ret_ast != NULL ? trailing_ret_ast : type_ast
      );

      $$.target_ast = oper_ast->as.oper.ret_ast;

      DUMP_AST( "oper_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

oper_c_ast
  : // in_attr: type_c_ast
    scope_sname_c_opt Y_OPERATOR c_operator
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "oper_c_ast", "OPERATOR c_operator" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_SNAME( "scope_sname_c_opt", $1 );
      DUMP_STR( "c_operator", c_oper_get( $3 )->name );

      $$ = type_ast;
      c_sname_set( &$$->sname, &$1 );
      $$->as.oper.oper_id = $3;

      DUMP_AST( "oper_c_ast", $$ );
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
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $2 );

      $$ = c_ast_new_gc( K_POINTER, &@$ );
      $$->type.stids = c_tid_check( $2, C_TPID_STORE );
      c_ast_set_parent( type_ast, $$ );

      DUMP_AST( "pointer_type_c_ast", $$ );
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
      DUMP_AST( "(type_c_ast)", type_ast );
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

      $$->as.ptr_mbr.class_sname = $1;
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
      DUMP_AST( "(type_c_ast)", type_ast );
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
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $2 );

      $$ = c_ast_new_gc( K_RVALUE_REFERENCE, &@$ );
      $$->type.stids = c_tid_check( $2, C_TPID_STORE );
      c_ast_set_parent( type_ast, $$ );

      DUMP_AST( "reference_type_c_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish typedef type declaration ////////////////////////////////////////

typedef_type_decl_c_ast
  : // in_attr: type_c_ast
    typedef_type_c_ast
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "typedef_type_decl_c_ast", "typedef_type_c_ast" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "typedef_type_c_ast", $1 );

      if ( c_type_is_tid_any( &type_ast->type, TS_TYPEDEF ) ) {
        //
        // If we're defining a type, return the type as-is.
        //
        $$ = $1;
      }
      else {
        //
        // Otherwise, return the type that it's typedef'd as.
        //
        $$ = CONST_CAST( c_ast_t*, c_ast_untypedef( $1 ) );
      }

      DUMP_AST( "typedef_type_c_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C++ user-defined conversion operator declaration ////////////////

user_defined_conversion_decl_c_astp
  : // in_attr: type_c_ast
    scope_sname_c_opt Y_OPERATOR type_c_ast
    {
      ia_type_ast_push( $3 );
    }
    udc_decl_c_ast_opt lparen_exp rparen_func_qualifier_list_c_stid_opt
    noexcept_c_stid_opt func_equals_c_stid_opt
    {
      ia_type_ast_pop();
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "user_defined_conversion_decl_c_astp",
                  "scope_sname_c_opt OPERATOR "
                  "type_c_ast udc_decl_c_ast_opt '(' ')' "
                  "func_qualifier_list_c_stid_opt noexcept_c_stid_opt "
                  "func_equals_c_stid_opt" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_SNAME( "scope_sname_c_opt", $1 );
      DUMP_AST( "type_c_ast", $3 );
      DUMP_AST( "udc_decl_c_ast_opt", $5 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", $7 );
      DUMP_TID( "noexcept_c_stid_opt", $8 );
      DUMP_TID( "func_equals_c_stid_opt", $9 );

      $$.ast = c_ast_new_gc( K_USER_DEF_CONVERSION, &@$ );
      $$.ast->sname = $1;
      $$.ast->type.stids = c_tid_check( $7 | $8 | $9, C_TPID_STORE );
      if ( type_ast != NULL )
        c_type_or_eq( &$$.ast->type, &type_ast->type );
      $$.ast->as.udef_conv.conv_ast = $5 != NULL ? $5 : $3;
      $$.target_ast = $$.ast->as.udef_conv.conv_ast;

      DUMP_AST( "user_defined_conversion_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// C++ user-definer literal declaration //////////////////////////////////////

user_defined_literal_decl_c_astp
  : // in_attr: type_c_ast
    user_defined_literal_c_ast lparen_exp param_list_c_ast_exp ')'
    noexcept_c_stid_opt trailing_return_type_c_ast_opt
    {
      c_ast_t *const udl_c_ast = $1;
      c_tid_t  const noexcept_stid = $5;
      c_ast_t *const trailing_ret_ast = $6;
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "user_defined_literal_decl_c_astp",
                  "user_defined_literal_c_ast '(' param_list_c_ast_exp ')' "
                  "noexcept_c_stid_opt trailing_return_type_c_ast_opt" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "user_defined_literal_c_ast", udl_c_ast );
      DUMP_AST_LIST( "param_list_c_ast_exp", $3 );
      DUMP_TID( "noexcept_c_stid_opt", noexcept_stid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );

      c_ast_t *const udl_ast = c_ast_new_gc( K_USER_DEF_LITERAL, &@$ );
      udl_ast->type.stids = c_tid_check( noexcept_stid, C_TPID_STORE );
      udl_ast->as.udef_lit.param_ast_list = $3;

      $$.ast = c_ast_add_func(
        udl_c_ast,
        udl_ast,
        trailing_ret_ast != NULL ? trailing_ret_ast : type_ast
      );

      $$.target_ast = udl_ast->as.udef_lit.ret_ast;

      DUMP_AST( "user_defined_literal_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

user_defined_literal_c_ast
  : // in_attr: type_c_ast
    scope_sname_c_opt Y_OPERATOR quote2_exp name_exp
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "user_defined_literal_c_ast", "OPERATOR \"\" NAME" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_SNAME( "scope_sname_c_opt", $1 );
      DUMP_STR( "name", $4 );

      $$ = type_ast;
      c_sname_set( &$$->sname, &$1 );
      c_sname_append_name( &$$->sname, $4 );

      DUMP_AST( "user_defined_literal_c_ast", $$ );
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
      DUMP_AST( "(type_c_ast)", type_ast );
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

array_size_c_ast
  : array_size_c_int
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = $1;
    }
  | '[' type_qualifier_list_c_stid rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = C_ARRAY_SIZE_NONE;
      $$->as.array.stids = c_tid_check( $2, C_TPID_STORE );
    }
  | '[' type_qualifier_list_c_stid static_stid_opt Y_INT_LIT rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = $4;
      $$->as.array.stids = c_tid_check( $2 | $3, C_TPID_STORE );
    }
  | '[' type_qualifier_list_c_stid_opt '*' rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = C_ARRAY_SIZE_VARIABLE;
      $$->as.array.stids = c_tid_check( $2, C_TPID_STORE );
    }
  | '[' Y_STATIC type_qualifier_list_c_stid_opt Y_INT_LIT rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = $4;
      $$->as.array.stids = c_tid_check( $2 | $3, C_TPID_STORE );
    }
  ;

/// Gibberish block cast (Apple extension) ////////////////////////////////////

block_cast_c_astp                       // Apple extension
  : // in_attr: type_c_ast
    '(' Y_CIRC
    { //
      // A block AST has to be the type inherited attribute for cast_c_astp_opt
      // so we have to create it here.
      //
      ia_type_ast_push( c_ast_new_gc( K_APPLE_BLOCK, &@$ ) );
    }
    type_qualifier_list_c_stid_opt cast_c_astp_opt rparen_exp
    lparen_exp param_list_c_ast_opt ')'
    {
      c_ast_t *const block_ast = ia_type_ast_pop();
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "block_cast_c_astp",
                  "'(' '^' type_qualifier_list_c_stid_opt cast_c_astp_opt ')' "
                  "'(' param_list_c_ast_opt ')'" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_TID( "type_qualifier_list_c_stid_opt", $4 );
      DUMP_AST( "cast_c_astp_opt", $5.ast );
      DUMP_AST_LIST( "param_list_c_ast_opt", $8 );

      C_TYPE_ADD_TID( &block_ast->type, $4, @4 );
      block_ast->as.block.param_ast_list = $8;
      $$.ast = c_ast_add_func( $5.ast, block_ast, type_ast );
      $$.target_ast = block_ast->as.block.ret_ast;

      DUMP_AST( "block_cast_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ function cast /////////////////////////////////////////////

func_cast_c_astp
  : // in_attr: type_c_ast
    cast2_c_astp '(' param_list_c_ast_opt rparen_func_qualifier_list_c_stid_opt
    trailing_return_type_c_ast_opt
    {
      c_ast_t *const cast2_c_ast = $1.ast;
      c_tid_t  const func_ref_qualifier_stid = $4;
      c_ast_t *const trailing_ret_ast = $5;
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "func_cast_c_astp",
                  "cast2_c_astp '(' param_list_c_ast_opt ')' "
                  "func_qualifier_list_c_stid_opt "
                  "trailing_return_type_c_ast_opt" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "cast2_c_ast", cast2_c_ast );
      DUMP_AST_LIST( "param_list_c_ast_opt", $3 );
      DUMP_TID( "func_qualifier_list_c_stid_opt", func_ref_qualifier_stid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );
      DUMP_AST( "target_ast", $1.target_ast );

      c_tid_t const func_stid = func_ref_qualifier_stid;

      c_ast_t *const func_ast = c_ast_new_gc( K_FUNCTION, &@$ );
      func_ast->type.stids = c_tid_check( func_stid, C_TPID_STORE );
      func_ast->as.func.param_ast_list = $3;

      if ( $1.target_ast != NULL ) {
        $$.ast = cast2_c_ast;
        PJL_IGNORE_RV( c_ast_add_func( $1.target_ast, func_ast, type_ast ) );
      }
      else {
        $$.ast = c_ast_add_func(
          cast2_c_ast,
          func_ast,
          trailing_ret_ast != NULL ? trailing_ret_ast : type_ast
        );
      }

      $$.target_ast = func_ast->as.func.ret_ast;

      DUMP_AST( "func_cast_c_astp", $$.ast );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ nested cast ///////////////////////////////////////////////

nested_cast_c_astp
  : '('
    {
      ia_type_ast_push( c_ast_new_gc( K_PLACEHOLDER, &@$ ) );
      ++ast_depth;
    }
    cast_c_astp_opt rparen_exp
    {
      ia_type_ast_pop();
      --ast_depth;

      DUMP_START( "nested_cast_c_astp", "'(' cast_c_astp_opt ')'" );
      DUMP_AST( "cast_c_astp_opt", $3.ast );

      $$ = $3;
      $$.ast->loc = @$;

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
  : type_modifier_list_c_type           // allows implicit int in K&R C
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
      c_type_t type = opt_lang < LANG_C_99 ? C_TYPE_LIT_B( TB_INT ) : T_NONE;

      C_TYPE_ADD( &type, &$1, @1 );

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
      C_TYPE_ADD( &$$->type, &$1, @1 );

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
      C_TYPE_ADD( &$$, &$2, @2 );

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
      if ( $$.stids == TS_REGISTER )
        lexer_find |= LEXER_FIND_TYPEDEFS;
      else
        lexer_find &= ~LEXER_FIND_TYPEDEFS;
    }
  | type_qualifier_c_stid         { $$ = C_TYPE_LIT_S( $1 ); }
  | storage_class_c_type
  | attribute_specifier_list_c_atid
    {
      $$ = C_TYPE_LIT_A( $1 );
    }
  ;

type_modifier_base_type
  : Y__COMPLEX                    { $$ = C_TYPE_LIT_B( $1 ); }
  | Y__IMAGINARY                  { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_LONG                        { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_SHORT                       { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_SIGNED                      { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_UNSIGNED                    { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_EMC__SAT                    { $$ = C_TYPE_LIT_B( $1 ); }
    /*
     * Register is here (rather than in storage_class_c_type) because it's the
     * only storage class that can be specified for function parameters.
     * Therefore, it's simpler to treat it as any other type modifier.
     */
  | Y_REGISTER                    { $$ = C_TYPE_LIT_S( $1 ); }
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
      C_TYPE_ADD( &$$->type, &$2, @2 );

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
      C_TYPE_ADD_TID( &$$->type, $2, @2 );

      DUMP_AST( "east_modified_type_c_ast", $$ );
      DUMP_END();
    }
  ;

atomic_builtin_typedef_type_c_ast
  : atomic_specifier_type_c_ast
  | builtin_type_ast
  | typedef_type_c_ast
  ;

/// C Gibberish _Atomic types /////////////////////////////////////////////////

atomic_specifier_type_c_ast
  : Y__ATOMIC_SPEC lparen_exp type_c_ast
    {
      ia_type_ast_push( $3 );
    }
    cast_c_astp_opt rparen_exp
    {
      ia_type_ast_pop();

      DUMP_START( "atomic_specifier_type_c_ast",
                  "ATOMIC '(' type_c_ast cast_c_astp_opt ')'" );
      DUMP_AST( "type_c_ast", $3 );
      DUMP_AST( "cast_c_astp_opt", $5.ast );

      $$ = $5.ast != NULL ? $5.ast : $3;
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
          "%s can not be of \"%s\"\n",
          L__ATOMIC, c_type_name_c( &error_type )
        );
        PARSE_ABORT();
      }

      C_TYPE_ADD_TID( &$$->type, TS_ATOMIC, @1 );

      DUMP_AST( "atomic_specifier_type_c_ast", $$ );
      DUMP_END();
    }
  ;

/// Gibberish C/C++ built-in types ////////////////////////////////////////////

builtin_type_ast
  : builtin_btid
    {
      DUMP_START( "builtin_type_ast", "builtin_btid" );
      DUMP_TID( "builtin_btid", $1 );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.btids = c_tid_check( $1, C_TPID_BASE );

      DUMP_AST( "builtin_type_ast", $$ );
      DUMP_END();
    }
  ;

builtin_btid
  : Y_VOID
  | Y_AUTO_TYPE
  | Y__BOOL
  | Y_BOOL
  | Y_CHAR
  | Y_CHAR8_T
  | Y_CHAR16_T
  | Y_CHAR32_T
  | Y_WCHAR_T
  | Y_INT
  | Y_FLOAT
  | Y_DOUBLE
  | Y_EMC__ACCUM
  | Y_EMC__FRACT
  ;

/// Gibberish C/C++ enum, class, struct, & union types ////////////////////////

enum_class_struct_union_c_ast
  : enum_class_struct_union_c_btid attribute_specifier_list_c_atid_opt
    any_sname_c_exp enum_fixed_type_c_ast_opt
    {
      DUMP_START( "enum_class_struct_union_c_ast",
                  "enum_class_struct_union_c_btid "
                  "attribute_specifier_list_c_atid_opt sname" );
      DUMP_TID( "enum_class_struct_union_c_btid", $1 );
      DUMP_TID( "attribute_specifier_list_c_atid_opt", $2 );
      DUMP_SNAME( "any_sname_c", $3 );
      DUMP_AST( "enum_fixed_type_c_ast_opt", $4 );

      $$ = c_ast_new_gc( K_ENUM_CLASS_STRUCT_UNION, &@$ );
      $$->type.btids = c_tid_check( $1, C_TPID_BASE );
      $$->type.atids = c_tid_check( $2, C_TPID_ATTR );
      $$->as.ecsu.of_ast = $4;
      $$->as.ecsu.ecsu_sname = $3;

      DUMP_AST( "enum_class_struct_union_c_ast", $$ );
      DUMP_END();
    }

  | enum_class_struct_union_c_btid attribute_specifier_list_c_atid_opt
    any_sname_c_opt '{'
    {
      print_error( &@4,
        "explaining %s declarations not supported by %s\n",
        c_tid_name_c( $1 ), CDECL
      );
      c_sname_cleanup( &$3 );
      PARSE_ABORT();
    }
  ;

enum_class_struct_union_c_btid
  : enum_btid
  | class_struct_union_btid
  ;

/// Gibberish C/C++ enum type /////////////////////////////////////////////////

enum_btid
  : Y_ENUM class_struct_btid_opt  { $$ = $1 | $2; }
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
      C_TYPE_ADD_TID( &$$->type, $1, @1 );
      C_TYPE_ADD_TID( &$$->type, $3, @3 );

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
      C_TYPE_ADD_TID( &$$->type, $2, @2 );

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
      C_TID_ADD( &$$, $2, @2 );

      DUMP_TID( "enum_fixed_type_modifier_list_btid", $$ );
      DUMP_END();
    }

  | enum_fixed_type_modifier_btid
  ;

enum_fixed_type_modifier_btid
  : Y_LONG
  | Y_SHORT
  | Y_SIGNED
  | Y_UNSIGNED
  ;

enum_unmodified_fixed_type_c_ast
  : builtin_type_ast
  | typedef_type_c_ast
  ;

/// Gibberish C/C++ class, struct, & union types //////////////////////////////

class_struct_btid_opt
  : /* empty */                   { $$ = TB_NONE; }
  | class_struct_btid
  ;

class_struct_btid
  : Y_CLASS
  | Y_STRUCT
  ;

class_struct_union_btid
  : class_struct_btid
  | Y_UNION
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
      C_TID_ADD( &$$, $2, @2 );

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
  : Y__ATOMIC_QUAL
  | cv_qualifier_stid
  | restrict_qualifier_c_stid
  | Y_UPC_RELAXED
  | Y_UPC_SHARED                  %prec Y_PREC_LESS_THAN_upc_layout_qualifier
  | Y_UPC_SHARED upc_layout_qualifier_c
  | Y_UPC_STRICT
  ;

cv_qualifier_stid
  : Y_CONST
  | Y_VOLATILE
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
      C_TID_ADD( &$$, $2, @2 );

      DUMP_TID( "cv_qualifier_list_stid_opt", $$ );
      DUMP_END();
    }
  ;

restrict_qualifier_c_stid
  : Y_RESTRICT                          // C only
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since both "restrict" and "__restrict" map to TS_RESTRICT and the
      // AST has no "memory" of which it was.
      //
      if ( OPT_LANG_IS(CPP_ANY) ) {
        print_error( &@1,
          "\"%s\" not supported in C++; use \"%s\" instead\n",
          L_RESTRICT, L_GNU___RESTRICT
        );
        PARSE_ABORT();
      }
    }
  | Y_GNU___RESTRICT                    // GNU C/C++ extension
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
  : Y_AUTO_STORAGE                { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_APPLE___BLOCK               { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_CONSTEVAL                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_CONSTEXPR                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_CONSTINIT                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXPLICIT                    { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXPORT                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXTERN                      { $$ = C_TYPE_LIT_S( $1 ); }
  | extern_linkage_c_stid         { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_FINAL                       { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_FRIEND                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_INLINE                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_MUTABLE                     { $$ = C_TYPE_LIT_S( $1 ); }
  | Y__NORETURN                   { $$ = C_TYPE_LIT_A( $1 ); }
  | Y_NORETURN                    { $$ = C_TYPE_LIT_A( $1 ); }
  | Y_OVERRIDE                    { $$ = C_TYPE_LIT_S( $1 ); }
//| Y_REGISTER                          // in type_modifier_base_type
  | Y_STATIC                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_TYPEDEF                     { $$ = C_TYPE_LIT_S( $1 ); }
  | Y__THREAD_LOCAL               { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_THREAD_LOCAL                { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_VIRTUAL                     { $$ = C_TYPE_LIT_S( $1 ); }
  ;

/// Gibberish C/C++ attributes ////////////////////////////////////////////////

attribute_specifier_list_c_atid_opt
  : /* empty */                   { $$ = TA_NONE; }
  | attribute_specifier_list_c_atid
  ;

attribute_specifier_list_c_atid
  : Y_ATTR_BEGIN '['
    {
      if ( unsupported( LANG_C_CPP_MIN(2X,11) ) ) {
        print_error( &@1,
          "\"[[\" attribute syntax not supported%s\n",
          c_lang_which( LANG_C_CPP_MIN(2X,11) )
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
  | Y_USING name_exp colon_exp
    {
      print_warning( &@1,
        "\"%s\" in attributes not supported by %s (ignoring)\n",
        L_USING, CDECL
      );
      free( $2 );
    }
  ;

attribute_list_c_atid_opt
  : /* empty */                   { $$ = TA_NONE; }
  | attribute_list_c_atid
  ;

attribute_list_c_atid
  : attribute_list_c_atid comma_exp attribute_c_atid
    {
      DUMP_START( "attribute_list_c_atid",
                  "attribute_list_c_atid , attribute_c_atid" );
      DUMP_TID( "attribute_list_c_atid", $1 );
      DUMP_TID( "attribute_c_atid", $3 );

      $$ = $1;
      C_TID_ADD( &$$, $3, @3 );

      DUMP_TID( "attribute_list_c_atid", $$ );
      DUMP_END();
    }

  | attribute_c_atid
  ;

attribute_c_atid
  : Y_CARRIES_DEPENDENCY
  | Y_DEPRECATED attribute_str_arg_c_opt
  | Y_MAYBE_UNUSED
  | Y_NODISCARD attribute_str_arg_c_opt
  | Y_NORETURN
  | Y_NO_UNIQUE_ADDRESS
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
        c_keyword_t const *const k =
          c_keyword_find( name, opt_lang_newer(), C_KW_CTX_ATTRIBUTE );
        if ( k != NULL && c_tid_tpid( k->tid ) == C_TPID_ATTR ) {
          adj = "unsupported";
          lang_ids = k->lang_ids;
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
  : Y_GNU___ATTRIBUTE__
    {
      attr_syntax_not_supported( &@1, L_GNU___ATTRIBUTE__ );
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
  : gnu_attribuet_list_c comma_exp gnu_attribute_c
  | gnu_attribute_c
  ;

gnu_attribute_c
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
  : Y_MSC___DECLSPEC
    {
      attr_syntax_not_supported( &@1, L_MSC___DECLSPEC );
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
     * can just use gnu_attribute_c here.
     *
     * Note that Microsoft attributes are separated by whitespace, not commas
     * as in GNU's, so that's why msc_attribuet_list_c is needed.
     */
  : msc_attribuet_list_c gnu_attribute_c
  | gnu_attribute_c
  ;

///////////////////////////////////////////////////////////////////////////////
//  ENGLISH DECLARATIONS                                                     //
///////////////////////////////////////////////////////////////////////////////

decl_english_ast
  : array_decl_english_ast
  | constructor_decl_english_ast
  | destructor_decl_english_ast
  | qualified_decl_english_ast
  | user_defined_literal_decl_english_ast
  | var_decl_english_ast
  ;

/// English C/C++ array declaration ///////////////////////////////////////////

array_decl_english_ast
  : Y_ARRAY static_stid_opt array_qualifier_list_english_stid_opt
    array_size_int_opt of_exp decl_english_ast
    {
      DUMP_START( "array_decl_english_ast",
                  "ARRAY static_stid_opt array_qualifier_list_english_stid_opt "
                  "array_size_num_opt OF decl_english_ast" );
      DUMP_TID( "static_stid_opt", $2 );
      DUMP_TID( "array_qualifier_list_english_stid_opt", $3 );
      DUMP_INT( "array_size_int_opt", $4 );
      DUMP_AST( "decl_english_ast", $6 );

      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = $4;
      $$->as.array.stids = c_tid_check( $2 | $3, C_TPID_STORE );
      c_ast_set_parent( $6, $$ );

      DUMP_AST( "array_decl_english_ast", $$ );
      DUMP_END();
    }

  | Y_VARIABLE length_opt array_exp array_qualifier_list_english_stid_opt
    of_exp decl_english_ast
    {
      DUMP_START( "array_decl_english_ast",
                  "VARIABLE LENGTH ARRAY array_qualifier_list_english_stid_opt "
                  "OF decl_english_ast" );
      DUMP_TID( "array_qualifier_list_english_stid_opt", $4 );
      DUMP_AST( "decl_english_ast", $6 );

      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = C_ARRAY_SIZE_VARIABLE;
      $$->as.array.stids = c_tid_check( $4, C_TPID_STORE );
      c_ast_set_parent( $6, $$ );

      DUMP_AST( "array_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

array_qualifier_list_english_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | array_qualifier_list_english_stid
  ;

array_qualifier_list_english_stid
  : cv_qualifier_stid
  ;

array_size_int_opt
  : /* empty */                   { $$ = C_ARRAY_SIZE_NONE; }
  | Y_INT_LIT
  | '*'                           { $$ = C_ARRAY_SIZE_VARIABLE; }
  ;

length_opt
  : /* empty */
  | Y_LENGTH
  ;

/// English block declaration (Apple extension) ///////////////////////////////

block_decl_english_ast                  // Apple extension
  : // in_attr: qualifier
    Y_APPLE_BLOCK paren_decl_list_english_opt returning_english_ast_opt
    {
      DUMP_START( "block_decl_english_ast",
                  "BLOCK paren_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $2 );
      DUMP_AST( "returning_english_ast_opt", $3 );

      $$ = c_ast_new_gc( K_APPLE_BLOCK, &@$ );
      $$->as.block.param_ast_list = $2;
      c_ast_set_parent( $3, $$ );

      DUMP_AST( "block_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

/// English C++ constructor declaration ///////////////////////////////////////

constructor_decl_english_ast
  : Y_CONSTRUCTOR paren_decl_list_english_opt
    {
      DUMP_START( "constructor_decl_english_ast",
                  "CONSTRUCTOR paren_decl_list_english_opt" );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $2 );

      $$ = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      $$->as.constructor.param_ast_list = $2;

      DUMP_AST( "constructor_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

/// English C++ destructor declaration ////////////////////////////////////////

destructor_decl_english_ast
  : Y_DESTRUCTOR parens_opt
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
    func_qualifier_english_type_opt member_or_non_member_mask_opt
    Y_FUNCTION paren_decl_list_english_opt returning_english_ast_opt
    {
      DUMP_START( "func_decl_english_ast",
                  "ref_qualifier_english_stid_opt "
                  "member_or_non_member_mask_opt "
                  "FUNCTION paren_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_TYPE( "func_qualifier_english_type_opt", $1 );
      DUMP_INT( "member_or_non_member_mask_opt", $2 );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $4 );
      DUMP_AST( "returning_english_ast_opt", $5 );

      $$ = c_ast_new_gc( K_FUNCTION, &@$ );
      $$->type = $1;
      $$->as.func.param_ast_list = $4;
      $$->as.func.flags = $2;
      c_ast_set_parent( $5, $$ );

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
  : Y_MSC___CDECL
  | Y_MSC___CLRCALL
  | Y_MSC___FASTCALL
  | Y_MSC___STDCALL
  | Y_MSC___THISCALL
  | Y_MSC___VECTORCALL
  ;

/// English C++ operator declaration //////////////////////////////////////////

oper_decl_english_ast
  : type_qualifier_list_english_type_opt ref_qualifier_english_stid_opt
    member_or_non_member_mask_opt operator_exp paren_decl_list_english_opt
    returning_english_ast_opt
    {
      DUMP_START( "oper_decl_english_ast",
                  "type_qualifier_list_english_type_opt "
                  "ref_qualifier_english_stid_opt "
                  "member_or_non_member_mask_opt "
                  "OPERATOR paren_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_TYPE( "type_qualifier_list_english_type_opt", $1 );
      DUMP_TID( "ref_qualifier_english_stid_opt", $2 );
      DUMP_INT( "member_or_non_member_mask_opt", $3 );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $5 );
      DUMP_AST( "returning_english_ast_opt", $6 );

      $$ = c_ast_new_gc( K_OPERATOR, &@$ );
      C_TYPE_ADD( &$$->type, &$1, @1 );
      C_TYPE_ADD_TID( &$$->type, $2, @2 );
      $$->as.oper.param_ast_list = $5;
      $$->as.oper.flags = $3;
      c_ast_set_parent( $6, $$ );

      DUMP_AST( "oper_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

/// English C/C++ parenthesized declaration ///////////////////////////////////

paren_decl_list_english_opt
  : /* empty */                   { slist_init( &$$ ); }
  | paren_decl_list_english
  ;

paren_decl_list_english
  : '(' decl_list_english_opt ')'
    {
      DUMP_START( "paren_decl_list_english",
                  "'(' decl_list_english_opt ')'" );
      DUMP_AST_LIST( "decl_list_english_opt", $2 );

      $$ = $2;

      DUMP_AST_LIST( "paren_decl_list_english", $$ );
      DUMP_END();
    }
  ;

decl_list_english_opt
  : /* empty */                   { slist_init( &$$ ); }
  | decl_list_english
  ;

decl_list_english
  : decl_list_english comma_exp decl_english_ast
    {
      DUMP_START( "decl_list_english",
                  "decl_list_english ',' decl_english_ast" );
      DUMP_AST_LIST( "decl_list_english", $1 );
      DUMP_AST( "decl_english_ast", $3 );

      $$ = $1;
      slist_push_tail( &$$, $3 );

      DUMP_AST_LIST( "decl_list_english", $$ );
      DUMP_END();
    }

  | decl_english_ast
    {
      DUMP_START( "decl_list_english", "decl_english_ast" );
      DUMP_AST( "decl_english_ast", $1 );

      if ( $1->kind == K_FUNCTION ) {
        //
        // From the C11 standard, section 6.3.2.1(4):
        //
        //    [A] function designator with type "function returning type" is
        //    converted to an expression that has type "pointer to function
        //    returning type."
        //
        $1 = c_ast_pointer( $1, &gc_ast_list );
      }

      slist_init( &$$ );
      slist_push_tail( &$$, $1 );

      DUMP_AST_LIST( "decl_list_english", $$ );
      DUMP_END();
    }
  ;

/// English C++ reference qualifier declaration ///////////////////////////////

ref_qualifier_english_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_REFERENCE                   { $$ = TS_REFERENCE; }
  | Y_RVALUE reference_exp        { $$ = TS_RVALUE_REFERENCE; }
  ;

/// English C/C++ function(like) returning declaration ////////////////////////

returning_english_ast_opt
  : /* empty */
    {
      DUMP_START( "returning_english_ast_opt", "<empty>" );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      // see the comment in "type_c_ast"
      $$->type.btids = opt_lang < LANG_C_99 ? TB_INT : TB_VOID;

      DUMP_AST( "returning_english_ast_opt", $$ );
      DUMP_END();
    }

  | returning_english_ast
  ;

returning_english_ast
  : Y_RETURNING decl_english_ast
    {
      DUMP_START( "returning_english_ast", "RETURNING decl_english_ast" );
      DUMP_AST( "decl_english_ast", $2 );

      $$ = $2;

      DUMP_AST( "returning_english_ast", $$ );
      DUMP_END();
    }

  | Y_RETURNING error
    {
      elaborate_error( "English expected after \"%s\"", L_RETURNING );
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
      C_TYPE_ADD( &$$->type, &$1, @1 );

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
      C_TYPE_ADD( &$$, &$2, @2 );

      DUMP_TYPE( "type_qualifier_list_english_type", $$ );
      DUMP_END();
    }

  | type_qualifier_english_type
  ;

type_qualifier_english_type
  : attribute_english_atid        { $$ = C_TYPE_LIT_A( $1 ); }
  | type_qualifier_english_stid   { $$ = C_TYPE_LIT_S( $1 ); }
  ;

type_qualifier_english_stid
  : Y__ATOMIC_QUAL
  | cv_qualifier_stid
  | restrict_qualifier_c_stid
  | Y_UPC_RELAXED
  | Y_UPC_SHARED                  %prec Y_PREC_LESS_THAN_upc_layout_qualifier
  | Y_UPC_SHARED upc_layout_qualifier_english
  | Y_UPC_STRICT
  ;

upc_layout_qualifier_english
  : Y_INT_LIT
  | '*'
  ;

qualifiable_decl_english_ast
  : block_decl_english_ast
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
    Y_POINTER to_exp decl_english_ast
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
    Y_POINTER to_exp Y_MEMBER of_exp class_struct_union_btid_exp
    sname_english_exp decl_english_ast
    {
      DUMP_START( "pointer_to_member_decl_english",
                  "POINTER TO MEMBER OF "
                  "class_struct_union_btid_exp "
                  "sname_english decl_english_ast" );
      DUMP_TID( "class_struct_union_btid_exp", $5 );
      DUMP_SNAME( "sname_english_exp", $6 );
      DUMP_AST( "decl_english_ast", $7 );

      $$ = c_ast_new_gc( K_POINTER_TO_MEMBER, &@$ );
      $$->as.ptr_mbr.class_sname = $6;
      c_ast_set_parent( $7, $$ );
      C_TYPE_ADD_TID( &$$->type, $5, @5 );

      DUMP_AST( "pointer_to_member_decl_english", $$ );
      DUMP_END();
    }

  | Y_POINTER to_exp error
    {
      if ( OPT_LANG_IS(CPP_ANY) )
        elaborate_error( "type name or \"%s\" expected", L_MEMBER );
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
  : Y_REFERENCE
    {
      $$ = c_ast_new_gc( K_REFERENCE, &@$ );
    }

  | Y_RVALUE reference_exp
    {
      $$ = c_ast_new_gc( K_RVALUE_REFERENCE, &@$ );
    }
  ;

/// English C++ user-defined literal declaration //////////////////////////////

user_defined_literal_decl_english_ast
  : Y_USER_DEFINED literal_exp paren_decl_list_english_opt
    returning_english_ast_opt
    { //
      // User-defined literals are supported only in C++11 and later.
      // (However, we always allow them in configuration files.)
      //
      // This check is better to do now in the parser rather than later in the
      // AST because it has to be done in fewer places in the code plus gives a
      // better error location.
      //
      if ( unsupported( LANG_CPP_MIN(11) ) ) {
        print_error( &@1,
          "%s %s not supported%s\n",
          H_USER_DEFINED, L_LITERAL, c_lang_which( LANG_CPP_MIN(11) )
        );
        PARSE_ABORT();
      }

      DUMP_START( "user_defined_literal_decl_english_ast",
                  "USER-DEFINED LITERAL paren_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $3 );
      DUMP_AST( "returning_english_ast_opt", $4 );

      $$ = c_ast_new_gc( K_USER_DEF_LITERAL, &@$ );
      $$->as.udef_lit.param_ast_list = $3;
      c_ast_set_parent( $4, $$ );

      DUMP_AST( "user_defined_literal_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

/// English C/C++ variable declaration ////////////////////////////////////////

var_decl_english_ast
    /*
     * Ordinary variable declaration.
     */
  : sname_c Y_AS decl_english_ast
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
        C_TYPE_ADD( &$$->type, &$1, @1 );
      }

      DUMP_AST( "type_english_ast", $$ );
      DUMP_END();
    }

  | // in_attr: qualifier
    type_modifier_list_english_type     // allows implicit int in K&R C
    {
      DUMP_START( "type_english_ast", "type_modifier_list_english_type" );
      DUMP_TYPE( "type_modifier_list_english_type", $1 );

      // see the comment in "type_c_ast"
      c_type_t type = C_TYPE_LIT_B( opt_lang < LANG_C_99 ? TB_INT : TB_NONE );

      C_TYPE_ADD( &type, &$1, @1 );

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
      C_TYPE_ADD( &$$, &$2, @2 );

      DUMP_TYPE( "type_modifier_list_english_type", $$ );
      DUMP_END();
    }

  | type_modifier_english_type
  ;

type_modifier_english_type
  : type_modifier_base_type
  ;

unmodified_type_english_ast
  : builtin_type_ast
  | enum_class_struct_union_english_ast
  | typedef_type_c_ast
  ;

enum_class_struct_union_english_ast
  : enum_class_struct_union_c_btid any_sname_c_exp
    of_type_enum_fixed_type_english_ast_opt
    {
      DUMP_START( "enum_class_struct_union_english_ast",
                  "enum_class_struct_union_c_btid sname" );
      DUMP_TID( "enum_class_struct_union_c_btid", $1 );
      DUMP_SNAME( "sname", $2 );
      DUMP_AST( "enum_fixed_type_english_ast", $3 );

      $$ = c_ast_new_gc( K_ENUM_CLASS_STRUCT_UNION, &@$ );
      $$->type.btids = c_tid_check( $1, C_TPID_BASE );
      $$->as.ecsu.of_ast = $3;
      $$->as.ecsu.ecsu_sname = $2;

      DUMP_AST( "enum_class_struct_union_english_ast", $$ );
      DUMP_END();
    }
  ;

of_type_enum_fixed_type_english_ast_opt
  : /* empty */                         { $$ = NULL; }
  | Y_OF type_opt enum_fixed_type_english_ast
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
        C_TYPE_ADD_TID( &$$->type, $1, @1 );
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
      C_TID_ADD( &$$, $2, @2 );

      DUMP_TID( "enum_fixed_type_modifier_list_english_btid", $$ );
      DUMP_END();
    }

  | enum_fixed_type_modifier_btid
  ;

enum_unmodified_fixed_type_english_ast
  : builtin_type_ast
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

typedef_type_c_ast
  : any_typedef sub_scope_sname_c_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();
      c_ast_t const *type_for_ast = $1->ast;

      DUMP_START( "typedef_type_c_ast", "any_typedef" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "any_typedef.ast", type_for_ast );
      DUMP_SNAME( "sub_scope_sname_c_opt", $2 );

      if ( c_sname_empty( &$2 ) ) {
ttntd:  $$ = c_ast_new_gc( K_TYPEDEF, &@$ );
        $$->type.btids = TB_TYPEDEF;
        $$->as.tdef.for_ast = type_for_ast;
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

scope_sname_c_opt
  : /* empty */                   { c_sname_init( &$$ ); }

  | sname_c Y_COLON2
    {
      $$ = $1;
      c_sname_set_local_type( &$$, &C_TYPE_LIT_B( TB_SCOPE ) );
    }

  | any_typedef Y_COLON2
    { //
      // This is for a case like:
      //
      //      define S as struct S
      //      explain bool S::operator!() const
      //
      // that is: a typedef'd type used for a scope.
      //
      $$ = c_sname_dup( &$1->ast->sname );
    }
  ;

sname_c
  : sname_c Y_COLON2 Y_NAME
    {
      // see the comment in "of_scope_english"
      if ( unsupported( LANG_CPP_ANY ) ) {
        print_error( &@2, "scoped names not supported in C\n" );
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
    sname_c bit_field_c_int_opt
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

      DUMP_START( "sname_c_ast", "sname_c" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_SNAME( "sname", $1 );
      DUMP_INT( "bit_field_c_int_opt", $2 );

      $$ = type_ast;
      c_sname_set( &$$->sname, &$1 );

      bool ok = true;
      //
      // This check has to be done now in the parser rather than later in the
      // AST since we need to use the builtin union member now.
      //
      if ( $2 != 0 && (ok = c_ast_is_builtin_any( $$, TB_ANY_INTEGRAL )) )
        $$->as.builtin.bit_width = (c_bit_width_t)$2;

      DUMP_AST( "sname_c_ast", $$ );
      DUMP_END();

      if ( !ok ) {
        print_error( &@2, "bit-fields can be only of integral types\n" );
        PARSE_ABORT();
      }
    }
  ;

bit_field_c_int_opt
  : /* empty */                   { $$ = 0; }
  | ':' int_exp
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since we use 0 to mean "no bit-field."
      //
      if ( $2 == 0 ) {
        print_error( &@2, "bit-field width must be > 0\n" );
        PARSE_ABORT();
      }
      $$ = $2;
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

      c_sname_t sname = $2;
      c_sname_append_name( &sname, $1 );

      //
      // See if the full name is the name of a typedef'd type.
      //
      c_typedef_t const *const tdef = c_typedef_find_sname( &sname );
      if ( tdef != NULL ) {
        $$ = c_ast_new_gc( K_TYPEDEF, &@$ );
        $$->type.btids = TB_TYPEDEF;
        $$->as.tdef.for_ast = tdef->ast;
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

sname_list_english
  : sname_list_english ',' sname_english
    {
      DUMP_START( "sname_list_english", "sname_english" );
      DUMP_SNAME_LIST( "sname_list_english", $1 );
      DUMP_SNAME( "sname_english", $3 );

      $$ = $1;
      c_sname_t *const temp_sname = MALLOC( c_sname_t, 1 );
      *temp_sname = $3;
      slist_push_tail( &$$, temp_sname );

      DUMP_SNAME_LIST( "sname_list_english", $$ );
      DUMP_END();
    }

  | sname_english
    {
      DUMP_START( "sname_list_english", "sname_english" );
      DUMP_SNAME( "sname_english", $1 );

      slist_init( &$$ );
      c_sname_t *const temp_sname = MALLOC( c_sname_t, 1 );
      *temp_sname = $1;
      slist_push_tail( &$$, temp_sname );

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
  : Y_ADDRESS
  | error
    {
      keyword_expected( L_ADDRESS );
    }
  ;

array_exp
  : Y_ARRAY
  | error
    {
      keyword_expected( L_ARRAY );
    }
  ;

as_exp
  : Y_AS
    {
      if ( OPT_LANG_IS(CPP_ANY) ) {
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
      keyword_expected( L_AS );
    }
  ;

as_into_to_exp
  : Y_AS
  | Y_INTO
  | Y_TO
  | error
    {
      elaborate_error(
        "\"%s\", \"%s\", or \"%s\" expected",
        L_AS, L_INTO, L_TO
      );
    }
  ;

cast_exp
  : Y_CAST
  | error
    {
      keyword_expected( L_CAST );
    }
  ;

class_struct_union_btid_exp
  : class_struct_union_btid
  | error
    {
      elaborate_error(
        "\"%s\", \"%s\" or \"%s\" expected",
        L_CLASS, L_STRUCT, L_UNION
      );
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
  : Y_CONVERSION
  | error
    {
      keyword_expected( L_CONVERSION );
    }
  ;

c_operator
  : Y_NEW                           { $$ = C_OP_NEW             ; }
  | Y_NEW '[' rbracket_exp          { $$ = C_OP_NEW_ARRAY       ; }
  | Y_DELETE                        { $$ = C_OP_DELETE          ; }
  | Y_DELETE '[' rbracket_exp       { $$ = C_OP_DELETE_ARRAY    ; }
  | Y_EXCLAM                        { $$ = C_OP_EXCLAM          ; }
  | Y_EXCLAM_EQ                     { $$ = C_OP_EXCLAM_EQ       ; }
  | '%'                             { $$ = C_OP_PERCENT         ; }
  | Y_PERCENT_EQ                    { $$ = C_OP_PERCENT_EQ      ; }
  | Y_AMPER                         { $$ = C_OP_AMPER           ; }
  | Y_AMPER2                        { $$ = C_OP_AMPER2          ; }
  | Y_AMPER_EQ                      { $$ = C_OP_AMPER_EQ        ; }
  | '(' rparen_exp                  { $$ = C_OP_PARENS          ; }
  | '*'                             { $$ = C_OP_STAR            ; }
  | Y_STAR_EQ                       { $$ = C_OP_STAR_EQ         ; }
  | '+'                             { $$ = C_OP_PLUS            ; }
  | Y_PLUS2                         { $$ = C_OP_PLUS2           ; }
  | Y_PLUS_EQ                       { $$ = C_OP_PLUS_EQ         ; }
  | ','                             { $$ = C_OP_COMMA           ; }
  | '-'                             { $$ = C_OP_MINUS           ; }
  | Y_MINUS2                        { $$ = C_OP_MINUS2          ; }
  | Y_MINUS_EQ                      { $$ = C_OP_MINUS_EQ        ; }
  | Y_ARROW                         { $$ = C_OP_ARROW           ; }
  | Y_ARROW_STAR                    { $$ = C_OP_ARROW_STAR      ; }
  | '.'                             { $$ = C_OP_DOT             ; }
  | Y_DOT_STAR                      { $$ = C_OP_DOT_STAR        ; }
  | '/'                             { $$ = C_OP_SLASH           ; }
  | Y_SLASH_EQ                      { $$ = C_OP_SLASH_EQ        ; }
  | Y_COLON2                        { $$ = C_OP_COLON2          ; }
  | '<'                             { $$ = C_OP_LESS            ; }
  | Y_LESS2                         { $$ = C_OP_LESS2           ; }
  | Y_LESS2_EQ                      { $$ = C_OP_LESS2_EQ        ; }
  | Y_LESS_EQ                       { $$ = C_OP_LESS_EQ         ; }
  | Y_LESS_EQ_GREATER               { $$ = C_OP_LESS_EQ_GREATER ; }
  | '='                             { $$ = C_OP_EQ              ; }
  | Y_EQ2                           { $$ = C_OP_EQ2             ; }
  | '>'                             { $$ = C_OP_GREATER         ; }
  | Y_GREATER2                      { $$ = C_OP_GREATER2        ; }
  | Y_GREATER2_EQ                   { $$ = C_OP_GREATER2_EQ     ; }
  | Y_GREATER_EQ                    { $$ = C_OP_GREATER_EQ      ; }
  | Y_QMARK_COLON                   { $$ = C_OP_QMARK_COLON     ; }
  | '[' rbracket_exp                { $$ = C_OP_BRACKETS        ; }
  | Y_CIRC                          { $$ = C_OP_CIRC            ; }
  | Y_CIRC_EQ                       { $$ = C_OP_CIRC_EQ         ; }
  | Y_PIPE                          { $$ = C_OP_PIPE            ; }
  | Y_PIPE2                         { $$ = C_OP_PIPE2           ; }
  | Y_PIPE_EQ                       { $$ = C_OP_PIPE_EQ         ; }
  | Y_TILDE                         { $$ = C_OP_TILDE           ; }
  ;

dependency_exp
  : Y_DEPENDENCY
  | error
    {
      keyword_expected( L_DEPENDENCY );
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
  : Y_EXTERN linkage_stid         { $$ = $2; }
  | Y_EXTERN linkage_stid '{'
    {
      print_error( &@3,
        "scoped linkage declarations not supported by %s\n",
        CDECL
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
      if ( OPT_LANG_IS(C_ANY) && strchr( $1, ':' ) != NULL ) {
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
  | Y_INLINE
  ;

int_exp
  : Y_INT_LIT
  | error
    {
      elaborate_error( "integer expected" );
    }
  ;

literal_exp
  : Y_LITERAL
  | error
    {
      keyword_expected( L_LITERAL );
    }
  ;

local_exp
  : Y_LOCAL
  | error
    {
      keyword_expected( L_LOCAL );
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

member_or_non_member_mask_opt
  : /* empty */                   { $$ = C_FN_UNSPECIFIED; }
  | Y_MEMBER                      { $$ = C_FN_MEMBER     ; }
  | Y_NON_MEMBER                  { $$ = C_FN_NON_MEMBER ; }
  ;

namespace_btid_exp
  : Y_NAMESPACE
  | error
    {
      keyword_expected( L_NAMESPACE );
    }
  ;

namespace_type
  : Y_NAMESPACE                   { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_INLINE namespace_btid_exp   { $$ = C_TYPE_LIT( $2, $1, TA_NONE ); }
  ;

of_exp
  : Y_OF
  | error
    {
      keyword_expected( L_OF );
    }
  ;

of_scope_english
  : Y_OF scope_english_type_exp any_sname_c_exp
    { //
      // Scoped names are supported only in C++.  (However, we always allow
      // them in configuration files.)
      //
      // This check is better to do now in the parser rather than later in the
      // AST because it has to be done in fewer places in the code plus gives a
      // better error location.
      //
      if ( unsupported( LANG_CPP_ANY ) ) {
        print_error( &@2, "scoped names not supported in C\n" );
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
  : Y_OPERATOR
  | error
    {
      keyword_expected( L_OPERATOR );
    }
  ;

operator_opt
  : /* empty */
  | Y_OPERATOR
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
  : Y_REFERENCE
  | error
    {
      keyword_expected( L_REFERENCE );
    }
  ;

returning_exp
  : Y_RETURNING
  | error
    {
      keyword_expected( L_RETURNING );
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
  | Y_SCOPE                       { $$ = C_TYPE_LIT_B( TB_SCOPE ); }
  ;

scope_english_type_exp
  : scope_english_type
  | error
    {
      elaborate_error(
        "\"%s\", \"%s\", \"%s\", \"%s\", or \"%s\" expected",
        L_CLASS, L_NAMESPACE, L_SCOPE, L_STRUCT, L_UNION
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
  | Y_STATIC
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

to_exp
  : Y_TO
  | error
    {
      keyword_expected( L_TO );
    }
  ;

type_opt
  : /* empty */
  | Y_TYPEDEF
  ;

typename_flag_opt
  : /* empty */                   { $$ = false; }
  | Y_TYPENAME                    { $$ = true; }
  ;

unused_exp
  : Y_UNUSED
  | error
    {
      keyword_expected( L_UNUSED );
    }
  ;

virtual_stid_exp
  : Y_VIRTUAL
  | error
    {
      keyword_expected( L_VIRTUAL );
    }
  ;

virtual_stid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_VIRTUAL
  ;

%%

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans up global parser data at program termination.
 */
void parser_cleanup( void ) {
  c_ast_list_gc( &typedef_ast_list );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
