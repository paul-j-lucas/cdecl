/*
**      cdecl -- C gibberish translator
**      src/lexer.l
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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
#include <string.h>

#ifdef __GNUC__
// Silence these warnings for flex-generated code.
#pragma GCC diagnostic ignored "-Wredundant-decls"
#endif /* __GNUC__ */

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
 * Sets <code>\ref lexer_token</code> to \a S and returns \a T.
 *
 * @param S The string to set the token to.
 * @param T The token to return.
 */
#define SET_TOKEN_TO_AND_RETURN(S,T) \
  BLOCK( SET_TOKEN_TO(S); return (T); )

/**
 * Sets <code>\ref lexer_token</code> to \a S and returns \a S[0].
 *
 * @param S The string to set the token to.
 */
#define SET_TOKEN_TO_AND_RETURN_IT(S) \
  SET_TOKEN_TO_AND_RETURN( (S), lexer_token[0] )

/**
 * Advances \a S over all \a CHARS.
 *
 * @param S The string pointer to advance.
 * @param CHARS A string containing the characters to skip over.
 * @return Returns the updated \a S.
 */
#define SKIP_CHARS(S,CHARS)       ((S) += strspn( (S), (CHARS) ))

/**
 * Advances \a S over all whitespace.
 *
 * @param S The string pointer to advance.
 * @return Returns the updated \a S.
 */
#define SKIP_WS(S)                SKIP_CHARS( (S), " \f\r\t\v" )

/**
 * Overrides flex's fatal error message to print the message in our format and
 * also exit with the status code we want.
 *
 * @param MSG The error message to print.
 */
#define YY_FATAL_ERROR(MSG)       lexer_fatal( (MSG) )

/**
 * Overrides flex's input.
 *
 * @param BUF The buffer to use.
 * @param BYTES_READ Set to the number of bytes read.
 * @param BYTES_MAX The maximum number of bytes to read.
 */
#define YY_INPUT(BUF,BYTES_READ,BYTES_MAX) \
  (BYTES_READ) = (int)lexer_get_input( (BUF), (size_t)(BYTES_MAX) )

/**
 * This code is inserted by flex at the beginning of each rule to set the
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
  char const *literal;                  ///< C string literal of the keyword.
  int         y_token;                  ///< Bison token number.
  char const *c_keyword_synonym;        ///< Synonym of C/C++ keyword, if any.
  bool        allow_explain;            ///< Allow when explaining C/C++?
};
typedef struct cdecl_keyword cdecl_keyword_t;

/**
 * Specify that the previosuly given literal is a synonym for \a C_KEYWORD.
 *
 * @param C_KEYWORD The C/C++ keyword literal (`L_xxx`) that is the synonym.
 * @param ALLOW If `true, allow this synonym when explaining C/C++.
 */
#define C_SYN(C_KEYWORD,ALLOW)  0, C_KEYWORD, ALLOW

/**
 * Specify that the previosuly given literal maps to Bison tokan \a T.
 *
 * @param T The Bison token ID (`Y_xxx`).
 */
#define TOKEN(T)                T, NULL,      false

/**
 * All cdecl keywords (that are not C/C++ keywords).
 */
static cdecl_keyword_t const CDECL_KEYWORDS[] = {
  { L__ALIGNAS,       C_SYN( L_ALIGNAS,       true  ) },
  { L_ALIGNED,        TOKEN( Y_ALIGNED )              },
  { L_ALL,            TOKEN( Y_ALL )                  },
  { L_ARRAY,          TOKEN( Y_ARRAY )                },
  { L_AS,             TOKEN( Y_AS )                   },
  { L_ATOMIC,         C_SYN( L__ATOMIC,       false ) },
  { L_AUTOMATIC,      C_SYN( L_AUTO,          false ) },
  { L_BLOCK,          TOKEN( Y_BLOCK )                },  // Apple extension
  { L__BOOL,          C_SYN( L_BOOL,          true  ) },
  { L_BYTES,          TOKEN( Y_BYTES )                },
  { L_CAST,           TOKEN( Y_CAST )                 },
  { L_CHARACTER,      C_SYN( L_CHAR,          false ) },
  { L_COMPLEX,        C_SYN( L__COMPLEX,      true  ) },
  { L_COMMAND,        TOKEN( Y_COMMANDS )             },
  { L_COMMANDS,       TOKEN( Y_COMMANDS )             },
  { L_CONSTANT,       C_SYN( L_CONST,         false ) },
  { L_CONSTRUCTOR,    TOKEN( Y_CONSTRUCTOR )          },
  { L_CONVERSION,     TOKEN( Y_CONVERSION )           },
  { L_DECLARE,        TOKEN( Y_DECLARE )              },
  { L_DEFAULTED,      C_SYN( L_DEFAULT,       false ) },
  { L_DEFINE,         TOKEN( Y_DEFINE )               },
  { L_DELETED,        C_SYN( L_DELETE,        false ) },
  { L_DESTRUCTOR,     TOKEN( Y_DESTRUCTOR )           },
  { L_DYNAMIC,        TOKEN( Y_DYNAMIC )              },
  { L_ENGLISH,        TOKEN( Y_ENGLISH )              },
  { L_ENUMERATION,    C_SYN( L_ENUM,          false ) },
  { L_EXIT,           TOKEN( Y_QUIT )                 },
  { L_EXPLAIN,        TOKEN( Y_EXPLAIN )              },
  { L_EXTERNAL,       C_SYN( L_EXTERN,        false ) },
  { L_FUNC,           TOKEN( Y_FUNCTION )             },
  { L_FUNCTION,       TOKEN( Y_FUNCTION )             },
  { L_HELP,           TOKEN( Y_HELP )                 },
  { L_IMAGINARY,      C_SYN( L__IMAGINARY,    true  ) },
  { L_INTEGER,        C_SYN( L_INT,           false ) },
  { L_INTO,           TOKEN( Y_INTO )                 },
  { L_LEN,            TOKEN( Y_LENGTH )               },
  { L_LENGTH,         TOKEN( Y_LENGTH )               },
  { L_LITERAL,        TOKEN( Y_LITERAL )              },
  { L_MBR,            TOKEN( Y_MEMBER ),              },
  { L_MEMBER,         TOKEN( Y_MEMBER ),              },
  { L_NON_MBR,        TOKEN( Y_NON_MEMBER ),          },
  { L_NON_MEMBER,     TOKEN( Y_NON_MEMBER ),          },
  { L_NORETURN,       C_SYN( L__NORETURN,     true  ) },
  { L_OF,             TOKEN( Y_OF ),                  },
  { L_OPER,           TOKEN( Y_OPERATOR )             },
  { L_OVERRIDDEN_ENG, C_SYN( L_OVERRIDE,      false ) },
  { L_POINTER,        TOKEN( Y_POINTER )              },
  { L_PREDEF,         TOKEN( Y_PREDEFINED )           },
  { L_PREDEFINED,     TOKEN( Y_PREDEFINED )           },
  { L_PTR,            TOKEN( Y_POINTER )              },
  { L_PURE,           TOKEN( Y_PURE )                 },
  { L_Q,              TOKEN( Y_QUIT )                 },
  { L_QUIT,           TOKEN( Y_QUIT )                 },
  { L_REF,            TOKEN( Y_REFERENCE )            },
  { L_REFERENCE,      TOKEN( Y_REFERENCE )            },
  { L_REINTERPRET,    TOKEN( Y_REINTERPRET )          },
  { L_RESTRICTED,     C_SYN( L_RESTRICT,      false ) },
  { L_RET,            TOKEN( Y_RETURNING )            },
  { L_RETURNING,      TOKEN( Y_RETURNING )            },
  { L_RVALUE,         TOKEN( Y_RVALUE )               },
  { L_SCOPE,          TOKEN( Y_SCOPE ),               },
  { L_SET_COMMAND,    TOKEN( Y_SET )                  },
  { L_SHOW,           TOKEN( Y_SHOW )                 },
  { L_STRUCTURE,      C_SYN( L_STRUCT,        false ) },
  { L__THREAD_LOCAL,  C_SYN( L_THREAD_LOCAL,  true  ) },
  { L_TO,             TOKEN( Y_TO )                   },
  { L_TYPE,           C_SYN( L_TYPEDEF,       false ) },
  { L_USER,           TOKEN( Y_USER )                 },
  { L_VAR,            TOKEN( Y_VARIABLE )             },
  { L_VARARGS,        C_SYN( L_ELLIPSIS,      false ) },
  { L_VARIABLE,       TOKEN( Y_VARIABLE )             },
  { L_VARIADIC,       C_SYN( L_ELLIPSIS,      false ) },
  { L_VECTOR,         TOKEN( Y_ARRAY )                },
  { NULL,             TOKEN( 0 )                      }
};

/**
 * Hyphenated English-literal-to-C-literal mapping.
 */
struct c_hyphenated {
  char const *e_literal;                ///< Hyphenated English literal.
  char const *c_literal;                ///< C language literal it maps to.
};
typedef struct c_hyphenated c_hyphenated_t;

/**
 * All hyphenated English keywords that are synonyms for C/C++ keywords.  These
 * are handled seperately because hyphenated words are not valid identifiers
 * (since `-` is not valid in an identifier).
 */
static c_hyphenated_t const C_HYPHENATED_KEYWORDS[] = {
  { "carries-dependency", L_CARRIES_DEPENDENCY  },
  { "maybe-unused",       L_MAYBE_UNUSED        },
  { "no-exception",       L_NOEXCEPT            },
  { "non-discardable",    L_NODISCARD           },
  { "non-returning",      L__NORETURN           },
  { "non-throwing",       L_THROW               },
  { "thread-local",       L_THREAD_LOCAL        },
  { NULL,                 NULL                  }
};

/**
 * Hyphenated cdecl keyword English-literal-to-Bison-token mapping.
 */
struct cdecl_hyphenated {
  char const *e_literal;                ///< Hyphenated English literal.
  int         y_token;                  ///< Bison token number.
};
typedef struct cdecl_hyphenated cdecl_hyphenated_t;

/**
 * All hyphenated English keywords that are cdecl keywords.  These are handled
 * seperately because hyphenated words are not valid identifiers (since `-` is
 * not valid in an identifier).
 */
static cdecl_hyphenated_t const E_HYPHENATED_KEYWORDS[] = {
  { "non-mbr",      Y_NON_MEMBER    },
  { "non-member",   Y_NON_MEMBER    },
  { "user-def",     Y_USER_DEFINED  },
  { "user-defined", Y_USER_DEFINED  },
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
static size_t     input_sent;           ///< How many bytes returned to flex.
static bool       is_eof;               ///< Encountered EOF?

// local functions
static void       lexer_fatal( char const* );
static void       lexer_update_loc( void );

////////// local functions ////////////////////////////////////////////////////

/**
 * Finds a hyphenated English literal that is a synonym for a C/C++ language
 * literal.
 *
 * @param s The string to find.
 * @return Returns the corresponding C/C++ language literal or null if none.
 */
static char const* c_hyphenated_find( char const *s ) {
  assert( s != NULL );
  char const *c_literal = NULL;
  for ( c_hyphenated_t const *h = C_HYPHENATED_KEYWORDS; h->e_literal; ++h ) {
    if ( strcmp( s, h->e_literal ) == 0 ) {
      c_literal = h->c_literal;
      if ( c_literal == L__NORETURN && C_LANG_IS_CPP() ) {
        //
        // _Noreturn is a special case. In C11, it's "_Noreturn"; in C++11,
        // it's "noreturn".
        //
        // Depending on the current language, we map "non-returning" either to
        // "_Noreturn" or "noreturn".
        //
        c_literal = L_NORETURN;
      }
      break;
    }
  } // for
  return c_literal;
}

/**
 * Finds a hyphenated English literal that is a cdecl keyword.
 *
 * @param s The string to find.
 * @return If found, returns the corresponding Bison token; otherwise returns
 * Y_HYPHENATED_NAME.
 */
static int cdecl_hyphenated_find( char const *s ) {
  assert( s != NULL );
  for ( cdecl_hyphenated_t const *h = E_HYPHENATED_KEYWORDS; h->e_literal; ++h ) {
    if ( strcmp( s, h->e_literal ) == 0 )
      return h->y_token;
  } // for
  return Y_HYPHENATED_NAME;
}

/**
 * Finds a cdecl keyword matching \a s, if any.
 *
 * @param s The string to find.
 * @return Returns a pointer to the corresponding keyword or null if not found.
 */
static cdecl_keyword_t const* cdecl_keyword_find( char const *s ) {
  assert( s != NULL );
  for ( cdecl_keyword_t const *k = CDECL_KEYWORDS; k->literal; ++k ) {
    if ( strcmp( s, k->literal ) != 0 )
      continue;
    if ( c_mode == C_ENGLISH_TO_GIBBERISH ||
         (k->c_keyword_synonym && k->allow_explain) )
      return k;
    break;
  } // for
  return NULL;
}

/**
 * Gets a line of input for flex and keeps a copy for use later if printing an
 * error message.
 *
 * @param buf A pointer to the buffer to write into.
 * @param buf_capacity The capacity of \a buf.
 * @return Returns the number of bytes read.
 */
static size_t lexer_get_input( char *buf, size_t buf_capacity ) {
  assert( buf != NULL );
  size_t input_avail = input_len - input_sent;
  if ( input_avail == 0 ) {
    static size_t input_capacity;
    ssize_t bytes_read = getline( &input_buf, &input_capacity, yyin );
    if ( unlikely( bytes_read == -1 ) ) {
      FERROR( yyin );
      bytes_read = 0;
    }
    input_avail = input_len = (size_t)bytes_read;
    input_sent = 0;
  }
  //
  // Given that the default buffer capacity (YY_READ_BUF_SIZE) for flex is
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
 * Parses a scoped name, e.g. `a::b::c`.
 *
 * @param s The string to parse.
 * @param sname The `c_sname` to parse into.
 * @return Returns `true` only if the scoped name was successfully parsed.
 */
static bool parse_sname( char const *s, c_sname_t *sname ) {
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
static bool parse_sname_dtor( char const *s, c_sname_t *sname ) {
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
 * Standard lex function to know whether to continue parsing upon reaching EOF.
 *
 * @return Returns 1 if done or 0 to continue parsing (a new file set via
 * \c yyin).
 */
static int yywrap( void ) {
  return 1 /* done */;
}

////////// extern functions ///////////////////////////////////////////////////

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
  c_mode = C_ENGLISH_TO_GIBBERISH;
  input_len = input_sent = 0;
  lexer_find_typedef = true;
  newline();
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

cknr_name     ([cC][kK]&[rR])|([kK]&[rR][cC]?)
cpp_name      [cC]"++"({D}{D})?
dtor_sname    ({identifier}{S}*::{S}*)+(~{S}*|compl{S}+){identifier}
hyphenated    [a-z]+-([a-z]+-)*[a-z]+
int_suffix    ([lL][lL]?[uU]?)|([uU][lL]?[lL]?)
lang_name     {cknr_name}|{cpp_name}
sname         {identifier}({S}*::{S}*{identifier})+

bin_int       0[bB]{B}+{int_suffix}?
oct_int       0{O}*{int_suffix}?
dec_int       [1-9]{D}*{int_suffix}?
hex_int       0[xX]{H}+{int_suffix}?

%%

              /*
               * Special case: if the _Atomic keyword is immediately followed
               * by a '(', it is interpreted as a type specifier, not as a type
               * qualifier.
               */
_Atomic/{S}*\(  { SET_TOKEN; return Y_ATOMIC_SPEC; }

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
                if ( parse_sname_dtor( lexer_token, &sname ) ) {
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
                if ( parse_sname( lexer_token, &sname ) ) {
                  if ( lexer_find_typedef ) {
                    //
                    // 1. See if it's a typedef'd type.
                    //
                    c_typedef_t const *const t = c_typedef_find( &sname );
                    if ( t != NULL ) {
                      yylval.c_typedef = t;
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
                  if ( cdk->c_keyword_synonym == NULL )
                    return cdk->y_token;
                  if ( cdk->c_keyword_synonym == L__NORETURN &&
                       C_LANG_IS_CPP() ) {
                    //
                    // _Noreturn is a special case. In C11, it's "_Noreturn"
                    // and is a reserved word. In C++11, it's "noreturn", but
                    // is NOT a reserved word.
                    //
                    // When parsing C11, we also want to allow "noreturn" as a
                    // synonym (since that's provided via <stdnoreturn.h>).
                    //
                    // When parsing C++11, we want to recognize "noreturn" only
                    // when it's an attribute as [[noreturn]].
                    //
                    // When parsing English:
                    //
                    //  + We want to allow "_Noreturn" when we're translating
                    //    to C only and not C++.
                    //
                    //  + We want to allow either "noreturn" or "non-returning"
                    //    in all cases.
                    //
                    // However, when either parsing or translating to C++, we
                    // do NOT want to allow "noreturn" to be a synonym for
                    // "_Noreturn" since that's illegal in C++.  Hence, we do
                    // nothing.
                    //
                  }
                  else {
                    lexer_token = cdk->c_keyword_synonym;
                  }
                }

                if ( lexer_find_typedef ) {
                  //
                  // 2. See if it's a typedef'd type.
                  //
                  SLIST_TEMP_INIT_VAR( typedef_name, lexer_token, NULL );
                  c_typedef_t const *const t = c_typedef_find( &typedef_name );
                  if ( t != NULL ) {
                    yylval.c_typedef = t;
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
                  return ck->y_token;
                }

                //
                // 4. Otherwise, it's just an ordinary name.
                //
                yylval.name = check_strdup( lexer_token );
                return Y_NAME;
              }

{hyphenated}  {
                SET_TOKEN;
                char const *const c_literal = c_hyphenated_find( lexer_token );
                if ( c_literal != NULL ) {
                  ASSERT_MODE( ENGLISH_TO_GIBBERISH );
                  lexer_token = c_literal;
                  goto find_c_keyword;
                }
                int const y_token = cdecl_hyphenated_find( lexer_token );
                if ( y_token == Y_HYPHENATED_NAME )
                  yylval.name = check_strdup( lexer_token );
                return y_token;
              }

{lang_name}   {
                SET_TOKEN;
                return Y_LANG_NAME;
              }

{bin_int}     { SET_TOKEN; yylval.number = parse_int(  2 ); return Y_NUMBER; }
{oct_int}     { SET_TOKEN; yylval.number = parse_int(  8 ); return Y_NUMBER; }
{dec_int}     { SET_TOKEN; yylval.number = parse_int( 10 ); return Y_NUMBER; }
{hex_int}     { SET_TOKEN; yylval.number = parse_int( 16 ); return Y_NUMBER; }

"<:<:"        { SET_TOKEN_TO_AND_RETURN   ( "[[", Y_LBRACKET2 ); }
":>:>"        { SET_TOKEN_TO_AND_RETURN   ( "]]", Y_RBRACKET2 ); }

"<:"          { SET_TOKEN_TO_AND_RETURN_IT( "["               ); }
":>"          { SET_TOKEN_TO_AND_RETURN_IT( "]"               ); }

"??(??("      { SET_TOKEN_TO_AND_RETURN   ( "[[", Y_LBRACKET2 ); }
"??)??)"      { SET_TOKEN_TO_AND_RETURN   ( "]]", Y_RBRACKET2 ); }
"??'="        { SET_TOKEN_TO_AND_RETURN   ( "^=", Y_CIRC_EQ   ); }
"??!="        { SET_TOKEN_TO_AND_RETURN   ( "|=", Y_PIPE_EQ   ); }
"??!??!"      { SET_TOKEN_TO_AND_RETURN   ( "||", Y_PIPE2     ); }

"??!"         { SET_TOKEN_TO_AND_RETURN   ( "|", Y_PIPE       ); }
"??'"         { SET_TOKEN_TO_AND_RETURN   ( "^", Y_CIRC       ); }
"??("         { SET_TOKEN_TO_AND_RETURN_IT( "["               ); }
"??)"         { SET_TOKEN_TO_AND_RETURN_IT( "]"               ); }
"??-"         { SET_TOKEN_TO_AND_RETURN   ( "~", Y_TILDE      ); }

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
"="           |
">"           |
"["           |
"]"           |
"{"           |
"}"           { SET_TOKEN; return lexer_token[0]    ; }

"/*"          {                         /* ignore C-style comments */
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

"//".*        ;                         /* ignore C++-style comments */
#.*           ;                         /* ignore preprocessor directives */
{S}+          ;                         /* ignore all other whitespace */
\\\n          ;                         /* eat escaped newlines */

\n            {
                SET_TOKEN;
                newline();
                return Y_END;
              }

<<EOF>>       {
                SET_TOKEN;
                newline();
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

.             {
                SET_TOKEN;
                return Y_ERROR;
              }

%%

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Called by flex only when there's a fatal error.
 *
 * @param msg The error message to print.
 */
static void lexer_fatal( char const *msg ) {
  //
  // This is defined down here to avoid having to declare yy_fatal_error
  // ourselves and having to get it right being subject to possible changes in
  // its signature in different flex versions.
  //
  INTERNAL_ERR( "%s\n", msg );

  // Not reached -- "called" just to silence the "unused function" warning.
  yy_fatal_error( msg );
}

/**
 * Update the parser's location.
 * @note This is called by flex via `YY_USER_ACTION`.
 */
static void lexer_update_loc( void ) {
  yylloc.first_line   = yylloc.last_line = (int)yylineno;
  yylloc.first_column = column;
  yylloc.last_column  = column + (int)yyleng - 1;
  column += (int)yyleng;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
