/*
**      cdecl -- C gibberish translator
**      src/help.c
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
**
**      This program is free software: you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation, either version 3 of the License, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 * Defines functions for printing the help text.
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
/*  8 */  { "  exit | quit | q", NULL },
/*  9 */  { "english:", NULL },
/* 10 */  { "  [<storage>] array [<number>] of <english>", NULL },
/* 11 */  { "  block [([<arg-list>])] returning <english>", NULL },
/* 12 */  { "  [<storage>] [{const | volatile}] function [([<arg-list>])] returning <english>", NULL },
/* 13 */  { "  [{const | volatile | restrict}] pointer to <english>",
            "  [{const | volatile}] pointer to [member of class <name>] <english>" },
/* 14 */  { "",
            "  [rvalue] reference to <english>" },
/* 15 */  { "  <type>", NULL },
/* 16 */  { "type:", NULL },
/* 17 */  { "  [<storage>] [{<modifier>}] [<C-type>]",
            "  [<storage>] [{<modifier>}] [<C++-type>]" },
/* 18 */  { "  { enum | struct | union } <name>",
            "  { enum | struct | union | class } <name>" },
/* 19 */  { "arg-list: a comma separated list of <name>, <english>, or <name> as <english>",
            "arg-list: a comma separated list of <english> or <name> as <english>" },
/* 20 */  { "gibberish: a C declaration, like \"int *x\"; or cast, like \"(int*)x\"",
            "gibberish: a C++ declaration, like \"int *x\"; or cast, like \"(int*)x\"" },
/* 21 */  { "C-type: bool char char16_t char32_t wchar_t int float double size_t void",
            "C++-type: bool char char16_t char32_t wchar_t int float double size_t void" },
/* 22 */  { "modifier: short long signed unsigned const restrict volatile",
            "modifier: short long signed unsigned const volatile" },
/* 23 */  { "storage: auto extern register static _Thread_local",
            "storage: constexpr extern friend register static thread_local [pure] virtual" },
          { NULL, NULL }
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints a line of help text (in color, if possible and requested).
 *
 * @param line The line to print.
 */
static void print_help_line( char const *line ) {
  if ( !line[0] )
    return;
  for ( char const *c = line; *c; ++c ) {
    switch ( *c ) {
      case '<':
        SGR_START_COLOR( stdout, help_nonterm );
        break;
      case '>':
        PUTC_OUT( *c );
        SGR_END_COLOR( stdout );
        continue;

      case '[':
      case ']':
      case '{':
      case '|':
      case '}':
        SGR_START_COLOR( stdout, help_punct );
        PUTC_OUT( *c );
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
            PUTC_OUT( *c );
          } // for
        }
    } // switch
    PUTC_OUT( *c );
  } // for
  PUTC_OUT( '\n' );
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
