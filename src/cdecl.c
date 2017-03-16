/*
**      cdecl -- C gibberish translator
**      src/cdecl.c
*/

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "color.h"
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
#include <sysexits.h>

#if HAVE_READLINE
#include <readline/readline.h>
#endif /* HAVE_READLINE */

#define PROMPT_SUFFIX             ">"

///////////////////////////////////////////////////////////////////////////////

// extern variables
extern FILE        *yyin;

// extern variable definitions
char const         *me;                 // program name
char const         *prompt;
char               *prompt_buf;

// local variables
static bool         is_input_a_tty;     // is our input from a tty?

// extern functions
int                 yyparse( void );

// local functions
static void         cdecl_init( int*, char const*** );
static bool         is_command( char const* );
static bool         parse_command_line( char const*, int, char const*[] );
static bool         parse_files( int, char const*[] );
static bool         parse_stdin( void );
static bool         parse_string( char const*, size_t );
static void         prompt_init( void );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks to see whether we're running genuine GNU readline and not some other
 * library emulating it.
 *
 * Some readline emulators, e.g., editline, have a bug that makes color prompts
 * not work correctly; see <http://stackoverflow.com/a/31333315/99089>.
 *
 * So, unless we know we're using genuine GNU readline, use this function to
 * disable color prompts.
 *
 * @return Returns \c true only if we're running genuine GNU readline.
 */
static inline bool have_genuine_gnu_readline( void ) {
#if HAVE_DECL_RL_GNU_READLINE_P
  return rl_gnu_readline_p == 1;
#else
  return false;
#endif /* HAVE_DECL_RL_GNU_READLINE_P */
}

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const **argv ) {
  cdecl_init( &argc, &argv );

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
 * @param s The null-terminated string to check
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
 * Parses command-line options and sets up the prompt.
 *
 * @param argc The length of \a argv.
 * @param argv The command-line arguments from main().
 */
static void cdecl_init( int *pargc, char const ***pargv ) {
  atexit( cdecl_cleanup );
  options_init( pargc, pargv );
  is_input_a_tty = isatty( fileno( fin ) );
  prompt_init();
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
  size_t buf_len = 1 /* ';' */;
  bool space;                           // need to putput a space?

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
  *s = ';';

  return parse_string( buf, buf_len );
}

/**
 * Parses one or more files.
 *
 * @param argc The length of \a argv.
 * @param argv The command-line arguments from main().
 * @return Returns \c true only upon success.
 */
static bool parse_files( int argc, char const *argv[] ) {
  bool ok = true;

  for ( int i = 0; i < argc && ok; ++i ) {
    if ( strcmp( argv[i], "-" ) == 0 )
      ok = parse_stdin();
    else {
      FILE *const fin = fopen( argv[i], "r" );
      if ( fin == NULL )
        PMESSAGE_EXIT( EX_NOINPUT, "%s: %s\n", argv[i], STRERROR );
      yyin = fin;
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
    for ( char *line; (line = readline_wrapper( prompt )); )
      ok = parse_string( line, strlen( line ) );
  } else {
    yyin = fin;
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
  yyin = fmemopen( s, s_len, "r" );
  bool const ok = yyparse() == 0;
  fclose( yyin );
  return ok;
}

/**
 * Initializes the prompt.
 */
static void prompt_init( void ) {
  size_t prompt_len =
    (strlen( CPPDECL )) +
    (strlen( PROMPT_SUFFIX ) + 1/*space*/);

  if ( have_genuine_gnu_readline() && sgr_prompt ) {
    prompt_len +=
      1 /* RL_PROMPT_START_IGNORE */ +
      (sizeof( SGR_START SGR_EL ) - 1) +
      (strlen( sgr_prompt )) +
      1 /* RL_PROMPT_END_IGNORE */ +
      (sizeof( SGR_END SGR_EL ) - 1);
  }

  FREE( prompt_buf );
  prompt_buf = MALLOC( char, prompt_len + 1/*null*/ );
  char *p = prompt_buf;

  char color_buf[20];

  if ( have_genuine_gnu_readline() && sgr_prompt ) {
    *p++ = RL_PROMPT_START_IGNORE;
    SGR_SSTART_COLOR( color_buf, prompt );
    p += strcpy_len( p, color_buf );
    *p++ = RL_PROMPT_END_IGNORE;
  }

  p += strcpy_len( p, opt_lang >= LANG_CPP_MIN ? CPPDECL : PACKAGE );
  p += strcpy_len( p, PROMPT_SUFFIX );

  if ( have_genuine_gnu_readline() && sgr_prompt ) {
    *p++ = RL_PROMPT_START_IGNORE;
    SGR_SEND_COLOR( color_buf );
    p += strcpy_len( p, color_buf );
    *p++ = RL_PROMPT_END_IGNORE;
  }

  *p++ = ' ';
  *p = '\0';

  prompt = prompt_buf;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
