/*
**      cdecl -- C gibberish translator
**      src/cdecl.c
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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
#include <limits.h>                     /* for PATH_MAX */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>                     /* for isatty(3) */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * The type of cdecl command in least-to-most restrictive order.
 */
enum c_command {
  COMMAND_ANY,                          ///< Any command.
  COMMAND_FIRST_ARG,                    ///< `cdecl` _command_ _args_ ...
  COMMAND_PROG_NAME                     ///< _command_ _args_ ...
};
typedef enum c_command c_command_t;

// extern variable definitions
c_init_t            c_init;             // initialization state
c_mode_t            c_mode;             // parsing english or gibberish?
char const         *command_line;       // command from command line, if any
size_t              command_line_len;   // length of command_line
size_t              inserted_len;       // length of inserted string
bool                is_input_a_tty;     // is our input from a tty?
char const         *me;                 // program name

// extern functions
C_WARN_UNUSED_RESULT
extern bool         parse_string( char const*, size_t );

extern void         parser_cleanup( void );

C_WARN_UNUSED_RESULT
extern int          yyparse( void );

extern void         yyrestart( FILE* );

// local functions
static void         cdecl_cleanup( void );

C_WARN_UNUSED_RESULT
static bool         parse_argv( int, char const*[] );

C_WARN_UNUSED_RESULT
static bool         parse_command_line( char const*, int, char const*[] );

C_WARN_UNUSED_RESULT
static bool         parse_files( int, char const*const[] );

C_WARN_UNUSED_RESULT
static bool         parse_stdin( void );

static void         read_conf_file( void );

C_WARN_UNUSED_RESULT
static bool         starts_with_keyword( char const*, char const*, size_t );

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
  c_init = C_INIT_READ_CONF;

  // ...

  c_init = C_INIT_DONE;
  bool const ok = parse_argv( argc, argv );
  exit( ok ? EX_OK : EX_DATAERR );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether \a s is a cdecl command.
 *
 * @param s The null-terminated string to check.
 * @param command_type The type of commands to check against.
 * @return Returns `true` only if \a s is a command.
 */
C_WARN_UNUSED_RESULT
static bool is_command( char const *s, c_command_t command_type ) {
  struct argv_command {
    char const   *keyword;              // The keyword literal.
    c_command_t   command_type;         // The type of command.
  };
  typedef struct argv_command argv_command_t;

  //
  // Subset of cdecl commands (see CDECL_COMMANDS in autocomplete.c) that can
  // either be the program name or the first command-line argument.
  //
  static argv_command_t const ARGV_COMMANDS[] = {
    //
    // If this array is modified, also check CDECL_COMMANDS[] in
    // autocomplete.c.
    //
    { L_CAST,         COMMAND_PROG_NAME },
    { L_CONST,        COMMAND_FIRST_ARG },  // const cast
    { L_DECLARE,      COMMAND_PROG_NAME },
    { L_DEFINE,       COMMAND_FIRST_ARG },
    { L_DYNAMIC,      COMMAND_FIRST_ARG },  // dynamic cast
    { L_EXIT,         COMMAND_ANY       },
    { L_EXPLAIN,      COMMAND_PROG_NAME },
    { L_HELP,         COMMAND_FIRST_ARG },
    { L_NAMESPACE,    COMMAND_FIRST_ARG },
    { L_Q,            COMMAND_ANY       },
    { L_QUIT,         COMMAND_ANY       },
    { L_REINTERPRET,  COMMAND_FIRST_ARG },  // reinterpret cast
    { L_SET_COMMAND,  COMMAND_FIRST_ARG },
    { L_SHOW,         COMMAND_FIRST_ARG },
    { L_STATIC,       COMMAND_FIRST_ARG },  // static cast
    { L_TYPEDEF,      COMMAND_FIRST_ARG },
    { L_USING,        COMMAND_FIRST_ARG },
    { NULL,           COMMAND_FIRST_ARG },
  };

  SKIP_WS( s );

  for ( argv_command_t const *c = ARGV_COMMANDS; c->keyword != NULL; ++c ) {
    if ( c->command_type >= command_type ) {
      size_t const keyword_len = strlen( c->keyword );
      if ( starts_with_keyword( s, c->keyword, keyword_len ) ) {
        if ( c->keyword == L_CONST || c->keyword == L_STATIC ) {
          //
          // When in explain-by-default mode, a special case has to be made for
          // const and static since explain is implied only when NOT followed
          // by "cast":
          //
          //      const int *p                      // Implies explain.
          //      const cast p into pointer to int  // Does NOT imply explain.
          //
          char const *p = s + keyword_len;
          if ( isspace( *p ) ) {
            SKIP_WS( p );
            if ( !starts_with_keyword( p, L_CAST, 4 ) )
              break;
          }
        }
        return true;
      }
    }
  } // for
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
C_WARN_UNUSED_RESULT
static bool parse_argv( int argc, char const *argv[] ) {
  if ( argc == 0 )                      // cdecl
    return parse_stdin();
  if ( is_command( me, COMMAND_PROG_NAME ) )
    return parse_command_line( me, argc, argv );

  //
  // Note that options_init() adjusts argv such that argv[0] becomes the first
  // argument (and no longer the program name).
  //
  if ( is_command( argv[0], COMMAND_FIRST_ARG ) )
    return parse_command_line( NULL, argc, argv );

  if ( opt_explain )
    return parse_command_line( L_EXPLAIN, argc, argv );

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
C_WARN_UNUSED_RESULT
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
  char *s = CONST_CAST(char*, command_line);

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
 * @param file The FILE to read from.
 * @return Returns `true` only upon success.
 */
C_WARN_UNUSED_RESULT
static bool parse_file( FILE *file ) {
  bool ok = true;

  for ( char buf[ 1024 ]; fgets( buf, sizeof buf, file ) != NULL; ) {
    if ( !parse_string( buf, 0 ) )
      ok = false;
  } // for
  FERROR( file );

  return ok;
}

/**
 * Parses one or more files.
 *
 * @param num_files The length of \a files.
 * @param files An array of file names.
 * @return Returns `true` only upon success.
 */
C_WARN_UNUSED_RESULT
static bool parse_files( int num_files, char const *const files[] ) {
  bool ok = true;

  for ( int i = 0; i < num_files && ok; ++i ) {
    if ( strcmp( files[i], "-" ) == 0 ) {
      ok = parse_stdin();
    }
    else {
      FILE *const file = fopen( files[i], "r" );
      if ( unlikely( file == NULL ) )
        PMESSAGE_EXIT( EX_NOINPUT, "%s: %s\n", files[i], STRERROR() );
      if ( !parse_file( file ) )
        ok = false;
      C_IGNORE_RV( fclose( file ) );
    }
  } // for
  return ok;
}

/**
 * Parses standard input.
 *
 * @return Returns `true` only upon success.
 */
C_WARN_UNUSED_RESULT
static bool parse_stdin( void ) {
  bool ok = true;
  is_input_a_tty = isatty( fileno( fin ) );

  if ( is_input_a_tty || opt_interactive ) {
    if ( opt_prompt )
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
C_WARN_UNUSED_RESULT
bool parse_string( char const *s, size_t s_len ) {
  if ( s_len == 0 )
    s_len = strlen( s );

  bool reset_command_line = false;
  if ( command_line == NULL ) {
    //
    // The print code relies on command_line being set, so set it.
    //
    command_line = s;
    command_line_len = s_len;
    reset_command_line = true;
  }

  char *explain_buf = NULL;
  if ( opt_explain && !is_command( s, COMMAND_ANY ) ) {
    //
    // The string doesn't start with a command: insert "explain " and set
    // inserted_len so the print_*() functions subtract it from the error
    // column to get the correct column within the original string.
    //
    inserted_len = strlen( L_EXPLAIN ) + 1/*space*/;
    s_len += inserted_len;
    explain_buf = MALLOC( char, s_len + 1/*NULL*/ );
    char *p = strcpy_end( explain_buf, L_EXPLAIN );
    p = chrcpy_end( p, ' ' );
    C_IGNORE_RV( strcpy_end( p, s ) );
    s = explain_buf;
  }

  FILE *const temp = fmemopen( CONST_CAST( void*, s ), s_len, "r" );
  yyrestart( temp );
  bool const ok = yyparse() == 0;
  C_IGNORE_RV( fclose( temp ) );

  free( explain_buf );
  inserted_len = 0;

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
  bool const is_explicit_conf_file = (opt_conf_file != NULL);

  if ( !is_explicit_conf_file ) {       // no explicit conf file: use default
    char const *const home = home_dir();
    if ( home == NULL )
      return;
    static char conf_path_buf[ PATH_MAX ];
    strcpy( conf_path_buf, home );
    path_append( conf_path_buf, CONF_FILE_NAME_DEFAULT );
    opt_conf_file = conf_path_buf;
  }

  FILE *const fconf = fopen( opt_conf_file, "r" );
  if ( fconf == NULL ) {
    if ( is_explicit_conf_file )
      PMESSAGE_EXIT( EX_NOINPUT, "%s: %s\n", opt_conf_file, STRERROR() );
    return;
  }

  //
  // Before reading the configuration file, temporarily set the language to the
  // maximum supported C++ so "using" declarations, if any, won't cause the
  // parser to error out.
  //
  c_lang_id_t const orig_lang = opt_lang;
  opt_lang = LANG_CPP_NEW;
  C_IGNORE_RV( parse_file( fconf ) );
  opt_lang = orig_lang;

  C_IGNORE_RV( fclose( fconf ) );
}

/**
 * Checks whether \a s start with a keyword.
 *
 * @param s The null-terminated string to check.
 * @param keyword The keyword to check against.
 * @param keyword_len The length of \a keyword.
 * @return Returns `true` only if \a s starts with \a keyword.
 */
C_WARN_UNUSED_RESULT
static bool starts_with_keyword( char const *s, char const *keyword,
                                 size_t keyword_len ) {
  return  strncmp( s, keyword, keyword_len ) == 0 &&
          !is_ident( keyword[ keyword_len ] );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
