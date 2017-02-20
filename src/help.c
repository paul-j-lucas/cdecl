/*
**      cdecl -- C gibberish translator
**      src/help.c
*/

// local
#include "options.h"

// standard
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////

struct help_text {
  char const *text;                     // generic text 
  char const *cpp_text;                 // C++ specific text 
};
typedef struct help_text help_text_t;

///////////////////////////////////////////////////////////////////////////////

/**
 * Help text (limited to 80 columns and 23 lines so it fits on an 80x24
 * screen).
 */
static help_text_t const HELP_TEXT[] = {
/*  1 */ { "[] means optional; {} means 1 or more; <> means defined elsewhere",
           NULL },
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
/* 12 */ { "  array [<number>] of <english>", NULL },
/* 13 */ { "  block [( <decl-list> )] returning <english>", NULL },
/* 14 */ { "  [{ const | volatile | restrict }] pointer to <english>",
           "  [{const|volatile}] {pointer|reference} to [member of class <name>] <english>" },
/* 15 */{ "  <type>", NULL },
/* 16 */{ "type:", NULL },
/* 17 */{ "  {[<storage-class>] [{<modifier>}] [<C-type>]}", NULL },
/* 18 */{ "  { struct | union | enum } <name>",
          "  {struct|class|union|enum} <name>" },
/* 19 */{ "decllist: a comma separated list of <name>, <english>, or <name> as <english>", NULL },
/* 20 */{ "name: a C identifier", NULL },
/* 21 */{ "gibberish: a C declaration, like 'int *x', or cast, like '(int *)x'", NULL },
/* 22 */{ "storage-class: auto, extern, register, or static",
          "storage-class: extern, register, or static" },
/* 23 */{ "C-type: bool, int, char, wchar_t, float, double, or void", NULL },
/* 24 */{ "modifier: short, long, signed, unsigned, const, volatile, or restrict",
          "modifier: short, long, signed, unsigned, const, or volatile" },
  { NULL, NULL }
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints the help message to standard output.
 */
void print_help( void ) {
  for ( help_text_t const *ht = HELP_TEXT; ht->text; ++ht ) {
    if ( opt_lang == LANG_CPP && ht->cpp_text )
      printf( " %s\n", ht->cpp_text );
    else
      printf( " %s\n", ht->text );
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
