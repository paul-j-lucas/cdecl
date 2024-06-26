/*
**      cdecl -- C gibberish translator
**      src/parse.c
**
**      Copyright (C) 2017-2024  Paul J. Lucas, et al.
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
 * Defines functions for parsing input.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "parse.h"
#include "cdecl.h"
#include "cdecl_command.h"
#include "cdecl_parser.h"
#include "cli_options.h"
#include "help.h"
#include "lexer.h"
#include "options.h"
#include "p_macro.h"
#include "print.h"
#include "prompt.h"
#include "read_line.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>                     /* for isatty(3) */

/// @endcond

/**
 * @addtogroup parser-api-group
 * @{
 */

// local functions
NODISCARD
static int cdecl_parse_stdin( void );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether \a s is the start of a C/C++ comment.
 *
 * @param s The string to check.
 * @return Returns `true` only if it is.
 */
NODISCARD
static bool is_c_comment( char const *s ) {
  return s[0] == '/' && (s[1] == '*' || s[1] == '/');
}

/**
 * Checks whether we're **cdecl**.
 *
 * @returns Returns `true` only if we are.
 *
 * @sa is_cppdecl()
 */
NODISCARD
static inline bool is_cdecl( void ) {
  return strcmp( me, CDECL ) == 0;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether we should infer a command based on \a s.
 *
 * @param s The string to check.
 * @return Returns `true` only if we should _not_ infer a command.
 */
NODISCARD
static bool no_infer_command( char const *s ) {
  assert( s != NULL );
  SKIP_WS( s );
  switch ( s[0] ) {
    case '\0':
      return true;
    case 'q':
      ++s;                              // 'q' must be by itself on the line
      SKIP_WS( s );
      return s[0] == '\0';
    default:
      return is_c_comment( s );
  } // switch
}

/**
 * Parses a **cdecl** command.
 *
 * @remarks If \a command is NULL and \a cli_count is 0, calls
 * cdecl_parse_stdin().
 *
 * @param command The **cdecl** command to parse, but only if its \ref
 * cdecl_command::kind "kind" is #CDECL_COMMAND_PROG_NAME; NULL otherwise.
 * @param cli_count The size of \a cli_value.
 * @param cli_value The argument values, if any.  Note that, unlike `main()`'s
 * `argv`, this contains _only_ the command-line arguments _after_ the program
 * name.
 * @return Returns `EX_OK` upon success or another value upon failure.
 */
NODISCARD
static int cdecl_parse_command( char const *command, size_t cli_count,
                                char const *const cli_value[const] ) {
  if ( command == NULL && cli_count == 0 ) // invoked as just cdecl or c++decl
    return cdecl_parse_stdin();

  strbuf_t sbuf;
  bool space;

  strbuf_init( &sbuf );
  // If command wasn't cdecl or c++decl, start the command string with it.
  if ( (space = command != NULL) )
    strbuf_puts( &sbuf, command );
  // Concatenate arguments, if any, into a single string.
  for ( size_t i = 0; i < cli_count; ++i )
    strbuf_sepc_puts( &sbuf, ' ', &space, cli_value[i] );

  int const status = cdecl_parse_string( sbuf.str, sbuf.len );
  strbuf_cleanup( &sbuf );
  return status;
}

/**
 * Parses **cdecl** commands from \a fin.
 *
 * @param fin The `FILE` to read from.
 * @param return_on_error If `true`, return immediately upon encountering an
 * error; if `false`, return only upon encountering EOF.
 * @return Returns `EX_OK` upon success of the last line read or another value
 * upon failure.
 */
NODISCARD
static int cdecl_parse_file_impl( FILE *fin, bool return_on_error ) {
  assert( fin != NULL );

  strbuf_t sbuf;
  strbuf_init( &sbuf );
  int status = EX_OK;

  while ( strbuf_read_line( &sbuf, fin, cdecl_prompt, &yylineno ) ) {
    // We don't just call yyrestart( fin ) and yyparse() directly because
    // cdecl_parse_string() may also insert a command for opt_infer_command.
    status = cdecl_parse_string( sbuf.str, sbuf.len );
    if ( status != EX_OK && return_on_error )
      break;
    strbuf_reset( &sbuf );
  } // while

  strbuf_cleanup( &sbuf );
  return status;
}

/**
 * Attempts to parse a macro name at the beginning of \a s.
 *
 * @param s The string to parse.
 * @return Returns `true` only if \a s begins with an identifier that is a
 * macro.
 */
NODISCARD
static bool cdecl_parse_macro( char const *s ) {
  assert( s != NULL );
  SKIP_WS( s );
  if ( !is_ident_first( *s ) )
    return false;
  char const *const ident = s;
  SKIP_CHARS( s, IDENT_CHARS );
  size_t const ident_len = STATIC_CAST( size_t, s - ident );

  strbuf_t sbuf;
  strbuf_init( &sbuf );
  strbuf_putsn( &sbuf, ident, ident_len );
  p_macro_t const *const macro = p_macro_find( sbuf.str );
  strbuf_cleanup( &sbuf );
  return macro != NULL;
}

/**
 * Parses **cdecl** commands from standard input until EOF.
 *
 * @return Returns `EX_OK` upon success or another value upon failure.
 */
NODISCARD
static int cdecl_parse_stdin( void ) {
  cdecl_interactive = isatty( STDIN_FILENO );
  if ( cdecl_interactive && opt_prompt )
    PUTS( "Type \"help\" or \"?\" for help\n" );
  return cdecl_parse_file_impl( stdin, /*return_on_error=*/false );
}

////////// extern functions ///////////////////////////////////////////////////

int cdecl_parse_cli( size_t cli_count, char const *const cli_value[const] ) {
  char const *command_literal = NULL;
  char const *find_what;
  cdecl_command_t const *found_command;
  char const *invalid_as;

  if ( is_cdecl() || is_cppdecl() ) {
    if ( cli_count > 0 ) {
      //
      // Is the first word of the first argument a command?
      //
      find_what = cli_value[0];
      found_command = cdecl_command_find( find_what );
      if ( found_command != NULL ) {
        if ( found_command->kind == CDECL_COMMAND_LANG_ONLY ) {
          invalid_as = "a first argument";
          goto invalid_command;
        }
      }
    }
  }
  else {
    //
    // Is the program name itself a command, i.e., cast, declare, or explain?
    //
    find_what = me;
    found_command = cdecl_command_find( find_what );
    if ( found_command != NULL ) {
      if ( found_command->kind != CDECL_COMMAND_PROG_NAME ) {
        invalid_as = "a program name";
        goto invalid_command;
      }
      command_literal = me;
    }
  }

  return cdecl_parse_command( command_literal, cli_count, cli_value );

invalid_command:
  EPRINTF( "%s: \"%s\": invalid command (as %s)", me, find_what, invalid_as );
  if ( found_command == NULL && print_suggestions( DYM_COMMANDS, find_what ) )
    EPUTC( '\n' );
  else
    print_use_help();
  return EX_USAGE;
}

int cdecl_parse_file( FILE *fin ) {
  return cdecl_parse_file_impl( fin, /*return_on_error=*/true );
}

int cdecl_parse_string( char const *s, size_t s_len ) {
  assert( s != NULL );

  // The code in print.c relies on command_line being set, so set it.
  print_params.command_line = s;
  print_params.command_line_len = s_len;

  strbuf_t sbuf;
  bool const infer_command = opt_infer_command &&
    !no_infer_command( s ) &&
    cdecl_command_find( s ) == NULL;

  if ( infer_command ) {
    //
    // The string doesn't start with either a command or a macro: insert either
    // "explain " or "expand " and set inserted_len so the print_*() functions
    // subtract it from the error column to get the correct column within the
    // original string.
    //
    char const *insert_str;
    if ( cdecl_parse_macro( s ) ) {
      static char const EXPAND_SP[] = "expand ";
      print_params.inserted_len = STRLITLEN( EXPAND_SP );
      insert_str = EXPAND_SP;
    }
    else {
      static char const EXPLAIN_SP[] = "explain ";
      print_params.inserted_len = STRLITLEN( EXPLAIN_SP );
      insert_str = EXPLAIN_SP;
    }

    strbuf_init( &sbuf );
    strbuf_reserve( &sbuf, print_params.inserted_len + s_len );
    strbuf_putsn( &sbuf, insert_str, print_params.inserted_len );
    strbuf_putsn( &sbuf, s, s_len );
    s = sbuf.str;
    s_len = sbuf.len;
  }

  FILE *const mem_file = fmemopen( CONST_CAST( void*, s ), s_len, "r" );
  PERROR_EXIT_IF( mem_file == NULL, EX_IOERR );
  yyrestart( mem_file );

  if ( opt_echo_commands && !cdecl_interactive && cdecl_initialized ) {
    //
    // Echo the original command (without "explain" or "expand" possibly having
    // been inserted) without a trailing newline (if any) so we can always
    // print a newline ourselves -- but don't touch the original command line.
    //
    size_t echo_len = print_params.command_line_len;
    strn_rtrim( print_params.command_line, &echo_len );
    PRINTF(
      "%s%.*s\n",
      cdecl_prompt[0], STATIC_CAST( int, echo_len ), print_params.command_line
    );
    FFLUSH( stdout );
  }

  int const rv = yyparse();
  fclose( mem_file );
  if ( unlikely( rv == 2 ) ) {
    //
    // Bison has already printed "memory exhausted" via yyerror() that doesn't
    // print a newline, so print one now.
    //
    // LCOV_EXCL_START
    EPUTC( '\n' );
    _Exit( EX_SOFTWARE );
    // LCOV_EXCL_STOP
  }

  if ( infer_command ) {
    strbuf_cleanup( &sbuf );
    print_params.inserted_len = 0;
  }

  return rv == 0 ? EX_OK : EX_DATAERR;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
