/*
**      cdecl -- C gibberish translator
**      src/cli_options.c
**
**      Copyright (C) 2017-2023  Paul J. Lucas, et al.
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
 * Defines functions for command-line options.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cli_options.h"
#ifdef WITH_READLINE
#include "autocomplete.h"
#endif /* WITH_READLINE */
#include "c_lang.h"
#include "c_type.h"
#include "cdecl.h"
#include "color.h"
#include "help.h"
#include "options.h"
#include "print.h"
#include "strbuf.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

// in ascending option character ASCII order
#define OPT_DIGRAPHS          2
#define OPT_TRIGRAPHS         3
#define OPT_ALT_TOKENS        a
#ifdef YYDEBUG
#define OPT_BISON_DEBUG       B
#endif /* YYDEBUG */
#define OPT_CONFIG            c
#define OPT_NO_CONFIG         C
#ifdef ENABLE_CDECL_DEBUG
#define OPT_CDECL_DEBUG       d
#endif /* ENABLE_CDECL_DEBUG */
#define OPT_ECHO_COMMANDS     O
#define OPT_EXPLAIN           e
#define OPT_EAST_CONST        E
#define OPT_FILE              f
#ifdef ENABLE_FLEX_DEBUG
#define OPT_FLEX_DEBUG        F
#endif /* ENABLE_FLEX_DEBUG */
#define OPT_HELP              h
#define OPT_EXPLICIT_INT      i
#define OPT_COLOR             k
#define OPT_OUTPUT            o
#define OPT_NO_PROMPT         p
#define OPT_TRAILING_RETURN   r
#define OPT_NO_SEMICOLON      s
#define OPT_EXPLICIT_ECSU     S
#define OPT_NO_TYPEDEFS       t
#define OPT_NO_ENGLISH_TYPES  T
#define OPT_NO_USING          u
#define OPT_VERSION           v
#define OPT_WEST_POINTER      w
#define OPT_LANGUAGE          x

/// Command-line short option as a character literal.
#define COPT(X)                   CHARIFY(OPT_##X)

/// Command-line option as a string literal.
#define SOPT(X)                   STRINGIFY(OPT_##X)

/// Command-line short option as a parenthesized, dashed string literal for the
/// usage message.
#define UOPT(X)                   " (-" SOPT(X) ") "

/// @endcond

/**
 * @addtogroup cli-options-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Long command-line options.
 */
static struct option const CLI_OPTIONS[] = {
  //
  // If this is updated, ensure the following are updated to match:
  //
  //  1. Calls to opt_check_exclusive().
  //  2. Calls to opt_check_mutually_exclusive().
  //  3. The message in print_usage().
  //  4. The corresponding "set" option in SET_OPTIONS in set_options.c.
  //
  { "alt-tokens",       no_argument,        NULL, COPT(ALT_TOKENS)        },
#ifdef YYDEBUG
  { "bison-debug",      no_argument,        NULL, COPT(BISON_DEBUG)       },
#endif /* YYDEBUG */
  { "color",            required_argument,  NULL, COPT(COLOR)             },
  { "config",           required_argument,  NULL, COPT(CONFIG)            },
#ifdef ENABLE_CDECL_DEBUG
  { "debug",            no_argument,        NULL, COPT(CDECL_DEBUG)       },
#endif /* ENABLE_CDECL_DEBUG */
  { "digraphs",         no_argument,        NULL, COPT(DIGRAPHS)          },
  { "east-const",       no_argument,        NULL, COPT(EAST_CONST)        },
  { "echo-commands",    no_argument,        NULL, COPT(ECHO_COMMANDS)     },
  { "explain",          no_argument,        NULL, COPT(EXPLAIN)           },
  { "explicit-ecsu",    required_argument,  NULL, COPT(EXPLICIT_ECSU)     },
  { "explicit-int",     required_argument,  NULL, COPT(EXPLICIT_INT)      },
  { "file",             required_argument,  NULL, COPT(FILE)              },
#ifdef ENABLE_FLEX_DEBUG
  { "flex-debug",       no_argument,        NULL, COPT(FLEX_DEBUG)        },
#endif /* ENABLE_FLEX_DEBUG */
  { "help",             no_argument,        NULL, COPT(HELP)              },
  { "language",         required_argument,  NULL, COPT(LANGUAGE)          },
  { "no-config",        no_argument,        NULL, COPT(NO_CONFIG)         },
  { "no-english-types", no_argument,        NULL, COPT(NO_ENGLISH_TYPES)  },
  { "no-prompt",        no_argument,        NULL, COPT(NO_PROMPT)         },
  { "no-semicolon",     no_argument,        NULL, COPT(NO_SEMICOLON)      },
  { "no-typedefs",      no_argument,        NULL, COPT(NO_TYPEDEFS)       },
  { "no-using",         no_argument,        NULL, COPT(NO_USING)          },
  { "output",           required_argument,  NULL, COPT(OUTPUT)            },
  { "trailing-return",  no_argument,        NULL, COPT(TRAILING_RETURN)   },
  { "trigraphs",        no_argument,        NULL, COPT(TRIGRAPHS)         },
  { "version",          no_argument,        NULL, COPT(VERSION)           },
  { "west-pointer",     required_argument,  NULL, COPT(WEST_POINTER)      },
  { NULL,               0,                  NULL, 0                       }
};

// local variables
static bool         opts_given[ 128 ];  ///< Table of options that were given.

// local functions
NODISCARD
static char const*  opt_format( char ),
                 *  opt_get_long( char );

_Noreturn
static void         print_usage( int );

static void         print_version( bool );

////////// local functions ////////////////////////////////////////////////////

/**
 * Makes the `optstring` (short option) equivalent of \a opts for the third
 * argument of `getopt_long()`.
 *
 * @param opts An array of options to make the short option string from.  Its
 * last element must be all zeros.
 * @return Returns the `optstring` for the third argument of `getopt_long()`.
 * The caller is responsible for freeing it.
 */
NODISCARD
static char const* make_short_opts( struct option const opts[static const 2] ) {
  // pre-flight to calculate string length
  size_t len = 1;                       // for leading ':'
  for ( struct option const *opt = opts; opt->name != NULL; ++opt ) {
    assert( opt->has_arg >= 0 && opt->has_arg <= 2 );
    len += 1 + STATIC_CAST( unsigned, opt->has_arg );
  } // for

  char *const short_opts = MALLOC( char, len + 1/*\0*/ );
  char *s = short_opts;

  *s++ = ':';                           // return missing argument as ':'
  for ( struct option const *opt = opts; opt->name != NULL; ++opt ) {
    assert( opt->val > 0 && opt->val < 128 );
    *s++ = STATIC_CAST( char, opt->val );
    switch ( opt->has_arg ) {
      case optional_argument:
        *s++ = ':';
        FALLTHROUGH;
      case required_argument:
        *s++ = ':';
    } // switch
  } // for
  *s = '\0';

  return short_opts;
}

/**
 * If \a opt was given, checks that _only_ it was given and, if not, prints an
 * error message and exits; if \a opt was not given, does nothing.
 *
 * @param opt The option to check for.
 */
static void opt_check_exclusive( char opt ) {
  if ( !opts_given[ STATIC_CAST( unsigned, opt ) ] )
    return;
  for ( size_t i = '0'; i < ARRAY_SIZE( opts_given ); ++i ) {
    char const curr_opt = STATIC_CAST( char, i );
    if ( curr_opt == opt )
      continue;
    if ( opts_given[ STATIC_CAST( unsigned, curr_opt ) ] ) {
      fatal_error( EX_USAGE,
        "%s can be given only by itself\n",
        opt_format( opt )
      );
    }
  } // for
}

/**
 * If \a opt was given, checks that no option in \a opts was also given.  If it
 * was, prints an error message and exits; if it wasn't, does nothing.
 *
 * @param opt The option.
 * @param opts The set of options.
 */
static void opt_check_mutually_exclusive( char opt, char const *opts ) {
  assert( opts != NULL );
  if ( !opts_given[ STATIC_CAST( unsigned, opt ) ] )
    return;
  for ( ; *opts != '\0'; ++opts ) {
    assert( *opts != opt );
    if ( opts_given[ STATIC_CAST( unsigned, *opts ) ] ) {
      fatal_error( EX_USAGE,
        "%s and %s are mutually exclusive\n",
        opt_format( opt ),
        opt_format( *opts )
      );
    }
  } // for
}

/**
 * Formats an option as `[--%%s/]-%%c` where `%%s` is the long option (if any)
 * and `%%c` is the short option.
 *
 * @param short_opt The short option (along with its corresponding long option,
 * if any) to format.
 * @return Returns said formatted string.
 *
 * @warning The pointer returned is to a small number of static buffers, so you
 * can't do something like call this more than twice in the same `printf()`
 * statement.
 */
NODISCARD
static char const* opt_format( char short_opt ) {
  static strbuf_t sbufs[2];
  static unsigned buf_index;

  strbuf_t *const sbuf = &sbufs[ buf_index++ % ARRAY_SIZE( sbufs ) ];
  strbuf_reset( sbuf );

  char const *const long_opt = opt_get_long( short_opt );
  strbuf_printf(
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
NODISCARD
static char const* opt_get_long( char short_opt ) {
  FOREACH_CLI_OPTION( opt ) {
    if ( opt->val == short_opt )
      return opt->name;
  } // for
  return "";
}

/**
 * Prints that \a value is an invalid value for \a opt to standard error and
 * exits.
 *
 * @param opt The option that has the invalid \a value.
 * @param value The invalid value for \a opt.
 * @param must_be What \a opt must be instead.
 */
_Noreturn
static void opt_invalid_value( char opt, char const *value,
                               char const *must_be ) {
  fatal_error( EX_USAGE,
    "\"%s\": invalid value for %s; must be %s\n",
    value, opt_format( opt ), must_be
  );
}

/**
 * Parses a color "when" value.
 *
 * @param when The null-terminated "when" string to parse.
 * @return Returns the associated \ref color_when or prints an error message
 * and exits if \a when is invalid.
 */
NODISCARD
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
    strbuf_sepsn_puts( &when_sbuf, ", ", 2, &comma, m->map_when );
  opt_invalid_value( COPT(COLOR), when, when_sbuf.str );
}

/**
 * Parses a language name.
 *
 * @param lang_name The null-terminated name to parse.
 * @return Returns the \ref c_lang_id_t corresponding to \a lang_name.
 */
NODISCARD
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
      strbuf_sepsn_puts( &langs_sbuf, ", ", 2, &comma, lang->name );
  } // for
  opt_invalid_value( COPT(LANGUAGE), lang_name, langs_sbuf.str );
}

/**
 * Parses command-line options.
 *
 * @param pargc A pointer to the argument count from main().
 * @param pargv A pointer to the argument values from main().
 */
static void parse_options( int *pargc, char const **pargv[const] ) {
  opterr = 0;                           // suppress default error message

  char const *      fin_path = "-";
  char const *      fout_path = "-";
  int               opt;
  bool              opt_help = false;
  unsigned          opt_version = 0;
  char const *const short_opts = make_short_opts( CLI_OPTIONS );

  for (;;) {
    opt = getopt_long(
      *pargc, CONST_CAST( char**, *pargv ), short_opts, CLI_OPTIONS,
      /*longindex=*/NULL
    );
    if ( opt == -1 )
      break;
    switch ( opt ) {
      case COPT(ALT_TOKENS):
        opt_alt_tokens = true;
        break;
#ifdef YYDEBUG
      case COPT(BISON_DEBUG):
        opt_bison_debug = true;
        break;
#endif /* YYDEBUG */
#ifdef ENABLE_CDECL_DEBUG
      case COPT(CDECL_DEBUG):
        opt_cdecl_debug = true;
        break;
#endif /* ENABLE_CDECL_DEBUG */
      case COPT(COLOR):
        opt_color_when = parse_color_when( optarg );
        break;
      case COPT(CONFIG):
        if ( *SKIP_WS( optarg ) == '\0' )
          goto missing_arg;
        opt_conf_path = optarg;
        break;
      case COPT(DIGRAPHS):
        opt_graph = C_GRAPH_DI;
        break;
      case COPT(EAST_CONST):
        opt_east_const = true;
        break;
      case COPT(ECHO_COMMANDS):
        opt_echo_commands = true;
        break;
      case COPT(EXPLAIN):
        opt_explain = true;
        break;
      case COPT(EXPLICIT_ECSU):
        if ( !parse_explicit_ecsu( optarg ) )
          opt_invalid_value(
            COPT(EXPLICIT_ECSU), optarg, "*, -, or {e|c|s|u}+"
          );
        break;
      case COPT(EXPLICIT_INT):
        if ( !parse_explicit_int( optarg ) )
          opt_invalid_value(
            COPT(EXPLICIT_INT), optarg, "*, -, i, u, or {[u]{i|s|l[l]}[,]}+"
          );
        break;
      case COPT(FILE):
        if ( *SKIP_WS( optarg ) == '\0' )
          goto missing_arg;
        fin_path = optarg;
        break;
#ifdef ENABLE_FLEX_DEBUG
      case COPT(FLEX_DEBUG):
        opt_flex_debug = true;
        break;
#endif /* ENABLE_FLEX_DEBUG */
      case COPT(HELP):
        opt_help = true;
        break;
      case COPT(LANGUAGE):
        opt_lang = parse_lang( optarg );
        break;
      case COPT(TRIGRAPHS):
        opt_graph = C_GRAPH_TRI;
        break;
      case COPT(NO_CONFIG):
        opt_read_conf = false;
        break;
      case COPT(NO_ENGLISH_TYPES):
        opt_english_types = false;
        break;
      case COPT(NO_PROMPT):
        opt_prompt = false;
        break;
      case COPT(NO_SEMICOLON):
        opt_semicolon = false;
        break;
      case COPT(NO_TYPEDEFS):
        opt_typedefs = false;
        break;
      case COPT(NO_USING):
        opt_using = false;
        break;
      case COPT(TRAILING_RETURN):
        opt_trailing_ret = true;
        break;
      case COPT(OUTPUT):
        if ( *SKIP_WS( optarg ) == '\0' )
          goto missing_arg;
        fout_path = optarg;
        break;
      case COPT(VERSION):
        ++opt_version;
        break;
      case COPT(WEST_POINTER):
        if ( !parse_west_pointer( optarg ) )
          opt_invalid_value(
            COPT(WEST_POINTER), optarg, "*, -, or {b|f|l|o|r|t}+"
          );
        break;

      case ':':
        goto missing_arg;
      case '?':
        goto invalid_opt;

      default:
        // LCOV_EXCL_START
        if ( isprint( opt ) )
          INTERNAL_ERROR(
            "'%c': unaccounted-for getopt_long() return value\n", opt
          );
        INTERNAL_ERROR(
          "%d: unaccounted-for getopt_long() return value\n", opt
        );
        // LCOV_EXCL_STOP
    } // switch
    opts_given[ opt ] = true;
  } // for

  FREE( short_opts );

  *pargc -= optind;
  *pargv += optind;

  opt_check_exclusive( COPT(HELP) );
  opt_check_exclusive( COPT(VERSION) );
  opt_check_mutually_exclusive( COPT(DIGRAPHS), SOPT(TRIGRAPHS) );

  if ( strcmp( fin_path, "-" ) != 0 ) {
    FILE *const fin = fopen( fin_path, "r" );
    if ( fin == NULL )
      fatal_error( EX_NOINPUT, "\"%s\": %s\n", fin_path, STRERROR() );
    DUP2( fileno( fin ), STDIN_FILENO );
    PJL_IGNORE_RV( fclose( fin ) );
  }

  if ( strcmp( fout_path, "-" ) != 0 ) {
    FILE *const fout = fopen( fout_path, "w" );
    if ( fout == NULL )
      fatal_error( EX_CANTCREAT, "\"%s\": %s\n", fout_path, STRERROR() );
    DUP2( fileno( fout ), STDOUT_FILENO );
    PJL_IGNORE_RV( fclose( fout ) );
  }

  if ( opt_help )
    print_usage( *pargc > 0 ? EX_USAGE : EX_OK );

  if ( opt_version > 0 ) {
    if ( *pargc > 0 )                   // cdecl -v foo
      print_usage( EX_USAGE );
    print_version( /*verbose=*/opt_version > 1 );
    exit( EX_OK );
  }

  return;

invalid_opt:
  NO_OP;
  // Determine whether the invalid option was short or long.
  char const *invalid_opt = (*pargv)[ optind - 1 ];
  if ( invalid_opt != NULL && strncmp( invalid_opt, "--", 2 ) == 0 ) {
    invalid_opt += 2;                   // skip over "--"
    EPRINTF( "%s: \"%s\": invalid option", me, invalid_opt );
    if ( !print_suggestions( DYM_CLI_OPTIONS, invalid_opt ) )
      goto use_help;
    EPUTC( '\n' );
    exit( EX_USAGE );
  }
  EPRINTF( "%s: '%c': invalid option", me, STATIC_CAST( char, optopt ) );

use_help:
  print_use_help();
  exit( EX_USAGE );

missing_arg:
  fatal_error( EX_USAGE,
    "\"%s\" requires an argument\n",
    opt_format( STATIC_CAST( char, opt == ':' ? optopt : opt ) )
  );
}

/**
 * Prints the **cdecl** usage message, then exits.
 *
 * @param status The status to exit with.  If it is `EX_OK`, prints to standard
 * output; otherwise prints to standard error.
 */
_Noreturn
static void print_usage( int status ) {
  fprintf( status == EX_OK ? stdout : stderr,
    "usage: %s [options] [command...]\n"
    "options:\n"
    "  --alt-tokens        " UOPT(ALT_TOKENS)       "Print alternative tokens.\n"
#ifdef YYDEBUG
    "  --bison-debug       " UOPT(BISON_DEBUG)      "Print Bison debug output.\n"
#endif /* YYDEBUG */
    "  --color=WHEN        " UOPT(COLOR)            "Colorize output WHEN [default: not_file].\n"
    "  --config=FILE       " UOPT(CONFIG)           "Configuration file path [default: ~/." CONF_FILE_NAME_DEFAULT "].\n"
#ifdef ENABLE_CDECL_DEBUG
    "  --debug             " UOPT(CDECL_DEBUG)      "Print " CDECL " debug output.\n"
#endif /* ENABLE_CDECL_DEBUG */
    "  --digraphs          " UOPT(DIGRAPHS)         "Print digraphs.\n"
    "  --east-const        " UOPT(EAST_CONST)       "Print in \"east const\" form.\n"
    "  --echo-commands     " UOPT(ECHO_COMMANDS)    "Echo commands given before corresponding output.\n"
    "  --explain           " UOPT(EXPLAIN)          "Assume \"explain\" when no other command is given.\n"
    "  --explicit-ecsu=WHEN" UOPT(EXPLICIT_ECSU)    "Print \"class\", \"struct\", \"union\" explicitly WHEN.\n"
    "  --explicit-int=WHEN " UOPT(EXPLICIT_INT)     "Print \"int\" explicitly WHEN.\n"
    "  --file=FILE         " UOPT(FILE)             "Read from FILE [default: stdin].\n"
#ifdef ENABLE_FLEX_DEBUG
    "  --flex-debug        " UOPT(FLEX_DEBUG)       "Print Flex debug output.\n"
#endif /* ENABLE_FLEX_DEBUG */
    "  --help              " UOPT(HELP)             "Print this help and exit.\n"
    "  --language=LANG     " UOPT(LANGUAGE)         "Use LANG.\n"
    "  --no-config         " UOPT(NO_CONFIG)        "Suppress reading configuration file.\n"
    "  --no-english-types  " UOPT(NO_ENGLISH_TYPES) "Print types in C/C++, not English.\n"
    "  --no-prompt         " UOPT(NO_PROMPT)        "Suppress printing prompts.\n"
    "  --no-semicolon      " UOPT(NO_SEMICOLON)     "Suppress printing final semicolon for declarations.\n"
    "  --no-typedefs       " UOPT(NO_TYPEDEFS)      "Suppress predefining standard types.\n"
    "  --no-using          " UOPT(NO_USING)         "Declare types with typedef, not using, in C++.\n"
    "  --output=FILE       " UOPT(OUTPUT)           "Write to FILE [default: stdout].\n"
    "  --trailing-return   " UOPT(TRAILING_RETURN)  "Print trailing return type in C++.\n"
    "  --trigraphs         " UOPT(TRIGRAPHS)        "Print trigraphs.\n"
    "  --version           " UOPT(VERSION)          "Print version and exit.\n"
    "  --west-pointer      " UOPT(WEST_POINTER)     "Print *, &, and && next to type.\n"
    "\n"
    PACKAGE_NAME " home page: " PACKAGE_URL "\n"
    "Report bugs to: " PACKAGE_BUGREPORT "\n",
    me
  );
  exit( status );
}

/**
 * Convenience macro for printing a `configure` option.
 *
 * @param OPT The option string literal to print (without the leading `--`).
 */
#define PUT_CONFIG_OPT(OPT) BLOCK( \
  fputs( "\n  --" OPT, stdout ); printed_opt = true; )

/**
 * Prints the **cdecl** version and possibly configure feature &amp; package
 * options and whether GNU **readline**(3) is genuine, then exits.
 *
 * @param verbose If `true`, prints configure feature &amp; package options and
 * whether GNU **readline**(3) is genuine.
 */
static void print_version( bool verbose ) {
  puts( PACKAGE_STRING );
  if ( !verbose )
    return;
  fputs( "configure feature & package options:", stdout );
  bool printed_opt = false;
#ifdef ENABLE_ASAN
  PUT_CONFIG_OPT( "enable-asan" );
#endif /* ENABLE_ASAN */
#ifdef NDEBUG
  PUT_CONFIG_OPT( "disable-assert" );
#endif /* NDEBUG */
#ifdef ENABLE_BISON_DEBUG
  PUT_CONFIG_OPT( "enable-bison-debug" );
#endif /* ENABLE_BISON_DEBUG */
#ifndef ENABLE_CDECL_DEBUG
  PUT_CONFIG_OPT( "disable-cdecl-debug" );
#endif /* ENABLE_CDECL_DEBUG */
#ifdef ENABLE_COVERAGE
  PUT_CONFIG_OPT( "enable-coverage" );
#endif /* ENABLE_COVERAGE */
#ifdef ENABLE_FLEX_DEBUG
  PUT_CONFIG_OPT( "enable-flex-debug" );
#endif /* ENABLE_FLEX_DEBUG */
#ifdef ENABLE_MSAN
  PUT_CONFIG_OPT( "enable-msan" );
#endif /* ENABLE_MSAN */
#ifndef WITH_READLINE
  PUT_CONFIG_OPT( "without-readline" );
#endif /* WITH_READLINE */
#ifndef ENABLE_TERM_SIZE
  PUT_CONFIG_OPT( "disable-term-size" );
#endif /* ENABLE_TERM_SIZE */
#ifdef ENABLE_UBSAN
  PUT_CONFIG_OPT( "enable-ubsan" );
#endif /* ENABLE_UBSAN */
  if ( !printed_opt )
    fputs( " none", stdout );
  putchar( '\n' );
#ifdef WITH_READLINE
  printf( "genuine GNU readline(3): %s\n", HAVE_GENUINE_GNU_READLINE ? "yes" : "no" );
#endif /* WITH_READLINE */
}

////////// extern functions ///////////////////////////////////////////////////

void cli_option_init( int *pargc, char const **pargv[const] ) {
  ASSERT_RUN_ONCE();
  assert( pargc != NULL );
  assert( pargv != NULL );

  opt_lang = is_cppdecl() ? LANG_CPP_NEW : LANG_C_NEW;
#ifdef ENABLE_FLEX_DEBUG
  //
  // When -d is specified, Flex enables debugging by default -- undo that.
  //
  opt_flex_debug = false;
#endif /* ENABLE_FLEX_DEBUG */
  parse_options( pargc, pargv );
  c_lang_set( opt_lang );
}

struct option const* cli_option_next( struct option const *opt ) {
  return opt == NULL ? CLI_OPTIONS : (++opt)->name == NULL ? NULL : opt;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
