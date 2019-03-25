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

/// @endcond

/**
 * Asserts that we're in \a MODE of operation.
 * If not, returns the error token.
 *
 * @param MODE The `c_mode` we're asserting.
 */
#define ASSERT_MODE(MODE) \
  BLOCK( if ( c_mode != MODE_ ## MODE ) return Y_ERROR; )

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
 * @hideinitializer
 */
#define SKIP_CHARS(S,CHARS)       ((S) += strspn( (S), (CHARS) ))

/**
 * Advances \a S over all whitespace.
 *
 * @param S The string pointer to advance.
 * @return Returns the updated \a S.
 * @hideinitializer
 */
#define SKIP_WS(S)                SKIP_CHARS( (S), " \f\r\t\v" )

/**
 * Overrides flex's fatal error message to print the message in our format and
 * also exit with the status code we want.
 *
 * @param MSG The error message to print.
 * @hideinitializer
 */
#define YY_FATAL_ERROR(MSG)       lexer_fatal( (MSG) )

/**
 * Overrides flex's input.
 *
 * @param BUF The buffer to use.
 * @param BYTES_READ Set to the number of bytes read.
 * @param BYTES_MAX The maximum number of bytes to read.
 * @hideinitializer
 */
#define YY_INPUT(BUF,BYTES_READ,BYTES_MAX) \
  (BYTES_READ) = lexer_get_input( (BUF), (BYTES_MAX) )

/**
 * This code is inserted by flex at the beginning of each rule to set the
 * current token location information.
 *
 * @hideinitializer
 */
#define YY_USER_ACTION BLOCK(                         \
  yylloc.first_line   = yylloc.last_line = yylineno;  \
  yylloc.first_column = column;                       \
  yylloc.last_column  = column + yyleng - 1;          \
  column += yyleng; );                  /* must include trailing ';' */

/**
 * This code must be manually inserted before a REJECT to undo YY_USER_ACTION.
 *
 * @hideinitializer
 */
#define YY_USER_ACTION_UNDO_FOR_REJECT \
  column -= (int)yyleng

///////////////////////////////////////////////////////////////////////////////

/**
 * cdecl keyword info.
 */
struct keyword {
  char const *literal;                  ///< C string literal of the keyword.
  int         y_token;                  ///< Bison token number.
  char const *c_keyword_synonym;        ///< Synonym of C/C++ keyword, if any.
  bool        allow_explain;            ///< Allow when explaining C/C++?
};
typedef struct keyword keyword_t;

/// @cond DOXYGEN_IGNORE
#define C_SYN(C_KEYWORD,ALLOW)  0, C_KEYWORD, ALLOW
#define TOKEN(T)                T, NULL,      false
/// @endcond

/**
 * All cdecl keywords (that are not C/C++ keywords).
 *
 * @hideinitializer
 */
static keyword_t const CDECL_KEYWORDS[] = {
  { L_ALL,            TOKEN( Y_ALL )                  },
  { L_ARRAY,          TOKEN( Y_ARRAY )                },
  { L_AS,             TOKEN( Y_AS )                   },
  { L_ATOMIC,         C_SYN( L__ATOMIC,       false ) },
  { L_AUTOMATIC,      C_SYN( L_AUTO,          false ) },
  { L_BLOCK,          TOKEN( Y_BLOCK )                },  // Apple extension
  { L__BOOL,          C_SYN( L_BOOL,          true  ) },
  { L_CAST,           TOKEN( Y_CAST )                 },
  { L_CHARACTER,      C_SYN( L_CHAR,          false ) },
  { L_COMPLEX,        C_SYN( L__COMPLEX,      true  ) },
  { L_CONSTANT,       C_SYN( L_CONST,         false ) },
  { L_DECLARE,        TOKEN( Y_DECLARE )              },
  { L_DEFINE,         TOKEN( Y_DEFINE )               },
  { L_DYNAMIC,        TOKEN( Y_DYNAMIC )              },
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
  { L_MBR,            TOKEN( Y_MEMBER ),              },
  { L_MEMBER,         TOKEN( Y_MEMBER ),              },
  { L_NON_MBR,        TOKEN( Y_NON_MEMBER ),          },
  { L_NON_MEMBER,     TOKEN( Y_NON_MEMBER ),          },
  { L_NORETURN,       C_SYN( L__NORETURN,     true  ) },
  { L_OF,             TOKEN( Y_OF ),                  },
  { L_OPER,           TOKEN( Y_OPERATOR )             },
  { L_OVERRIDDEN,     C_SYN( L_OVERRIDE,      false ) },
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
  { L_SET,            TOKEN( Y_SET )                  },
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
 * Hyphenated English keyword-to-literal mapping.
 */
struct hyphenated {
  char const *token;                    ///< Hyphenated English literal.
  char const *literal;                  ///< C language literal it maps to.
};
typedef struct hyphenated hyphenated_t;

/**
 * All hyphenated English keywords that are synonyms for C/C++ keywords.  These
 * are handled seperately because hyphenated words are not valid identifiers
 * (since `-` is not valid in an identifier).
 *
 * @hideinitializer
 */
static hyphenated_t const HYPHENATED_KEYWORDS[] = {
  { "carries-dependency", L_CARRIES_DEPENDENCY  },
  { "maybe-unused",       L_MAYBE_UNUSED        },
  { "no-exception",       L_NOEXCEPT            },
  { "non-discardable",    L_NODISCARD           },
  { "non-returning",      L__NORETURN           },
  { "non-throwing",       L_THROW               },
};

// extern variables
static char const lexer_token_init_buf; ///< So lexer_token is never null.
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

////////// local functions ////////////////////////////////////////////////////

/**
 * Finds a cdecl keyword matching \a s, if any.
 *
 * @param s The string to find.
 * @return Returns a pointer to the corresponding keyword or null if not found.
 */
static keyword_t const* cdecl_keyword_find( char const *s ) {
  assert( s != NULL );
  for ( keyword_t const *k = CDECL_KEYWORDS; k->literal; ++k ) {
    if ( strcmp( s, k->literal ) != 0 )
      continue;
    if ( c_mode == MODE_ENGLISH_TO_GIBBERISH ||
         (k->c_keyword_synonym && k->allow_explain) )
      return k;
    break;
  } // for
  return NULL;
}

/**
 * Finds a hyphenated English literal that is a synonym for a C/C++ language
 * literal.
 *
 * @param s The string to find.
 * @return Returns the corresponding C/C++ language literal or null if none.
 */
static char const* hyphenated_find( char const *s ) {
  assert( s != NULL );
  char const *literal = NULL;
  for ( hyphenated_t const *h = HYPHENATED_KEYWORDS; h->token; ++h ) {
    if ( strcmp( s, h->token ) == 0 ) {
      literal = h->literal;
      if ( literal == L__NORETURN && C_LANG_IS_CPP() ) {
        //
        // _Noreturn is a special case. In C11, it's "_Noreturn"; in C++11,
        // it's "noreturn".
        //
        // Depending on the current language, we map "non-returning" either to
        // "_Noreturn" or "noreturn".
        //
        literal = L_NORETURN;
      }
      break;
    }
  } // for
  return literal;
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
    if ( bytes_read == -1 ) {
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
  while ( *++s != '\0' && (isalnum( *s ) || *s == '_') )
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
 * @return Returns the scoped name.
 */
static c_sname_t parse_sname( char const *s ) {
  assert( s != NULL );
  c_sname_t rv;
  c_sname_init( &rv );

  for ( char const *end; parse_identifier( s, &end ); ) {
    c_sname_append_name( &rv, strndup( s, end - s ) );
    SKIP_WS( end );
    if ( strncmp( end, "::", 2 ) != 0 )
      break;
    s = end + 2;
    SKIP_WS( s );
  } // for

  return rv;
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
    *pcolumn = column - strlen( lexer_token );
}

void lexer_reset( bool hard_reset ) {
  if ( hard_reset ) {
    is_eof = false;
    yylineno = 0;
  }
  c_mode = MODE_ENGLISH_TO_GIBBERISH;
  input_len = input_sent = 0;
  newline();
}

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

%}

letter        [A-Za-z_]
az            [a-z]
B             [01]
O             [0-7]
D             [0-9]
H             [0-9A-Fa-f]
S             [ \f\r\t\v]

cknr_name     ([cC][kK]&[rR])|([kK]&[rR][cC]?)
cpp_name      [cC]"++"({D}{D})?
hyphenated    {az}+-{az}+
identifier    {letter}({letter}|{D})*
int_suffix    ([lL][lL]?[uU]?)|([uU][lL]?[lL]?)
lang_name     {cknr_name}|{cpp_name}
scoped_name   {identifier}({S}*::{S}*{identifier})+

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

{scoped_name} {
                SET_TOKEN;
                c_sname_t temp_name = parse_sname( lexer_token );
                c_typedef_t const *const t = c_typedef_find( &temp_name );
                c_sname_free( &temp_name );
                if ( t != NULL ) {
                  yylval.c_typedef = t;
                  return Y_TYPEDEF_TYPE;
                }
                YY_USER_ACTION_UNDO_FOR_REJECT;
                REJECT;
              }

{identifier}  {
                SET_TOKEN;

                //
                // 1. See if it's a cdecl keyword.
                //
                keyword_t const *const ck = cdecl_keyword_find( lexer_token );
                if ( ck != NULL ) {
                  if ( ck->c_keyword_synonym == NULL )
                    return ck->y_token;
                  if ( ck->c_keyword_synonym == L__NORETURN &&
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
                    lexer_token = ck->c_keyword_synonym;
                  }
                }

                //
                // 2. See if it's a typedef'd type.
                //
                SLIST_TEMP_INIT_VAR( typedef_name, lexer_token );
                c_typedef_t const *const t = c_typedef_find( &typedef_name );
                if ( t != NULL ) {
                  yylval.c_typedef = t;
                  return Y_TYPEDEF_TYPE;
                }

                //
                // 3. See if it's a C/C++ keyword.
                //
        find_c_keyword:
                NO_OP;
                c_keyword_t const *k = c_keyword_find( lexer_token, opt_lang );
                if ( k == NULL && c_mode == MODE_ENGLISH_TO_GIBBERISH ) {
                  //
                  // We have to look-up attribute names when parsing English
                  // because there can't be any special syntax for them, e.g.:
                  //
                  //      declare x as deprecated int
                  //
                  k = c_attribute_find( lexer_token );
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
                if ( k != NULL ) {
                  yylval.type_id = k->type_id;
                  return k->y_token;
                }

                //
                // 4. Otherwise, it's just an ordinary name.
                //
                yylval.name = check_strdup( lexer_token );
                return Y_NAME;
              }

              /*
               * Special case for hyphenated cdecl keywords that are NOT
               * synonyms for C/C++ keywords.  (Since there are currently only
               * two of these, just enumerate them here inline rather than have
               * a look-up table.)
               */
"non-member"  |
"non-mbr"     {
                ASSERT_MODE( ENGLISH_TO_GIBBERISH );
                SET_TOKEN;
                return Y_NON_MEMBER;
              }

{hyphenated}  {
                ASSERT_MODE( ENGLISH_TO_GIBBERISH );
                SET_TOKEN;
                char const *const literal = hyphenated_find( yytext );
                if ( literal == NULL )
                  return Y_ERROR;
                lexer_token = literal;
                goto find_c_keyword;
              }

{lang_name}   {
                SET_TOKEN;
                yylval.name = check_strdup( lexer_token );
                return Y_LANG_NAME;
              }

{bin_int}     { SET_TOKEN; yylval.number = parse_int(  2 ); return Y_NUMBER; }
{oct_int}     { SET_TOKEN; yylval.number = parse_int(  8 ); return Y_NUMBER; }
{dec_int}     { SET_TOKEN; yylval.number = parse_int( 10 ); return Y_NUMBER; }
{hex_int}     { SET_TOKEN; yylval.number = parse_int( 16 ); return Y_NUMBER; }

"<:<:"        { SET_TOKEN_TO_AND_RETURN( "[[", Y_LBRACKET2 ); }
":>:>"        { SET_TOKEN_TO_AND_RETURN( "]]", Y_RBRACKET2 ); }

"<:"          { SET_TOKEN_TO_AND_RETURN_IT( "[" ); }
":>"          { SET_TOKEN_TO_AND_RETURN_IT( "]" ); }

"??(??("      { SET_TOKEN_TO_AND_RETURN( "[[", Y_LBRACKET2 ); }
"??)??)"      { SET_TOKEN_TO_AND_RETURN( "]]", Y_RBRACKET2 ); }
"??'="        { SET_TOKEN_TO_AND_RETURN( "^=", Y_CIRC_EQ   ); }
"??!="        { SET_TOKEN_TO_AND_RETURN( "|=", Y_PIPE_EQ   ); }
"??!??!"      { SET_TOKEN_TO_AND_RETURN( "||", Y_PIPE2     ); }

"??!"         { SET_TOKEN_TO_AND_RETURN_IT( "|" ); }
"??'"         { SET_TOKEN_TO_AND_RETURN_IT( "^" ); }
"??("         { SET_TOKEN_TO_AND_RETURN_IT( "[" ); }
"??)"         { SET_TOKEN_TO_AND_RETURN_IT( "]" ); }
"??-"         { SET_TOKEN_TO_AND_RETURN_IT( "~" ); }

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

"!="          { SET_TOKEN; return Y_NOT_EQ          ; }
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

"!"           |
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
";"           |
"<"           |
"="           |
">"           |
"["           |
"]"           |
"^"           |
"{"           |
"|"           |
"}"           |
"~"           { SET_TOKEN; return lexer_token[0]; }

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

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
