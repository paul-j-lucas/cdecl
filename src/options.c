/*
**      cdecl -- C gibberish translator
**      src/options.c
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
 * Defines global variables and functions for command-line options.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_lang.h"
#include "c_type.h"
#include "cdecl.h"
#include "color.h"
#include "options.h"
#include "print.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#define GAVE_OPTION(OPT)    (opts_given[ (unsigned char)(OPT) ])
#define OPT_BUF_SIZE        32          /* used for format_opt() */
#define SET_OPTION(OPT)     (opts_given[ (unsigned char)(OPT) ] = (char)(OPT))

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// extern option variables
bool                opt_alt_tokens;
#ifdef ENABLE_CDECL_DEBUG
bool                opt_cdecl_debug;
#endif /* ENABLE_CDECL_DEBUG */
char const         *opt_conf_file;
bool                opt_east_const;
bool                opt_explain;
c_graph_t           opt_graph;
bool                opt_interactive;
c_lang_id_t         opt_lang;
bool                opt_no_conf;
bool                opt_prompt = true;
bool                opt_semicolon = true;
bool                opt_typedefs = true;

// other extern variables
FILE               *fin;
FILE               *fout;

/**
 * The integer type(s) that `int` shall be print explicitly for in C/C++
 * declarations even when not needed because the type(s) contain at least one
 * integer modifier, e.g., `unsigned`.
 *
 * The elements are:
 *
 *  Idx | Contains type(s) for
 *  ----|---------------------
 *  `0` | signed integers
 *  `1` | unsigned integers
 *
 * @sa any_explicit_int()
 * @sa is_explicit_int()
 * @sa parse_opt_explicit_int()
 * @sa set_opt_explicit_int()
 */
static c_type_id_t  opt_explicit_int[] = { TB_NONE, TB_NONE };

/**
 * Long options.
 */
static struct option const LONG_OPTS[] = {
  //
  // If this array is modified, also modify SHORT_OPTS, the call(s) to
  // check_mutually_exclusive() in parse_options(), the message in usage(), and
  // the corresponding "set" option in SET_OPTIONS in set.c.
  //
  { "digraphs",     no_argument,        NULL, '2' },
  { "trigraphs",    no_argument,        NULL, '3' },
  { "alt-tokens",   no_argument,        NULL, 'a' },
#ifdef YYDEBUG
  { "bison-debug",  no_argument,        NULL, 'B' },
#endif /* YYDEBUG */
  { "config",       required_argument,  NULL, 'c' },
  { "no-config",    no_argument,        NULL, 'C' },
#ifdef ENABLE_CDECL_DEBUG
  { "debug",        no_argument,        NULL, 'd' },
#endif /* ENABLE_CDECL_DEBUG */
  { "explain",      no_argument,        NULL, 'e' },
  { "east-const",   no_argument,        NULL, 'E' },
  { "file",         required_argument,  NULL, 'f' },
#ifdef ENABLE_FLEX_DEBUG
  { "flex-debug",   no_argument,        NULL, 'F' },
#endif /* ENABLE_FLEX_DEBUG */
  { "help",         no_argument,        NULL, 'h' },
  { "interactive",  no_argument,        NULL, 'i' },
  { "explicit-int", required_argument,  NULL, 'I' },
  { "color",        required_argument,  NULL, 'k' },
  { "output",       required_argument,  NULL, 'o' },
  { "no-prompt",    no_argument,        NULL, 'p' },
  { "no-semicolon", no_argument,        NULL, 's' },
  { "no-typedefs",  no_argument,        NULL, 't' },
  { "version",      no_argument,        NULL, 'v' },
  { "language",     required_argument,  NULL, 'x' },
  { NULL,           0,                  NULL, 0   }
};

/**
 * Short options.
 */
static char const   SHORT_OPTS[] = "23ac:CeEf:iI:k:o:pstvx:"
#ifdef ENABLE_CDECL_DEBUG
  "d"
#endif /* ENABLE_CDECL_DEBUG */
#ifdef ENABLE_FLEX_DEBUG
  "F"
#endif /* ENABLE_FLEX_DEBUG */
#ifdef YYDEBUG
  "B"
#endif /* YYDEBUG */
;

// local variables
static char         opts_given[ 128 ];  ///< Table of options that were given.

// local functions
PJL_WARN_UNUSED_RESULT
static char*        format_opt( char, char[], size_t );

PJL_WARN_UNUSED_RESULT
static char const*  get_long_opt( char );

static void         usage( void );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Sets the given integer type(s) that `int` shall print explicitly for in
 * C/C++ declarations even when not needed because \a tid contains at least one
 * integer modifier, e.g., `unsigned`.
 *
 * @param tid The type(s) to print `int` explicitly for.  When multiple type
 * bits are set simultaneously, they are all considered either signed or
 * unsigned.
 *
 * @sa is_explicit_int()
 */
static inline void set_opt_explicit_int( c_type_id_t tid ) {
  assert( c_type_id_part_id( tid ) == TPID_BASE );
  bool const is_unsigned = (tid & TB_UNSIGNED) != TB_NONE;
  opt_explicit_int[ is_unsigned ] |= tid & c_type_id_compl( TB_UNSIGNED );
}

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
    for ( ; *opt != '\0'; ++opt ) {
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
    if ( gave_count == 0 )
      break;
    opt = opts2;
  } // for
}

/**
 * Formats an option as `[--%%s/]-%%c` where `%%s` is the long option (if any)
 * and `%%c` is the short option.
 *
 * @param short_opt The short option (along with its corresponding long option,
 * if any) to format.
 * @param buf The buffer to use.
 * @param buf_size The size of \a buf.
 * @return Returns \a buf.
 */
PJL_WARN_UNUSED_RESULT
static char* format_opt( char short_opt, char buf[const], size_t buf_size ) {
  char const *const long_opt = get_long_opt( short_opt );
  snprintf(
    buf, buf_size, "%s%s%s-%c",
    *long_opt != '\0' ? "--" : "",
    long_opt,
    *long_opt != '\0' ? "/" : "",
    short_opt
  );
  return buf;
}

/**
 * Gets the corresponding name of the long option for the given short option.
 *
 * @param short_opt The short option to get the corresponding long option for.
 * @return Returns the said option or the empty string if none.
 */
PJL_WARN_UNUSED_RESULT
static char const* get_long_opt( char short_opt ) {
  for ( struct option const *long_opt = LONG_OPTS; long_opt->name != NULL;
        ++long_opt ) {
    if ( long_opt->val == short_opt )
      return long_opt->name;
  } // for
  return "";
}

/**
 * Checks whether we're c++decl.
 *
 * @returns Returns `true` only if we are.
 */
PJL_WARN_UNUSED_RESULT
static bool is_cppdecl( void ) {
  static char const *const NAMES[] = {
    CPPDECL,
    "cppdecl",
    "cxxdecl",
    NULL
  };

  for ( char const *const *pname = NAMES; *pname != NULL; ++pname )
    if ( strcasecmp( *pname, me ) == 0 )
      return true;
  return false;
}

/**
 * Parses a color "when" value.
 *
 * @param when The null-terminated "when" string to parse.
 * @return Returns the associated <code>\ref color_when</code> or prints an
 * error message and exits if \a when is invalid.
 */
PJL_WARN_UNUSED_RESULT
static color_when_t parse_opt_color_when( char const *when ) {
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

  for ( colorize_map_t const *m = COLORIZE_MAP; m->map_when != NULL; ++m ) {
    if ( strcasecmp( when, m->map_when ) == 0 )
      return m->map_colorization;
    // sum sizes of names in case we need to construct an error message
    names_buf_size += strlen( m->map_when ) + 2 /* ", " */;
  } // for

  // name not found: construct valid name list for an error message
  char names_buf[ names_buf_size ];
  char *pnames = names_buf;
  for ( colorize_map_t const *m = COLORIZE_MAP; m->map_when != NULL; ++m ) {
    if ( pnames > names_buf )
      pnames = strcpy_end( pnames, ", " );
    pnames = strcpy_end( pnames, m->map_when );
  } // for

  char opt_buf[ OPT_BUF_SIZE ];
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid value for %s; must be one of:\n\t%s\n",
    when, format_opt( 'k', opt_buf, sizeof opt_buf ), names_buf
  );
}

/**
 * Parses a language name.
 *
 * @param s The null-terminated string to parse.
 * @return Returns the <code>\ref c_lang_id_t</code> corresponding to \a s.
 */
PJL_WARN_UNUSED_RESULT
static c_lang_id_t parse_opt_lang( char const *s ) {
  assert( s != NULL );

  c_lang_id_t const lang_id = c_lang_find( s );
  if ( lang_id != LANG_NONE )
    return lang_id;

  char opt_buf[ OPT_BUF_SIZE ];
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid value for %s; must be one of:\n\t%s\n",
    s, format_opt( 'x', opt_buf, sizeof opt_buf ), c_lang_names()
  );
}

/**
 * Parses command-line options.
 *
 * @param argc The argument count from `main()`.
 * @param argv The argument values from `main()`.
 */
static void parse_options( int argc, char const *argv[const] ) {
  optind = opterr = 1;

  color_when_t  color_when = COLOR_WHEN_DEFAULT;
  char const   *fin_path = "-";
  char const   *fout_path = "-";
  bool          print_version = false;

  for (;;) {
    int opt = getopt_long( argc, (char**)argv, SHORT_OPTS, LONG_OPTS, NULL );
    if ( opt == -1 )
      break;
    SET_OPTION( opt );
    switch ( opt ) {
      case '2': opt_graph       = C_GRAPH_DI;                     break;
      case '3': opt_graph       = C_GRAPH_TRI;                    break;
      case 'a': opt_alt_tokens  = true;                           break;
#ifdef YYDEBUG
      case 'B': opt_bison_debug = true;                           break;
#endif /* YYDEBUG */
      case 'c': opt_conf_file   = optarg;                         break;
      case 'C': opt_no_conf     = true;                           break;
#ifdef ENABLE_CDECL_DEBUG
      case 'd': opt_cdecl_debug = true;                           break;
#endif /* ENABLE_CDECL_DEBUG */
      case 'e': opt_explain     = true;                           break;
      case 'E': opt_east_const  = true;                           break;
      case 'f': fin_path        = optarg;                         break;
#ifdef ENABLE_FLEX_DEBUG
      case 'F': opt_flex_debug  = true;                           break;
#endif /* ENABLE_FLEX_DEBUG */
   // case 'h': usage();        // default case handles this
      case 'i': opt_interactive = true;                           break;
      case 'I': parse_opt_explicit_int( NULL, optarg );           break;
      case 'k': color_when      = parse_opt_color_when( optarg ); break;
      case 'o': fout_path       = optarg;                         break;
      case 'p': opt_prompt      = false;                          break;
      case 's': opt_semicolon   = false;                          break;
      case 't': opt_typedefs    = false;                          break;
      case 'v': print_version   = true;                           break;
      case 'x': opt_lang        = parse_opt_lang( optarg );       break;
      default : usage();
    } // switch
  } // for

  check_mutually_exclusive( "2", "3" );
  check_mutually_exclusive( "v", "23aBcCdeEfFhiIkopstx" );

  if ( print_version ) {
    printf( "%s\n", PACKAGE_STRING );
    exit( EX_OK );
  }

  if ( strcmp( fin_path, "-" ) != 0 && !(fin = fopen( fin_path, "r" )) )
    PMESSAGE_EXIT( EX_NOINPUT, "\"%s\": %s\n", fin_path, STRERROR() );
  if ( strcmp( fout_path, "-" ) != 0 && !(fout = fopen( fout_path, "w" )) )
    PMESSAGE_EXIT( EX_CANTCREAT, "\"%s\": %s\n", fout_path, STRERROR() );

  if ( fin == NULL )
    fin = stdin;
  if ( fout == NULL )
    fout = stdout;

  colorize = should_colorize( color_when );
  if ( colorize ) {
    if ( !(colors_parse( getenv( "CDECL_COLORS" ) )
        || colors_parse( getenv( "GCC_COLORS" ) )) ) {
      PJL_IGNORE_RV( colors_parse( COLORS_DEFAULT ) );
    }
  }
}

/**
 * Prints the usage message to standard error.
 */
static void usage( void ) {
  PUTS_OUT(
"usage: " PACKAGE " [options] [command...]\n"
"       " PACKAGE " [options] files...\n"
"options:\n"
"  --alt-tokens    (-a)  Print alternative tokens.\n"
#ifdef YYDEBUG
"  --bison-debug   (-B)  Enable Bison debug output.\n"
#endif /* YYDEBUG */
"  --color=WHEN    (-k)  When to colorize output [default: not_file].\n"
"  --config=FILE   (-c)  The configuration file [default: ~/" CONF_FILE_NAME_DEFAULT "].\n"
#ifdef ENABLE_CDECL_DEBUG
"  --debug         (-d)  Enable debug output.\n"
#endif /* ENABLE_CDECL_DEBUG */
"  --digraphs      (-2)  Print digraphs.\n"
"  --east-const    (-E)  Print in \"east const\" form.\n"
"  --explain       (-e)  Assume \"explain\" when no other command is given.\n"
"  --file=FILE     (-f)  Read from this file [default: stdin].\n"
#ifdef ENABLE_FLEX_DEBUG
"  --flex-debug    (-F)  Enable Flex debug output.\n"
#endif /* ENABLE_FLEX_DEBUG */
"  --help          (-h)  Print this help and exit.\n"
"  --interactive   (-i)  Force interactive mode.\n"
"  --language=LANG (-x)  Use LANG.\n"
"  --no-config     (-C)  Suppress reading configuration file.\n"
"  --no-prompt     (-p)  Suppress prompt.\n"
"  --no-semicolon  (-s)  Suppress printing trailing semicolon in declarations.\n"
"  --no-typedefs   (-t)  Suppress predefining standard types.\n"
"  --output=FILE   (-o)  Write to this file [default: stdout].\n"
"  --trigraphs     (-3)  Print trigraphs.\n"
"  --version       (-v)  Print version and exit.\n"
"\n"
"Report bugs to: " PACKAGE_BUGREPORT "\n"
PACKAGE_NAME " home page: " PACKAGE_URL "\n"
  );
  exit( EX_USAGE );
}

////////// extern functions ///////////////////////////////////////////////////

bool any_explicit_int( void ) {
  return opt_explicit_int[0] != TB_NONE || opt_explicit_int[1] != TB_NONE;
}

bool is_explicit_int( c_type_id_t tid ) {
  assert( c_type_id_part_id( tid ) == TPID_BASE );

  if ( tid == TB_UNSIGNED ) {
    //
    // Special case: "unsigned" by itself means "unsigned int."
    //
    tid |= TB_INT;
  }
  else if ( (tid & TB_LONG_LONG) != TB_NONE ) {
    //
    // Special case: for long long, its type is always combined with TB_LONG,
    // i.e., two bits are set.  Therefore, to check for explicit int for long
    // long, we first have to turn off the TB_LONG bit.
    //
    tid &= c_type_id_compl( TB_LONG );
  }
  bool const is_unsigned = (tid & TB_UNSIGNED) != TB_NONE;
  tid &= c_type_id_compl( TB_UNSIGNED );
  return (tid & opt_explicit_int[ is_unsigned ]) != TB_NONE;
}

void parse_opt_explicit_int( c_loc_t const *loc, char const *s ) {
  assert( s != NULL );

  char opt_buf[ OPT_BUF_SIZE ];
  c_type_id_t tid = TB_NONE;

  for ( ; *s != '\0'; ++s ) {
    switch ( *s ) {
      case 'i':
      case 'I':
        if ( (tid & TB_UNSIGNED) == TB_NONE ) {
          // If only 'i' is specified, it means all signed integer types shall
          // be explicit.
          tid |= TB_SHORT | TB_INT | TB_LONG | TB_LONG_LONG;
        } else {
          tid |= TB_INT;
        }
        break;
      case 'l':
      case 'L':
        if ( s[1] == 'l' || s[1] == 'L' ) {
          tid |= TB_LONG_LONG;
          ++s;
        } else {
          tid |= TB_LONG;
        }
        break;
      case 's':
      case 'S':
        tid |= TB_SHORT;
        break;
      case 'u':
      case 'U':
        tid |= TB_UNSIGNED;
        if ( s[1] == '\0' || s[1] == ',' ) {
          // If only 'u' is specified, it means all unsigned integer types
          // shall be explicit.
          tid |= TB_SHORT | TB_INT | TB_LONG | TB_LONG_LONG;
          break;
        }
        continue;
      case ',':
        break;
      default:
        if ( loc == NULL )
          PMESSAGE_EXIT( EX_USAGE,
            "\"%s\": invalid explicit int for %s:"
            " '%c': must be one of: i, u, or {[u]{isl[l]}[,]}+\n",
            s, format_opt( 'I', opt_buf, sizeof opt_buf ), *s
          );
        print_error( loc,
          "\"%s\": invalid explicit-int:"
          " must be one of: i, u, or {[u]{isl[l]}[,]}+\n",
          s
        );
        return;
    } // switch
    set_opt_explicit_int( tid );
    tid = TB_NONE;
  } // for
}

void print_opt_explicit_int( void ) {
  bool const is_explicit_s   = is_explicit_int( TB_SHORT );
  bool const is_explicit_i   = is_explicit_int( TB_INT );
  bool const is_explicit_l   = is_explicit_int( TB_LONG );
  bool const is_explicit_ll  = is_explicit_int( TB_LONG_LONG );

  bool const is_explicit_us  = is_explicit_int( TB_UNSIGNED | TB_SHORT );
  bool const is_explicit_ui  = is_explicit_int( TB_UNSIGNED | TB_INT );
  bool const is_explicit_ul  = is_explicit_int( TB_UNSIGNED | TB_LONG );
  bool const is_explicit_ull = is_explicit_int( TB_UNSIGNED | TB_LONG_LONG );

  if ( is_explicit_s & is_explicit_i && is_explicit_l && is_explicit_ll ) {
    PUTC_OUT( 'i' );
  }
  else {
    if ( is_explicit_s   ) PUTC_OUT(  's'  );
    if ( is_explicit_i   ) PUTC_OUT(  'i'  );
    if ( is_explicit_l   ) PUTC_OUT(  'l'  );
    if ( is_explicit_ll  ) PUTS_OUT(  "ll" );
  }

  if ( is_explicit_us & is_explicit_ui && is_explicit_ul && is_explicit_ull ) {
    PUTC_OUT( 'u' );
  }
  else {
    if ( is_explicit_us  ) PUTS_OUT( "us"  );
    if ( is_explicit_ui  ) PUTS_OUT( "ui"  );
    if ( is_explicit_ul  ) PUTS_OUT( "ul"  );
    if ( is_explicit_ull ) PUTS_OUT( "ull" );
  }
}

void options_init( int *pargc, char const **pargv[const] ) {
  assert( pargc != NULL );
  assert( pargv != NULL );

  me = base_name( (*pargv)[0] );
  opt_lang = is_cppdecl() ? LANG_CPP_NEW : LANG_C_NEW;
#ifdef ENABLE_FLEX_DEBUG
  //
  // When -d is specified, Flex enables debugging by default -- undo that.
  //
  opt_flex_debug = false;
#endif /* ENABLE_FLEX_DEBUG */
  parse_options( *pargc, *pargv );
  c_lang_set( opt_lang );
  *pargc -= optind;
  *pargv += optind;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
