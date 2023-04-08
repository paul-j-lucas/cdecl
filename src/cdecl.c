/*
**      cdecl -- C gibberish translator
**      src/cdecl.c
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
 * Defines main() as well as functions for initialization, clean-up, and
 * parsing user input.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cdecl.h"
#include "c_ast.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_typedef.h"
#include "cdecl_command.h"
#include "cdecl_keyword.h"
#include "cdecl_parser.h"
#include "cli_options.h"
#include "color.h"
#include "help.h"
#include "lexer.h"
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
#include <unistd.h>                     /* for isatty(3) */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// extern variable definitions
bool          cdecl_initialized;
bool          cdecl_interactive;
cdecl_mode_t  cdecl_mode;
char const   *me;

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
  c_keyword_init();
  cdecl_keyword_init();
  cli_option_init( &argc, &argv );
  lexer_init();
  c_typedef_init();
  lexer_reset( /*hard_reset=*/true );   // resets line number
  if ( opt_read_conf )
    conf_init();
  cdecl_initialized = true;
  //
  // Note that cli_option_init() adjusts argv such that argv[0] becomes the
  // first argument, if any, and no longer the program name.
  //
  exit( cdecl_parse_cli( STATIC_CAST( size_t, argc ), argv ) );
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

/**
 * Parses a **cdecl** command.
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

  while ( strbuf_read_line( &sbuf, CDECL, fin, fout, cdecl_prompt ) ) {
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
  cdecl_interactive = isatty( STDIN_FILENO );
  if ( cdecl_interactive && opt_prompt )
    PUTS( "Type \"help\" or \"?\" for help\n" );
  return cdecl_parse_file( stdin, stdout, /*return_on_error=*/false );
}

/**
 * Reads the configuration file, if any.
 * In priority order:
 *
 *  1. Either the `--config` or `-c` command-line option; or:
 *  2. The value of the `CDECLRC` environment variable; or:
 *  3. `~/.cdeclrc`
 *
 * @note This function must be called as most once.
 */
static void conf_init( void ) {
  ASSERT_RUN_ONCE();

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
  PERROR_EXIT_IF( temp_file == NULL, EX_IOERR );
  yyrestart( temp_file );

  if ( opt_echo_commands && !cdecl_interactive && cdecl_initialized ) {
    //
    // Echo the original command (without "explain" possibly having been
    // inserted) without a trailing newline (if any) so we can always print a
    // newline ourselves -- but don't touch the original command line.
    //
    size_t echo_len = print_params.command_line_len;
    strn_rtrim( print_params.command_line, &echo_len );
    PRINTF(
      "%s%.*s\n",
      cdecl_prompt[0], STATIC_CAST( int, echo_len ), print_params.command_line
    );
    FFLUSH( stdout );
  }

  int const status = yyparse() == 0 ? EX_OK : EX_DATAERR;
  PJL_IGNORE_RV( fclose( temp_file ) );

  if ( insert_explain ) {
    strbuf_cleanup( &explain_buf );
    print_params.inserted_len = 0;
  }

  return status;
}

bool is_cppdecl( void ) {
  static char const *const NAMES[] = {
    CPPDECL,
    "cppdecl",
    "cxxdecl",
    NULL
  };

  for ( char const *const *pname = NAMES; *pname != NULL; ++pname ) {
    if ( strcmp( *pname, me ) == 0 )
      return true;
  } // for
  return false;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
