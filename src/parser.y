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

%expect 47

%{
/** @endcond */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_operator.h"
#include "c_type.h"
#include "c_typedef.h"
#include "cdecl.h"
#include "color.h"
#ifdef ENABLE_CDECL_DEBUG
#include "dump.h"
#endif /* ENABLE_CDECL_DEBUG */
#include "did_you_mean.h"
#include "english.h"
#include "gibberish.h"
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
 * Calls c_ast_check_declaration(): if the check fails, calls #PARSE_ABORT().
 *
 * @param AST The AST to check.
 */
#define C_AST_CHECK_DECL(AST) \
  BLOCK( if ( !c_ast_check_declaration( AST ) ) PARSE_ABORT(); )

/**
 * Calls c_type_add(): if adding the type fails, calls #PARSE_ABORT().
 *
 * @param DST_TYPE The <code>\ref c_type</code> to add to.
 * @param NEW_TYPE The <code>\ref c_type</code> to add.
 * @param NEW_LOC The source location of \a NEW_TYPE.
 *
 * @sa #C_TYPE_ADD_TID()
 * @sa #C_TYPE_ID_ADD()
 */
#define C_TYPE_ADD(DST_TYPE,NEW_TYPE,NEW_LOC) BLOCK( \
  if ( !c_type_add( (DST_TYPE), (NEW_TYPE), &(NEW_LOC) ) ) PARSE_ABORT(); )

/**
 * Calls c_type_add_tid(): if adding the type fails, calls #PARSE_ABORT().
 *
 * @param DST_TYPE The <code>\ref c_type</code> to add to.
 * @param NEW_TID The <code>\ref c_type_id_t</code> to add.
 * @param NEW_LOC The source location of \a NEW_TID.
 *
 * @sa #C_TYPE_ADD()
 * @sa #C_TYPE_ID_ADD()
 */
#define C_TYPE_ADD_TID(DST_TYPE,NEW_TID,NEW_LOC) BLOCK( \
  if ( !c_type_add_tid( (DST_TYPE), (NEW_TID), &(NEW_LOC) ) ) PARSE_ABORT(); )

/**
 * Calls c_type_id_add(): if adding the type fails, calls #PARSE_ABORT().
 *
 * @param DST_TID The <code>\ref c_type_id_t</code> to add to.
 * @param NEW_TID The <code>\ref c_type_id_t</code> to add.
 * @param NEW_LOC The source location of \a NEW_TID.
 *
 * @sa #C_TYPE_ADD()
 * @sa #C_TYPE_ADD_TID()
 */
#define C_TYPE_ID_ADD(DST_TID,NEW_TID,NEW_LOC) BLOCK( \
  if ( !c_type_id_add( (DST_TID), (NEW_TID), &(NEW_LOC) ) ) PARSE_ABORT(); )

/**
 * Calls #elaborate_error_dym() with a <code>\ref dym_kind_t</code> of
 * #DYM_NONE.
 *
 * @param ... Arguments passed to fl_elaborate_error().
 *
 * @note
 * This must be used _only_ after an `error` token, e.g.:
 * @code
 *  | Y_DEFINE error
 *    {
 *      elaborate_error( "name expected" );
 *    }
 * @endcode
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
 */
#define elaborate_error_dym(DYM_KINDS,...) BLOCK( \
  fl_elaborate_error( __FILE__, __LINE__, (DYM_KINDS), __VA_ARGS__ ); PARSE_ABORT(); )

/**
 * Aborts the current parse (presumably after an error message has been
 * printed).
 */
#define PARSE_ABORT()             BLOCK( parse_cleanup( true ); YYABORT; )

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
 * Macros that are used to dump a trace during parsing when `opt_cdecl_debug`
 * is `true`.
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
 * Dumps an `s_list` of AST.
 *
 * @param KEY The key name to print.
 * @param AST_LIST The `s_list` of AST to dump.
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
  DUMP_COMMA;                           \
  FPRINTF( stdout, "  " KEY " = %s", ((BOOL) ? "true" : "false") ); )

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
 * @sa #DUMP_STR()
 */
#define DUMP_SNAME(KEY,SNAME) IF_DEBUG( \
  DUMP_COMMA; PUTS( "  " KEY " = " );   \
  c_sname_dump( &(SNAME), stdout ); )

/**
 * Dumps a C string.
 *
 * @param KEY The key name to print.
 * @param STR The C string to dump.
 *
 * @sa #DUMP_INT()
 * @sa #DUMP_SNAME()
 */
#define DUMP_STR(KEY,STR) IF_DEBUG(   \
  DUMP_COMMA; PUTS( "  " );           \
  kv_dump( (KEY), (STR), stdout ); )

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
 * (typically, calling `print_error()` and #PARSE_ABORT()) so that the entire
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
 * Ends a dump block.
 *
 * @sa #DUMP_START()
 */
#define DUMP_END()                IF_DEBUG( PUTS( "\n}\n" ); )

/**
 * Dumps a <code>\ref c_type_id_t</code>.
 *
 * @param KEY The key name to print.
 * @param TID The <code>\ref c_type_id_t</code> to dump.
 *
 * @sa #DUMP_TYPE()
 */
#define DUMP_TID(KEY,TID) IF_DEBUG( \
  DUMP_COMMA; PUTS( "  " KEY " = " ); c_type_id_dump( (TID), stdout ); )

/**
 * Dumps a <code>\ref c_type</code>.
 *
 * @param KEY The key name to print.
 * @param TYPE The <code>\ref c_type</code> to dump.
 *
 * @sa #DUMP_TID()
 */
#define DUMP_TYPE(KEY,TYPE) IF_DEBUG( \
  DUMP_COMMA; PUTS( "  " KEY " = " ); c_type_dump( (TYPE), stdout ); )

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
  slist_t       qualifier_stack;  ///< `c_qualifier` stack.
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
  c_type_id_t qual_tid;                 ///< E.g., #TS_CONST or #TS_VOLATILE.
  c_loc_t     loc;                      ///< Qualifier source location.
};
typedef struct c_qualifier c_qualifier_t;

/**
 * Information for show_type_visitor().
 */
struct show_type_info {
  unsigned        show_which;           ///< Predefined, user, or both?
  c_gib_kind_t    gib_kind;             ///< Kind of gibberish to print.
};
typedef struct show_type_info show_type_info_t;

// extern functions
extern void           print_help( char const* );

// local variables
static c_ast_depth_t  ast_depth;        ///< Parentheses nesting depth.
static bool           error_newlined = true;
static c_ast_list_t   gc_ast_list;      ///< `c_ast` nodes freed after parse.
static in_attr_t      in_attr;          ///< Inherited attributes.
static c_ast_list_t   typedef_ast_list; ///< `c_ast` nodes for `typedef`s.

////////// inline functions ///////////////////////////////////////////////////

/**
 * Garbage-collect the AST nodes on \a ast_list.
 *
 * @param ast_list The AST list to free.
 */
static inline void c_ast_list_gc( c_ast_list_t *ast_list ) {
  slist_free( ast_list, NULL, (slist_node_data_free_fn_t)&c_ast_free );
}

/**
 * Creates a new AST and adds it to <code>\ref gc_ast_list</code>.
 *
 * @param kind_id The kind of AST to create.
 * @param loc A pointer to the token location data.
 * @return Returns a pointer to a new AST.
 *
 * @sa c_ast_pair_new_gc()
 */
PJL_WARN_UNUSED_RESULT
static inline c_ast_t* c_ast_new_gc( c_kind_id_t kind_id, c_loc_t const *loc ) {
  return c_ast_new( kind_id, ast_depth, loc, &gc_ast_list );
}

/**
 * Set our mode to deciphering gibberish into English.
 */
static inline void gibberish_to_english( void ) {
  c_mode = C_GIBBERISH_TO_ENGLISH;
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
 * Peeks at the location of the qualifier at the top of the
 * \ref in_attr.qualifier_stack "qualifier inherited attribute stack".
 *
 * @note This is a macro instead of an inline function because it should return
 * a reference (not a pointer), but C doesn't have references.
 *
 * @return Returns said qualifier location.
 *
 * @sa ia_qual_peek_tid()
 * @sa ia_qual_pop()
 * @sa ia_qual_push_tid()
 */
#define ia_qual_peek_loc() \
  (((c_qualifier_t*)slist_peek_head( &in_attr.qualifier_stack ))->loc)

/**
 * Peeks at the qualifier at the top of the
 * \ref in_attr.qualifier_stack "qualifier inherited attribute stack".
 *
 * @return Returns said qualifier.
 *
 * @sa #ia_qual_peek_loc()
 * @sa ia_qual_pop()
 * @sa ia_qual_push_tid()
 */
PJL_WARN_UNUSED_RESULT
static inline c_type_id_t ia_qual_peek_tid( void ) {
  c_qualifier_t const *const qual = slist_peek_head( &in_attr.qualifier_stack );
  return qual->qual_tid;
}

/**
 * Pops a qualifier from the
 * \ref in_attr.qualifier_stack "qualifer inherited attribute stack" and frees
 * it.
 *
 * @sa #ia_qual_peek_loc()
 * @sa ia_qual_peek_tid()
 * @sa ia_qual_push_tid()
 */
static inline void ia_qual_pop( void ) {
  FREE( slist_pop_head( &in_attr.qualifier_stack ) );
}

/**
 * Gets a printable string of <code>\ref lexer_token</code>.
 *
 * @return Returns said string or NULL if <code>\ref lexer_token</code> is the
 * empty string.
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
 * Checks if the current language is _not_ among \a lang_ids.
 *
 * @param lang_ids The bitwise-or of language(s).
 * @return Returns `true` only if cdecl has been initialized and `opt_lang` is
 * _not_ among \a lang_ids.
 */
PJL_WARN_UNUSED_RESULT
static inline bool unsupported( c_lang_id_t lang_ids ) {
  return c_initialized && (opt_lang & lang_ids) == LANG_NONE;
}

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans up parser data at program termination.
 */
void parser_cleanup( void ) {
  c_ast_list_gc( &typedef_ast_list );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Adds a type to the global set.
 *
 * @param decl_keyword The keyword used for the declaration.
 * @param type_ast The AST of the type to add.
 * @param type_decl_loc The location of the offending type declaration.
 * @return Returns `true` either if the type was added or it's equivalent to
 * the existing type; `false` if a different type already exists having the
 * same name.
 */
PJL_WARN_UNUSED_RESULT
static bool add_type( char const *decl_keyword, c_ast_t const *type_ast,
                      c_loc_t const *type_decl_loc ) {
  assert( decl_keyword != NULL );
  assert( type_ast != NULL );
  assert( type_decl_loc != NULL );

  c_typedef_t const *const old_tdef = c_typedef_add( type_ast );
  if ( old_tdef == NULL ) {             // type was added
    //
    // We have to move the AST from the gc_ast_list so it won't be garbage
    // collected at the end of the parse to a separate typedef_ast_list that's
    // freed only at program termination.
    //
    slist_push_list_tail( &typedef_ast_list, &gc_ast_list );
  }
  else if ( old_tdef->ast != NULL ) {   // type exists and isn't equivalent
    print_error( type_decl_loc,
      "\"%s\": \"%s\" redefinition with different type; original is: ",
      c_ast_full_name( type_ast ), decl_keyword
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
 * Creates a pointer AST to \a ast.
 *
 * @param ast The AST to create a pointer to.
 * @return Returns the new pointer AST.
 */
c_ast_t* c_ast_pointer( c_ast_t *ast ) {
  c_ast_t *const ptr_ast = c_ast_new_gc( K_POINTER, &ast->loc );
  ptr_ast->sname = c_ast_take_name( ast );
  c_ast_set_parent( ast, ptr_ast );
  return ptr_ast;
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
 * @sa yyerror()
 */
PJL_PRINTF_LIKE_FUNC(4)
static void fl_elaborate_error( char const *file, int line,
                                dym_kind_t dym_kinds, char const *format,
                                ... ) {
  assert( format != NULL );
  if ( error_newlined )
    return;

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
      c_keyword_find( error_token, LANG_ANY, C_KW_CTX_ALL );
    if ( k != NULL ) {
      c_lang_id_t const oldest_lang = c_lang_oldest( k->lang_ids );
      if ( oldest_lang > opt_lang )
        EPRINTF( "; not a keyword until %s", c_lang_name( oldest_lang ) );
      else
        EPRINTF( " (\"%s\" is a keyword)", error_token );
    }
    print_suggestions( dym_kinds, error_token );
  }

  EPUTC( '\n' );
  error_newlined = true;
}

/**
 * Frees all resources used by \ref in_attr "inherited attributes".
 */
static void ia_free( void ) {
  c_sname_free( &in_attr.current_scope );
  slist_free( &in_attr.qualifier_stack, NULL, &free );
  // Do _not_ pass &c_ast_free for the 3rd argument! All AST nodes were already
  // free'd from the gc_ast_list in parse_cleanup(). Just free the slist nodes.
  slist_free( &in_attr.type_ast_stack, NULL, NULL );
  c_ast_list_gc( &in_attr.typedef_ast_list );
  MEM_ZERO( &in_attr );
}

/**
 * Pushes a qualifier onto the
 * \ref in_attr.qualifier_stack "qualifier inherited attribute stack."
 *
 * @param qual_tid The qualifier to push.
 * @param loc A pointer to the source location of the qualifier.
 *
 * @sa #ia_qual_peek_loc()
 * @sa ia_qual_peek_tid()
 * @sa ia_qual_pop()
 */
static void ia_qual_push_tid( c_type_id_t qual_tid, c_loc_t const *loc ) {
  c_type_id_check( qual_tid, C_TPID_STORE );
  assert( (qual_tid & c_type_id_compl( TS_MASK_QUALIFIER )) == TS_NONE );
  assert( loc != NULL );

  c_qualifier_t *const qual = MALLOC( c_qualifier_t, 1 );
  qual->qual_tid = qual_tid;
  qual->loc = *loc;
  slist_push_head( &in_attr.qualifier_stack, qual );
}

/**
 * Cleans-up parser data after each parse.
 *
 * @param hard_reset If `true`, does a "hard" reset that currently resets the
 * EOF flag of the lexer.  This should be `true` if an error occurs and
 * `YYABORT` is called.
 *
 * @sa parse_init()
 */
static void parse_cleanup( bool hard_reset ) {
  lexer_reset( hard_reset );
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
  c_mode = C_ENGLISH_TO_GIBBERISH;
  if ( false_set( &error_newlined ) )
    FPUTC( '\n', fout );
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
 * @param tdef The <code>\ref c_typedef</code> to print.
 * @param data Optional data passed to the visitor: in this case, the bitmask
 * of which `typedef`s to print.
 * @return Always returns `false`.
 */
PJL_WARN_UNUSED_RESULT
static bool show_type_visitor( c_typedef_t const *tdef, void *data ) {
  assert( tdef != NULL );
  assert( data != NULL );

  show_type_info_t const *const sti = data;

  bool const show_type = tdef->user_defined ?
    (sti->show_which & SHOW_USER_TYPES) != 0 :
    (sti->show_which & SHOW_PREDEFINED_TYPES) != 0;

  bool const show_in_lang =
    (sti->show_which & SHOW_ALL_TYPES) != 0 ||
    (tdef->lang_ids & opt_lang) != LANG_NONE;

  if ( show_type && show_in_lang ) {
    if ( sti->gib_kind == C_GIB_NONE )
      c_ast_explain_type( tdef->ast, fout );
    else
      c_typedef_gibberish( tdef, sti->gib_kind, fout );
  }
  return false;
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
 * @sa print_loc()
 */
static void yyerror( char const *msg ) {
  assert( msg != NULL );

  c_loc_t const loc = lexer_loc();
  print_loc( &loc );

  SGR_START_COLOR( stderr, error );
  EPUTS( msg );                         // no newline
  SGR_END_COLOR( stderr );
  error_newlined = false;

  parse_cleanup( false );
}

/** @} */

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

%}

//
// The difference between "literal" and "name" is that "literal" points to a
// string that a rule is NOT responsible for free'ing (a predefined `L_`
// literal) whereas "name" points to a string that a rule IS responsible for
// free'ing.
//
%union {
  c_alignas_t         align;
  c_ast_t            *ast;        // for the AST being built
  c_ast_list_t        ast_list;   // for declarations and function parameters
  c_ast_pair_t        ast_pair;   // for the AST being built
  unsigned            bitmask;    // multipurpose bitmask (used by show)
  bool                flag;       // simple flag
  int                 int_val;    // for array sizes
  c_gib_kind_t        gib_kind;   // kind of gibberish
  char const         *literal;    // token L_* literal (for new-style casts)
  char               *name;       // name being declared or explained
  c_oper_id_t         oper_id;    // overloaded operator ID
  c_sname_t           sname;      // name being declared or explained
  c_typedef_t const  *tdef;       // typedef
  c_type_t            type;       // complete type
  c_type_id_t         type_id;    // built-ins, storage classes, & qualifiers
}

                    // cdecl commands
%token              Y_CAST
//                  Y_CLASS             // covered in C++11
//                  Y_CONST             // covered in C89
%token              Y_DECLARE
%token              Y_DEFINE
%token              Y_DYNAMIC
//                  Y_EXIT              // mapped by lexer to Y_QUIT
%token              Y_EXPLAIN
%token              Y_HELP
//                  Y_NAMESPACE         // covered in C++11
%token              Y_QUIT
%token              Y_REINTERPRET
%token              Y_SET
%token              Y_SHOW
//                  Y_STATIC            // covered in K&R C
//                  Y_STRUCT            // covered in K&R C
//                  Y_TYPEDEF           // covered in K&R C
//                  Y_UNION             // covered in K&R C
//                  Y_USING             // covered in C++11

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
%token              Y_FUNCTION
%token              Y_INTO
%token              Y_LENGTH
%token              Y_LINKAGE
%token              Y_LITERAL
%token              Y_MEMBER
%token              Y_NON_MEMBER
%token              Y_OF
%token              Y_POINTER
%token              Y_PREDEFINED
%token              Y_PURE
%token              Y_REFERENCE
%token              Y_RETURNING
%token              Y_RVALUE
%token              Y_SCOPE
%token              Y_TO
%token              Y_TYPES
%token              Y_USER
%token              Y_USER_DEFINED
%token              Y_VARIABLE
%token              Y_WIDTH

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
%token  <type_id>   Y_AUTO_STORAGE      // C version of "auto"
%token              Y_BREAK
%token              Y_CASE
%token  <type_id>   Y_CHAR
%token              Y_CONTINUE
%token  <type_id>   Y_DEFAULT
%token              Y_DO
%token  <type_id>   Y_DOUBLE
%token              Y_ELSE
%token  <type_id>   Y_EXTERN
%token  <type_id>   Y_FLOAT
%token              Y_FOR
%token              Y_GOTO
%token              Y_IF
%token  <type_id>   Y_INT
%token  <type_id>   Y_LONG
%token  <type_id>   Y_REGISTER
%token              Y_RETURN
%token  <type_id>   Y_SHORT
%token  <type_id>   Y_STATIC
%token  <type_id>   Y_STRUCT
%token              Y_SWITCH
%token  <type_id>   Y_TYPEDEF
%token  <type_id>   Y_UNION
%token  <type_id>   Y_UNSIGNED
%token              Y_WHILE

                    // C89
%token              Y_ASM
%token  <type_id>   Y_CONST
%token              Y_ELLIPSIS    "..." // for varargs
%token  <type_id>   Y_ENUM
%token  <type_id>   Y_SIGNED
%token  <type_id>   Y_VOID
%token  <type_id>   Y_VOLATILE

                    // C95
%token  <type_id>   Y_WCHAR_T

                    // C99
%token  <type_id>   Y__BOOL
%token  <type_id>   Y__COMPLEX
%token  <type_id>   Y__IMAGINARY
%token  <type_id>   Y_INLINE
%token  <type_id>   Y_RESTRICT

                    // C11
%token              Y__ALIGNAS
%token              Y__ALIGNOF
%token  <type_id>   Y__ATOMIC_QUAL      // qualifier: _Atomic type
%token  <type_id>   Y__ATOMIC_SPEC      // specifier: _Atomic (type)
%token              Y__GENERIC
%token  <type_id>   Y__NORETURN
%token              Y__STATIC_ASSERT
%token  <type_id>   Y__THREAD_LOCAL

                    // C++
%token  <type_id>   Y_BOOL
%token              Y_CATCH
%token  <type_id>   Y_CLASS
%token  <literal>   Y_CONST_CAST
%token  <sname>     Y_CONSTRUCTOR_SNAME // e.g., S::T::T
%token  <type_id>   Y_DELETE
%token  <sname>     Y_DESTRUCTOR_SNAME  // e.g., S::T::~T
%token  <literal>   Y_DYNAMIC_CAST
%token  <type_id>   Y_EXPLICIT
%token  <type_id>   Y_FALSE             // for noexcept(false)
%token  <type_id>   Y_FRIEND
%token  <type_id>   Y_MUTABLE
%token  <type_id>   Y_NAMESPACE
%token              Y_NEW
%token              Y_OPERATOR
%token              Y_PRIVATE
%token              Y_PROTECTED
%token              Y_PUBLIC
%token  <literal>   Y_REINTERPRET_CAST
%token  <literal>   Y_STATIC_CAST
%token              Y_TEMPLATE
%token              Y_THIS
%token  <type_id>   Y_THROW
%token  <type_id>   Y_TRUE              // for noexcept(true)
%token              Y_TRY
%token              Y_TYPEID
%token  <flag>      Y_TYPENAME
%token  <type_id>   Y_USING
%token  <type_id>   Y_VIRTUAL

                    // C11 & C++11
%token  <type_id>   Y_CHAR16_T
%token  <type_id>   Y_CHAR32_T

                    // C2X & C++11
%token              Y_ATTR_BEGIN        // First '[' of "[[" for an attribute.

                    // C++11
%token              Y_ALIGNAS
%token              Y_ALIGNOF
%token  <type_id>   Y_AUTO_TYPE         // C++11 version of "auto"
%token  <type_id>   Y_CARRIES_DEPENDENCY
%token  <type_id>   Y_CONSTEXPR
%token              Y_DECLTYPE
%token  <type_id>   Y_FINAL
%token  <type_id>   Y_NOEXCEPT
%token              Y_NULLPTR
%token  <type_id>   Y_OVERRIDE
%token              Y_QUOTE2            // for user-defined literals
%token              Y_STATIC_ASSERT
%token  <type_id>   Y_THREAD_LOCAL

                    // C2X & C++14
%token  <type_id>   Y_DEPRECATED

                    // C2X & C++17
%token  <type_id>   Y_MAYBE_UNUSED
%token  <type_id>   Y_NODISCARD

                    // C++17
%token  <type_id>   Y_NORETURN

                    // C2X & C++20
%token  <type_id>   Y_CHAR8_T

                    // C++20
%token              Y_CONCEPT
%token  <type_id>   Y_CONSTEVAL
%token  <type_id>   Y_CONSTINIT
%token              Y_CO_AWAIT
%token              Y_CO_RETURN
%token              Y_CO_YIELD
%token  <type_id>   Y_EXPORT
%token  <type_id>   Y_NO_UNIQUE_ADDRESS
%token              Y_REQUIRES

                    // Embedded C extensions
%token  <type_id>   Y_EMC__ACCUM
%token  <type_id>   Y_EMC__FRACT
%token  <type_id>   Y_EMC__SAT

                    // Unified Parallel C extensions
%token  <type_id>   Y_UPC_RELAXED
%token  <type_id>   Y_UPC_SHARED
%token  <type_id>   Y_UPC_STRICT

                    // GNU extensions
%token              Y_GNU___ATTRIBUTE__
%token  <type_id>   Y_GNU___RESTRICT

                    // Apple extensions
%token  <type_id>   Y_APPLE___BLOCK     // __block storage class
%token              Y_APPLE_BLOCK       // English for '^'

                    // Miscellaneous
%token              ':'
%token              ';'
%token              '{' '}'
%token  <name>      Y_CHAR_LIT
%token              Y_END
%token              Y_ERROR
%token  <int_val>   Y_INT_LIT
%token  <name>      Y_NAME
%token  <name>      Y_SET_OPTION
%token  <name>      Y_STR_LIT
%token  <tdef>      Y_TYPEDEF_NAME      // e.g., size_t
%token  <tdef>      Y_TYPEDEF_SNAME     // e.g., std::string

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
//      + <type_id>: "_tid" is appended.
//      + <type>: "_type" is appended.
//  4. Is expected, "_exp" is appended; is optional, "_opt" is appended.
//
// Sort using: sort -bdk3

                    // Pseudo-English
%type   <ast>       alignas_or_width_decl_english_ast
%type   <align>     alignas_specifier_english
%type   <ast>       array_decl_english_ast
%type   <int_val>   array_size_int_opt
%type   <type_id>   attribute_english_tid
%type   <ast>       block_decl_english_ast
%type   <ast>       constructor_decl_english_ast
%type   <ast>       decl_english_ast
%type   <ast_list>  decl_list_english decl_list_english_opt
%type   <ast>       destructor_decl_english_ast
%type   <ast>       enum_class_struct_union_english_ast
%type   <ast>       enum_fixed_type_english_ast
%type   <type_id>   enum_fixed_type_modifier_list_english_tid
%type   <type_id>   enum_fixed_type_modifier_list_english_tid_opt
%type   <ast>       enum_unmodified_fixed_type_english_ast
%type   <ast>       func_decl_english_ast
%type   <bitmask>   member_or_non_member_mask_opt
%type   <literal>   new_style_cast_english_literal
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
%type   <type_id>   ref_qualifier_english_tid_opt
%type   <ast>       returning_english_ast returning_english_ast_opt
%type   <type>      scope_english_type scope_english_type_exp
%type   <sname>     sname_english sname_english_exp
%type   <ast>       sname_english_ast
%type   <type>      storage_class_english_type
%type   <type>      storage_class_list_english_type_opt
%type   <ast>       type_english_ast
%type   <type>      type_modifier_english_type
%type   <type>      type_modifier_list_english_type
%type   <type>      type_modifier_list_english_type_opt
%type   <type_id>   type_qualifier_list_english_tid
%type   <type_id>   type_qualifier_list_english_tid_opt
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
%type   <literal>   new_style_cast_c_literal
%type   <ast>       pointer_cast_c_ast
%type   <ast>       pointer_to_member_cast_c_ast
%type   <ast>       reference_cast_c_ast

                    // C/C++ declarations
%type   <align>     alignas_specifier_c
%type   <sname>     any_sname_c any_sname_c_exp any_sname_c_opt
%type   <ast_pair>  array_decl_c_astp
%type   <ast>       array_size_c_ast
%type   <int_val>   array_size_c_int
%type   <ast>       atomic_specifier_type_c_ast
%type   <type_id>   attribute_c_tid
%type   <type_id>   attribute_list_c_tid attribute_list_c_tid_opt
%type   <type_id>   attribute_specifier_list_c_tid
%type   <type_id>   attribute_specifier_list_c_tid_opt
%type   <int_val>   bit_field_c_int_opt
%type   <ast_pair>  block_decl_c_astp
%type   <ast_pair>  decl_c_astp decl2_c_astp
%type   <ast>       destructor_decl_c_ast
%type   <ast>       enum_class_struct_union_c_ast
%type   <ast>       enum_fixed_type_c_ast enum_fixed_type_c_ast_opt
%type   <type_id>   enum_fixed_type_modifier_list_tid
%type   <type_id>   enum_fixed_type_modifier_list_tid_opt
%type   <type_id>   enum_fixed_type_modifier_tid
%type   <ast>       enum_unmodified_fixed_type_c_ast
%type   <ast>       file_scope_constructor_decl_c_ast
%type   <ast>       file_scope_destructor_decl_c_ast
%type   <ast_pair>  func_decl_c_astp
%type   <type_id>   func_equals_c_tid_opt
%type   <type_id>   func_qualifier_c_tid
%type   <type_id>   func_qualifier_list_c_tid_opt
%type   <type_id>   func_ref_qualifier_c_tid_opt
%type   <ast>       knr_func_or_constructor_decl_c_ast
%type   <type_id>   linkage_tid
%type   <ast_pair>  nested_decl_c_astp
%type   <type_id>   noexcept_c_tid_opt
%type   <ast>       oper_c_ast
%type   <ast_pair>  oper_decl_c_astp
%type   <ast>       param_c_ast
%type   <ast_list>  param_list_c_ast param_list_c_ast_opt
%type   <ast>       placeholder_c_ast
%type   <ast_pair>  pointer_decl_c_astp
%type   <ast_pair>  pointer_to_member_decl_c_astp
%type   <ast>       pointer_to_member_type_c_ast
%type   <ast>       pointer_type_c_ast
%type   <ast_pair>  reference_decl_c_astp
%type   <ast>       reference_type_c_ast
%type   <type_id>   restrict_qualifier_c_tid
%type   <type_id>   restrict_qualifier_c_tid_opt
%type   <type_id>   rparen_func_qualifier_list_c_tid_opt
%type   <sname>     scope_sname_c_opt
%type   <sname>     sname_c sname_c_exp sname_c_opt
%type   <ast>       sname_c_ast
%type   <type_id>   extern_linkage_c_tid extern_linkage_c_tid_opt
%type   <type>      storage_class_c_type
%type   <ast>       trailing_return_type_c_ast_opt
%type   <ast>       type_c_ast
%type   <sname>     typedef_sname_c
%type   <ast>       typedef_type_c_ast
%type   <ast>       typedef_type_decl_c_ast
%type   <type>      type_modifier_c_type
%type   <type>      type_modifier_list_c_type type_modifier_list_c_type_opt
%type   <type_id>   type_qualifier_c_tid
%type   <type_id>   type_qualifier_list_c_tid type_qualifier_list_c_tid_opt
%type   <ast>       unmodified_type_c_ast
%type   <ast_pair>  user_defined_conversion_decl_c_astp
%type   <ast>       user_defined_literal_c_ast
%type   <ast_pair>  user_defined_literal_decl_c_astp
%type   <ast>       using_decl_c_ast

                    // C++ user-defined conversions
%type   <ast>       pointer_to_member_udc_decl_c_ast
%type   <ast>       pointer_udc_decl_c_ast
%type   <ast>       reference_udc_decl_c_ast
%type   <ast>       udc_decl_c_ast udc_decl_c_ast_opt

                    // Miscellaneous
%type   <name>      any_name any_name_exp
%type   <tdef>      any_typedef
%type   <type_id>   builtin_tid
%type   <ast>       builtin_type_ast
%type   <type_id>   class_struct_tid class_struct_tid_exp class_struct_tid_opt
%type   <type_id>   class_struct_union_tid
%type   <oper_id>   c_operator
%type   <type_id>   cv_qualifier_tid cv_qualifier_list_tid_opt
%type   <type_id>   enum_tid enum_class_struct_union_c_tid
%type   <literal>   help_what_literal_opt
%type   <type_id>   inline_tid_opt
%type   <int_val>   int_exp
%type   <ast>       name_ast
%type   <name>      name_exp
%type   <type_id>   namespace_tid_exp
%type   <type>      namespace_type
%type   <type_id>   no_except_bool_tid_exp
%type   <bitmask>   predefined_or_user_mask_opt
%type   <name>      set_option_value_opt
%type   <gib_kind>  show_format show_format_exp show_format_opt
%type   <bitmask>   show_which_types_mask_opt
%type   <type_id>   static_tid_opt
%type   <type>      type_modifier_base_type
%type   <flag>      typename_flag_opt
%type   <type_id>   virtual_tid_exp virtual_tid_opt

//
// Bison %destructors.  We don't use the <identifier> syntax because older
// versions of Bison don't support it.
//
// Clean-up of AST nodes is done via garbage collection using gc_ast_list.
//

// c_ast_list_t
%destructor { DTRACE; c_ast_list_free( &$$ ); } decl_list_english
%destructor { DTRACE; c_ast_list_free( &$$ ); } decl_list_english_opt
%destructor { DTRACE; c_ast_list_free( &$$ ); } paren_decl_list_english
%destructor { DTRACE; c_ast_list_free( &$$ ); } paren_decl_list_english_opt
%destructor { DTRACE; c_ast_list_free( &$$ ); } param_list_c_ast
%destructor { DTRACE; c_ast_list_free( &$$ ); } param_list_c_ast_opt

// name
%destructor { DTRACE; FREE( $$ ); } any_name
%destructor { DTRACE; FREE( $$ ); } any_name_exp
%destructor { DTRACE; FREE( $$ ); } Y_CHAR_LIT
%destructor { DTRACE; FREE( $$ ); } name_exp
%destructor { DTRACE; FREE( $$ ); } set_option_value_opt
%destructor { DTRACE; FREE( $$ ); } Y_NAME
%destructor { DTRACE; FREE( $$ ); } Y_SET_OPTION
%destructor { DTRACE; FREE( $$ ); } Y_STR_LIT

// sname
%destructor { DTRACE; c_sname_free( &$$ ); } any_sname_c
%destructor { DTRACE; c_sname_free( &$$ ); } any_sname_c_exp
%destructor { DTRACE; c_sname_free( &$$ ); } any_sname_c_opt
%destructor { DTRACE; c_sname_free( &$$ ); } of_scope_english
%destructor { DTRACE; c_sname_free( &$$ ); } of_scope_list_english
%destructor { DTRACE; c_sname_free( &$$ ); } of_scope_list_english_opt
%destructor { DTRACE; c_sname_free( &$$ ); } scope_sname_c_opt
%destructor { DTRACE; c_sname_free( &$$ ); } sname_c
%destructor { DTRACE; c_sname_free( &$$ ); } sname_c_exp
%destructor { DTRACE; c_sname_free( &$$ ); } sname_c_opt
%destructor { DTRACE; c_sname_free( &$$ ); } sname_english
%destructor { DTRACE; c_sname_free( &$$ ); } sname_english_exp
%destructor { DTRACE; c_sname_free( &$$ ); } typedef_sname_c
%destructor { DTRACE; c_sname_free( &$$ ); } Y_CONSTRUCTOR_SNAME
%destructor { DTRACE; c_sname_free( &$$ ); } Y_DESTRUCTOR_SNAME

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
  : cast_english semi_or_end
  | declare_english semi_or_end
  | define_english semi_or_end
  | explain_c semi_or_end
  | help_command semi_or_end
  | quit_command semi_or_end
  | scope_declaration_c
  | set_command semi_or_end
  | show_command semi_or_end
  | typedef_declaration_c semi_or_end
  | using_declaration_c semi_or_end
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
//  cast
///////////////////////////////////////////////////////////////////////////////

cast_english
    /*
     * C-style cast.
     */
  : Y_CAST sname_english_exp as_into_to_exp decl_english_ast
    {
      DUMP_START( "cast_english",
                  "CAST sname_english_exp as_into_to_exp decl_english_ast" );
      DUMP_SNAME( "sname_english_exp", $2 );
      DUMP_AST( "decl_english_ast", $4 );
      DUMP_END();

      bool const ok = c_ast_check_cast( $4 );
      if ( ok ) {
        FPUTC( '(', fout );
        c_ast_gibberish( $4, C_GIB_CAST, fout );
        FPRINTF( fout, ")%s\n", c_sname_full_name( &$2 ) );
      }

      c_sname_free( &$2 );
      if ( !ok )
        PARSE_ABORT();
    }

    /*
     * New C++-style cast.
     */
  | new_style_cast_english_literal cast_exp sname_english_exp as_into_to_exp
    decl_english_ast
    {
      DUMP_START( "cast_english",
                  "new_style_cast_english_literal CAST sname_english_exp "
                  "as_into_to_exp decl_english_ast" );
      DUMP_STR( "new_style_cast_english_literal", $1 );
      DUMP_SNAME( "sname_english_exp", $3 );
      DUMP_AST( "decl_english_ast", $5 );
      DUMP_END();

      bool ok = false;

      if ( unsupported( LANG_CPP_MIN(11) ) ) {
        print_error( &@1,
          "%s is not supported%s\n", $1, c_lang_until( LANG_CPP_11 )
        );
      }
      else if ( (ok = c_ast_check_cast( $5 )) ) {
        FPRINTF( fout, "%s<", $1 );
        c_ast_gibberish( $5, C_GIB_CAST, fout );
        FPRINTF( fout, ">(%s)\n", c_sname_full_name( &$3 ) );
      }

      c_sname_free( &$3 );
      if ( !ok )
        PARSE_ABORT();
    }
  ;

new_style_cast_english_literal
  : Y_CONST                       { $$ = L_CONST_CAST;        }
  | Y_DYNAMIC                     { $$ = L_DYNAMIC_CAST;      }
  | Y_REINTERPRET                 { $$ = L_REINTERPRET_CAST;  }
  | Y_STATIC                      { $$ = L_STATIC_CAST;       }
  ;

///////////////////////////////////////////////////////////////////////////////
//  declare
///////////////////////////////////////////////////////////////////////////////

declare_english
    /*
     * Common declaration, e.g.: declare x as int.
     */
  : Y_DECLARE sname_english as_exp storage_class_list_english_type_opt
    alignas_or_width_decl_english_ast
    {
      if ( $5->kind_id == K_NAME ) {
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
        assert( !c_ast_empty_name( $5 ) );
        print_error_unknown_name( &@5, &$5->sname );
        c_sname_free( &$2 );
        PARSE_ABORT();
      }

      DUMP_START( "declare_english",
                  "DECLARE sname AS storage_class_list_english_type_opt "
                  "alignas_or_width_decl_english_ast" );
      DUMP_SNAME( "sname", $2 );
      DUMP_TYPE( "storage_class_list_english_type_opt", &$4 );
      DUMP_AST( "alignas_or_width_decl_english_ast", $5 );

      c_ast_set_sname( $5, &$2 );
      $5->loc = @2;
      C_TYPE_ADD( &$5->type, &$4, @4 );

      DUMP_AST( "decl_english", $5 );
      DUMP_END();

      C_AST_CHECK_DECL( $5 );
      c_ast_gibberish( $5, C_GIB_DECL, fout );
      if ( opt_semicolon )
        FPUTC( ';', fout );
      FPUTC( '\n', fout );
    }

    /*
     * C++ overloaded operator declaration.
     */
  | Y_DECLARE c_operator of_scope_list_english_opt as_exp
    storage_class_list_english_type_opt oper_decl_english_ast
    {
      DUMP_START( "declare_english",
                  "DECLARE c_operator of_scope_list_english_opt AS "
                  "storage_class_list_english_type_opt "
                  "oper_decl_english_ast" );
      DUMP_STR( "c_operator", c_oper_get( $2 )->name );
      DUMP_SNAME( "of_scope_list_english_opt", $3 );
      DUMP_TYPE( "storage_class_list_english_type_opt", &$5 );
      DUMP_AST( "oper_decl_english_ast", $6 );

      c_ast_set_sname( $6, &$3 );
      $6->loc = @2;
      $6->as.oper.oper_id = $2;
      C_TYPE_ADD( &$6->type, &$5, @5 );

      DUMP_AST( "declare_english", $6 );
      DUMP_END();

      C_AST_CHECK_DECL( $6 );
      c_ast_gibberish( $6, C_GIB_DECL, fout );
      if ( opt_semicolon )
        FPUTC( ';', fout );
      FPUTC( '\n', fout );
    }

    /*
     * C++ user-defined conversion operator declaration.
     */
  | Y_DECLARE udc_storage_class_list_english_type_opt cv_qualifier_list_tid_opt
    Y_USER_DEFINED conversion_exp operator_opt of_scope_list_english_opt
    returning_exp decl_english_ast
    {
      DUMP_START( "declare_english",
                  "DECLARE storage_class_list_english_type_opt "
                  "cv_qualifier_list_tid_opt "
                  "USER-DEFINED CONVERSION OPERATOR "
                  "of_scope_list_english_opt "
                  "RETURNING decl_english_ast" );
      DUMP_TYPE( "storage_class_list_english_type_opt", &$2 );
      DUMP_TID( "cv_qualifier_list_tid_opt", $3 );
      DUMP_SNAME( "of_scope_list_english_opt", $7 );
      DUMP_AST( "decl_english_ast", $9 );

      c_ast_t *const conv_ast = c_ast_new_gc( K_USER_DEF_CONVERSION, &@$ );
      c_ast_set_sname( conv_ast, &$7 );
      conv_ast->type = c_type_or( &$2, &C_TYPE_LIT_S( $3 ) );
      c_ast_set_parent( $9, conv_ast );

      DUMP_AST( "declare_english", conv_ast );
      DUMP_END();

      C_AST_CHECK_DECL( conv_ast );
      c_ast_gibberish( conv_ast, C_GIB_DECL, fout );
      if ( opt_semicolon )
        FPUTC( ';', fout );
      FPUTC( '\n', fout );
    }

  | Y_DECLARE error
    {
      if ( OPT_LANG_IS(CPP_ANY) )
        elaborate_error( "name or %s expected", L_OPERATOR );
      else
        elaborate_error( "name expected" );
    }
  ;

storage_class_list_english_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | storage_class_list_english_type_opt storage_class_english_type
    {
      DUMP_START( "storage_class_list_english_type_opt",
                  "storage_class_list_english_type_opt "
                  "storage_class_english_type" );
      DUMP_TYPE( "storage_class_list_english_type_opt", &$1 );
      DUMP_TYPE( "storage_class_english_type", &$2 );

      $$ = $1;
      C_TYPE_ADD( &$$, &$2, @2 );

      DUMP_TYPE( "storage_class_list_english_type_opt", &$$ );
      DUMP_END();
    }
  ;

storage_class_english_type
  : attribute_english_tid         { $$ = C_TYPE_LIT_A( $1 ); }
  | Y_AUTO_STORAGE                { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_APPLE___BLOCK               { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_CONSTEVAL                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_CONSTEXPR                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_CONSTINIT                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_DEFAULT                     { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_DELETE                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXPLICIT                    { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXPORT                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXTERN                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXTERN linkage_tid linkage_opt
    {
      $$ = C_TYPE_LIT_S( $2 );
    }
  | Y_FINAL                       { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_FRIEND                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_INLINE                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_MUTABLE                     { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_NOEXCEPT                    { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_OVERRIDE                    { $$ = C_TYPE_LIT_S( $1 ); }
//| Y_REGISTER                          // in type_modifier_list_english_type
  | Y_STATIC                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y__THREAD_LOCAL               { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_THREAD_LOCAL                { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_THROW                       { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_TYPEDEF                     { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_VIRTUAL                     { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_PURE virtual_tid_exp        { $$ = C_TYPE_LIT_S( TS_PURE_VIRTUAL | $2 ); }
  ;

attribute_english_tid
  : Y_CARRIES_DEPENDENCY
  | Y_DEPRECATED
  | Y_MAYBE_UNUSED
  | Y_NODISCARD
  | Y__NORETURN
  | Y_NORETURN
  | Y_NO_UNIQUE_ADDRESS
  ;

linkage_tid
  : Y_STR_LIT
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

      FREE( $1 );
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
      DUMP_TYPE( "udc_storage_class_list_english_type_opt", &$1 );
      DUMP_TYPE( "udc_storage_class_english_type", &$2 );

      $$ = $1;
      C_TYPE_ADD( &$$, &$2, @2 );

      DUMP_TYPE( "udc_storage_class_list_english_type_opt", &$$ );
      DUMP_END();
    }
  ;

udc_storage_class_english_type
    /*
     * We need a seperate storage class set for user-defined conversion
     * operators without "delete" to eliminiate a shift/reduce conflict; shift:
     *
     *      declare delete as ...
     *
     * where "delete" is the operator; and reduce:
     *
     *      declare delete[d] user-defined conversion operator ...
     *
     * where "delete" is storage-class-like.  The "delete" can safely be
     * removed since only special members can be deleted anyway.
     */
  : attribute_english_tid         { $$ = C_TYPE_LIT_A( $1 ); }
  | Y_CONSTEVAL                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_CONSTEXPR                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_CONSTINIT                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXPLICIT                    { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXPORT                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_FINAL                       { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_FRIEND                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_INLINE                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_NOEXCEPT                    { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_OVERRIDE                    { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_THROW                       { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_VIRTUAL                     { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_PURE virtual_tid_exp        { $$ = C_TYPE_LIT_S( TS_PURE_VIRTUAL | $2 ); }
  ;

alignas_or_width_decl_english_ast
  : decl_english_ast              { $$ = $1; }

  | decl_english_ast alignas_specifier_english
    {
      $$ = $1;
      $$->align = $2;
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
      $$->as.builtin.bit_width = (unsigned)$2;
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
    {
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

///////////////////////////////////////////////////////////////////////////////
//  define
///////////////////////////////////////////////////////////////////////////////

define_english
  : Y_DEFINE sname_english_exp as_exp storage_class_list_english_type_opt
    decl_english_ast
    {
      DUMP_START( "define_english",
                  "DEFINE sname_english AS "
                  "storage_class_list_english_type_opt decl_english_ast" );
      DUMP_SNAME( "sname", $2 );
      DUMP_TYPE( "storage_class_list_english_type_opt", &$4 );
      DUMP_AST( "decl_english_ast", $5 );

      if ( $5->kind_id == K_NAME ) {  // see the comment in "declare_english"
        assert( !c_ast_empty_name( $5 ) );
        print_error_unknown_name( &@5, &$5->sname );
        c_sname_free( &$2 );
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
      //  i.e., a defined type with a storage class.
      //
      bool ok = c_type_add( &$5->type, &T_TS_TYPEDEF, &@4 ) &&
                c_type_add( &$5->type, &$4, &@4 ) &&
                c_ast_check_declaration( $5 );

      if ( ok ) {
        // Once the semantic checks pass, remove the TS_TYPEDEF.
        PJL_IGNORE_RV( c_ast_take_type_any( $5, &T_TS_TYPEDEF ) );

        if ( c_sname_count( &$2 ) > 1 ) {
          c_type_t scope_type = *c_sname_local_type( &$2 );
          if ( scope_type.base_tid == TB_SCOPE ) {
            //
            // Replace the generic "scope" with "namespace".
            //
            scope_type.base_tid = TB_NAMESPACE;
            c_sname_set_local_type( &$2, &scope_type );
          }
        }
        c_ast_set_sname( $5, &$2 );
        DUMP_AST( "defined.ast", $5 );
        ok = add_type( L_DEFINE, $5, &@5 );
      }

      DUMP_END();

      if ( !ok ) {
        c_sname_free( &$2 );
        PARSE_ABORT();
      }
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  explain
///////////////////////////////////////////////////////////////////////////////

explain_c
    /*
     * C-style cast.
     */
  : explain '(' type_c_ast
    {
      ia_type_ast_push( $3 );
    }
    cast_c_astp_opt rparen_exp sname_c_opt
    {
      ia_type_ast_pop();

      DUMP_START( "explain_c",
                  "EXPLAIN '(' type_c_ast cast_c_astp_opt ')' sname_c_opt" );
      DUMP_AST( "type_c_ast", $3 );
      DUMP_AST( "cast_c_astp_opt", $5.ast );
      DUMP_SNAME( "sname_c_opt", $7 );

      c_ast_t *const cast_ast = c_ast_patch_placeholder( $3, $5.ast );

      DUMP_AST( "explain_c", cast_ast );
      DUMP_END();

      bool const ok = c_ast_check_cast( cast_ast );
      if ( ok ) {
        FPUTS( L_CAST, fout );
        if ( !c_sname_empty( &$7 ) ) {
          FPUTC( ' ', fout );
          c_sname_english( &$7, fout );
        }
        FPRINTF( fout, " %s ", L_INTO );
        c_ast_english( cast_ast, fout );
        FPUTC( '\n', fout );
      }

      c_sname_free( &$7 );
      if ( !ok )
        PARSE_ABORT();
    }

    /*
     * New C++-style cast.
     */
  | explain new_style_cast_c_literal lt_exp type_c_ast
    {
      ia_type_ast_push( $4 );
    }
    cast_c_astp_opt gt_exp lparen_exp sname_c_exp rparen_exp
    {
      ia_type_ast_pop();

      DUMP_START( "explain_c",
                  "EXPLAIN new_style_cast_c_literal "
                  "'<' type_c_ast cast_c_astp_opt '>' '(' sname ')'" );
      DUMP_STR( "new_style_cast_c_literal", $2 );
      DUMP_AST( "type_c_ast", $4 );
      DUMP_AST( "cast_c_astp_opt", $6.ast );
      DUMP_SNAME( "sname", $9 );

      c_ast_t const *const cast_ast = c_ast_patch_placeholder( $4, $6.ast );

      DUMP_AST( "explain_c", cast_ast );
      DUMP_END();

      bool ok = false;

      if ( unsupported( LANG_CPP_ANY ) ) {
        print_error( &@2, "%s_cast is not supported in C\n", $2 );
      }
      else {
        if ( (ok = c_ast_check_cast( cast_ast )) ) {
          FPRINTF( fout, "%s %s ", $2, L_CAST );
          c_sname_english( &$9, fout );
          FPRINTF( fout, " %s ", L_INTO );
          c_ast_english( cast_ast, fout );
          FPUTC( '\n', fout );
        }
      }

      c_sname_free( &$9 );
      if ( !ok )
        PARSE_ABORT();
    }

    /*
     * Common declaration, e.g.: T x.
     */
  | explain type_c_ast { ia_type_ast_push( $2 ); } decl_list_c_opt
    {
      ia_type_ast_pop();
    }

    /*
     * Common declaration with alignas, e.g.: alignas(8) T x.
     */
  | explain alignas_specifier_c { in_attr.align = $2; }
    typename_flag_opt { in_attr.typename = $4; }
    type_c_ast { ia_type_ast_push( $6 ); } decl_list_c
    {
      ia_type_ast_pop();
    }

    /*
     * Common C++ declaration with typename (without alignas), e.g.:
     *
     *      explain typename T::U x
     *
     * (We can't use typename_flag_opt because it would introduce more
     * shift/reduce conflicts.)
     */
  | explain Y_TYPENAME { in_attr.typename = true; }
    type_c_ast { ia_type_ast_push( $4 ); } decl_list_c
    {
      ia_type_ast_pop();
    }

    /*
     * K&R C implicit int function and C++ in-class constructor declaration.
     */
  | explain knr_func_or_constructor_decl_c_ast
    {
      DUMP_START( "explain_c", "EXPLAIN knr_func_or_constructor_decl_c_ast" );
      DUMP_AST( "knr_func_or_constructor_decl_c_ast", $2 );
      DUMP_END();

      C_AST_CHECK_DECL( $2 );
      c_ast_explain_declaration( $2, fout );
    }

    /*
     * C++ file-scope constructor definition, e.g.: S::S([params]).
     */
  | explain file_scope_constructor_decl_c_ast
    {
      DUMP_START( "explain_c", "EXPLAIN file_scope_constructor_decl_c_ast" );
      DUMP_AST( "file_scope_constructor_decl_c_ast", $2 );
      DUMP_END();

      C_AST_CHECK_DECL( $2 );
      c_ast_explain_declaration( $2, fout );
    }

    /*
     * C++ in-class destructor declaration, e.g.: ~S().
     */
  | explain destructor_decl_c_ast
    {
      DUMP_START( "explain_c", "EXPLAIN destructor_decl_c_ast" );
      DUMP_AST( "destructor_decl_c_ast", $2 );
      DUMP_END();

      C_AST_CHECK_DECL( $2 );
      c_ast_explain_declaration( $2, fout );
    }

    /*
     * C++ file scope destructor definition, e.g.: S::~S().
     */
  | explain file_scope_destructor_decl_c_ast
    {
      DUMP_START( "explain_c", "EXPLAIN file_scope_destructor_decl_c_ast" );
      DUMP_AST( "file_scope_destructor_decl_c_ast", $2 );
      DUMP_END();

      C_AST_CHECK_DECL( $2 );
      c_ast_explain_declaration( $2, fout );
    }

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
      DUMP_START( "explain_c", "EXPLAIN user_defined_conversion_decl_c_astp" );
      DUMP_AST( "user_defined_conversion_decl_c_astp", $2.ast );
      DUMP_END();

      C_AST_CHECK_DECL( $2.ast );
      c_ast_explain_declaration( $2.ast, fout );
    }

    /*
     * C++ using declaration.
     */
  | explain extern_linkage_c_tid_opt using_decl_c_ast
    {
      DUMP_START( "explain_c",
                  "EXPLAIN extern_linkage_c_tid_opt using_decl_c_ast" );
      DUMP_TID( "extern_linkage_c_tid_opt", $2 );
      DUMP_AST( "using_decl_c_ast", $3 );
      DUMP_END();

      C_TYPE_ADD_TID( &$3->type, $2, @2 );

      C_AST_CHECK_DECL( $3 );
      c_ast_explain_declaration( $3, fout );
    }

    /*
     * If we match an sname, it means it wasn't an sname for a type (otherwise
     * we would have matched the "Common declaration" case above).
     */
  | explain sname_c
    {
      print_error_unknown_name( &@2, &$2 );
      c_sname_free( &$2 );
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
      // tell the lexer to return cdecl keywords (e.g., "func") are returned as
      // ordinary names, otherwise gibberish like:
      //
      //      int func(void);
      //
      // would result in a parser error.
      //
      gibberish_to_english();
    }
  ;

new_style_cast_c_literal
  : Y_CONST_CAST                  { $$ = L_CONST;       }
  | Y_DYNAMIC_CAST                { $$ = L_DYNAMIC;     }
  | Y_REINTERPRET_CAST            { $$ = L_REINTERPRET; }
  | Y_STATIC_CAST                 { $$ = L_STATIC;      }
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
      DUMP_END();

      if ( type_ast->kind_id != K_ENUM_CLASS_STRUCT_UNION ) {
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
      c_ast_set_sname( type_ast, &temp_sname );

      C_AST_CHECK_DECL( type_ast );
      c_ast_explain_type( type_ast, fout );
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
    {
      c_ast_t *const type_ast = ia_type_ast_peek();

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
        in_attr.typename, &in_attr.align, type_ast, $1.ast, &@1
      );

      DUMP_AST( "decl_c", decl_ast );
      DUMP_END();

      if ( decl_ast == NULL )
        PARSE_ABORT();
      C_AST_CHECK_DECL( decl_ast );
      c_ast_explain_declaration( decl_ast, fout );

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
      c_sname_free( &type_ast->sname );
    }
  ;

extern_linkage_c_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | extern_linkage_c_tid
  ;

extern_linkage_c_tid
  : Y_EXTERN linkage_tid          { $$ = $2; }
  | Y_EXTERN linkage_tid '{'
    {
      print_error( &@3,
        "explaining scoped linkage declarations is not supported by %s\n",
        PACKAGE
      );
      PARSE_ABORT();
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  help
///////////////////////////////////////////////////////////////////////////////

help_command
  : Y_HELP help_what_literal_opt  { print_help( $2 ); }
  ;

help_what_literal_opt
  : /* empty */                   { $$ = L_DEFAULT;   }
  | Y_COMMANDS                    { $$ = L_COMMANDS;  }
  | Y_ENGLISH                     { $$ = L_ENGLISH;   }
  ;

///////////////////////////////////////////////////////////////////////////////
//  quit
///////////////////////////////////////////////////////////////////////////////

quit_command
  : Y_QUIT                        { quit(); }
  ;

///////////////////////////////////////////////////////////////////////////////
//  scoped declarations
///////////////////////////////////////////////////////////////////////////////

scope_declaration_c
  : class_struct_union_declaration_c
  | enum_declaration_c
  | namespace_declaration_c
  ;

class_struct_union_declaration_c
    /*
     * C++ scoped declaration, e.g.: class C { typedef int I; };
     */
  : class_struct_union_tid
    {
      // see the comment in "explain"
      gibberish_to_english();
    }
    any_sname_c
    {
      if ( OPT_LANG_IS(C_ANY) && !c_sname_empty( &in_attr.current_scope ) ) {
        print_error( &@1, "nested types are not supported in C\n" );
        PARSE_ABORT();
      }

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
          PARSE_ABORT();
        }
      }
      c_sname_set_local_type( &$3, &C_TYPE_LIT_B( $1 ) );

      DUMP_START( "class_struct_union_declaration_c",
                  "class_struct_union_tid sname '{' "
                  "in_scope_declaration_c_opt "
                  "'}' ';'" );
      DUMP_TID( "class_struct_union_tid", $1 );
      DUMP_SNAME( "any_sname_c", $3 );

      c_sname_append_sname( &in_attr.current_scope, &$3 );

      c_ast_t *const csu_ast = c_ast_new_gc( K_ENUM_CLASS_STRUCT_UNION, &@3 );
      csu_ast->sname = c_sname_dup( &in_attr.current_scope );
      csu_ast->type.base_tid = c_type_id_check( $1, C_TPID_BASE );
      c_sname_append_name(
        &csu_ast->as.ecsu.ecsu_sname,
        check_strdup( c_sname_local_name( &in_attr.current_scope ) )
      );

      DUMP_AST( "class_struct_union_declaration_c", csu_ast );
      DUMP_END();

      if ( !add_type( c_type_id_name_c( $1 ), csu_ast, &@1 ) )
        PARSE_ABORT();
    }
    brace_in_scope_declaration_c_opt
  ;

enum_declaration_c
    /*
     * C/C++ enum declaration, e.g.: enum E;
     */
  : enum_tid
    {
      // see the comment in "explain"
      gibberish_to_english();
    }
    any_sname_c_exp enum_fixed_type_c_ast_opt
    {
      c_sname_set_local_type( &$3, &C_TYPE_LIT_B( $1 ) );

      DUMP_START( "enum_declaration_c", "enum_tid sname ;" );
      DUMP_TID( "enum_tid", $1 );
      DUMP_SNAME( "any_sname_c", $3 );
      DUMP_AST( "enum_fixed_type_c_ast_opt", $4 );

      c_sname_append_sname( &in_attr.current_scope, &$3 );

      c_ast_t *const enum_ast = c_ast_new_gc( K_ENUM_CLASS_STRUCT_UNION, &@3 );
      enum_ast->sname = c_sname_dup( &in_attr.current_scope );
      enum_ast->type.base_tid = c_type_id_check( $1, C_TPID_BASE );
      enum_ast->as.ecsu.of_ast = $4;
      c_sname_append_name(
        &enum_ast->as.ecsu.ecsu_sname,
        check_strdup( c_sname_local_name( &in_attr.current_scope ) )
      );

      DUMP_AST( "enum_declaration_c", enum_ast );
      DUMP_END();

      if ( !add_type( c_type_id_name_c( $1 ), enum_ast, &@1 ) )
        PARSE_ABORT();
    }
  ;

namespace_declaration_c
    /*
     * C++ namespace declaration, e.g.: namespace NS { typedef int I; }
     */
  : namespace_type
    {
      // see the comment in "explain"
      gibberish_to_english();
    }
    sname_c
    { //
      // Make every scope's type be $1 for nested namespaces.
      //
      FOREACH_SCOPE( scope, $3.head, NULL ) {
        c_type_t scope_type = c_scope_data( scope )->type;
        scope_type.base_tid =
          (scope_type.base_tid & c_type_id_compl( TB_SCOPE )) | $1.base_tid;
        c_scope_data( scope )->type = scope_type;
      } // for

      DUMP_START( "namespace_declaration_c",
                  "namespace_type sname '{' "
                  "in_scope_declaration_c_opt "
                  "'}' [';']" );
      DUMP_TYPE( "namespace_type", &$1 );
      DUMP_SNAME( "any_sname_c", $3 );
      DUMP_END();

      bool ok = false;

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
          "nested %s declarations are not supported%s\n",
          L_NAMESPACE, c_lang_until( LANG_CPP_17 )
        );
      }
      else {
        //
        // Ensure that "namespace" isn't nested within a class/struct/union.
        //
        c_type_t const *const outer_type =
          c_sname_local_type( &in_attr.current_scope );
        if ( !(ok = !c_type_is_tid_any( outer_type, TB_ANY_CLASS ) ) ) {
          print_error( &@1,
            "\"%s\" may only be nested within a %s\n",
            L_NAMESPACE, L_NAMESPACE
          );
        }
      }

      if ( !ok ) {
        c_sname_free( &$3 );
        PARSE_ABORT();
      }

      c_sname_append_sname( &in_attr.current_scope, &$3 );
    }
    brace_in_scope_declaration_c
  ;

brace_in_scope_declaration_c_opt
  : /* empty */
  | brace_in_scope_declaration_c
  ;

brace_in_scope_declaration_c
  : '{' '}'
  | '{' in_scope_declaration_c semi_opt rbrace_exp
  ;

in_scope_declaration_c
  : scope_declaration_c
  | typedef_declaration_c semi_exp
  | using_declaration_c semi_exp
  ;

///////////////////////////////////////////////////////////////////////////////
//  set
///////////////////////////////////////////////////////////////////////////////

set_command
  : Y_SET                         { option_set( NULL, NULL, NULL, NULL ); }
  | Y_SET set_option_list
  ;

set_option_list
  : set_option_list set_option
  | set_option
  ;

set_option
  : Y_SET_OPTION set_option_value_opt
    {
      option_set( $1, &@1, $2, &@2 );
      FREE( $1 );
      FREE( $2 );
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

///////////////////////////////////////////////////////////////////////////////
//  show
///////////////////////////////////////////////////////////////////////////////

show_command
  : Y_SHOW any_typedef show_format_opt
    {
      DUMP_START( "show_command", "SHOW any_typedef show_format_opt" );
      DUMP_AST( "any_typedef.ast", $2->ast );
      DUMP_INT( "show_format_opt", $3 );
      DUMP_END();

      if ( $3 == C_GIB_NONE )
        c_ast_explain_type( $2->ast, fout );
      else
        c_typedef_gibberish( $2, $3, fout );
    }

  | Y_SHOW any_typedef Y_AS show_format_exp
    {
      DUMP_START( "show_command", "SHOW any_typedef AS show_format_exp" );
      DUMP_AST( "any_typedef.ast", $2->ast );
      DUMP_INT( "show_format_exp", $4 );
      DUMP_END();

      c_typedef_gibberish( $2, $4, fout );
    }

  | Y_SHOW show_which_types_mask_opt show_format_opt
    {
      show_type_info_t sti = { $2, $3 };
      c_typedef_visit( &show_type_visitor, &sti );
    }

  | Y_SHOW show_which_types_mask_opt Y_AS show_format_exp
    {
      show_type_info_t sti = { $2, $4 };
      c_typedef_visit( &show_type_visitor, &sti );
    }

  | Y_SHOW Y_NAME
    {
      if ( opt_lang < LANG_CPP_11 ) {
        print_error( &@2,
          "\"%s\": not defined as type via %s or %s",
          $2, L_DEFINE, L_TYPEDEF
        );
      } else {
        print_error( &@2,
          "\"%s\": not defined as type via %s, %s, or %s",
          $2, L_DEFINE, L_TYPEDEF, L_USING
        );
      }
      print_suggestions( DYM_C_TYPES, $2 );
      EPUTC( '\n' );
      FREE( $2 );
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
  | Y_USING                       { $$ = C_GIB_USING; }
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

///////////////////////////////////////////////////////////////////////////////
//  typedef
///////////////////////////////////////////////////////////////////////////////

typedef_declaration_c
  : Y_TYPEDEF typename_flag_opt
    {
      in_attr.typename = $2;
      // see the comment in "explain"
      gibberish_to_english();
    }
    type_c_ast
    {
      if ( $2 && !c_ast_is_typename_ok( $4 ) )
        PARSE_ABORT();

      // see the comment in define_english about TS_TYPEDEF
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
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "decl_c_astp", $1.ast );

      c_ast_t *typedef_ast;
      c_sname_t temp_sname;

      if ( $1.ast->kind_id == K_TYPEDEF ) {
        //
        // This is for either of the following cases:
        //
        //      typedef int int32_t;
        //      typedef int32_t int_least32_t;
        //
        // that is: any type name followed by an existing typedef name.
        //
        typedef_ast = type_ast;
        if ( c_ast_empty_name( typedef_ast ) )
          typedef_ast->sname = c_ast_dup_name( $1.ast->as.tdef.for_ast );
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
        c_ast_set_sname( typedef_ast, &temp_sname );
      }

      C_AST_CHECK_DECL( typedef_ast );
      // see the comment in define_english about TS_TYPEDEF
      PJL_IGNORE_RV( c_ast_take_type_any( typedef_ast, &T_TS_TYPEDEF ) );

      if ( c_ast_count_name( typedef_ast ) > 1 ) {
        print_error( &@1,
          "%s names can not be scoped; use: %s %s { %s ... }\n",
          L_TYPEDEF, L_NAMESPACE, c_ast_scope_name( typedef_ast ), L_TYPEDEF
        );
        PARSE_ABORT();
      }

      temp_sname = c_sname_dup( &in_attr.current_scope );
      c_ast_set_local_type(
        typedef_ast, c_sname_local_type( &in_attr.current_scope )
      );
      c_ast_prepend_sname( typedef_ast, &temp_sname );

      DUMP_AST( "typedef_decl_c", typedef_ast );
      DUMP_END();

      if ( !add_type( L_TYPEDEF, typedef_ast, &@1 ) )
        PARSE_ABORT();
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  using
///////////////////////////////////////////////////////////////////////////////

using_declaration_c
  : using_decl_c_ast
    {
      // see the comment in define_english about TS_TYPEDEF
      PJL_IGNORE_RV( c_ast_take_type_any( $1, &T_TS_TYPEDEF ) );

      if ( !add_type( L_USING, $1, &@1 ) )
        PARSE_ABORT();
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  declaration english productions
///////////////////////////////////////////////////////////////////////////////

decl_english_ast
  : array_decl_english_ast
  | constructor_decl_english_ast
  | destructor_decl_english_ast
  | qualified_decl_english_ast
  | user_defined_literal_decl_english_ast
  | var_decl_english_ast
  ;

array_decl_english_ast
  : Y_ARRAY static_tid_opt type_qualifier_list_english_tid_opt
    array_size_int_opt of_exp decl_english_ast
    {
      DUMP_START( "array_decl_english_ast",
                  "ARRAY static_tid_opt type_qualifier_list_english_tid_opt "
                  "array_size_num_opt OF decl_english_ast" );
      DUMP_TID( "static_tid_opt", $2 );
      DUMP_TID( "type_qualifier_list_english_tid_opt", $3 );
      DUMP_INT( "array_size_int_opt", $4 );
      DUMP_AST( "decl_english_ast", $6 );

      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = $4;
      $$->as.array.store_tid = c_type_id_check( $2 | $3, C_TPID_STORE );
      c_ast_set_parent( $6, $$ );

      DUMP_AST( "array_decl_english_ast", $$ );
      DUMP_END();
    }

  | Y_VARIABLE length_opt array_exp type_qualifier_list_english_tid_opt
    of_exp decl_english_ast
    {
      DUMP_START( "array_decl_english_ast",
                  "VARIABLE LENGTH ARRAY type_qualifier_list_english_tid_opt "
                  "OF decl_english_ast" );
      DUMP_TID( "type_qualifier_list_english_tid_opt", $4 );
      DUMP_AST( "decl_english_ast", $6 );

      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = C_ARRAY_SIZE_VARIABLE;
      $$->as.array.store_tid = c_type_id_check( $4, C_TPID_STORE );
      c_ast_set_parent( $6, $$ );

      DUMP_AST( "array_decl_english_ast", $$ );
      DUMP_END();
    }
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

block_decl_english_ast                  // Apple extension
  : // in_attr: qualifier
    Y_APPLE_BLOCK paren_decl_list_english_opt returning_english_ast_opt
    {
      DUMP_START( "block_decl_english_ast",
                  "BLOCK paren_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_TID( "(qualifier)", ia_qual_peek_tid() );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $2 );
      DUMP_AST( "returning_english_ast_opt", $3 );

      $$ = c_ast_new_gc( K_APPLE_BLOCK, &@$ );
      $$->type.store_tid = ia_qual_peek_tid();
      $$->as.block.param_ast_list = $2;
      c_ast_set_parent( $3, $$ );

      DUMP_AST( "block_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

constructor_decl_english_ast
  : Y_CONSTRUCTOR paren_decl_list_english_opt
    {
      DUMP_START( "constructor_decl_english_ast",
                  "CONSTRUCTOR paren_decl_list_english_opt" );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $2 );

      $$ = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      $$->as.func.param_ast_list = $2;

      DUMP_AST( "constructor_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

destructor_decl_english_ast
  : Y_DESTRUCTOR
    {
      DUMP_START( "destructor_decl_english_ast", "DESTRUCTOR" );

      $$ = c_ast_new_gc( K_DESTRUCTOR, &@$ );

      DUMP_AST( "destructor_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

func_decl_english_ast
  : // in_attr: qualifier
    ref_qualifier_english_tid_opt member_or_non_member_mask_opt
    Y_FUNCTION paren_decl_list_english_opt returning_english_ast_opt
    {
      DUMP_START( "func_decl_english_ast",
                  "ref_qualifier_english_tid_opt "
                  "member_or_non_member_mask_opt "
                  "FUNCTION paren_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_TID( "(qualifier)", ia_qual_peek_tid() );
      DUMP_TID( "ref_qualifier_english_tid_opt", $1 );
      DUMP_INT( "member_or_non_member_mask_opt", $2 );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $4 );
      DUMP_AST( "returning_english_ast_opt", $5 );

      $$ = c_ast_new_gc( K_FUNCTION, &@$ );
      $$->type.store_tid =
        c_type_id_check( ia_qual_peek_tid() | $1, C_TPID_STORE );
      $$->as.func.param_ast_list = $4;
      $$->as.func.flags = $2;
      c_ast_set_parent( $5, $$ );

      DUMP_AST( "func_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

oper_decl_english_ast
  : type_qualifier_list_english_tid_opt { ia_qual_push_tid( $1, &@1 ); }
    ref_qualifier_english_tid_opt member_or_non_member_mask_opt
    operator_exp paren_decl_list_english_opt returning_english_ast_opt
    {
      ia_qual_pop();
      DUMP_START( "oper_decl_english_ast",
                  "member_or_non_member_mask_opt "
                  "OPERATOR paren_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_TID( "ref_qualifier_english_tid_opt", $3 );
      DUMP_INT( "member_or_non_member_mask_opt", $4 );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $6 );
      DUMP_AST( "returning_english_ast_opt", $7 );

      $$ = c_ast_new_gc( K_OPERATOR, &@$ );
      $$->type.store_tid = c_type_id_check( $1 | $3, C_TPID_STORE );
      $$->as.oper.param_ast_list = $6;
      $$->as.oper.flags = $4;
      c_ast_set_parent( $7, $$ );

      DUMP_AST( "oper_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

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
  : decl_english_ast
    {
      DUMP_START( "decl_list_english", "decl_english_ast" );
      DUMP_AST( "decl_english_ast", $1 );

      if ( $1->kind_id == K_FUNCTION ) {
        //
        // From the C standard, section 6.3.2.1(4):
        //
        //    [A] function designator with type "function returning type" is
        //    converted to an expression that has type "pointer to function
        //    returning type."
        //
        $1 = c_ast_pointer( $1 );
      }

      slist_init( &$$ );
      slist_push_tail( &$$, $1 );

      DUMP_AST_LIST( "decl_list_english", $$ );
      DUMP_END();
    }

  | decl_list_english comma_exp decl_english_ast
    {
      DUMP_START( "decl_list_english",
                  "decl_list_english',' decl_english_ast" );
      DUMP_AST_LIST( "decl_list_english", $1 );
      DUMP_AST( "decl_english_ast", $3 );

      $$ = $1;
      slist_push_tail( &$$, $3 );

      DUMP_AST_LIST( "decl_list_english", $$ );
      DUMP_END();
    }
  ;

ref_qualifier_english_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_REFERENCE                   { $$ = TS_REFERENCE; }
  | Y_RVALUE reference_exp        { $$ = TS_RVALUE_REFERENCE; }
  ;

returning_english_ast_opt
  : /* empty */
    {
      DUMP_START( "returning_english_ast_opt", "<empty>" );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      // see the comment in "type_c_ast"
      $$->type.base_tid = opt_lang < LANG_C_99 ? TB_INT : TB_VOID;

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

qualified_decl_english_ast
  : type_qualifier_list_english_tid_opt { ia_qual_push_tid( $1, &@1 ); }
    qualifiable_decl_english_ast
    {
      ia_qual_pop();
      DUMP_START( "qualified_decl_english_ast",
                  "type_qualifier_list_english_tid_opt "
                  "qualifiable_decl_english_ast" );
      DUMP_TID( "type_qualifier_list_english_tid_opt", $1 );
      DUMP_AST( "qualifiable_decl_english_ast", $3 );

      $$ = $3;

      DUMP_AST( "qualified_decl_english_ast", $$ );
      DUMP_END();
    }
  ;

type_qualifier_list_english_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | type_qualifier_list_english_tid
  ;

type_qualifier_list_english_tid
  : type_qualifier_list_english_tid type_qualifier_c_tid
    {
      DUMP_START( "type_qualifier_list_english_tid",
                  "type_qualifier_list_english_tid type_qualifier_c_tid" );
      DUMP_TID( "type_qualifier_list_english_tid", $1 );
      DUMP_TID( "type_qualifier_c_tid", $2 );

      $$ = $1;
      C_TYPE_ID_ADD( &$$, $2, @2 );

      DUMP_TID( "type_qualifier_list_english_tid", $$ );
      DUMP_END();
    }

  | type_qualifier_c_tid
  ;

qualifiable_decl_english_ast
  : block_decl_english_ast
  | func_decl_english_ast
  | pointer_decl_english_ast
  | reference_decl_english_ast
  | type_english_ast
  ;

pointer_decl_english_ast
    /*
     * Ordinary pointer declaration.
     */
  : // in_attr: qualifier
    Y_POINTER to_exp decl_english_ast
    {
      DUMP_START( "pointer_decl_english_ast", "POINTER TO decl_english_ast" );
      DUMP_TID( "(qualifier)", ia_qual_peek_tid() );
      DUMP_AST( "decl_english_ast", $3 );

      if ( $3->kind_id == K_NAME ) {  // see the comment in "declare_english"
        assert( !c_ast_empty_name( $3 ) );
        print_error_unknown_name( &@3, &$3->sname );
        PARSE_ABORT();
      }

      $$ = c_ast_new_gc( K_POINTER, &@$ );
      $$->type.store_tid = ia_qual_peek_tid();
      c_ast_set_parent( $3, $$ );

      DUMP_AST( "pointer_decl_english_ast", $$ );
      DUMP_END();
    }

    /*
     * C++ pointer-to-member declaration.
     */
  | // in_attr: qualifier
    Y_POINTER to_exp Y_MEMBER of_exp class_struct_tid_exp sname_english_exp
    decl_english_ast
    {
      DUMP_START( "pointer_to_member_decl_english",
                  "POINTER TO MEMBER OF "
                  "class_struct_tid sname_english decl_english_ast" );
      DUMP_TID( "(qualifier)", ia_qual_peek_tid() );
      DUMP_TID( "class_struct_tid", $5 );
      DUMP_SNAME( "sname_english_exp", $6 );
      DUMP_AST( "decl_english_ast", $7 );

      $$ = c_ast_new_gc( K_POINTER_TO_MEMBER, &@$ );
      $$->type.store_tid = ia_qual_peek_tid();
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

reference_decl_english_ast
  : // in_attr: qualifier
    reference_english_ast to_exp decl_english_ast
    {
      DUMP_START( "reference_decl_english_ast",
                  "reference_english_ast TO decl_english_ast" );
      DUMP_TID( "(qualifier)", ia_qual_peek_tid() );
      DUMP_AST( "reference_english_ast", $1 );
      DUMP_AST( "decl_english_ast", $3 );

      $$ = $1;
      c_ast_set_parent( $3, $$ );
      C_TYPE_ADD_TID( &$$->type, ia_qual_peek_tid(), ia_qual_peek_loc() );

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
          "%s %s is not supported%s\n",
          H_USER_DEFINED, L_LITERAL, c_lang_until( LANG_CPP_11 )
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

var_decl_english_ast
    /*
     * Ordinary variable declaration.
     */
  : sname_c Y_AS decl_english_ast
    {
      DUMP_START( "var_decl_english_ast", "NAME AS decl_english_ast" );
      DUMP_SNAME( "sname", $1 );
      DUMP_AST( "decl_english_ast", $3 );

      if ( $3->kind_id == K_NAME ) {  // see the comment in "declare_english"
        assert( !c_ast_empty_name( $3 ) );
        print_error_unknown_name( &@3, &$3->sname );
        PARSE_ABORT();
      }

      $$ = $3;
      c_ast_set_sname( $$, &$1 );

      DUMP_AST( "var_decl_english_ast", $$ );
      DUMP_END();
    }

    /*
     * K&R C type-less variable declaration.
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
//  type english productions
///////////////////////////////////////////////////////////////////////////////

type_english_ast
  : // in_attr: qualifier
    type_modifier_list_english_type_opt unmodified_type_english_ast
    {
      DUMP_START( "type_english_ast",
                  "type_modifier_list_english_type_opt "
                  "unmodified_type_english_ast" );
      DUMP_TYPE( "type_modifier_list_english_type_opt", &$1 );
      DUMP_AST( "unmodified_type_english_ast", $2 );
      DUMP_TID( "(qualifier)", ia_qual_peek_tid() );

      $$ = $2;
      C_TYPE_ADD_TID( &$$->type, ia_qual_peek_tid(), ia_qual_peek_loc() );
      C_TYPE_ADD( &$$->type, &$1, @1 );

      DUMP_AST( "type_english_ast", $$ );
      DUMP_END();
    }

  | // in_attr: qualifier
    type_modifier_list_english_type     // allows implicit int in K&R C
    {
      DUMP_START( "type_english_ast", "type_modifier_list_english_type" );
      DUMP_TYPE( "type_modifier_list_english_type", &$1 );
      DUMP_TID( "(qualifier)", ia_qual_peek_tid() );

      // see the comment in "type_c_ast"
      c_type_t type = C_TYPE_LIT_B( opt_lang < LANG_C_99 ? TB_INT : TB_NONE );

      C_TYPE_ADD_TID( &type, ia_qual_peek_tid(), ia_qual_peek_loc() );
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
      DUMP_TYPE( "type_modifier_list_english_type", &$1 );
      DUMP_TYPE( "type_modifier_english_type", &$2 );

      $$ = $1;
      C_TYPE_ADD( &$$, &$2, @2 );

      DUMP_TYPE( "type_modifier_list_english_type", &$$ );
      DUMP_END();
    }

  | type_modifier_english_type
  ;

type_modifier_english_type
  : attribute_english_tid         { $$ = C_TYPE_LIT_A( $1 ); }
  | type_modifier_base_type
  ;

unmodified_type_english_ast
  : builtin_type_ast
  | enum_class_struct_union_english_ast
  | typedef_type_c_ast
  ;

enum_class_struct_union_english_ast
  : enum_class_struct_union_c_tid any_sname_c_exp
    of_type_enum_fixed_type_english_ast_opt
    {
      DUMP_START( "enum_class_struct_union_english_ast",
                  "enum_class_struct_union_c_tid sname" );
      DUMP_TID( "enum_class_struct_union_c_tid", $1 );
      DUMP_SNAME( "sname", $2 );
      DUMP_AST( "enum_fixed_type_english_ast", $3 );

      $$ = c_ast_new_gc( K_ENUM_CLASS_STRUCT_UNION, &@$ );
      $$->type.base_tid = c_type_id_check( $1, C_TPID_BASE );
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
  : enum_fixed_type_modifier_list_english_tid_opt
    enum_unmodified_fixed_type_english_ast
    {
      DUMP_START( "enum_fixed_type_english_ast",
                  "enum_fixed_type_modifier_list_tid" );
      DUMP_TID( "enum_fixed_type_modifier_list_tid", $1 );
      DUMP_AST( "enum_unmodified_fixed_type_english_ast", $2 );

      $$ = $2;
      C_TYPE_ADD_TID( &$$->type, $1, @1 );

      DUMP_AST( "enum_fixed_type_english_ast", $$ );
      DUMP_END();
    }

  | enum_fixed_type_modifier_list_english_tid
    {
      DUMP_START( "enum_fixed_type_english_ast",
                  "enum_fixed_type_modifier_list_english_tid" );
      DUMP_TID( "enum_fixed_type_modifier_list_english_tid", $1 );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.base_tid = c_type_id_check( $1, C_TPID_BASE );

      DUMP_AST( "enum_fixed_type_english_ast", $$ );
      DUMP_END();
    }
  ;

enum_fixed_type_modifier_list_english_tid_opt
  : /* empty */                   { $$ = TB_NONE; }
  | enum_fixed_type_modifier_list_english_tid
  ;

enum_fixed_type_modifier_list_english_tid
  : enum_fixed_type_modifier_list_english_tid enum_fixed_type_modifier_tid
    {
      DUMP_START( "enum_fixed_type_modifier_list_english_tid",
                  "enum_fixed_type_modifier_list_english_tid "
                  "enum_fixed_type_modifier_tid" );
      DUMP_TID( "enum_fixed_type_modifier_list_english_tid", $1 );
      DUMP_TID( "enum_fixed_type_modifier_tid", $2 );

      $$ = $1;
      C_TYPE_ID_ADD( &$$, $2, @2 );

      DUMP_TID( "enum_fixed_type_modifier_list_english_tid", $$ );
      DUMP_END();
    }

  | enum_fixed_type_modifier_tid
  ;

enum_unmodified_fixed_type_english_ast
  : builtin_type_ast
  | sname_english_ast
  ;

///////////////////////////////////////////////////////////////////////////////
//  declaration gibberish productions
///////////////////////////////////////////////////////////////////////////////

decl_c_astp
  : decl2_c_astp
  | pointer_decl_c_astp
  | pointer_to_member_decl_c_astp
  | reference_decl_c_astp
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

array_decl_c_astp
  : // in_attr: type_c_ast
    decl2_c_astp array_size_c_int gnu_attribute_specifier_list_c_opt
    {
      DUMP_START( "array_decl_c_astp", "decl2_c_astp array_size_c_int" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_AST( "decl2_c_astp", $1.ast );
      DUMP_AST( "target_ast", $1.target_ast );
      DUMP_INT( "array_size_c_int", $2 );

      c_ast_t *const array_ast = c_ast_new_gc( K_ARRAY, &@$ );
      array_ast->as.array.size = $2;
      c_ast_set_parent( c_ast_new_gc( K_PLACEHOLDER, &@1 ), array_ast );

      if ( $1.target_ast != NULL ) {    // array-of or function/block-ret type
        $$.ast = $1.ast;
        $$.target_ast = c_ast_add_array( $1.target_ast, array_ast );
      } else {
        $$.ast = c_ast_add_array( $1.ast, array_ast );
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

block_decl_c_astp                       // Apple extension
  : // in_attr: type_c_ast
    '(' Y_CIRC
    { //
      // A block AST has to be the type inherited attribute for decl_c_astp so
      // we have to create it here.
      //
      ia_type_ast_push( c_ast_new_gc( K_APPLE_BLOCK, &@$ ) );
    }
    type_qualifier_list_c_tid_opt decl_c_astp rparen_exp
    lparen_exp param_list_c_ast_opt ')' gnu_attribute_specifier_list_c_opt
    {
      c_ast_t *const block_ast = ia_type_ast_pop();

      DUMP_START( "block_decl_c_astp",
                  "'(' '^' type_qualifier_list_c_tid_opt decl_c_astp ')' "
                  "'(' param_list_c_ast_opt ')'" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_TID( "type_qualifier_list_c_tid_opt", $4 );
      DUMP_AST( "decl_c_astp", $5.ast );
      DUMP_AST_LIST( "param_list_c_ast_opt", $8 );

      C_TYPE_ADD_TID( &block_ast->type, $4, @4 );
      block_ast->as.block.param_ast_list = $8;
      $$.ast = c_ast_add_func( $5.ast, ia_type_ast_peek(), block_ast );
      $$.target_ast = block_ast->as.block.ret_ast;

      DUMP_AST( "block_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

destructor_decl_c_ast
    /*
     * C++ in-class destructor declaration, e.g.: ~S().
     */
  : virtual_tid_opt Y_TILDE any_name_exp
    lparen_exp rparen_func_qualifier_list_c_tid_opt noexcept_c_tid_opt
    gnu_attribute_specifier_list_c_opt func_equals_c_tid_opt
    {
      DUMP_START( "destructor_decl_c_ast",
                  "virtual_opt '~' NAME '(' ')' func_qualifier_list_c_tid_opt "
                  "noexcept_c_tid_opt gnu_attribute_specifier_list_c_opt "
                  "func_equals_c_tid_opt" );
      DUMP_TID( "virtual_tid_opt", $1 );
      DUMP_STR( "any_name_exp", $3 );
      DUMP_TID( "func_qualifier_list_c_tid_opt", $5 );
      DUMP_TID( "noexcept_c_tid_opt", $6 );
      DUMP_TID( "func_equals_c_tid_opt", $8 );

      $$ = c_ast_new_gc( K_DESTRUCTOR, &@$ );
      c_ast_append_name( $$, $3 );
      $$->type.store_tid = c_type_id_check( $1 | $5 | $6 | $8, C_TPID_STORE );

      DUMP_AST( "destructor_decl_c_ast", $$ );
      DUMP_END();
    }
  ;

file_scope_constructor_decl_c_ast
  : inline_tid_opt Y_CONSTRUCTOR_SNAME
    lparen_exp param_list_c_ast_opt rparen_func_qualifier_list_c_tid_opt
    noexcept_c_tid_opt gnu_attribute_specifier_list_c_opt
    {
      DUMP_START( "file_scope_constructor_decl_c_ast",
                  "inline_opt CONSTRUCTOR_SNAME '(' param_list_c_ast_opt ')' "
                  "func_qualifier_list_c_tid_opt noexcept_c_tid_opt" );
      DUMP_TID( "inline_tid_opt", $1 );
      DUMP_SNAME( "CONSTRUCTOR_SNAME", $2 );
      DUMP_AST_LIST( "param_list_c_ast_opt", $4 );
      DUMP_TID( "func_qualifier_list_c_tid_opt", $5 );
      DUMP_TID( "noexcept_c_tid_opt", $6 );

      c_sname_set_scope_type( &$2, &C_TYPE_LIT_B( TB_CLASS ) );

      $$ = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      $$->sname = $2;
      $$->type.store_tid = c_type_id_check( $1 | $5 | $6, C_TPID_STORE );
      $$->as.constructor.param_ast_list = $4;

      DUMP_AST( "file_scope_constructor_decl_c_ast", $$ );
      DUMP_END();
    }
  ;

file_scope_destructor_decl_c_ast
  : inline_tid_opt Y_DESTRUCTOR_SNAME
    lparen_exp rparen_func_qualifier_list_c_tid_opt noexcept_c_tid_opt
    gnu_attribute_specifier_list_c_opt
    {
      DUMP_START( "file_scope_destructor_decl_c_ast",
                  "inline_opt DESTRUCTOR_SNAME '(' ')' "
                  "func_qualifier_list_c_tid_opt noexcept_c_tid_opt" );
      DUMP_TID( "inline_tid_opt", $1 );
      DUMP_SNAME( "DESTRUCTOR_SNAME", $2 );
      DUMP_TID( "func_qualifier_list_c_tid_opt", $4 );
      DUMP_TID( "noexcept_c_tid_opt", $5 );

      c_sname_set_scope_type( &$2, &C_TYPE_LIT_B( TB_CLASS ) );

      $$ = c_ast_new_gc( K_DESTRUCTOR, &@$ );
      $$->sname = $2;
      $$->type.store_tid = c_type_id_check( $1 | $4 | $5, C_TPID_STORE );

      DUMP_AST( "file_scope_destructor_decl_c_ast", $$ );
      DUMP_END();
    }
  ;

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
    rparen_func_qualifier_list_c_tid_opt func_ref_qualifier_c_tid_opt
    noexcept_c_tid_opt trailing_return_type_c_ast_opt func_equals_c_tid_opt
    {
      c_ast_t    *const decl2_ast = $1.ast;
      c_type_id_t const func_qualifier_tid = $4;
      c_type_id_t const func_ref_qualifier_tid = $5;
      c_type_id_t const noexcept_tid = $6;
      c_type_id_t const func_equals_tid = $8;
      c_ast_t    *const trailing_ret_ast = $7;
      c_ast_t    *const type_ast = ia_type_ast_peek();

      DUMP_START( "func_decl_c_astp",
                  "decl2_c_astp '(' param_list_c_ast_opt ')' "
                  "func_qualifier_list_c_tid_opt "
                  "func_ref_qualifier_c_tid_opt noexcept_c_tid_opt "
                  "trailing_return_type_c_ast_opt "
                  "func_equals_c_tid_opt" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "decl2_c_astp", decl2_ast );
      DUMP_AST_LIST( "param_list_c_ast_opt", $3 );
      DUMP_TID( "func_qualifier_list_c_tid_opt", func_qualifier_tid );
      DUMP_TID( "func_ref_qualifier_c_tid_opt", func_ref_qualifier_tid );
      DUMP_TID( "noexcept_c_tid_opt", noexcept_tid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );
      DUMP_TID( "func_equals_c_tid_opt", func_equals_tid );
      DUMP_AST( "target_ast", $1.target_ast );

      c_type_id_t const func_store_tid =
        func_qualifier_tid | func_ref_qualifier_tid | noexcept_tid |
        func_equals_tid;

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
        type_ast->type.base_tid == TB_NONE &&

        // + The existing type does _not_ have any non-constructor storage
        //   classes.
        !c_type_is_tid_any( &type_ast->type, TS_NOT_CONSTRUCTOR ) &&

        ( // + The existing type has any constructor-only storage-class-like
          //   types (e.g., explicit).
          c_type_is_tid_any( &type_ast->type, TS_CONSTRUCTOR_ONLY ) ||

          // + Or the existing type only has storage-class-like types that may
          //   be applied to constructors.
          only_bits_set(
            c_type_id_no_tpid( type_ast->type.store_tid ),
            c_type_id_no_tpid( TS_CONSTRUCTOR_DECL )
          )
        ) &&

        // + The new type does _not_ have any non-constructor storage classes.
        (func_store_tid & TS_NOT_CONSTRUCTOR) == TS_NONE;

      c_ast_t *const func_ast =
        c_ast_new_gc( assume_constructor ? K_CONSTRUCTOR : K_FUNCTION, &@$ );
      func_ast->type.store_tid =
        c_type_id_check( func_store_tid, C_TPID_STORE );
      func_ast->as.func.param_ast_list = $3;

      if ( assume_constructor ) {
        assert( trailing_ret_ast == NULL );
        c_type_or_eq( &func_ast->type, &type_ast->type );
        $$.ast = func_ast;
      }
      else if ( $1.target_ast != NULL ) {
        $$.ast = decl2_ast;
        PJL_IGNORE_RV( c_ast_add_func( $1.target_ast, type_ast, func_ast ) );
      }
      else {
        $$.ast = c_ast_add_func(
          decl2_ast,
          trailing_ret_ast != NULL ? trailing_ret_ast : type_ast,
          func_ast
        );
      }

      $$.target_ast = func_ast->as.func.ret_ast;

      DUMP_AST( "func_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

knr_func_or_constructor_decl_c_ast
    /*
     * K&R C implicit int function and C++ in-class constructor declaration.
     *
     * This grammar rule handles both since they're so similar (and so would
     * cause grammar conflicts if they were separate rules in an LALR(1)
     * parser).
     */
  : Y_NAME '(' param_list_c_ast_opt rparen_func_qualifier_list_c_tid_opt
    {
      DUMP_START( "knr_func_or_constructor_decl_c_ast",
                  "NAME '(' param_list_c_ast_opt ')'" );
      DUMP_STR( "NAME", $1 );
      DUMP_AST_LIST( "param_list_c_ast_opt", $3 );
      DUMP_TID( "func_qualifier_list_c_tid_opt", $4 );

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
        ret_ast->type.base_tid = TB_INT;

        $$ = c_ast_new_gc( K_FUNCTION, &@$ );
        $$->as.func.ret_ast = ret_ast;
      } else {
        //
        // In C++, encountering a name followed by '(' declares an in-class
        // constructor.
        //
        $$ = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
        $$->type.store_tid = c_type_id_check( $4, C_TPID_STORE );
      }

      c_ast_set_sname( $$, &sname );
      $$->as.func.param_ast_list = $3;

      DUMP_AST( "knr_func_or_constructor_decl_c_ast", $$ );
      DUMP_END();

      if ( (opt_lang & LANG_C_MIN(99)) != LANG_NONE ) {
        //
        // In C99 and later, however, implicit int is an error.  This check has
        // to be done now in the parser rather than later in the AST since the
        // AST had no "memory" that the return type was implicitly int.
        //
        print_error( &@1,
          "implicit \"%s\" functions are illegal in %s and later\n",
          L_INT, c_lang_name( LANG_C_99 )
        );
        PARSE_ABORT();
      }
    }
  ;

rparen_func_qualifier_list_c_tid_opt
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
    func_qualifier_list_c_tid_opt
    {
      lexer_keyword_ctx = C_KW_CTX_ALL;
      $$ = $3;
    }
  ;

func_qualifier_list_c_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | func_qualifier_list_c_tid_opt func_qualifier_c_tid
    {
      DUMP_START( "func_qualifier_list_c_tid_opt",
                  "func_qualifier_list_c_tid_opt func_qualifier_c_tid" );
      DUMP_TID( "func_qualifier_list_c_tid_opt", $1 );
      DUMP_TID( "func_qualifier_c_tid", $2 );

      $$ = $1;
      C_TYPE_ID_ADD( &$$, $2, @2 );

      DUMP_TID( "func_qualifier_list_c_tid_opt", $$ );
      DUMP_END();
    }
  ;

func_qualifier_c_tid
  : cv_qualifier_tid
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

func_ref_qualifier_c_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_AMPER                       { $$ = TS_REFERENCE; }
  | Y_AMPER2                      { $$ = TS_RVALUE_REFERENCE; }
  ;

noexcept_c_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_NOEXCEPT
  | Y_NOEXCEPT '(' no_except_bool_tid_exp rparen_exp
    {
      $$ = $3;
    }
  | Y_THROW lparen_exp rparen_exp
  ;

no_except_bool_tid_exp
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

      DUMP_START( "trailing_return_type_c_ast_opt",
                  "type_c_ast cast_c_astp_opt" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
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
          "trailing return type is not supported%s\n",
          c_lang_until( LANG_CPP_11 )
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
      if ( ia_type_ast_peek()->type.base_tid != TB_AUTO ) {
        print_error( &ia_type_ast_peek()->loc,
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

func_equals_c_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | '=' Y_DEFAULT                 { $$ = $2; }
  | '=' Y_DELETE                  { $$ = $2; }
  | '=' Y_INT_LIT
    {
      if ( $2 != 0 ) {
        print_error( &@2, "'0' expected\n" );
        PARSE_ABORT();
      }
      $$ = TS_PURE_VIRTUAL | TS_VIRTUAL;
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

nested_decl_c_astp
  : '(' placeholder_c_ast
    {
      ia_type_ast_push( $2 );
      ++ast_depth;
    }
    decl_c_astp rparen_exp
    {
      ia_type_ast_pop();
      --ast_depth;

      DUMP_START( "nested_decl_c_astp",
                  "'(' placeholder_c_ast decl_c_astp ')'" );
      DUMP_AST( "placeholder_c_ast", $2 );
      DUMP_AST( "decl_c_astp", $4.ast );

      $$ = $4;

      DUMP_AST( "nested_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

oper_decl_c_astp
  : // in_attr: type_c_ast
    oper_c_ast lparen_exp param_list_c_ast_opt
    rparen_func_qualifier_list_c_tid_opt func_ref_qualifier_c_tid_opt
    noexcept_c_tid_opt trailing_return_type_c_ast_opt func_equals_c_tid_opt
    {
      c_ast_t    *const oper_c_ast = $1;
      c_type_id_t const func_qualifier_tid = $4;
      c_type_id_t const func_ref_qualifier_tid = $5;
      c_type_id_t const func_equals_tid = $8;
      c_type_id_t const noexcept_tid = $6;
      c_ast_t    *const trailing_ret_ast = $7;
      c_ast_t    *const type_ast = ia_type_ast_peek();

      DUMP_START( "oper_decl_c_astp",
                  "oper_c_ast '(' param_list_c_ast_opt ')' "
                  "func_qualifier_list_c_tid_opt "
                  "func_ref_qualifier_c_tid_opt noexcept_c_tid_opt "
                  "trailing_return_type_c_ast_opt "
                  "func_equals_c_tid_opt" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "oper_c_ast", oper_c_ast );
      DUMP_AST_LIST( "param_list_c_ast_opt", $3 );
      DUMP_TID( "func_qualifier_list_c_tid_opt", func_qualifier_tid );
      DUMP_TID( "func_ref_qualifier_c_tid_opt", func_ref_qualifier_tid );
      DUMP_TID( "noexcept_c_tid_opt", noexcept_tid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );
      DUMP_TID( "func_equals_c_tid_opt", func_equals_tid );

      c_type_id_t const oper_store_tid =
        func_qualifier_tid | func_ref_qualifier_tid | noexcept_tid |
        func_equals_tid;

      c_ast_t *const oper_ast = c_ast_new_gc( K_OPERATOR, &@$ );
      oper_ast->type.store_tid =
        c_type_id_check( oper_store_tid, C_TPID_STORE );
      oper_ast->as.oper.param_ast_list = $3;
      oper_ast->as.oper.oper_id = oper_c_ast->as.oper.oper_id;

      $$.ast = c_ast_add_func(
        oper_c_ast,
        trailing_ret_ast != NULL ? trailing_ret_ast : type_ast,
        oper_ast
      );

      $$.target_ast = oper_ast->as.oper.ret_ast;

      DUMP_AST( "oper_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

oper_c_ast
  : // in_attr: type_c_ast
    scope_sname_c_opt operator_exp c_operator
    {
      DUMP_START( "oper_c_ast", "OPERATOR c_operator" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_SNAME( "scope_sname_c_opt", $1 );
      DUMP_STR( "c_operator", c_oper_get( $3 )->name );

      $$ = ia_type_ast_peek();
      c_ast_set_sname( $$, &$1 );
      $$->as.oper.oper_id = $3;

      DUMP_AST( "oper_c_ast", $$ );
      DUMP_END();
    }
  ;

placeholder_c_ast
  : /* empty */                   { $$ = c_ast_new_gc( K_PLACEHOLDER, &@$ ); }
  ;

pointer_decl_c_astp
  : pointer_type_c_ast { ia_type_ast_push( $1 ); } decl_c_astp
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_decl_c_astp", "pointer_type_c_ast decl_c_astp" );
      DUMP_AST( "pointer_type_c_ast", $1 );
      DUMP_AST( "decl_c_astp", $3.ast );

      PJL_IGNORE_RV( c_ast_patch_placeholder( $1, $3.ast ) );
      $$ = $3;

      DUMP_AST( "pointer_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

pointer_type_c_ast
  : // in_attr: type_c_ast
    '*' type_qualifier_list_c_tid_opt
    {
      DUMP_START( "pointer_type_c_ast", "* type_qualifier_list_c_tid_opt" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_TID( "type_qualifier_list_c_tid_opt", $2 );

      $$ = c_ast_new_gc( K_POINTER, &@$ );
      $$->type.store_tid = c_type_id_check( $2, C_TPID_STORE );
      c_ast_set_parent( ia_type_ast_peek(), $$ );

      DUMP_AST( "pointer_type_c_ast", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_decl_c_astp
  : pointer_to_member_type_c_ast { ia_type_ast_push( $1 ); } decl_c_astp
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_to_member_decl_c_astp",
                  "pointer_to_member_type_c_ast decl_c_astp" );
      DUMP_AST( "pointer_to_member_type_c_ast", $1 );
      DUMP_AST( "decl_c_astp", $3.ast );

      $$ = $3;

      DUMP_AST( "pointer_to_member_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

pointer_to_member_type_c_ast
  : // in_attr: type_c_ast
    any_sname_c Y_COLON2_STAR cv_qualifier_list_tid_opt
    {
      DUMP_START( "pointer_to_member_type_c_ast",
                  "sname '::*' cv_qualifier_list_tid_opt" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_SNAME( "sname", $1 );
      DUMP_TID( "cv_qualifier_list_tid_opt", $3 );

      $$ = c_ast_new_gc( K_POINTER_TO_MEMBER, &@$ );

      c_type_t scope_type = *c_sname_local_type( &$1 );
      if ( (scope_type.base_tid & TB_ANY_SCOPE) == TB_NONE ) {
        //
        // The sname has no scope type, but we now know there's a pointer-to-
        // member of it, so it must be a class.  (It could alternatively be a
        // struct, but we have no context to know, so just pick class because
        // it's more C++-like.)
        //
        scope_type.base_tid = TB_CLASS;
        c_sname_set_local_type( &$1, &scope_type );
      }

      // adopt sname's scope type for the AST
      $$->type = c_type_or( &C_TYPE_LIT_S( $3 ), &scope_type );

      $$->as.ptr_mbr.class_sname = $1;
      c_ast_set_parent( ia_type_ast_peek(), $$ );

      DUMP_AST( "pointer_to_member_type_c_ast", $$ );
      DUMP_END();
    }
  ;

reference_decl_c_astp
  : reference_type_c_ast { ia_type_ast_push( $1 ); } decl_c_astp
    {
      ia_type_ast_pop();

      DUMP_START( "reference_decl_c_astp", "reference_type_c_ast decl_c_astp" );
      DUMP_AST( "reference_type_c_ast", $1 );
      DUMP_AST( "decl_c_astp", $3.ast );

      $$ = $3;

      DUMP_AST( "reference_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

reference_type_c_ast
  : // in_attr: type_c_ast
    Y_AMPER restrict_qualifier_c_tid_opt
    {
      DUMP_START( "reference_type_c_ast", "&" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_TID( "restrict_qualifier_c_tid_opt", $2 );

      $$ = c_ast_new_gc( K_REFERENCE, &@$ );
      $$->type.store_tid = c_type_id_check( $2, C_TPID_STORE );
      c_ast_set_parent( ia_type_ast_peek(), $$ );

      DUMP_AST( "reference_type_c_ast", $$ );
      DUMP_END();
    }

  | // in_attr: type_c_ast
    Y_AMPER2 restrict_qualifier_c_tid_opt
    {
      DUMP_START( "reference_type_c_ast", "&&" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_TID( "restrict_qualifier_c_tid_opt", $2 );

      $$ = c_ast_new_gc( K_RVALUE_REFERENCE, &@$ );
      $$->type.store_tid = c_type_id_check( $2, C_TPID_STORE );
      c_ast_set_parent( ia_type_ast_peek(), $$ );

      DUMP_AST( "reference_type_c_ast", $$ );
      DUMP_END();
    }
  ;

restrict_qualifier_c_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | restrict_qualifier_c_tid
  ;

typedef_type_decl_c_ast
  : // in_attr: type_c_ast
    typedef_type_c_ast
    {
      DUMP_START( "typedef_type_decl_c_ast", "typedef_type_c_ast" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_AST( "typedef_type_c_ast", $1 );

      if ( c_type_is_tid_any( &ia_type_ast_peek()->type, TS_TYPEDEF ) ) {
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

user_defined_conversion_decl_c_astp
  : // in_attr: type_c_ast
    scope_sname_c_opt Y_OPERATOR type_c_ast
    {
      ia_type_ast_push( $3 );
    }
    udc_decl_c_ast_opt lparen_exp rparen_func_qualifier_list_c_tid_opt
    noexcept_c_tid_opt func_equals_c_tid_opt
    {
      ia_type_ast_pop();

      DUMP_START( "user_defined_conversion_decl_c_astp",
                  "scope_sname_c_opt OPERATOR "
                  "type_c_ast udc_decl_c_ast_opt '(' ')' "
                  "func_qualifier_list_c_tid_opt noexcept_c_tid_opt "
                  "func_equals_c_tid_opt" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_SNAME( "scope_sname_c_opt", $1 );
      DUMP_AST( "type_c_ast", $3 );
      DUMP_AST( "udc_decl_c_ast_opt", $5 );
      DUMP_TID( "func_qualifier_list_c_tid_opt", $7 );
      DUMP_TID( "noexcept_c_tid_opt", $8 );
      DUMP_TID( "func_equals_c_tid_opt", $9 );

      $$.ast = c_ast_new_gc( K_USER_DEF_CONVERSION, &@$ );
      $$.ast->sname = $1;
      $$.ast->type.store_tid = c_type_id_check( $7 | $8 | $9, C_TPID_STORE );
      if ( ia_type_ast_peek() != NULL )
        c_type_or_eq( &$$.ast->type, &ia_type_ast_peek()->type );
      $$.ast->as.udef_conv.conv_ast = $5 != NULL ? $5 : $3;
      $$.target_ast = $$.ast->as.udef_conv.conv_ast;

      DUMP_AST( "user_defined_conversion_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

user_defined_literal_decl_c_astp
  : // in_attr: type_c_ast
    user_defined_literal_c_ast lparen_exp param_list_c_ast ')'
    noexcept_c_tid_opt trailing_return_type_c_ast_opt
    {
      c_ast_t    *const udl_c_ast = $1;
      c_type_id_t const noexcept_tid = $5;
      c_ast_t    *const trailing_ret_ast = $6;
      c_ast_t    *const type_ast = ia_type_ast_peek();

      DUMP_START( "user_defined_literal_decl_c_astp",
                  "user_defined_literal_c_ast '(' param_list_c_ast ')' "
                  "noexcept_c_tid_opt trailing_return_type_c_ast_opt" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "user_defined_literal_c_ast", udl_c_ast );
      DUMP_AST_LIST( "param_list_c_ast", $3 );
      DUMP_TID( "noexcept_c_tid_opt", noexcept_tid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );

      c_ast_t *const udl_ast = c_ast_new_gc( K_USER_DEF_LITERAL, &@$ );
      udl_ast->type.store_tid = c_type_id_check( noexcept_tid, C_TPID_STORE );
      udl_ast->as.udef_lit.param_ast_list = $3;

      $$.ast = c_ast_add_func(
        udl_c_ast,
        trailing_ret_ast != NULL ? trailing_ret_ast : type_ast,
        udl_ast
      );

      $$.target_ast = udl_ast->as.udef_lit.ret_ast;

      DUMP_AST( "user_defined_literal_decl_c_astp", $$.ast );
      DUMP_END();
    }
  ;

user_defined_literal_c_ast
  : // in_attr: type_c_ast
    scope_sname_c_opt operator_exp quote2_exp name_exp
    {
      DUMP_START( "user_defined_literal_c_ast", "OPERATOR \"\" NAME" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_SNAME( "scope_sname_c_opt", $1 );
      DUMP_STR( "name", $4 );

      $$ = ia_type_ast_peek();
      c_ast_set_sname( $$, &$1 );
      c_ast_append_name( $$, $4 );

      DUMP_AST( "user_defined_literal_c_ast", $$ );
      DUMP_END();
    }
  ;

using_decl_c_ast
  : Y_USING
    {
      // see the comment in "explain"
      gibberish_to_english();
    }
    any_name_exp equals_exp type_c_ast
    {
      // see the comment in "define_english" about TS_TYPEDEF
      C_TYPE_ADD_TID( &$5->type, TS_TYPEDEF, @5 );
      ia_type_ast_push( $5 );
    }
    cast_c_astp_opt
    {
      ia_type_ast_pop();

      DUMP_START( "using_decl_c_ast",
                  "USING any_name_exp '=' type_c_ast cast_c_astp_opt" );
      DUMP_STR( "any_name_exp", $3 );
      DUMP_AST( "type_c_ast", $5 );
      DUMP_AST( "cast_c_astp_opt", $7.ast );

      $$ = c_ast_patch_placeholder( $5, $7.ast );
      c_ast_set_name( $$, $3 );

      c_sname_t temp_sname = c_sname_dup( &in_attr.current_scope );
      c_ast_set_local_type( $$, c_sname_local_type( &in_attr.current_scope ) );
      c_ast_prepend_sname( $$, &temp_sname );

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
          "\"%s\" is not supported%s\n",
          L_USING, c_lang_until( LANG_CPP_11 )
        );
        PARSE_ABORT();
      }

      C_AST_CHECK_DECL( $$ );
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  function parameter gibberish productions
///////////////////////////////////////////////////////////////////////////////

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

      if ( $$->kind_id == K_FUNCTION )
        $$ = c_ast_pointer( $$ );       // see the comment in decl_english_ast

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

///////////////////////////////////////////////////////////////////////////////
//  type gibberish productions
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
      DUMP_TYPE( "type_modifier_list_c_type", &$1 );

      //
      // Prior to C99, typeless declarations are implicitly int, so we set it
      // here.  In C99 and later, however, implicit int is an error, so we
      // don't set it here and c_ast_check_declaration() will catch the error
      // later.
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
  | type_modifier_list_c_type unmodified_type_c_ast
    type_modifier_list_c_type_opt
    {
      DUMP_START( "type_c_ast",
                  "type_modifier_list_c_type unmodified_type_c_ast "
                  "type_modifier_list_c_type_opt" );
      DUMP_TYPE( "type_modifier_list_c_type", &$1 );
      DUMP_AST( "unmodified_type_c_ast", $2 );
      DUMP_TYPE( "type_modifier_list_c_type_opt", &$3 );

      $$ = $2;
      C_TYPE_ADD( &$$->type, &$1, @1 );
      C_TYPE_ADD( &$$->type, &$3, @3 );

      DUMP_AST( "type_c_ast", $$ );
      DUMP_END();
    }

    /*
     * Type with trailing type-modifier(s) declarations:
     *
     *      int const i;                // uncommon, but legal
     */
  | unmodified_type_c_ast type_modifier_list_c_type_opt
    {
      DUMP_START( "type_c_ast",
                  "unmodified_type_c_ast type_modifier_list_c_type_opt" );
      DUMP_AST( "unmodified_type_c_ast", $1 );
      DUMP_TYPE( "type_modifier_list_c_type_opt", &$2 );

      $$ = $1;
      C_TYPE_ADD( &$$->type, &$2, @2 );

      DUMP_AST( "type_c_ast", $$ );
      DUMP_END();
    }
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
      DUMP_TYPE( "type_modifier_list_c_type", &$1 );
      DUMP_TYPE( "type_modifier_c_type", &$2 );

      $$ = $1;
      C_TYPE_ADD( &$$, &$2, @2 );

      DUMP_TYPE( "type_modifier_list_c_type", &$$ );
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
      // is that, without an unmodified_type_c_ast (like int), the parser would
      // ordinarily take the typedef'd type (here, the size_t) as part of the
      // type_c_ast and then be out of tokens for the decl_c_astp -- at which
      // time it'll complain.
      //
      // Since type modifiers can't apply to a typedef'd type (e.g., "long
      // size_t" is illegal), we tell the lexer not to return either
      // Y_TYPEDEF_NAME or Y_TYPEDEF_SNAME if we encounter at least one type
      // modifier (except "register" since it's is really a storage class --
      // see the comment in type_modifier_base_type about "register").
      //
      if ( $$.store_tid == TS_REGISTER )
        lexer_find |= LEXER_FIND_TYPEDEFS;
      else
        lexer_find &= ~LEXER_FIND_TYPEDEFS;
    }
  | type_qualifier_c_tid          { $$ = C_TYPE_LIT_S( $1 ); }
  | storage_class_c_type
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

unmodified_type_c_ast
  : atomic_specifier_type_c_ast
  | builtin_type_ast
  | enum_class_struct_union_c_ast
  | typedef_type_c_ast
  ;

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
      C_TYPE_ADD_TID( &$$->type, TS_ATOMIC, @1 );

      DUMP_AST( "atomic_specifier_type_c_ast", $$ );
      DUMP_END();
    }
  ;

builtin_type_ast
  : builtin_tid
    {
      DUMP_START( "builtin_type_ast", "builtin_tid" );
      DUMP_TID( "builtin_tid", $1 );

      $$ = c_ast_new_gc( K_BUILTIN, &@$ );
      $$->type.base_tid = c_type_id_check( $1, C_TPID_BASE );

      DUMP_AST( "builtin_type_ast", $$ );
      DUMP_END();
    }
  ;

builtin_tid
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

enum_class_struct_union_c_ast
  : enum_class_struct_union_c_tid attribute_specifier_list_c_tid_opt
    any_sname_c_exp enum_fixed_type_c_ast_opt
    {
      DUMP_START( "enum_class_struct_union_c_ast",
                  "enum_class_struct_union_c_tid "
                  "attribute_specifier_list_c_tid_opt sname" );
      DUMP_TID( "enum_class_struct_union_c_tid", $1 );
      DUMP_TID( "attribute_specifier_list_c_tid_opt", $2 );
      DUMP_SNAME( "any_sname_c", $3 );
      DUMP_AST( "enum_fixed_type_c_ast_opt", $4 );

      $$ = c_ast_new_gc( K_ENUM_CLASS_STRUCT_UNION, &@$ );
      $$->type.base_tid = c_type_id_check( $1, C_TPID_BASE );
      $$->type.attr_tid = c_type_id_check( $2, C_TPID_ATTR );
      $$->as.ecsu.of_ast = $4;
      $$->as.ecsu.ecsu_sname = $3;

      DUMP_AST( "enum_class_struct_union_c_ast", $$ );
      DUMP_END();
    }

  | enum_class_struct_union_c_tid attribute_specifier_list_c_tid_opt
    any_sname_c_opt '{'
    {
      print_error( &@4,
        "explaining %s declarations is not supported by %s\n",
        c_type_id_name_c( $1 ), PACKAGE
      );
      c_sname_free( &$3 );
      PARSE_ABORT();
    }
  ;

enum_class_struct_union_c_tid
  : enum_tid
  | class_struct_union_tid
  ;

enum_tid
  : Y_ENUM class_struct_tid_opt   { $$ = $1 | $2; }
  ;

enum_fixed_type_c_ast_opt
  : /* empty */                   { $$ = NULL; }
  | ':' enum_fixed_type_c_ast     { $$ = $2; }
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
  : enum_fixed_type_modifier_list_tid
    {
      DUMP_START( "enum_fixed_type_c_ast",
                  "enum_fixed_type_modifier_list_tid" );
      DUMP_TID( "enum_fixed_type_modifier_list_tid", $1 );

      $$ = c_ast_new_gc( K_BUILTIN, &@1 );
      $$->type.base_tid = c_type_id_check( $1, C_TPID_BASE );

      DUMP_AST( "enum_fixed_type_c_ast", $$ );
      DUMP_END();
    }

    /*
     * Type-modifier(s) type with optional training type-modifier(s):
     *
     *      enum E : short int unsigned // uncommon, but legal
     */
  | enum_fixed_type_modifier_list_tid enum_unmodified_fixed_type_c_ast
    enum_fixed_type_modifier_list_tid_opt
    {
      DUMP_START( "enum_fixed_type_c_ast",
                  "enum_fixed_type_modifier_list_tid "
                  "enum_unmodified_fixed_type_c_ast "
                  "enum_fixed_type_modifier_list_tid_opt" );
      DUMP_TID( "enum_fixed_type_modifier_list_tid", $1 );
      DUMP_AST( "enum_unmodified_fixed_type_c_ast", $2 );
      DUMP_TID( "enum_fixed_type_modifier_list_tid_opt", $3 );

      $$ = $2;
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
  | enum_unmodified_fixed_type_c_ast enum_fixed_type_modifier_list_tid_opt
    {
      DUMP_START( "enum_fixed_type_c_ast",
                  "enum_unmodified_fixed_type_c_ast "
                  "enum_fixed_type_modifier_list_tid_opt" );
      DUMP_AST( "enum_unmodified_fixed_type_c_ast", $1 );
      DUMP_TID( "enum_fixed_type_modifier_list_tid_opt", $2 );

      $$ = $1;
      C_TYPE_ADD_TID( &$$->type, $2, @2 );

      DUMP_AST( "enum_fixed_type_c_ast", $$ );
      DUMP_END();
    }
  ;

enum_fixed_type_modifier_list_tid_opt
  : /* empty */                   { $$ = TB_NONE; }
  | enum_fixed_type_modifier_list_tid
  ;

enum_fixed_type_modifier_list_tid
  : enum_fixed_type_modifier_list_tid enum_fixed_type_modifier_tid
    {
      DUMP_START( "enum_fixed_type_modifier_list_tid",
                  "enum_fixed_type_modifier_list_tid "
                  "enum_fixed_type_modifier_tid" );
      DUMP_TID( "enum_fixed_type_modifier_list_tid", $1 );
      DUMP_TID( "enum_fixed_type_modifier_tid", $2 );

      $$ = $1;
      C_TYPE_ID_ADD( &$$, $2, @2 );

      DUMP_TID( "enum_fixed_type_modifier_list_tid", $$ );
      DUMP_END();
    }

  | enum_fixed_type_modifier_tid
  ;

enum_fixed_type_modifier_tid
  : Y_LONG
  | Y_SHORT
  | Y_SIGNED
  | Y_UNSIGNED
  ;

enum_unmodified_fixed_type_c_ast
  : builtin_type_ast
  | typedef_type_c_ast
  ;

class_struct_tid_opt
  : /* empty */                   { $$ = TB_NONE; }
  | class_struct_tid
  ;

class_struct_tid
  : Y_CLASS
  | Y_STRUCT
  ;

class_struct_union_tid
  : class_struct_tid
  | Y_UNION
  ;

type_qualifier_list_c_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | type_qualifier_list_c_tid
  ;

type_qualifier_list_c_tid
  : type_qualifier_list_c_tid type_qualifier_c_tid
    gnu_attribute_specifier_list_c_opt
    {
      DUMP_START( "type_qualifier_list_c_tid",
                  "type_qualifier_list_c_tid type_qualifier_c_tid" );
      DUMP_TID( "type_qualifier_list_c_tid", $1 );
      DUMP_TID( "type_qualifier_c_tid", $2 );

      $$ = $1;
      C_TYPE_ID_ADD( &$$, $2, @2 );

      DUMP_TID( "type_qualifier_list_c_tid", $$ );
      DUMP_END();
    }

  | gnu_attribute_specifier_list_c type_qualifier_c_tid
    {
      $$ = $2;
    }

  | type_qualifier_c_tid gnu_attribute_specifier_list_c_opt
    {
      $$ = $1;
    }
  ;

type_qualifier_c_tid
  : Y__ATOMIC_QUAL
  | cv_qualifier_tid
  | restrict_qualifier_c_tid
  | Y_UPC_RELAXED
  | Y_UPC_SHARED upc_layout_qualifier_opt
  | Y_UPC_STRICT
  ;

cv_qualifier_tid
  : Y_CONST
  | Y_VOLATILE
  ;

cv_qualifier_list_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | cv_qualifier_list_tid_opt cv_qualifier_tid
    {
      DUMP_START( "cv_qualifier_list_tid_opt",
                  "cv_qualifier_list_tid_opt cv_qualifier_tid" );
      DUMP_TID( "cv_qualifier_list_tid_opt", $1 );
      DUMP_TID( "cv_qualifier_tid", $2 );

      $$ = $1;
      C_TYPE_ID_ADD( &$$, $2, @2 );

      DUMP_TID( "cv_qualifier_list_tid_opt", $$ );
      DUMP_END();
    }
  ;

restrict_qualifier_c_tid
  : Y_RESTRICT                          // C only
    { //
      // This check has to be done now in the parser rather than later in the
      // AST since both "restrict" and "__restrict" map to TS_RESTRICT and the
      // AST has no "memory" of which it was.
      //
      if ( OPT_LANG_IS(CPP_ANY) ) {
        print_error( &@1,
          "\"%s\" is not supported in C++; use \"%s\" instead\n",
          L_RESTRICT, L_GNU___RESTRICT
        );
        PARSE_ABORT();
      }
    }
  | Y_GNU___RESTRICT                    // GNU C/C++ extension
  ;

upc_layout_qualifier_opt
  : /* empty */
  | '[' ']'
  | '[' Y_INT_LIT rbracket_exp
  | '[' '*' rbracket_exp
  | '[' error ']'
    {
      elaborate_error( "one of nothing, integer, or '*' expected" );
    }
  ;

storage_class_c_type
  : attribute_specifier_list_c_tid
    {
      $$ = C_TYPE_LIT_A( $1 );
    }
  | Y_AUTO_STORAGE                { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_APPLE___BLOCK               { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_CONSTEVAL                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_CONSTEXPR                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_CONSTINIT                   { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXPLICIT                    { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXPORT                      { $$ = C_TYPE_LIT_S( $1 ); }
  | Y_EXTERN                      { $$ = C_TYPE_LIT_S( $1 ); }
  | extern_linkage_c_tid          { $$ = C_TYPE_LIT_S( $1 ); }
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

attribute_specifier_list_c_tid_opt
  : /* empty */                   { $$ = TA_NONE; }
  | attribute_specifier_list_c_tid
  ;

attribute_specifier_list_c_tid
  : Y_ATTR_BEGIN '['
    {
      if ( unsupported( LANG_C_CPP_MIN(2X,11) ) ) {
        print_error( &@1,
          "\"[[\" attribute syntax is not supported%s\n",
          c_lang_until( LANG_C_CPP_MIN(2X,11) )
        );
        PARSE_ABORT();
      }
      lexer_keyword_ctx = C_KW_CTX_ATTRIBUTE;
    }
    using_opt attribute_list_c_tid_opt ']' rbracket_exp
    {
      lexer_keyword_ctx = C_KW_CTX_ALL;

      DUMP_START( "attribute_specifier_list_c_tid",
                  "'[[' using_opt attribute_list_c_tid_opt ']]'" );
      DUMP_TID( "attribute_list_c_tid_opt", $5 );

      $$ = $5;

      DUMP_END();
    }

  | gnu_attribute_specifier_list_c
    {
      $$ = TA_NONE;
    }
  ;

using_opt
  : /* empty */
  | Y_USING name_exp colon_exp
    {
      print_warning( &@1,
        "\"%s\" in attributes is not supported by %s (ignoring)\n",
        L_USING, PACKAGE
      );
      FREE( $2 );
    }
  ;

attribute_list_c_tid_opt
  : /* empty */                   { $$ = TA_NONE; }
  | attribute_list_c_tid
  ;

attribute_list_c_tid
  : attribute_list_c_tid comma_exp attribute_c_tid
    {
      DUMP_START( "attribute_list_c_tid",
                  "attribute_list_c_tid , attribute_c_tid" );
      DUMP_TID( "attribute_list_c_tid", $1 );
      DUMP_TID( "attribute_c_tid", $3 );

      $$ = $1;
      C_TYPE_ID_ADD( &$$, $3, @3 );

      DUMP_TID( "attribute_list_c_tid", $$ );
      DUMP_END();
    }

  | attribute_c_tid
  ;

attribute_c_tid
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
          "\"%s\": namespaced attributes are not supported by %s\n",
          c_sname_full_name( &$1 ), PACKAGE
        );
      }
      else {
        char const *adj = "unknown";
        c_lang_id_t lang_ids = LANG_NONE;

        char const *const name = c_sname_local_name( &$1 );
        c_keyword_t const *const k =
          c_keyword_find( name, opt_lang_newer(), C_KW_CTX_ATTRIBUTE );
        if ( k != NULL && c_type_id_tpid( k->type_id ) == C_TPID_ATTR ) {
          adj = "unsupported";
          lang_ids = k->lang_ids;
        }
        print_warning( &@1, "\"%s\": %s attribute", name, adj );
        if ( lang_ids != LANG_NONE )
          EPRINTF( " until %s", c_lang_oldest_name( lang_ids ) );
        print_suggestions( DYM_C_ATTRIBUTES, name );
        EPUTC( '\n' );
      }

      $$ = TA_NONE;
      c_sname_free( &$1 );
    }
  | error
    {
      elaborate_error_dym( DYM_C_ATTRIBUTES, "attribute name expected" );
    }
  ;

attribute_str_arg_c_opt
  : /* empty */
  | '(' Y_STR_LIT rparen_exp
    {
      print_warning( &@1,
        "attribute arguments are not supported by %s (ignoring)\n", PACKAGE
      );
      FREE( $2 );
    }
  | '(' error ')'
    {
      elaborate_error( "string literal expected" );
    }
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
      print_warning( &@1,
        "\"%s\" is not supported by %s (ignoring)",
        L_GNU___ATTRIBUTE__, PACKAGE
      );
      if ( (opt_lang & LANG_C_CPP_MIN(2X,11)) != LANG_NONE )
        print_hint( "[[...]]" );
      else
        EPUTC( '\n' );
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
      FREE( $1 );
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
  : Y_NAME                        { FREE( $1 ); }
  | Y_INT_LIT
  | Y_CHAR_LIT                    { FREE( $1 ); }
  | Y_STR_LIT                     { FREE( $1 ); }
  | '(' gnu_attribute_arg_list_c rparen_exp
  ;

///////////////////////////////////////////////////////////////////////////////
//  cast gibberish productions
///////////////////////////////////////////////////////////////////////////////

cast_c_astp_opt
  : /* empty */                   { $$ = (c_ast_pair_t){ NULL, NULL }; }
  | cast_c_astp
  ;

cast_c_astp
  : cast2_c_astp
  | pointer_cast_c_ast            { $$ = (c_ast_pair_t){ $1, NULL }; }
  | pointer_to_member_cast_c_ast  { $$ = (c_ast_pair_t){ $1, NULL }; }
  | reference_cast_c_ast          { $$ = (c_ast_pair_t){ $1, NULL }; }
  ;

cast2_c_astp
  : array_cast_c_astp
  | block_cast_c_astp
  | func_cast_c_astp
  | nested_cast_c_astp
  | sname_c_ast                   { $$ = (c_ast_pair_t){ $1, NULL }; }
//| typedef_type_decl_c_ast             // you can't cast a type
  ;

array_cast_c_astp
  : // in_attr: type_c_ast
    cast_c_astp_opt array_size_c_ast
    {
      DUMP_START( "array_cast_c_astp", "cast_c_astp_opt array_size_c_int" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_AST( "cast_c_astp_opt", $1.ast );
      DUMP_AST( "target_ast", $1.target_ast );
      DUMP_AST( "array_size_c_ast", $2 );

      c_ast_set_parent( c_ast_new_gc( K_PLACEHOLDER, &@1 ), $2 );

      if ( $1.target_ast != NULL ) {    // array-of or function-like-ret type
        $$.ast = $1.ast;
        $$.target_ast = c_ast_add_array( $1.target_ast, $2 );
      } else {
        c_ast_t *const ast = $1.ast != NULL ? $1.ast : ia_type_ast_peek();
        $$.ast = c_ast_add_array( ast, $2 );
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
  | '[' type_qualifier_list_c_tid rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = C_ARRAY_SIZE_NONE;
      $$->as.array.store_tid = c_type_id_check( $2, C_TPID_STORE );
    }
  | '[' type_qualifier_list_c_tid static_tid_opt Y_INT_LIT rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = $4;
      $$->as.array.store_tid = c_type_id_check( $2 | $3, C_TPID_STORE );
    }
  | '[' type_qualifier_list_c_tid_opt '*' rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = C_ARRAY_SIZE_VARIABLE;
      $$->as.array.store_tid = c_type_id_check( $2, C_TPID_STORE );
    }
  | '[' Y_STATIC type_qualifier_list_c_tid_opt Y_INT_LIT rbracket_exp
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = $4;
      $$->as.array.store_tid = c_type_id_check( $2 | $3, C_TPID_STORE );
    }
  ;

static_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_STATIC
  ;

block_cast_c_astp                       // Apple extension
  : // in_attr: type_c_ast
    '(' Y_CIRC
    { //
      // A block AST has to be the type inherited attribute for cast_c_astp_opt
      // so we have to create it here.
      //
      ia_type_ast_push( c_ast_new_gc( K_APPLE_BLOCK, &@$ ) );
    }
    type_qualifier_list_c_tid_opt cast_c_astp_opt rparen_exp
    lparen_exp param_list_c_ast_opt ')'
    {
      c_ast_t *const block_ast = ia_type_ast_pop();

      DUMP_START( "block_cast_c_astp",
                  "'(' '^' type_qualifier_list_c_tid_opt cast_c_astp_opt ')' "
                  "'(' param_list_c_ast_opt ')'" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_TID( "type_qualifier_list_c_tid_opt", $4 );
      DUMP_AST( "cast_c_astp_opt", $5.ast );
      DUMP_AST_LIST( "param_list_c_ast_opt", $8 );

      C_TYPE_ADD_TID( &block_ast->type, $4, @4 );
      block_ast->as.block.param_ast_list = $8;
      $$.ast = c_ast_add_func( $5.ast, ia_type_ast_peek(), block_ast );
      $$.target_ast = block_ast->as.block.ret_ast;

      DUMP_AST( "block_cast_c_astp", $$.ast );
      DUMP_END();
    }
  ;

func_cast_c_astp
  : // in_attr: type_c_ast
    cast2_c_astp '(' param_list_c_ast_opt rparen_func_qualifier_list_c_tid_opt
    trailing_return_type_c_ast_opt
    {
      c_ast_t    *const cast2_c_ast = $1.ast;
      c_type_id_t const func_ref_qualifier_tid = $4;
      c_ast_t    *const trailing_ret_ast = $5;
      c_ast_t    *const type_ast = ia_type_ast_peek();

      DUMP_START( "func_cast_c_astp",
                  "cast2_c_astp '(' param_list_c_ast_opt ')' "
                  "func_qualifier_list_c_tid_opt "
                  "trailing_return_type_c_ast_opt" );
      DUMP_AST( "(type_c_ast)", type_ast );
      DUMP_AST( "cast2_c_ast", cast2_c_ast );
      DUMP_AST_LIST( "param_list_c_ast_opt", $3 );
      DUMP_TID( "func_qualifier_list_c_tid_opt", func_ref_qualifier_tid );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );
      DUMP_AST( "target_ast", $1.target_ast );

      c_type_id_t const func_store_tid = func_ref_qualifier_tid;

      c_ast_t *const func_ast = c_ast_new_gc( K_FUNCTION, &@$ );
      func_ast->type.store_tid =
        c_type_id_check( func_store_tid, C_TPID_STORE );
      func_ast->as.func.param_ast_list = $3;

      if ( $1.target_ast != NULL ) {
        $$.ast = $1.ast;
        PJL_IGNORE_RV( c_ast_add_func( $1.target_ast, type_ast, func_ast ) );
      }
      else {
        $$.ast = c_ast_add_func(
          cast2_c_ast,
          trailing_ret_ast != NULL ? trailing_ret_ast : type_ast,
          func_ast
        );
      }

      $$.target_ast = func_ast->as.func.ret_ast;

      DUMP_AST( "func_cast_c_astp", $$.ast );
      DUMP_END();
    }
  ;

nested_cast_c_astp
  : '(' placeholder_c_ast
    {
      ia_type_ast_push( $2 );
      ++ast_depth;
    }
    cast_c_astp_opt rparen_exp
    {
      ia_type_ast_pop();
      --ast_depth;

      DUMP_START( "nested_cast_c_astp",
                  "'(' placeholder_c_ast cast_c_astp_opt ')'" );
      DUMP_AST( "placeholder_c_ast", $2 );
      DUMP_AST( "cast_c_astp_opt", $4.ast );

      $$ = $4;

      DUMP_AST( "nested_cast_c_astp", $$.ast );
      DUMP_END();
    }
  ;

pointer_cast_c_ast
  : pointer_type_c_ast { ia_type_ast_push( $1 ); } cast_c_astp_opt
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_cast_c_ast", "pointer_type_c_ast cast_c_astp_opt" );
      DUMP_AST( "pointer_type_c_ast", $1 );
      DUMP_AST( "cast_c_astp_opt", $3.ast );

      $$ = c_ast_patch_placeholder( $1, $3.ast );

      DUMP_AST( "pointer_cast_c_ast", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_cast_c_ast
  : pointer_to_member_type_c_ast { ia_type_ast_push( $1 ); } cast_c_astp_opt
    {
      ia_type_ast_pop();

      DUMP_START( "pointer_to_member_cast_c_ast",
                  "pointer_to_member_type_c_ast cast_c_astp_opt" );
      DUMP_AST( "pointer_to_member_type_c_ast", $1 );
      DUMP_AST( "cast_c_astp_opt", $3.ast );

      $$ = c_ast_patch_placeholder( $1, $3.ast );

      DUMP_AST( "pointer_to_member_cast_c_ast", $$ );
      DUMP_END();
    }
  ;

reference_cast_c_ast
  : reference_type_c_ast { ia_type_ast_push( $1 ); } cast_c_astp_opt
    {
      ia_type_ast_pop();

      DUMP_START( "reference_cast_c_ast",
                  "reference_type_c_ast cast_c_astp_opt" );
      DUMP_AST( "reference_type_c_ast", $1 );
      DUMP_AST( "cast_c_astp_opt", $3.ast );

      $$ = c_ast_patch_placeholder( $1, $3.ast );

      DUMP_AST( "reference_cast_c_ast", $$ );
      DUMP_END();
    }
  ;

///////////////////////////////////////////////////////////////////////////////
//  user-defined conversion gibberish productions
//
//  These are a subset of cast gibberish productions, specifically without
//  arrays, blocks, functions, or nested declarations, all of which are either
//  illegal or ambiguous.
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
//  name productions
///////////////////////////////////////////////////////////////////////////////

any_name
  : Y_NAME
  | Y_TYPEDEF_NAME
    {
      assert( c_ast_count_name( $1->ast ) == 1 );
      $$ = check_strdup( c_ast_local_name( $1->ast ) );
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
      c_ast_set_name( $$, $1 );

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
  : any_typedef
    {
      DUMP_START( "typedef_type_c_ast", "any_typedef" );
      DUMP_AST( "any_typedef.ast", $1->ast );

      $$ = c_ast_new_gc( K_TYPEDEF, &@$ );
      $$->type.base_tid = TB_TYPEDEF;
      $$->as.tdef.for_ast = $1->ast;

      DUMP_AST( "typedef_type_c_ast", $$ );
      DUMP_END();
    }

  | // in_attr: type_c_ast
    any_typedef Y_COLON2 sname_c
    { //
      // This is for a case like:
      //
      //      define S as struct S
      //      explain int S::x
      //
      // that is: a typedef'd type used for a scope.
      //
      DUMP_START( "typedef_type_c_ast", "any_typedef '::' sname_c" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_AST( "any_typedef.ast", $1->ast );
      DUMP_SNAME( "sname_c", $3 );

      if ( ia_type_ast_peek() == NULL ) {
        print_error_unknown_name( &@3, &$3 );
        PARSE_ABORT();
      }

      $$ = ia_type_ast_peek();
      c_sname_t temp_name = c_ast_dup_name( $1->ast );
      c_ast_set_sname( $$, &temp_name );
      c_ast_append_sname( $$, &$3 );

      DUMP_AST( "typedef_type_c_ast", $$ );
      DUMP_END();
    }

  | // in_attr: type_c_ast
    any_typedef Y_COLON2 typedef_sname_c
    { //
      // This is for a case like:
      //
      //      define S as struct S
      //      define T as struct T
      //      explain int S::T::x
      //
      // that is: a typedef'd type used for an intermediate scope.
      //
      DUMP_START( "typedef_type_c_ast", "any_typedef '::' typedef_sname_c" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_AST( "any_typedef", $1->ast );
      DUMP_SNAME( "typedef_sname_c", $3 );

      if ( ia_type_ast_peek() == NULL ) {
        print_error_unknown_name( &@3, &$3 );
        PARSE_ABORT();
      }

      $$ = ia_type_ast_peek();
      c_sname_t temp_name = c_ast_dup_name( $1->ast );
      c_ast_set_sname( $$, &temp_name );
      c_ast_append_sname( $$, &$3 );

      DUMP_AST( "typedef_type_c_ast", $$ );
      DUMP_END();
    }
  ;

scope_sname_c_opt
  : /* empty */                   { c_sname_init( &$$ ); }

  | sname_c Y_COLON2
    {
      $$ = $1;
      if ( c_type_is_none( c_sname_local_type( &$1 ) ) ) {
        //
        // Since we know the name in this context (followed by "::") definitely
        // refers to a scope, set the scoped name's type to TB_SCOPE (if it
        // doesn't already have a scope type).
        //
        c_sname_set_local_type( &$$, &C_TYPE_LIT_B( TB_SCOPE ) );
      }
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
      $$ = c_ast_dup_name( $1->ast );
    }
  ;

sname_c
  : sname_c Y_COLON2 Y_NAME
    {
      // see the comment in "of_scope_english"
      if ( unsupported( LANG_CPP_ANY ) ) {
        print_error( &@2, "scoped names are not supported in C\n" );
        PARSE_ABORT();
      }

      DUMP_START( "sname_c", "sname_c '::' NAME" );
      DUMP_SNAME( "sname_c", $1 );
      DUMP_STR( "name", $3 );

      if ( c_type_is_none( c_sname_local_type( &$1 ) ) )
        c_sname_set_local_type( &$1, &C_TYPE_LIT_B( TB_SCOPE ) );
      $$ = $1;
      c_sname_append_name( &$$, $3 );

      DUMP_SNAME( "sname_c", $$ );
      DUMP_END();
    }

  | sname_c Y_COLON2 any_typedef
    {
      DUMP_START( "sname_c", "sname_c '::' any_typedef" );
      DUMP_SNAME( "sname_c", $1 );
      DUMP_AST( "any_typedef.ast", $3->ast );

      //
      // This is for a case like:
      //
      //      define S::int8_t as char
      //
      // that is: the type int8_t is an existing type in no scope being defined
      // as a distinct type in a new scope.
      //
      if ( c_type_is_none( c_sname_local_type( &$1 ) ) )
        c_sname_set_local_type( &$1, &C_TYPE_LIT_B( TB_SCOPE ) );
      $$ = $1;
      c_sname_t temp = c_ast_dup_name( $3->ast );
      c_sname_append_sname( &$$, &temp );

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
      DUMP_START( "sname_c_ast", "sname_c" );
      DUMP_AST( "(type_c_ast)", ia_type_ast_peek() );
      DUMP_SNAME( "sname", $1 );
      DUMP_INT( "bit_field_c_int_opt", $2 );

      $$ = ia_type_ast_peek();
      c_ast_set_sname( $$, &$1 );

      bool ok = true;
      //
      // This check has to be done now in the parser rather than later in the
      // AST since we need to use the builtin union member now.
      //
      if ( $2 != 0 && (ok = c_ast_is_builtin_any( $$, TB_ANY_INTEGRAL )) )
        $$->as.builtin.bit_width = (unsigned)$2;

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
    {
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
      DUMP_START( "Y_NAME of_scope_list_english_opt", "NAME" );
      DUMP_STR( "NAME", $1 );
      DUMP_SNAME( "of_scope_list_english_opt", $2 );

      c_sname_t sname = $2;
      c_sname_append_name( &sname, $1 );

      //
      // See if the full name is the name of a typedef'd type.
      //
      c_typedef_t const *const tdef = c_typedef_find( &sname );
      if ( tdef != NULL ) {
        $$ = c_ast_new_gc( K_TYPEDEF, &@$ );
        $$->type.base_tid = TB_TYPEDEF;
        $$->as.tdef.for_ast = tdef->ast;
        c_sname_free( &sname );
      } else {
        $$ = c_ast_new_gc( K_NAME, &@$ );
        c_ast_set_sname( $$, &sname );
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
      c_sname_set_local_type( &$$, c_ast_local_type( $3->ast ) );
      c_sname_t temp = c_ast_dup_name( $3->ast );
      c_sname_append_sname( &$$, &temp );

      DUMP_SNAME( "typedef_sname_c", $$ );
      DUMP_END();
    }

  | any_typedef                   { $$ = c_ast_dup_name( $1->ast ); }
  ;

///////////////////////////////////////////////////////////////////////////////
//  miscellaneous productions
///////////////////////////////////////////////////////////////////////////////

array_exp
  : Y_ARRAY
  | error
    {
      elaborate_error( "\"%s\" expected", L_ARRAY );
    }
  ;

as_exp
  : Y_AS
    { //
      // For either "declare" or "define", neither "override" nor "final" must
      // be matched initially to allow for cases like:
      //
      //      c++decl> declare final as int
      //      int final;
      //
      // (which is legal).  However, after parsing "as", the keyword context
      // has to be set to C_KW_CTX_MBR_FUNC to be able to match "override" and
      // "final", e.g.:
      //
      //      c++decl> declare f as final function
      //      void f() final;
      //
      lexer_keyword_ctx = C_KW_CTX_MBR_FUNC;
    }
  | error
    {
      elaborate_error( "\"%s\" expected", L_AS );
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
      elaborate_error( "\"%s\" expected", L_CAST );
    }
  ;

class_struct_tid_exp
  : class_struct_tid
  | error
    {
      if ( OPT_LANG_IS(CPP_ANY) )
        elaborate_error( "\"%s\" or \"%s\" expected", L_CLASS, L_STRUCT );
      else
        elaborate_error( "\"%s\" expected", L_STRUCT );
    }
  ;

colon_exp
  : ':'
  | error
    {
      elaborate_error( "':' expected" );
    }
  ;

comma_exp
  : ','
  | error
    {
      elaborate_error( "',' expected" );
    }
  ;

conversion_exp
  : Y_CONVERSION
  | error
    {
      elaborate_error( "\"%s\" expected", L_CONVERSION );
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

equals_exp
  : '='
  | error
    {
      elaborate_error( "'=' expected" );
    }
  ;

gt_exp
  : '>'
  | error
    {
      elaborate_error( "'>' expected" );
    }
  ;

inline_tid_opt
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
      elaborate_error( "\"%s\" expected", L_LITERAL );
    }
  ;

lparen_exp
  : '('
  | error
    {
      elaborate_error( "'(' expected" );
    }
  ;

lt_exp
  : '<'
  | error
    {
      elaborate_error( "'<' expected" );
    }
  ;

member_or_non_member_mask_opt
  : /* empty */                   { $$ = C_FUNC_UNSPECIFIED; }
  | Y_MEMBER                      { $$ = C_FUNC_MEMBER     ; }
  | Y_NON_MEMBER                  { $$ = C_FUNC_NON_MEMBER ; }
  ;

namespace_tid_exp
  : Y_NAMESPACE
  | error
    {
      elaborate_error( "\"%s\" expected", L_NAMESPACE );
    }
  ;

namespace_type
  : Y_NAMESPACE                   { $$ = C_TYPE_LIT_B( $1 ); }
  | Y_INLINE namespace_tid_exp    { $$ = C_TYPE_LIT( $2, $1, TA_NONE ); }
  ;

of_exp
  : Y_OF
  | error
    {
      elaborate_error( "\"%s\" expected", L_OF );
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
        print_error( &@2, "scoped names are not supported in C\n" );
        PARSE_ABORT();
      }
      $$ = $3;
      c_sname_set_local_type( &$$, &$2 );
    }
  ;

of_scope_list_english
  : of_scope_list_english of_scope_english
    {
      //
      // Ensure that neither "namespace" nor "scope" are nested within a
      // class/struct/union.
      //
      c_type_t const *const inner_type = c_sname_local_type( &$1 );
      c_type_t const *const outer_type = c_sname_local_type( &$2 );
      if ( c_type_is_tid_any( inner_type, TB_NAMESPACE | TB_SCOPE ) &&
           c_type_is_tid_any( outer_type, TB_ANY_CLASS ) ) {
        print_error( &@2,
          "\"%s\" may only be nested within a %s or %s\n",
          c_type_name_eng( inner_type ), L_NAMESPACE, L_SCOPE
        );
        PARSE_ABORT();
      }

      $$ = $2;                          // "of scope X of scope Y" means Y::X
      c_sname_set_local_type( &$$, inner_type );
      c_sname_append_sname( &$$, &$1 );
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
      elaborate_error( "\"%s\" expected", L_OPERATOR );
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
      elaborate_error( "'}' expected" );
    }
  ;

rbracket_exp
  : ']'
  | error
    {
      elaborate_error( "']' expected" );
    }
  ;

reference_exp
  : Y_REFERENCE
  | error
    {
      elaborate_error( "\"%s\" expected", L_REFERENCE );
    }
  ;

returning_exp
  : Y_RETURNING
  | error
    {
      elaborate_error( "\"%s\" expected", L_RETURNING );
    }
  ;

rparen_exp
  : ')'
  | error
    {
      elaborate_error( "')' expected" );
    }
  ;

scope_english_type
  : class_struct_union_tid        { $$ = C_TYPE_LIT_B( $1 ); }
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
      elaborate_error( "';' expected" );
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

to_exp
  : Y_TO
  | error
    {
      elaborate_error( "\"%s\" expected", L_TO );
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

virtual_tid_exp
  : Y_VIRTUAL
  | error
    {
      elaborate_error( "\"%s\" expected", L_VIRTUAL );
    }
  ;

virtual_tid_opt
  : /* empty */                   { $$ = TS_NONE; }
  | Y_VIRTUAL
  ;

%%

/// @endcond

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
