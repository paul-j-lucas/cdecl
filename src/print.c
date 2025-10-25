/*
**      cdecl -- C gibberish translator
**      src/print.c
**
**      Copyright (C) 2017-2025  Paul J. Lucas
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
 * Defines functions for printing error and warning messages.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "print.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_sname.h"
#include "c_typedef.h"
#include "cdecl.h"
#include "cdecl_dym.h"
#include "cdecl_keyword.h"
#include "cdecl_term.h"
#include "color.h"
#include "english.h"
#include "gibberish.h"
#include "lexer.h"
#include "options.h"
#include "p_macro.h"
#include "prompt.h"
#include "strbuf.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>                      /* for isspace(3) */
#include <stdarg.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <stdio.h>
#include <string.h>

/// @endcond

/// @cond DOXYGEN_IGNORE

// local constants
static char const *const  MORE[]     = { "...", "..." };
static size_t const       MORE_LEN[] = { 3,     3     };

/// @endcond

/**
 * @addtogroup printing-errors-warnings-group
 * @{
 */

// local functions
static void               print_input_line( size_t*, size_t );

NODISCARD
static size_t             token_len( char const*, size_t, size_t );

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries.

// extern variables
print_params_t            print_params;

/// @endcond

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints a message to standard error.
 *
 * @note In debug mode, also prints the file & line where the function was
 * called from.
 * @note A newline is _not_ printed.
 *
 * @param file The name of the file where this function was called from.
 * @param line The line number within \a file where this function was called
 * from.
 * @param loc The location of the error; may be NULL.
 * @param what What to print, e.g., `error` or `warning`.
 * @param what_color The color to print \a what in, if any.
 * @param format The `printf()` style format string.
 * @param args The `printf()` arguments.
 */
static void fl_print_impl( char const *file, int line, c_loc_t const *loc,
                           char const *what, char const *what_color,
                           char const *format, va_list args ) {
  assert( format != NULL );
  assert( what != NULL );

  if ( loc != NULL )
    print_loc( loc );
  color_start( stderr, what_color );
  EPUTS( what );
  color_end( stderr, what_color );
  EPUTS( ": " );

  print_debug_file_line( file, line );

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
  vfprintf( stderr, format, args );
#pragma GCC diagnostic pop
}

/**
 * Helper function for print_suggestions() and fput_list() that gets the string
 * for a \ref did_you_mean::known "known" literal.
 *
 * @param ppelt A pointer to the pointer to the \ref did_you_mean element to
 * get the string of.  On return, it is advanced to the next element.
 * @return Returns a pointer to the next "Did you mean" suggestion string or
 * NULL if none.
 */
NODISCARD
static char const* fput_list_dym_gets( void const **ppelt ) {
  did_you_mean_t const *const dym = *ppelt;
  if ( dym->known == NULL )
    return NULL;
  *ppelt = dym + 1;

  static strbuf_t sbufs[2];
  static unsigned buf_index;

  strbuf_t *const sbuf = &sbufs[ buf_index++ % ARRAY_SIZE( sbufs ) ];
  strbuf_reset( sbuf );
  return strbuf_printf( sbuf, "\"%s\"", dym->known );
}

/**
 * Gets the current input line.
 *
 * @param rv_len A pointer to receive the length of the input line.
 * @return Returns the input line.
 */
NODISCARD
static char const* get_input_line( size_t *rv_len ) {
  char const *input_line = lexer_input_line( rv_len );
  assert( input_line != NULL );
  if ( *rv_len == 0 ) {                 // no input? try command line
    input_line = print_params.command_line;
    assert( input_line != NULL );
    *rv_len = print_params.command_line_len;
  }
  if ( *rv_len >= print_params.inserted_len ) {
    input_line += print_params.inserted_len;
    *rv_len -= print_params.inserted_len;
  }
  assert( *rv_len > 0 );

  //
  // Chop off whitespace (if any) so we can always print a newline ourselves.
  //
  strn_rtrim( input_line, rv_len );

  return input_line;
}

/**
 * Prints the name of \a ast followed by `(aka` followed by the underlying type
 * in either pseudo-English or gibberish (depending on how it was declared).
 * For example, if a type was declared in pseudo-English like:
 *
 *      define RI as reference to int
 *
 * prints `"RI" (aka "reference to integer")`, that is the type name followed
 * by `(aka` and the underlying type in pseudo-English.
 *
 * However, if the underlying type was declared in gibberish like:
 *
 *      using RI = int&
 *
 * prints `"RI" (aka "int&")`, that is the type name followed by `(aka` and the
 * underlying type in gibberish.
 *
 * @note A newline is _not_ printed.
 *
 * @param ast The \ref c_ast to print.
 * @param fout The `FILE` to print to.
 */
static void print_ast_name_aka( c_ast_t const *ast, FILE *fout ) {
  assert( ast != NULL );
  assert( ast->kind != K_TYPEDEF );
  assert( !c_sname_empty( &ast->sname ) );
  assert( fout != NULL );

  FPRINTF( fout, "\"%s\" (aka, \"", c_sname_gibberish( &ast->sname ) );
  // Look-up the type so we can print it how it was originally defined.
  c_typedef_t const *const tdef = c_typedef_find_sname( &ast->sname );
  assert( tdef != NULL );
  print_type_ast( tdef, fout );
  FPUTS( "\")", fout );
}

/**
 * Prints the error line (if not interactive) and a `^` (in color, if possible
 * and requested) under the offending token.
 *
 * @param error_column The zero-based column of the offending token.
 * @return Returns \a error_column, adjusted (if necessary).
 */
NODISCARD
static size_t print_caret( size_t error_column ) {
  if ( !print_params.opt_no_print_input_line )
    error_column -= print_params.inserted_len;

  unsigned const term_columns = term_get_columns();
  size_t caret_column;

  if ( cdecl_is_interactive || opt_echo_commands ||
       print_params.opt_no_print_input_line ) {
    //
    // If we're interactive or echoing commands, we can put the ^ under the
    // already existing token we printed or the user typed for the recent
    // command, but we have to add the length of the prompt.
    //
    // However, if opt_no_print_input_line is true, we were instructed not to
    // print the input line (because the calling code will presumably print it
    // itself), so don't add in the length of the prompt.
    //
    size_t const prompt_len =
      print_params.opt_no_print_input_line ? 0 : cdecl_prompt_len();
    caret_column = (error_column + prompt_len) % term_columns;
  }
  else {
    //
    // Otherwise we have to print the line containing the error then print the
    // ^ under that.
    //
    print_input_line( &error_column, term_columns );
    caret_column = error_column;
  }

  FPUTNSP( caret_column, stderr );
  color_start( stderr, sgr_caret );
  EPUTC( '^' );
  color_end( stderr, sgr_caret );
  EPUTC( '\n' );

  return error_column;
}

/**
 * Prints the input line, "scrolled" to the left with `...` printed if
 * necessary, so that \a error_column is always within \a term_columns.
 *
 * @param error_column A pointer to the zero-based column of the offending
 * token.  It is adjusted if necessary to be the terminal column at which the
 * `^` should be printed.
 * @param term_columns The number of columns of the terminal.
 */
static void print_input_line( size_t *error_column, size_t term_columns ) {
  size_t input_line_len;
  char const *input_line = get_input_line( &input_line_len );
  assert( input_line != NULL );
  assert( input_line_len > 0 );

  if ( *error_column > input_line_len )
    *error_column = input_line_len;

  --term_columns;                     // more aesthetically pleasing

  //
  // If the error is due to unexpected end of input, back up the error
  // column so it refers to a non-null character.
  //
  if ( *error_column > 0 && input_line[ *error_column ] == '\0' )
    --*error_column;

  size_t const token_columns =
    token_len( input_line, input_line_len, *error_column );
  size_t const error_end_column = *error_column + token_columns - 1;

  //
  // Start with the number of printable columns equal to the length of the
  // line.
  //
  size_t print_columns = input_line_len;

  //
  // If the number of printable columns exceeds the number of terminal
  // columns, there is "more" on the right, so limit the number of printable
  // columns.
  //
  bool more[2];                       // [0] = left; [1] = right
  more[1] = print_columns > term_columns;
  if ( more[1] )
    print_columns = term_columns;

  //
  // If the error end column is past the number of printable columns, there
  // is "more" on the left since we will "scroll" the line to the left.
  //
  more[0] = error_end_column > print_columns;

  //
  // However, if there is "more" on the right but the end of the error token
  // is at the end of the line, then we can print through the end of the line
  // without any "more."
  //
  if ( more[1] ) {
    if ( error_end_column < input_line_len - 1 )
      print_columns -= MORE_LEN[1];
    else
      more[1] = false;
  }

  if ( more[0] ) {
    //
    // There is "more" on the left so we have to adjust the error column, the
    // number of printable columns, and the offset into the input line that we
    // start printing at to give the appearance that the input line has been
    // "scrolled" to the left.
    //
    assert( print_columns >= token_columns );
    size_t const error_column_term = print_columns - token_columns;
    print_columns -= MORE_LEN[0];
    assert( *error_column > error_column_term );
    input_line += MORE_LEN[0] + (*error_column - error_column_term);
    *error_column = error_column_term;
  }

  EPRINTF( "%s%.*s%s\n",
    (more[0] ? MORE[0] : ""),
    STATIC_CAST( int, print_columns ), input_line,
    (more[1] ? MORE[1] : "")
  );
}

/**
 * Gets the length of a token in \a s.
 *
 * @remarks
 * @parblock
 * Characters are divided into three classes:
 *
 *  + Whitespace.
 *  + Identifier (`[A-Za-z0-9_]`).
 *  + Everything else (e.g., punctuation).
 *
 * A token is composed of characters in exclusively one class.  The class is
 * determined by `s[` \a token_offset `]`.  The length of the token is the
 * number of consecutive characters of the same class.
 * @endparblock
 *
 * @param s The string to use.
 * @param s_len The length of \a s.
 * @param token_offset The offset within \a s of the start of the token.
 * @return Returns the length of the token.
 */
NODISCARD
static size_t token_len( char const *s, size_t s_len, size_t token_offset ) {
  assert( s != NULL );

  char const *const end = s + s_len;
  s += token_offset;

  if ( s >= end )
    return 0;

  bool const is_s0_ident = is_ident( s[0] );
  bool const is_s0_space = isspace( s[0] );

  char const *const s0 = s;
  while ( ++s < end && *s != '\0' ) {
    if ( is_s0_ident ) {
      if ( !is_ident( *s ) )
        break;
    }
    else if ( is_s0_space ) {
      if ( !isspace( *s ) )
        break;
    }
    else {
      if ( is_ident( *s ) || isspace( *s ) )
        break;
    }
  } // while
  return STATIC_CAST( size_t, s - s0 );
}

////////// extern functions ///////////////////////////////////////////////////

void fl_print_error( char const *file, int line, c_loc_t const *loc,
                     char const *format, ... ) {
  assert( format != NULL );
  va_list args;
  va_start( args, format );
  fl_print_impl( file, line, loc, "error", sgr_error, format, args );
  va_end( args );
}

void fl_print_error_unknown_name( char const *file, int line,
                                  c_loc_t const *loc, c_sname_t const *sname ) {
  assert( sname != NULL );

  dym_kind_t dym_kind = DYM_NONE;

  // Must dup this since c_sname_gibberish() returns a temporary buffer.
  char const *const name = check_strdup( c_sname_gibberish( sname ) );

  c_keyword_t const *const ck =
    c_keyword_find( name, LANG_ANY, C_KW_CTX_DEFAULT );
  if ( ck != NULL ) {
    char const *what = NULL;

    switch ( c_tid_tpid( ck->tid ) ) {
      case C_TPID_NONE:                 // e.g., "break"
      case C_TPID_STORE:                // e.g., "extern"
        dym_kind = DYM_C_KEYWORDS;
        what = "keyword";
        break;
      case C_TPID_BASE:                 // e.g., "char"
        dym_kind = DYM_C_TYPES;
        what = "type";
        break;
      case C_TPID_ATTR:
        dym_kind = DYM_C_ATTRIBUTES;    // e.g., "noreturn"
        what = "attribute";
        break;
    } // switch

    fl_print_error( file, line, loc,
      "\"%s\": unsupported %s%s", name, what, c_lang_which( ck->lang_ids )
    );
  }
  else {
    fl_print_error( file, line, loc, "\"%s\": unknown name", name );
  }

  print_suggestions( dym_kind, name );
  EPUTC( '\n' );
  FREE( name );
}

void fl_print_warning( char const *file, int line, c_loc_t const *loc,
                       char const *format, ... ) {
  assert( format != NULL );
  va_list args;
  va_start( args, format );
  fl_print_impl( file, line, loc, "warning", sgr_warning, format, args );
  va_end( args );
}

void print_ast_kind_aka( c_ast_t const *ast, FILE *fout ) {
  assert( ast != NULL );
  assert( fout != NULL );

  c_ast_t const *const raw_ast = c_ast_untypedef( ast );
  FPUTS( c_kind_name( raw_ast->kind ), fout );

  if ( raw_ast != ast ) {
    FPUTS( " type ", fout );
    print_ast_name_aka( raw_ast, fout );
  }
}

void print_ast_type_aka( c_ast_t const *ast, FILE *fout ) {
  assert( ast != NULL );
  assert( fout != NULL );

  c_ast_t const *const raw_ast = c_ast_untypedef( ast );
  if ( raw_ast == ast ) {               // not a typedef
    FPUTC( '"', fout );
    if ( is_english_to_gibberish() )
      c_ast_english( ast, C_ENG_DECL | C_ENG_OPT_OMIT_DECLARE, fout );
    else
      c_ast_gibberish( ast, C_GIB_USING, fout );
    FPUTC( '"', fout );
  }
  else {
    print_ast_name_aka( raw_ast, fout );
  }
}

void print_debug_file_line( char const *file, int line ) {
  assert( file != NULL );
  assert( line > 0 );
  if ( opt_cdecl_debug != CDECL_DEBUG_NO )
    EPRINTF( "[%s:%d] ", file, line );  // LCOV_EXCL_LINE
}

void print_error_token_is_a( char const *error_token ) {
  if ( error_token == NULL )
    return;

  p_macro_t const *const macro = p_macro_find( error_token );
  if ( macro != NULL ) {
    EPRINTF( " (\"%s\" is a macro)", error_token );
    return;
  }

  c_keyword_t const *const ck =
    c_keyword_find( error_token, LANG_ANY, C_KW_CTX_DEFAULT );
  if ( ck != NULL ) {
    c_lang_id_t const lang_ids = ck->lang_ids & ~LANGX_MASK;
    c_lang_id_t const oldest_lang_id = c_lang_oldest( lang_ids );
    if ( oldest_lang_id > opt_lang_id ) {
      EPRINTF(
        "; \"%s\" not a keyword until %s",
        error_token,
        c_lang_name( oldest_lang_id )
      );
    } else {
      EPRINTF( " (\"%s\" is a keyword", error_token );
      if ( lang_ids != ck->lang_ids )
        EPRINTF( " in %s", c_lang_name( c_lang_oldest( ck->lang_ids ) ) );
      EPUTC( ')' );
    }
    return;
  }

  if ( is_english_to_gibberish() ) {
    cdecl_keyword_t const *const cdk = cdecl_keyword_find( error_token );
    if ( cdk != NULL )
      EPRINTF( " (\"%s\" is a " CDECL " keyword)", error_token );
  }
}

void print_hint( char const *format, ... ) {
  assert( format != NULL );
  EPUTS( "; did you mean " );
  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
  EPUTS( "?\n" );
}

void print_loc( c_loc_t const *loc ) {
  assert( loc != NULL );
  unsigned const line = opt_lineno + STATIC_CAST( unsigned, loc->first_line );
  size_t const column = print_caret( STATIC_CAST( size_t, loc->first_column ) );

  if ( line > 0 ) {
    char const *path = NULL;
    if ( cdecl_input_path != NULL )
      path = cdecl_input_path;
    else if ( strcmp( opt_file, "-" ) != 0 )
      path = opt_file;

    if ( path != NULL ) {
      color_start( stderr, sgr_locus );
      EPUTS( path );
      color_end( stderr, sgr_locus );
      EPUTC( ':' );
    }

    if ( path != NULL || opt_lineno > 0 ||
        (!cdecl_is_interactive && print_params.command_line == NULL) ) {
      color_start( stderr, sgr_locus );
      EPRINTF( "%u", line );
      color_end( stderr, sgr_locus );
      EPUTC( ',' );
    }
  }

  color_start( stderr, sgr_locus );
  EPRINTF( "%zu", column + 1 );
  color_end( stderr, sgr_locus );
  EPUTS( ": " );
}

bool print_suggestions( dym_kind_t kinds, char const *unknown_token ) {
  did_you_mean_t const *const dym = cdecl_dym_new( kinds, unknown_token );
  if ( dym == NULL )
    return false;
  EPUTS( "; did you mean " );
  fput_list( stderr, dym, &fput_list_dym_gets );
  EPUTC( '?' );
  cdecl_dym_free( dym );
  return true;
}

void print_type_ast( c_typedef_t const *tdef, FILE *fout ) {
  assert( tdef != NULL );
  assert( fout != NULL );

  if ( (tdef->decl_flags & C_ENG_DECL) != 0 )
    c_ast_english( tdef->ast, tdef->decl_flags | C_ENG_OPT_OMIT_DECLARE, fout );
  else
    c_ast_gibberish( tdef->ast, C_GIB_USING, fout );
}

void print_type_decl( c_typedef_t const *tdef, decl_flags_t decl_flags,
                      FILE *fout ) {
  assert( tdef != NULL );
  assert( is_1_bit_in_set( decl_flags, C_TYPE_DECL_ANY ) );
  assert( fout != NULL );

  if ( (decl_flags & C_ENG_DECL) != 0 )
    c_typedef_english( tdef, fout );
  else
    c_typedef_gibberish( tdef, decl_flags, fout );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
