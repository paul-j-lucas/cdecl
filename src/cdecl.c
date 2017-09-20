/*
**      cdecl -- C gibberish translator
**      src/cdecl.c
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
 * Defines \c main() as well as functions for initialization, clean-up, and
 * parsing user input.
 */

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "color.h"
#include "lang.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "prompt.h"
#include "typedefs.h"
#include "util.h"

// standard
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>                     /* for isatty(3) */

///////////////////////////////////////////////////////////////////////////////

// extern variable definitions
c_mode_t            c_mode;             // are we declaring or explaining?
char const         *command_line;       // command from command line, if any
size_t              command_line_len;   // length of command_line
bool                is_input_a_tty;     // is our input from a tty?
char const         *me;                 // program name

// extern functions
extern bool         parse_string( char const*, size_t );
extern int          yyparse( void );
extern void         yyrestart( FILE* );

// local functions
static void         cdecl_cleanup( void );
static bool         is_command( char const* );
static bool         parse_argv( int, char const*[] );
static bool         parse_command_line( char const*, int, char const*[] );
static bool         parse_files( int, char const*[] );
static bool         parse_stdin( void );
static void         read_conf_file( void );

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const **argv ) {
  atexit( cdecl_cleanup );
  options_init( &argc, &argv );

  c_typedef_init();
  lexer_reset( true );                  // resets line number

  if ( !opt_no_conf )
    read_conf_file();
  opt_conf_file = NULL;                 // don't print in errors any more

  bool const ok = parse_argv( argc, argv );
  exit( ok ? EX_OK : EX_DATAERR );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether \a s is a cdecl command: cast, declare, explain, etc.
 *
 * @param s The null-terminated string to check.
 * @return Returns \c true only if \a s is a command.
 */
static bool is_command( char const *s ) {
  static char const *const COMMANDS[] = {
    L_CAST,
    L_CONST,                            // cast ...
    L_DECLARE,
    L_DYNAMIC,                          // cast ...
    L_EXPLAIN,
    L_HELP,
    L_REINTERPRET,                      // cast ...
    L_STATIC,                           // cast ...
    NULL
  };
  for ( char const *const *c = COMMANDS; *c; ++c )
    if ( strcmp( *c, s ) == 0 )
      return true;
  return false;
}

/**
 * Cleans up cdecl data.
 */
static void cdecl_cleanup( void ) {
  free_now();
  c_typedef_cleanup();                  // must go before c_ast_cleanup()
  c_ast_cleanup();
}

/**
 * Parses argv to figure out what kind of arguments were given.
 *
 * @param argc The length of \a argv.
 * @param argv The command-line arguments.
 * @return Returns \c true only upon success.
 */
static bool parse_argv( int argc, char const *argv[] ) {
  if ( argc == 0 )                      // cdecl
    return parse_stdin();
  if ( is_command( me ) )               // {cast|declare|explain} arg ...
    return parse_command_line( me, argc, argv );
  if ( is_command( argv[0] ) )          // cdecl {cast|declare|explain} arg ...
    return parse_command_line( NULL, argc, argv );

  // cdecl '{cast|declare|explain} arg ...'
  char *remainder = CONST_CAST( char*, argv[0] );
  char *first_word = strsep( &remainder, " \t" );
  if ( first_word != NULL && is_command( first_word ) ) {
    char const *const command = check_strdup( first_word );
    while ( (*first_word++ = *remainder++) )
      ;
    return parse_command_line( command, argc, argv );
  }

  return parse_files( argc, argv );
}

/**
 * Parses a cdecl command from the command-line.
 *
 * @param command The value main()'s \c argv[0] if it is a cdecl command; null
 * otherwise and \c argv[1] is a cdecl command.
 * @param argc The length of \a argv.
 * @param argv The command-line arguments.
 * @return Returns \c true only upon success.
 */
static bool parse_command_line( char const *command, int argc,
                                char const *argv[] ) {
  bool space;                           // need to output a space?

  // pre-flight to calc command_line_len
  command_line_len = 0;
  if ( (space = command != NULL) )
    command_line_len += strlen( command );
  for ( int i = 0; i < argc; ++i )
    command_line_len += true_or_set( &space ) + strlen( argv[i] );

  command_line = MALLOC( char, command_line_len + 1/*null*/ );
  char *s = (char*)command_line;

  // build cdecl command
  if ( (space = command != NULL) )
    s += strcpy_len( s, command );
  for ( int i = 0; i < argc; ++i ) {
    if ( true_or_set( &space ) )
      *s++ = ' ';
    s += strcpy_len( s, argv[i] );
  } // for

  bool const ok = parse_string( command_line, command_line_len );
  FREE( command_line );
  command_line = NULL;
  command_line_len = 0;
  return ok;
}

/**
 * Parses one or more files.
 *
 * @param num_files The length of \a files.
 * @param files An array of file names.
 * @return Returns \c true only upon success.
 */
static bool parse_files( int num_files, char const *files[] ) {
  bool ok = true;

  for ( int i = 0; i < num_files && ok; ++i ) {
    if ( strcmp( files[i], "-" ) == 0 )
      ok = parse_stdin();
    else {
      FILE *const fin = fopen( files[i], "r" );
      if ( fin == NULL )
        PMESSAGE_EXIT( EX_NOINPUT, "%s: %s\n", files[i], STRERROR );
      yyrestart( fin );
      ok = yyparse() == 0;
      fclose( fin );
    }
  } // for
  return ok;
}

/**
 * Parses standard input.
 *
 * @return Returns \c true only upon success.
 */
static bool parse_stdin( void ) {
  bool ok;

  is_input_a_tty = isatty( fileno( fin ) );

  if ( is_input_a_tty || opt_interactive ) {
    if ( !opt_quiet )
      FPRINTF( fout, "Type \"%s\" or \"?\" for help\n", L_HELP );
    ok = true;
    for ( char *line; (line = read_input_line( prompt[0], prompt[1] )); )
      ok = parse_string( line, 0 );
  } else {
    yyrestart( fin );
    ok = yyparse() == 0;
  }

  is_input_a_tty = false;
  return ok;
}

/**
 * Parses a string.
 *
 * @param s The null-terminated string to parse.
 * @param s_len the length of \a s.
 * @return Returns \c true only upon success.
 */
bool parse_string( char const *s, size_t s_len ) {
  if ( s_len == 0 )
    s_len = strlen( s );
  if ( command_line == NULL ) {
    command_line = s;
    command_line_len = s_len;
  }
  FILE *const temp = fmemopen( CONST_CAST( void*, s ), s_len, "r" );
  yyrestart( temp );
  bool const ok = yyparse() == 0;
  fclose( temp );
  return ok;
}

/**
 * Reads the configuration file, if any.
 */
static void read_conf_file( void ) {
  static char conf_path_buf[ PATH_MAX ];
  bool const explicit_conf_file = (opt_conf_file != NULL);

  if ( !explicit_conf_file ) {
    char const *const home = home_dir();
    if ( !home )
      return;
    strcpy( conf_path_buf, home );
    path_append( conf_path_buf, CONF_FILE_NAME );
    opt_conf_file = conf_path_buf;
  }

  FILE *const cin = fopen( opt_conf_file, "r" );
  if ( !cin ) {
    if ( explicit_conf_file )
      PMESSAGE_EXIT( EX_NOINPUT, "%s: %s\n", opt_conf_file, STRERROR );
    return;
  }

  yyrestart( cin );
  (void)yyparse();
  fclose( cin );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
