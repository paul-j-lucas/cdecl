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

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "color.h"
#include "literals.h"
#include "options.h"
#include "prompt.h"
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
bool                is_input_a_tty;     // is our input from a tty?
char const         *me;                 // program name

// extern functions
extern int          yyparse( void );
extern void         yyrestart( FILE* );

// local functions
static void         cdecl_cleanup( void );
static bool         is_command( char const* );
static bool         parse_command_line( char const*, int, char const*[] );
static bool         parse_files( int, char const*[] );
static bool         parse_stdin( void );
static bool         parse_string( char const*, size_t );

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const **argv ) {
  atexit( cdecl_cleanup );
  options_init( &argc, &argv );
  is_input_a_tty = isatty( fileno( fin ) );
  cdecl_prompt_init();

  bool ok;
  if ( argc == 0 )                      // cdecl
    ok = parse_stdin();
  else if ( is_command( me ) )          // {cast|declare|explain} arg ...
    ok = parse_command_line( me, argc, argv );
  else if ( is_command( argv[0] ) )     // cdecl {cast|declare|explain} arg ...
    ok = parse_command_line( NULL, argc, argv );
  else
    ok = parse_files( argc, argv );

  exit( ok ? EX_OK : EX_DATAERR );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether \a s is a cdecl command: cast, declare, or explain.
 *
 * @param s The null-terminated string to check.
 * @return Returns \c true only if \a s is a command.
 */
static bool is_command( char const *s ) {
  static char const *const COMMANDS[] = {
    L_CAST,
    L_DECLARE,
    L_EXPLAIN,
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
  c_ast_cleanup();
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
  size_t buf_len = 0;
  bool space;                           // need to output a space?

  // pre-flight to calc buf_len
  if ( (space = command != NULL) )
    buf_len += strlen( command );
  for ( int i = 0; i < argc; ++i )
    buf_len += true_or_set( &space ) + strlen( argv[i] );

  char *const buf = (char*)free_later( malloc( buf_len + 1/*null*/ ) );
  char *s = buf;

  // build cdecl command
  if ( (space = command != NULL) )
    s += strcpy_len( buf, command );
  for ( int i = 0; i < argc; ++i ) {
    if ( true_or_set( &space ) )
      *s++ = ' ';
    s += strcpy_len( s, argv[i] );
  } // for

  return parse_string( buf, buf_len );
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

  if ( is_input_a_tty || opt_interactive ) {
    if ( !opt_quiet )
      printf( "Type \"%s\" or \"?\" for help\n", L_HELP );
    ok = true;
    for ( char *line; (line = readline_wrapper( prompt[0], prompt[1] )); )
      ok = parse_string( line, strlen( line ) );
  } else {
    yyrestart( fin );
    ok = yyparse() == 0;
    is_input_a_tty = false;
  }
  return ok;
}

/**
 * Parses a string.
 *
 * @param s The null-terminated string to parse.
 * @param s_len the length of \a s.
 * @return Returns \c true only upon success.
 */
static bool parse_string( char const *s, size_t s_len ) {
  FILE *const temp = fmemopen( CONST_CAST( void*, s ), s_len, "r" );
  yyrestart( temp );
  bool const ok = yyparse() == 0;
  fclose( temp );
  return ok;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
