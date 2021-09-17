/*
**      cdecl -- C gibberish translator
**      src/lexer.l
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
 * Defines helper macros, data structures, variables, functions, and the
 * tokenizer for C/C++ declarations.
 */

/** @cond DOXYGEN_IGNORE */

%option warn
%option yylineno

%{
/** @endcond */

// local
#include "pjl_config.h"                 /* must go first */
#include "lexer.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_typedef.h"
#include "cdecl.h"
#include "gibberish.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "strbuf.h"
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
#pragma GCC diagnostic ignored "-Wconversion"
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
 * C++ raw string delimiter maximum length.
 */
#define RSTR_DELIM_LEN_MAX        16u

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
#define YY_FATAL_ERROR(MSG)       lexer_fatal( MSG )

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
  token_column -= (int)yyleng

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
   * an element that has #LANG_ANY for lang_ids; hence subset(s) of language(s)
   * cases come first and, failing to match opt_lang against any of those,
   * matches the last (default) element.
   */
  c_lang_lit_t const *lang_syn;
};
typedef struct cdecl_keyword cdecl_keyword_t;

/**
 * Specify that the previosuly given literal is a synonym for the given
 * language-specific literals.
 *
 * @param ALLOW If `true`, allow this synonym when explaining C/C++.
 * @param ... Array of `c_lang_lit`.
 *
 * @sa #CDECL_KEYWORDS for examples.
 */
#define C_SYN(ALLOW,...)          (ALLOW), 0, (c_lang_lit_t const[])__VA_ARGS__

/**
 * Special-case of #C_SYN when there is only one language(s)/literal pair.
 *
 * @param ALLOW If `true`, allow this synonym when explaining C/C++.
 * @param C_KEYWORD The C/C++ keyword literal (`L_xxx`) that is the synonym.
 *
 * @sa #CDECL_KEYWORDS for examples.
 */
#define C_SY1(ALLOW,C_KEYWORD)    C_SYN( (ALLOW), { { LANG_ANY, C_KEYWORD } } )

/**
 * Specify that the previosuly given literal maps to Bison tokan \a Y_ID.
 *
 * @param Y_ID The Bison token ID (`Y_xxx`).
 *
 * @sa #CDECL_KEYWORDS for examples.
 */
#define TOKEN(Y_ID)               false, (Y_ID), NULL

/**
 * All cdecl keywords that are (mostly) _not_ C/C++ keywords.
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
 * To have a literal that is pseudo-English be a synonym for exactly one
 * corresponding C/C++ keyword literal, but only when converting pseudo-English
 * to gibberish, use #C_SY1 with `false`:
 *
 *      // The "atomic" literal is a synonym for the "_Atomic" literal, but
 *      // only when converting from pseudo-English to gibberish:
 *      { L_ATOMIC,         C_SY1( false, L__ATOMIC     ) },
 *
 * To do the same, but allow the literal at any time (i.e., also when
 * converting gibberish to pseudo-English), use #C_SY1 with `true`:
 *
 *      // The "imaginary" literal is always a synonym for the "_Imaginary"
 *      // literal.
 *      { L_IMAGINARY,      C_SY1( true,  L__IMAGINARY  ) },
 *
 * To have a literal that is pseudo-English be a synonym for more than one
 * corresponding C/C++ keyword depending on the current language, use #C_SYN
 * with the last row always containing #LANG_ANY:
 *
 *      // The "noreturn" literal is a synonym for the "_Noreturn" literal only
 *      // in C11 and later.
 *      { L_NORETURN,       C_SYN( true,
 *                            { { LANG_C_MIN(11), L__NORETURN },
 *                              { LANG_ANY,       L_NORETURN  } } ) },
 *
 * Exceptions are `bool`, `complex`, `const`, and `volatile` that are included
 * here as cdecl keywords so each maps to its language-specific literal.
 */
static cdecl_keyword_t const CDECL_KEYWORDS[] = {
  { L_ALIGN,          TOKEN( Y_ALIGNED                    ) },
  { L_ALIGNED,        TOKEN( Y_ALIGNED                    ) },
  { L_ALL,            TOKEN( Y_ALL                        ) },
  { L_ARRAY,          TOKEN( Y_ARRAY                      ) },
  { L_AS,             TOKEN( Y_AS                         ) },
  { L_ATOMIC,         C_SY1( false, L__ATOMIC             ) },
  { L_AUTOMATIC,      C_SY1( false, L_AUTO                ) },
  { L_APPLE_BLOCK,    TOKEN( Y_APPLE_BLOCK                ) },
  { L_BITS,           TOKEN( Y_BITS                       ) },
  { L_BOOL,           C_SYN( true,
                        { { LANG_C_MIN(99), L__BOOL },
                          { LANG_ANY,       L_BOOL  } } ) },
  { L_BYTES,          TOKEN( Y_BYTES                      ) },
  { H_CARRIES_DEPENDENCY,
                      C_SY1( false, L_CARRIES_DEPENDENCY  ) },
  { L_CAST,           TOKEN( Y_CAST                       ) },
  { L_CHARACTER,      C_SY1( false, L_CHAR                ) },
  { L_COMPLEX,        C_SYN( true,
                        { { LANG_C_MAX(95), L_GNU___COMPLEX },
                          { LANG_C_MIN(99), L__COMPLEX      },
                          { LANG_ANY,       L_COMPLEX       } } ) },
  { L_COMMAND,        TOKEN( Y_COMMANDS                   ) },
  { L_COMMANDS,       TOKEN( Y_COMMANDS                   ) },
  { L_CONST,          C_SYN( false,
                        { { LANG_C_KNR, L_GNU___CONST },
                          { LANG_ANY,   L_CONST       } } ) },
  { L_CONSTANT,       C_SYN( false,
                        { { LANG_C_KNR, L_GNU___CONST },
                          { LANG_ANY,   L_CONST       } } ) },
  { L_CONSTRUCTOR,    TOKEN( Y_CONSTRUCTOR                ) },
  { L_CONV,           TOKEN( Y_CONVERSION                 ) },
  { L_CONVERSION,     TOKEN( Y_CONVERSION                 ) },
  { L_CTOR,           TOKEN( Y_CONSTRUCTOR                ) },
  { L_DECLARE,        TOKEN( Y_DECLARE                    ) },
  { L_DEFAULTED,      C_SY1( false, L_DEFAULT             ) },
  { L_DEFINE,         TOKEN( Y_DEFINE                     ) },
  { L_DELETED,        C_SY1( false, L_DELETE              ) },
  { L_DESTRUCTOR,     TOKEN( Y_DESTRUCTOR                 ) },
  { H_DOUBLE_PRECISION,
                      C_SY1( false, L_DOUBLE              ) },
  { L_DTOR,           TOKEN( Y_DESTRUCTOR                 ) },
  { L_DYNAMIC,        TOKEN( Y_DYNAMIC                    ) },
  { L_ENGLISH,        TOKEN( Y_ENGLISH                    ) },
  { L_ENUMERATION,    C_SY1( false, L_ENUM                ) },
  { L_EXIT,           TOKEN( Y_QUIT                       ) },
  { L_EXPLAIN,        TOKEN( Y_EXPLAIN                    ) },
  { L_EXPORTED,       C_SY1( false, L_EXPORT              ) },
  { L_EXTERNAL,       C_SY1( false, L_EXTERN              ) },
  { H_FLOATING_POINT, C_SY1( false, L_FLOAT               ) },
  { L_FUNC,           TOKEN( Y_FUNCTION                   ) },
  { L_FUNCTION,       TOKEN( Y_FUNCTION                   ) },
  { L_HELP,           TOKEN( Y_HELP                       ) },
  { L_IMAGINARY,      C_SYN( true,
                        { { LANG_C_MIN(99), L__IMAGINARY },
                          { LANG_ANY,       L_IMAGINARY  } } ) },
  { L_INTEGER,        C_SY1( false, L_INT                 ) },
  { L_INTO,           TOKEN( Y_INTO                       ) },
  { L_LEN,            TOKEN( Y_LENGTH                     ) },
  { L_LENGTH,         TOKEN( Y_LENGTH                     ) },
  { L_LINKAGE,        TOKEN( Y_LINKAGE                    ) },
  { L_LITERAL,        TOKEN( Y_LITERAL                    ) },
  { H_MAYBE_UNUSED,   C_SY1( false, L_MAYBE_UNUSED        ) },
  { L_MBR,            TOKEN( Y_MEMBER                     ) },
  { L_MEMBER,         TOKEN( Y_MEMBER                     ) },
  { H_NO_DISCARD,     C_SY1( false, L_NODISCARD           ) },
  { H_NO_EXCEPT,      C_SY1( false, L_NOEXCEPT            ) },
  { H_NO_EXCEPTION,   C_SY1( false, L_NOEXCEPT            ) },
  { H_NO_RETURN,      C_SYN( false,
                        { { LANG_C_MIN(11), L__NORETURN },
                          { LANG_ANY,       L_NORETURN  } } ) },
  { H_NON_DISCARDABLE,
                      C_SY1( false, L_NODISCARD           ) },
  { H_NON_MBR,        TOKEN( Y_NON_MEMBER                 ) },
  { H_NON_MEMBER,     TOKEN( Y_NON_MEMBER                 ) },
  { H_NON_RETURNING,  C_SYN( false,
                        { { LANG_C_MIN(11), L__NORETURN },
                          { LANG_ANY,       L_NORETURN  } } ) },
  { H_NON_THROWING,   C_SY1( false, L_THROW               ) },
  { H_NO_UNIQUE_ADDRESS,
                      C_SY1( false, L_NO_UNIQUE_ADDRESS   ) },
  { H_NON_UNIQUE_ADDRESS,
                      C_SY1( false, L_NO_UNIQUE_ADDRESS   ) },
  { L_NORETURN,       C_SYN( true,
                        { { LANG_C_MIN(11), L__NORETURN },
                          { LANG_ANY,       L_NORETURN  } } ) },
  { L_OF,             TOKEN( Y_OF                         ) },
  { L_OPER,           TOKEN( Y_OPERATOR                   ) },
  { L_OVERRIDDEN,     C_SY1( false, L_OVERRIDE            ) },
  { L_POINTER,        TOKEN( Y_POINTER                    ) },
  { L_PREDEF,         TOKEN( Y_PREDEFINED                 ) },
  { L_PREDEFINED,     TOKEN( Y_PREDEFINED                 ) },
  { L_PTR,            TOKEN( Y_POINTER                    ) },
  { L_PURE,           TOKEN( Y_PURE                       ) },
  { L_Q,              TOKEN( Y_QUIT                       ) },
  { L_QUIT,           TOKEN( Y_QUIT                       ) },
  { L_REF,            TOKEN( Y_REFERENCE                  ) },
  { L_REFERENCE,      TOKEN( Y_REFERENCE                  ) },
  { L_REINTERPRET,    TOKEN( Y_REINTERPRET                ) },
  { L_RESTRICTED,     C_SYN( false,
                        { { LANG_C_MAX(95) | LANG_CPP_ANY, L_GNU___RESTRICT },
                          { LANG_ANY,                      L_RESTRICT } } ) },
  { L_RET,            TOKEN( Y_RETURNING                  ) },
  { L_RETURNING,      TOKEN( Y_RETURNING                  ) },
  { L_RVALUE,         TOKEN( Y_RVALUE                     ) },
  { L_SCOPE,          TOKEN( Y_SCOPE                      ) },
  { L_SET_COMMAND,    TOKEN( Y_SET                        ) },
  { L_SHOW,           TOKEN( Y_SHOW                       ) },
  { L_STRUCTURE,      C_SY1( false, L_STRUCT              ) },
  { L_THREAD_LOCAL,   C_SYN( true,
                        { { LANG_C_MIN(11), L__THREAD_LOCAL },
                          { LANG_ANY,       L_THREAD_LOCAL  } } ) },
  { H_THREAD_LOCAL,   C_SYN( true,
                        { { LANG_C_CPP_MAX(99,03), L_GNU___THREAD  },
                          { LANG_C_MIN(11),        L__THREAD_LOCAL },
                          { LANG_ANY,              L_THREAD_LOCAL  } } ) },
  { L_TO,             TOKEN( Y_TO                         ) },
  { L_TYPE,           C_SY1( false, L_TYPEDEF             ) },
  { L_USER,           TOKEN( Y_USER                       ) },
  { H_USER_DEF,       TOKEN( Y_USER_DEFINED               ) },
  { H_USER_DEFINED,   TOKEN( Y_USER_DEFINED               ) },
  { L_VAR,            TOKEN( Y_VARIABLE                   ) },
  { L_VARARGS,        TOKEN( Y_ELLIPSIS                   ) },
  { L_VARIABLE,       TOKEN( Y_VARIABLE                   ) },
  { L_VARIADIC,       TOKEN( Y_ELLIPSIS                   ) },
  { L_VECTOR,         TOKEN( Y_ARRAY                      ) },
  { L_VOLATILE,       C_SYN( false,
                        { { LANG_C_KNR, L_GNU___VOLATILE },
                          { LANG_ANY,   L_VOLATILE       } } ) },
  { L_WIDTH,          TOKEN( Y_WIDTH                      ) },

  // Embedded C extensions
  { L_EMC_ACCUM,      C_SYN( true,
                        { { LANG_C_99, L_EMC__ACCUM },
                          { LANG_ANY,  NULL         } } ) },
  { L_EMC_FRACT,      C_SYN( true,
                        { { LANG_C_99, L_EMC__FRACT },
                          { LANG_ANY,  NULL         } } ) },
  { L_EMC_SAT,        C_SYN( false,
                        { { LANG_C_99, L_EMC__SAT   },
                          { LANG_ANY,  NULL         } } ) },
  { L_EMC_SATURATED,  C_SYN( true,
                        { { LANG_C_99, L_EMC__SAT   },
                          { LANG_ANY,  NULL         } } ) },

  // Microsoft extensions
  { L_MSC_CDECL,      C_SY1( false, L_MSC___CDECL         ) },
  { L_MSC_CLRCALL,    C_SY1( false, L_MSC___CLRCALL       ) },
  { L_MSC_FASTCALL,   C_SY1( false, L_MSC___FASTCALL      ) },
  { L_MSC_STDCALL,    C_SY1( false, L_MSC___STDCALL       ) },
  { L_MSC_THISCALL,   C_SY1( false, L_MSC___THISCALL      ) },
  { L_MSC_VECTORCALL, C_SY1( false, L_MSC___VECTORCALL    ) },
  { L_MSC_WINAPI,     C_SY1( true,  L_MSC___STDCALL       ) },

  { NULL,             TOKEN( 0                            ) }
};

static char const lexer_token_init_buf; ///< So lexer_token is never NULL.

// extern variables
unsigned          lexer_find = LEXER_FIND_ALL;
c_keyword_ctx_t   lexer_keyword_ctx;

// A separate pointer for the current token allows it:
//  + To be `const`.
//  + To be assigned to either other `const` strings or string literals.
char const       *lexer_token = &lexer_token_init_buf;

// local variables
static bool       digraph_warned;       ///< Printed digraph warning once?
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
noreturn
static void       lexer_fatal( char const* );

static void       lexer_update_loc( void );

////////// local functions ////////////////////////////////////////////////////

/**
 * Finds a cdecl keyword matching \a s, if any.
 *
 * @param s The string to find.
 * @return Returns a pointer to the corresponding keyword or NULL if not found.
 */
PJL_WARN_UNUSED_RESULT
static cdecl_keyword_t const* cdecl_keyword_find( char const *s ) {
  assert( s != NULL );
  // the list is small, so linear search is good enough
  for ( cdecl_keyword_t const *k = CDECL_KEYWORDS; k->literal != NULL; ++k ) {
    if ( strcmp( s, k->literal ) != 0 )
      continue;
    if ( (lexer_find & LEXER_FIND_CDECL_KEYWORDS) != 0 ||
         (k->lang_syn != NULL && k->allow_explain) ) {
      return k;
    }
    break;
  } // for
  return NULL;
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

  for ( char const *end; (end = parse_identifier( s )) != NULL; ) {
    char *const name = strndup( s, (size_t)(end - s) );

    // Ensure that the name is NOT a keyword.
    c_keyword_t const *const k =
      c_keyword_find( name, opt_lang, C_KW_CTX_DEFAULT );
    if ( k != NULL ) {
      FREE( name );
      if ( k->literal == L_COMPL ) {    // k->literal is set to L_* so == is OK
        char const *const t = s + strlen( L_COMPL );
        if ( isspace( *t ) ) {          // except treat "compl" as '~'
          s = t + 1;
          goto tilde;
        }
      }
      break;
    }
    c_sname_append_name( &rv, name );

    SKIP_WS( end );
    if ( *end == '\0' && parsed_tilde ) {
      *sname = rv;
      return true;
    }
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

  c_sname_free( &rv );
  return false;
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
 * Resets the token column position upon encountering a newline.
 */
static inline void newline( void ) {
  newline_column = token_column;
  token_column = 0;
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
 * Sets the current token to \a token for the current digraph sequence.
 * Additionally, if the current language is K&R C, prints a warning that
 * digraphs are not supported until C89 (only once per parse).
 *
 * @param token The token the digraph maps to.
 *
 * @sa set_trigraph()
 */
static void set_digraph( char const *token ) {
  if ( opt_lang < LANG_C_95 && false_set( &digraph_warned ) )
    print_warning( &yylloc, "digraphs not supported until C95\n" );
  SET_TOKEN_TO( token );
}

/**
 * Sets the current token to \a token for the current trigraph sequence.
 * Additionally, if the current language is K&R C, prints a warning that
 * trigraphs are not supported until C89 (only once per parse).
 *
 * @param token The token the trigraph maps to.
 *
 * @sa set_digraph()
 */
static void set_trigraph( char const *token ) {
  if ( (opt_lang == LANG_C_KNR || opt_lang >= LANG_CPP_17) &&
       false_set( &trigraph_warned ) ) {
    print_warning( &yylloc,
      opt_lang == LANG_C_KNR ?
        "trigraphs not supported until C89\n" :
        "trigraphs no longer supported since C++17\n"
    );
  }
  SET_TOKEN_TO( token );
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

L             [A-Za-z_]
B             [01]
O             [0-7]
D             [0-9]
H             [0-9A-Fa-f]
NI            [^A-Za-z_0-9]
S             [ \f\r\t\v]

identifier    {L}({L}|{D})*
sname         {identifier}({S}*::{S}*{identifier})+
dtor_sname    ({identifier}{S}*::{S}*)+(~|compl{S}){S}*{identifier}
hyphenated    [a-z]+-([a-z]+-)*[a-z]+

glob_scope    \*?({identifier}\*?)*
glob          (\*\*|{glob_scope})({S}*::{S}*{glob_scope})*

chr_str_pfx   (L|u8?|U)?
rstr_pfx      {chr_str_pfx}R\"[^ \f\n\r\t\v()\\]*"("
int_suffix    ([lL][lL]?[uU]?)|([uU][lL]?[lL]?)
set_option    [^=; \f\n\r\t\v]+

bin_int       0[bB]{B}+{int_suffix}?
oct_int       0{O}*{int_suffix}?
dec_int       [1-9]{D}*{int_suffix}?
hex_int       0[xX]{H}+{int_suffix}?

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
                return Y__ATOMIC_SPEC;
              }

              /*
               * Special case: if "declare" is eventually followed by "user-
               * defined", e.g.:
               *
               *      c++decl> declare overriden user-defined \
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
declare/{S}({S}|{L})*user-def(ined)? {
                SET_TOKEN;
                lexer_keyword_ctx = C_KW_CTX_MBR_FUNC;
                return Y_DECLARE;
              }

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
                if ( c_sname_parse( lexer_token, &sname ) ) {
                  //
                  // 1. See if it's a constructor name.
                  //
                  if ( c_sname_is_ctor( &sname ) ) {
                    yylval.sname = sname;
                    return Y_CONSTRUCTOR_SNAME;
                  }

                  if ( (lexer_find & LEXER_FIND_TYPEDEFS) != 0 ) {
                    //
                    // 2. See if it's a typedef'd type.
                    //
                    c_typedef_t const *const tdef =
                      c_typedef_find_sname( &sname );
                    if ( tdef != NULL ) {
                      yylval.tdef = tdef;
                      c_sname_free( &sname );
                      return Y_TYPEDEF_SNAME;
                    }
                  }

                  //
                  // 3. Otherwise, reject it.
                  //
                  c_sname_free( &sname );
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
                cdecl_keyword_t const *const ck =
                  cdecl_keyword_find( lexer_token );
                if ( ck != NULL ) {
                  if ( ck->lang_syn == NULL ) {
                    if ( ck->literal == L_SET_COMMAND ) {
                      //
                      // For the "set" command, we want to allow (almost) any
                      // character sequence for the command's options, so we
                      // use an exclusive start state.
                      //
                      BEGIN( X_SET );
                    }
                    else if ( ck->literal == L_SHOW ) {
                      //
                      // For the "show" command, we need to allow globs.
                      //
                      BEGIN( S_SHOW );
                    }
                    return ck->yy_token_id;
                  }
                  char const *const literal = c_lang_literal( ck->lang_syn );
                  if ( literal != NULL ) {
                    SET_TOKEN_TO( literal );
                    goto find_c_keyword;
                  }
                }

                if ( (lexer_find & LEXER_FIND_TYPEDEFS) != 0 ) {
                  //
                  // 2. See if it's a typedef'd type.
                  //
                  SNAME_VAR_INIT( sname, lexer_token );
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
                  c_keyword_t const *const k =
                    c_keyword_find( lexer_token, opt_lang, lexer_keyword_ctx );
                  if ( k != NULL ) {
                    yylval.tid = k->tid;
                    return k->yy_token_id;
                  }
                }

                //
                // 4. Otherwise, it's just an ordinary name.
                //
                yylval.name = check_strdup( lexer_token );
                return Y_NAME;
              }

              /* Integer literals. */
{bin_int}     { SET_TOKEN; yylval.int_val = parse_int(  2 ); return Y_INT_LIT; }
{oct_int}     { SET_TOKEN; yylval.int_val = parse_int(  8 ); return Y_INT_LIT; }
{dec_int}     { SET_TOKEN; yylval.int_val = parse_int( 10 ); return Y_INT_LIT; }
{hex_int}     { SET_TOKEN; yylval.int_val = parse_int( 16 ); return Y_INT_LIT; }

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
"<:"          { set_digraph( "[" ); return '['; }
":>"          { set_digraph( "]" ); return ']'; }

              /* Trigraphs. */
"??'="        { set_trigraph( "^=" ); return Y_CIRC_EQ; }
"??!="        { set_trigraph( "|=" ); return Y_PIPE_EQ; }
"??!??!"      { set_trigraph( "||" ); return Y_PIPE2  ; }
"??!"         { set_trigraph( "|"  ); return Y_PIPE   ; }
"??'"         { set_trigraph( "^"  ); return Y_CIRC   ; }
"??("         { set_trigraph( "["  ); return '['      ; }
"??)"         { set_trigraph( "]"  ); return ']'      ; }
"??-"         { set_trigraph( "~"  ); return Y_TILDE  ; }

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
"^="          { SET_TOKEN; return Y_CIRC_EQ         ; }
"|="          { SET_TOKEN; return Y_PIPE_EQ         ; }
"||"          { SET_TOKEN; return Y_PIPE2           ; }

              /* Single-character operators having alternative tokens. */
"!"           { SET_TOKEN; return Y_EXCLAM          ; }
"&"           { SET_TOKEN; return Y_AMPER           ; }
"^"           { SET_TOKEN; return Y_CIRC            ; }
"|"           { SET_TOKEN; return Y_PIPE            ; }
"~"           { SET_TOKEN; return Y_TILDE           ; }

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

                size_t const delim_len = (size_t)(p - q);
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
  [^)]+       { strbuf_catsn( &str_lit_buf, yytext, yyleng ); }
  ")"         {
                char const *d;
                for ( d = rstr_delim; *d != '\0'; ++d, ++token_column ) {
                  int const c = input();
                  if ( c == EOF ) {
                    print_error( &yylloc, "unterminated string literal\n" );
                    strbuf_free( &str_lit_buf );
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
                    strbuf_catf( &str_lit_buf,
                      ")%.*s", (int)(d - rstr_delim), rstr_delim
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

{chr_str_pfx}['"] {
                strbuf_init( &str_lit_buf );
                //
                // Save the start location because we want to use it as the
                // location for the literal, not its end location.
                //
                str_lit_loc = yylloc;

                char const quote = yytext[ yyleng - 1 ];
                BEGIN( quote == '"' ? X_STR : X_CHAR );
              }

              /*
               * Common code between X_STR and X_CHAR.  Multicharacter literals
               * are legal, but implementation-defined.
               */
<X_CHAR,X_STR>{
              /* Escaped characters are copied verbatim, not interpreted. */
  \\(.|\n)    { strbuf_catsn( &str_lit_buf, yytext, yyleng ); }
  \n          {
                print_error( &str_lit_loc,
                  "unterminated %s literal\n",
                  YY_START == X_STR ? "string" : "character"
                );
                strbuf_free( &str_lit_buf );
                return Y_LEXER_ERROR;
              }
}

<X_CHAR>{
  [^'\\\n]+   { strbuf_catsn( &str_lit_buf, yytext, yyleng ); }
  \'          {
                yylloc.first_line   = str_lit_loc.first_line;
                yylloc.first_column = str_lit_loc.first_column;
                yylval.str_val = strbuf_take( &str_lit_buf );
                BEGIN( INITIAL );
                return Y_CHAR_LIT;
              }
}

<X_STR>{      /* X_STR is the same as X_CHAR except " replaces '. */
  [^"\\\n]+   { strbuf_catsn( &str_lit_buf, yytext, yyleng ); }
  \"          {
                yylloc.first_line   = str_lit_loc.first_line;
                yylloc.first_column = str_lit_loc.first_column;
                yylval.str_val = strbuf_take( &str_lit_buf );
                BEGIN( INITIAL );
                return Y_STR_LIT;
              }
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
noreturn
static void lexer_fatal( char const *msg ) {
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
  yylloc.first_column = token_column;
  yylloc.last_column  = token_column + (int)yyleng - 1;
  token_column += (int)yyleng;
}

////////// extern functions ///////////////////////////////////////////////////

char const* lexer_input_line( size_t *plen ) {
  if ( plen != NULL )
    *plen = input_len;
  return input_buf;
}

c_loc_t lexer_loc( void ) {
  c_loc_t rv;
  rv.first_line = yylineno;

  if ( token_column == 0 )
    rv.first_column = newline_column;
  else if ( lexer_token[0] == '\n' )
    rv.first_column = token_column;
  else
    rv.first_column = token_column - (int)strlen( lexer_token );

  rv.last_line = rv.first_line;
  rv.last_column = rv.first_column;
  return rv;
}

void lexer_reset( bool hard_reset ) {
  if ( hard_reset ) {
    is_eof = false;
    yylineno = 0;
  }
  BEGIN( INITIAL );
  digraph_warned = trigraph_warned = false;
  input_len = input_sent = 0;
  lexer_find = LEXER_FIND_ALL;
  lexer_keyword_ctx = C_KW_CTX_DEFAULT;
  newline();
  strbuf_free( &str_lit_buf );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
