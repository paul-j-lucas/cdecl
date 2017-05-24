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
/*  1 */  { "[] = 0 or 1; * = 0 or more; {} = one of; | = alternate; <> = defined elsewhere", NULL },
/*  2 */  { "command:", NULL },
/*  3 */  { "  declare <name> as <english>         | set [options]", NULL },
/*  4 */  { "  cast <name> into <english>          | help | ?", NULL },
/*  5 */  { "  explain <gibberish>                 | exit | quit | q", NULL },
/*  6 */  { "english:", NULL },
/*  7 */  { "  [<storage>]* array [<number>] of <english>", NULL },
/*  8 */  { "  block [([<args>])] [returning <english>]", NULL },
/*  9 */  { "  [<storage>]* function [([<args>])] [returning <english>]",
            "  [<storage>]* [<fn-qualifier>]* function [([<args>])] [returning <english>]" },
/* 10 */  { "  [<cv-qualifier>]* pointer to <english>",
            "  [<cv-qualifier>]* pointer to [member of class <name>] <english>" },
/* 11 */  { NULL,
            "  [rvalue] reference to <english>" },
/* 12 */  { "  <type>", NULL },
/* 13 */  { "type:", NULL },
/* 14 */  { "  [<storage>]* [<modifier>]* [<C-type>]",
            "  [<storage>]* [<modifier>]* [<C++-type>]" },
/* 15 */  { "  { enum | struct | union } <name>",
            "  { enum [class|struct] | struct | union | class } <name>" },
/* 16 */  { "args: a comma separated list of <name>, <english>, or <name> as <english>",
            "args: a comma separated list of <english> or <name> as <english>" },
/* 17 */  { "gibberish: a C declaration, like \"int x\"; or cast, like \"(int)x\"",
            "gibberish: a C++ declaration, like \"int x\"; or cast, like \"(int)x\"" },
/* 18 */  { "C-type: bool char char16_t char32_t wchar_t int float double size_t void",
            "C++-type: bool char char16_t char32_t wchar_t int float double size_t void" },
/* 19 */  { "cv-qualifier: _Atomic const restrict volatile",
            "cv-qualifier: const volatile" },
/* 20 */  { NULL,
            "fn-qualifier: const volatile [rvalue] reference" },
/* 21 */  { "modifier: short long signed unsigned atomic const restrict volatile",
            "modifier: short long signed unsigned const volatile" },
/* 22 */  { "name: a C identifier",
            "name: a C++ identifier" },
/* 23 */  { "storage: auto extern register static thread_local",
            "storage: constexpr extern friend register static thread_local [pure] virtual" },
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints a line of help text (in color, if possible and requested).
 *
 * @param line The line to print.
 */
static void print_help_line( char const *line ) {
  for ( char const *c = line; *c; ++c ) {
    switch ( *c ) {
      case '<':
        SGR_START_COLOR( stdout, help_nonterm );
        break;
      case '>':
        PUTC_OUT( *c );
        SGR_END_COLOR( stdout );
        continue;

      case '*':
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
  for ( size_t i = 0; i < ARRAY_SIZE( HELP_TEXT ); ++i ) {
    help_text_t const *const ht = &HELP_TEXT[i];
    if ( opt_lang >= LANG_CPP_MIN && ht->cpp_text )
      print_help_line( ht->cpp_text );
    else if ( ht->text )
      print_help_line( ht->text );
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
