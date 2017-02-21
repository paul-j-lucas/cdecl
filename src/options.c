/*
**      cdecl -- C gibberish translator
**      src/options.c
**
**      Paul J. Lucas
*/

// local
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

#define GAVE_OPTION(OPT)          (opts_given[ (unsigned char)(OPT) ])
#define OPT_BUF_SIZE              32    /* used for format_opt() */
#define SET_OPTION(OPT)           (opts_given[ (unsigned char)(OPT) ] = (OPT))

// extern option variables
bool                opt_debug;
char const         *opt_fin;
char const         *opt_fout;
bool                opt_interactive;
lang_t              opt_lang;
bool                opt_make_c;
bool                opt_quiet;

// other extern variables
FILE               *fin;
FILE               *fout;

// local variables
static char         opts_given[ 128 ];

// local functions
static char*        format_opt( char, char[], size_t );
static char const*  get_long_opt( char );
static void         usage( void );

///////////////////////////////////////////////////////////////////////////////

static char const SHORT_OPTS[] = "89acdDikpqvx:";

static struct option const LONG_OPTS[] = {
  { "c89",          no_argument,        NULL, '8' },
  { "c99",          no_argument,        NULL, '9' },
  { "debug",        no_argument,        NULL, 'd' },
  { "language",     required_argument,  NULL, 'x' },
  { "interactive",  no_argument,        NULL, 'i' },
  { "quiet",        no_argument,        NULL, 'q' },
  { "version",      no_argument,        NULL, 'v' },
  { "yydebug",      no_argument,        NULL, 'D' },
  { NULL,           0,                  NULL, 0   }
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks that no options were given that are among the two given mutually
 * exclusive sets of short options.
 * Prints an error message and exits if any such options are found.
 *
 * @param opts1 The first set of short options.
 * @param opts2 The second set of short options.
 */
static void check_mutually_exclusive( char const *opts1, char const *opts2 ) {
  assert( opts1 );
  assert( opts2 );

  unsigned gave_count = 0;
  char const *opt = opts1;
  char gave_opt1 = '\0';

  for ( unsigned i = 0; i < 2; ++i ) {
    for ( ; *opt; ++opt ) {
      if ( GAVE_OPTION( *opt ) ) {
        if ( ++gave_count > 1 ) {
          char const gave_opt2 = *opt;
          char opt1_buf[ OPT_BUF_SIZE ];
          char opt2_buf[ OPT_BUF_SIZE ];
          PMESSAGE_EXIT( EX_USAGE,
            "%s and %s are mutually exclusive\n",
            format_opt( gave_opt1, opt1_buf, sizeof opt1_buf ),
            format_opt( gave_opt2, opt2_buf, sizeof opt2_buf  )
          );
        }
        gave_opt1 = *opt;
        break;
      }
    } // for
    if ( !gave_count )
      break;
    opt = opts2;
  } // for
}

/**
 * Formats an option as <code>[--%s/]-%c</code> where \c %s is the long option
 * (if any) and %c is the short option.
 *
 * @param short_opt The short option (along with its corresponding long option,
 * if any) to format.
 * @param buf The buffer to use.
 * @param buf_size The size of \a buf.
 * @return Returns \a buf.
 */
static char* format_opt( char short_opt, char buf[], size_t size ) {
  char const *const long_opt = get_long_opt( short_opt );
  snprintf(
    buf, size, "%s%s%s-%c",
    *long_opt ? "--" : "", long_opt, *long_opt ? "/" : "", short_opt
  );
  return buf;
}

/**
 * Gets the corresponding name of the long option for the given short option.
 *
 * @param short_opt The short option to get the corresponding long option for.
 * @return Returns the said option or the empty string if none.
 */
static char const* get_long_opt( char short_opt ) {
  for ( struct option const *long_opt = LONG_OPTS; long_opt->name;
        ++long_opt ) {
    if ( long_opt->val == short_opt )
      return long_opt->name;
  } // for
  return "";
}

/**
 * Parses a language name.
 *
 * @param s The null-terminated string to parse.
 * @return Returns the lang_t corresponding to \a s.
 */
static lang_t parse_lang( char const *s ) {
  struct lang_map {
    char const *name;
    lang_t      lang;
  };
  typedef struct lang_map lang_map_t;

  static lang_map_t const LANG_MAP[] = {
    { "cknr",   LANG_C_KNR  },
    { "knr",    LANG_C_KNR  },
    { "knrc",   LANG_C_KNR  },
    { "c89",    LANG_C_89   },
    { "c95",    LANG_C_95   },
    { "c99",    LANG_C_99   },
    { "c11",    LANG_C_11   },
    { "c++",    LANG_CPP    },
    { "c++11",  LANG_CPP_11 },
    { NULL,     LANG_NONE   },
  };

  size_t values_buf_size = 1;           // for trailing null

  for ( lang_map_t const *m = LANG_MAP; m->name; ++m ) {
    if ( strcasecmp( s, m->name ) == 0 )
      return m->lang;
    values_buf_size += strlen( m->name ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char *const values_buf = (char*)free_later( MALLOC( char, values_buf_size ) );
  char *pvalues = values_buf;
  for ( lang_map_t const *m = LANG_MAP; m->name; ++m ) {
    if ( pvalues > values_buf ) {
      strcpy( pvalues, ", " );
      pvalues += 2;
    }
    strcpy( pvalues, m->name );
    pvalues += strlen( m->name );
  } // for

  char opt_buf[ OPT_BUF_SIZE ];
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid value for %s; must be one of:\n\t%s\n",
    s, format_opt( 'x', opt_buf, sizeof opt_buf ), values_buf
  );
}

/**
 * Parses command-line options.
 *
 * @param argc The argument count from \c main().
 * @param argv The argument values from \c main().
 */
static void parse_options( int argc, char const *argv[] ) {
  optind = opterr = 1;
  bool print_version = false;

  for (;;) {
    int opt = getopt_long( argc, (char**)argv, SHORT_OPTS, LONG_OPTS, NULL );
    if ( opt == -1 )
      break;
    SET_OPTION( opt );
    switch ( opt ) {
      case '8':
      case 'a': opt_lang        = LANG_C_89;            break;
      case '9': opt_lang        = LANG_C_99;            break;
      case 'c': opt_make_c      = true;                 break;
      case 'd': opt_debug       = true;
      case 'D': yydebug         = true;                 break;
      case 'i': opt_interactive = true;                 break;
      case 'k':
      case 'p': opt_lang        = LANG_C_KNR;           break;
      case 'q': opt_quiet       = true;                 break;
      case 'v': print_version   = true;                 break;
      case 'x': opt_lang        = parse_lang( optarg ); break;
      default : usage();
    } // switch
  } // for

  check_mutually_exclusive( "a", "px" );
  check_mutually_exclusive( "v", "aipx" );

  if ( print_version ) {
    PRINT_ERR( "%s\n", PACKAGE_STRING );
    exit( EX_OK );
  }
}

static void usage( void ) {
  PRINT_ERR( "usage: %s [options] [files...]\n", me );
  PRINT_ERR( "       %s -v\n", me );
  PRINT_ERR( "\noptions:\n" );
  PRINT_ERR( "  -8        Use C89.\n" );
  PRINT_ERR( "  -9        Use C99.\n" );
  PRINT_ERR( "  -a        Sams as -8 (for historical cdecl compatibility).\n" );
  PRINT_ERR( "  -c        Create compilable output (include ; and {}).\n" );
#ifdef WITH_CDECL_DEBUG
  PRINT_ERR( "  -d        Enable debugging.\n" );
#endif /* WITH_CDECL_DEBUG */
#ifdef YYDEBUG
  PRINT_ERR( "  -D        Enable YACC debugging.\n" );
#endif /* YYDEBUG */
  PRINT_ERR( "  -i        Force interactive mode.\n" );
  PRINT_ERR( "  -k        Use K&R C.\n" );
  PRINT_ERR( "  -p        Same as -k (for historical cdecl compatibility).\n" );
  PRINT_ERR( "  -q        Be quiet (disable prompt).\n" );
  PRINT_ERR( "  -v        Print version an exit.\n" );
  PRINT_ERR( "  -+        Use C++.\n" );
  PRINT_ERR( "  -x lang   Use <lang>.\n" );
  exit( EX_USAGE );
}

////////// extern functions ///////////////////////////////////////////////////

void options_init( int argc, char const *argv[] ) {
  me = base_name( argv[0] );

  if ( strcasecmp( me, "c++decl" ) == 0 ||
       strcasecmp( me, "cppdecl" ) == 0 ||
       strcasecmp( me, "cxxdecl" ) == 0 ) {
    opt_lang = LANG_CPP;
  } else {
    opt_lang = LANG_C_11;
  }

  parse_options( argc, argv );
  argc -= optind, argv += optind;
  if ( argc )
    usage();

  if ( opt_fin && !(fin = fopen( opt_fin, "r" )) )
    PMESSAGE_EXIT( EX_NOINPUT, "\"%s\": %s\n", opt_fin, STRERROR );
  if ( opt_fout && !(fout = fopen( opt_fout, "w" )) )
    PMESSAGE_EXIT( EX_CANTCREAT, "\"%s\": %s\n", opt_fout, STRERROR );

  if ( !fin )
    fin = stdin;
  if ( !fout )
    fout = stdout;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
