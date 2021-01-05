/*
**      cdecl -- C gibberish translator
**      src/cdecl.c
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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
#include "pjl_config.h"                 /* must go first */
#include "cdecl.h"
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

c_command_t const CDECL_COMMANDS[] = {
  //
  // If this array is modified, also check CDECL_COMMANDS[] in
  // autocomplete.c.
  //
  { L_CAST,         C_COMMAND_PROG_NAME,  LANG_ALL     },
  { L_CLASS,        C_COMMAND_FIRST_ARG,  LANG_CPP_ALL },
  { L_CONST,        C_COMMAND_FIRST_ARG,  LANG_CPP_ALL }, // const cast
  { L_DECLARE,      C_COMMAND_PROG_NAME,  LANG_ALL     },
  { L_DEFINE,       C_COMMAND_FIRST_ARG,  LANG_ALL     },
  { L_DYNAMIC,      C_COMMAND_FIRST_ARG,  LANG_CPP_ALL }, // dynamic cast
  { L_EXIT,         C_COMMAND_LANG_ONLY,  LANG_ALL     },
  { L_EXPLAIN,      C_COMMAND_PROG_NAME,  LANG_ALL     },
  { L_HELP,         C_COMMAND_FIRST_ARG,  LANG_ALL     },
  { L_NAMESPACE,    C_COMMAND_FIRST_ARG,  LANG_CPP_ALL },
  { L_QUIT,         C_COMMAND_LANG_ONLY,  LANG_ALL     },
  { L_REINTERPRET,  C_COMMAND_FIRST_ARG,  LANG_CPP_ALL }, // reinterpret cast
  { L_SET_COMMAND,  C_COMMAND_FIRST_ARG,  LANG_ALL     },
  { L_SHOW,         C_COMMAND_FIRST_ARG,  LANG_ALL     },
  { L_STATIC,       C_COMMAND_FIRST_ARG,  LANG_CPP_ALL }, // static cast
  { L_STRUCT,       C_COMMAND_FIRST_ARG,  LANG_ALL     },
  { L_TYPEDEF,      C_COMMAND_FIRST_ARG,  LANG_ALL     },
  { L_UNION,        C_COMMAND_FIRST_ARG,  LANG_ALL     },
  { L_USING,        C_COMMAND_FIRST_ARG,  LANG_CPP_MIN(11) },
  { NULL,           C_COMMAND_ANY,        LANG_NONE    },
};

// extern variable definitions
bool                c_initialized;
c_mode_t            c_mode;             // parsing english or gibberish?
char const         *command_line;       // command from command line, if any
size_t              command_line_len;   // length of command_line
size_t              inserted_len;       // length of inserted string
bool                is_input_a_tty;     // is our input from a tty?
char const         *me;                 // program name

// extern functions
PJL_WARN_UNUSED_RESULT
extern bool         parse_string( char const*, size_t );

extern void         parser_cleanup( void );

PJL_WARN_UNUSED_RESULT
extern int          yyparse( void );

extern void         yyrestart( FILE* );

// local functions
static void         cdecl_cleanup( void );

PJL_WARN_UNUSED_RESULT
static bool         parse_argv( int, char const*[] );

PJL_WARN_UNUSED_RESULT
static bool         parse_command_line( char const*, int, char const*[] );

PJL_WARN_UNUSED_RESULT
static bool         parse_files( int, char const*const[] );

PJL_WARN_UNUSED_RESULT
static bool         parse_stdin( void );

static void         read_conf_file( void );

PJL_WARN_UNUSED_RESULT
static bool         starts_with( char const*, char const*, size_t );

////////// main ///////////////////////////////////////////////////////////////

/**
 * The main entry point.
 *
 * @param argc The command-line argument count.
 * @param argv The command-line argument values.
 * @return Returns 0 on success, non-zero on failure.
 */
int main( int argc, char const *argv[] ) {
  atexit( cdecl_cleanup );
  options_init( &argc, &argv );

  c_typedef_init();
  lexer_reset( true );                  // resets line number

  if ( !opt_no_conf )
    read_conf_file();
  opt_conf_file = NULL;                 // don't print in errors any more
  c_initialized = true;

  bool const ok = parse_argv( argc, argv );
  exit( ok ? EX_OK : EX_DATAERR );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether \a s is a cdecl command.
 *
 * @param s The null-terminated string to check.
 * @param command_kind The type of commands to check against.
 * @return Returns `true` only if \a s is a command.
 */
PJL_WARN_UNUSED_RESULT
static bool is_command( char const *s, c_command_kind_t command_kind ) {
  assert( s != NULL );
  SKIP_WS( s );

  for ( c_command_t const *c = CDECL_COMMANDS; c->literal != NULL; ++c ) {
    if ( c->kind >= command_kind ) {
      size_t const literal_len = strlen( c->literal );
      if ( starts_with( s, c->literal, literal_len ) ) {
        if ( c->literal == L_CONST || c->literal == L_STATIC ) {
          //
          // When in explain-by-default mode, a special case has to be made for
          // const and static since explain is implied only when NOT followed
          // by "cast":
          //
          //      const int *p                      // Implies explain.
          //      const cast p into pointer to int  // Does NOT imply explain.
          //
          char const *p = s + literal_len;
          if ( !isspace( *p ) )
            break;
          SKIP_WS( p );
          if ( !starts_with( p, L_CAST, 4 ) )
            break;
          p += 4;
          if ( !isspace( *p ) )
            break;
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
PJL_WARN_UNUSED_RESULT
static bool parse_argv( int argc, char const *argv[const] ) {
  if ( argc == 0 )                      // cdecl
    return parse_stdin();
  if ( is_command( me, C_COMMAND_PROG_NAME ) )
    return parse_command_line( me, argc, argv );

  //
  // Note that options_init() adjusts argv such that argv[0] becomes the first
  // argument (and no longer the program name).
  //
  if ( is_command( argv[0], C_COMMAND_FIRST_ARG ) )
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
PJL_WARN_UNUSED_RESULT
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
PJL_WARN_UNUSED_RESULT
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
PJL_WARN_UNUSED_RESULT
static bool parse_files( int num_files, char const *const files[const] ) {
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
      PJL_IGNORE_RV( fclose( file ) );
    }
  } // for
  return ok;
}

/**
 * Parses standard input.
 *
 * @return Returns `true` only upon success.
 */
PJL_WARN_UNUSED_RESULT
static bool parse_stdin( void ) {
  bool ok = true;
  is_input_a_tty = isatty( fileno( fin ) );

  if ( is_input_a_tty || opt_interactive ) {
    if ( opt_prompt )
      FPRINTF( fout, "Type \"%s\" or \"?\" for help\n", L_HELP );
    ok = true;
    for (;;) {
      char *const line = read_input_line( cdecl_prompt[0], cdecl_prompt[1] );
      if ( line == NULL )
        break;
      ok = parse_string( line, 0 );
    } // for
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
PJL_WARN_UNUSED_RESULT
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
  if ( opt_explain && !is_command( s, C_COMMAND_ANY ) ) {
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
    PJL_IGNORE_RV( strcpy_end( p, s ) );
    s = explain_buf;
  }

  FILE *const temp = fmemopen( CONST_CAST( void*, s ), s_len, "r" );
  yyrestart( temp );
  bool const ok = yyparse() == 0;
  PJL_IGNORE_RV( fclose( temp ) );

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
  PJL_IGNORE_RV( parse_file( fconf ) );
  opt_lang = orig_lang;

  PJL_IGNORE_RV( fclose( fconf ) );
}

/**
 * Checks whether \a s start with a partial string.
 *
 * @param s The null-terminated string to check.
 * @param partial The partial string to check against.
 * @param partial_len The length of \a partial.
 * @return Returns `true` only if \a s starts with \a partial.
 */
PJL_WARN_UNUSED_RESULT
static bool starts_with( char const *s, char const *partial,
                         size_t partial_len ) {
  return  strncmp( s, partial, partial_len ) == 0 &&
          !is_ident( partial[ partial_len ] );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
