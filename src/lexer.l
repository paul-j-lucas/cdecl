/*
**      cdecl -- C gibberish translator
**      src/lexer.l
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
 * Defines helper macros, data structures, variables, functions, and the
 * tokenizer for C/C++ declarations.
 */

/** @cond DOXYGEN_IGNORE */

%option warn
%option yylineno

%top {
#include "pjl_config.h"                 /* IWYU pragma: keep */
}

%{
/** @endcond */

// local
#include "lexer.h"
#include "c_ast.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_sname.h"
#include "c_typedef.h"
#include "cdecl.h"
#include "cdecl_keyword.h"
#include "literals.h"
#include "options.h"
#include "p_keyword.h"
#include "print.h"
#include "red_black.h"
#include "slist.h"
#include "strbuf.h"
#include "util.h"
#include "cdecl_parser.h"               /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL, size_t */
#include <stdio.h>
#include <stdlib.h>                     /* for strtol(3) */
#include <string.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <wordexp.h>

// Silence these warnings for Flex-generated code.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * @addtogroup lexer-group
 * @{
 */

/**
 * Switch to lexer \a STATE start-state saving the current start-state.
 *
 * @param STATE The start-state to switch to.
 *
 * @sa #BEGIN_RETURN
 */
#define BEGIN_CALL(STATE) \
  BLOCK( yy_start_stack_push(); lexer_begin( STATE ); )

/**
 * Switch to the previous lexer start-state saved by #BEGIN_CALL().
 *
 * @sa #BEGIN_CALL()
 */
#define BEGIN_RETURN              lexer_begin( yy_start_stack_pop() )

/**
 * A **cdecl**-specfic version of Flex's `REJECT` that also undoes
 * #YY_USER_ACTION.
 *
 * @note This _must_ be used instead of Flex's `REJECT`.
 */
#define CDECL_REJECT \
  BLOCK( token_column -= STATIC_CAST( c_loc_num_t, yyleng ); REJECT; )

/**
 * C++ raw string delimiter maximum length.
 */
#define RSTR_DELIM_LEN_MAX        16u

/**
 * Overrides Flex's fatal error message to print the message in our format and
 * also exit with the status code we want.
 *
 * @param MSG The error message to print.
 */
#define YY_FATAL_ERROR(MSG)       lexer_fatal( (MSG) )

/**
 * Overrides Flex's input.
 *
 * @param BUF The buffer to use.
 * @param BYTES_READ Set to the number of bytes read.
 * @param BYTES_MAX The maximum number of bytes to read.
 */
#define YY_INPUT(BUF,BYTES_READ,BYTES_MAX) \
  (BYTES_READ) = lexer_get_input( (BUF), STATIC_CAST( yy_size_t, (BYTES_MAX) ) )

/**
 * This code is inserted by Flex at the beginning of each rule to set the
 * current token location information.
 *
 * @note Flex expects this to end with a `;`.
 */
#define YY_USER_ACTION \
  BLOCK( is_constrained_auto = false; lexer_update_loc(); );

///////////////////////////////////////////////////////////////////////////////

/**
 * Data to keep for a file that is `include`d.
 *
 * @remarks
 * @parblock
 * When a file is included, an <code>%include_file_info</code> is
 * pushed onto \ref include_stack such that:
 *
 *  + \ref prev_orig_path is the unresolved path of the _previous_ include (or
 *    configuration) file, if any, that is the value of \ref cdecl_input_path.
 *
 *  + \ref curr_real_path is the resolved path of the _new_ file about to be
 *    included.  We need to remember the resolved path in order to be able to
 *    remove it from \ref include_set upon EOF.
 *
 * @endparblock
 * @note We need \ref prev_lineno because Flex doesn't restore `yylineno` when
 * `yypop_buffer_state()` is called.
 */
struct include_file_info {
  char const *prev_orig_path;           ///< The previous file's original path.
  char const *curr_real_path;           ///< The current file's real path.
  c_loc_num_t orig_opt_lineno;          ///< Original value of \ref opt_lineno.
  c_loc_num_t prev_lineno;              ///< The file's last line number.
};
typedef struct include_file_info include_file_info_t;

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries.

// extern variables
lexer_find_kind_t lexer_find = LEXER_FIND_ANY;
bool              lexer_in_c_comment;
bool              lexer_is_param_list_decl;
c_keyword_ctx_t   lexer_keyword_ctx;

/// @endcond

// local variables
static bool         digraph_warned;     ///< Printed digraph warning once?
static rb_tree_t    include_set;        ///< Set of resolved include paths.
static slist_t      include_stack;      ///< Stack of include paths.
static strbuf_t     input_sbuf;         ///< Entire current input line.
static size_t       input_sent;         ///< How many bytes returned to Flex.
static bool         is_constrained_auto;///< _Identifier_ followed by `auto`?
static bool         is_eof;             ///< Encountered EOF?
static bool         is_func_like_macro; ///< Defining a function-like macro?
static strbuf_t     str_lit_buf;        ///< String (or character) literal.
static c_loc_t      str_lit_loc;        ///< String literal starting location.
static c_loc_num_t  token_column;       ///< Column position of current token.
static bool         trigraph_warned;    ///< Printed trigraph warning once?

/// C++ raw string literal delimiter.
static char         rstr_delim[ RSTR_DELIM_LEN_MAX + 1/*"*/ + 1/*\0*/ ];

/**
 * Stack of `yylineno` values.
 *
 * @remarks Currently, we need only to remember 1 previous value between
 * lexer_push_string() and lexer_pop_string(), but making this a "stack"
 * future-proofs the code since it can much more easily be extended.
 *
 * @sa yylineno_stack_pop()
 * @sa yylineno_stack_push()
 */
static c_loc_num_t  yylineno_stack[1];

/// Top of \ref yylineno_stack.
static int          yylineno_stack_top = -1;

/**
 * Stack of `YY_START` values.
 *
 * @remarks Currently, we need only to remember 1 previous value, but making
 * this a "stack" future-proofs the code since it can much more easily be
 * extended.
 *
 * @sa yy_start_stack_pop()
 * @sa yy_start_stack_push()
 */
static int        yy_start_stack[1];

/// Top of \ref yy_start_stack.
static int        yy_start_stack_top = -1;

// local functions
static void       lexer_begin( int );

_Noreturn
static void       lexer_fatal( char const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether \a s is null: if so, returns a copy of the empty string.
 *
 * @param s The pointer to check.
 * @return If \a s is null, returns a copy of the empty string; otherwise
 * returns \a s.
 *
 * @sa empty_if_null()
 */
static char* empty_dup_if_null( char *s ) {
  if ( s == NULL )
    s = check_strdup( "" );
  return s;
}

/**
 * Frees all memory associated with \a ifi _including_ \a ifi itself.
 *
 * @param ifi The \ref include_file_info to free.  If NULL, does nothing.
 */
static void ifi_free( include_file_info_t *ifi ) {
  if ( ifi != NULL ) {
    FREE( ifi->prev_orig_path );
    FREE( ifi->curr_real_path );
    free( ifi );
  }
}

/**
 * Frees all memory used by include files.
 *
 * @sa include_init()
 */
static void include_cleanup( void ) {
  slist_cleanup( &include_stack, POINTER_CAST( slist_free_fn_t, &ifi_free ) );
  // Do not pass free() as the second argument since the resolved include path
  // strings are shared with and owned by include_stack.
  rb_tree_cleanup( &include_set, /*free_fn=*/NULL );
}

/**
 * Initializes include files.
 *
 * @sa include_cleanup()
 */
static void include_init( void ) {
  // Must pass RB_DPTR since the resolved include path strings are shared with
  // and owned by include_stack.
  rb_tree_init( &include_set, RB_DPTR, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );
}

/**
 * Lexer-specific wrapper around cdecl_keyword_find() that finds a **cdecl**
 * keyword, but only if we're currently supposed to or we're always supposed to
 * find a particular keyword.
 *
 * @param literal The literal to find.
 * @return Returns a pointer to the corresponding cdecl_keyword or NULL if not
 * found or we're not currently supposed to find it.
 */
NODISCARD
static inline
cdecl_keyword_t const* lexer_cdecl_keyword_find( char const *literal ) {
  cdecl_keyword_t const *const cdk = cdecl_keyword_find( literal );
  if ( cdk == NULL )
    return NULL;
  if ( (lexer_find & LEXER_FIND_CDECL_KEYWORDS) != 0 || cdk->always_find )
    return cdk;
  return NULL;
}

/**
 * Cleans up lexer data at program termination.
 *
 * @note This function is called only via **atexit**(3).
 *
 * @sa lexer_init()
 */
static void lexer_cleanup( void ) {
  include_cleanup();
  strbuf_cleanup( &str_lit_buf );
}

/**
 * Gets a line of input for Flex and keeps a copy for use later if printing an
 * error message.
 *
 * @param buf A pointer to the buffer to write into.
 * @param buf_cap The capacity of \a buf.
 * @return Returns the number of bytes read.
 */
NODISCARD
static yy_size_t lexer_get_input( char *buf, yy_size_t buf_cap ) {
  assert( buf != NULL );

  for (;;) {
    size_t const bytes_read = fread( buf, 1, buf_cap, yyin );
    if ( ferror( yyin ) ) {
      if ( errno != EINTR )
        perror_exit( EX_IOERR );
      clearerr( yyin );
      continue;
    }
    strbuf_reset( &input_sbuf );
    strbuf_putsn( &input_sbuf, buf, bytes_read );
    return bytes_read;
  } // for
}

/**
 * Pops a buffer from Flex's input.
 */
static void lexer_pop_buffer( void ) {
  // The example code in the Flex manual leaks file handles; see:
  // https://stackoverflow.com/a/27512485/99089
  assert( yyin != NULL );
  assert( yyin != stdin );
  fclose( yyin );
  yypop_buffer_state();
  assert( YY_CURRENT_BUFFER != NULL );
}

/**
 * Update the parser's location.
 * @note This is called by Flex via #YY_USER_ACTION.
 */
static void lexer_update_loc( void ) {
  yylloc.first_line   = yylloc.last_line = yylineno;
  yylloc.first_column = token_column;
  yylloc.last_column  = token_column + yyleng - 1;
  token_column += yyleng;
}

/**
 * Resets the token column position upon encountering a newline.
 */
static inline void newline( void ) {
  token_column = 0;
}

/**
 * Parses an integer from \ref yytext.
 *
 * @param base The integer base to use.
 * @return Returns the integer value.
 */
NODISCARD
static int parse_int( int base ) {
  char const *s = yytext;
  bool const is_neg = s[0] == '-';

  if ( is_neg )
    ++s;

  if ( base == 2 ) {
    // Prior to C23, strtol(3) doesn't understand a "0b" (binary) prefix, so
    // skip over it.
    assert( s[0] == '0' );              // String should start with "0b" ...
    assert( tolower( s[1] ) == 'b' );   // ... since it was just lex'd as such.
    s += STRLITLEN( "0b" );
  }

  char no_digit_seps[ MAX_DEC_INT_DIGITS(uintmax_t) + 1/*\0*/ ];

  if ( strchr( s, '\'' ) != NULL ) {
    // Strip digit separator (') characters since strtol(3) doesn't understand
    // them.
    char *t = no_digit_seps;
    do {
      if ( *s != '\'' )
        *t++ = *s;
    } while ( *s++ != '\0' );
    s = no_digit_seps;
  }

  errno = 0;
  long rv = strtol( s, /*endptr=*/NULL, base );
  if ( is_neg )
    rv = -rv;
  if ( unlikely( errno != 0 || rv < INT_MIN || rv > INT_MAX ) ) {
    // LCOV_EXCL_START
    rv = rv < INT_MIN ? INT_MIN : INT_MAX;
    print_warning( &yylloc, "integer out of range; clamped to %ld\n", rv );
    // LCOV_EXCL_STOP
  }

  return STATIC_CAST( int, rv );
}

/**
 * Pops the current input file, if any.
 *
 * @return Returns `true` only if an include file was popped.
 *
 * @sa push_file()
 * @sa https://westes.github.io/flex/manual/Multiple-Input-Buffers.html
 */
NODISCARD
static bool pop_file( void ) {
  if ( slist_empty( &include_stack ) )
    return false;

  include_file_info_t *const ifi = slist_pop_front( &include_stack );
  assert( ifi != NULL );

  rb_node_t *const found_rb = rb_tree_find( &include_set, ifi->curr_real_path );
  assert( found_rb != NULL );
  // No need to delete found_rb->node.data since it's shared with and owned by
  // ifi.
  rb_tree_delete( &include_set, found_rb );

  cdecl_input_path = ifi->prev_orig_path;
  opt_lineno = ifi->orig_opt_lineno;
  yylineno = ifi->prev_lineno;
  ifi_free( ifi );
  lexer_pop_buffer();
  return true;
}

/**
 * Pushes the current input file and sets \a path as the new file to read
 * subsequent input from.
 *
 * @param path The path to read subsequent input from until EOF.  Shell
 * metacharacters, e.g., `~`, are expanded.
 * @param path_loc The location of \a path.
 *
 * @sa pop_file()
 * @sa http://westes.github.io/flex/manual/Multiple-Input-Buffers.html
 */
static void push_file( char const *path, c_loc_t const *path_loc ) {
  path = null_if_empty( path );
  if ( path == NULL ) {
    print_error( path_loc, "empty path\n" );
    return;
  }

  wordexp_t we;
  int const rv_we = wordexp( path, &we, /*flags=*/0 );

  char const *real_path = NULL;

  switch ( rv_we ) {
    case 0:                             // success
      if ( we.we_wordc != 1 ) {
        print_error( path_loc, "\"%s\": too many files\n", path );
        goto done;
      }
      break;
    case WRDE_BADCHAR:
      print_error( path_loc,
        "\"%s\": contains unquoted shell characters\n", path
      );
      return;
    // LCOV_EXCL_START
    case WRDE_SYNTAX:
      print_error( path_loc, "\"%s\": path syntax error\n", path );
      return;
    case WRDE_NOSPACE:                  // unlikely
      print_error( path_loc, "out of memory\n" );
      _Exit( EX_OSERR );
    case WRDE_BADVAL:                   // can't happen
    case WRDE_CMDSUB:                   // can't happen
    default:
      UNEXPECTED_INT_VALUE( rv_we );
    // LCOV_EXCL_STOP
  } // switch

  real_path = realpath( we.we_wordv[0], /*real_buf=*/NULL );
  if ( real_path == NULL ) {
    print_error( path_loc, "\"%s\": could not resolve path\n", path );
    goto done;
  }

  if ( rb_tree_find( &include_set, real_path ) != NULL ) {
    print_error( path_loc, "\"%s\": file previously included\n", path );
    goto done;
  }

  struct stat st;
  if ( stat( real_path, &st ) < 0 ) {
    // LCOV_EXCL_START
    print_error( path_loc, "\"%s\": %s\n", real_path, STRERROR() );
    goto done;
    // LCOV_EXCL_STOP
  }
  if ( !S_ISREG( st.st_mode ) ) {
    // LCOV_EXCL_START
    print_error( path_loc, "\"%s\": not a plain file\n", path );
    goto done;
    // LCOV_EXCL_STOP
  }

  FILE *const include_file = fopen( real_path, "r" );
  if ( include_file == NULL ) {
    // LCOV_EXCL_START
    print_error( path_loc, "\"%s\": %s\n", path, STRERROR() );
    goto done;
    // LCOV_EXCL_STOP
  }

  //
  // Now that we know the path resolves, it wasn't previously included, it's a
  // plain file, and we can open it, we can insert it into include_set.
  //
  PJL_DISCARD_RV(
    rb_tree_insert( &include_set, CONST_CAST( char*, real_path ), 0 )
  );

  include_file_info_t *const ifi = MALLOC( include_file_info_t, 1 );
  *ifi = (include_file_info_t){
    .prev_orig_path = cdecl_input_path,
    .curr_real_path = real_path,
    .orig_opt_lineno = opt_lineno,
    .prev_lineno = yylineno
  };
  slist_push_front( &include_stack, ifi );
  real_path = NULL;                     // now owned by ifi above
  opt_lineno = 0;                       // applies only to original file

  yyin = include_file;
  yypush_buffer_state( yy_create_buffer( yyin, YY_BUF_SIZE ) );
  cdecl_input_path = check_strdup( we.we_wordv[0] );
  yylineno = 1;
  newline();

done:
  FREE( real_path );
  wordfree( &we );                      // call only if rv_we == 0
}

/**
 * Sets the current token to \a token for the current digraph sequence.
 * Additionally, if the current language is older than C95, prints a warning
 * that digraphs are not supported until C95 (only once per parse).
 *
 * @param token The token the digraph maps to.
 *
 * @sa set_trigraph()
 */
static void set_digraph( char const *token ) {
  if ( !OPT_LANG_IS( DIGRAPHS ) && false_set( &digraph_warned ) ) {
    print_warning( &yylloc,
      "digraphs not supported%s\n", C_LANG_WHICH( DIGRAPHS )
    );
  }
  set_yytext( token );
}

/**
 * Sets the current token to \a token for the current trigraph sequence.
 * Additionally, if the current language is K&R&nbsp;C or C++17 or later,
 * prints a warning that trigraphs are not supported (only once per parse).
 *
 * @param token The token the trigraph maps to.
 *
 * @sa set_digraph()
 */
static void set_trigraph( char const *token ) {
  if ( !OPT_LANG_IS( TRIGRAPHS ) && false_set( &trigraph_warned ) ) {
    print_warning( &yylloc,
      "trigraphs not supported%s\n", C_LANG_WHICH( TRIGRAPHS )
    );
  }
  set_yytext( token );
}

/**
 * Pops \ref yylineno_stack into `yylineno`.
 *
 * @sa yylineno_stack_push()
 */
static void yylineno_stack_pop( void ) {
  assert( yylineno_stack_top >= 0 );
  yylineno = yylineno_stack[ yylineno_stack_top-- ];
}

/**
 * Pushes the current `yylineno` value onto \ref yylineno_stack.
 *
 * @sa yylineno_stack_pop()
 */
static void yylineno_stack_push( void ) {
  ++yylineno_stack_top;
  assert( yylineno_stack_top < ARRAY_SIZE( yylineno_stack ) );
  yylineno_stack[ yylineno_stack_top ] = yylineno;
}

/**
 * Pops \ref yy_start_stack.
 *
 * @return Returns a previously pushed `YY_START` value.
 *
 * @sa yy_start_stack_push()
 */
PJL_DISCARD
static int yy_start_stack_pop( void ) {
  assert( yy_start_stack_top >= 0 );
  return yy_start_stack[ yy_start_stack_top-- ];
}

/**
 * Pushes the current `YY_START` value onto \ref yy_start_stack.
 *
 * @sa yy_start_stack_pop()
 */
static void yy_start_stack_push( void ) {
  ++yy_start_stack_top;
  assert( yy_start_stack_top < ARRAY_SIZE( yy_start_stack ) );
  yy_start_stack[ yy_start_stack_top ] = YY_START;
}

/**
 * Standard lex function to know whether to continue parsing upon reaching EOF.
 *
 * @return Returns 1 if done or 0 to continue parsing (a new file set via
 * `yyin`).
 */
static int yywrap( void ) {
  return /*done=*/1;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/// @cond DOXYGEN_IGNORE

%}

L             [A-Za-z_]
B             [01]
O             [0-7]
D             [0-9]
H             [0-9A-Fa-f]
NI            [^A-Za-z_0-9]
S             [ \f\r\t\v]
NS            [^ \f\n\r\t\v]

identifier    {L}({L}|{D})*
sname         {identifier}({S}*::{S}*{identifier})+
dtor_sname    ({identifier}{S}*::{S}*)+(~|compl{S}){S}*{identifier}
oper_sname    ({identifier}{S}*::{S}*)+operator{NI}
hyphenated    [a-z]+-([a-z]+-)*[a-z]+

glob_scope    \*?({identifier}\*?)*
glob          (\*\*|{glob_scope})({S}*::{S}*{glob_scope})*

cstr_pfx      L|u8?|U
rstr_pfx      {cstr_pfx}?R\"[^ \f\n\r\t\v()\\]*"("

flt_sfx       [flFL]
hex_pfx       0[xX]
int_sfx       [lL][lL]?[uU]?|wb|WB|[uU]([lL][lL]?|wb|WB|[zZ])?|[zZ][uU]?
set_option    [^=; \f\n\r\t\v]+

bin_int       -?0[bB]{B}+('{B}+)*{int_sfx}?
oct_int       -?0{O}*('{O}+)*{int_sfx}?
dec_int       -?[1-9]{D}*('{D}+)*{int_sfx}?
hex_int       -?{hex_pfx}{H}+('{H}+)*{int_sfx}?

exp           [eE][+-]?{D}+
bexp          [pP][+-]?{D}+
dec_frac      ({D}*"."{D}+)|({D}+".")
hex_frac      ({H}*"."{H}+)|({H}+".")
dec_flt       ({dec_frac}{exp}?)|({D}+{exp})
hex_flt       {hex_pfx}({hex_frac}|{H}+){bexp}

flt_lit       -?({dec_flt}|{hex_flt}){flt_sfx}?

/*
 * For C-style comments.
 */
%x X_C_COMMENT

/*
 * For the "expand" command.
 */
%x X_EXPAND

/*
 * For "include" files.
 */
%x X_INCLUDE

/*
 * For the C preprocessor.
 */
%x X_PRE_COMMAND X_PRE_TOKENS

/*
 * For the "set" command, we want to allow (almost) any character sequence for
 * the command's options.
 */
%x X_SET

/*
 * For the "show" command, we want to allow globs (scoped names containing
 * `*`).
 */
%s S_SHOW

/*
 * For C character and string literals.
 */
%x X_CHAR X_STR X_RSTR

%%
              /*
               * Special case: if "_Atomic" is immediately followed by a '(',
               * it is interpreted as a type specifier, not as a type
               * qualifier.
               */
_Atomic/{S}*\( {
                return Y__Atomic_SPEC;
              }

              /*
               * Special case: if "auto" is optionally followed by "const"
               * and/or "volatile", optionally followed by either '&' or "&&",
               * and followed by '[', it's a C++ structured binding "auto" ---
               * unless we're currently parsing a function-like parameter list
               * (see below).
               *
               * For example, given:
               *
               *      auto const [N]        // structured binding
               *      auto const x[N]       // array of reference to const auto
               *
               * you can't tell whether it's an ordinary "auto" declaration or
               * a structured binding declaration until you see '[' without
               * having seen a name like "x".  Hence, this special case to
               * look-ahead more than one token and return a structured binding
               * "auto".
               *
               * However, for function parameters, a declaration like:
               *
               *      int f(auto const[N])  // unnamed array N of const auto
               *
               * is always an (unnamed) array of const ordinary "auto" and
               * never a structured binding.  When parsing a parameter list,
               * the parser sets lexer_is_param_list_decl so a structured
               * binding "auto" won't be returned.
               */
auto/({S}+(const|volatile))*{S}*?&?&?{S}*(\[|<:|\?\?\() {
                if ( !lexer_is_param_list_decl )
                  return Y_auto_STRUCTURED_BINDING;
                CDECL_REJECT;
              }

              /*
               * Special case: if "const[ant]" is immediately followed by one
               * of "eval[uation]", "expr[ession]", or "init[ialization]",
               * return a special English version of the "const" token to
               * disambiguate it (as part of one of those three storage
               * classes) from the normal "const" that's a CV qualifier.
               */
const(ant)?/{S}+(eval(uation)?|expr(ession)?|init(ialization)?){NI} {
                return Y_constant;
              }

              /*
               * Special case: if "declare" is eventually followed by "user-
               * defined", e.g.:
               *
               *      c++decl> declare overridden user-defined \
               *        conversion operator returning int
               *      operator int() override;
               *
               * the keyword context has to be set to C_KW_CTX_MBR_FUNC to be
               * able to match "override" and "final" (that ordinarily are not
               * matched unless within a member function declaration).
               *
               * The context can't always be set to C_KW_CTX_MBR_FUNC after
               * "declare" otherwise "override" and "final" would match when
               * they shouldn't, e.g.:
               *
               *      c++decl> declare final as int
               *      int final;
               *
               * (which is legal).
               */
declare/{S}({S}|{L})*user(-|{S}+)def(ined)?{NI} {
                lexer_keyword_ctx = C_KW_CTX_MBR_FUNC;
                return Y_declare;
              }

              /*
               * Special case: implement "include" files entirely within the
               * lexer.  See:
               *
               * http://westes.github.io/flex/manual/Multiple-Input-Buffers.html
               */
^({S}*#{S}*)?include{S}*\" {
                strbuf_init( &str_lit_buf );
                //
                // Save the start location because we want to use it as the
                // location for the literal, not its end location.
                //
                str_lit_loc = yylloc;
                str_lit_loc.first_column = yylloc.last_column;
                lexer_begin( X_INCLUDE );
              }

              /*
               * Special case: make `q` a synonym for `quit`, but only when
               * it's the only thing on a line other than whitespace.  In all
               * other cases, `q` should be treated as an ordinary identifier.
               * This is done to allow things like:
               *
               *      cdecl> declare p, q as pointer to int
               *      int *p, *q;
               *
               * This isn't handled by having a `q` entry in CDECL_KEYWORDS
               * because that would make `q` a synonym all the time.
               *
               * Note that we can't simply do:
               *
               *      ^{S}*q{S}*$
               *
               * because `$` only matches a newline and not "end of string."
               *
               * To forbid a string like `q x`, we first have to match its
               * pattern explicitly and always forbid it.
               */
^{S}*q{S}+{NS}+ {
                char const *const orig_yytext = yytext;
                // update yytext to be only the offending token past the 'q'
                SKIP_WS( yytext );
                assert( yytext[0] == 'q' );
                ++yytext;
                SKIP_WS( yytext );
                // update yyloc to be at the start of the offending token
                token_column = yytext - orig_yytext;
                lexer_update_loc();
                return Y_LEXER_ERROR;
              }
^{S}*q{S}*    { set_yytext( L_quit ); return Y_quit; }

              /*
               * Special case: similar to {sname} below, handle scoped
               * destructor names in the lexer so destructor names are
               * recognized as such, e.g.:
               *
               *      S::T::T           // not a destructor
               *      S::T::U           // not a destructor
               *      S::T::~T          // a destructor
               *      S::T::~U          // not a destructor (and an error)
               */
{dtor_sname}  {
                c_sname_t sname;
                if ( c_sname_parse_dtor( yytext, &sname ) > 0 ) {
                  yylval.sname = sname;
                  return Y_DESTRUCTOR_SNAME;
                }
                print_error( &yylloc,
                  "matching class name after '~' expected\n"
                );
                return Y_LEXER_ERROR;
              }

              /*
               * Special case: similar to {sname} below, handle scoped
               * operators in the lexer to simplify the grammar, e.g.:
               *
               *      S::T::operator    // sname = "S::T"
               */
{oper_sname}  {
                c_sname_t sname;
                size_t const sname_len = c_sname_parse( yytext, &sname );
                if ( sname_len == 0 )
                  CDECL_REJECT;

                //
                // c_sname_parse() will not include "::operator" in the parsed
                // scoped name, so the returned length will only include the
                // actual scoped name, e.g., "S::T".  We therefore have to tell
                // Flex to put the characters "::operator" back onto the input
                // stream.
                //
                yyless( STATIC_CAST( int, sname_len ) );

                //
                // See if it's a typedef'd type: if so, copy the type's scoped
                // name so we get its scope types (if any).
                //
                c_typedef_t const *const tdef = c_typedef_find_sname( &sname );
                if ( tdef != NULL ) {
                  c_sname_cleanup( &sname );
                  sname = c_sname_dup( &tdef->ast->sname );
                }

                yylval.sname = sname;
                return Y_OPERATOR_SNAME;
              }

              /*
               * Special case: handle concept (constrained) "auto" for an
               * {sname}, e.g.:
               *
               *      std::integral auto x
               *
               * in the lexer so:
               *
               * 1. Conflicts don't arrise in the grammar since you can't tell
               *    an {sname} is a concept until you parse the "auto".
               *
               * 2. Concepts don't have to be pre-declared via a pseudo-C++
               *    syntax like:
               *
               *          namespace std { concept integral; }
               *
               *    prior to use.
               */
{sname}/{S}+auto{NI} {
                c_sname_t sname;
                size_t const sname_len = c_sname_parse( yytext, &sname );
                if ( sname_len == 0 )
                  CDECL_REJECT;
                yylval.sname = sname;
                return Y_CONCEPT_SNAME;
              }

              /*
               * Special case: handle scoped names in the lexer so:
               *
               * 1. Constructors are recognized as such, specifically, when the
               *    scoped name's last two scopes match, e.g.:
               *
               *          A::B::C       // not a constructor
               *          S::T::T       // a constructor
               *
               *    This is needed because constructors and ordinary
               *    declarations are lexically ambiguous in a LALR(1) parser:
               *
               *          A::B(x);      // declare x as A::B with unneeded ()
               *          S::S()        // define constructor for S
               *
               * 2. Previously declared scope-types are recognized as such,
               *    e.g.:
               *
               *          define S::T as int
               *          explain S::T x
               *
               * The trailing context of a Non-Indentifier (NI) character is
               * necessary to prevent Flex from recognizing partial identifiers
               * upon REJECT.  For example, given these declarations:
               *
               *      namespace X::YY { class T; }
               *      namespace X     { class Y; }
               *      namespace X::YY { class U; }
               *
               * Just as in the first declaration, when "X::YY" is encountered
               * in the third declaration, we would ordinarily REJECT a match
               * because "X::YY" is not a type.  (Hence, "X::YY" would be
               * returned to the parser as three separate tokens "X", "::", and
               * "YY" just as they were in the first declaration.)
               *
               * However, upon REJECT, Flex backs off one character at a time,
               * so it will next try to match "X::Y" and succeed since it's a
               * substring of "X::YY".  The "X::Y" is then looked-up and finds
               * the class X::Y (from the second declaration).  This is of
               * course wrong semantically, but Flex doesn't know anything
               * about semantics, i.e., it doesn't know that the longest set of
               * contiguous alphanumeric characters comprises an identifier and
               * shouldn't be split.
               *
               * Using the trailing context prevents Flex from matching the
               * partial token.
               */
{sname}/{NI}  {
                c_sname_t sname;
                size_t const sname_len = c_sname_parse( yytext, &sname );
                if ( sname_len == 0 )
                  CDECL_REJECT;

                if ( sname_len < STATIC_CAST( size_t, yyleng ) )
                  yyless( sname_len );

                //
                // 1. See if it's a constructor name.
                //
                if ( c_sname_is_ctor( &sname ) ) {
                  yylval.sname = sname;
                  return Y_CONSTRUCTOR_SNAME;
                }

                if ( (lexer_find & LEXER_FIND_TYPES) != 0 ) {
                  //
                  // 2. See if it's a typedef'd type.
                  //
                  c_typedef_t const *const tdef =
                    c_typedef_find_sname( &sname );
                  if ( tdef != NULL ) {
                    yylval.tdef = tdef;
                    c_sname_cleanup( &sname );
                    return Y_TYPEDEF_SNAME_TDEF;
                  }
                }

                //
                // 3. Otherwise, reject it.
                //
                c_sname_cleanup( &sname );
                CDECL_REJECT;
              }

              /*
               * Special case: match hyphenated tokens.  We need a separate
               * rule because '-' isn't a valid character in an identifier.
               */
{hyphenated}  {
                //
                // Hyphenated tokens are legal only in pseudo-English.
                //
                if ( (lexer_find & LEXER_FIND_CDECL_KEYWORDS) == 0 )
                  return Y_ERROR;

                //
                // Now that we've matched a hyphenated token, use the same
                // keyword-matching code.
                //
                goto find_cdecl_keyword;
              }

  /***************************************************************************/
  /*  IDENTIFIERS                                                            */
  /***************************************************************************/

              /*
               * Special case: similar to the special case for {sname} auto,
               * handle concept (constrained) auto for an {identifier}, e.g.:
               *
               *      C auto x
               *
               * where "C" is a concept.
               */
{identifier}/{S}+auto{NI} {
                is_constrained_auto = true;
                //
                // Now that we've set the flag, use the same keyword-matching
                // code.
                //
                goto find_cdecl_keyword;
              }

{identifier}  {
                //
                // 1. See if it's a cdecl keyword.
                //
        find_cdecl_keyword:;
                cdecl_keyword_t const *const cdk =
                  lexer_cdecl_keyword_find( yytext );
                if ( cdk != NULL ) {
                  if ( cdk->lang_syn == NULL ) {
                    if ( cdk->literal == L_expand ) {
                      //
                      // For the "expand" command, we want to allow all tokens,
                      // but also return a few more that ordinarily aren't
                      // returned to the parser.
                      //
                      lexer_begin( X_EXPAND );
                    }
                    else if ( cdk->literal == L_set ) {
                      //
                      // For the "set" command, we want to allow (almost) any
                      // character sequence for the command's options, so we
                      // use an exclusive start state.
                      //
                      lexer_begin( X_SET );
                    }
                    else if ( cdk->literal == L_show ) {
                      //
                      // For the "show" command, we need to allow globs.
                      //
                      lexer_begin( S_SHOW );
                    }
                    return cdk->y_token_id;
                  }
                  char const *const literal = c_lang_literal( cdk->lang_syn );
                  if ( literal != NULL ) {
                    set_yytext( literal );
                    goto find_c_keyword;
                  }
                }

                if ( (lexer_find & LEXER_FIND_TYPES) != 0 ) {
                  //
                  // 2. See if it's a typedef'd type.
                  //
                  c_typedef_t const *const tdef =
                    c_typedef_find_sname( &C_SNAME_LIT( yytext ) );
                  if ( tdef != NULL ) {
                    yylval.tdef = tdef;
                    return Y_TYPEDEF_NAME_TDEF;
                  }
                }

        find_c_keyword:
                if ( (lexer_find & LEXER_FIND_C_KEYWORDS) != 0 ) {
                  //
                  // 3. See if it's a C/C++ keyword.
                  //
                  c_keyword_t const *const ck = c_keyword_find(
                    yytext, opt_lang_id, lexer_keyword_ctx
                  );
                  if ( ck != NULL ) {
                    yylval.tid = ck->tid;
                    return ck->y_token_id;
                  }
                }

                if ( is_constrained_auto ) {
                  //
                  // 4. If it's constrained auto, it's a concept name.
                  //
                  c_sname_init_name( &yylval.sname, check_strdup( yytext ) );
                  return Y_CONCEPT_SNAME;
                }

                //
                // 5. Otherwise, it's just an ordinary name.
                //
                yylval.name = check_strdup( yytext );
                return Y_NAME;
              }

  /***************************************************************************/
  /*  NUMERIC LITERALS                                                       */
  /***************************************************************************/

<INITIAL,X_PRE_TOKENS>{

{bin_int}     { yylval.int_val = parse_int(  2 ); return Y_INT_LIT; }
{oct_int}     { yylval.int_val = parse_int(  8 ); return Y_INT_LIT; }
{dec_int}     { yylval.int_val = parse_int( 10 ); return Y_INT_LIT; }
{hex_int}     { yylval.int_val = parse_int( 16 ); return Y_INT_LIT; }

{flt_lit}     {
                // cdecl doesn't care what the actual float value is, only the
                // characters comprising the token for the macro processor.
                return Y_FLOAT_LIT;
              }

} /* <INITIAL,X_PRE_TOKENS> */

  /***************************************************************************/
  /*  ATTRIBUTES                                                             */
  /***************************************************************************/

<INITIAL,X_PRE_TOKENS>{

              /*
               * Special case: if '[' is immediately followed by another '[',
               * return a distinct token to decrease the number of shift/reduce
               * conflicts.
               */
"<:"/{S}*"<:"   { set_digraph( "[" );  return Y_ATTR_BEGIN; }
"??("/{S}*"??(" { set_trigraph( "[" ); return Y_ATTR_BEGIN; }
"["/{S}*"["     {                      return Y_ATTR_BEGIN; }

} /* <INITIAL,X_PRE_TOKENS> */

  /***************************************************************************/
  /*  OPERATORS & PUNCTUATION                                                */
  /***************************************************************************/

<INITIAL,X_PRE_TOKENS>{
              /*
               * Special case: if "::" is immediately followed by a '*', return
               * a distinct token to make it possible to distinguish between:
               *
               *    <name>::<name>::<name>
               *    <name>::<name>::*
               *
               * in an LALR(1) parser.
               */
"::"/{S}*"*"  { return Y_COLON_COLON_STAR       ; }
"::"          { return Y_COLON_COLON            ; }

              /* Multi-character operators. */
"!="          { return Y_EXCLAM_EQUAL           ; }
"%="          { return Y_PERCENT_EQUAL          ; }
"&&"          { return Y_AMPER_AMPER            ; }
"&="          { return Y_AMPER_EQUAL            ; }
"*="          { return Y_STAR_EQUAL             ; }
"++"          { return Y_PLUS_PLUS              ; }
"+="          { return Y_PLUS_EQUAL             ; }
"--"          { return Y_MINUS_MINUS            ; }
"-="          { return Y_MINUS_EQUAL            ; }
"->"          { return Y_MINUS_GREATER          ; }
"->*"         { return Y_MINUS_GREATER_STAR     ; }
".*"          { return Y_DOT_STAR               ; }
"..."         { return Y_ELLIPSIS               ; }
"/="          { return Y_SLASH_EQUAL            ; }
"<<"          { return Y_LESS_LESS              ; }
"<<="         { return Y_LESS_LESS_EQUAL        ; }
"<="          { return Y_LESS_EQUAL             ; }
"<=>"         { return Y_LESS_EQUAL_GREATER     ; }
"=="          { return Y_EQUAL_EQUAL            ; }
">="          { return Y_GREATER_EQUAL          ; }
">>"          { return Y_GREATER_GREATER        ; }
">>="         { return Y_GREATER_GREATER_EQUAL  ; }
"?:"          { return Y_QMARK_COLON            ; }
"^="          { return Y_CARET_EQUAL            ; }
"|="          { return Y_PIPE_EQUAL             ; }
"||"          { return Y_PIPE_PIPE              ; }

              /*
               * Special case: if '#' is the first non-whitespace character on
               * a line, assume it's the start of a C preprocessor command.
               *
               * This needs to be here before the regular "#" a few lines below
               * so Flex will match this rule before that rule.
               */
^{S}*"#"      {
                lexer_begin( X_PRE_COMMAND );
                return '#';
              }

              /*
               * Same as above, but for the digraph equivalent of '#'.
               */
^{S}*"%:"     {
                set_digraph( "#" );
                lexer_begin( X_PRE_COMMAND );
                return '#';
              }

              /*
               * Same as above, but for the trigraph equivalent of '#'.
               */
^{S}*"??="    {
                set_trigraph( "#" );
                lexer_begin( X_PRE_COMMAND );
                return '#';
              }

              /* Single-character operators and miscellaneous punctuation. */
"!"           |
"#"           |
"%"           |
"&"           |
"("           |
")"           |
"*"           |
"+"           |
","           |
"-"           |
"."           |
"/"           |
":"           |
";"           |
"<"           |
"="           |
">"           |
"?"           |
"["           |
"]"           |
"^"           |
"{"           |
"|"           |
"}"           |
"~"           { return yytext[0]                ; }

} /* <INITIAL,X_PRE_TOKENS> */

  /***************************************************************************/
  /*  DIGRAPHS & TRIGRAPHS                                                   */
  /***************************************************************************/

<*>{
              /* Digraphs. */
"%:"          { set_digraph( "#" ); return '#'; }
"<:"          { set_digraph( "[" ); return '['; }
":>"          { set_digraph( "]" ); return ']'; }
"<%"          { set_digraph( "{" ); return '{'; }
"%>"          { set_digraph( "}" ); return '}'; }

              /* Trigraphs. */
"??'="        { set_trigraph( "^=" ); return Y_CARET_EQUAL; }
"??!="        { set_trigraph( "|=" ); return Y_PIPE_EQUAL ; }
"??!??!"      { set_trigraph( "||" ); return Y_PIPE_PIPE  ; }
"??'"         { set_trigraph( "^"  ); return '^'          ; }
"??("         { set_trigraph( "["  ); return '['          ; }
"??/"         { set_trigraph( "\\" ); return '\\'         ; }
"??)"         { set_trigraph( "]"  ); return ']'          ; }
"??<"         { set_trigraph( "{"  ); return '{'          ; }
"??!"         { set_trigraph( "|"  ); return '|'          ; }
"??>"         { set_trigraph( "}"  ); return '}'          ; }
"??-"         { set_trigraph( "~"  ); return '~'          ; }

} /* <*> */

  /***************************************************************************/
  /*  RAW STRING LITERALS                                                    */
  /***************************************************************************/

<INITIAL,X_PRE_TOKENS>{

{rstr_pfx}    { // e.g.: u8R"abc(...)abc"
                // find:    ^   ^
                char const *q = strchr( yytext, '"' );
                assert( q != NULL );
                char const *const p = strchr( ++q, '(' );
                assert( p != NULL );

                size_t const delim_len = STATIC_CAST( size_t, p - q );
                if ( delim_len > RSTR_DELIM_LEN_MAX ) {
                  print_error( &yylloc,
                    "raw string literal delimiter "
                    "exceeds maximum length of %u\n",
                    RSTR_DELIM_LEN_MAX
                  );
                  return Y_LEXER_ERROR;
                }

                strncpy( rstr_delim, q, delim_len );
                //
                // To make the delimiter-match code simpler, include the
                // closing '"' as part of the delimiter.  (Using strcpy() also
                // has the benefit of ensuring rstr_delim is null-terminated.)
                //
                strcpy( rstr_delim + delim_len, "\"" );

                strbuf_init( &str_lit_buf );
                //
                // Save the start location because we want to use it as the
                // location for the literal, not its end location.
                //
                str_lit_loc = yylloc;

                BEGIN_CALL( X_RSTR );
              }

} /* <INITIAL,X_PRE_TOKENS> */

<X_RSTR>{

  [^)]+       { strbuf_putsn( &str_lit_buf, yytext, yyleng ); }
  ")"         {
                char const *d;
                for ( d = rstr_delim; *d != '\0'; ++d, ++token_column ) {
                  int const c = input();
                  if ( c == 0 || c == EOF ) {
                    print_error( &yylloc, "unterminated string literal\n" );
                    strbuf_reset( &str_lit_buf );
                    return Y_LEXER_ERROR;
                  }
                  if ( c == *d )
                    continue;
                  //
                  // Found a mismatch with the delimiter, e.g.:
                  //
                  //      )abc"       // delimiter
                  //      )abx"       // what was found
                  //
                  // Hence, it's not the actual delimiter, but part of the
                  // string literal: append the part that matched (")ab").
                  //
                  int const part_len = STATIC_CAST( int, d - rstr_delim );
                  strbuf_printf( &str_lit_buf, ")%.*s", part_len, rstr_delim );
                  //
                  // Unput the character that caused the mismatch since it
                  // could be ')' that could potentially start a real match,
                  // e.g.:
                  //
                  //      R"abc(X)ab)abc"
                  //                ^
                  //
                  // Hence the raw string is "X)ab".
                  //
                  unput( c );
                  break;
                } // for

                if ( *d == '\0' ) {     // found delimiter
                  yylloc.first_line   = str_lit_loc.first_line;
                  yylloc.first_column = str_lit_loc.first_column;
                  yylval.str_val =
                    empty_dup_if_null( strbuf_take( &str_lit_buf ) );
                  BEGIN_RETURN;
                  return Y_STR_LIT;
                }
              }

} /* <X_RSTR> */

  /***************************************************************************/
  /*  STRING LITERALS, CHARACTER LITERALS, & INCLUDE FILES                   */
  /***************************************************************************/

<INITIAL,X_PRE_TOKENS>{

  {cstr_pfx}?['"] {
                strbuf_init( &str_lit_buf );
                //
                // Save the start location because we want to use it as the
                // location for the literal, not its end location.
                //
                str_lit_loc = yylloc;

                char const quote = yytext[ yyleng - 1 ];
                BEGIN_CALL( (quote == '"' ? X_STR : X_CHAR) );
              }

} /* <INITIAL,X_PRE_TOKENS> */

              /*
               * Common code between X_CHAR, X_INCLUDE, and X_STR.
               * Multicharacter literals are legal, but implementation-defined.
               */
<X_CHAR,X_INCLUDE,X_STR>{

              /* Escaped characters are copied verbatim, not interpreted. */
  \\(.|\n)    { strbuf_putsn( &str_lit_buf, yytext, yyleng ); }
  \n          {
                print_error( &str_lit_loc,
                  "unterminated %s literal\n",
                  YY_START == X_CHAR ? "character" : "string"
                );
                strbuf_reset( &str_lit_buf );
                return Y_LEXER_ERROR;
              }

} /* <X_CHAR,X_INCLUDE,X_STR> */

<X_CHAR>{

  [^'\\\n]+   { strbuf_putsn( &str_lit_buf, yytext, yyleng ); }
  \'          {
                yylloc.first_line   = str_lit_loc.first_line;
                yylloc.first_column = str_lit_loc.first_column;
                yylval.str_val = strbuf_take( &str_lit_buf );
                BEGIN_RETURN;
                return Y_CHAR_LIT;
              }

} /* <X_CHAR> */

              /*
               * Common code between X_INCLUDE and X_STR:
               *
               * 1. X_INCLUDE is the same as X_STR except we need to do
               *    different things upon the terminating " (which is why
               *    X_INCLUDE exists rather than just using X_STR).
               *
               * 2. X_STR is the same as X_CHAR except " replaces '.
               */
<X_INCLUDE,X_STR>[^"\\\n]+ {
                strbuf_putsn( &str_lit_buf, yytext, yyleng );
              }

<X_INCLUDE>\" {
                push_file( str_lit_buf.str, &str_lit_loc );
                strbuf_reset( &str_lit_buf );
                lexer_begin( INITIAL );
              }

<X_STR>\"     {
                yylloc.first_line   = str_lit_loc.first_line;
                yylloc.first_column = str_lit_loc.first_column;
                yylval.str_val =
                  empty_dup_if_null( strbuf_take( &str_lit_buf ) );
                BEGIN_RETURN;
                return Y_STR_LIT;
              }

  /***************************************************************************/
  /*  COMMENTS                                                               */
  /***************************************************************************/

<*>{

  "/*"        { BEGIN_CALL( X_C_COMMENT ); }
  "//".*      ;                         /* ignore C++-style comments */

} /* <*> */

<X_C_COMMENT>{

  ([^*]|\*+[^/])+ {                    // ignore C-style comment except for */
                token_column += yyleng;
              }

  \n          { newline(); }

  "*/"        {
                BEGIN_RETURN;
                if ( YY_START == X_PRE_TOKENS ) {
                  //
                  // When lexing tokens for the C preprocessor, comments have
                  // to turn into a space, e.g.:
                  //
                  //      cdecl> #define Q(A,B)  A/**/B
                  //      cdecl> expand Q(x,y)
                  //      Q(x, y) => A B
                  //      | A => x
                  //      | B => y
                  //      Q(x, y) => x y
                  //
                  return Y_PRE_SPACE;
                }
              }

} /* <X_C_COMMENT> */

  /***************************************************************************/
  /*  WHITESPACE                                                             */
  /***************************************************************************/

<*>{S}+       {
                if ( YY_START == X_PRE_TOKENS ) {
                  //
                  // When lexing tokens for the C preprocessor, whitespace is
                  // significant.
                  //
                  return Y_PRE_SPACE;
                }
              }

<*>\n         {
                newline();
                lexer_begin( INITIAL );
                return Y_END;
              }

  /***************************************************************************/
  /*  PREPROCESSOR                                                           */
  /***************************************************************************/

<X_PRE_COMMAND>{
              /*
               * Special case: if "define" is followed by an identifier that is
               * _immediately_ followed by '(' with no intervening whitespace,
               * it's defining a function-like macro, so set a flag we can use
               * it in {identifier} below.
               */
  define/{S}+{identifier}\( {
                is_func_like_macro = true;
                return Y_PRE_define;
              }

  {identifier} {
                p_keyword_t const *const pk = p_keyword_find( yytext );
                if ( pk != NULL )
                  return pk->y_token_id;
                yylval.name = check_strdup( yytext );
                if ( !is_func_like_macro ) {
                  //
                  // We can switch to X_PRE_TOKENS now; otherwise we switch
                  // after lexing ')' below.
                  //
                  lexer_begin( X_PRE_TOKENS );
                }
                return Y_NAME;
              }

  "("         |
  ","         { return yytext[0]    ; }
  ")"         {
                lexer_begin( X_PRE_TOKENS );
                return ')';
              }

  "..."       { return Y_ELLIPSIS   ; }

} /* <X_PRE_COMMAND> */

<X_PRE_TOKENS>{

  {identifier} {
                if ( strcmp( yytext, L_PRE___VA_ARGS__ ) == 0 )
                  return Y_PRE___VA_ARGS__;
                if ( strcmp( yytext, L_PRE___VA_OPT__ ) == 0 )
                  return Y_PRE___VA_OPT__;
                yylval.name = check_strdup( yytext );
                return Y_NAME;
              }

  "%:%:"      { set_digraph( "##" ); return Y_PRE_CONCAT; }
  "%:"        { set_digraph( "#"  ); return '#'         ; }

  "??=??="    { set_trigraph( "##" ); return Y_PRE_CONCAT ; }
  "??="       { set_trigraph( "#"  ); return '#'          ; }

  "##"        { return Y_PRE_CONCAT ; }

  "#"         |
  "$"         |
  "@"         |
  "`"         { return yytext[0]    ; }

} /* <X_PRE_TOKENS> */

<X_EXPAND>{
              /*
               * Special case: if an identifier is followed by '(', it's a
               * function-like macro, so defer switching to the X_PRE_TOKENS
               * state until after lexing '(' so we don't have to deal with
               * optional whitespace before the '(' being significant.
               */
  {identifier}/{S}*\( {
                yylval.name = check_strdup( yytext );
                return Y_NAME;
              }

  {identifier} {                        // object-like macro
                yylval.name = check_strdup( yytext );
                lexer_begin( X_PRE_TOKENS );
                return Y_NAME;
              }

  "("         {
                lexer_begin( X_PRE_TOKENS );
                return '(';
              }

} /* <X_EXPAND> */

  /***************************************************************************/
  /*  OTHER START STATES                                                     */
  /***************************************************************************/

<X_SET>{

  {set_option} {
                yylval.name = check_strdup( yytext );
                return Y_SET_OPTION;
              }

  "="         { return yytext[0]; }
  ";"         {
                lexer_begin( INITIAL );
                return yytext[0];
              }

} /* <X_SET> */

<S_SHOW>{

  {glob}      {
                yylval.name = check_strdup( yytext );
                return Y_GLOB;
              }

  ";"         {
                lexer_begin( INITIAL );
                return yytext[0];
              }

} /* <S_SHOW> */

  /***************************************************************************/
  /*  NON-MATCH & EOF                                                        */
  /***************************************************************************/

<*>.          {
                return Y_ERROR;
              }

<*><<EOF>>    {
                newline();
                if ( YY_START != X_C_COMMENT )
                  lexer_begin( INITIAL );

                if ( pop_file() )
                  return Y_END;

                //
                // The first time we encounter EOF (not for an include file),
                // we want to treat it as if it were a newline by returning
                // Y_END so commands in the parser always end in Y_END.
                //
                // Requesting more characters after encountering EOF will
                // simply continue to return EOF, so the second time we
                // encounter EOF, treat it as EOF by returning no token.
                //
                return (is_eof = !is_eof) ? Y_END : YY_NULL;
              }

%%

/// @endcond

// Re-enable warnings.
#pragma clang diagnostic pop
#pragma GCC diagnostic pop

////////// local functions ////////////////////////////////////////////////////

/**
 * @addtogroup lexer-group
 * @{
 */

/**
 * Wrapper around the Flex `BEGIN` macro so that we can do additional things
 * when switching lexer start states.
 *
 * @param start The lexer start state to switch to.
 */
static void lexer_begin( int start ) {
  BEGIN( start );
  lexer_in_c_comment = YY_START == X_C_COMMENT;
}

// LCOV_EXCL_START
/**
 * Called by Flex only when there's a fatal error.
 *
 * @param msg The error message to print.
 */
_Noreturn
static void lexer_fatal( char const *msg ) {
  if ( msg == NULL ) {
    // Never true -- here just to silence the "unused function" warning.
    yy_fatal_error( msg );
  }

  //
  // This is defined down here to avoid having to declare yy_fatal_error
  // ourselves and having to get it right being subject to possible changes in
  // its signature in different Flex versions.
  //
  INTERNAL_ERROR( "lexer_fatal(): %s\n", msg );
}
// LCOV_EXCL_STOP

/** @} */

////////// extern functions ///////////////////////////////////////////////////

void lexer_init( void ) {
  ASSERT_RUN_ONCE();
  ATEXIT( &lexer_cleanup );
  include_init();
}

char const* lexer_input_line( size_t *rv_len ) {
  assert( rv_len != NULL );
  *rv_len = input_sbuf.len;
  return input_sbuf.str;
}

c_loc_t lexer_loc( void ) {
  c_loc_t rv_loc = yylloc;
  if ( yytext[0] == '\n' ) {
    //
    // If the current token is '\n', it means first_line & last_line are
    // already on the next line, but we want the line that the '\n' is on which
    // is the previous line.
    //
    --rv_loc.first_line;
    --rv_loc.last_line;
  }
  return rv_loc;
}

void lexer_pop_string( void ) {
  lexer_pop_buffer();
  yylineno_stack_pop();
  lexer_begin( INITIAL );
}

void lexer_push_string( char const *s, size_t s_len, c_loc_num_t line_no ) {
  assert( s != NULL );
  FILE *const f = fmemopen( CONST_CAST( void*, s ), s_len, "r" );
  PERROR_EXIT_IF( f == NULL, EX_IOERR );
  yyin = f;
  yypush_buffer_state( yy_create_buffer( yyin, YY_BUF_SIZE ) );
  yylineno_stack_push();
  yylineno = line_no;
  lexer_begin( X_PRE_TOKENS );
}

void lexer_reset( bool hard_reset ) {
  if ( hard_reset ) {
    is_eof = false;
    include_cleanup();
    include_init();
    newline();
    if ( opt_lineno == 0 )
      yylineno = 1;
  }
  if ( hard_reset || !lexer_in_c_comment ) {
    lexer_begin( INITIAL );
    yy_start_stack_top = -1;
  }
  digraph_warned = trigraph_warned = false;
  strbuf_reset( &input_sbuf );
  input_sent = 0;
  is_constrained_auto = false;
  is_func_like_macro = false;
  lexer_find = LEXER_FIND_ANY;
  lexer_is_param_list_decl = false;
  lexer_keyword_ctx = C_KW_CTX_DEFAULT;
  strbuf_reset( &str_lit_buf );
  yylineno_stack_top = -1;
}

char const* printable_yytext( void ) {
  switch ( yytext[0] ) {
    case '\0':
    case '\n':
      return NULL;
    default:
      return yytext;
  } // switch
}

///////////////////////////////////////////////////////////////////////////////

extern inline bool is_english_to_gibberish( void );
extern inline bool is_gibberish_to_english( void );
extern inline void set_yytext( char const* );

/* vim:set et sw=2 ts=2: */
