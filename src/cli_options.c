/*
**      cdecl -- C gibberish translator
**      src/cli_options.c
**
**      Copyright (C) 2017-2026  Paul J. Lucas, et al.
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
#include "pjl_config.h"                 /* IWYU pragma: keep */
#include "cli_options.h"
#include "c_lang.h"
#include "cdecl.h"
#include "cdecl_command.h"
#include "color.h"
#include "help.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "strbuf.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>                    /* for strcasecmp(3) */
#include <sysexits.h>

// in ascending option character ASCII order
#define OPT_DIGRAPHS          2
#define OPT_TRIGRAPHS         3
#define OPT_ALT_TOKENS        a
#define OPT_NO_BUFFER_STDOUT  b
#ifdef ENABLE_BISON_DEBUG
#define OPT_BISON_DEBUG       B
#endif /* ENABLE_BISON_DEBUG */
#define OPT_CONFIG            c
#define OPT_NO_CONFIG         C
#define OPT_CDECL_DEBUG       d
#define OPT_EAST_CONST        e
#define OPT_ECHO_COMMANDS     E
#define OPT_FILE              f
#ifdef ENABLE_FLEX_DEBUG
#define OPT_FLEX_DEBUG        F
#endif /* ENABLE_FLEX_DEBUG */
#define OPT_HELP              h
#define OPT_EXPLICIT_INT      i
#define OPT_INFER_COMMAND     I
#define OPT_COLOR             k
#define OPT_COMMANDS          K
#define OPT_LINENO            L
#define OPT_PERMISSIVE_TYPES  p
#define OPT_OUTPUT            o
#define OPT_OPTIONS           O
#define OPT_NO_PROMPT         P
#define OPT_TRAILING_RETURN   r
#define OPT_NO_SEMICOLON      s
#define OPT_EXPLICIT_ECSU     S
#define OPT_NO_TYPEDEFS       t
#define OPT_NO_ENGLISH_TYPES  T
#define OPT_NO_USING          u
#define OPT_VERSION           v
#define OPT_WEST_DECL         w
#define OPT_LANGUAGE          x

/// Command-line short option as a character literal.
#define COPT(X)                   CHARIFY(OPT_##X)

/// Command-line option as a string literal.
#define SOPT(X)                   STRINGIFY(OPT_##X)

/// @endcond

/**
 * @addtogroup cli-options-group
 * @{
 */

/**
 * Prints that \a VALUE is an invalid value for \a OPT and what it must be
 * instead to standard error and exits.
 *
 * @param OPT The option _without_ the `OPT_` prefix.
 * @param VALUE The invalid value for \a OPT.
 * @param FORMAT The `printf()` format string literal to use.
 * @param ... The `printf()` arguments.
 */
#define INVALID_OPT_VALUE(OPT,VALUE,FORMAT,...)           \
  fatal_error( EX_USAGE,                                  \
    "\"%s\": invalid value for %s; must be " FORMAT "\n", \
    (VALUE), get_opt_format( COPT(OPT) )                  \
    VA_OPT( (,), __VA_ARGS__ ) __VA_ARGS__                \
  )

///////////////////////////////////////////////////////////////////////////////

/**
 * Long command-line options.
 *
 * @sa CLI_OPTIONS_HELP
 */
static struct option const CLI_OPTIONS[] = {
  //
  // If this is updated, ensure the following are updated to match:
  //
  //  1. Calls to check_opt_exclusive().
  //  2. Calls to check_opt_mutually_exclusive().
  //  3. The corresponding "set" option in SET_OPTIONS in set_options.c.
  //
  { L_OPT_alt_tokens,       no_argument,        NULL, COPT(ALT_TOKENS)        },
#ifdef ENABLE_BISON_DEBUG
  { L_OPT_bison_debug,      no_argument,        NULL, COPT(BISON_DEBUG)       },
#endif /* ENABLE_BISON_DEBUG */
  { L_OPT_color,            required_argument,  NULL, COPT(COLOR)             },
  { L_OPT_commands,         no_argument,        NULL, COPT(COMMANDS)          },
  { L_OPT_config,           required_argument,  NULL, COPT(CONFIG)            },
  { L_OPT_debug,            optional_argument,  NULL, COPT(CDECL_DEBUG)       },
  { L_OPT_digraphs,         no_argument,        NULL, COPT(DIGRAPHS)          },
  { L_OPT_east_const,       no_argument,        NULL, COPT(EAST_CONST)        },
  { L_OPT_echo_commands,    no_argument,        NULL, COPT(ECHO_COMMANDS)     },
  { L_OPT_explicit_ecsu,    required_argument,  NULL, COPT(EXPLICIT_ECSU)     },
  { L_OPT_explicit_int,     required_argument,  NULL, COPT(EXPLICIT_INT)      },
  { L_OPT_file,             required_argument,  NULL, COPT(FILE)              },
#ifdef ENABLE_FLEX_DEBUG
  { L_OPT_flex_debug,       no_argument,        NULL, COPT(FLEX_DEBUG)        },
#endif /* ENABLE_FLEX_DEBUG */
  { L_OPT_help,             no_argument,        NULL, COPT(HELP)              },
  { L_OPT_infer_command,    no_argument,        NULL, COPT(INFER_COMMAND)     },
  { L_OPT_language,         required_argument,  NULL, COPT(LANGUAGE)          },
  { L_OPT_lineno,           required_argument,  NULL, COPT(LINENO)            },
  { "no-buffer-stdout",     no_argument,        NULL, COPT(NO_BUFFER_STDOUT)  },
  { "no-config",            no_argument,        NULL, COPT(NO_CONFIG)         },
  { "no-english-types",     no_argument,        NULL, COPT(NO_ENGLISH_TYPES)  },
  { "no-prompt",            no_argument,        NULL, COPT(NO_PROMPT)         },
  { "no-semicolon",         no_argument,        NULL, COPT(NO_SEMICOLON)      },
  { "no-typedefs",          no_argument,        NULL, COPT(NO_TYPEDEFS)       },
  { "no-using",             no_argument,        NULL, COPT(NO_USING)          },
  { L_OPT_options,          no_argument,        NULL, COPT(OPTIONS)           },
  { L_OPT_output,           required_argument,  NULL, COPT(OUTPUT)            },
  { L_OPT_permissive_types, no_argument,        NULL, COPT(PERMISSIVE_TYPES)  },
  { L_OPT_trailing_return,  no_argument,        NULL, COPT(TRAILING_RETURN)   },
  { L_OPT_trigraphs,        no_argument,        NULL, COPT(TRIGRAPHS)         },
  { L_OPT_version,          no_argument,        NULL, COPT(VERSION)           },
  { L_OPT_west_decl,        required_argument,  NULL, COPT(WEST_DECL)         },
  { NULL,                   0,                  NULL, 0                       }
};

/**
 * Command-line options help.
 *
 * @note It is indexed by short option characters.
 *
 * @sa CLI_OPTIONS
 * @sa get_opt_help()
 */
static char const *const CLI_OPTIONS_HELP[] = {
  [ COPT(ALT_TOKENS) ] = "Print alternative tokens",
#ifdef ENABLE_BISON_DEBUG
  [ COPT(BISON_DEBUG) ] = "Print Bison debug output",
#endif /* ENABLE_BISON_DEBUG */
  [ COPT(COLOR) ] = "Colorize output [default: not_file]",
  [ COPT(COMMANDS) ] = "Print commands (for shell completion)",
  [ COPT(CONFIG) ] = "Configuration file path [default: ~/" CONF_FILE_NAME_DEFAULT "]",
  [ COPT(CDECL_DEBUG) ] = "Print " CDECL " debug output",
  [ COPT(DIGRAPHS) ] = "Print digraphs",
  [ COPT(EAST_CONST) ] = "Print in \"east const\" form",
  [ COPT(ECHO_COMMANDS) ] = "Echo commands given before corresponding output",
  [ COPT(EXPLICIT_ECSU) ] = "Print \"class\", \"struct\", \"union\" explicitly",
  [ COPT(EXPLICIT_INT) ] = "Print \"int\" explicitly",
  [ COPT(FILE) ] = "Read from file [default: stdin]",
#ifdef ENABLE_FLEX_DEBUG
  [ COPT(FLEX_DEBUG) ] = "Print Flex debug output",
#endif /* ENABLE_FLEX_DEBUG */
  [ COPT(HELP) ] = "Print this help and exit",
  [ COPT(INFER_COMMAND) ] = "Try to infer command when none is given",
  [ COPT(LANGUAGE) ] = "Use language",
  [ COPT(LINENO) ] = "Add to all line numbers in messages",
  [ COPT(NO_BUFFER_STDOUT) ] = "Set stdout to unbuffered",
  [ COPT(NO_CONFIG) ] = "Suppress reading configuration file",
  [ COPT(NO_ENGLISH_TYPES) ] = "Print types in C/C++, not English",
  [ COPT(NO_PROMPT) ] = "Suppress printing prompts",
  [ COPT(NO_SEMICOLON) ] = "Suppress printing final semicolon for declarations",
  [ COPT(NO_TYPEDEFS) ] = "Suppress predefining standard types",
  [ COPT(NO_USING) ] = "Declare types with typedef, not using, in C++",
  [ COPT(OPTIONS) ] = "Print command-line options (for shell completion)",
  [ COPT(OUTPUT) ] = "Write to file [default: stdout]",
  [ COPT(PERMISSIVE_TYPES) ] = "Permit other language keywords as types",
  [ COPT(TRAILING_RETURN) ] = "Print trailing return type in C++",
  [ COPT(TRIGRAPHS) ] = "Print trigraphs",
  [ COPT(VERSION) ] = "Print version and exit",
  [ COPT(WEST_DECL) ] = "Print *, &, and && next to type",
};

// local variables
static bool         is_opt_given[128];  ///< Table of options that were given.

// local functions
NODISCARD
static char const*  get_opt_format( char ),
                 *  get_opt_long( char );

static void         print_commands( void );
static void         print_options( void );

_Noreturn
static void         print_usage( int );

static void         print_version( bool );

////////// local functions ////////////////////////////////////////////////////

/**
 * If \a opt was given, checks that _only_ it was given and, if not, prints an
 * error message and exits; if \a opt was not given, does nothing.
 *
 * @param opt The option to check for.
 *
 * @sa check_opt_mutually_exclusive()
 */
static void check_opt_exclusive( char opt ) {
  if ( !is_opt_given[ STATIC_CAST( unsigned, opt ) ] )
    return;
  for ( size_t i = '0'; i < ARRAY_SIZE( is_opt_given ); ++i ) {
    char const curr_opt = STATIC_CAST( char, i );
    if ( curr_opt == opt )
      continue;
    if ( is_opt_given[ STATIC_CAST( unsigned, curr_opt ) ] ) {
      fatal_error( EX_USAGE,
        "%s can be given only by itself\n",
        get_opt_format( opt )
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
 *
 * @sa check_opt_exclusive()
 */
static void check_opt_mutually_exclusive( char opt, char const *opts ) {
  assert( opts != NULL );
  if ( !is_opt_given[ STATIC_CAST( unsigned, opt ) ] )
    return;
  for ( ; *opts != '\0'; ++opts ) {
    assert( *opts != opt );
    if ( is_opt_given[ STATIC_CAST( unsigned, *opts ) ] ) {
      fatal_error( EX_USAGE,
        "%s and %s are mutually exclusive\n",
        get_opt_format( opt ),
        get_opt_format( *opts )
      );
    }
  } // for
}

/**
 * Checks option combinations for semantic errors.
 */
static void check_options( void ) {
  check_opt_exclusive( COPT(HELP) );
  check_opt_exclusive( COPT(VERSION) );

  check_opt_mutually_exclusive( COPT(COMMANDS),
    SOPT(ALT_TOKENS)
    SOPT(COLOR)
    SOPT(DIGRAPHS)
    SOPT(EAST_CONST)
    SOPT(EXPLICIT_ECSU)
    SOPT(EXPLICIT_INT)
    SOPT(FILE)
    SOPT(INFER_COMMAND)
    SOPT(NO_ENGLISH_TYPES)
    SOPT(NO_PROMPT)
    SOPT(NO_SEMICOLON)
    SOPT(NO_TYPEDEFS)
    SOPT(NO_USING)
    SOPT(OPTIONS)
    SOPT(TRAILING_RETURN)
    SOPT(TRIGRAPHS)
    SOPT(WEST_DECL)
  );

  check_opt_mutually_exclusive( COPT(OPTIONS),
    SOPT(ALT_TOKENS)
    SOPT(COLOR)
    SOPT(COMMANDS)
    SOPT(DIGRAPHS)
    SOPT(EAST_CONST)
    SOPT(EXPLICIT_ECSU)
    SOPT(EXPLICIT_INT)
    SOPT(FILE)
    SOPT(INFER_COMMAND)
    SOPT(NO_ENGLISH_TYPES)
    SOPT(NO_PROMPT)
    SOPT(NO_SEMICOLON)
    SOPT(NO_TYPEDEFS)
    SOPT(NO_USING)
    SOPT(TRAILING_RETURN)
    SOPT(TRIGRAPHS)
    SOPT(WEST_DECL)
  );

  check_opt_mutually_exclusive( COPT(DIGRAPHS), SOPT(TRIGRAPHS) );
  check_opt_mutually_exclusive( COPT(FILE), SOPT(LINENO) );
}

/**
 * Formats an option as `[--%%s/]-%%c` where `%%s` is the long option (if any)
 * and `%%c` is the short option.
 *
 * @param short_opt The short option (along with its corresponding long option,
 * if any) to format.
 * @return Returns said formatted string.
 *
 * @warning The pointer returned is to one of a small number of static buffers,
 * so you can't do something like call this more than twice in the same
 * `printf()` statement.
 */
NODISCARD
static char const* get_opt_format( char short_opt ) {
  static strbuf_t sbufs[2];
  static unsigned buf_index;

  strbuf_t *const sbuf = &sbufs[ buf_index++ % ARRAY_SIZE( sbufs ) ];
  strbuf_reset( sbuf );

  char const *const long_opt = get_opt_long( short_opt );
  return strbuf_printf(
    sbuf, "%s%s%s-%c",
    long_opt[0] != '\0' ? "--" : "",
    long_opt,
    long_opt[0] != '\0' ? "/" : "",
    short_opt
  );
}

/**
 * Gets the help message for \a opt.
 *
 * @param opt The option to get the help for.
 * @return Returns said help message.
 */
NODISCARD
static char const* get_opt_help( int opt ) {
  assert( opt > 0 );
  assert( (unsigned)opt < ARRAY_SIZE( CLI_OPTIONS_HELP ) );
  char const *const help = CLI_OPTIONS_HELP[ opt ];
  assert( help != NULL );
  return help;
}

/**
 * Gets the corresponding name of the long option for \a short_opt.
 *
 * @param short_opt The short option to get the corresponding long option for.
 * @return Returns said name or the empty string if none.
 */
NODISCARD
static char const* get_opt_long( char short_opt ) {
  FOREACH_CLI_OPTION( opt ) {
    if ( opt->val == short_opt )
      return opt->name;
  } // for
  return "";                            // LCOV_EXCL_LINE
}

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
    len += 1 + STATIC_CAST( size_t, opt->has_arg );
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
  };

  assert( when != NULL );

  FOREACH_ARRAY_ELEMENT( colorize_map_t, m, COLORIZE_MAP ) {
    if ( strcasecmp( when, m->map_when ) == 0 )
      return m->map_colorization;
  } // for

  // name not found: construct valid name list for an error message
  strbuf_t when_sbuf;
  strbuf_init( &when_sbuf );
  bool comma = false;
  FOREACH_ARRAY_ELEMENT( colorize_map_t, m, COLORIZE_MAP )
    strbuf_sepsn_puts( &when_sbuf, ", ", 2, &comma, m->map_when );
  INVALID_OPT_VALUE( COLOR, when, "%s", when_sbuf.str );
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
  INVALID_OPT_VALUE( LANGUAGE, lang_name, "%s", langs_sbuf.str );
}

/**
 * Parses command-line options.
 *
 * @param pargc A pointer to the argument count from main().
 * @param pargv A pointer to the argument values from main().
 */
static void parse_options( int *const pargc, char const *const *pargv[] ) {
  opterr = 0;                           // suppress default error message

  char const *      fout_path = "-";
  int               opt;
  bool              opt_buffer_stdout = true;
  bool              opt_commands = false;
  bool              opt_help = false;
  bool              opt_no_config = false;
  bool              opt_options = false;
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
#ifdef ENABLE_BISON_DEBUG
      case COPT(BISON_DEBUG):
        opt_bison_debug = true;
        break;
#endif /* ENABLE_BISON_DEBUG */
      case COPT(CDECL_DEBUG):
        if ( !parse_cdecl_debug( empty_if_null( optarg ) ) ) {
          INVALID_OPT_VALUE(
            CDECL_DEBUG, optarg, "[%s]+|*|-", OPT_CDECL_DEBUG_ALL
          );
        }
        break;
      case COPT(COLOR):
        opt_color_when = parse_color_when( optarg );
        break;
      case COPT(COMMANDS):
        opt_commands = true;
        break;
      case COPT(CONFIG):
        if ( *SKIP_WS( optarg ) == '\0' )
          goto missing_arg;
        opt_config_path = optarg;
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
      case COPT(EXPLICIT_ECSU):
        if ( !parse_explicit_ecsu( optarg ) )
          INVALID_OPT_VALUE( EXPLICIT_ECSU, optarg, "[%s]+|*|-", OPT_ECSU_ALL );
        break;
      case COPT(EXPLICIT_INT):
        if ( !parse_explicit_int( optarg ) ) {
          INVALID_OPT_VALUE(
            EXPLICIT_INT, optarg, "i|u|{[u]{i|s|l[l]}[,]}+|*|-"
          );
        }
        break;
      case COPT(FILE):
        if ( *SKIP_WS( optarg ) == '\0' )
          goto missing_arg;
        opt_file = optarg;
        break;
#ifdef ENABLE_FLEX_DEBUG
      case COPT(FLEX_DEBUG):
        opt_flex_debug = true;
        break;
#endif /* ENABLE_FLEX_DEBUG */
      case COPT(HELP):
        opt_help = true;
        break;
      case COPT(INFER_COMMAND):
        opt_infer_command = true;
        break;
      case COPT(LANGUAGE):
        opt_lang_id = parse_lang( optarg );
        break;
      case COPT(LINENO):;
        unsigned long long n = check_strtoull( optarg, 1, USHRT_MAX );
        if ( n == ULLONG_MAX ) {
          fatal_error( EX_USAGE,
            "\"%s\": invalid value for %s; must be in range 1-%u\n",
            optarg, get_opt_format( COPT(LINENO) ),
            STATIC_CAST( unsigned, USHRT_MAX )
          );
        }
        opt_lineno = STATIC_CAST( unsigned, n );
        break;
      case COPT(NO_BUFFER_STDOUT):
        opt_buffer_stdout = false;
        break;
      case COPT(NO_CONFIG):
        opt_no_config = true;
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
        if ( opt_predef_types > PREDEF_TYPES_NONE )
          --opt_predef_types;
        break;
      case COPT(NO_USING):
        opt_using = false;
        break;
      case COPT(PERMISSIVE_TYPES):
        opt_permissive_types = true;
        break;
      case COPT(TRAILING_RETURN):
        opt_trailing_ret = true;
        break;
      case COPT(TRIGRAPHS):
        opt_graph = C_GRAPH_TRI;
        break;
      case COPT(OPTIONS):
        opt_options = true;
        break;
      case COPT(OUTPUT):
        if ( *SKIP_WS( optarg ) == '\0' )
          goto missing_arg;
        fout_path = optarg;
        break;
      case COPT(VERSION):
        ++opt_version;
        break;
      case COPT(WEST_DECL):
        if ( !parse_west_decl( optarg ) ) {
          INVALID_OPT_VALUE(
            WEST_DECL, optarg, "[%s]+|*|-", OPT_WEST_DECL_ALL
          );
        }
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
    is_opt_given[ opt ] = true;
  } // for

  FREE( short_opts );

  *pargc -= optind;
  *pargv += optind;

  check_options();

  if ( strcmp( opt_file, "-" ) != 0 && !freopen( opt_file, "r", stdin ) )
    fatal_error( EX_NOINPUT, "\"%s\": %s\n", opt_file, STRERROR() );

  if ( strcmp( fout_path, "-" ) != 0 && !freopen( fout_path, "w", stdout ) )
    fatal_error( EX_CANTCREAT, "\"%s\": %s\n", fout_path, STRERROR() );

  if ( !opt_buffer_stdout )
    setvbuf( stdout, /*buf=*/NULL, _IONBF, /*size=*/0 );

  if ( opt_commands ) {
    if ( *pargc > 0 )                   // cdecl -K foo
      print_usage( EX_USAGE );
    print_commands();
    exit( EX_OK );
  }

  if ( opt_help )
    print_usage( *pargc > 0 ? EX_USAGE : EX_OK );

  if ( opt_no_config )
    opt_read_config = false;
  else if ( opt_config_path != NULL )
    opt_read_config = true;

  if ( opt_options ) {
    if ( *pargc > 0 )                   // cdecl -O foo
      print_usage( EX_USAGE );
    print_options();
    exit( EX_OK );
  }

  if ( opt_version > 0 ) {
    if ( *pargc > 0 )                   // cdecl -v foo
      print_usage( EX_USAGE );
    // LCOV_EXCL_START -- since the version changes
    print_version( /*verbose=*/opt_version > 1 );
    exit( EX_OK );
    // LCOV_EXCL_STOP
  }

  return;

invalid_opt:;
  // Determine whether the invalid option was short or long.
  char const *invalid_opt = (*pargv)[ optind - 1 ];
  if ( invalid_opt != NULL && STRNCMPLIT( invalid_opt, "--" ) == 0 ) {
    invalid_opt += STRLITLEN( "--" );
    EPRINTF( "%s: \"%s\": invalid option", prog_name, invalid_opt );
    if ( !print_suggestions( DYM_CLI_OPTIONS, invalid_opt ) )
      goto use_help;
    EPUTC( '\n' );
    exit( EX_USAGE );
  }
  EPRINTF( "%s: '%c': invalid option", prog_name, STATIC_CAST( char, optopt ) );

use_help:
  print_use_help();
  exit( EX_USAGE );

missing_arg:
  fatal_error( EX_USAGE,
    "\"%s\" requires an argument\n",
    get_opt_format( STATIC_CAST( char, opt == ':' ? optopt : opt ) )
  );
}

/**
 * Prints all **cdecl** commands for the current language that can be given on
 * the command-line.
 *
 * @remarks The use-case is for a shell completion function to be able to call
 * **cdecl** to generate the commands to complete.
 *
 * @sa print_options()
 */
static void print_commands( void ) {
  FOREACH_CDECL_COMMAND( command ) {
    if ( command->kind == CDECL_COMMAND_LANG_ONLY )
      continue;
    if ( !opt_lang_is_any( command->lang_ids ) )
      continue;
    puts( command->literal );
  } // for
}

/**
 * Prints all **cdecl** command-line options in an easily parsable format.
 *
 * @remarks The use-case is for a shell completion function to be able to call
 * **cdecl** to generate the options to complete.
 *
 * @sa print_commands()
 */
static void print_options( void ) {
  FOREACH_CLI_OPTION( opt )
    PRINTF( "--%s -%c %s\n", opt->name, opt->val, get_opt_help( opt->val ) );
}

/**
 * Prints the **cdecl** usage message, then exits.
 *
 * @param status The status to exit with.  If it is `EX_OK`, prints to standard
 * output; otherwise prints to standard error.
 */
_Noreturn
static void print_usage( int status ) {
  // pre-flight to calculate longest long option length
  size_t longest_opt_len = 0;
  FOREACH_CLI_OPTION( opt ) {
    size_t opt_len = strlen( opt->name );
    switch ( opt->has_arg ) {
      case no_argument:
        break;
      case optional_argument:
        opt_len += STRLITLEN( "[=ARG]" );
        break;
      case required_argument:
        opt_len += STRLITLEN( "=ARG" );
        break;
    } // switch
    if ( opt_len > longest_opt_len )
      longest_opt_len = opt_len;
  } // for

  FILE *const fout = status == EX_OK ? stdout : stderr;
  FPRINTF( fout, "usage: %s [options] [command...]\noptions:\n", prog_name );

  FOREACH_CLI_OPTION( opt ) {
    FPRINTF( fout, "  --%s", opt->name );
    size_t opt_len = strlen( opt->name );
    switch ( opt->has_arg ) {
      case no_argument:
        break;
      case optional_argument:
        opt_len += STATIC_CAST( size_t, fprintf( fout, "[=ARG]" ) );
        break;
      case required_argument:
        opt_len += STATIC_CAST( size_t, fprintf( fout, "=ARG" ) );
        break;
    } // switch
    assert( opt_len <= longest_opt_len );
    FPUTNSP( longest_opt_len - opt_len, fout );
    FPRINTF( fout, " (-%c) %s.\n", opt->val, get_opt_help( opt->val ) );
  } // for

  FPUTS(
    "\n"
    PACKAGE_NAME " home page: " PACKAGE_URL "\n"
    "Report bugs to: " PACKAGE_BUGREPORT "\n",
    fout
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

// LCOV_EXCL_START
/**
 * Prints the **cdecl** version and possibly configure feature &amp; package
 * options and whether GNU **readline**(3) is genuine.
 *
 * @param verbose If `true`, prints configure feature &amp; package options and
 * whether GNU **readline**(3) is genuine.
 */
static void print_version( bool verbose ) {
  PUTS(
    PACKAGE_STRING "\n"
    "Copyright (C) " CDECL_COPYRIGHT_YEAR " " CDECL_PRIMARY_AUTHOR "\n"
    "License " CDECL_LICENSE " <" CDECL_LICENSE_URL ">.\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY to the extent permitted by law.\n"
  );
  if ( !verbose )
    return;

  PUTS( "\nconfigure feature & package options:" );
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
    PUTS( " none" );
  putchar( '\n' );
#ifdef WITH_READLINE
  printf( "genuine GNU readline(3): %s\n", HAVE_GENUINE_GNU_READLINE ? "yes" : "no" );
#endif /* WITH_READLINE */
}
// LCOV_EXCL_STOP

////////// extern functions ///////////////////////////////////////////////////

struct option const* cli_option_next( struct option const *opt ) {
  return opt == NULL ? CLI_OPTIONS : (++opt)->name == NULL ? NULL : opt;
}

void cli_options_init( int *const pargc, char const *const *pargv[] ) {
  ASSERT_RUN_ONCE();
  assert( pargc != NULL );
  assert( pargv != NULL );

  opt_lang_id = is_cppdecl() ? LANG_CPP_NEW : LANG_C_NEW;
#ifdef ENABLE_FLEX_DEBUG
  //
  // When -d is specified, Flex enables debugging by default -- undo that.
  //
  opt_flex_debug = false;
#endif /* ENABLE_FLEX_DEBUG */

  if ( cdecl_is_testing ) {
    //
    // Don't read user's ~/.cdeclrc, if any, by default since it'll interfere
    // with testing.
    //
    opt_read_config = false;
  }

  parse_options( pargc, pargv );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
