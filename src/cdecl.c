/*
**      cdecl -- C gibberish translator
**      src/cdecl.c
*/

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "literals.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGRAM_NAME_MAX_LEN      10    /* at least big enough for "c++decl" */
#define PROMPT_SUFFIX             "> "

///////////////////////////////////////////////////////////////////////////////

// extern variables
extern FILE        *yyin;

// extern variable definitions
char const         *me;                 // program name
char const         *prompt;
char                prompt_buf[ PROGRAM_NAME_MAX_LEN + sizeof PROMPT_SUFFIX ];

// local variables
static bool         is_argv0_a_command; // is argv[0] is a command?
static bool         is_fin_a_tty;       // is our input from a tty?

// extern functions
int                 yyparse( void );

// local functions
static bool         called_as_command( char const* );
static void         cdecl_init( int, char const*[] );
static int          parse_command_line( int, char const*[] );
static int          parse_files( int, char const*[] );
static int          parse_stdin( void );
static int          parse_string( char const*, size_t );

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const *argv[] ) {
  cdecl_init( argc, argv );

  int rv = 0;

  if ( optind == argc )                 // no file names or "-"
    rv = parse_stdin();
  else if ( (is_argv0_a_command = called_as_command( argv[ optind ] )) )
    rv = parse_command_line( argc, argv );
  else
    rv = parse_files( argc, argv );

  exit( rv );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * TODO
 *
 * @param argn TODO
 */
static bool called_as_command( char const *argn ) {
  static char const *const COMMANDS[] = {
    L_CAST,
    L_DECLARE,
    L_EXPLAIN,
    NULL
  };

  for ( char const *const *c = COMMANDS; *c; ++c )
    if ( strcmp( *c, me ) == 0 || strcmp( *c, argn ) == 0 )
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
 * Parses command-line options and sets up the prompt.
 *
 * @param argc The number of command-line arguments from main().
 * @param argv The command-line arguments from main().
 */
static void cdecl_init( int argc, char const *argv[] ) {
  atexit( cdecl_cleanup );
  options_init( argc, argv );

  is_fin_a_tty = isatty( fileno( fin ) );

  // init the prompt
  strcpy( prompt_buf, opt_lang >= LANG_CPP_MIN ? "c++decl" : PACKAGE );
  strcat( prompt_buf, PROMPT_SUFFIX );
  prompt = prompt_buf;
}

/**
 * TODO
 *
 * @param argc The number of command-line arguments from main().
 * @param argv The command-line arguments from main().
 * @return TODO
 */
static int parse_command_line( int argc, char const *argv[] ) {
  size_t buf_size = 1/*\n*/ + 1/*null*/;

  if ( is_argv0_a_command )
    buf_size += strlen( me );
  for ( int i = optind; i < argc; ++i )
    buf_size += 1 + strlen( argv[i] );

  char *const buf = MALLOC( char, buf_size );
  char *p = buf;
  if ( is_argv0_a_command )
    p += strcpy_len( buf, me );

  for ( int i = optind; i < argc; ++i ) {
    *p++ = ' ';
    p += strcpy_len( p, argv[i] );
  }

  int const rv = parse_string( buf, buf_size );
  FREE( buf );
  return rv;
}

/**
 * TODO
 *
 * @param argc The number of command-line arguments from main().
 * @param argv The command-line arguments from main().
 * @return Returns zero on success, non-zero on error.
 */
static int parse_files( int argc, char const *argv[] ) {
  int rv = 0;

  for ( ; optind < argc; ++optind ) {
    if ( strcmp( argv[ optind ], "-" ) == 0 )
      rv = parse_stdin();
    else {
      FILE *const fin = fopen( argv[ optind ], "r" );
      if ( fin == NULL )
        PERROR_EXIT( EX_NOINPUT );
      yyin = fin;
      rv = yyparse();
    }
    if ( rv != 0 )
      break;
  } // for
  return rv;
}

/**
 * TODO
 *
 * @return Returns zero on success, non-zero on error.
 */
static int parse_stdin( void ) {
  int rv;

  if ( is_fin_a_tty || opt_interactive ) {
    if ( !opt_quiet )
      printf( "Type \"%s\" or \"?\" for help\n", L_HELP );
    rv = 0;
    for ( char *line; (line = readline_wrapper()); )
      rv = parse_string( line, strlen( line ) );
  } else {
    yyin = fin;
    rv = yyparse();
    is_fin_a_tty = false;
  }
  return rv;
}

/**
 * TODO
 *
 * @param s The null-terminated string to parse.
 * @param s_len the length of \a s.
 * @return Returns zero on success, non-zero on error.
 */
static int parse_string( char const *s, size_t s_len ) {
  yyin = fmemopen( s, s_len, "r" );
  int const rv = yyparse();
  fclose( yyin );
  return rv;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
