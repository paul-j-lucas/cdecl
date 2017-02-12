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
#if dodebug
bool DebugFlag = 0;    /* -d, output debugging trace info */
#endif

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

  /* this sets up the prompt, which is on by default */
  size_t len = strlen( me );
  if ( len > PROMPT_MAX_LEN )
    len = PROMPT_MAX_LEN;
  strncpy( prompt_buf, me, len );
  prompt_buf[ len   ] = '>';
  prompt_buf[ len+1 ] = ' ';
  prompt_buf[ len+2 ] = '\0';

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

/*
 * cdecl - ANSI C and C++ declaration composer & decoder
 *
 *  originally written
 *    Graham Ross
 *    once at tektronix!tekmdp!grahamr
 *    now at Context, Inc.
 *
 *  modified to provide hints for unsupported types
 *  added argument lists for functions
 *  added 'explain cast' grammar
 *  added #ifdef for 'create program' feature
 *    ???? (sorry, I lost your name and login)
 *
 *  conversion to ANSI C
 *    David Wolverton
 *    ihnp4!houxs!daw
 *
 *  merged D. Wolverton's ANSI C version w/ ????'s version
 *  added function prototypes
 *  added C++ declarations
 *  made type combination checking table driven
 *  added checks for void variable combinations
 *  made 'create program' feature a runtime option
 *  added file parsing as well as just stdin
 *  added help message at beginning
 *  added prompts when on a TTY or in interactive mode
 *  added getopt() usage
 *  added -a, -r, -p, -c, -d, -D, -V, -i and -+ options
 *  delinted
 *  added #defines for those without getopt or void
 *  added 'set options' command
 *  added 'quit/exit' command
 *  added synonyms
 *    Tony Hansen
 *    attmail!tony, ihnp4!pegasus!hansen
 *
 *  added extern, register, static
 *  added links to explain, cast, declare
 *  separately developed ANSI C support
 *    Merlyn LeRoy
 *    merlyn@rose3.rosemount.com
 *
 *  merged versions from LeRoy
 *  added tmpfile() support
 *  allow more parts to be missing during explanations
 *    Tony Hansen
 *    attmail!tony, ihnp4!pegasus!hansen
 *
 *  added GNU readline() support
 *  added dotmpfile_from_string() to support readline()
 *  outdented C help text to prevent line from wrapping
 *  minor tweaking of makefile && mv makefile Makefile
 *  took out interactive and nointeractive commands when
 *      compiled with readline support
 *  added prompt and noprompt commands, -q option
 *    Dave Conrad
 *    conrad@detroit.freenet.org
 *
 *  added support for Apple's "blocks"
 *          Peter Ammon
 *          cdecl@ridiculousfish.com
 */

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
