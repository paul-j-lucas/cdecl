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
 * Defines `main()` as well as functions for initialization, clean-up, and
 * parsing user input.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "c_ast.h"
#include "c_lang.h"
#include "c_typedef.h"
#include "color.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "prompt.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

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

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// extern variable definitions
c_init_t            c_init;             // initialization state
c_mode_t            c_mode;             // parsing english or gibberish?
char const         *command_line;       // command from command line, if any
size_t              command_line_len;   // length of command_line
bool                is_input_a_tty;     // is our input from a tty?
char const         *me;                 // program name

// extern functions
extern bool         parse_string( char const*, size_t );
extern void         parser_cleanup( void );
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

/**
 * The main entry point.
 *
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns 0 on success, non-zero on failure.
 */
int main( int argc, char const **argv ) {
  atexit( cdecl_cleanup );
  options_init( &argc, &argv );

  c_typedef_init();
  lexer_reset( true );                  // resets line number

  if ( !opt_no_conf )
    read_conf_file();
  opt_conf_file = NULL;                 // don't print in errors any more
  c_init = INIT_READ_CONF;

  // ...

  c_init = INIT_DONE;
  bool const ok = parse_argv( argc, argv );
  exit( ok ? EX_OK : EX_DATAERR );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether \a s is a cdecl command: `cast`, `declare`, `explain`, etc.
 *
 * @param s The null-terminated string to check.
 * @return Returns `true` only if \a s is a command.
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
  for ( char const *const *c = COMMANDS; *c != NULL; ++c )
    if ( strcmp( *c, s ) == 0 )
      return true;
  return false;
}

/**
 * Cleans up cdecl data.
 */
static void cdecl_cleanup( void ) {
  free_now();
  c_typedef_cleanup();
  parser_cleanup();                     // must go before c_ast_cleanup()
  c_ast_cleanup();
}

/**
 * Parses \a argv to figure out what kind of arguments were given.
 *
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns `true` only upon success.
 */
static bool parse_argv( int argc, char const *argv[] ) {
  if ( argc == 0 )                      // cdecl
    return parse_stdin();
  if ( is_command( me ) )               // {cast|declare|explain} arg ...
    return parse_command_line( me, argc, argv );
  if ( is_command( argv[0] ) )          // cdecl {cast|declare|explain} arg ...
    return parse_command_line( NULL, argc, argv );

  // cdecl '{cast|declare|explain} arg ...'
  char *argv0 = CONST_CAST( char*, argv[0] );
  for ( char *first_word; (first_word = strsep( &argv0, " \t" )) != NULL; ) {
    if ( unlikely( first_word[0] == '\0' ) ) {
      //
      // Leading whitespace in a quoted argument, e.g.:
      //
      //      cdecl ' declare x as int'
      //             ^
      //
      // results in an "empty field": skip it.
      //
      continue;
    }

    if ( is_command( first_word ) ) {
      //
      // Now that we've split off the command, set the argument to the
      // remaining characters.  E.g., given:
      //
      //      cdecl 'declare x as int'
      //
      // set argv[0] to "x as int" by changing the pointer.
      //
      argv[0] = argv0;
      return parse_command_line( first_word, argc, argv );
    }

    break;                              // nothing special about first argument
  } // for

  return parse_files( argc, argv );     // assume arguments are file names
}

/**
 * Parses a cdecl command from the command-line.
 *
 * @param command The value `main()`'s `argv[0]` if it is a cdecl command; null
 * otherwise and `argv[1]` is a cdecl command.
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns `true` only upon success.
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
    s = strcpy_end( s, command );
  for ( int i = 0; i < argc; ++i ) {
    if ( true_or_set( &space ) )
      *s++ = ' ';
    s = strcpy_end( s, argv[i] );
  } // for

  bool const ok = parse_string( command_line, command_line_len );
  FREE( command_line );
  command_line = NULL;
  command_line_len = 0;
  return ok;
}

/**
 * Parses a file.
 *
 * @param fin The FILE to read from.
 * @return Returns `true` only upon success.
 */
static bool parse_file( FILE *fin ) {
  bool ok = true;

  for ( char buf[ 1024 ]; fgets( buf, sizeof buf, fin ) != NULL; ) {
    if ( !parse_string( buf, 0 ) )
      ok = false;
  } // for
  FERROR( fin );

  return ok;
}

/**
 * Parses one or more files.
 *
 * @param num_files The length of \a files.
 * @param files An array of file names.
 * @return Returns `true` only upon success.
 */
static bool parse_files( int num_files, char const *files[] ) {
  bool ok = true;

  for ( int i = 0; i < num_files && ok; ++i ) {
    if ( strcmp( files[i], "-" ) == 0 )
      ok = parse_stdin();
    else {
      FILE *const fin = fopen( files[i], "r" );
      if ( unlikely( fin == NULL ) )
        PMESSAGE_EXIT( EX_NOINPUT, "%s: %s\n", files[i], STRERROR );
      if ( !parse_file( fin ) )
        ok = false;
      fclose( fin );
    }
  } // for
  return ok;
}

/**
 * Parses standard input.
 *
 * @return Returns `true` only upon success.
 */
static bool parse_stdin( void ) {
  bool ok = true;
  is_input_a_tty = isatty( fileno( fin ) );

  if ( is_input_a_tty || opt_interactive ) {
    if ( !opt_quiet )
      FPRINTF( fout, "Type \"%s\" or \"?\" for help\n", L_HELP );
    ok = true;
    for ( char *line; (line = read_input_line( prompt[0], prompt[1] )); )
      ok = parse_string( line, 0 );
  } else {
    ok = parse_file( fin );
  }

  is_input_a_tty = false;
  return ok;
}

/**
 * Parses a string.
 *
 * @param s The null-terminated string to parse.
 * @param s_len The length of \a s.  If 0, it will be calculated.
 * @return Returns `true` only upon success.
 */
bool parse_string( char const *s, size_t s_len ) {
  if ( s_len == 0 )
    s_len = strlen( s );

  bool reset_command_line = false;
  if ( command_line == NULL ) {
    //
    // The diagnostics code relies on command_line being set, so set it.
    //
    command_line = s;
    command_line_len = s_len;
    reset_command_line = true;
  }

  FILE *const temp = fmemopen( CONST_CAST( void*, s ), s_len, "r" );
  yyrestart( temp );
  bool const ok = yyparse() == 0;
  fclose( temp );

  if ( reset_command_line ) {
    command_line = NULL;
    command_line_len = 0;
  }
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
    if ( home == NULL )
      return;
    strcpy( conf_path_buf, home );
    path_append( conf_path_buf, CONF_FILE_NAME_DEFAULT );
    opt_conf_file = conf_path_buf;
  }

  FILE *const cin = fopen( opt_conf_file, "r" );
  if ( unlikely( cin == NULL ) ) {
    if ( explicit_conf_file )
      PMESSAGE_EXIT( EX_NOINPUT, "%s: %s\n", opt_conf_file, STRERROR );
    return;
  }

  //
  // Before reading the configuration file, temporarily set the language to the
  // maximum supported C++ so "using" declarations, if any, won't cause the
  // parser to error out.
  //
  c_lang_id_t const orig_lang = opt_lang;
  opt_lang = LANG_CPP_NEW;
  (void)parse_file( cin );
  opt_lang = orig_lang;

  fclose( cin );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
