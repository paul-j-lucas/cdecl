/*
**    cdecl -- C gibberish translator
**    src/cdecl.c
*/

// local
#include "config.h"
#include "cdecl.h"
#include "cdgram.h"
#include "options.h"
#include "readline.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
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

// local variables
static bool         is_keyword;         // s argv[0] is a keyword?
static bool         is_tty;             // is stdin connected to a tty?
static char         prompt_buf[ PROMPT_MAX_LEN + 2/*> */ + 1/*null*/ ];
static char const  *prompt_ptr;

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

char *cat(char const *, ...);
void c_type_check(void);
void print_prompt(void);
void doprompt(void), noprompt(void);
void unsupp(char const*, char const*);
void notsupported(char const *, char const *, char const *);
void doset(char const *);
void dodeclare(char*, char*, char*, char*, char*);
void dodexplain(char*, char*, char*, char*, char*);
void docexplain(char*, char*, char*, char*);

/* variables used during parsing */
unsigned modbits = 0;
int arbdims = 1;
char const *savedname = 0;
char const unknown_name[] = "unknown_name";
char prev = 0;    /* the current type of the variable being examined */
                  /*    values  type           */
                  /*  p pointer          */
                  /*  r reference        */
                  /*  f function         */
                  /*  a array (of arbitrary dimensions)    */
                  /*  A array with dimensions      */
                  /*  n name           */
                  /*  v void           */
                  /*  s struct | class         */
                  /*  t simple type (int, long, etc.)    */

/* options */
bool MkProgramFlag;                     // -c, output {} and ; after declarations

#if dodebug
bool DebugFlag = 0;    /* -d, output debugging trace info */
#endif

#ifdef doyydebug    /* compile in yacc trace statements */
#define YYDEBUG 1
#endif /* doyydebug */

/* the names and bits checked for each row in the above array */
struct c_type_info {
  char const *name;
  unsigned    bit;
};
typedef struct c_type_info c_type_info_t;

c_type_info_t const C_TYPE_INFO[] = {
  { "void",     C_TYPE_VOID     },
  { "bool",     C_TYPE_BOOL     },
  { "char",     C_TYPE_CHAR     },
  { "wchar_t",  C_TYPE_WCHAR_T  },
  { "short",    C_TYPE_SHORT    },
  { "int",      C_TYPE_INT      },
  { "long",     C_TYPE_LONG     },
  { "signed",   C_TYPE_SIGNED   },
  { "unsigned", C_TYPE_UNSIGNED },
  { "float",    C_TYPE_FLOAT    },
  { "double",   C_TYPE_DOUBLE   }
};

/* definitions (and abbreviations) for type combinations cross check table */
#define ALWAYS  0                       // always okay 
#define NEVER   1                       // never allowed 
#define KNR     3                       // not allowed in Pre-ANSI compiler 
#define ANSI    4                       // not allowed anymore in ANSI compiler 

#define _ ALWAYS
#define X NEVER
#define K KNR
#define A ANSI

static char const crosscheck[][ ARRAY_SIZE( C_TYPE_INFO ) ] = {
  /*               v b c w s i l s u f d */
  /* void     */ { _,X,X,X,X,X,X,X,X,X,X },
  /* bool     */ { X,_,X,X,X,X,X,X,X,X,X },
  /* char     */ { X,X,_,X,X,X,X,_,_,X,X },
  /* wchar_t  */ { X,X,X,_,X,X,X,X,X,X,X },
  /* short    */ { X,X,X,X,X,_,X,_,_,X,X },
  /* int      */ { X,X,X,X,_,X,_,_,_,X,X },
  /* long     */ { X,X,X,X,X,_,X,_,_,X,_ },
  /* signed   */ { X,X,_,X,_,_,_,X,X,X,X },
  /* unsigned */ { X,X,_,X,_,_,_,X,X,X,X },
  /* float    */ { X,X,X,X,X,X,X,X,X,X,X },
  /* double   */ { X,X,X,X,X,X,_,X,X,X,X }
};

#if 0
/* This is an lower left triangular array. If we needed */
/* to save 9 bytes, the "long" row can be removed. */
char const crosscheck_old[9][9] = {
  /*              L, I, S, C, V, U, S, F, D, */
  /* long */      _, _, _, _, _, _, _, _, _,
  /* int */       _, _, _, _, _, _, _, _, _,
  /* short */     X, _, _, _, _, _, _, _, _,
  /* char */      X, X, X, _, _, _, _, _, _,
  /* void */      X, X, X, X, _, _, _, _, _,
  /* unsigned */  R, _, R, R, X, _, _, _, _,
  /* signed */    K, K, K, K, X, X, _, _, _,
  /* float */     A, X, X, X, X, X, X, _, _,
  /* double */    K, X, X, X, X, X, X, X, _
};
#endif


/* Run through the crosscheck array looking */
/* for unsupported combinations of types. */
void c_type_check() {

  /* Loop through the types */
  /* (skip the "long" row) */
  for ( size_t i = 1; i < ARRAY_SIZE( C_TYPE_INFO ); ++i ) {
    /* if this type is in use */
    if ((modbits & C_TYPE_INFO[i].bit) != 0) {
      /* check for other types also in use */
      for ( size_t j = 0; j < i; ++j ) {
        /* this type is not in use */
        if (!(modbits & C_TYPE_INFO[j].bit))
            continue;
        /* check the type of restriction */
        int restriction = crosscheck[i][j];
        if (restriction == ALWAYS)
            continue;
        char const *t1 = C_TYPE_INFO[i].name;
        char const *t2 = C_TYPE_INFO[j].name;
        if (restriction == NEVER) {
          notsupported("", t1, t2);
        }
        else if (restriction == KNR) {
          if ( opt_lang == LANG_C_KNR )
            notsupported(" (Pre-ANSI Compiler)", t1, t2);
        }
        else if (restriction == ANSI) {
          if ( opt_lang != LANG_C_KNR )
            notsupported(" (ANSI Compiler)", t1, t2);
        }
        else {
          fprintf (stderr,
            "%s: Internal error in crosscheck[%zu,%zu]=%d!\n",
            me, i, j, restriction);
          exit(1);
        }
      } // for
    }
  } // for
}

/* undefine these as they are no longer needed */
#undef _
#undef ALWAYS
#undef X
#undef NEVER
#undef P
#undef KNR
#undef A
#undef ANSI

/* Write out a message about something */
/* being unsupported, possibly with a hint. */
void unsupp( char const *s, char const *hint ) {
  notsupported("", s, NULL);
  if (hint)
    fprintf( stderr, "\t(maybe you mean \"%s\")\n", hint );
}

/* Write out a message about something */
/* being unsupported on a particular compiler. */
void notsupported( char const *compiler, char const *type1, char const *type2)
{
  if (type2)
    fprintf(stderr,
      "Warning: Unsupported in%s C%s -- '%s' with '%s'\n",
      compiler, opt_lang == LANG_CXX ? "++" : "", type1, type2);
  else
    fprintf(stderr,
      "Warning: Unsupported in%s C%s -- '%s'\n",
      compiler, opt_lang == LANG_CXX ? "++" : "", type1);
}

char* cat( char const *s1, ... ) {
  size_t len = 1;
  va_list args;

  /* find the length which needs to be allocated */
  va_start( args, s1 );
  for ( char const *s = s1; s; s = va_arg( args, char const* ) )
    len += strlen( s );
  va_end( args );

  /* allocate it */
  char *const newstr = MALLOC( char, len );
  newstr[0] = '\0';

  /* copy in the strings */
  va_start( args, s1 );
  for ( char const *s = s1; s; s = va_arg( args, char const* ) ) {
    strcat( newstr, s );
    free( (void*)s );
  } // for
  va_end( args );

  return newstr;
}

/* Tell how to invoke cdecl. */
/* Manage the prompts. */
static int prompting;

void doprompt() { prompting = 1; }
void noprompt() { prompting = 0; }

void print_prompt() {
#ifndef USE_READLINE
  if ( (is_tty || opt_interactive) && prompting ) {
    printf( "%s", prompt_ptr );
    fflush( stdout );
  }
#endif
}

/* print out a declaration */
void dodeclare(name, storage, left, right, type)
char *name, *storage, *left, *right, *type;
{
  if (prev == 'v')
    unsupp("Variable of type void", "variable of type pointer to void");

  if (*storage == 'r') {
    switch (prev) {
      case 'f': unsupp("Register function", NULL); break;
      case 'A':
      case 'a': unsupp("Register array", NULL); break;
      case 's': unsupp("Register struct/class", NULL); break;
    } // switch
  }

  if ( *storage )
    printf( "%s ", storage );

  printf(
    "%s %s%s%s", type, left,
    name ? name : (prev == 'f') ? "f" : "var", right
  );
  if ( MkProgramFlag ) {
    if ( (prev == 'f') && (*storage != 'e') )
      printf( " { }\n" );
    else
      printf( ";\n" );
  } else {
    printf( "\n" );
  }
  free( storage );
  free( left );
  free( right );
  free( type );
  if ( name )
    free( name );
}

void dodexplain(storage, constvol1, constvol2, type, decl)
  char *storage, *constvol1, *constvol2, *type, *decl;
{
  if (type && (strcmp(type, "void") == 0)) {
    if (prev == 'n')
      unsupp("Variable of type void", "variable of type pointer to void");
    else if (prev == 'a')
      unsupp("array of type void", "array of type pointer to void");
    else if (prev == 'r')
      unsupp("reference to type void", "pointer to void");
  }

  if (*storage == 'r')
    switch (prev) {
      case 'f': unsupp("Register function", NULL); break;
      case 'A':
      case 'a': unsupp("Register array", NULL); break;
      case 's': unsupp("Register struct/union/enum/class", NULL); break;
    } // switch

  printf( "declare %s as ", savedname );
  if ( *storage )
    printf( "%s ", storage );
  printf( "%s", decl );
  if ( *constvol1 )
    printf( "%s ", constvol1 );
  if ( *constvol2 )
    printf( "%s ", constvol2 );
  printf( "%s\n", type ? type : "int" );
}

void docexplain(constvol, type, cast, name)
char *constvol, *type, *cast, *name;
{
  if (strcmp(type, "void") == 0) {
    if (prev == 'a')
      unsupp("array of type void", "array of type pointer to void");
    else if (prev == 'r')
      unsupp("reference to type void", "pointer to void");
  }
  printf( "cast %s into %s", name, cast );
  if ( strlen( constvol ) > 0 )
    printf( "%s ", constvol );
  printf( "%s\n", type );
}

/* Do the appropriate things for the "set" command. */
void doset( char const *opt ) {
  if (strcmp(opt, "create") == 0)
    { MkProgramFlag = 1; }
  else if (strcmp(opt, "nocreate") == 0)
    { MkProgramFlag = 0; }
  else if (strcmp(opt, "prompt") == 0)
    { prompting = 1; prompt_ptr = prompt_buf; }
  else if (strcmp(opt, "noprompt") == 0)
    { prompting = 0; prompt_ptr = ""; }
#ifndef USE_READLINE
    /* I cannot seem to figure out what nointeractive was intended to do --
     * it didn't work well to begin with, and it causes problem with
     * readline, so I'm removing it, for now.  -i still works.
     */
  else if (strcmp(opt, "interactive") == 0)
    { opt_interactive = 1; }
  else if (strcmp(opt, "nointeractive") == 0)
    { opt_interactive = 0; is_tty = 0; }
#endif
  else if (strcmp(opt, "preansi") == 0)
    { opt_lang = LANG_C_KNR; }
  else if (strcmp(opt, "ansi") == 0)
    { opt_lang = LANG_C_ANSI; }
  else if (strcmp(opt, "cplusplus") == 0)
    { opt_lang = LANG_CXX; }
#ifdef dodebug
  else if (strcmp(opt, "debug") == 0)
    { DebugFlag = 1; }
  else if (strcmp(opt, "nodebug") == 0)
    { DebugFlag = 0; }
#endif /* dodebug */
#ifdef doyydebug
  else if (strcmp(opt, "yydebug") == 0)
    { yydebug = 1; }
  else if (strcmp(opt, "noyydebug") == 0)
    { yydebug = 0; }
#endif /* doyydebug */
  else {
    if ((strcmp(opt, unknown_name) != 0) &&
        (strcmp(opt, "options") != 0))
      printf("Unknown set option: '%s'\n", opt);

    printf("Valid set options (and command line equivalents) are:\n");
    printf("\toptions\n");
    printf("\tcreate (-c), nocreate\n");
    printf("\tprompt, noprompt (-q)\n");
#ifndef USE_READLINE
    printf("\tinteractive (-i), nointeractive\n");
#endif
    printf("\tpreansi (-p), ansi (-a) or cplusplus (-+)\n");
#ifdef dodebug
    printf("\tdebug (-d), nodebug\n");
#endif /* dodebug */
#ifdef doyydebug
    printf("\tyydebug (-D), noyydebug\n");
#endif /* doyydebug */

    printf("\nCurrent set values are:\n");
    printf("\t%screate\n", MkProgramFlag ? "   " : " no");
    printf("\t%sprompt\n", prompt_ptr[0] ? "   " : " no");
    printf("\t%sinteractive\n", (is_tty || opt_interactive) ? "   " : " no");
    if (opt_lang == LANG_C_KNR)
      printf("\t   preansi\n");
    else
      printf("\t(nopreansi)\n");
    if ( opt_lang == LANG_C_ANSI )
      printf("\t   ansi\n");
    else
      printf("\t(noansi)\n");
    if ( opt_lang == LANG_CXX )
      printf("\t   cplusplus\n");
    else
      printf("\t(nocplusplus)\n");
#ifdef dodebug
    printf("\t%sdebug\n", DebugFlag ? "   " : " no");
#endif /* dodebug */
#ifdef doyydebug
    printf("\t%syydebug\n", yydebug ? "   " : " no");
#endif /* doyydebug */
  }
}

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
