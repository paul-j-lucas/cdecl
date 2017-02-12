%{
/*
**    cdecl -- C gibberish translator
**    src/cdgram.y
*/

// local
#include "config.h"
#include "cdecl.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef dodebug
# define Debug(x) do { if (DebugFlag) fprintf x; } while (0)
#else
# define Debug(x) /* nothing */
#endif

extern char const   unknown_name[];
extern unsigned     modbits;
extern char const  *savedname;
extern bool         DebugFlag;
extern char         prev;
extern int          arbdims;

extern char*  cat();
extern void   c_type_check( void );
extern int    yylex( void );

///////////////////////////////////////////////////////////////////////////////

static void do_cast( char const *name, char const *left, char const *right,
                     char const *type ) {
	assert( left );
  assert( right );
  assert( type );

  size_t const lenl = strlen( left ), lenr = strlen( right );

  if ( prev == 'f' )
    unsupp( "Cast into function", "cast into pointer to function" );
  else if (prev=='A' || prev=='a')
    unsupp( "Cast into array","cast into pointer" );
  printf(
    "(%s%*s%s)%s\n",
    type, (int)(lenl + lenr ? lenl + 1 : 0),
    left, right, name ? name : "expression"
  );
  free( (void*)left );
  free( (void*)right );
  free( (void*)type );
  if ( name )
    free( (void*)name );
}

static void print_help( void );

static void yyerror( char const *s ) {
  PRINT_ERR( "%s\n", s );
  Debug((stdout, "yychar=%d\n", yychar));
}

int yywrap( void ) {
  return 1;
}

///////////////////////////////////////////////////////////////////////////////

%}

%union {
  char *dynstr;
  struct {
    char *left;
    char *right;
    char *type;
  } halves;
}

%token  T_ARRAY
%token  T_AS
%token  T_BLOCK
%token  T_CAST
%token  T_COMMA
%token  T_DECLARE
%token  T_DOUBLECOLON
%token  T_EXPLAIN
%token  T_FUNCTION
%token  T_HELP
%token  T_INTO
%token  T_MEMBER
%token  T_OF
%token  T_POINTER
%token  T_REFERENCE
%token  T_RETURNING
%token  T_SET
%token  T_TO

%token  <dynstr> T_VOID
%token  <dynstr> T_BOOL
%token  <dynstr> T_CHAR
%token  <dynstr> T_WCHAR_T
%token  <dynstr> T_SHORT T_INT T_LONG T_FLOAT T_DOUBLE
%token  <dynstr> T_SIGNED
%token  <dynstr> T_UNSIGNED
%token  <dynstr> T_CLASS
%token  <dynstr> T_CONST_VOLATILE
%token  <dynstr> T_ENUM
%token  <dynstr> T_NAME
%token  <dynstr> T_STRUCT T_UNION
%token  <dynstr> T_NUMBER

%token  <dynstr> T_AUTO T_EXTERN T_REGISTER T_STATIC
%type   <dynstr> adecllist adims c_type cast castlist cdecl cdecl1 cdims
%type   <dynstr> constvol_list ClassStruct mod_list mod_list1 modifier
%type   <dynstr> opt_constvol_list optNAME opt_storage storage StrClaUniEnum
%type   <dynstr> tname type
%type   <halves> adecl

%start prog

%%

prog
  : /* empty */
  | prog stmt
    {
      print_prompt();
      prev = 0;
    }
  ;

stmt
  : T_HELP NL
    {
      Debug((stderr, "stmt: help\n"));
      print_help();
    }

  | T_DECLARE T_NAME T_AS opt_storage adecl NL
    {
      Debug((stderr, "stmt: DECLARE NAME AS opt_storage adecl\n"));
      Debug((stderr, "\tNAME='%s'\n", $2));
      Debug((stderr, "\topt_storage='%s'\n", $4));
      Debug((stderr, "\tacdecl.left='%s'\n", $5.left));
      Debug((stderr, "\tacdecl.right='%s'\n", $5.right));
      Debug((stderr, "\tacdecl.type='%s'\n", $5.type));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
      dodeclare($2, $4, $5.left, $5.right, $5.type);
    }

  | T_DECLARE opt_storage adecl NL
    {
      Debug((stderr, "stmt: DECLARE opt_storage adecl\n"));
      Debug((stderr, "\topt_storage='%s'\n", $2));
      Debug((stderr, "\tacdecl.left='%s'\n", $3.left));
      Debug((stderr, "\tacdecl.right='%s'\n", $3.right));
      Debug((stderr, "\tacdecl.type='%s'\n", $3.type));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
      dodeclare(NULL, $2, $3.left, $3.right, $3.type);
    }

  | T_CAST T_NAME T_INTO adecl NL
    {
      Debug((stderr, "stmt: CAST NAME AS adecl\n"));
      Debug((stderr, "\tNAME='%s'\n", $2));
      Debug((stderr, "\tacdecl.left='%s'\n", $4.left));
      Debug((stderr, "\tacdecl.right='%s'\n", $4.right));
      Debug((stderr, "\tacdecl.type='%s'\n", $4.type));
      do_cast($2, $4.left, $4.right, $4.type);
    }

  | T_CAST adecl NL
    {
      Debug((stderr, "stmt: CAST adecl\n"));
      Debug((stderr, "\tacdecl.left='%s'\n", $2.left));
      Debug((stderr, "\tacdecl.right='%s'\n", $2.right));
      Debug((stderr, "\tacdecl.type='%s'\n", $2.type));
      do_cast(NULL, $2.left, $2.right, $2.type);
    }

  | T_EXPLAIN opt_storage opt_constvol_list type opt_constvol_list cdecl NL
    {
      Debug((stderr, "stmt: EXPLAIN opt_storage opt_constvol_list type cdecl\n"));
      Debug((stderr, "\topt_storage='%s'\n", $2));
      Debug((stderr, "\topt_constvol_list='%s'\n", $3));
      Debug((stderr, "\ttype='%s'\n", $4));
      Debug((stderr, "\tcdecl='%s'\n", $6));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
      dodexplain($2, $3, $5, $4, $6);
    }

  | T_EXPLAIN storage opt_constvol_list cdecl NL
    {
      Debug((stderr, "stmt: EXPLAIN storage opt_constvol_list cdecl\n"));
      Debug((stderr, "\tstorage='%s'\n", $2));
      Debug((stderr, "\topt_constvol_list='%s'\n", $3));
      Debug((stderr, "\tcdecl='%s'\n", $4));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
      dodexplain($2, $3, NULL, NULL, $4);
    }

  | T_EXPLAIN opt_storage constvol_list cdecl NL
    {
      Debug((stderr, "stmt: EXPLAIN opt_storage constvol_list cdecl\n"));
      Debug((stderr, "\topt_storage='%s'\n", $2));
      Debug((stderr, "\tconstvol_list='%s'\n", $3));
      Debug((stderr, "\tcdecl='%s'\n", $4));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
      dodexplain($2, $3, NULL, NULL, $4);
    }

  | T_EXPLAIN '(' opt_constvol_list type cast ')' optNAME NL
    {
      Debug((stderr, "stmt: EXPLAIN ( opt_constvol_list type cast ) optNAME\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $3));
      Debug((stderr, "\ttype='%s'\n", $4));
      Debug((stderr, "\tcast='%s'\n", $5));
      Debug((stderr, "\tNAME='%s'\n", $7));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
      docexplain($3, $4, $5, $7);
    }

  | T_SET optNAME NL
    {
      Debug((stderr, "stmt: SET optNAME\n"));
      Debug((stderr, "\toptNAME='%s'\n", $2));
      doset($2);
    }

  | NL
  | error NL
    {
      yyerrok;
    }
  ;

NL
  : '\n'
    {
      doprompt();
    }
  | ';'
    {
      noprompt();
    }
  ;

optNAME
  : T_NAME
    {
      Debug((stderr, "optNAME: NAME\n"));
      Debug((stderr, "\tNAME='%s'\n", $1));
      $$ = $1;
    }

  | /* empty */
    {
      Debug((stderr, "optNAME: EMPTY\n"));
      $$ = strdup(unknown_name);
    }
  ;

cdecl
  : cdecl1
  | '*' opt_constvol_list cdecl
    {
      Debug((stderr, "cdecl: * opt_constvol_list cdecl\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $2));
      Debug((stderr, "\tcdecl='%s'\n", $3));
      $$ = cat($3,$2,strdup(strlen($2)?" pointer to ":"pointer to "),NULL);
      prev = 'p';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | T_NAME T_DOUBLECOLON '*' cdecl
    {
      Debug((stderr, "cdecl: NAME DOUBLECOLON '*' cdecl\n"));
      Debug((stderr, "\tNAME='%s'\n", $1));
      Debug((stderr, "\tcdecl='%s'\n", $4));
      if (opt_lang != LANG_CXX)
        unsupp("pointer to member of class", NULL);
      $$ = cat($4,strdup("pointer to member of class "),$1,strdup(" "),NULL);
      prev = 'p';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | '&' opt_constvol_list cdecl
    {
      Debug((stderr, "cdecl: & opt_constvol_list cdecl\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $2));
      Debug((stderr, "\tcdecl='%s'\n", $3));
      if (opt_lang != LANG_CXX)
        unsupp("reference", NULL);
      $$ = cat($3,$2,strdup(strlen($2)?" reference to ":"reference to "),NULL);
      prev = 'r';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }
  ;

cdecl1
  : cdecl1 '(' ')'
    {
      Debug((stderr, "cdecl1: cdecl1()\n"));
      Debug((stderr, "\tcdecl1='%s'\n", $1));
      $$ = cat($1,strdup("function returning "),NULL);
      prev = 'f';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | '(' '^' opt_constvol_list cdecl ')' '(' ')'
    {
      char const *sp = "";
      Debug((stderr, "cdecl1: (^ opt_constvol_list cdecl)()\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $3));
      Debug((stderr, "\tcdecl='%s'\n", $4));
      if (strlen($3) > 0)
          sp = " ";
      $$ = cat($4, $3, strdup(sp), strdup("block returning "), NULL);
      prev = 'b';
    }

  | '(' '^' opt_constvol_list cdecl ')' '(' castlist ')'
    {
      char const *sp = "";
      Debug((stderr, "cdecl1: (^ opt_constvol_list cdecl)( castlist )\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $3));
      Debug((stderr, "\tcdecl='%s'\n", $4));
      Debug((stderr, "\tcastlist='%s'\n", $7));
      if (strlen($3) > 0)
        sp = " ";
      $$ = cat($4, $3, strdup(sp), strdup("block ("), $7, strdup(") returning "), NULL);
      prev = 'b';
    }

  | cdecl1 '(' castlist ')'
    {
      Debug((stderr, "cdecl1: cdecl1(castlist)\n"));
      Debug((stderr, "\tcdecl1='%s'\n", $1));
      Debug((stderr, "\tcastlist='%s'\n", $3));
      $$ = cat($1, strdup("function ("),
          $3, strdup(") returning "), NULL);
      prev = 'f';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | cdecl1 cdims
    {
      Debug((stderr, "cdecl1: cdecl1 cdims\n"));
      Debug((stderr, "\tcdecl1='%s'\n", $1));
      Debug((stderr, "\tcdims='%s'\n", $2));
      $$ = cat($1,strdup("array "),$2,NULL);
      prev = 'a';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | '(' cdecl ')'
    {
      Debug((stderr, "cdecl1: (cdecl)\n"));
      Debug((stderr, "\tcdecl='%s'\n", $2));
      $$ = $2;
      /* prev = prev; */
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | T_NAME
    {
      Debug((stderr, "cdecl1: NAME\n"));
      Debug((stderr, "\tNAME='%s'\n", $1));
      savedname = $1;
      $$ = strdup("");
      prev = 'n';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }
  ;

castlist
  : castlist T_COMMA castlist
    {
      Debug((stderr, "castlist: castlist1, castlist2\n"));
      Debug((stderr, "\tcastlist1='%s'\n", $1));
      Debug((stderr, "\tcastlist2='%s'\n", $3));
      $$ = cat($1, strdup(", "), $3, NULL);
    }

  | opt_constvol_list type cast
    {
      Debug((stderr, "castlist: opt_constvol_list type cast\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $1));
      Debug((stderr, "\ttype='%s'\n", $2));
      Debug((stderr, "\tcast='%s'\n", $3));
      $$ = cat($3, $1, strdup(strlen($1) ? " " : ""), $2, NULL);
    }

  | T_NAME
    {
      $$ = $1;
    }
  ;

adecllist
  : /* empty */
    {
      Debug((stderr, "adecllist: EMPTY\n"));
      $$ = strdup("");
    }

  | adecllist T_COMMA adecllist
    {
      Debug((stderr, "adecllist: adecllist1, adecllist2\n"));
      Debug((stderr, "\tadecllist1='%s'\n", $1));
      Debug((stderr, "\tadecllist2='%s'\n", $3));
      $$ = cat($1, strdup(", "), $3, NULL);
    }

  | T_NAME
    {
      Debug((stderr, "adecllist: NAME\n"));
      Debug((stderr, "\tNAME='%s'\n", $1));
      $$ = $1;
    }

  | adecl
    {
      Debug((stderr, "adecllist: adecl\n"));
      Debug((stderr, "\tadecl.left='%s'\n", $1.left));
      Debug((stderr, "\tadecl.right='%s'\n", $1.right));
      Debug((stderr, "\tadecl.type='%s'\n", $1.type));
      $$ = cat($1.type, strdup(" "), $1.left, $1.right, NULL);
    }

  | T_NAME T_AS adecl
    {
      Debug((stderr, "adecllist: NAME AS adecl\n"));
      Debug((stderr, "\tNAME='%s'\n", $1));
      Debug((stderr, "\tadecl.left='%s'\n", $3.left));
      Debug((stderr, "\tadecl.right='%s'\n", $3.right));
      Debug((stderr, "\tadecl.type='%s'\n", $3.type));
      $$ = cat($3.type, strdup(" "), $3.left, $1, $3.right, NULL);
    }
  ;

cast
  : /* empty */
    {
      Debug((stderr, "cast: EMPTY\n"));
      $$ = strdup("");
      /* prev = prev; */
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | '(' ')'
    {
      Debug((stderr, "cast: ()\n"));
      $$ = strdup("function returning ");
      prev = 'f';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | '(' cast ')' '(' ')'
    {
      Debug((stderr, "cast: (cast)()\n"));
      Debug((stderr, "\tcast='%s'\n", $2));
      $$ = cat($2,strdup("function returning "),NULL);
      prev = 'f';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | '(' cast ')' '(' castlist ')'
    {
      Debug((stderr, "cast: (cast)(castlist)\n"));
      Debug((stderr, "\tcast='%s'\n", $2));
      Debug((stderr, "\tcastlist='%s'\n", $5));
      $$ = cat($2,strdup("function ("),$5,strdup(") returning "),NULL);
      prev = 'f';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | '(' '^' cast ')' '(' ')'
    {
      Debug((stderr, "cast: (^ cast)()\n"));
      Debug((stderr, "\tcast='%s'\n", $3));
      $$ = cat($3,strdup("block returning "),NULL);
      prev = 'b';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | '(' '^' cast ')' '(' castlist ')'
    {
      Debug((stderr, "cast: (^ cast)(castlist)\n"));
      Debug((stderr, "\tcast='%s'\n", $3));
      $$ = cat($3,strdup("block ("), $6, strdup(") returning "),NULL);
      prev = 'b';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | '(' cast ')'
    {
      Debug((stderr, "cast: (cast)\n"));
      Debug((stderr, "\tcast='%s'\n", $2));
      $$ = $2;
      /* prev = prev; */
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | T_NAME T_DOUBLECOLON '*' cast
    {
      Debug((stderr, "cast: NAME::*cast\n"));
      Debug((stderr, "\tcast='%s'\n", $4));
      if (opt_lang != LANG_CXX)
        unsupp("pointer to member of class", NULL);
      $$ = cat($4,strdup("pointer to member of class "),$1,strdup(" "),NULL);
      prev = 'p';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | '*' cast
    {
      Debug((stderr, "cast: *cast\n"));
      Debug((stderr, "\tcast='%s'\n", $2));
      $$ = cat($2,strdup("pointer to "),NULL);
      prev = 'p';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | '&' cast
    {
      Debug((stderr, "cast: &cast\n"));
      Debug((stderr, "\tcast='%s'\n", $2));
      if (opt_lang != LANG_CXX)
        unsupp("reference", NULL);
      $$ = cat($2,strdup("reference to "),NULL);
      prev = 'r';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | cast cdims
    {
      Debug((stderr, "cast: cast cdims\n"));
      Debug((stderr, "\tcast='%s'\n", $1));
      Debug((stderr, "\tcdims='%s'\n", $2));
      $$ = cat($1,strdup("array "),$2,NULL);
      prev = 'a';
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }
  ;

cdims
  : '[' ']'
    {
      Debug((stderr, "cdims: []\n"));
      $$ = strdup("of ");
    }

  | '[' T_NUMBER ']'
    {
      Debug((stderr, "cdims: [NUMBER]\n"));
      Debug((stderr, "\tNUMBER='%s'\n", $2));
      $$ = cat($2,strdup(" of "),NULL);
    }
  ;

adecl
  : T_FUNCTION T_RETURNING adecl
    {
      Debug((stderr, "adecl: FUNCTION RETURNING adecl\n"));
      Debug((stderr, "\tadecl.left='%s'\n", $3.left));
      Debug((stderr, "\tadecl.right='%s'\n", $3.right));
      Debug((stderr, "\tadecl.type='%s'\n", $3.type));
      if (prev == 'f')
        unsupp("Function returning function",
                "function returning pointer to function");
      else if (prev=='A' || prev=='a')
        unsupp("Function returning array",
                "function returning pointer");
      $$.left = $3.left;
      $$.right = cat(strdup("()"),$3.right,NULL);
      $$.type = $3.type;
      prev = 'f';
      Debug((stderr, "\n\tadecl now =\n"));
      Debug((stderr, "\t\tadecl.left='%s'\n", $$.left));
      Debug((stderr, "\t\tadecl.right='%s'\n", $$.right));
      Debug((stderr, "\t\tadecl.type='%s'\n", $$.type));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | T_FUNCTION '(' adecllist ')' T_RETURNING adecl
    {
      Debug((stderr, "adecl: FUNCTION (adecllist) RETURNING adecl\n"));
      Debug((stderr, "\tadecllist='%s'\n", $3));
      Debug((stderr, "\tadecl.left='%s'\n", $6.left));
      Debug((stderr, "\tadecl.right='%s'\n", $6.right));
      Debug((stderr, "\tadecl.type='%s'\n", $6.type));
      if (prev == 'f')
        unsupp("Function returning function",
                "function returning pointer to function");
      else if (prev=='A' || prev=='a')
        unsupp("Function returning array",
                "function returning pointer");
      $$.left = $6.left;
      $$.right = cat(strdup("("),$3,strdup(")"),$6.right,NULL);
      $$.type = $6.type;
      prev = 'f';
      Debug((stderr, "\n\tadecl now =\n"));
      Debug((stderr, "\t\tadecl.left='%s'\n", $$.left));
      Debug((stderr, "\t\tadecl.right='%s'\n", $$.right));
      Debug((stderr, "\t\tadecl.type='%s'\n", $$.type));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | opt_constvol_list T_BLOCK T_RETURNING adecl
    {
      char const *sp = "";
      Debug((stderr, "adecl: opt_constvol_list BLOCK RETURNING adecl\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $1));
      Debug((stderr, "\tadecl.left='%s'\n", $4.left));
      Debug((stderr, "\tadecl.right='%s'\n", $4.right));
      Debug((stderr, "\tadecl.type='%s'\n", $4.type));
      if (prev == 'f')
        unsupp("Block returning function",
               "block returning pointer to function");
      else if (prev=='A' || prev=='a')
        unsupp("Block returning array",
               "block returning pointer");
      if (strlen($1) != 0)
        sp = " ";
      $$.left = cat($4.left, strdup("(^"), strdup(sp), $1, strdup(sp), NULL);
      $$.right = cat(strdup(")()"),$4.right,NULL);
      $$.type = $4.type;
      prev = 'b';
    }

  | opt_constvol_list T_BLOCK '(' adecllist ')' T_RETURNING adecl
    {
      char const *sp = "";
      Debug((stderr, "adecl: opt_constvol_list BLOCK RETURNING adecl\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $1));
      Debug((stderr, "\tadecllist='%s'\n", $4));
      Debug((stderr, "\tadecl.left='%s'\n", $7.left));
      Debug((stderr, "\tadecl.right='%s'\n", $7.right));
      Debug((stderr, "\tadecl.type='%s'\n", $7.type));
      if (prev == 'f')
        unsupp("Block returning function",
               "block returning pointer to function");
      else if (prev=='A' || prev=='a')
        unsupp("Block returning array",
               "block returning pointer");
      if (strlen($1) != 0)
          sp = " ";
      $$.left = cat($7.left, strdup("(^"), strdup(sp), $1, strdup(sp), NULL);
      $$.right = cat(strdup(")("), $4, strdup(")"), $7.right, NULL);
      $$.type = $7.type;
      prev = 'b';
    }

  | T_ARRAY adims T_OF adecl
    {
      Debug((stderr, "adecl: ARRAY adims OF adecl\n"));
      Debug((stderr, "\tadims='%s'\n", $2));
      Debug((stderr, "\tadecl.left='%s'\n", $4.left));
      Debug((stderr, "\tadecl.right='%s'\n", $4.right));
      Debug((stderr, "\tadecl.type='%s'\n", $4.type));
      if (prev == 'f')
        unsupp("Array of function",
                "array of pointer to function");
      else if (prev == 'a')
        unsupp("Inner array of unspecified size",
                "array of pointer");
      else if (prev == 'v')
        unsupp("Array of void",
                "pointer to void");
      if (arbdims)
        prev = 'a';
      else
        prev = 'A';
      $$.left = $4.left;
      $$.right = cat($2,$4.right,NULL);
      $$.type = $4.type;
      Debug((stderr, "\n\tadecl now =\n"));
      Debug((stderr, "\t\tadecl.left='%s'\n", $$.left));
      Debug((stderr, "\t\tadecl.right='%s'\n", $$.right));
      Debug((stderr, "\t\tadecl.type='%s'\n", $$.type));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | opt_constvol_list T_POINTER T_TO adecl
    {
      char const *op = "", *cp = "", *sp = "";

      Debug((stderr, "adecl: opt_constvol_list POINTER TO adecl\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $1));
      Debug((stderr, "\tadecl.left='%s'\n", $4.left));
      Debug((stderr, "\tadecl.right='%s'\n", $4.right));
      Debug((stderr, "\tadecl.type='%s'\n", $4.type));
      if (prev == 'a')
        unsupp("Pointer to array of unspecified dimension",
                "pointer to object");
      if (prev=='a' || prev=='A' || prev=='f') {
        op = "(";
        cp = ")";
      }
      if (strlen($1) != 0)
        sp = " ";
      $$.left = cat($4.left,strdup(op),strdup("*"), strdup(sp),$1,strdup(sp),NULL);
      $$.right = cat(strdup(cp),$4.right,NULL);
      $$.type = $4.type;
      prev = 'p';
      Debug((stderr, "\n\tadecl now =\n"));
      Debug((stderr, "\t\tadecl.left='%s'\n", $$.left));
      Debug((stderr, "\t\tadecl.right='%s'\n", $$.right));
      Debug((stderr, "\t\tadecl.type='%s'\n", $$.type));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | opt_constvol_list T_POINTER T_TO T_MEMBER T_OF ClassStruct T_NAME adecl
    {
      char const *op = "", *cp = "", *sp = "";

      Debug((stderr, "adecl: opt_constvol_list POINTER TO MEMBER OF ClassStruct NAME adecl\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $1));
      Debug((stderr, "\tClassStruct='%s'\n", $6));
      Debug((stderr, "\tNAME='%s'\n", $7));
      Debug((stderr, "\tadecl.left='%s'\n", $8.left));
      Debug((stderr, "\tadecl.right='%s'\n", $8.right));
      Debug((stderr, "\tadecl.type='%s'\n", $8.type));
      if (opt_lang != LANG_CXX)
        unsupp("pointer to member of class", NULL);
      if (prev == 'a')
        unsupp("Pointer to array of unspecified dimension",
                "pointer to object");
      if (prev=='a' || prev=='A' || prev=='f') {
        op = "(";
        cp = ")";
      }
      if (strlen($1) != 0)
        sp = " ";
      $$.left = cat($8.left,strdup(op),$7,strdup("::*"), strdup(sp),$1,strdup(sp),NULL);
      $$.right = cat(strdup(cp),$8.right,NULL);
      $$.type = $8.type;
      prev = 'p';
      Debug((stderr, "\n\tadecl now =\n"));
      Debug((stderr, "\t\tadecl.left='%s'\n", $$.left));
      Debug((stderr, "\t\tadecl.right='%s'\n", $$.right));
      Debug((stderr, "\t\tadecl.type='%s'\n", $$.type));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | opt_constvol_list T_REFERENCE T_TO adecl
    {
      char const *op = "", *cp = "", *sp = "";

      Debug((stderr, "adecl: opt_constvol_list REFERENCE TO adecl\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $1));
      Debug((stderr, "\tadecl.left='%s'\n", $4.left));
      Debug((stderr, "\tadecl.right='%s'\n", $4.right));
      Debug((stderr, "\tadecl.type='%s'\n", $4.type));
      if (opt_lang != LANG_CXX)
        unsupp("reference", NULL);
      if (prev == 'v')
        unsupp("Reference to void",
                "pointer to void");
      else if (prev == 'a')
        unsupp("Reference to array of unspecified dimension",
                "reference to object");
      if (prev=='a' || prev=='A' || prev=='f') {
        op = "(";
        cp = ")";
      }
      if (strlen($1) != 0)
        sp = " ";
      $$.left = cat($4.left,strdup(op),strdup("&"), strdup(sp),$1,strdup(sp),NULL);
      $$.right = cat(strdup(cp),$4.right,NULL);
      $$.type = $4.type;
      prev = 'r';
      Debug((stderr, "\n\tadecl now =\n"));
      Debug((stderr, "\t\tadecl.left='%s'\n", $$.left));
      Debug((stderr, "\t\tadecl.right='%s'\n", $$.right));
      Debug((stderr, "\t\tadecl.type='%s'\n", $$.type));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }

  | opt_constvol_list type
    {
      Debug((stderr, "adecl: opt_constvol_list type\n"));
      Debug((stderr, "\topt_constvol_list='%s'\n", $1));
      Debug((stderr, "\ttype='%s'\n", $2));
      $$.left = strdup("");
      $$.right = strdup("");
      $$.type = cat($1,strdup(strlen($1)?" ":""),$2,NULL);
      if (strcmp($2, "void") == 0)
          prev = 'v';
      else if ((strncmp($2, "struct", 6) == 0) ||
                (strncmp($2, "class", 5) == 0))
          prev = 's';
      else
          prev = 't';
      Debug((stderr, "\n\tadecl now =\n"));
      Debug((stderr, "\t\tadecl.left='%s'\n", $$.left));
      Debug((stderr, "\t\tadecl.right='%s'\n", $$.right));
      Debug((stderr, "\t\tadecl.type='%s'\n", $$.type));
      Debug((stderr, "\tprev = '%s'\n", visible(prev)));
    }
  ;

adims
  : /* empty */
    {
      Debug((stderr, "adims: EMPTY\n"));
      arbdims = 1;
      $$ = strdup("[]");
    }

  | T_NUMBER
    {
      Debug((stderr, "adims: NUMBER\n"));
      Debug((stderr, "\tNUMBER='%s'\n", $1));
      arbdims = 0;
      $$ = cat(strdup("["),$1,strdup("]"),NULL);
    }
  ;

type
  : tinit c_type
    {
      Debug((stderr, "type: tinit c_type\n"));
      Debug((stderr, "\ttinit=''\n"));
      Debug((stderr, "\tc_type='%s'\n", $2));
      c_type_check();
      $$ = $2;
    }
  ;

tinit
  : /* empty */
    {
      Debug((stderr, "tinit: EMPTY\n"));
      modbits = 0;
    }
  ;

c_type
  : mod_list
    {
      Debug((stderr, "c_type: mod_list\n"));
      Debug((stderr, "\tmod_list='%s'\n", $1));
      $$ = $1;
    }

  | tname
    {
      Debug((stderr, "c_type: tname\n"));
      Debug((stderr, "\ttname='%s'\n", $1));
      $$ = $1;
    }

  | mod_list tname
    {
      Debug((stderr, "c_type: mod_list tname\n"));
      Debug((stderr, "\tmod_list='%s'\n", $1));
      Debug((stderr, "\ttname='%s'\n", $2));
      $$ = cat($1,strdup(" "),$2,NULL);
    }

  | StrClaUniEnum T_NAME
    {
      Debug((stderr, "c_type: StrClaUniEnum NAME\n"));
      Debug((stderr, "\tStrClaUniEnum='%s'\n", $1));
      Debug((stderr, "\tNAME='%s'\n", $2));
      $$ = cat($1,strdup(" "),$2,NULL);
    }
  ;

StrClaUniEnum
  : ClassStruct
  | T_ENUM
  | T_UNION
    {
      $$ = $1;
    }
  ;

ClassStruct
  : T_STRUCT
  | T_CLASS
    {
      $$ = $1;
    }
  ;

tname
  : T_INT
    {
      Debug((stderr, "tname: INT\n"));
      Debug((stderr, "\tINT='%s'\n", $1));
      modbits |= C_TYPE_INT; $$ = $1;
    }

  | T_BOOL
    {
      Debug((stderr, "tname: BOOL\n"));
      Debug((stderr, "\tCHAR='%s'\n", $1));
      modbits |= C_TYPE_BOOL; $$ = $1;
    }

  | T_CHAR
    {
      Debug((stderr, "tname: CHAR\n"));
      Debug((stderr, "\tCHAR='%s'\n", $1));
      modbits |= C_TYPE_CHAR; $$ = $1;
    }

  | T_WCHAR_T
    {
      Debug((stderr, "tname: WCHAR_T\n"));
      Debug((stderr, "\tCHAR='%s'\n", $1));
      modbits |= C_TYPE_WCHAR_T; $$ = $1;
    }

  | T_FLOAT
    {
      Debug((stderr, "tname: FLOAT\n"));
      Debug((stderr, "\tFLOAT='%s'\n", $1));
      modbits |= C_TYPE_FLOAT; $$ = $1;
    }

  | T_DOUBLE
    {
      Debug((stderr, "tname: DOUBLE\n"));
      Debug((stderr, "\tDOUBLE='%s'\n", $1));
      modbits |= C_TYPE_DOUBLE; $$ = $1;
    }

  | T_VOID
    {
      Debug((stderr, "tname: VOID\n"));
      Debug((stderr, "\tVOID='%s'\n", $1));
      modbits |= C_TYPE_VOID; $$ = $1;
    }
  ;

mod_list
  : modifier mod_list1
    {
      Debug((stderr, "mod_list: modifier mod_list1\n"));
      Debug((stderr, "\tmodifier='%s'\n", $1));
      Debug((stderr, "\tmod_list1='%s'\n", $2));
      $$ = cat($1,strdup(" "),$2,NULL);
    }

  | modifier
    {
      Debug((stderr, "mod_list: modifier\n"));
      Debug((stderr, "\tmodifier='%s'\n", $1));
      $$ = $1;
    }
  ;

mod_list1
  : mod_list
    {
      Debug((stderr, "mod_list1: mod_list\n"));
      Debug((stderr, "\tmod_list='%s'\n", $1));
      $$ = $1;
    }

  | T_CONST_VOLATILE
    {
      Debug((stderr, "mod_list1: CONSTVOLATILE\n"));
      Debug((stderr, "\tCONSTVOLATILE='%s'\n", $1));
      if (opt_lang == LANG_C_KNR)
        notsupported(" (Pre-ANSI Compiler)", $1, NULL);
      else if ((strcmp($1, "noalias") == 0) && opt_lang == LANG_CXX)
        unsupp($1, NULL);
      $$ = $1;
    }
  ;

modifier
  : T_UNSIGNED
    {
      Debug((stderr, "modifier: UNSIGNED\n"));
      Debug((stderr, "\tUNSIGNED='%s'\n", $1));
      modbits |= C_TYPE_UNSIGNED; $$ = $1;
    }

  | T_SIGNED
    {
      Debug((stderr, "modifier: SIGNED\n"));
      Debug((stderr, "\tSIGNED='%s'\n", $1));
      modbits |= C_TYPE_SIGNED; $$ = $1;
    }

  | T_LONG
    {
      Debug((stderr, "modifier: LONG\n"));
      Debug((stderr, "\tLONG='%s'\n", $1));
      modbits |= C_TYPE_LONG; $$ = $1;
    }

  | T_SHORT
    {
      Debug((stderr, "modifier: SHORT\n"));
      Debug((stderr, "\tSHORT='%s'\n", $1));
      modbits |= C_TYPE_SHORT; $$ = $1;
    }
  ;

opt_constvol_list
  : T_CONST_VOLATILE opt_constvol_list
    {
      Debug((stderr, "opt_constvol_list: CONSTVOLATILE opt_constvol_list\n"));
      Debug((stderr, "\tCONSTVOLATILE='%s'\n", $1));
      Debug((stderr, "\topt_constvol_list='%s'\n", $2));
      if (opt_lang == LANG_C_KNR)
        notsupported(" (Pre-ANSI Compiler)", $1, NULL);
      else if ((strcmp($1, "noalias") == 0) && opt_lang == LANG_CXX)
        unsupp($1, NULL);
      $$ = cat($1,strdup(strlen($2) ? " " : ""),$2,NULL);
    }

  | /* empty */
    {
      Debug((stderr, "opt_constvol_list: EMPTY\n"));
      $$ = strdup("");
    }
  ;

constvol_list
  : T_CONST_VOLATILE opt_constvol_list
    {
      Debug((stderr, "constvol_list: CONSTVOLATILE opt_constvol_list\n"));
      Debug((stderr, "\tCONSTVOLATILE='%s'\n", $1));
      Debug((stderr, "\topt_constvol_list='%s'\n", $2));
      if (opt_lang == LANG_C_KNR)
        notsupported(" (Pre-ANSI Compiler)", $1, NULL);
      else if ((strcmp($1, "noalias") == 0) && opt_lang == LANG_CXX)
        unsupp($1, NULL);
      $$ = cat($1,strdup(strlen($2) ? " " : ""),$2,NULL);
    }
  ;

storage
  : T_AUTO
  | T_EXTERN
  | T_REGISTER
  | T_STATIC
    {
      Debug((stderr, "storage: AUTO,EXTERN,STATIC,REGISTER (%s)\n", $1));
      $$ = $1;
    }
  ;

opt_storage
  : storage
    {
      Debug((stderr, "opt_storage: storage=%s\n", $1));
      $$ = $1;
    }

  | /* empty */
    {
      Debug((stderr, "opt_storage: EMPTY\n"));
      $$ = strdup("");
    }
  ;

%%

///////////////////////////////////////////////////////////////////////////////

/* the help messages */
struct help_text {
  char const *text;                     // generic text 
  char const *cpptext;                  // C++ specific text 
};
typedef struct help_text help_text_t;

static help_text_t const HELP_TEXT[] = {
  // up-to 23 lines of help text so it fits on (24x80) screens

/*  1 */ { "[] means optional; {} means 1 or more; <> means defined elsewhere", NULL },
/*  2 */ { "  commands are separated by ';' and newlines", NULL },
/*  3 */ { "command:", NULL },
/*  4 */ { "  declare <name> as <english>", NULL },
/*  5 */ { "  cast <name> into <english>", NULL },
/*  6 */ { "  explain <gibberish>", NULL },
/*  7 */ { "  set or set options", NULL },
/*  8 */ { "  help, ?", NULL },
/*  9 */ { "  quit or exit", NULL },
/* 10 */ { "english:", NULL },
/* 11 */ { "  function [( <decl-list> )] returning <english>", NULL },
/* 12 */ { "  block [( <decl-list> )] returning <english>", NULL },
/* 13 */ { "  array [<number>] of <english>", NULL },
/* 14 */ { "  [{ const | volatile | noalias }] pointer to <english>",
    "  [{const|volatile}] {pointer|reference} to [member of class <name>] <english>" },
/* 15 */{ "  <type>", NULL },
/* 16 */{ "type:", NULL },
/* 17 */{ "  {[<storage-class>] [{<modifier>}] [<C-type>]}", NULL },
/* 18 */{ "  { struct | union | enum } <name>",
    "  {struct|class|union|enum} <name>" },
/* 19 */{ "decllist: a comma separated list of <name>, <english> or <name> as <english>", NULL },
/* 20 */{ "name: a C identifier", NULL },
/* 21 */{ "gibberish: a C declaration, like 'int *x', or cast, like '(int *)x'", NULL },
/* 22 */{ "storage-class: extern, static, auto, register", NULL },
/* 23 */{ "C-type: int, char, float, double, or void", NULL },
/* 24 */{ "modifier: short, long, signed, unsigned, const, volatile, or noalias",
    "modifier: short, long, signed, unsigned, const, or volatile" },
  { NULL, NULL }
};

static void print_help( void ) {
  char const *const fmt = opt_lang == LANG_CXX ? " %s\n" : "  %s\n";

  for ( help_text_t const *p = HELP_TEXT; p->text; p++ ) {
    if (opt_lang == LANG_CXX && p->cpptext)
      printf( fmt, p->cpptext );
    else
      printf( fmt, p->text );
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
