/*
**    cdecl -- C gibberish translator
**    src/cdlex.l
*/

%option nounput

%{
// local
#include "config.h"
#include "cdgram.h"
#include "options.h"
#include "util.h"

// standard
#include <string.h>

#define CSD(X)                    check_strdup(X)
#define YYTEXT                    CSD(yytext)

#define BEFORE(LANG)              (LANG - 1)
#define NOT_BEFORE(LANG,TOKEN)    not_lang( BEFORE(LANG), TOKEN )

///////////////////////////////////////////////////////////////////////////////

// extern functions
extern int  yywrap( void );

////////// local functions ////////////////////////////////////////////////////

static int not_lang( lang_t forbidden, int token ) {
  if ( opt_lang & forbidden )
    return T_ERROR;
  yylval.dynstr = YYTEXT;
  return token;
}

///////////////////////////////////////////////////////////////////////////////
%}

A             [A-Za-z_]
O             [0-7]
D             [0-9]
H             [0-9A-Fa-f]
AD            [A-Za-z_0-9]

%%

              /* commands */
cast          return T_CAST;
declare       return T_DECLARE;
exit          return 0;
explain       return T_EXPLAIN;
help          return T_HELP;
q             return 0;
quit          return 0;
set           return T_SET;
"?"           return T_HELP;

              /* english */
array         return T_ARRAY;
as            return T_AS;
block         return T_BLOCK;           /* Apple extension */
character     { yylval.dynstr = CSD("char");    return T_CHAR; }
func          return T_FUNCTION;
function      return T_FUNCTION;
integer       { yylval.dynstr = CSD("int");     return T_INT; }
into          return T_INTO;
member        return T_MEMBER;
of            return T_OF;
pointer       return T_POINTER;
ptr           return T_POINTER;
ref           return T_REFERENCE;
reference     return T_REFERENCE;
ret           return T_RETURNING;
returning     return T_RETURNING;
structure     { yylval.dynstr = CSD("struct");  return T_STRUCT; }
to            return T_TO;
vector        return T_ARRAY;

              /* K&R C */
auto          { yylval.dynstr = YYTEXT;         return T_AUTO; }
char          { yylval.dynstr = YYTEXT;         return T_CHAR; }
double        { yylval.dynstr = YYTEXT;         return T_DOUBLE; }
extern        { yylval.dynstr = YYTEXT;         return T_EXTERN; }
float         { yylval.dynstr = YYTEXT;         return T_FLOAT; }
int           { yylval.dynstr = YYTEXT;         return T_INT; }
long          { yylval.dynstr = YYTEXT;         return T_LONG; }
register      { yylval.dynstr = YYTEXT;         return T_REGISTER; }
short         { yylval.dynstr = YYTEXT;         return T_SHORT; }
static        { yylval.dynstr = YYTEXT;         return T_STATIC; }
struct        { yylval.dynstr = YYTEXT;         return T_STRUCT; }
union         { yylval.dynstr = YYTEXT;         return T_UNION; }
unsigned      { yylval.dynstr = YYTEXT;         return T_UNSIGNED; }

              /* C89 */
const         { return NOT_BEFORE( LANG_C_89, T_CONST ); }
constant      { yylval.dynstr = CSD("const");   return T_CONST; }
enum          { return NOT_BEFORE( LANG_C_89, T_ENUM ); }
enumeration   { yylval.dynstr = CSD("enum");    return T_ENUM; }
signed        { return NOT_BEFORE( LANG_C_89, T_SIGNED ); }
void          { return NOT_BEFORE( LANG_C_89, T_VOID ); }
volatile      { return NOT_BEFORE( LANG_C_89, T_VOLATILE ); }

              /* C99 */
bool          { return NOT_BEFORE( LANG_C_99, T_BOOL ); }
_Bool         { yylval.dynstr = CSD("bool");    return T_BOOL; }
complex       { return NOT_BEFORE( LANG_C_99, T_COMPLEX ); }
_Complex      { yylval.dynstr = CSD("complex"); return T_COMPLEX; }
restrict      { return NOT_BEFORE( LANG_C_99, T_RESTRICT ); }
wchar_t       { return NOT_BEFORE( LANG_C_99, T_WCHAR_T ); }

              /* C11 */
_Noreturn     { return NOT_BEFORE( LANG_C_11, T_NORETURN ); }
_Thread_local { return NOT_BEFORE( LANG_C_11, T_THREAD_LOCAL ); }

              /* C++ */
class         { return NOT_BEFORE( LANG_CPP, T_CLASS ); }

              /* C11 & C++11 */
char16_t      { yylval.dynstr = YYTEXT;         return T_CHAR16_T; }
char32_t      { yylval.dynstr = YYTEXT;         return T_CHAR32_T; }

noalias       { yylval.dynstr = YYTEXT;         return T_VOLATILE; }

{A}{AD}*      { yylval.dynstr = YYTEXT;         return T_NAME; }
{D}+          |
0{O}+         |
0[xX]{H}+     { yylval.dynstr = YYTEXT;         return T_NUMBER; }

"::"          return T_COLON_COLON;

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

[;\n]         return T_END;

[ \f\r\t\v]+  ;                         /* ignore all other whitespace */

.             return T_ERROR;

%%

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
