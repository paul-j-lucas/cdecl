/*
**      cdecl -- C gibberish translator
**      src/lexer.l
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
 * Defines helper macros, data structures, variables, functions, and the
 * tokenizer for C/C++ declarations.
 */

/** @cond DOXYGEN_IGNORE */

%option warn
%option yylineno

%top {
#include "pjl_config.h"                 /* must go first */
}

%{
/** @endcond */

// local
#include "lexer.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_typedef.h"
#include "cdecl.h"
#include "cdecl_keyword.h"
#include "gibberish.h"
#include "literals.h"
#include "options.h"
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
#include <stdio.h>
#include <stdlib.h>                     /* for strtol(3) */
#include <string.h>
#include <wordexp.h>

// Silence these warnings for Flex-generated code.
#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wimplicit-int-conversion"
# pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif /* __clang__ */
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
# pragma GCC diagnostic ignored "-Wmisleading-indentation"
# pragma GCC diagnostic ignored "-Wredundant-decls"
# pragma GCC diagnostic ignored "-Wsign-compare"
# pragma GCC diagnostic ignored "-Wsign-conversion"
#endif /* __GNUC__ */

/// @endcond

/**
 * @addtogroup lexer-group
 * @{
 */

/**
 * The maximum number of decimal digits in the unsigned 64-bit integer
 * 18446744073709551615.
 */
#define MAX_DIGITS_64             20u

/**
 * C++ raw string delimiter maximum length.
 */
#define RSTR_DELIM_LEN_MAX        16u

/**
 * Sets \ref lexer_token to the current Flex token.
 */
#define SET_TOKEN                 SET_TOKEN_TO( yytext )

/**
 * Sets \ref lexer_token to \a S.
 *
 * @param S The string to set the token to.
 */
#define SET_TOKEN_TO(S)           (lexer_token = (S))

/**
 * Overrides Flex's fatal error message to print the message in our format and
 * also exit with the status code we want.
 *
 * @param MSG The error message to print.
 */
#define YY_FATAL_ERROR(MSG)       lexer_fatal( MSG )

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
 */
#define YY_USER_ACTION            lexer_update_loc(); /* must include ';' */

/**
 * This code must be manually inserted before a REJECT to undo #YY_USER_ACTION.
 */
#define YY_USER_ACTION_UNDO_FOR_REJECT \
  token_column -= STATIC_CAST( int, yyleng )

/**
 * Data to keep for a file that is `include`d.
 *
 * @remarks
 * @parblock
 * When a file is included, an <code>%include_file_info</code> is
 * pushed onto \ref include_stack such that:
 *
 *  + \ref prev_orig_path is the unresolved path of the _previous_ include (or
 *    configuration) file, if any, that is the value of print_params.conf_path.
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
  int         prev_lineno;              ///< The file's last line number.
};
typedef struct include_file_info include_file_info_t;

static char const lexer_token_init_buf; ///< So lexer_token is never NULL.

/// @cond DOXYGEN_IGNORE
/// Otherwise Doxygen generates two entries for each option.

// extern variables
lexer_find_kind_t lexer_find = LEXER_FIND_ANY;
c_keyword_ctx_t   lexer_keyword_ctx;

// A separate pointer for the current token allows it:
//  + To be `const`.
//  + To be assigned to either other `const` strings or string literals.
char const       *lexer_token = &lexer_token_init_buf;

/// @endcond

// local variables
static bool       digraph_warned;       ///< Printed digraph warning once?
static rb_tree_t  include_set;          ///< Set of resolved include paths.
static slist_t    include_stack;        ///< Stack of include paths.
static char      *input_buf;            ///< Entire current input line.
static size_t     input_len;            ///< Length of `input_buf`.
static size_t     input_sent;           ///< How many bytes returned to Flex.
static bool       is_eof;               ///< Encountered EOF?
static int        newline_column;       ///< Column position of newline.
static strbuf_t   str_lit_buf;          ///< String (or character) literal.
static c_loc_t    str_lit_loc;          ///< String literal starting location.
static int        token_column;         ///< Column position of current token.
static bool       trigraph_warned;      ///< Printed trigraph warning once?

/// C++ raw string literal delimiter.
static char       rstr_delim[ RSTR_DELIM_LEN_MAX + 1/*"*/ + 1/*\0*/ ];

// local functions
_Noreturn
static void       lexer_fatal( char const* );

////////// local functions ////////////////////////////////////////////////////

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
  rb_tree_init( &include_set, POINTER_CAST( rb_cmp_fn_t, &strcmp ) );
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
 * Cleans up lexer data.
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
  yy_size_t input_avail = input_len - input_sent;
  if ( input_avail == 0 ) {
    static size_t input_cap;
    ssize_t bytes_read = getline( &input_buf, &input_cap, yyin );
    if ( unlikely( bytes_read == -1 ) ) {
      FERROR( yyin );
      bytes_read = 0;
    }
    input_avail = input_len = STATIC_CAST( yy_size_t, bytes_read );
    input_sent = 0;
  }
  //
  // Given that the default buffer capacity (YY_READ_BUF_SIZE) for Flex is
  // 8192, it's unlikely that this will ever be true and that we'll have to
  // return the input line in chunks; but might as well code for the case.
  //
  if ( unlikely( input_avail > buf_cap ) )
    input_avail = buf_cap;
  memcpy( buf, input_buf + input_sent, input_avail );
  input_sent += input_avail;
  return input_avail;
}

/**
 * Update the parser's location.
 * @note This is called by Flex via #YY_USER_ACTION.
 */
static void lexer_update_loc( void ) {
  yylloc.first_line   = yylloc.last_line = STATIC_CAST( int, yylineno );
  yylloc.first_column = token_column;
  yylloc.last_column  = token_column + STATIC_CAST( int, yyleng ) - 1;
  token_column += STATIC_CAST( int, yyleng );
}

/**
 * Resets the token column position upon encountering a newline.
 */
static inline void newline( void ) {
  newline_column = token_column;
  token_column = 0;
}

/**
 * Parses an integer from \ref lexer_token.
 *
 * @param base The integer base to use.
 * @return Returns the integer value.
 */
NODISCARD
static int parse_int( int base ) {
  char const *s = lexer_token;

  if ( base == 2 ) {
    // Prior to C23, strtol(3) doesn't understand a "0b" (binary) prefix, so
    // skip over it.
    assert( s[0] == '0' );
    assert( tolower( s[1] ) == 'b' );
    s += 2;
  }

  char no_digit_seps[ MAX_DIGITS_64 + 1/*\0*/ ];

  if ( strchr( s, '\'' ) != NULL ) {
    char *t = no_digit_seps;
    do {
      if ( *s != '\'' )
        *t++ = *s;
    } while ( *s++ != '\0' );
    s = no_digit_seps;
  }

  errno = 0;
  long const rv = strtol( s, /*endptr=*/NULL, base );
  if ( unlikely( errno != 0 || rv > INT_MAX ) ) {
    print_warning( &yylloc, "integer out of range; clamped to %d\n", INT_MAX );
    return INT_MAX;
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
  // Ignore the curr_real_path returned by rb_tree_delete() since it's shared
  // with and owned by ifi.
  PJL_IGNORE_RV( rb_tree_delete( &include_set, found_rb ) );

  print_params.conf_path = ifi->prev_orig_path;
  yylineno = ifi->prev_lineno;
  ifi_free( ifi );

  // The example code in the Flex manual leaks file handles; see:
  // https://stackoverflow.com/a/27512485/99089
  assert( yyin != NULL );
  assert( yyin != stdin );
  PJL_IGNORE_RV( fclose( yyin ) );
  yypop_buffer_state();
  assert( YY_CURRENT_BUFFER != NULL );

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
        print_error( path_loc, "%s: too many files\n", path );
        goto done;
      }
      break;
    case WRDE_BADCHAR:
      print_error( path_loc, "%s: contains unquoted shell characters\n", path );
      return;
    case WRDE_SYNTAX:
      print_error( path_loc, "%s: path syntax error\n", path );
      return;
    // LCOV_EXCL_START
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
    print_error( path_loc, "%s: could not resolve path\n", path );
    goto done;
  }

  if ( rb_tree_find( &include_set, real_path ) != NULL ) {
    print_error( path_loc, "%s: file previously included\n", path );
    goto done;
  }

  if ( !path_is_file( real_path ) ) {
    print_error( path_loc, "%s: not a plain file\n", path );
    goto done;
  }

  FILE *const include_file = fopen( real_path, "r" );
  if ( include_file == NULL ) {
    print_error( path_loc, "%s: %s\n", path, STRERROR() );
    goto done;
  }

  //
  // Now that we know the path resolves, it wasn't previously included, it's a
  // plain file, and we can open it, we can insert it into include_set.
  //
  PJL_IGNORE_RV(
    rb_tree_insert( &include_set, CONST_CAST( void*, real_path ) )
  );

  include_file_info_t *const ifi = MALLOC( include_file_info_t, 1 );
  *ifi = (include_file_info_t){ print_params.conf_path, real_path, yylineno };
  slist_push_front( &include_stack, ifi );
  real_path = NULL;                     // now owned by ifi above

  yyin = include_file;
  yypush_buffer_state( yy_create_buffer( yyin, YY_BUF_SIZE ) );
  print_params.conf_path = check_strdup( we.we_wordv[0] );
  yylineno = 0;

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
  if ( !OPT_LANG_IS( DIGRAPHS ) && false_set( &digraph_warned ) )
    print_warning( &yylloc,
      "digraphs not supported%s\n", C_LANG_WHICH( DIGRAPHS )
    );
  SET_TOKEN_TO( token );
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
  SET_TOKEN_TO( token );
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
NS            [^ \f\r\t\v]

identifier    {L}({L}|{D})*
sname         {identifier}({S}*::{S}*{identifier})+
dtor_sname    ({identifier}{S}*::{S}*)+(~|compl{S}){S}*{identifier}
oper_sname    ({identifier}{S}*::{S}*)+operator{NI}
hyphenated    [a-z]+-([a-z]+-)*[a-z]+

glob_scope    \*?({identifier}\*?)*
glob          (\*\*|{glob_scope})({S}*::{S}*{glob_scope})*

cstr_pfx      L|u8?|U
rstr_pfx      {cstr_pfx}?R\"[^ \f\n\r\t\v()\\]*"("
int_sfx       [lL][lL]?[uU]?|wb|WB|[uU]([lL][lL]?|wb|WB|[zZ])?|[zZ][uU]?
set_option    [^=; \f\n\r\t\v]+

bin_int       0[bB]{B}+('{B}+)*{int_sfx}?
oct_int       0{O}*('{O}+)*{int_sfx}?
dec_int       [1-9]{D}*('{D}+)*{int_sfx}?
hex_int       0[xX]{H}+('{H}+)*{int_sfx}?

/*
 * For "include" files.
 */
%x X_INCL

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
                SET_TOKEN;
                return Y__Atomic_SPEC;
              }

              /*
               * Special case: if "const[ant]" is immediately followed by one
               * of "eval[uation]", "expr[ession]", or "init[ialization]",
               * return a special English version of the "const" token to
               * disambiguate it (as part of one of those three storage
               * classes) from the normal "const" that's a CV qualifier.
               */
const(ant)?/{S}+(eval(uation)?|expr(ession)?|init(ialization)?){NI} {
                SET_TOKEN;
                return Y_const_ENG;
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
                SET_TOKEN;
                lexer_keyword_ctx = C_KW_CTX_MBR_FUNC;
                return Y_declare;
              }

              /*
               * Special case: implement "include" files entirely within the
               * lexer.  See:
               *
               * http://westes.github.io/flex/manual/Multiple-Input-Buffers.html
               */
include{S}*\" {
                strbuf_init( &str_lit_buf );
                //
                // Save the start location because we want to use it as the
                // location for the literal, not its end location.
                //
                str_lit_loc = yylloc;
                str_lit_loc.first_column = yylloc.last_column;
                BEGIN( X_INCL );
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
^{S}*q{S}+{NS} {
                char const *s = yytext + yylloc.last_column;
                // Set the offending token to the one past the `q`.
                while ( !isspace( s[-1] ) )
                  --s;
                SET_TOKEN_TO( s );
                return Y_LEXER_ERROR;
              }
^{S}*q{S}*    { SET_TOKEN_TO( L_quit ); return Y_quit; }

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
                SET_TOKEN;

                c_sname_t sname;
                if ( c_sname_parse_dtor( lexer_token, &sname ) ) {
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
                SET_TOKEN;

                c_sname_t sname;
                size_t const sname_len = c_sname_parse( lexer_token, &sname );
                assert( sname_len > 0 );

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
                SET_TOKEN;

                c_sname_t sname;
                size_t const sname_len = c_sname_parse( lexer_token, &sname );
                if ( sname_len > 0 ) {
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
                      return Y_TYPEDEF_SNAME;
                    }
                  }

                  //
                  // 3. Otherwise, reject it.
                  //
                  c_sname_cleanup( &sname );
                }
                YY_USER_ACTION_UNDO_FOR_REJECT;
                REJECT;
              }

              /*
               * Special case: match hyphenated tokens.  We need a separate
               * rule because '-' isn't a valid character in an identifier.
               */
{hyphenated}  {
                SET_TOKEN;
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

{identifier}  {
                SET_TOKEN;

                //
                // 1. See if it's a cdecl keyword.
                //
        find_cdecl_keyword:
                NO_OP;
                cdecl_keyword_t const *const cdk =
                  lexer_cdecl_keyword_find( lexer_token );
                if ( cdk != NULL ) {
                  if ( cdk->lang_syn == NULL ) {
                    if ( cdk->literal == L_set ) {
                      //
                      // For the "set" command, we want to allow (almost) any
                      // character sequence for the command's options, so we
                      // use an exclusive start state.
                      //
                      BEGIN( X_SET );
                    }
                    else if ( cdk->literal == L_show ) {
                      //
                      // For the "show" command, we need to allow globs.
                      //
                      BEGIN( S_SHOW );
                    }
                    return cdk->y_token_id;
                  }
                  char const *const literal = c_lang_literal( cdk->lang_syn );
                  if ( literal != NULL ) {
                    SET_TOKEN_TO( literal );
                    goto find_c_keyword;
                  }
                }

                if ( (lexer_find & LEXER_FIND_TYPES) != 0 ) {
                  //
                  // 2. See if it's a typedef'd type.
                  //
                  SNAME_VAR_INIT_NAME( sname, lexer_token );
                  c_typedef_t const *const tdef =
                    c_typedef_find_sname( &sname );
                  if ( tdef != NULL ) {
                    yylval.tdef = tdef;
                    return Y_TYPEDEF_NAME;
                  }
                }

        find_c_keyword:
                if ( (lexer_find & LEXER_FIND_C_KEYWORDS) != 0 ) {
                  //
                  // 3. See if it's a C/C++ keyword.
                  //
                  c_keyword_t const *const ck =
                    c_keyword_find( lexer_token, opt_lang, lexer_keyword_ctx );
                  if ( ck != NULL ) {
                    yylval.tid = ck->tid;
                    return ck->y_token_id;
                  }
                }

                //
                // 4. Otherwise, it's just an ordinary name.
                //
                yylval.name = check_strdup( lexer_token );
                return Y_NAME;
              }

              /* Integer literals. */
{bin_int}     {
                SET_TOKEN;
                yylval.uint_val = parse_int( 2 );
                return Y_UINT_LIT;
              }
{oct_int}     {
                SET_TOKEN;
                yylval.uint_val = parse_int( 8 );
                return Y_UINT_LIT;
              }
{dec_int}     {
                SET_TOKEN;
                yylval.uint_val = parse_int( 10 );
                return Y_UINT_LIT;
              }
{hex_int}     {
                SET_TOKEN;
                yylval.uint_val = parse_int( 16 );
                return Y_UINT_LIT;
              }

              /*
               * Special case: if '[' is immediately followed by another '[',
               * return a distinct token to decrease the number of shift/reduce
               * conflicts.
               *
               * (This doesn't use | to have the actions fall through to
               * eliminate a Flex warning with trailing context.)
               */
"<:"/{S}*"<:"   { set_digraph( "[" );  return Y_ATTR_BEGIN; }
"??("/{S}*"??(" { set_trigraph( "[" ); return Y_ATTR_BEGIN; }
"["/{S}*"["     { SET_TOKEN;           return Y_ATTR_BEGIN; }

              /* Digraphs. */
"<%"          { set_digraph( "{" ); return '{'; }
"%>"          { set_digraph( "}" ); return '}'; }
"<:"          { set_digraph( "[" ); return '['; }
":>"          { set_digraph( "]" ); return ']'; }

              /* Trigraphs. */
"??'="        { set_trigraph( "^=" ); return Y_CARET_EQUAL; }
"??!="        { set_trigraph( "|=" ); return Y_PIPE_EQUAL ; }
"??!??!"      { set_trigraph( "||" ); return Y_PIPE2      ; }
"??!"         { set_trigraph( "|"  ); return Y_PIPE       ; }
"??'"         { set_trigraph( "^"  ); return Y_CARET      ; }
"??("         { set_trigraph( "["  ); return '['          ; }
"??)"         { set_trigraph( "]"  ); return ']'          ; }
"??<"         { set_trigraph( "{"  ); return '{'          ; }
"??>"         { set_trigraph( "}"  ); return '}'          ; }
"??-"         { set_trigraph( "~"  ); return Y_TILDE      ; }

              /*
               * Special case: if "::" is immediately followed by a '*', return
               * a distinct token to make it possible to distinguish between:
               *
               *    <name>::<name>::<name>
               *    <name>::<name>::*
               *
               * in an LALR(1) parser.
               */
"::"{S}*"*"   { SET_TOKEN; return Y_COLON2_STAR       ; }
"::"          { SET_TOKEN; return Y_COLON2            ; }

              /* Multi-character operators. */
"!="          { SET_TOKEN; return Y_EXCLAM_EQUAL      ; }
"\"\""        { SET_TOKEN; return Y_QUOTE2            ; }
"%="          { SET_TOKEN; return Y_PERCENT_EQUAL     ; }
"&&"          { SET_TOKEN; return Y_AMPER2            ; }
"&="          { SET_TOKEN; return Y_AMPER_EQUAL       ; }
"*="          { SET_TOKEN; return Y_STAR_EQUAL        ; }
"++"          { SET_TOKEN; return Y_PLUS2             ; }
"+="          { SET_TOKEN; return Y_PLUS_EQUAL        ; }
"--"          { SET_TOKEN; return Y_MINUS2            ; }
"-="          { SET_TOKEN; return Y_MINUS_EQUAL       ; }
"->"          { SET_TOKEN; return Y_ARROW             ; }
"->*"         { SET_TOKEN; return Y_ARROW_STAR        ; }
".*"          { SET_TOKEN; return Y_DOT_STAR          ; }
"..."         { SET_TOKEN; return Y_ELLIPSIS          ; }
"/="          { SET_TOKEN; return Y_SLASH_EQUAL       ; }
"<<"          { SET_TOKEN; return Y_LESS2             ; }
"<<="         { SET_TOKEN; return Y_LESS2_EQUAL       ; }
"<="          { SET_TOKEN; return Y_LESS_EQUAL        ; }
"<=>"         { SET_TOKEN; return Y_LESS_EQUAL_GREATER; }
"=="          { SET_TOKEN; return Y_EQUAL2            ; }
">="          { SET_TOKEN; return Y_GREATER_EQUAL     ; }
">>"          { SET_TOKEN; return Y_GREATER2          ; }
">>="         { SET_TOKEN; return Y_GREATER2_EQUAL    ; }
"?:"          { SET_TOKEN; return Y_QMARK_COLON       ; }
"^="          { SET_TOKEN; return Y_CARET_EQUAL       ; }
"|="          { SET_TOKEN; return Y_PIPE_EQUAL        ; }
"||"          { SET_TOKEN; return Y_PIPE2             ; }

              /* Single-character operators having alternative tokens. */
"!"           { SET_TOKEN; return Y_EXCLAM            ; }
"&"           { SET_TOKEN; return Y_AMPER             ; }
"^"           { SET_TOKEN; return Y_CARET             ; }
"|"           { SET_TOKEN; return Y_PIPE              ; }
"~"           { SET_TOKEN; return Y_TILDE             ; }

              /* Synonym for "help", not "?:". */
"?"           { SET_TOKEN; return Y_help              ; }

              /* Single-character operators and miscellaneous punctuation. */
"%"           |
"("           |
")"           |
"*"           |
"+"           |
","           |
"-"           |
"."           |
"/"           |
":"           | /* Not an operator: used for bitfields. */
";"           |
"<"           |
<*>"="        | /* allow in "set" command also, e.g.: set lang=c++ */
">"           |
"["           |
"]"           |
"{"           |
"}"           { SET_TOKEN; return lexer_token[0]    ; }

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

                BEGIN( X_RSTR );
              }

<X_RSTR>{
  [^)]+       { strbuf_putsn( &str_lit_buf, yytext, yyleng ); }
  ")"         {
                char const *d;
                for ( d = rstr_delim; *d != '\0'; ++d, ++token_column ) {
                  int const c = input();
                  if ( c == EOF ) {
                    print_error( &yylloc, "unterminated string literal\n" );
                    strbuf_reset( &str_lit_buf );
                    return Y_LEXER_ERROR;
                  }
                  if ( c != *d ) {
                    //
                    // Found a mismatch with the delimiter, e.g.:
                    //
                    //      )abc"       // delimiter
                    //      )abx"       // what was found
                    //
                    // Hence, it's not the actual delimiter, but part of the
                    // string literal: append the part that matched (")ab").
                    //
                    strbuf_printf( &str_lit_buf,
                      ")%.*s", STATIC_CAST( int, d - rstr_delim ), rstr_delim
                    );
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
                  }
                } // for

                if ( *d == '\0' ) {     // found delimiter
                  yylloc.first_line   = str_lit_loc.first_line;
                  yylloc.first_column = str_lit_loc.first_column;
                  yylval.str_val = strbuf_take( &str_lit_buf );
                  BEGIN( INITIAL );
                  return Y_STR_LIT;
                }
              }
}

{cstr_pfx}?['"] {
                strbuf_init( &str_lit_buf );
                //
                // Save the start location because we want to use it as the
                // location for the literal, not its end location.
                //
                str_lit_loc = yylloc;

                char const quote = yytext[ yyleng - 1 ];
                BEGIN( (quote == '"' ? X_STR : X_CHAR) );
              }

              /*
               * Common code between X_CHAR, X_INCL, and X_STR.  Multicharacter
               * literals are legal, but implementation-defined.
               */
<X_CHAR,X_INCL,X_STR>{
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
}

<X_CHAR>{
  [^'\\\n]+   { strbuf_putsn( &str_lit_buf, yytext, yyleng ); }
  \'          {
                yylloc.first_line   = str_lit_loc.first_line;
                yylloc.first_column = str_lit_loc.first_column;
                yylval.str_val = strbuf_take( &str_lit_buf );
                BEGIN( INITIAL );
                return Y_CHAR_LIT;
              }
}

              /*
               * Common code between X_INCL and X_STR:
               *
               * 1. X_INCL is the same as X_STR except we need to do different
               *    things upon the terminating " (which is why X_INCL exists
               *    rather than just using X_STR).
               *
               * 2. X_STR is the same as X_CHAR except " replaces '.
               */
<X_INCL,X_STR>[^"\\\n]+ {
                strbuf_putsn( &str_lit_buf, yytext, yyleng );
              }

<X_INCL>\"    {
                push_file( str_lit_buf.str, &str_lit_loc );
                strbuf_reset( &str_lit_buf );
                BEGIN( INITIAL );
              }

<X_STR>\"     {
                yylloc.first_line   = str_lit_loc.first_line;
                yylloc.first_column = str_lit_loc.first_column;
                yylval.str_val = strbuf_take( &str_lit_buf );
                BEGIN( INITIAL );
                return Y_STR_LIT;
              }

<*>"/*"       {                         /* ignore C-style comments */
                for ( int c = input(), prev = '\0'; ; prev = c, c = input() ) {
                  if ( c == EOF ) {
                    print_error( &yylloc, "unterminated comment\n" );
                    return Y_LEXER_ERROR;
                  }
                  ++token_column;
                  if ( c == '/' && prev == '*' )
                    break;
                  if ( c == '\n' )
                    newline();
                } // for
              }

<*>"//".*     ;                         /* ignore C++-style comments */
<*>#.*        ;                         /* ignore preprocessor directives */
<*>{S}+       ;                         /* ignore all other whitespace */
<*>\\\n       ;                         /* eat escaped newlines */

<*>\n         {
                SET_TOKEN;
                newline();
                BEGIN( INITIAL );
                return Y_END;
              }

<X_SET>{
  ";"         {
                SET_TOKEN;
                BEGIN( INITIAL );
                return lexer_token[0];
              }

  {set_option} {
                SET_TOKEN;
                yylval.name = check_strdup( lexer_token );
                return Y_SET_OPTION;
              }
}

<S_SHOW>{
  ";"         {
                SET_TOKEN;
                BEGIN( INITIAL );
                return lexer_token[0];
              }

  {glob}      {
                SET_TOKEN;
                yylval.name = check_strdup( lexer_token );
                return Y_GLOB;
              }
}

<*>.          {
                SET_TOKEN;
                return Y_ERROR;
              }

<*><<EOF>>    {
                SET_TOKEN;
                newline();
                BEGIN( INITIAL );

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
#ifdef __clang__
# pragma clang diagnostic pop
#endif /* __clang__ */
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif /* __GNUC__ */

////////// local functions ////////////////////////////////////////////////////

/**
 * @addtogroup lexer-group
 * @{
 */

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
  *rv_len = input_len;
  return input_buf;
}

c_loc_t lexer_loc( void ) {
  c_loc_t rv_loc;
  rv_loc.first_line = yylineno;

  if ( token_column == 0 )
    rv_loc.first_column = newline_column;
  else if ( lexer_token[0] == '\n' )
    rv_loc.first_column = token_column;
  else {
    rv_loc.first_column =
      token_column - STATIC_CAST( int, strlen( lexer_token ) );
    assert( rv_loc.first_column >= 0 );
  }

  rv_loc.last_line = rv_loc.first_line;
  rv_loc.last_column = rv_loc.first_column;
  return rv_loc;
}

char const* lexer_printable_token( void ) {
  switch ( lexer_token[0] ) {
    case '\0': return NULL;
    case '\n': return "\\n";
    default  : return lexer_token;
  } // switch
}

void lexer_reset( bool hard_reset ) {
  if ( hard_reset ) {
    is_eof = false;
    include_cleanup();
    include_init();
    newline();
    yylineno = 0;
  }
  BEGIN( INITIAL );
  digraph_warned = trigraph_warned = false;
  input_len = input_sent = 0;
  lexer_find = LEXER_FIND_ANY;
  lexer_keyword_ctx = C_KW_CTX_DEFAULT;
  strbuf_reset( &str_lit_buf );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
