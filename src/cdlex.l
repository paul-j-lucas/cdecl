%{
/*
 *    cdecl -- C gibberish translator
 *    src/cdlex.l
 */

// local
#include "config.h"
#include "cdgram.h"
#include "util.h"

// standard
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

extern int yywrap( void );

%}

N             [0-9]
A             [A-Za-z_]
AN            [A-Za-z_0-9]

%%

array         return T_ARRAY;
as            return T_AS;
block         return T_BLOCK;
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

auto          { yylval.dynstr = strdup(yytext);   return T_AUTO; }
bool          { yylval.dynstr = strdup(yytext);   return T_BOOL; }
character     { yylval.dynstr = strdup("char");   return T_CHAR; }
char          { yylval.dynstr = strdup(yytext);   return T_CHAR; }
wchar_t       { yylval.dynstr = strdup(yytext);   return T_WCHAR_T; }
class         { yylval.dynstr = strdup(yytext);   return T_CLASS; }
constant      { yylval.dynstr = strdup("const");  return T_CONST_VOLATILE; }
const         { yylval.dynstr = strdup(yytext);   return T_CONST_VOLATILE; }
double        { yylval.dynstr = strdup(yytext);   return T_DOUBLE; }
enumeration   { yylval.dynstr = strdup("enum");   return T_ENUM; }
enum          { yylval.dynstr = strdup(yytext);   return T_ENUM; }
extern        { yylval.dynstr = strdup(yytext);   return T_EXTERN; }
float         { yylval.dynstr = strdup(yytext);   return T_FLOAT; }
integer       { yylval.dynstr = strdup("int");    return T_INT; }
int           { yylval.dynstr = strdup(yytext);   return T_INT; }
long          { yylval.dynstr = strdup(yytext);   return T_LONG; }
noalias       { yylval.dynstr = strdup(yytext);   return T_CONST_VOLATILE; }
register      { yylval.dynstr = strdup(yytext);   return T_REGISTER; }
short         { yylval.dynstr = strdup(yytext);   return T_SHORT; }
signed        { yylval.dynstr = strdup(yytext);   return T_SIGNED; }
static        { yylval.dynstr = strdup(yytext);   return T_STATIC; }
structure     { yylval.dynstr = strdup("struct"); return T_STRUCT; }
struct        { yylval.dynstr = strdup(yytext);   return T_STRUCT; }
union         { yylval.dynstr = strdup(yytext);   return T_UNION; }
unsigned      { yylval.dynstr = strdup(yytext);   return T_UNSIGNED; }
void          { yylval.dynstr = strdup(yytext);   return T_VOID; }
volatile      { yylval.dynstr = strdup(yytext);   return T_CONST_VOLATILE; }

{A}{AN}*      { yylval.dynstr = strdup(yytext);   return T_NAME; }
{N}+          { yylval.dynstr = strdup(yytext);   return T_NUMBER; }

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
