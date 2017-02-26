/*
**      cdecl -- C gibberish translator
**      src/cdlex.l
*/

%option nounput

%{

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "color.h"
#include "common.h"
#include "keywords.h"
#include "lang.h"
#include "literals.h"
#include "util.h"
#include "cdgram.h"                     /* must go last */

// standard
#include <string.h>

#define RETURN(TOKEN) \
  BLOCK( y_token_col += yyleng; return (TOKEN); )

///////////////////////////////////////////////////////////////////////////////

/**
 * cdecl keyword info.
 */
struct keyword {
  char const *literal;                  // C string literal of the keyword
  int         y_token;                  // yacc token number
  char const *c_keyword_synonym;        // English synonym of keyword, if any
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
  { L_SET,          TOKEN( Y_SET )        },
  { L_STRUCTURE,    C_SYN( L_STRUCT )     },
  { L_TO,           TOKEN( Y_TO )         },
  { L_VECTOR,       TOKEN( Y_ARRAY )      },
  { NULL,           TOKEN( 0 )            }
};

// local variables
static size_t  y_token_col;             // column position of current token

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints a '^' (in color, if possible and requested) under the offending
 * token.
 */
void print_caret( void ) {
  size_t const n = strlen( prompt ) + y_token_col - strlen( yytext );
  PRINT_ERR( "%*s", (int)n, "" );
  SGR_START_COLOR( caret );
  FPUTC( '^', stderr );
  SGR_END_COLOR();
  FPUTC( '\n', stderr );
}

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
 * Parses a number from \c yytext and sets \a yylval.number.
 *
 * @param base The number base to use.
 */
static inline void parse_number( int base ) {
  //
  // We don't need to check for errors from strtol(3) here since the lexer
  // guarantees that we'll be passed only a valid number token.
  //
  yylval.number = (int)strtol( yytext, NULL, base );
}

///////////////////////////////////////////////////////////////////////////////

%}

A             [A-Za-z_]
O             [0-7]
D             [0-9]
H             [0-9A-Fa-f]

%%

{A}({A}|{D})* {
                // use our own pointer in case yytext[]
                char const *name = yytext;

                keyword_t const *const k = keyword_find( name );
                if ( k ) {
                  if ( k->c_keyword_synonym )
                    name = (char*)k->c_keyword_synonym;
                  else
                    RETURN( k->y_token );
                }

                c_keyword_t const *const ck = c_keyword_find_literal( name );
                if ( ck ) {
                  yylval.type = ck->type;
                  RETURN( ck->y_token );
                }

                yylval.name = check_strdup( name );
                RETURN( Y_NAME );
              }

[1-9]{D}*     { parse_number( 10 ); RETURN( Y_NUMBER ); }
0{O}*         { parse_number(  8 ); RETURN( Y_NUMBER ); }
0[xX]{H}+     { parse_number( 16 ); RETURN( Y_NUMBER ); }

"::"          RETURN( Y_COLON_COLON );

"?"           RETURN( Y_HELP );

"^"           |                         /* Apple extension: block indicator */
"&"           |
"("           |
")"           |
"*"           |
","           |
"["           |
"]"           RETURN( *yytext );        /* for blocks (Apple extension) */

"/*"          {                         /* ignore C-style comments */
                y_token_col += yyleng;
                for ( int c = input(), prev_c = '\0'; c != EOF;
                      prev_c = c, c = input() ) {
                  ++y_token_col;
                  switch ( c ) {
                    case '\n':
                      y_token_col = 0;
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
#.*           ;                         /* ignore script-style comments */
[ \f\r\t\v]+  y_token_col += yyleng;    /* ignore all other whitespace */
\n            { y_token_col = 0; return Y_END; }
";"           RETURN( Y_END );
.             RETURN( Y_ERROR );

%%

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
