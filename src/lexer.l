/*
**      cdecl -- C gibberish translator
**      src/lexer.l
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
 * Defines helper macros, data structures, variables, functions, and the
 * tokenizer for C/C++ declarations.
 */

/** @cond DOXYGEN_IGNORE */

%option nounput
%option yylineno

%{
/** @endcond */

// local
#include "cdecl.h"                      /* must go first */
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_typedef.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "util.h"
#include "parser.h"                     /* must go last */

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>                     /* for strtol(3) */
#include <stdnoreturn.h>
#include <string.h>

#ifdef __GNUC__
// Silence these warnings for Flex-generated code.
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif /* __GNUC__ */

#ifdef __clang__
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif /* __clang__ */

/// @endcond

/**
 * Asserts that we're in \a MODE of operation.
 * If not, returns the error token.
 *
 * @param MODE The `c_mode` we're asserting.
 */
#define ASSERT_MODE(MODE) \
  BLOCK( if ( c_mode != C_ ## MODE ) return Y_ERROR; )

/**
 * Sets <code>\ref lexer_token</code> to the current Flex token.
 */
#define SET_TOKEN                 SET_TOKEN_TO( yytext )

/**
 * Sets <code>\ref lexer_token</code> to \a S.
 *
 * @param S The string to set the token to.
 */
#define SET_TOKEN_TO(S)           (lexer_token = (S))

/**
 * Sets <code>\ref lexer_token</code> to \a S and returns \a S[0].
 *
 * @param S The string to set the token to.
 */
#define SET_TOKEN_TO_AND_RETURN(S) \
  BLOCK( SET_TOKEN_TO(S); return lexer_token[0]; )

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
  (BYTES_READ) = lexer_get_input( (BUF), (yy_size_t)(BYTES_MAX) )

/**
 * This code is inserted by Flex at the beginning of each rule to set the
 * current token location information.
 */
#define YY_USER_ACTION            lexer_update_loc(); /* must include ';' */

/**
 * This code must be manually inserted before a REJECT to undo YY_USER_ACTION.
 */
#define YY_USER_ACTION_UNDO_FOR_REJECT \
  column -= (int)yyleng

///////////////////////////////////////////////////////////////////////////////

/**
 * cdecl keyword info.
 */
struct cdecl_keyword {
  char const         *literal;          ///< C string literal of the keyword.
  bool                allow_explain;    ///< Allow when explaining C/C++?
  yytokentype         yy_token_id;      ///< Bison token number; or:

  /**
   * Array of language(s)/synonym-keyword pair(s).  The array is terminated by
   * an element that has #LANG_ALL for lang_ids; hence subset(s) of language(s)
   * cases come first and, failing to match opt_lang against any of those,
   * matches the last (default) element.
   */
  c_lang_lit_t const *lang_syn;
};
typedef struct cdecl_keyword cdecl_keyword_t;

/**
 * Specify that the previosuly given literal is a synonym for \a C_KEYWORD.
 *
 * @param ALLOW If `true`, allow this synonym when explaining C/C++.
 * @param ... Array of `c_lang_lit`.
 */
#define C_SYN(ALLOW,...)          ALLOW, 0, (c_lang_lit_t const[])__VA_ARGS__

/**
 * Special-case of #C_SYN when there is only one language(s)/literal pair.
 *
 * @param ALLOW If `true`, allow this synonym when explaining C/C++.
 * @param C_KEYWORD The C/C++ keyword literal (`L_xxx`) that is the synonym.
 */
#define C_SY1(ALLOW,C_KEYWORD)    C_SYN( ALLOW, { { LANG_ALL, C_KEYWORD } } )

/**
 * Specify that the previosuly given literal maps to Bison tokan \a Y_ID.
 *
 * @param Y_ID The Bison token ID (`Y_xxx`).
 */
#define TOKEN(Y_ID)               false, Y_ID, NULL

/**
 * All cdecl keywords that are (mostly) not C/C++ keywords.
 *
 * To have a literal for a cdecl keyword map to its corresponding token, use
 * #TOKEN:
 *
 *      // The "aligned" literal maps to the Y_ALIGNED token:
 *      { L_ALIGNED,        TOKEN( Y_ALIGNED            ) }
 *
 * To have a literal that is a synonym for another literal for a cdecl keyword
 * map to the other literal's same token, use #TOKEN with the other literal's
 * token:
 *
 *      // The "align" literal synonym also maps to the Y_ALIGNED token:
 *      { L_ALIGN,          TOKEN( Y_ALIGNED            ) },
 *
 * To have a literal that is English be a synonym for exactly one corresponding
 * C/C++ keyword literal, but only when converting English to gibberish, use
 * #C_SY1 with `false`:
 *
 *      // The "atomic" literal is a synonym for the "_Atomic" literal, but
 *      // only when converting from English to gibberish:
 *      { L_ATOMIC,         C_SY1( false, L__ATOMIC     ) },
 *
 * To do the same, but allow the literal at any time (i.e., also when
 * converting gibberish to English), use #C_SY1 with `true`:
 *
 *      // The "imaginary" literal is always a synonym for the "_Imaginary"
 *      // literal.
 *      { L_IMAGINARY,      C_SY1( true,  L__IMAGINARY  ) },
 *
 * To have a literal that is English be a synonym for more than one
 * corresponding C/C++ keyword depending on the current language, use #C_SYN
 * with the last row always containing LANG_ALL:
 *
 *      // The "noreturn" literal is a synonym for the "_Noreturn" literal only
 *      // in C11 and later.
 *      { L_NORETURN,       C_SYN( true,
 *                            { { LANG_C_MIN(11), L__NORETURN },
 *                              { LANG_ALL,       L_NORETURN  } } ) },
 *
 * Exceptions are `bool`, `complex`, `const`, and `volatile` that are included
 * here as cdecl keywords so each maps to its language-specific literal.
 */
static cdecl_keyword_t const CDECL_KEYWORDS[] = {
  { L_ALIGN,          TOKEN( Y_ALIGNED            ) },
  { L_ALIGNED,        TOKEN( Y_ALIGNED            ) },
  { L_ALL,            TOKEN( Y_ALL                ) },
  { L_ARRAY,          TOKEN( Y_ARRAY              ) },
  { L_AS,             TOKEN( Y_AS                 ) },
  { L_ATOMIC,         C_SY1( false, L__ATOMIC     ) },
  { L_AUTOMATIC,      C_SY1( false, L_AUTO        ) },
  { L_APPLE_BLOCK,    TOKEN( Y_APPLE_BLOCK        ) },
  { L_BOOL,           C_SYN( true,
                        { { LANG_C_MIN(99), L__BOOL },
                          { LANG_ALL,       L_BOOL  } } ) },
  { L_BYTES,          TOKEN( Y_BYTES              ) },
  { L_CAST,           TOKEN( Y_CAST               ) },
  { L_CHARACTER,      C_SY1( false, L_CHAR        ) },
  { L_COMPLEX,        C_SYN( true,
                        { { LANG_C_MAX(95), L_GNU___COMPLEX },
                          { LANG_C_MIN(99), L__COMPLEX      },
                          { LANG_ALL,       L_COMPLEX       } } ) },
  { L_COMMAND,        TOKEN( Y_COMMANDS           ) },
  { L_COMMANDS,       TOKEN( Y_COMMANDS           ) },
  { L_CONST,          C_SYN( false,
                        { { LANG_C_KNR, L_GNU___CONST },
                          { LANG_ALL,   L_CONST       } } ) },
  { L_CONSTANT,       C_SYN( false,
                        { { LANG_C_KNR, L_GNU___CONST },
                          { LANG_ALL,   L_CONST       } } ) },
  { L_CONSTRUCTOR,    TOKEN( Y_CONSTRUCTOR        ) },
  { L_CONVERSION,     TOKEN( Y_CONVERSION         ) },
  { L_DECLARE,        TOKEN( Y_DECLARE            ) },
  { L_DEFAULTED,      C_SY1( false, L_DEFAULT     ) },
  { L_DEFINE,         TOKEN( Y_DEFINE             ) },
  { L_DELETED,        C_SY1( false, L_DELETE      ) },
  { L_DESTRUCTOR,     TOKEN( Y_DESTRUCTOR         ) },
  { L_DYNAMIC,        TOKEN( Y_DYNAMIC            ) },
  { L_ENGLISH,        TOKEN( Y_ENGLISH            ) },
  { L_ENUMERATION,    C_SY1( false, L_ENUM        ) },
  { L_EXIT,           TOKEN( Y_QUIT               ) },
  { L_EXPLAIN,        TOKEN( Y_EXPLAIN            ) },
  { L_EXPORTED,       C_SY1( false, L_EXPORT      ) },
  { L_EXTERNAL,       C_SY1( false, L_EXTERN      ) },
  { L_FUNC,           TOKEN( Y_FUNCTION           ) },
  { L_FUNCTION,       TOKEN( Y_FUNCTION           ) },
  { L_HELP,           TOKEN( Y_HELP               ) },
  { L_IMAGINARY,      C_SY1( true,  L__IMAGINARY  ) },
  { L_INTEGER,        C_SY1( false, L_INT         ) },
  { L_INTO,           TOKEN( Y_INTO               ) },
  { L_LEN,            TOKEN( Y_LENGTH             ) },
  { L_LENGTH,         TOKEN( Y_LENGTH             ) },
  { L_LITERAL,        TOKEN( Y_LITERAL            ) },
  { L_MBR,            TOKEN( Y_MEMBER             ) },
  { L_MEMBER,         TOKEN( Y_MEMBER             ) },
  { L_NON_MBR,        TOKEN( Y_NON_MEMBER         ) },
  { L_NON_MEMBER,     TOKEN( Y_NON_MEMBER         ) },
  { L_NORETURN,       C_SYN( true,
                        { { LANG_C_MIN(11), L__NORETURN },
                          { LANG_ALL,       L_NORETURN  } } ) },
  { L_OF,             TOKEN( Y_OF                 ) },
  { L_OPER,           TOKEN( Y_OPERATOR           ) },
  { L_OVERRIDDEN,     C_SY1( false, L_OVERRIDE    ) },
  { L_POINTER,        TOKEN( Y_POINTER            ) },
  { L_PREDEF,         TOKEN( Y_PREDEFINED         ) },
  { L_PREDEFINED,     TOKEN( Y_PREDEFINED         ) },
  { L_PTR,            TOKEN( Y_POINTER            ) },
  { L_PURE,           TOKEN( Y_PURE               ) },
  { L_Q,              TOKEN( Y_QUIT               ) },
  { L_QUIT,           TOKEN( Y_QUIT               ) },
  { L_REF,            TOKEN( Y_REFERENCE          ) },
  { L_REFERENCE,      TOKEN( Y_REFERENCE          ) },
  { L_REINTERPRET,    TOKEN( Y_REINTERPRET        ) },
  { L_RESTRICTED,     C_SYN( false,
                        { { LANG_C_MAX(95) | LANG_CPP_ALL, L_GNU___RESTRICT },
                          { LANG_ALL, L_RESTRICT } } ) },
  { L_RET,            TOKEN( Y_RETURNING          ) },
  { L_RETURNING,      TOKEN( Y_RETURNING          ) },
  { L_RVALUE,         TOKEN( Y_RVALUE             ) },
  { L_SCOPE,          TOKEN( Y_SCOPE              ) },
  { L_SET_COMMAND,    TOKEN( Y_SET                ) },
  { L_SHOW,           TOKEN( Y_SHOW               ) },
  { L_STRUCTURE,      C_SY1( false, L_STRUCT      ) },
  { L_THREAD_LOCAL,   C_SYN( true,
                        { { LANG_C_CPP_MAX(99,03), L_GNU___THREAD  },
                          { LANG_C_MIN(11),        L__THREAD_LOCAL },
                          { LANG_ALL,              L_THREAD_LOCAL  } } ) },
  { L_TO,             TOKEN( Y_TO                 ) },
  { L_TYPE,           C_SY1( false, L_TYPEDEF     ) },
  { L_USER,           TOKEN( Y_USER               ) },
  { L_VAR,            TOKEN( Y_VARIABLE           ) },
  { L_VARARGS,        TOKEN( Y_ELLIPSIS           ) },
  { L_VARIABLE,       TOKEN( Y_VARIABLE           ) },
  { L_VARIADIC,       TOKEN( Y_ELLIPSIS           ) },
  { L_VECTOR,         TOKEN( Y_ARRAY              ) },
  { L_VOLATILE,       C_SYN( false,
                        { { LANG_C_KNR, L_GNU___VOLATILE },
                          { LANG_ALL,   L_VOLATILE       } } ) },

  // Embedded C extensions
  { L_EMC_ACCUM,      C_SYN( true,
                        { { LANG_C_99, L_EMC__ACCUM },
                          { LANG_ALL,  NULL          } } ) },
  { L_EMC_FRACT,      C_SYN( true,
                        { { LANG_C_99, L_EMC__FRACT },
                          { LANG_ALL,  NULL          } } ) },
  { L_EMC_SAT,        C_SYN( false,
                        { { LANG_C_99, L_EMC__SAT },
                          { LANG_ALL,  NULL        } } ) },
  { L_EMC_SATURATED,  C_SYN( true,
                        { { LANG_C_99, L_EMC__SAT },
                          { LANG_ALL,  NULL        } } ) },

  { NULL,             TOKEN( 0                    ) }
};

/**
 * Hyphenated English-literal-to-C-literal mapping.
 */
struct c_hyphen {
  char const *literal;                  ///< Hyphenated English literal.

  /**
   * Array of language(s)/literal pair(s).  The array is terminated by an
   * element that has #LANG_ALL for lang_ids; hence subset(s) of language(s)
   * cases come first and, failing to match opt_lang against any of those,
   * matches the last (default) element.
   */
  c_lang_lit_t const *lang_lit;
};
typedef struct c_hyphen c_hyphen_t;

/**
 * All hyphenated English keywords that are synonyms for C/C++ keywords.  These
 * are handled seperately because hyphenated words are not valid identifiers
 * (since `-` is not valid in an identifier).
 */
static c_hyphen_t const C_HYPHEN_KEYWORDS[] = {
  { L_CARRIES_DEPENDENCY_H,
    C_LANG_LIT( { LANG_ALL, L_CARRIES_DEPENDENCY } ) },

  { L_MAYBE_UNUSED_H,
    C_LANG_LIT( { LANG_ALL, L_MAYBE_UNUSED } ) },

  { L_NO_EXCEPTION,
    C_LANG_LIT( { LANG_ALL, L_NOEXCEPT } ) },

  { L_NON_DISCARDABLE,
    C_LANG_LIT( { LANG_ALL, L_NODISCARD } ) },

  { L_NON_RETURNING,
    C_LANG_LIT( { LANG_C_ALL, L__NORETURN  },
                { LANG_ALL,   L_NORETURN } ) },

  { L_NON_THROWING,
    C_LANG_LIT( { LANG_ALL, L_THROW } ) },

  { L_NON_UNIQUE_ADDRESS,
    C_LANG_LIT( { LANG_ALL, L_NO_UNIQUE_ADDRESS } ) },

  { L_THREAD_LOCAL_H,
    C_LANG_LIT( { LANG_C_CPP_MAX(99,03), L_GNU___THREAD  },
                { LANG_C_ALL,            L__THREAD_LOCAL },
                { LANG_ALL,              L_THREAD_LOCAL  } ) },

  { NULL,
    C_LANG_LIT( { LANG_ALL, NULL } ) }
};

/**
 * Hyphenated cdecl keyword English-literal-to-Bison-token mapping.
 */
struct cdecl_hyphen {
  char const *literal;                  ///< Hyphenated English literal.
  yytokentype yy_token_id;              ///< Bison token number.
};
typedef struct cdecl_hyphen cdecl_hyphen_t;

/**
 * All hyphenated English keywords that are cdecl keywords.  These are handled
 * seperately because hyphenated words are not valid identifiers (since `-` is
 * not valid in an identifier).
 */
static cdecl_hyphen_t const CDECL_HYPHEN_KEYWORDS[] = {
  { L_NON_MBR,      Y_NON_MEMBER    },
  { L_NON_MEMBER,   Y_NON_MEMBER    },
  { L_USER_DEF,     Y_USER_DEFINED  },
  { L_USER_DEFINED, Y_USER_DEFINED  },
  { NULL,           0               }
};

static char const lexer_token_init_buf; ///< So lexer_token is never null.

// extern variables
bool              lexer_find_typedef = true;
/** Guaranteed pointer of `yytext`. */
char const       *lexer_token = &lexer_token_init_buf;

// local variables
static int        column;               ///< Column position of current token.
static int        column_newline;       ///< Column position of newline.
static char      *input_buf;            ///< Entire current input line.
static size_t     input_len;            ///< Length of line in `input_buf`.
static size_t     input_sent;           ///< How many bytes returned to Flex.
static bool       is_eof;               ///< Encountered EOF?

// local functions
noreturn
static void       lexer_fatal( char const* );

static void       lexer_update_loc( void );

PJL_WARN_UNUSED_RESULT
static bool parse_identifier( char const*, char const** );

////////// local functions ////////////////////////////////////////////////////

/**
 * Finds a hyphenated English literal that is a synonym for a C/C++ language
 * literal.
 *
 * @param s The string to find.
 * @return Returns the corresponding C/C++ language literal or null if none.
 */
PJL_WARN_UNUSED_RESULT
static char const* c_hyphen_find( char const *s ) {
  assert( s != NULL );
  // the list is small, so linear search is good enough
  for ( c_hyphen_t const *h = C_HYPHEN_KEYWORDS; h->literal; ++h ) {
    if ( strcmp( s, h->literal ) == 0 )
      return c_lang_literal( h->lang_lit );
  } // for
  return NULL;
}

/**
 * Finds a hyphenated English literal that is a cdecl keyword.
 *
 * @param s The string to find.
 * @return Returns the corresponding Bison token, if found; Y_ERROR otherwise.
 */
PJL_WARN_UNUSED_RESULT
static yytokentype cdecl_hyphen_find( char const *s ) {
  assert( s != NULL );
  // the list is small, so linear search is good enough
  for ( cdecl_hyphen_t const *h = CDECL_HYPHEN_KEYWORDS; h->literal; ++h ) {
    if ( strcmp( s, h->literal ) == 0 )
      return h->yy_token_id;
  } // for
  return Y_ERROR;
}

/**
 * Finds a cdecl keyword matching \a s, if any.
 *
 * @param s The string to find.
 * @return Returns a pointer to the corresponding keyword or null if not found.
 */
PJL_WARN_UNUSED_RESULT
static cdecl_keyword_t const* cdecl_keyword_find( char const *s ) {
  assert( s != NULL );
  // the list is small, so linear search is good enough
  for ( cdecl_keyword_t const *k = CDECL_KEYWORDS; k->literal != NULL; ++k ) {
    if ( strcmp( s, k->literal ) != 0 )
      continue;
    if ( c_mode == C_ENGLISH_TO_GIBBERISH ||
         (k->lang_syn != NULL && k->allow_explain) )
      return k;
    break;
  } // for
  return NULL;
}

/**
 * Parses a scoped name, e.g. `a::b::c`.
 *
 * @param s The string to parse.
 * @param sname The `c_sname` to parse into.
 * @return Returns `true` only if the scoped name was successfully parsed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_sname_parse( char const *s, c_sname_t *sname ) {
  assert( s != NULL );
  assert( sname != NULL );

  c_sname_t rv;
  c_sname_init( &rv );

  for ( char const *end; parse_identifier( s, &end ); ) {
    char *const name = strndup( s, (size_t)(end - s) );

    // Ensure that the name is NOT a keyword.
    c_keyword_t const *const k = c_keyword_find( name, opt_lang );
    if ( k != NULL ) {
      FREE( name );
      c_sname_free( &rv );
      return false;
    }

    c_sname_append_name( &rv, name );
    SKIP_WS( end );
    if ( strncmp( end, "::", 2 ) != 0 )
      break;
    s = end + 2;
    SKIP_WS( s );
  } // for

  *sname = rv;
  return true;
}

/**
 * Parses a scoped destructor name, e.g. `S::T::~T()`.
 *
 * @param s The string to parse.
 * @param sname The `c_sname` to parse into.
 * @return Returns `true` only if the scoped name was successfully parsed.
 */
PJL_WARN_UNUSED_RESULT
static bool c_sname_parse_dtor( char const *s, c_sname_t *sname ) {
  assert( s != NULL );
  assert( sname != NULL );

  bool parsed_tilde = false;
  c_sname_t rv;
  c_sname_init( &rv );

  for ( char const *end; parse_identifier( s, &end ); ) {
    char *const name = strndup( s, (size_t)(end - s) );

    // Ensure that the name is NOT a keyword.
    c_keyword_t const *const k = c_keyword_find( name, opt_lang );
    if ( k != NULL ) {
      FREE( name );
      if ( k->literal == L_COMPL ) {    // k->literal is set to L_* so == is OK
        char const *const t = s + strlen( L_COMPL );
        if ( isspace( *t ) ) {          // except treat "compl" as '~'
          s = t + 1;
          goto tilde;
        }
      }
      c_sname_free( &rv );
      return false;
    }

    c_sname_append_name( &rv, name );
    if ( parsed_tilde )
      break;
    SKIP_WS( end );
    if ( strncmp( end, "::", 2 ) != 0 )
      break;
    s = end + 2;
    SKIP_WS( s );
    if ( *s == '~' ) {
      ++s;
tilde:
      SKIP_WS( s );
      parsed_tilde = true;
    }
  } // for

  *sname = rv;
  return true;
}

/**
 * Gets a line of input for Flex and keeps a copy for use later if printing an
 * error message.
 *
 * @param buf A pointer to the buffer to write into.
 * @param buf_capacity The capacity of \a buf.
 * @return Returns the number of bytes read.
 */
PJL_WARN_UNUSED_RESULT
static yy_size_t lexer_get_input( char *buf, yy_size_t buf_capacity ) {
  assert( buf != NULL );
  yy_size_t input_avail = input_len - input_sent;
  if ( input_avail == 0 ) {
    static size_t input_capacity;
    ssize_t bytes_read = getline( &input_buf, &input_capacity, yyin );
    if ( unlikely( bytes_read == -1 ) ) {
      FERROR( yyin );
      bytes_read = 0;
    }
    input_avail = input_len = (yy_size_t)bytes_read;
    input_sent = 0;
  }
  //
  // Given that the default buffer capacity (YY_READ_BUF_SIZE) for Flex is
  // 8192, it's unlikely that this will ever be true and that we'll have to
  // return the input line in chunks; but might as well code for the case.
  //
  if ( input_avail > buf_capacity )
    input_avail = buf_capacity;
  memcpy( buf, input_buf + input_sent, input_avail );
  input_sent += input_avail;
  return input_avail;
}

/**
 * Resets the column position upon encountering a newline.
 */
static inline void newline( void ) {
  column_newline = column;
  column = 0;
}

/**
 * Parses a C/C++ identifier.
 *
 * @param s The string to parse.
 * @param end Upon successful return, set to point to one character past the
 * end of the identifier.
 * @return Returns `true` only if an identifier was parsed.
 */
PJL_WARN_UNUSED_RESULT
static bool parse_identifier( char const *s, char const **end ) {
  assert( s != NULL );
  assert( end != NULL );
  if ( !(isalpha( s[0] ) || s[0] == '_') )
    return false;
  while ( *++s != '\0' && is_ident( *s ) )
    ;
  *end = s;
  return true;
}

/**
 * Parses an integer from <code>\ref lexer_token</code>.
 *
 * @param base The integer base to use.
 * @return Returns the integer value.
 */
PJL_WARN_UNUSED_RESULT
static inline int parse_int( int base ) {
  //
  // We don't need to check for errors from strtol(3) here since the lexer
  // guarantees that we'll be passed only a valid number token.
  //
  // Standard strtol(3) doesn't understand a "0b" (binary) prefix, so skip over
  // it if the base is 2.
  //
  return (int)strtol( lexer_token + (base == 2 ? 2 : 0), NULL, base );
}

/**
 * Standard lex function to know whether to continue parsing upon reaching EOF.
 *
 * @return Returns 1 if done or 0 to continue parsing (a new file set via
 * \c yyin).
 */
PJL_WARN_UNUSED_RESULT
static int yywrap( void ) {
  return 1 /* done */;
}

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

%}

letter        [A-Za-z_]
B             [01]
O             [0-7]
D             [0-9]
H             [0-9A-Fa-f]
S             [ \f\r\t\v]

identifier    {letter}({letter}|{D})*

dtor_sname    ({identifier}{S}*::{S}*)+(~{S}*|compl{S}+){identifier}
hyphenated    [a-z]+-([a-z]+-)*[a-z]+
int_suffix    ([lL][lL]?[uU]?)|([uU][lL]?[lL]?)
set_option    [^=; \f\n\r\t\v]+
sname         {identifier}({S}*::{S}*{identifier})+

bin_int       0[bB]{B}+{int_suffix}?
oct_int       0{O}*{int_suffix}?
dec_int       [1-9]{D}*{int_suffix}?
hex_int       0[xX]{H}+{int_suffix}?

/*
 * For the "set" command, we want to allow (almost) any character sequence for
 * the command's options, so we use an exclusive start state.
 */
%x SET_COMMAND

%%
              /*
               * Special case: if the _Atomic keyword is immediately followed
               * by a '(', it is interpreted as a type specifier, not as a type
               * qualifier.
               */
_Atomic/{S}*\(  { SET_TOKEN; return Y__ATOMIC_SPEC; }

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
                  if ( c_sname_is_ctor( &sname ) ) {
                    yylval.sname = sname;
                    return Y_DESTRUCTOR_SNAME;
                  }
                  c_sname_free( &sname );
                }
                YY_USER_ACTION_UNDO_FOR_REJECT;
                REJECT;
              }

              /*
               * Special case: handle scoped names in the lexer so:
               *
               * 1. Previously declared scoped types are recognized as such,
               *    e.g.:
               *
               *          define S::T as int
               *          explain S::T x
               *
               * 2. Constructors are recognized as such, specifically, when the
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
               */
{sname}       {
                SET_TOKEN;
                c_sname_t sname;
                if ( c_sname_parse( lexer_token, &sname ) ) {
                  if ( lexer_find_typedef ) {
                    //
                    // 1. See if it's a typedef'd type.
                    //
                    c_typedef_t const *const t = c_typedef_find( &sname );
                    if ( t != NULL ) {
                      yylval.ast = CONST_CAST( c_ast_t*, t->ast );
                      c_sname_free( &sname );
                      return Y_TYPEDEF_SNAME;
                    }
                  }

                  //
                  // 2. See if it's a constructor name.
                  //
                  if ( c_sname_is_ctor( &sname ) ) {
                    yylval.sname = sname;
                    return Y_CONSTRUCTOR_SNAME;
                  }

                  //
                  // 3. Otherwise, reject it.
                  //
                  c_sname_free( &sname );
                }
                YY_USER_ACTION_UNDO_FOR_REJECT;
                REJECT;
              }

{identifier}  {
                SET_TOKEN;

                //
                // 1. See if it's a cdecl keyword.
                //
                cdecl_keyword_t const *cdk = cdecl_keyword_find( lexer_token );
                if ( cdk != NULL ) {
                  if ( cdk->lang_syn == NULL ) {
                    if ( cdk->literal == L_SET_COMMAND ) {
                      //
                      // For the "set" command, we want to allow (almost) any
                      // character sequence for the command's options, so we
                      // use an exclusive start state.
                      //
                      BEGIN( SET_COMMAND );
                    }
                    return cdk->yy_token_id;
                  }
                  char const *const literal = c_lang_literal( cdk->lang_syn );
                  if ( literal != NULL ) {
                    SET_TOKEN_TO( literal );
                    goto find_c_keyword;
                  }
                }

                if ( lexer_find_typedef ) {
                  //
                  // 2. See if it's a typedef'd type.
                  //
                  SNAME_VAR_INIT( typedef_name, lexer_token );
                  c_typedef_t const *const t = c_typedef_find( &typedef_name );
                  if ( t != NULL ) {
                    yylval.ast = CONST_CAST( c_ast_t*, t->ast );
                    return Y_TYPEDEF_NAME;
                  }
                }

                //
                // 3. See if it's a C/C++ keyword.
                //
        find_c_keyword:
                NO_OP;
                c_keyword_t const *ck = c_keyword_find( lexer_token, opt_lang );
                if ( ck == NULL && c_mode == C_ENGLISH_TO_GIBBERISH ) {
                  //
                  // We have to look-up attribute names when parsing English
                  // because there can't be any special syntax for them, e.g.:
                  //
                  //      declare x as deprecated int
                  //
                  ck = c_attribute_find( lexer_token );
                }
                else {
                  //
                  // We do NOT look-up attribute names when parsing C++ because
                  // they are not reserved words.  For example:
                  //
                  //      [[noreturn]] void noreturn();
                  //
                  // is legal.
                  //
                }
                if ( ck != NULL ) {
                  yylval.type_id = ck->type_id;
                  return ck->yy_token_id;
                }

                //
                // 4. Otherwise, it's just an ordinary name.
                //
                yylval.name = check_strdup( lexer_token );
                return Y_NAME;
              }

{hyphenated}  {
                SET_TOKEN;

                char const *const c_keyword = c_hyphen_find( lexer_token );
                if ( c_keyword != NULL ) {
                  ASSERT_MODE( ENGLISH_TO_GIBBERISH );
                  //
                  // The hyphenated token is an English synonym for a C/C++
                  // keyword, so set the lexer's token to that and go to the
                  // keyword-finding code.
                  //
                  SET_TOKEN_TO( c_keyword );
                  goto find_c_keyword;
                }

                return cdecl_hyphen_find( lexer_token );
              }

{bin_int}     { SET_TOKEN; yylval.number = parse_int(  2 ); return Y_NUMBER; }
{oct_int}     { SET_TOKEN; yylval.number = parse_int(  8 ); return Y_NUMBER; }
{dec_int}     { SET_TOKEN; yylval.number = parse_int( 10 ); return Y_NUMBER; }
{hex_int}     { SET_TOKEN; yylval.number = parse_int( 16 ); return Y_NUMBER; }

"<:<:"        { SET_TOKEN_TO( "[[" ); return Y_LBRACKET2; }
":>:>"        { SET_TOKEN_TO( "]]" ); return Y_RBRACKET2; }

"<:"          { SET_TOKEN_TO_AND_RETURN( "[" ); }
":>"          { SET_TOKEN_TO_AND_RETURN( "]" ); }

"??(??("      { SET_TOKEN_TO( "[[" ); return Y_LBRACKET2; }
"??)??)"      { SET_TOKEN_TO( "]]" ); return Y_RBRACKET2; }
"??'="        { SET_TOKEN_TO( "^=" ); return Y_CIRC_EQ  ; }
"??!="        { SET_TOKEN_TO( "|=" ); return Y_PIPE_EQ  ; }
"??!??!"      { SET_TOKEN_TO( "||" ); return Y_PIPE2    ; }

"??!"         { SET_TOKEN_TO( "|" ); return Y_PIPE      ; }
"??'"         { SET_TOKEN_TO( "^" ); return Y_CIRC      ; }
"??("         { SET_TOKEN_TO_AND_RETURN( "[" ); }
"??)"         { SET_TOKEN_TO_AND_RETURN( "]" ); }
"??-"         { SET_TOKEN_TO( "~" ); return Y_TILDE     ; }

              /*
               * Special case: if "::" is immediately followed by a '*', return
               * a distinct token to make it possible to distinguish between:
               *
               *    <name>::<name>::<name>
               *    <name>::<name>::*
               *
               * in an LALR(1) parser.
               */
"::"{S}*"*"   { SET_TOKEN; return Y_COLON2_STAR     ; }
"::"          { SET_TOKEN; return Y_COLON2          ; }

              /* Multi-character operators. */
"!="          { SET_TOKEN; return Y_EXCLAM_EQ       ; }
"\"\""        { SET_TOKEN; return Y_QUOTE2          ; }
"%="          { SET_TOKEN; return Y_PERCENT_EQ      ; }
"&&"          { SET_TOKEN; return Y_AMPER2          ; }
"&="          { SET_TOKEN; return Y_AMPER_EQ        ; }
"*="          { SET_TOKEN; return Y_STAR_EQ         ; }
"++"          { SET_TOKEN; return Y_PLUS2           ; }
"+="          { SET_TOKEN; return Y_PLUS_EQ         ; }
"--"          { SET_TOKEN; return Y_MINUS2          ; }
"-="          { SET_TOKEN; return Y_MINUS_EQ        ; }
"->"          { SET_TOKEN; return Y_ARROW           ; }
"->*"         { SET_TOKEN; return Y_ARROW_STAR      ; }
".*"          { SET_TOKEN; return Y_DOT_STAR        ; }
"..."         { SET_TOKEN; return Y_ELLIPSIS        ; }
"/="          { SET_TOKEN; return Y_SLASH_EQ        ; }
"<<"          { SET_TOKEN; return Y_LESS2           ; }
"<<="         { SET_TOKEN; return Y_LESS2_EQ        ; }
"<="          { SET_TOKEN; return Y_LESS_EQ         ; }
"<=>"         { SET_TOKEN; return Y_LESS_EQ_GREATER ; }
"=="          { SET_TOKEN; return Y_EQ2             ; }
">="          { SET_TOKEN; return Y_GREATER_EQ      ; }
">>"          { SET_TOKEN; return Y_GREATER2        ; }
">>="         { SET_TOKEN; return Y_GREATER2_EQ     ; }
"?"           { SET_TOKEN; return Y_HELP            ; }
"?:"          { SET_TOKEN; return Y_QMARK_COLON     ; }
"[["          { SET_TOKEN; return Y_LBRACKET2       ; }
"]]"          { SET_TOKEN; return Y_RBRACKET2       ; }
"^="          { SET_TOKEN; return Y_CIRC_EQ         ; }
"|="          { SET_TOKEN; return Y_PIPE_EQ         ; }
"||"          { SET_TOKEN; return Y_PIPE2           ; }

              /* Single-character operators having alternative tokens. */
"!"           { SET_TOKEN; return Y_EXCLAM          ; }
"&"           { SET_TOKEN; return Y_AMPER           ; }
"^"           { SET_TOKEN; return Y_CIRC            ; }
"|"           { SET_TOKEN; return Y_PIPE            ; }
"~"           { SET_TOKEN; return Y_TILDE           ; }

              /* Single-character operators. */
"%"           |
"("           |
")"           |
"*"           |
"+"           |
","           |
"-"           |
"."           |
"/"           |
";"           |
"<"           |
<*>"="        | /* allow in "set" command also, e.g.: set lang=c++ */
">"           |
"["           |
"]"           |
"{"           |
"}"           { SET_TOKEN; return lexer_token[0]    ; }

<*>"/*"       {                         /* ignore C-style comments */
                for ( int c = input(), prev_c = '\0'; ;
                      prev_c = c, c = input() ) {
                  if ( c == '\0' || c == EOF )
                    return Y_ERROR;
                  ++column;
                  if ( c == '/' && prev_c == '*' )
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

<SET_COMMAND>{
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

<*>.          {
                SET_TOKEN;
                return Y_ERROR;
              }

<*><<EOF>>    {
                SET_TOKEN;
                newline();
                BEGIN( INITIAL );
                //
                // The first time we encounter EOF, we want to treat it as if
                // it were a newline by returning Y_END so commands in the
                // parser always end in Y_END.
                //
                // Requesting more characters after encountering EOF will
                // simply continue to return EOF, so the second time we
                // encounter EOF, treat it as EOF by returning no token.
                //
                return (is_eof = !is_eof) ? Y_END : YY_NULL;
              }

%%

/// @endcond

////////// local functions ////////////////////////////////////////////////////

/**
 * Called by Flex only when there's a fatal error.
 *
 * @param msg The error message to print.
 */
static noreturn void lexer_fatal( char const *msg ) {
  //
  // This is defined down here to avoid having to declare yy_fatal_error
  // ourselves and having to get it right being subject to possible changes in
  // its signature in different Flex versions.
  //
  INTERNAL_ERR( "%s\n", msg );

  // Not reached -- "called" just to silence the "unused function" warning.
  yy_fatal_error( msg );
}

/**
 * Update the parser's location.
 * @note This is called by Flex via `YY_USER_ACTION`.
 */
static void lexer_update_loc( void ) {
  yylloc.first_line   = yylloc.last_line = (int)yylineno;
  yylloc.first_column = column;
  yylloc.last_column  = column + (int)yyleng - 1;
  column += (int)yyleng;
}

////////// extern functions ///////////////////////////////////////////////////

PJL_WARN_UNUSED_RESULT
char const* lexer_input_line( size_t *plen ) {
  if ( plen != NULL )
    *plen = input_len;
  return input_buf;
}

void lexer_loc( int *pline, int *pcolumn ) {
  assert( pline != NULL );
  assert( pcolumn != NULL );

  *pline = yylineno;

  if ( column == 0 )
    *pcolumn = column_newline;
  else if ( lexer_token[0] == '\n' )
    *pcolumn = column;
  else
    *pcolumn = column - (int)strlen( lexer_token );
}

void lexer_reset( bool hard_reset ) {
  if ( hard_reset ) {
    is_eof = false;
    yylineno = 0;
  }
  BEGIN( INITIAL );
  c_mode = C_ENGLISH_TO_GIBBERISH;
  input_len = input_sent = 0;
  lexer_find_typedef = true;
  newline();
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
