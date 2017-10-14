/*
**      cdecl -- C gibberish translator
**      src/lexer.l
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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

%option nounput
%option yylineno

%{

// local
#include "config.h"                     /* must go first */
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_typedef.h"
#include "lexer.h"
#include "literals.h"
#include "misc.h"
#include "util.h"
#include "parser.h"                     /* must go last */

// standard
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>                     /* for strtol(3) */
#include <string.h>

#define ASSERT_MODE(MODE) \
  BLOCK( if ( c_mode != MODE_ ## MODE ) return Y_ERROR; )

#define SET_TOKEN                 lexer_token = yytext

/**
 * Overrides flex's fatal error message to print the message in our format and
 * also exit with the status we want.
 * @hideinitializer
 */
#define YY_FATAL_ERROR(MSG)       lexer_fatal( (MSG) )

/**
 * Overrides flex's input.
 * @hideinitializer
 */
#define YY_INPUT(BUF,RESULT,MAX_SIZE) \
  (RESULT) = lexer_get_input( (BUF), (MAX_SIZE) )

/**
 * This code is inserted by flex at the beginning of each rule.
 * It sets the current token location information.
 * @hideinitializer
 */
#define YY_USER_ACTION                                \
  yylloc.first_line   = yylloc.last_line = yylineno;  \
  yylloc.first_column = column;                       \
  yylloc.last_column  = column + yyleng - 1;          \
  column += yyleng;

///////////////////////////////////////////////////////////////////////////////

/**
 * cdecl keyword info.
 */
struct keyword {
  char const *literal;                  // C string literal of the keyword
  int         y_token;                  // yacc token number
  char const *c_keyword_synonym;        // synonym of C/C++ keyword, if any
  bool        allow_explain;            // allow when explaining C/C++?
};
typedef struct keyword keyword_t;

#define C_SYN(C_KEYWORD,ALLOW)  0, C_KEYWORD, ALLOW
#define TOKEN(T)                T, NULL,      false

/**
 * All cdecl keywords (that are not C/C++ keywords).
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
  { L__COMPLEX,       C_SYN( L_COMPLEX,       true  ) },
  { L_CONSTANT,       C_SYN( L_CONST,         false ) },
  { L_DECLARE,        TOKEN( Y_DECLARE )              },
  { L_DEFINE,         TOKEN( Y_DEFINE )               },
  { L_DYNAMIC,        TOKEN( Y_DYNAMIC )              },
  { L_ENUMERATION,    C_SYN( L_ENUM,          false ) },
  { L_EXIT,           TOKEN( Y_QUIT )                 },
  { L_EXPLAIN,        TOKEN( Y_EXPLAIN )              },
  { L_FUNC,           TOKEN( Y_FUNCTION )             },
  { L_FUNCTION,       TOKEN( Y_FUNCTION )             },
  { L_HELP,           TOKEN( Y_HELP )                 },
  { L_INTEGER,        C_SYN( L_INT,           false ) },
  { L_INTO,           TOKEN( Y_INTO )                 },
  { L_LEN,            TOKEN( Y_LENGTH )               },
  { L_LENGTH,         TOKEN( Y_LENGTH )               },
  { L_MBR,            TOKEN( Y_MEMBER ),              },
  { L_MEMBER,         TOKEN( Y_MEMBER ),              },
  { L__NORETURN,      C_SYN( L_NORETURN,      true  ) },
  { L_OF,             TOKEN( Y_OF ),                  },
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
  { L_SET,            TOKEN( Y_SET )                  },
  { L_SHOW,           TOKEN( Y_SHOW )                 },
  { L_STRUCTURE,      C_SYN( L_STRUCT,        false ) },
  { L__THREAD_LOCAL,  C_SYN( L_THREAD_LOCAL,  true  ) },
  { L_TO,             TOKEN( Y_TO )                   },
  { L_TYPE,           C_SYN( L_TYPEDEF,       false ) },
  { L_TYPES,          TOKEN( Y_TYPES )                },
  { L_USER,           TOKEN( Y_USER )                 },
  { L_VAR,            TOKEN( Y_VARIABLE )             },
  { L_VARARGS,        C_SYN( L_ELLIPSIS,      false ) },
  { L_VARIABLE,       TOKEN( Y_VARIABLE )             },
  { L_VARIADIC,       C_SYN( L_ELLIPSIS,      false ) },
  { L_VECTOR,         TOKEN( Y_ARRAY )                },
  { NULL,             TOKEN( 0 )                      }
};

// extern variables
char const     *lexer_token;            // guaranteed pointer of yytext

// local variables
static int      column;                 // column position of current token
static int      column_newline;         // column position of newline
static char    *input_buf;              // entire current input line
static size_t   input_len;              // length of line in input_buf
static size_t   input_sent;             // how many chars returned to flex
static bool     is_eof;                 // encountered EOF?

// local functions
static void     lexer_fatal( char const* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Finds a cdecl keyword matching \a s, if any.
 *
 * @param s The string to find.
 * @return Returns a pointer to the corresponding keyword or null if not found.
 */
static keyword_t const* cdecl_keyword_find( char const *s ) {
  for ( keyword_t const *k = CDECL_KEYWORDS; k->literal; ++k ) {
    if ( strcmp( s, k->literal ) != 0 )
      continue;
    if ( c_mode == MODE_ENGLISH || (k->c_keyword_synonym && k->allow_explain) )
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
 * @param buf_capacity The size of \a buf.
 * @return Returns the number of bytes read.
 */
static size_t lexer_get_input( char *buf, size_t buf_capacity ) {
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
 * Parses an integer from \c lexer_token.
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
  if ( plen )
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
  c_mode = MODE_ENGLISH;
  input_len = input_sent = 0;
  newline();
}

///////////////////////////////////////////////////////////////////////////////

%}

A             [A-Za-z_]
B             [01]
O             [0-7]
D             [0-9]
H             [0-9A-Fa-f]
S             [ \f\r\t\v]

cknr_name     ([cC][kK]&[rR])|([kK]&[rR][cC]?)
cxx_name      [cC]"++"({D}{D})?
identifier    {A}({A}|{D})*
int_suffix    ([lL][lL]?[uU]?)|([uU][lL]?[lL]?)
lang_name     {cknr_name}|{cxx_name}

binary        0[bB]{B}+{int_suffix}?
octal         0{O}*{int_suffix}?
decimal       [1-9]{D}*{int_suffix}?
hexadecimal   0[xX]{H}+{int_suffix}?

%%

              /*
               * Special case: If the _Atomic keyword is immediately followed
               * by a left parenthesis, it is interpreted as a type specifier
               * ..., not as a type qualifier.
               */
_Atomic/{S}*\(  { SET_TOKEN; return Y_ATOMIC_SPEC; }

{identifier}* {
                SET_TOKEN;

                keyword_t const *const ck = cdecl_keyword_find( lexer_token );
                if ( ck ) {
                  if ( ck->c_keyword_synonym )
                    lexer_token = ck->c_keyword_synonym;
                  else
                    return ck->y_token;
                }

                c_typedef_t const *const t = c_typedef_find( lexer_token );
                if ( t ) {
                  yylval.c_typedef = t;
                  return Y_TYPEDEF_TYPE;
                }

        find_c_keyword:
                NO_OP;
                c_keyword_t const *const k = c_keyword_find( lexer_token );
                if ( k ) {
                  yylval.type = k->type;
                  return k->y_token;
                }

                yylval.name = check_strdup( lexer_token );
                return Y_NAME;
              }

{lang_name}   {
                SET_TOKEN;
                yylval.name = check_strdup( lexer_token );
                return Y_LANG_NAME;
              }

no-except     {
                SET_TOKEN;
                ASSERT_MODE( ENGLISH );
                lexer_token = L_NOEXCEPT;
                goto find_c_keyword;
              }

non-returning {
                SET_TOKEN;
                ASSERT_MODE( ENGLISH );
                lexer_token = L_NORETURN;
                goto find_c_keyword;
              }

non-throwing  {
                SET_TOKEN;
                ASSERT_MODE( ENGLISH );
                lexer_token = L_THROW;
                goto find_c_keyword;
              }

{binary}      { SET_TOKEN; yylval.number = parse_int(  2 ); return Y_NUMBER; }
{octal}       { SET_TOKEN; yylval.number = parse_int(  8 ); return Y_NUMBER; }
{decimal}     { SET_TOKEN; yylval.number = parse_int( 10 ); return Y_NUMBER; }
{hexadecimal} { SET_TOKEN; yylval.number = parse_int( 16 ); return Y_NUMBER; }

"&&"          { SET_TOKEN; return Y_DOUBLE_AMPERSAND; }
"->"          { SET_TOKEN; return Y_ARROW; }
"..."         { SET_TOKEN; return Y_ELLIPSIS; }
"::"          { SET_TOKEN; return Y_COLON_COLON; }
"?"           { SET_TOKEN; return Y_HELP; }

"^"           |                         /* Apple extension: block indicator */
"&"           |
"("           |
")"           |
"*"           |
","           |
"<"           |
"="           |
">"           |
"["           |
"]"           { SET_TOKEN; return lexer_token[0]; }
";"           { SET_TOKEN; return Y_END; }

"/*"          {                         /* ignore C-style comments */
                for ( int c = input(), prev_c = '\0'; c != EOF;
                      prev_c = c, c = input() ) {
                  ++column;
                  switch ( c ) {
                    case '\n':
                      newline();
                      break;
                    case '/':
                      if ( prev_c == '*' )
                        goto c_comment_done;
                      break;
                  } // switch
                } // for

        c_comment_done:
                NO_OP;
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
///////////////////////////////////////////////////////////////////////////////

/**
 * Called by flex only when there's a fatal error.
 *
 * @param msg The error message to print.
 *
 * @note This is defined down here to avoid having to declare yy_fatal_error
 * ourselves and having to get it right being subject to possible changes in
 * its signature in different flex versions.
 */
static void lexer_fatal( char const *msg ) {
  INTERNAL_ERR( "%s\n", msg );

  // Not reached -- "called" just to silence the "unused function" warning.
  yy_fatal_error( msg );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
