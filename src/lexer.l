/*
**      cdecl -- C gibberish translator
**      src/lexer.l
*/

%option nounput

%{

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "keywords.h"
#include "literals.h"
#include "util.h"
#include "parser.h"                     /* must go last */

// standard
#include <string.h>

#define SET_MY_TEXT   my_text = yytext

/**
 * This code is inserted by flex at the beginning of each rule.
 * It sets the current token location information.
 * @hideinitializer
 */
#define YY_USER_ACTION                        \
  yylloc.first_column = my_col;               \
  yylloc.last_column  = my_col + yyleng - 1;  \
  my_col += yyleng;

///////////////////////////////////////////////////////////////////////////////

/**
 * cdecl keyword info.
 */
struct keyword {
  char const *literal;                  // C string literal of the keyword
  int         y_token;                  // yacc token number
  char const *c_keyword_synonym;        // synonym of keyword, if any
};
typedef struct keyword keyword_t;

#define C_SYN(C_KEYWORD)  0, C_KEYWORD
#define TOKEN(T)          T, NULL

/**
 * All cdecl keywords (that are not C/C++ keywords).
 */
static keyword_t const CDECL_KEYWORDS[] = {
  { L_ARRAY,        TOKEN( Y_ARRAY )      },
  { L_AS,           TOKEN( Y_AS )         },
  { L_BLOCK,        TOKEN( Y_BLOCK )      },  // Apple extension
  { L__BOOL,        C_SYN( L_BOOL )       },
  { L_CAST,         TOKEN( Y_CAST )       },
  { L_CHARACTER,    C_SYN( L_CHAR )       },
  { L__COMPLEX,     C_SYN( L_COMPLEX )    },
  { L_CONSTANT,     C_SYN( L_CONST )      },
  { L_DECLARE,      TOKEN( Y_DECLARE )    },
  { L_ENUMERATION,  C_SYN( L_ENUM )       },
  { L_EXIT,         TOKEN( Y_QUIT )       },
  { L_EXPLAIN,      TOKEN( Y_EXPLAIN )    },
  { L_FUNC,         TOKEN( Y_FUNCTION )   },
  { L_FUNCTION,     TOKEN( Y_FUNCTION )   },
  { L_HELP,         TOKEN( Y_HELP )       },
  { L_INTEGER,      C_SYN( L_INT )        },
  { L_INTO,         TOKEN( Y_INTO )       },
  { L_MBR,          TOKEN( Y_MEMBER ),    },
  { L_MEMBER,       TOKEN( Y_MEMBER ),    },
  { L_OF,           TOKEN( Y_OF ),        },
  { L_POINTER,      TOKEN( Y_POINTER )    },
  { L_PTR,          TOKEN( Y_POINTER )    },
  { L_Q,            TOKEN( Y_QUIT )       },
  { L_QUIT,         TOKEN( Y_QUIT )       },
  { L_REF,          TOKEN( Y_REFERENCE )  },
  { L_REFERENCE,    TOKEN( Y_REFERENCE )  },
  { L_RESTRICTED,   C_SYN( L_RESTRICT )   },
  { L_RET,          TOKEN( Y_RETURNING )  },
  { L_RETURNING,    TOKEN( Y_RETURNING )  },
  { L_RVALUE,       TOKEN( Y_RVALUE )     },
  { L_SET,          TOKEN( Y_SET )        },
  { L_STRUCTURE,    C_SYN( L_STRUCT )     },
  { L_TO,           TOKEN( Y_TO )         },
  { L_TYPE,         C_SYN( L_TYPEDEF )    },
  { L_VARARGS,      C_SYN( L_ELLIPSIS )   },
  { L_VARIADIC,     C_SYN( L_ELLIPSIS )   },
  { L_VECTOR,       TOKEN( Y_ARRAY )      },
  { NULL,           TOKEN( 0 )            }
};

// extern variables
bool        explaining;                 // parsing "explain ..."?
size_t      my_col;                     // column position of current token
size_t      my_col_newline;             // column position of newline
char const *my_text;                    // guranteed pointer of yytext
YYLTYPE     yylloc;                     // location of current token

// local variables
static bool my_eof;

////////// local functions ////////////////////////////////////////////////////

/**
 * Finds the keyword matching \a s, if any.
 *
 * @param s The string to find.
 * @return Returns a pointer to the corresponding keyword or null if not found.
 */
static keyword_t const* keyword_find( char const *s ) {
  for ( keyword_t const *k = CDECL_KEYWORDS; k->literal; ++k ) {
    if ( strcmp( s, k->literal ) == 0 )
      return k;
  } // for
  return NULL;
}

/**
 * Do what's necessary after a newline character.
 */
static void newline( void ) {
  my_col_newline = my_col;              // my_col is the column of the newline
  my_col = 0;
}

/**
 * Parses an integer from \c my_text.
 *
 * @param base The integer base to use.
 * @return Returns the integer value.
 */
static inline int parse_int( int base ) {
  //
  // We don't need to check for errors from strtol(3) here since the lexer
  // guarantees that we'll be passed only a valid number token.
  //
  return (int)strtol( my_text, NULL, base );
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

/**
 * Resets the lexer to its initial state.
 *
 * @param hard_reset If \c true, does a "hard" reset that currently resets the
 * EOF flag also.
 */
void lexer_reset( bool hard_reset ) {
  if ( hard_reset )
    my_eof = false;
  explaining = false;
  newline();
}

///////////////////////////////////////////////////////////////////////////////

%}

A             [A-Za-z_]
O             [0-7]
D             [0-9]
H             [0-9A-Fa-f]

%%

{A}({A}|{D})* {
                SET_MY_TEXT;

                if ( !explaining ) {
                  keyword_t const *const k = keyword_find( my_text );
                  if ( k ) {
                    if ( k->c_keyword_synonym )
                      my_text = k->c_keyword_synonym;
                    else
                      return k->y_token;
                  }
                }

                c_keyword_t const *const k = c_keyword_find_literal( my_text );
                if ( k ) {
                  yylval.type = k->type;
                  return k->y_token;
                }

                yylval.name = check_strdup( my_text );
                return Y_NAME;
              }

"c++"         |
"c++98"       |
"c++03"       |
"c++11"       {
                SET_MY_TEXT;
                yylval.name = check_strdup( my_text );
                return Y_CPP_LANG_NAME;
              }

[1-9]{D}*     { SET_MY_TEXT; yylval.number = parse_int( 10 ); return Y_NUMBER; }
0{O}*         { SET_MY_TEXT; yylval.number = parse_int(  8 ); return Y_NUMBER; }
0[xX]{H}+     { SET_MY_TEXT; yylval.number = parse_int( 16 ); return Y_NUMBER; }

"&&"          return Y_RVALUE_REFERENCE;
"..."         return Y_ELLIPSIS;
"::"          return Y_COLON_COLON;
"?"           return Y_HELP;

"^"           |                         /* Apple extension: block indicator */
"&"           |
"("           |
")"           |
"*"           |
","           |
"["           |
"]"           { SET_MY_TEXT; return *my_text; }
";"           { SET_MY_TEXT; return Y_END; }

"/*"          {                         /* ignore C-style comments */
                for ( int c = input(), prev_c = '\0'; c != EOF;
                      prev_c = c, c = input() ) {
                  ++my_col;
                  switch ( c ) {
                    case '\n':
                      newline();
                      break;
                    case '/':
                      if ( prev_c == '*' )
                        goto done;
                      break;
                  } // switch
                } // for
              done:
                NO_OP;
              }

"//".*        ;                         /* ignore C++-style comments */
#.*           ;                         /* ignore preprocessor directives */
[ \f\r\t\v]+  ;                         /* ignore all other whitespace */
\\\n          ;                         /* eat escaped newlines */

\n            {
                SET_MY_TEXT;
                newline();
                return Y_END;
              }

<<EOF>>       {
                SET_MY_TEXT;
                newline();
                return (my_eof = !my_eof) ? Y_END : YY_NULL;
              }

.             {
                SET_MY_TEXT;
                return Y_ERROR;
              }

%%

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
