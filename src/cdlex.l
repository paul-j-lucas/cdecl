/*
**    cdecl -- C gibberish translator
**    src/cdlex.l
*/

%option nounput

%{
// local
#include "config.h"
#include "cdgram.h"
#include "util.h"

// standard
#include <string.h>

#define CSD(X)                    check_strdup(X)
#define YYTEXT                    CSD(yytext)

///////////////////////////////////////////////////////////////////////////////

// extern functions
extern int  yywrap( void );

///////////////////////////////////////////////////////////////////////////////
%}

N             [0-9]
A             [A-Za-z_]
AN            [A-Za-z_0-9]

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
constant      { yylval.dynstr = CSD("const");   return T_CONST_VOLATILE; }
enumeration   { yylval.dynstr = CSD("enum");    return T_ENUM; }
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
const         { yylval.dynstr = YYTEXT;         return T_CONST_VOLATILE; }
enum          { yylval.dynstr = YYTEXT;         return T_ENUM; }
signed        { yylval.dynstr = YYTEXT;         return T_SIGNED; }
void          { yylval.dynstr = YYTEXT;         return T_VOID; }
volatile      { yylval.dynstr = YYTEXT;         return T_CONST_VOLATILE; }

              /* C99 */
bool          { yylval.dynstr = YYTEXT;         return T_BOOL; }
_Bool         { yylval.dynstr = CSD("bool");    return T_BOOL; }
complex       { yylval.dynstr = YYTEXT;         return T_COMPLEX; }
_Complex      { yylval.dynstr = CSD("complex"); return T_COMPLEX; }
restrict      { yylval.dynstr = YYTEXT;         return T_RESTRICT; }
wchar_t       { yylval.dynstr = YYTEXT;         return T_WCHAR_T; }

              /* C++ */
class         { yylval.dynstr = YYTEXT;         return T_CLASS; }

noalias       { yylval.dynstr = YYTEXT;         return T_CONST_VOLATILE; }

{A}{AN}*      { yylval.dynstr = YYTEXT;         return T_NAME; }
{N}+          { yylval.dynstr = YYTEXT;         return T_NUMBER; }

"::"          return T_DOUBLECOLON;

"&"           |
"("           |
")"           |
"*"           |
","           |
";"           |
"["           |
"]"           |
"^"           |                         /* for blocks (Apple extension) */
"\n"          return *yytext;

"/*"          {                         /* C-style comments */
                for ( int c = input(), prev_c = '\0'; c != EOF;
                      prev_c = c, c = input() ) {
                  if ( prev_c == '*' && c == '/' )
                    break;
                } // for
              }

[#].*         ;                         /* script-style comments */

[ \t\r]+      ;

.             {
                PRINT_ERR( "\"%s\": illegal character\n", visible( *yytext ) );
                return *yytext;
              }

%%

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
