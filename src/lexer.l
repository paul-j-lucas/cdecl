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

/**
 * This code is inserted by flex at the beginning of each rule.
 * It sets the current token location information.
 * @hideinitializer
 */
#define YY_USER_ACTION                      \
  yylloc.first_column = y_col;              \
  yylloc.last_column  = y_col + yyleng - 1; \
  y_col += yyleng;

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
size_t      y_col;                      // column position of current token
size_t      y_col_newline;              // column position of newline
YYLTYPE     yylloc;                     // location of current token

// local variables
static bool eof;

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
 * Resets our notion of the current location to the beginning of the line.
 */
static void loc_reset( void ) {
  y_col_newline = y_col - 1 /* '\n' */;
  y_col = 0;
}

/**
 * Parses an integer from \c yytext.
 *
 * @param base The integer base to use.
 * @return Returns the integer value.
 */
static inline int parse_int( int base ) {
  //
  // We don't need to check for errors from strtol(3) here since the lexer
  // guarantees that we'll be passed only a valid number token.
  //
  return (int)strtol( yytext, NULL, base );
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
 */
void lexer_reset( void ) {
  explaining = false;
  loc_reset();
}

///////////////////////////////////////////////////////////////////////////////

%}

A             [A-Za-z_]
O             [0-7]
D             [0-9]
H             [0-9A-Fa-f]

%%

{A}({A}|{D})* {
                char const *name = yytext;

                if ( !explaining ) {
                  keyword_t const *const k = keyword_find( name );
                  if ( k ) {
                    if ( k->c_keyword_synonym )
                      name = k->c_keyword_synonym;
                    else
                      return k->y_token;
                  }
                }

                c_keyword_t const *const k = c_keyword_find_literal( name );
                if ( k ) {
                  yylval.type = k->type;
                  return k->y_token;
                }

                yylval.name = check_strdup( name );
                return Y_NAME;
              }

[1-9]{D}*     { yylval.number = parse_int( 10 ); return Y_NUMBER; }
0{O}*         { yylval.number = parse_int(  8 ); return Y_NUMBER; }
0[xX]{H}+     { yylval.number = parse_int( 16 ); return Y_NUMBER; }

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
"]"           return *yytext;

"/*"          {                         /* ignore C-style comments */
                for ( int c = input(), prev_c = '\0'; c != EOF;
                      prev_c = c, c = input() ) {
                  ++y_col;
                  switch ( c ) {
                    case '\n':
                      loc_reset();
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

[;\n]         {
                loc_reset();
                return Y_END;
              }

<<EOF>>       {
                loc_reset();
                return (eof = !eof) ? Y_END : YY_NULL;
              }

.             return Y_ERROR;

%%

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
