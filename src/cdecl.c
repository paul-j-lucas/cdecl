/*
**      cdecl -- C gibberish translator
**      src/cdecl.c
**
**      Copyright (C) 2017-2022  Paul J. Lucas, et al.
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
 * Defines main() as well as functions for initialization, clean-up, and
 * parsing user input.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cdecl.h"
#include "c_ast.h"
#include "c_lang.h"
#include "c_typedef.h"
#include "cdecl_command.h"
#include "cli_options.h"
#include "color.h"
#include "help.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "prompt.h"
#include "read_line.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// extern variable definitions
FILE         *cdecl_fin;
FILE         *cdecl_fout;
bool          cdecl_initialized;
cdecl_mode_t  cdecl_mode;
char const   *me;

// extern functions
extern void parser_cleanup( void );

/**
 * Bison: parse input.
 *
 * @return Returns 0 only if parsing was successful.
 */
NODISCARD
extern int  yyparse( void );

/**
 * Flex: immediately switch to reading \a file.
 *
 * @param in_file The `FILE` to read from.
 */
extern void yyrestart( FILE *in_file );

// local functions
static void cdecl_cleanup( void );
static void conf_init( void );

NODISCARD
static int  cdecl_parse_cli( size_t, char const *const[] ),
            cdecl_parse_command( char const*, size_t, char const *const[] ),
            cdecl_parse_stdin( void );

NODISCARD
static bool read_conf_file( char const* );

////////// main ///////////////////////////////////////////////////////////////

/**
 * The main entry point.
 *
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns 0 on success, non-zero on failure.
 */
int main( int argc, char const *argv[] ) {
  me = base_name( argv[0] );
  check_atexit( &cdecl_cleanup );
  cli_options_init( &argc, &argv );
  lexer_init();
  c_typedef_init();
  lexer_reset( /*hard_reset=*/true );   // resets line number
  if ( opt_read_conf )
    conf_init();
  cdecl_initialized = true;
  //
  // Note that cli_options_init() adjusts argv such that argv[0] becomes the
  // first argument, if any, and no longer the program name.
  //
  exit( cdecl_parse_cli( INTEGER_CAST( size_t, argc ), argv ) );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Cleans up **cdecl** data.
 */
static void cdecl_cleanup( void ) {
  free_now();
  parser_cleanup();                     // must go before c_ast_cleanup()
  c_ast_cleanup();
}

/**
 * Parses the command-line.
 *
 * @param cli_count The size of \a cli_value.
 * @param cli_value The command-line argument values, if any.  Note that,
 * unlike `main()`'s `argv`, this contains _only_ the command-line arguments
 * _after_ the program name.

 * @return Returns `EX_OK` upon success or another value upon failure.
 *
 * @note The parameters are _not_ named `argc` and `argv` intentionally to
 * avoid confusion since they're not the same.
 */
NODISCARD
static int cdecl_parse_cli( size_t cli_count,
                            char const *const cli_value[const] ) {
  char const *command_literal = NULL;
  char const *invalid_when = "";

  if ( is_cdecl( me ) || is_cppdecl( me ) )
    goto parse_command;

  //
  // Is the program name itself a command, i.e., cast, declare, or explain?
  //
  char const *find_what = me;
  cdecl_command_t const *found_command = cdecl_command_find( find_what );
  if ( found_command != NULL ) {
    switch ( found_command->kind ) {
      case CDECL_COMMAND_FIRST_ARG:
      case CDECL_COMMAND_LANG_ONLY:
        invalid_when = " (as a program name)";
        goto invalid_command;
      case CDECL_COMMAND_PROG_NAME:
        command_literal = me;
        goto parse_command;
    } // switch
  }

  if ( cli_count > 0 ) {
    //
    // Is the first word of the first argument a command?
    //
    find_what = cli_value[0];
    found_command = cdecl_command_find( find_what );
    if ( found_command != NULL ) {
      switch ( found_command->kind ) {
        case CDECL_COMMAND_FIRST_ARG:
        case CDECL_COMMAND_PROG_NAME:
          goto parse_command;
        case CDECL_COMMAND_LANG_ONLY:
          invalid_when = " (as a first argument)";
          goto invalid_command;
      } // switch
    }
  }

  if ( opt_explain )
    command_literal = L_EXPLAIN;

parse_command:
  return cdecl_parse_command( command_literal, cli_count, cli_value );

invalid_command:
  assert( find_what != NULL );
  EPRINTF( "%s: \"%s\": invalid command%s", me, find_what, invalid_when );
  if ( found_command == NULL && print_suggestions( DYM_COMMANDS, find_what ) )
    EPUTC( '\n' );
  else
    print_use_help();
  return EX_USAGE;
}

/**
 * Parses a **cdecl** command.
 *
 * @param command The **cdecl** command to parse, but only if its \ref
 * cdecl_command::kind "kind" is #CDECL_COMMAND_PROG_NAME; NULL otherwise.
 * @param arg_count The size of \a arg_value.
 * @param arg_value The argument values, if any.  Note that, unlike `main()`'s
 * `argv`, this contains _only_ the command-line arguments _after_ the program
 * name.
 * @return Returns `EX_OK` upon success or another value upon failure.
 */
NODISCARD
static int cdecl_parse_command( char const *command, size_t arg_count,
                                char const *const arg_value[const] ) {
  if ( command == NULL && arg_count == 0 ) // invoked as just cdecl or c++decl
    return cdecl_parse_stdin();

  strbuf_t sbuf;
  bool space;

  strbuf_init( &sbuf );
  // If command wasn't cdecl or c++decl, start the command string with it.
  if ( (space = command != NULL) )
    strbuf_puts( &sbuf, command );
  // Concatenate arguments, if any, into a single string.
  for ( size_t i = 0; i < arg_count; ++i )
    strbuf_sepc_puts( &sbuf, ' ', &space, arg_value[i] );

  int const status = cdecl_parse_string( sbuf.str, sbuf.len );
  strbuf_cleanup( &sbuf );
  return status;
}

/**
 * Parses **cdecl** commands from \a fin.
 *
 * @param fin The `FILE` to read from.
 * @param fout The `FILE` to write the prompts to, if any.
 * @param return_on_error If `true`, return immediately upon encountering an
 * error; if `false`, return only upon encountering EOF.
 * @return Returns `EX_OK` upon success of the last line read or another value
 * upon failure.
 */
NODISCARD
static int cdecl_parse_file( FILE *fin, FILE *fout, bool return_on_error ) {
  assert( fin != NULL );

  strbuf_t sbuf;
  strbuf_init( &sbuf );
  int status = EX_OK;

  while ( strbuf_read_line( &sbuf, fin, fout, cdecl_prompt ) ) {
    // We don't just call yyrestart( fin ) and yyparse() directly because
    // cdecl_parse_string() also inserts "explain " for opt_explain.
    status = cdecl_parse_string( sbuf.str, sbuf.len );
    if ( status != EX_OK && return_on_error )
      break;
    strbuf_reset( &sbuf );
  } // while

  strbuf_cleanup( &sbuf );
  return status;
}

/**
 * Parses **cdecl** commands from standard input.
 *
 * @return Returns `EX_OK` upon success or another value upon failure.
 */
NODISCARD
static int cdecl_parse_stdin( void ) {
  if ( opt_interactive && opt_prompt )
    FPUTS( "Type \"help\" or \"?\" for help\n", cdecl_fout );
  return cdecl_parse_file( cdecl_fin, cdecl_fout, /*return_on_error=*/false );
}

/**
 * Reads the configuration file, if any.
 * In priority order:
 *
 *  1. Either the `--config` or `-c` command-line option; or:
 *  2. The value of the `CDECLRC` environment variable; or:
 *  3. `~/.cdeclrc`
 */
static void conf_init( void ) {
  char const *conf_path = opt_conf_path;
  if ( conf_path == NULL )
    conf_path = null_if_empty( getenv( "CDECLRC" ) );

  strbuf_t sbuf;
  strbuf_init( &sbuf );

  if ( conf_path == NULL ) {
    char const *const home = home_dir();
    if ( home != NULL ) {
      strbuf_puts( &sbuf, home );
      strbuf_paths( &sbuf, "." CONF_FILE_NAME_DEFAULT );
      conf_path = sbuf.str;
    }
  }

  if ( conf_path != NULL ) {
    print_params.conf_path = conf_path;
    if ( !read_conf_file( conf_path ) && opt_conf_path != NULL )
      FATAL_ERR( EX_NOINPUT, "%s: %s\n", conf_path, STRERROR() );
    print_params.conf_path = NULL;
    strbuf_cleanup( &sbuf );
  }
}

/**
 * Reads the configuration file \a conf_path.
 *
 * @param conf_path The full path of the configuration file to read.
 * @return Returns `false` only if \a conf_path could not be opened for
 * reading.
 */
static bool read_conf_file( char const *conf_path ) {
  assert( conf_path != NULL );

  FILE *const conf_file = fopen( conf_path, "r" );
  if ( conf_file == NULL )
    return false;

  PJL_IGNORE_RV(
    cdecl_parse_file( conf_file, /*fout=*/NULL, /*return_on_error=*/true )
  );

  PJL_IGNORE_RV( fclose( conf_file ) );
  return true;
}

////////// extern functions ///////////////////////////////////////////////////

int cdecl_parse_string( char const *s, size_t s_len ) {
  assert( s != NULL );

  // The code in print.c relies on command_line being set, so set it.
  print_params.command_line = s;
  print_params.command_line_len = s_len;

  strbuf_t explain_buf;
  bool const insert_explain = opt_explain && cdecl_command_find( s ) == NULL;

  if ( insert_explain ) {
    //
    // The string doesn't start with a command: insert "explain " and set
    // inserted_len so the print_*() functions subtract it from the error
    // column to get the correct column within the original string.
    //
    static char const EXPLAIN_SP[] = "explain ";
    print_params.inserted_len = ARRAY_SIZE( EXPLAIN_SP ) - 1/*\0*/;
    strbuf_init( &explain_buf );
    strbuf_reserve( &explain_buf, print_params.inserted_len + s_len );
    strbuf_putsn( &explain_buf, EXPLAIN_SP, print_params.inserted_len );
    strbuf_putsn( &explain_buf, s, s_len );
    s = explain_buf.str;
    s_len = explain_buf.len;
  }

  FILE *const temp_file = fmemopen( CONST_CAST( void*, s ), s_len, "r" );
  perror_exit_if( temp_file == NULL, EX_IOERR );
  yyrestart( temp_file );
  int const status = yyparse() == 0 ? EX_OK : EX_DATAERR;
  PJL_IGNORE_RV( fclose( temp_file ) );

  if ( insert_explain ) {
    strbuf_cleanup( &explain_buf );
    print_params.inserted_len = 0;
  }

  return status;
}

bool is_cdecl( char const *prog_name ) {
  return strcmp( prog_name, CDECL ) == 0;
}

bool is_cppdecl( char const *prog_name ) {
  static char const *const NAMES[] = {
    CPPDECL,
    "cppdecl",
    "cxxdecl",
    NULL
  };

  for ( char const *const *pname = NAMES; *pname != NULL; ++pname ) {
    if ( strcmp( *pname, prog_name ) == 0 )
      return true;
  } // for
  return false;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
