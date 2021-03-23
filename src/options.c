/*
**      cdecl -- C gibberish translator
**      src/options.c
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
 * Defines global variables and functions for command-line options.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "options.h"
#include "c_lang.h"
#include "c_type.h"
#include "cdecl.h"
#include "color.h"
#include "print.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sysexits.h>

#define GAVE_OPTION(OPT)    (opts_given[ (unsigned char)(OPT) ])
#define SET_OPTION(OPT)     (GAVE_OPTION(OPT) = (char)(OPT))

// in ascending option character ASCII order
#define OPT_DIGRAPHS        '2'
#define OPT_TRIGRAPHS       '3'
#define OPT_ALT_TOKENS      'a'
#ifdef YYDEBUG
#define OPT_BISON_DEBUG     'B'
#endif /* YYDEBUG */
#define OPT_CONFIG          'c'
#define OPT_NO_CONFIG       'C'
#ifdef ENABLE_CDECL_DEBUG
#define OPT_CDECL_DEBUG     'd'
#endif /* ENABLE_CDECL_DEBUG */
#define OPT_EXPLAIN         'e'
#define OPT_EAST_CONST      'E'
#define OPT_FILE            'f'
#ifdef ENABLE_FLEX_DEBUG
#define OPT_FLEX_DEBUG      'F'
#endif /* ENABLE_FLEX_DEBUG */
#define OPT_HELP            'h'
#define OPT_INTERACTIVE     'i'
#define OPT_EXPLICIT_INT    'I'
#define OPT_COLOR           'k'
#define OPT_OUTPUT          'o'
#define OPT_NO_PROMPT       'p'
#define OPT_NO_SEMICOLON    's'
#define OPT_NO_TYPEDEFS     't'
#define OPT_VERSION         'v'
#define OPT_LANGUAGE        'x'

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
 * @sa parse_explicit_int()
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
  { "alt-tokens",   no_argument,        NULL, OPT_ALT_TOKENS    },
#ifdef YYDEBUG
  { "bison-debug",  no_argument,        NULL, OPT_BISON_DEBUG   },
#endif /* YYDEBUG */
  { "color",        required_argument,  NULL, OPT_COLOR         },
  { "config",       required_argument,  NULL, OPT_CONFIG        },
#ifdef ENABLE_CDECL_DEBUG
  { "debug",        no_argument,        NULL, OPT_CDECL_DEBUG   },
#endif /* ENABLE_CDECL_DEBUG */
  { "digraphs",     no_argument,        NULL, OPT_DIGRAPHS      },
  { "east-const",   no_argument,        NULL, OPT_EAST_CONST    },
  { "explain",      no_argument,        NULL, OPT_EXPLAIN       },
  { "explicit-int", required_argument,  NULL, OPT_EXPLICIT_INT  },
  { "file",         required_argument,  NULL, OPT_FILE          },
#ifdef ENABLE_FLEX_DEBUG
  { "flex-debug",   no_argument,        NULL, OPT_FLEX_DEBUG    },
#endif /* ENABLE_FLEX_DEBUG */
  { "help",         no_argument,        NULL, OPT_HELP          },
  { "interactive",  no_argument,        NULL, OPT_INTERACTIVE   },
  { "language",     required_argument,  NULL, OPT_LANGUAGE      },
  { "no-config",    no_argument,        NULL, OPT_NO_CONFIG     },
  { "no-prompt",    no_argument,        NULL, OPT_NO_PROMPT     },
  { "no-semicolon", no_argument,        NULL, OPT_NO_SEMICOLON  },
  { "no-typedefs",  no_argument,        NULL, OPT_NO_TYPEDEFS   },
  { "output",       required_argument,  NULL, OPT_OUTPUT        },
  { "trigraphs",    no_argument,        NULL, OPT_TRIGRAPHS     },
  { "version",      no_argument,        NULL, OPT_VERSION       },
  { NULL,           0,                  NULL, 0                 }
};

/**
 * Short options.
 *
 * @note
 * It _must_ start with `:` to make `getopt_long()` return `:` when a required
 * argument for a known option is missing.
 */
static char const   SHORT_OPTS[] = ":23ac:CeEf:hiI:k:o:pstvx:"
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
static char const*  opt_format( char, strbuf_t* );

PJL_WARN_UNUSED_RESULT
static char const*  opt_get_long( char );

noreturn
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
  assert( c_type_id_tpid( tid ) == C_TPID_BASE );
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
          strbuf_t opt1_sbuf, opt2_sbuf;
          PMESSAGE_EXIT( EX_USAGE,
            "%s and %s are mutually exclusive\n",
            opt_format( gave_opt1, &opt1_sbuf ),
            opt_format( gave_opt2, &opt2_sbuf )
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
 * Checks whether we're c++decl.
 *
 * @param prog_name The name of the program.
 * @returns Returns `true` only if we are.
 */
PJL_WARN_UNUSED_RESULT
static bool is_cppdecl( char const *prog_name ) {
  static char const *const NAMES[] = {
    CPPDECL,
    "cppdecl",
    "cxxdecl",
    NULL
  };

  for ( char const *const *pname = NAMES; *pname != NULL; ++pname ) {
    if ( strcasecmp( *pname, prog_name ) == 0 )
      return true;
  } // for
  return false;
}

/**
 * Formats an option as `[--%%s/]-%%c` where `%%s` is the long option (if any)
 * and `%%c` is the short option.
 *
 * @param short_opt The short option (along with its corresponding long option,
 * if any) to format.
 * @param sbuf A pointer to the strbuf to use.
 * @return Returns \a sbuf->str.
 */
PJL_WARN_UNUSED_RESULT
static char const* opt_format( char short_opt, strbuf_t *sbuf ) {
  assert( sbuf != NULL );
  strbuf_init( sbuf );
  char const *const long_opt = opt_get_long( short_opt );
  strbuf_catf(
    sbuf, "%s%s%s-%c",
    long_opt[0] != '\0' ? "--" : "",
    long_opt,
    long_opt[0] != '\0' ? "/" : "",
    short_opt
  );
  return sbuf->str;
}

/**
 * Gets the corresponding name of the long option for \a short_opt.
 *
 * @param short_opt The short option to get the corresponding long option for.
 * @return Returns said option or the empty string if none.
 */
PJL_WARN_UNUSED_RESULT
static char const* opt_get_long( char short_opt ) {
  FOREACH_CLI_OPTION( opt ) {
    if ( opt->val == short_opt )
      return opt->name;
  } // for
  return "";
}

/**
 * Parses a color "when" value.
 *
 * @param when The null-terminated "when" string to parse.
 * @return Returns the associated <code>\ref color_when</code> or prints an
 * error message and exits if \a when is invalid.
 */
PJL_WARN_UNUSED_RESULT
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

  for ( colorize_map_t const *m = COLORIZE_MAP; m->map_when != NULL; ++m ) {
    if ( strcasecmp( when, m->map_when ) == 0 )
      return m->map_colorization;
  } // for

  // name not found: construct valid name list for an error message
  strbuf_t when_sbuf;
  strbuf_init( &when_sbuf );
  bool comma = false;
  for ( colorize_map_t const *m = COLORIZE_MAP; m->map_when != NULL; ++m )
    strbuf_sepsn_cats( &when_sbuf, ", ", 2, &comma, m->map_when );

  strbuf_t opt_sbuf;
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid value for %s; must be one of: %s\n",
    when, opt_format( OPT_COLOR, &opt_sbuf ), when_sbuf.str
  );
}

/**
 * Parses a language name.
 *
 * @param lang_name The null-terminated name to parse.
 * @return Returns the <code>\ref c_lang_id_t</code> corresponding to \a
 * lang_name.
 */
PJL_WARN_UNUSED_RESULT
static c_lang_id_t parse_lang( char const *lang_name ) {
  assert( lang_name != NULL );

  c_lang_id_t const lang_id = c_lang_find( lang_name );
  if ( lang_id != LANG_NONE )
    return lang_id;

  strbuf_t langs_sbuf;
  strbuf_init( &langs_sbuf );
  bool comma = false;
  FOREACH_LANG( lang ) {
    if ( !lang->is_alias )
      strbuf_sepsn_cats( &langs_sbuf, ", ", 2, &comma, lang->name );
  } // for

  strbuf_t opt_sbuf;
  PMESSAGE_EXIT( EX_USAGE,
    "\"%s\": invalid value for %s; must be one of: %s\n",
    lang_name, opt_format( OPT_LANGUAGE, &opt_sbuf ), langs_sbuf.str
  );
}

/**
 * Parses command-line options.
 *
 * @param argc The argument count from main().
 * @param argv The argument values from main().
 */
static void parse_options( int argc, char const *argv[] ) {
  opterr = 0;                           // suppress default error message
  optind = 1;

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
      case OPT_ALT_TOKENS:  opt_alt_tokens  = true;                       break;
#ifdef YYDEBUG
      case OPT_BISON_DEBUG: opt_bison_debug = true;                       break;
#endif /* YYDEBUG */
#ifdef ENABLE_CDECL_DEBUG
      case OPT_CDECL_DEBUG: opt_cdecl_debug = true;                       break;
#endif /* ENABLE_CDECL_DEBUG */
      case OPT_COLOR:       color_when      = parse_color_when( optarg ); break;
      case OPT_CONFIG:      opt_conf_file   = optarg;                     break;
      case OPT_DIGRAPHS:    opt_graph       = C_GRAPH_DI;                 break;
      case OPT_EAST_CONST:  opt_east_const  = true;                       break;
      case OPT_EXPLAIN:     opt_explain     = true;                       break;
      case OPT_EXPLICIT_INT:parse_explicit_int( NULL, optarg );           break;
      case OPT_FILE:        fin_path        = optarg;                     break;
#ifdef ENABLE_FLEX_DEBUG
      case OPT_FLEX_DEBUG:  opt_flex_debug  = true;                       break;
#endif /* ENABLE_FLEX_DEBUG */
      case OPT_HELP:        usage();
      case OPT_INTERACTIVE: opt_interactive = true;                       break;
      case OPT_LANGUAGE:    opt_lang        = parse_lang( optarg );       break;
      case OPT_TRIGRAPHS:   opt_graph       = C_GRAPH_TRI;                break;
      case OPT_NO_CONFIG:   opt_no_conf     = true;                       break;
      case OPT_NO_PROMPT:   opt_prompt      = false;                      break;
      case OPT_NO_SEMICOLON:opt_semicolon   = false;                      break;
      case OPT_NO_TYPEDEFS: opt_typedefs    = false;                      break;
      case OPT_OUTPUT:      fout_path       = optarg;                     break;
      case OPT_VERSION:     print_version   = true;                       break;

      case ':': {
        strbuf_t sbuf;
        PMESSAGE_EXIT( EX_USAGE,
          "\"%s\" requires an argument\n", opt_format( (char)optopt, &sbuf )
        );
      }

      case '?':
        if ( --optind > 0 ) {           // defensive check
          char const *invalid_opt = argv[ optind ];
          //
          // We can offer "did you mean ...?" suggestions only if the invalid
          // option is a long option.
          //
          if ( invalid_opt != NULL && strncmp( invalid_opt, "--", 2 ) == 0 ) {
            invalid_opt += 2;           // skip over "--"
            EPRINTF( "%s: \"%s\": invalid option", me, invalid_opt );
            if ( !print_suggestions( DYM_CLI_OPTIONS, invalid_opt ) )
              goto use_help;
            EPUTC( '\n' );
            exit( EX_USAGE );
          }
        }
        EPRINTF( "%s: \"%c\": invalid option", me, (char)optopt );

use_help:
        EPUTS( "; use --help or -h for help\n" );
        exit( EX_USAGE );

      default:
        INTERNAL_ERR(
          "'%c': unaccounted-for getopt_long() return value\n", (char)opt
        );
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
 * Prints the usage message to standard error and exits.
 */
noreturn
static void usage( void ) {
  EPRINTF(
"usage: " PACKAGE " [options] [command...]\n"
"       " PACKAGE " [options] files...\n"
"options:\n"
"  --alt-tokens        (-%c)  Print alternative tokens.\n"
#ifdef YYDEBUG
"  --bison-debug       (-%c)  Enable Bison debug output.\n"
#endif /* YYDEBUG */
"  --color=WHEN        (-%c)  When to colorize output [default: not_file].\n"
"  --config=FILE       (-%c)  The configuration file [default: ~/" CONF_FILE_NAME_DEFAULT "].\n"
#ifdef ENABLE_CDECL_DEBUG
"  --debug             (-%c)  Enable debug output.\n"
#endif /* ENABLE_CDECL_DEBUG */
"  --digraphs          (-%c)  Print digraphs.\n"
"  --east-const        (-%c)  Print in \"east const\" form.\n"
"  --explain           (-%c)  Assume \"explain\" when no other command is given.\n"
"  --explicit-int=WHEN (-%c)  When to print \"int\" explicitly.\n"
"  --file=FILE         (-%c)  Read from this file [default: stdin].\n"
#ifdef ENABLE_FLEX_DEBUG
"  --flex-debug        (-%c)  Enable Flex debug output.\n"
#endif /* ENABLE_FLEX_DEBUG */
"  --help              (-%c)  Print this help and exit.\n"
"  --interactive       (-%c)  Force interactive mode.\n"
"  --language=LANG     (-%c)  Use LANG.\n"
"  --no-config         (-%c)  Suppress reading configuration file.\n"
"  --no-prompt         (-%c)  Suppress prompt.\n"
"  --no-semicolon      (-%c)  Suppress printing final semicolon for declarations.\n"
"  --no-typedefs       (-%c)  Suppress predefining standard types.\n"
"  --output=FILE       (-%c)  Write to this file [default: stdout].\n"
"  --trigraphs         (-%c)  Print trigraphs.\n"
"  --version           (-%c)  Print version and exit.\n"
"\n"
"Report bugs to: " PACKAGE_BUGREPORT "\n"
PACKAGE_NAME " home page: " PACKAGE_URL "\n",
    OPT_ALT_TOKENS,
#ifdef YYDEBUG
    OPT_BISON_DEBUG,
#endif /* YYDEBUG */
    OPT_COLOR,
    OPT_CONFIG,
#ifdef ENABLE_CDECL_DEBUG
    OPT_CDECL_DEBUG,
#endif /* ENABLE_CDECL_DEBUG */
    OPT_DIGRAPHS,
    OPT_EAST_CONST,
    OPT_EXPLAIN,
    OPT_EXPLICIT_INT,
    OPT_FILE,
#ifdef ENABLE_FLEX_DEBUG
    OPT_FLEX_DEBUG
#endif /* ENABLE_FLEX_DEBUG */
    OPT_HELP,
    OPT_INTERACTIVE,
    OPT_LANGUAGE,
    OPT_NO_CONFIG,
    OPT_NO_PROMPT,
    OPT_NO_SEMICOLON,
    OPT_NO_TYPEDEFS,
    OPT_OUTPUT,
    OPT_TRIGRAPHS,
    OPT_VERSION
  );
  exit( EX_USAGE );
}

////////// extern functions ///////////////////////////////////////////////////

bool any_explicit_int( void ) {
  return opt_explicit_int[0] != TB_NONE || opt_explicit_int[1] != TB_NONE;
}

struct option const* cli_option_next( struct option const *opt ) {
  if ( opt == NULL )
    opt = LONG_OPTS;
  else if ( (++opt)->name == NULL )
    opt = NULL;
  return opt;
}

bool is_explicit_int( c_type_id_t tid ) {
  assert( c_type_id_tpid( tid ) == C_TPID_BASE );

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

void parse_explicit_int( c_loc_t const *loc, char const *ei_format ) {
  assert( ei_format != NULL );

  c_type_id_t tid = TB_NONE;

  for ( char const *s = ei_format; *s != '\0'; ++s ) {
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
        if ( loc == NULL ) {
          strbuf_t opt_sbuf;
          PMESSAGE_EXIT( EX_USAGE,
            "\"%s\": invalid explicit int for %s:"
            " '%c': must be one of: i, u, or {[u]{isl[l]}[,]}+\n",
            s, opt_format( OPT_EXPLICIT_INT, &opt_sbuf ), *s
          );
        }
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

void print_explicit_int( FILE *out ) {
  bool const is_explicit_s   = is_explicit_int( TB_SHORT );
  bool const is_explicit_i   = is_explicit_int( TB_INT );
  bool const is_explicit_l   = is_explicit_int( TB_LONG );
  bool const is_explicit_ll  = is_explicit_int( TB_LONG_LONG );

  bool const is_explicit_us  = is_explicit_int( TB_UNSIGNED | TB_SHORT );
  bool const is_explicit_ui  = is_explicit_int( TB_UNSIGNED | TB_INT );
  bool const is_explicit_ul  = is_explicit_int( TB_UNSIGNED | TB_LONG );
  bool const is_explicit_ull = is_explicit_int( TB_UNSIGNED | TB_LONG_LONG );

  if ( is_explicit_s & is_explicit_i && is_explicit_l && is_explicit_ll ) {
    FPUTC( 'i', out );
  }
  else {
    if ( is_explicit_s   ) FPUTC(  's',  out );
    if ( is_explicit_i   ) FPUTC(  'i',  out );
    if ( is_explicit_l   ) FPUTC(  'l',  out );
    if ( is_explicit_ll  ) FPUTS(  "ll", out );
  }

  if ( is_explicit_us & is_explicit_ui && is_explicit_ul && is_explicit_ull ) {
    FPUTC( 'u', out );
  }
  else {
    if ( is_explicit_us  ) FPUTS( "us",  out );
    if ( is_explicit_ui  ) FPUTS( "ui",  out );
    if ( is_explicit_ul  ) FPUTS( "ul",  out );
    if ( is_explicit_ull ) FPUTS( "ull", out );
  }
}

void options_init( int *pargc, char const **pargv[] ) {
  assert( pargc != NULL );
  assert( pargv != NULL );

  me = base_name( (*pargv)[0] );
  opt_lang = is_cppdecl( me ) ? LANG_CPP_NEW : LANG_C_NEW;
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
