/*
**      cdecl -- C gibberish translator
**      src/diagnostics.c
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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
#include "config.h"                     /* must go first */
#include "color.h"
#include "diagnostics.h"
#include "lexer.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/// @endcond

/// @cond DOXYGEN_IGNORE

// local constants
static char const*    MORE[]     = { "...", "..." };
static size_t const   MORE_LEN[] = { 3,     3 };
static unsigned const TERM_COLUMNS_DEFAULT = 80;

/// @endcond

// local functions
static size_t       token_len( char const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints the error line (if not interactive) and a `^` (in color, if possible
 * and requested) under the offending token.
 *
 * @param error_column The zero-based column of the offending token.
 */
static void print_caret( size_t error_column ) {
#ifdef ENABLE_TERM_COLUMNS
  unsigned term_columns = get_term_columns();
  if ( term_columns == 0 )
    term_columns = TERM_COLUMNS_DEFAULT;
#else
  unsigned term_columns = 0;
#endif /* ENABLE_TERM_COLUMNS */

  size_t error_column_term = error_column;

  if ( is_input_a_tty || opt_interactive ) {
    //
    // If we're interactive, we can put the ^ under the already existing token
    // the user typed for the recent command, but we have to add the length of
    // the prompt.
    //
    error_column_term += strlen( opt_lang >= LANG_CPP_MIN ? CPPDECL : PACKAGE )
      + 2 /* "> " */;
    if ( term_columns )
      error_column_term %= term_columns;
  } else {
    --term_columns;                     // more aesthetically pleasing
    //
    // Otherwise we have to print out the line containing the error then put
    // the ^ under that.
    //
    size_t input_line_len;
    char const *input_line = lexer_input_line( &input_line_len );
    assert( input_line != NULL );
    if ( input_line_len == 0 ) {        // no input? try command line
      input_line = command_line;
      assert( input_line != NULL );
      input_line_len = command_line_len;
    }
    assert( error_column <= input_line_len );

    //
    // Chop off a newline (if any) so we can always print one ourselves.
    //
    if ( ends_with_chr( input_line, input_line_len, '\n' ) )
      --input_line_len;

    //
    // If the error is due to unexpected end of input, back up the error column
    // so it refers to a non-null character.
    //
    if ( error_column > 0 && input_line[ error_column ] == '\0' )
      --error_column;

    size_t const    token_columns = token_len( input_line + error_column );
    unsigned const  error_end_column = error_column + token_columns - 1;

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
    // However, if there is "more" on the right but end of the error token is
    // at the end of the line, then we can print through the end of the line
    // without any "more."
    //
    if ( more[1] ) {
      if ( error_end_column < input_line_len - 1 )
        print_columns -= MORE_LEN[1];
      else
        more[1] = false;
    }

    //
    // If there is "more" on the left, we have to adjust the error column, the
    // offset into the input line that we start printing at, and the number of
    // printable columns to give the appearance that the input line has been
    // "scrolled" to the left.
    //
    size_t print_offset;                // offset into input_line to print from
    if ( more[0] ) {
      error_column_term = print_columns - token_columns;
      print_offset = MORE_LEN[0] + (error_column - error_column_term);
      print_columns -= MORE_LEN[0];
    } else {
      print_offset = 0;
    }

    PRINT_ERR( "%s%.*s%s\n",
      (more[0] ? MORE[0] : ""),
      (int)print_columns, input_line + print_offset,
      (more[1] ? MORE[1] : "")
    );
  }

  PRINT_ERR( "%*s", (int)error_column_term, "" );
  SGR_START_COLOR( stderr, caret );
  PUTC_ERR( '^' );
  SGR_END_COLOR( stderr );
  PUTC_ERR( '\n' );
}

/**
 * Gets the length of the first token in \a s.  Characters are divided into
 * three classes:
 *
 *  + Whitespace.
 *  + Alpha-numeric.
 *  + Everything else (e.g., punctuation).
 *
 * A token is composed of characters in exclusively one class.  The class is
 * determined by `s[0]`.  The length of the token is the number of consecutive
 * characters of the same class starting at `s[0]`.
 *
 * @param s The null-terminated string to use.
 * @return Returns the length of the token.
 */
static size_t token_len( char const *s ) {
  char const *const s0 = s;
  bool const is_s0_space = isspace( *s0 );
  bool const is_s0_alnum = isalnum( *s0 );
  for ( ++s; *s != '\0'; ++s ) {
    if ( is_s0_space ) {
      if ( !isspace( *s ) )
        break;
    }
    else if ( is_s0_alnum ) {
      if ( !isalnum( *s ) )
        break;
    }
    else {
      if ( isalnum( *s ) || isspace( *s ) )
        break;
    }
  } // for
  return s - s0;
}

////////// extern functions ///////////////////////////////////////////////////

void print_error( c_loc_t const *loc, char const *format, ... ) {
  print_loc( loc );
  SGR_START_COLOR( stderr, error );
  PUTS_ERR( "error" );
  SGR_END_COLOR( stderr );
  PUTS_ERR( ": " );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );

  PUTC_ERR( '\n' );
}

void print_hint( char const *format, ... ) {
  PUTS_ERR( "\t(did you mean " );
  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );
  PUTS_ERR( "?)\n" );
}

void print_loc( c_loc_t const *loc ) {
  assert( loc );
  print_caret( loc->first_column );
  SGR_START_COLOR( stderr, locus );
  if ( opt_conf_file )
    PRINT_ERR( "%s:%d,", opt_conf_file, loc->first_line + 1 );
  PRINT_ERR( "%d", loc->first_column + 1 );
  SGR_END_COLOR( stderr );
  PUTS_ERR( ": " );
}

void print_warning( c_loc_t const *loc, char const *format, ... ) {
  print_loc( loc );
  SGR_START_COLOR( stderr, warning );
  PUTS_ERR( "warning" );
  SGR_END_COLOR( stderr );
  PUTS_ERR( ": " );

  va_list args;
  va_start( args, format );
  vfprintf( stderr, format, args );
  va_end( args );

  PUTC_ERR( '\n' );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
