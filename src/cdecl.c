/*
**    cdecl -- C gibberish translator
**    src/cdecl.c
*/

// local
#include "config.h"
#include "cdgram.h"
#include "options.h"
#include "readline.h"
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

/* maximum # of chars from progname to display in prompt */
#define PROMPT_MAX_LEN  32

///////////////////////////////////////////////////////////////////////////////

// extern variables
extern FILE        *yyin;

// extern variable definitions
char const         *me;                 // program name
bool                prompting;
char                prompt_buf[ PROMPT_MAX_LEN + 2/*> */ + 1/*null*/ ];
char const         *prompt_ptr;

// local variables
static bool         is_keyword;         // s argv[0] is a keyword?
static bool         is_tty;             // is stdin connected to a tty?

// extern functions
#ifdef HAVE_READLINE
void                readline_init( void );
#endif /* HAVE_READLINE */
int                 yyparse( void );

// local functions
static bool         called_as_keyword( char const* );
static void         cdecl_init( int, char const*[] );
static int          parse_command_line( int, char const*[] );
static int          parse_files( int, char const*[] );
static int          parse_stdin( void );
static int          parse_string( char const* );
static char*        readline_wrapper( void );

///////////////////////////////////////////////////////////////////////////////

/* variables used during parsing */
char const unknown_name[] = "unknown_name";

#ifdef doyydebug    /* compile in yacc trace statements */
#define YYDEBUG 1
#endif /* doyydebug */

////////// main ///////////////////////////////////////////////////////////////

int main( int argc, char const *argv[] ) {
  cdecl_init( argc, argv );

  int rv = 0;

  if ( optind == argc )                 // no file names or "-"
    rv = parse_stdin();
  else if ( (is_keyword = called_as_keyword( argv[ optind ] )) )
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
static bool called_as_keyword( char const *argn ) {
  static char const *const COMMANDS[] = {
    "cast",
    "declare",
    "explain",
    "help",
    "set",
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

  prompting = is_tty = isatty( STDIN_FILENO );

  // init the prompt
  strcpy( prompt_buf, opt_lang == LANG_CXX ? "c++decl" : "cdecl" );
  strcat( prompt_buf, "> " );
  prompt_ptr = prompt_buf;

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
  int ret = 0;
  FILE *tmpfp = tmpfile();
  if (!tmpfp) {
    int sverrno = errno;
    fprintf (stderr, "%s: cannot open temp file\n", me);
    errno = sverrno;
    perror(me);
    return 1;
  }

  if ( is_keyword ) {
    if (fputs(me, tmpfp) == EOF) {
      int sverrno;
errwrite:
      sverrno = errno;
      fprintf (stderr, "%s: error writing to temp file\n", me);
      errno = sverrno;
      perror(me);
      fclose(tmpfp);
      return 1;
    }
  }

  for ( ; optind < argc; optind++)
    if (fprintf(tmpfp, " %s", argv[optind]) == EOF)
      goto errwrite;

  if (putc('\n', tmpfp) == EOF)
    goto errwrite;

  rewind( tmpfp );
  yyin = tmpfp;
  ret += yyparse();
  fclose( tmpfp );

  return ret;
}

/**
 * TODO
 *
 * @param argc The number of command-line arguments from main().
 * @param argv The command-line arguments from main().
 * @return TODO
 */
static int parse_files( int argc, char const *argv[] ) {
  FILE *ifp;
  int ret = 0;

  for ( ; optind < argc; ++optind ) {
    if ( strcmp( argv[optind], "-" ) == 0 )
      ret = parse_stdin();
    else if ( (ifp = fopen( argv[optind], "r") ) == NULL ) {
      int sverrno = errno;
      fprintf (stderr, "%s: cannot open %s\n", me, argv[optind]);
      errno = sverrno;
      perror(argv[optind]);
      ret++;
    } else {
      yyin = ifp;
      ret += yyparse();
    }
  } // for
  return ret;
}

/**
 * TODO
 *
 * @return TODO
 */
static int parse_stdin() {
  int ret;

  if ( is_tty || opt_interactive ) {
    char *line, *oldline;
    int len, newline;

    if ( !opt_quiet )
      printf( "Type `help' or `?' for help\n" );

    ret = 0;
    while ( (line = readline_wrapper()) ) {
      if ( strcmp( line, "quit" ) == 0 || strcmp( line, "exit" ) == 0 ) {
        free( line );
        return ret;
      }

      newline = 0;
      /* readline() strips newline, we add semicolon if necessary */
      len = strlen(line);
      if (len && line[len-1] != '\n' && line[len-1] != ';') {
        newline = 1;
        oldline = line;
        line = malloc(len+2);
        strcpy(line, oldline);
        line[len] = ';';
        line[len+1] = '\0';
      }
      if ( len )
        ret = parse_string( line );
      if (newline)
        free( line );
    } // while
    puts( "" );
    return ret;
  }

  yyin = stdin;
  ret = yyparse();
  is_tty = false;
  return ret;
}

/**
 * TODO
 *
 * @param s TODO
 * @return TODO
 */
static int parse_string( char const *s ) {
  yyin = fmemopen( s, strlen( s ), "r" );
  int const rv = yyparse();
  fclose( yyin );
  return rv;
}

/**
 * TODO
 *
 * @return TODO
 */
static char* readline_wrapper( void ) {
  static char *line_read;

  if ( line_read )
    free( line_read );

  line_read = readline( prompt_ptr );
  if ( line_read && *line_read )
    add_history( line_read );

  return line_read;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
