/*
**      cdecl -- C gibberish translator
**      src/help.c
*/

// local
#include "config.h"                     /* must go first */
#include "color.h"
#include "common.h"
#include "options.h"
#include "util.h"

// standard
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

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
/*  1 */  { "| = alternate; [] = optional; {} = 1 or more; <> = defined elsewhere", NULL },
/*  2 */  { "  commands are separated by ';' or newline", NULL },
/*  3 */  { "command:", NULL },
/*  4 */  { "  declare <name> as <english>", NULL },
/*  5 */  { "  cast <name> into <english>", NULL },
/*  6 */  { "  explain <gibberish>", NULL },
/*  7 */  { "  set [options]", NULL },
/*  8 */  { "  help | ?", NULL },
/*  9 */  { "  exit | quit", NULL },
/* 10 */  { "english:", NULL },
/* 11 */  { "  array [<number>] of <english>", NULL },
/* 12 */  { "  block [( <decl-list> )] returning <english>", NULL },
/* 13 */  { "  function [( <decl-list> )] returning <english>", NULL },
/* 14 */  { "  [{ const[ant] | volatile | restrict[ed] }] pointer to <english>",
            "  [{const|volatile}] {pointer|ref[erence]} to [member of class <name>] <english>" },
/* 15 */  { "  <type>", NULL },
/* 16 */  { "type:", NULL },
/* 17 */  { "  {[<storage-class>] [{<modifier>}] [<C-type>]}", NULL },
/* 18 */  { "  { struct | union | enum } <name>",
            "  {struct|class|union|enum} <name>" },
/* 19 */  { "decl-list: a comma separated list of <name>, <english>, or <name> as <english>", NULL },
/* 20 */  { "name: a C identifier",
            "name: a C++ identifier" },
/* 21 */  { "gibberish: a C declaration, like \"int *x\", or cast, like \"(int*)x\"", NULL },
/* 22 */  { "storage-class: auto, extern, register, static, or _Thread_local",
            "storage-class: extern, register, static, or thread_local" },
/* 23 */  { "C-type: bool, int, char, char16_t, char32_t, wchar_t, float, double, or void", NULL },
/* 24 */  { "modifier: short, long, signed, unsigned, const, volatile, or restrict",
          "modifier: short, long, signed, unsigned, const, or volatile" },
          { NULL, NULL }
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints a line of help text (in color, if possible and requested).
 *
 * @param line The line to print.
 */
static void print_help_line( char const *line ) {
  FPUTS( "  ", stdout );
  for ( char const *c = line; *c; ++c ) {
    switch ( *c ) {
      case '<':
        SGR_START_COLOR( stdout, help_nonterm );
        break;
      case '>':
        FPUTC( *c, stdout );
        SGR_END_COLOR( stdout );
        continue;

      case '[':
      case ']':
      case '{':
      case '|':
      case '}':
        SGR_START_COLOR( stdout, help_punct );
        FPUTC( *c, stdout );
        SGR_END_COLOR( stdout );
        continue;

      default:
        if ( c == line && isalpha( *c ) ) {
          SGR_START_COLOR( stdout, help_title );
          for ( ; *c; ++c ) {
            if ( *c == ':' ) {
              SGR_END_COLOR( stdout );
              break;
            }
            FPUTC( *c, stdout );
          } // for
        }
    } // switch
    FPUTC( *c, stdout );
  } // for
  FPUTC( '\n', stdout );
}

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints the help message to standard output.
 */
void print_help( void ) {
  for ( help_text_t const *ht = HELP_TEXT; ht->text; ++ht ) {
    if ( opt_lang >= LANG_CPP_MIN && ht->cpp_text )
      print_help_line( ht->cpp_text );
    else
      print_help_line( ht->text );
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
