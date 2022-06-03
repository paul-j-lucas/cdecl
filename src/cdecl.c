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
PJL_WARN_UNUSED_RESULT
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

PJL_WARN_UNUSED_RESULT
static int  cdecl_parse_argv( int, char const *const[] ),
            cdecl_parse_command_line( char const*, int, char const *const[] ),
            cdecl_parse_stdin( void );

PJL_WARN_UNUSED_RESULT
static bool is_command( char const*, cdecl_command_kind_t ),
            read_conf_file( char const* ),
            starts_with_token( char const*, char const*, size_t );

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
  perror_exit_if( atexit( &cdecl_cleanup ) != 0, EX_OSERR );
  cli_options_init( &argc, &argv );
  c_typedef_init();
  lexer_reset( /*hard_reset=*/true );   // resets line number
  if ( opt_read_conf )
    conf_init();
  cdecl_initialized = true;
  exit( cdecl_parse_argv( argc, argv ) );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Cleans up cdecl data.
 */
static void cdecl_cleanup( void ) {
  free_now();
  parser_cleanup();                     // must go before c_ast_cleanup()
  c_ast_cleanup();
}

/**
 * Parses \a argv to figure out what kind of arguments were given.
 * If:
 *
 *  + There are zero arguments, parses stdin; else:
 *  + The program's name is one of `cast`, `declare`, or `explain`, parses the
 *    command line; else:
 *  + If the first argument is a cdecl command, parses the command line; else:
 *  + If \ref opt_explain is `true`, parses the command line; else:
 *  + Prints an error message.
 *
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns `EX_OK` upon success or another value upon failure.
 */
PJL_WARN_UNUSED_RESULT
static int cdecl_parse_argv( int argc, char const *const argv[const] ) {
  if ( argc == 0 )                      // cdecl
    return cdecl_parse_stdin();
  if ( is_command( me, CDECL_COMMAND_PROG_NAME ) )
    return cdecl_parse_command_line( /*command=*/me, argc, argv );

  //
  // Note that cli_options_init() adjusts argv such that argv[0] becomes the
  // first argument (and no longer the program name).
  //
  if ( is_command( argv[0], CDECL_COMMAND_FIRST_ARG ) )
    return cdecl_parse_command_line( /*command=*/NULL, argc, argv );

  if ( opt_explain )
    return cdecl_parse_command_line( L_EXPLAIN, argc, argv );

  EPRINTF( "%s: \"%s\": invalid command", me, argv[0] );
  if ( print_suggestions( DYM_COMMANDS, argv[0] ) )
    EPUTC( '\n' );
  else
    print_use_help();
  return EX_USAGE;
}

/**
 * Parses a cdecl command from the command-line.
 *
 * @param command The value of main()'s `argv[0]` if it's a cdecl command; NULL
 * otherwise and `argv[1]` is a cdecl command.
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns `EX_OK` upon success or another value upon failure.
 */
PJL_WARN_UNUSED_RESULT
static int cdecl_parse_command_line( char const *command, int argc,
                                     char const *const argv[const] ) {
  strbuf_t sbuf;
  bool space;

  strbuf_init( &sbuf );
  if ( (space = command != NULL) )
    strbuf_puts( &sbuf, command );
  for ( int i = 0; i < argc; ++i )
    strbuf_sepc_puts( &sbuf, ' ', &space, argv[i] );

  int const status = cdecl_parse_string( sbuf.str, sbuf.len );
  strbuf_cleanup( &sbuf );
  return status;
}

/**
 * Parses cdecl commands from \a fin.
 *
 * @param fin The `FILE` to read from.
 * @param fout The `FILE` to write the prompts to, if any.
 * @param return_on_error If `true`, return immediately upon encountering an
 * error; if `false`, return only upon encountering EOF.
 * @return Returns `EX_OK` upon success of the last line read or another value
 * upon failure.
 */
PJL_WARN_UNUSED_RESULT
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
 * Parses cdecl commands from standard input.
 *
 * @return Returns `EX_OK` upon success or another value upon failure.
 */
PJL_WARN_UNUSED_RESULT
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
 * Checks whether \a s is a cdecl command.
 *
 * @param s The null-terminated string to check.
 * @param command_kind The kind of commands to check against.
 * @return Returns `true` only if \a s is a command.
 */
PJL_WARN_UNUSED_RESULT
static bool is_command( char const *s, cdecl_command_kind_t command_kind ) {
  assert( s != NULL );
  SKIP_WS( s );

  FOREACH_CDECL_COMMAND( c ) {
    if ( c->kind < command_kind )
      continue;
    size_t const literal_len = strlen( c->literal );
    if ( !starts_with_token( s, c->literal, literal_len ) )
      continue;
    if ( c->literal == L_CONST || c->literal == L_STATIC ) {
      //
      // When in explain-by-default mode, a special case has to be made for
      // const and static since explain is implied only when NOT followed by
      // "cast":
      //
      //      const int *p                      // Implies explain.
      //      const cast p into pointer to int  // Does NOT imply explain.
      //
      char const *p = s + literal_len;
      if ( !isspace( *p ) )
        break;
      SKIP_WS( p );
      if ( !starts_with_token( p, L_CAST, 4 ) )
        break;
      p += 4;
      if ( !isspace( *p ) )
        break;
    }
    return true;
  } // for
  return false;
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

/**
 * Checks whether \a s starts with a token.  If so, the character following the
 * token in \a s also _must not_ be an identifier character, i.e., whitespace,
 * punctuation, or the null byte.
 *
 * @param s The null-terminated string to check.
 * @param token The token to check against.
 * @param token_len The length of \a token.
 * @return Returns `true` only if \a s starts with \a token.
 */
PJL_WARN_UNUSED_RESULT
static bool starts_with_token( char const *s, char const *token,
                               size_t token_len ) {
  assert( s != NULL );
  assert( token != NULL );
  return  strncmp( s, token, token_len ) == 0 &&
          !is_ident( token[ token_len ] );
}

////////// extern functions ///////////////////////////////////////////////////

int cdecl_parse_string( char const *s, size_t s_len ) {
  assert( s != NULL );

  // The code in print.c relies on command_line being set, so set it.
  print_params.command_line = s;
  print_params.command_line_len = s_len;

  strbuf_t explain_buf;
  bool const insert_explain =
    opt_explain && !is_command( s, CDECL_COMMAND_ANYWHERE );

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

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
