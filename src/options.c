/*
**      cdecl -- C gibberish translator
**      src/options.c
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

// local
#include "config.h"                     /* must go first */
#include "color.h"
#include "common.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

#define GAVE_OPTION(OPT)          (opts_given[ (unsigned char)(OPT) ])
#define OPT_BUF_SIZE              32    /* used for format_opt() */
#define SET_OPTION(OPT)           (opts_given[ (unsigned char)(OPT) ] = (OPT))

// extern option variables
#ifdef WITH_CDECL_DEBUG
bool                opt_debug;
#endif /* WITH_CDECL_DEBUG */
char const         *opt_fin;
char const         *opt_fout;
bool                opt_interactive;
c_lang_t            opt_lang;
bool                opt_semicolon = true;
bool                opt_quiet;

// other extern variables
FILE               *fin;
FILE               *fout;

// local constant definitions
static struct option const LONG_OPTS[] = {
  { "color",        required_argument,  NULL, 'c' },
#ifdef WITH_CDECL_DEBUG
  { "debug",        no_argument,        NULL, 'd' },
#endif /* WITH_CDECL_DEBUG */
  { "file",         required_argument,  NULL, 'f' },
  { "interactive",  no_argument,        NULL, 'i' },
  { "output",       required_argument,  NULL, 'o' },
  { "quiet",        no_argument,        NULL, 'q' },
  { "no-semicolon", no_argument,        NULL, 's' },
  { "version",      no_argument,        NULL, 'v' },
  { "language",     required_argument,  NULL, 'x' },
#ifdef YYDEBUG
  { "yydebug",      no_argument,        NULL, 'y' },
#endif /* YYDEBUG */
  { NULL,           0,                  NULL, 0   }
};

static char const   SHORT_OPTS[] = "c:f:io:qsvx:"
#ifdef WITH_CDECL_DEBUG
  "d"
#endif /* WITH_CDECL_DEBUG */
#ifdef YYDEBUG
  "y"
#endif /* YYDEBUG */
;

// local variables
static char         opts_given[ 128 ];

// local functions
static char*        format_opt( char, char[], size_t );
static char const*  get_long_opt( char );
static void         usage( void );

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
  assert( opts1 != NULL );
  assert( opts2 != NULL );

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
 * Checks whether we're c++decl.
 *
 * @returns \c true only if we are.
 */
static bool is_cppdecl( void ) {
  static char const *const NAMES[] = {
    CPPDECL,
    "cppdecl",
    "cxxdecl",
    NULL
  };

  for ( char const *const *name = NAMES; *name; ++name )
    if ( strcasecmp( *name, me ) == 0 )
      return true;
  return false;
}

/**
 * Parses a color "when" value.
 *
 * @param when The NULL-terminated "when" string to parse.
 * @return Returns the associated \c color_when_t
 * or prints an error message and exits if \a when is invalid.
 */
static color_when_t parse_color_when( char const *when ) {
  struct colorize_map {
    char const   *map_when;
    color_when_t  map_colorization;
  };
  typedef struct colorize_map colorize_map_t;

  static colorize_map_t const COLORIZE_MAP[] = {
    { "always",    COLOR_ALWAYS   },
    { "auto",      COLOR_ISATTY   },    // grep compatibility
    { "isatty",    COLOR_ISATTY   },    // explicit synonym for auto
    { "never",     COLOR_NEVER    },
    { "not_file",  COLOR_NOT_FILE },    // !ISREG( stdout )
    { "not_isreg", COLOR_NOT_FILE },    // synonym for not_isfile
    { "tty",       COLOR_ISATTY   },    // synonym for isatty
    { NULL,        COLOR_NEVER    }
  };

  assert( when != NULL );
  size_t names_buf_size = 1;            // for trailing NULL

  for ( colorize_map_t const *m = COLORIZE_MAP; m->map_when; ++m ) {
    if ( strcasecmp( when, m->map_when ) == 0 )
      return m->map_colorization;
    // sum sizes of names in case we need to construct an error message
    names_buf_size += strlen( m->map_when ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char *const names_buf = (char*)free_later( MALLOC( char, names_buf_size ) );
  char *pnames = names_buf;
  for ( colorize_map_t const *m = COLORIZE_MAP; m->map_when; ++m ) {
    if ( pnames > names_buf ) {
      strcpy( pnames, ", " );
      pnames += 2;
    }
    strcpy( pnames, m->map_when );
    pnames += strlen( m->map_when );
  } // for

  char opt_buf[ OPT_BUF_SIZE ];
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid value for %s; must be one of:\n\t%s\n",
    when, format_opt( 'c', opt_buf, sizeof opt_buf ), names_buf
  );
}

/**
 * Parses a language name.
 *
 * @param s The null-terminated string to parse.
 * @return Returns the c_lang_t corresponding to \a s.
 */
static c_lang_t parse_lang( char const *s ) {
  size_t values_buf_size = 1;           // for trailing null

  for ( c_lang_info_t const *info = C_LANG_INFO; info->name; ++info ) {
    if ( strcasecmp( s, info->name ) == 0 )
      return info->lang;
    values_buf_size += strlen( info->name ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char *const values_buf = (char*)free_later( MALLOC( char, values_buf_size ) );
  char *pvalues = values_buf;
  for ( c_lang_info_t const *info = C_LANG_INFO; info->name; ++info ) {
    if ( pvalues > values_buf ) {
      strcpy( pvalues, ", " );
      pvalues += 2;
    }
    strcpy( pvalues, info->name );
    pvalues += strlen( info->name );
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

  color_when_t  color_when = COLOR_WHEN_DEFAULT;
  bool          print_version = false;

  for (;;) {
    int opt = getopt_long( argc, (char**)argv, SHORT_OPTS, LONG_OPTS, NULL );
    if ( opt == -1 )
      break;
    SET_OPTION( opt );
    switch ( opt ) {
      case 'c': color_when      = parse_color_when( optarg ); break;
#ifdef WITH_CDECL_DEBUG
      case 'd': opt_debug       = true;                       break;
#endif /* WITH_CDECL_DEBUG */
      case 'f': opt_fin         = optarg;                     break;
      case 'i': opt_interactive = true;                       break;
      case 'o': opt_fout        = optarg;                     break;
      case 'q': opt_quiet       = true;                       break;
      case 's': opt_semicolon   = false;                      break;
      case 'v': print_version   = true;                       break;
      case 'x': opt_lang        = parse_lang( optarg );       break;
#ifdef YYDEBUG
      case 'y': yydebug         = true;                       break;
#endif /* YYDEBUG */
      default : usage();
    } // switch
  } // for

  check_mutually_exclusive( "v", "cdfioqsxy" );

  if ( print_version ) {
    PRINT_ERR( "%s\n", PACKAGE_STRING );
    exit( EX_OK );
  }

  if ( opt_fin && !(fin = fopen( opt_fin, "r" )) )
    PMESSAGE_EXIT( EX_NOINPUT, "\"%s\": %s\n", opt_fin, STRERROR );
  if ( opt_fout && !(fout = fopen( opt_fout, "w" )) )
    PMESSAGE_EXIT( EX_CANTCREAT, "\"%s\": %s\n", opt_fout, STRERROR );

  if ( !fin )
    fin = stdin;
  if ( !fout )
    fout = stdout;

  colorize = should_colorize( color_when );
  if ( colorize ) {
    if ( !(colors_parse( getenv( "CDECL_COLORS" ) )
        || colors_parse( getenv( "GCC_COLORS" ) )) ) {
      colors_parse( COLORS_DEFAULT );
    }
  }
}

static void usage( void ) {
  PRINT_ERR( "usage: %s [options] [files...]\n", me );
  PRINT_ERR( "       %s -v\n", me );
  PRINT_ERR( "\noptions:\n" );
  PRINT_ERR( "  -c when  Specify when to colorize output [default: not_file].\n" );
#ifdef WITH_CDECL_DEBUG
  PRINT_ERR( "  -d       Enable debug output.\n" );
#endif /* WITH_CDECL_DEBUG */
  PRINT_ERR( "  -f file  Read from this file [default: stdin].\n" );
  PRINT_ERR( "  -i       Force interactive mode.\n" );
  PRINT_ERR( "  -o file  Write to this file [default: stdout].\n" );
  PRINT_ERR( "  -q       Be quiet (disable prompt).\n" );
  PRINT_ERR( "  -s       Suppress trailing semicolon in declarations.\n" );
  PRINT_ERR( "  -v       Print version and exit.\n" );
  PRINT_ERR( "  -x lang  Use <lang>.\n" );
#ifdef YYDEBUG
  PRINT_ERR( "  -y       Enable bison debug output.\n" );
#endif /* YYDEBUG */
  exit( EX_USAGE );
}

////////// extern functions ///////////////////////////////////////////////////

void options_init( int *pargc, char const ***pargv ) {
  me = base_name( (*pargv)[0] );
  opt_lang = is_cppdecl() ? LANG_CPP_MAX : LANG_C_MAX;
  parse_options( *pargc, *pargv );
  *pargc -= optind, *pargv += optind;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
