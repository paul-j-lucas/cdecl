/*
**    cdecl -- C gibberish translator
**    src/cdlex.l
*/

%option noinput
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

array         return T_ARRAY;
as            return T_AS;
block         return T_BLOCK;           /* Apple extension */
cast          return T_CAST;
declare       return T_DECLARE;
exit          return 0;
explain       return T_EXPLAIN;
func          return T_FUNCTION;
function      return T_FUNCTION;
help          return T_HELP;
into          return T_INTO;
member        return T_MEMBER;
of            return T_OF;
pointer       return T_POINTER;
ptr           return T_POINTER;
q             return 0;
quit          return 0;
ref           return T_REFERENCE;
reference     return T_REFERENCE;
ret           return T_RETURNING;
returning     return T_RETURNING;
set           return T_SET;
to            return T_TO;
vector        return T_ARRAY;
::            return T_DOUBLECOLON;
[?]           return T_HELP;
[,]           return T_COMMA;

auto          { yylval.dynstr = YYTEXT;         return T_AUTO; }
bool          { yylval.dynstr = YYTEXT;         return T_BOOL; }
char          { yylval.dynstr = YYTEXT;         return T_CHAR; }
character     { yylval.dynstr = CSD("char");    return T_CHAR; }
class         { yylval.dynstr = YYTEXT;         return T_CLASS; }
const         { yylval.dynstr = YYTEXT;         return T_CONST_VOLATILE; }
constant      { yylval.dynstr = CSD("const");   return T_CONST_VOLATILE; }
double        { yylval.dynstr = YYTEXT;         return T_DOUBLE; }
enum          { yylval.dynstr = YYTEXT;         return T_ENUM; }
enumeration   { yylval.dynstr = CSD("enum");    return T_ENUM; }
extern        { yylval.dynstr = YYTEXT;         return T_EXTERN; }
float         { yylval.dynstr = YYTEXT;         return T_FLOAT; }
int           { yylval.dynstr = YYTEXT;         return T_INT; }
integer       { yylval.dynstr = CSD("int");     return T_INT; }
long          { yylval.dynstr = YYTEXT;         return T_LONG; }
noalias       { yylval.dynstr = YYTEXT;         return T_CONST_VOLATILE; }
register      { yylval.dynstr = YYTEXT;         return T_REGISTER; }
short         { yylval.dynstr = YYTEXT;         return T_SHORT; }
signed        { yylval.dynstr = YYTEXT;         return T_SIGNED; }
static        { yylval.dynstr = YYTEXT;         return T_STATIC; }
struct        { yylval.dynstr = YYTEXT;         return T_STRUCT; }
structure     { yylval.dynstr = CSD("struct");  return T_STRUCT; }
union         { yylval.dynstr = YYTEXT;         return T_UNION; }
unsigned      { yylval.dynstr = YYTEXT;         return T_UNSIGNED; }
void          { yylval.dynstr = YYTEXT;         return T_VOID; }
volatile      { yylval.dynstr = YYTEXT;         return T_CONST_VOLATILE; }
wchar_t       { yylval.dynstr = YYTEXT;         return T_WCHAR_T; }

{A}{AN}*      { yylval.dynstr = YYTEXT;         return T_NAME; }
{N}+          { yylval.dynstr = YYTEXT;         return T_NUMBER; }

[#].*         ;
[\t ]         ;

[&*[\]();^\n] return *yytext;

.             {
                PRINT_ERR( "bad character '%s'\n", visible( *yytext ) );
                return *yytext;
              }

%%

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
