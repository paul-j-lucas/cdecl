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
#include <readline.h>
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
static bool         is_stdin_a_tty;     // is stdin connected to a tty?

// extern functions
#ifdef HAVE_READLINE
void                readline_init( void );
#endif /* HAVE_READLINE */
int                 yyparse( void );

// local functions
static bool         called_as_command( char const* );
static void         cdecl_init( int, char const*[] );
static int          parse_command_line( int, char const*[] );
static int          parse_files( int, char const*[] );
static int          parse_stdin( void );
static int          parse_string( char const*, size_t );
static char*        readline_wrapper( void );

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
 * TODO
 *
 * @param argc The number of command-line arguments from main().
 * @param argv The command-line arguments from main().
 */
static void cdecl_init( int argc, char const *argv[] ) {
  atexit( cdecl_cleanup );
  options_init( argc, argv );

  is_stdin_a_tty = isatty( STDIN_FILENO );

  // init the prompt
  strcpy( prompt_buf, opt_lang == LANG_CPP ? "c++decl" : "cdecl" );
  strcat( prompt_buf, PROMPT_SUFFIX );
  prompt = prompt_buf;

#ifdef HAVE_READLINE
  readline_init();
#endif /* HAVE_READLINE */
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
 * @return TODO
 */
static int parse_files( int argc, char const *argv[] ) {
  int rv = 0;

  for ( ; optind < argc; ++optind ) {
    if ( strcmp( argv[optind], "-" ) == 0 )
      rv = parse_stdin();
    else {
      FILE *const fin = fopen( argv[optind], "r" );
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
 * @return TODO
 */
static int parse_stdin() {
  int rv;

  if ( is_stdin_a_tty || opt_interactive ) {

    if ( !opt_quiet )
      printf( "Type \"help\" or \"?\" for help\n" );

    rv = 0;
    for ( char *line; (line = readline_wrapper()); )
      rv = parse_string( line, strlen( line ) );

    puts( "" );
    return rv;
  }

  yyin = stdin;
  rv = yyparse();
  is_stdin_a_tty = false;
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

/**
 * Wraps GNU readline(3) by:
 *
 *  + Trimming lines of both leading and trailing whitespace.
 *  + Adding non-whitespace-only lines to the history.
 *  + Returning only non-whitespace-only lines.
 *  + Ensuring every line returned ends with a newline.
 *
 * @return Returns the line read or null for EOF.
 */
static char* readline_wrapper( void ) {
  static char *line_read;

  for (;;) {
    if ( line_read )
      free( line_read );
    line_read = readline( prompt );
    if ( !line_read )
      return NULL;

    char *line_nws = line_read;
    line_nws += strspn( line_nws, " \t\r" );

    size_t len = strlen( line_nws );
    bool trimmed_on_right = false;

    while ( len > 0 && isspace( line_nws[ --len ] ) ) {
      line_nws[ len ] = '\0';
      trimmed_on_right = true;
    } // while

    if ( line_nws ) {                   // if it wasn't all whitespace ...
      add_history( line_nws );
      //
      // readline() removes newlines, but we need newlines in the lexer to know
      // when to reset y_token_col, so we have to put a newline back.
      //
      if ( trimmed_on_right ) {
        //
        // Since we trimmed whitespace from the right side, there's room to
        // append a newline.  (The byte after the newline is guaranteed to be a
        // null byte.)
        //
        line_nws[ ++len ] = '\n';
      } else {
        //
        // Otherwise we need to allocate a whole new string, copy the old one
        // over (without leading whitespace, of course), and then append a
        // newline.  We re-use line_read so it'll be free'd on the next call.
        //
        free( line_read );
        line_read = MALLOC( char, len + 1/*\n*/ + 1/*\0*/ );
        len = strcpy_len( line_read, line_nws );
        line_read[ len++ ] = '\n';
        line_read[ len ] = '\0';
        line_nws = line_read;
      }
      return line_nws;
    }
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
