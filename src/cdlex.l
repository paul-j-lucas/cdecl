/*
**      cdecl -- C gibberish translator
**      src/cdlex.l
*/

%option nounput

%{

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "keywords.h"
#include "lang.h"
#include "literals.h"
#include "cdgram.h"                     /* must go last */

// standard
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * cdecl keyword info.
 */
struct keyword {
  char const *literal;                  // C string literal of the keyword
  int         y_token;                  // yacc token number
  char const *synonym_of_c;             // English synonym of C keyword
};
typedef struct keyword keyword_t;

#define C_SYN(C_KEYWORD)  0, C_KEYWORD
#define TOKEN(T)          T, NULL

/**
 * All cdecl keywords.
 */
static keyword_t const CDECL_KEYWORDS[] = {
  { "?",            TOKEN( Y_HELP )       },
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
  { L_RET,          TOKEN( Y_RETURNING )  },
  { L_RETURNING,    TOKEN( Y_RETURNING )  },
  { L_SET,          TOKEN( Y_SET )        },
  { L_STRUCTURE,    C_SYN( L_STRUCT )     },
  { L_TO,           TOKEN( Y_TO )         },
  { L_VECTOR,       TOKEN( Y_ARRAY )      },
  { NULL,           TOKEN( 0 )            }
};

// extern functions
extern int  yywrap( void );

///////////////////////////////////////////////////////////////////////////////

%}

A             [A-Za-z_]
O             [0-7]
D             [0-9]
H             [0-9A-Fa-f]

%%

{A}({A}|{D})* {
                for ( keyword_t const *k = CDECL_KEYWORDS; k->literal; ++k ) {
                  if ( strcmp( yytext, k->literal ) == 0 ) {
                    if ( k->synonym_of_c ) {
                      yytext = (char*)k->synonym_of_c;
                      break;
                    }
                    return k->y_token;
                  }
                } // for

                c_keyword_t const *k = keyword_find( yytext );
                if ( k ) {
                  yylval.type = k->type;
                  return k->y_token;
                } // for

                yylval.name = yytext;
                return Y_NAME;
              }

[1-9]{D}*     { yylval.number = strtol( yytext, NULL, 10 ); return Y_NUMBER; }
0{O}*         { yylval.number = strtol( yytext, NULL,  8 ); return Y_NUMBER; }
0[xX]{H}+     { yylval.number = strtol( yytext, NULL, 16 ); return Y_NUMBER; }

"::"          return Y_COLON_COLON;

"&"           |
"("           |
")"           |
"*"           |
","           |
"["           |
"]"           |
"^"           return *yytext;           /* for blocks (Apple extension) */

"/*"          {                         /* C-style comments */
                for ( int c = input(), prev_c = '\0'; c != EOF;
                      prev_c = c, c = input() ) {
                  if ( prev_c == '*' && c == '/' )
                    break;
                } // for
              }

[#].*         ;                         /* script-style comments */
[ \f\r\t\v]+  ;                         /* ignore all other whitespace */

[;\n]         return Y_END;

.             return Y_ERROR;

%%

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
