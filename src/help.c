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
/*  2 */  { "command:", NULL },
/*  3 */  { "  declare <name> as <english>", NULL },
/*  4 */  { "  cast <name> into <english>", NULL },
/*  5 */  { "  explain <gibberish>", NULL },
/*  6 */  { "  set [options]", NULL },
/*  7 */  { "  help | ?", NULL },
/*  8 */  { "  exit | quit", NULL },
/*  9 */  { "english:", NULL },
/* 10 */  { "  array [<number>] of <english>", NULL },
/* 11 */  { "  block [( [<decl-list>] )] returning <english>", NULL },
/* 12 */  { "  function [( [<decl-list>] )] returning <english>", NULL },
/* 13 */  { "  [{ const[ant] | volatile | restrict[ed] }] pointer to <english>",
            "  [{const|volatile}] {pointer|reference} to [member of class <name>] <english>" },
/* 14 */  { "  <type>", NULL },
/* 15 */  { "type:", NULL },
/* 16 */  { "  [<storage-class>] [{<modifier>}] [<C-type>]",
            "  [<storage-class>] [{<modifier>}] [<C++-type>]" },
/* 17 */  { "  { enum | struct | union } <name>",
            "  { enum | struct | union | class } <name>" },
/* 18 */  { "decl-list: a comma separated list of <name>, <english>, or <name> as <english>", NULL },
/* 19 */  { "name: a C identifier",
            "name: a C++ identifier" },
/* 20 */  { "gibberish: a C declaration, like \"int *x\", or cast, like \"(int*)x\"",
            "gibberish: a C++ declaration, like \"int *x\", or cast, like \"(int*)x\"" },
/* 21 */  { "C-type: bool, int, char, char16_t, char32_t, wchar_t, float, double, or void",
            "C++-type: bool, int, char, char16_t, char32_t, wchar_t, float, double, or void" },
/* 22 */  { "modifier: short, long, signed, unsigned, const, volatile, or restrict",
            "modifier: short, long, signed, unsigned, const, or volatile" },
/* 23 */  { "storage-class: auto, extern, register, static, or _Thread_local",
            "storage-class: extern, register, static, or thread_local" },
          { NULL, NULL }
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints a line of help text (in color, if possible and requested).
 *
 * @param line The line to print.
 */
static void print_help_line( char const *line ) {
  FPUTC( ' ', stdout );
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
