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

#define NOT_IN_LANG   "~"               /* don't print text for current lang */
#define SAME_AS_C     ""                /* C++ text is the same as C */

/**
 * Help text (limited to 80 columns and 23 lines so it fits on an 80x24
 * screen).
 */
static help_text_t const HELP_TEXT[] = {
/*  C | C++ */
/* ===|==== */
/*  1 |  1 */  { "[] = 0 or 1; * = 0 or more; {} = one of; | = alternate; <> = defined elsewhere", SAME_AS_C },
/*  2 |  2 */  { "command:", SAME_AS_C },
/*  3 |  3 */  { "  declare <name> as <english>         | set [options]         | help | ?", SAME_AS_C },
/*  4 |  4 */  { "  explain <gibberish>                 | exit | quit | q", SAME_AS_C },
/*  5 |  5 */  { "  cast <name> into <english>",
                 "  [const | dynamic | reinterpret | static] cast <name> into <english>" },
/*  6 |  6 */  { "  define <name> as <english>          | typedef <gibberish>", SAME_AS_C },
/*  7 |  7 */  { "  show { <name> | [all | predefined | user] types }", SAME_AS_C },
/*  8 |  8 */  { "english:", SAME_AS_C },
/*  9 |  9 */  { "  <storage>* array [[static] <cv-qualifier>* {<number>|\\*}] of <english>",
                 "  <storage>* array [<number>] of <english>" },
/* 10 | -- */  { "  <storage>* variable length array <cv-qualifier>* of <english>", NOT_IN_LANG },
/* 11 | 10 */  { "  block [([<args>])] [returning <english>]", SAME_AS_C },
/* 12 | 11 */  { "  <storage>* function [([<args>])] [returning <english>]",
                 "  <storage>* <fn-qualifier>* function [([<args>])] [returning <english>]" },
/* 13 | 12 */  { "  <cv-qualifier>* pointer to <english>",
                 "  <cv-qualifier>* pointer to [member of class <name>] <english>" },
/* -- | 13 */  { NOT_IN_LANG,
                 "  [rvalue] reference to <english>" },
/* 14 | 14 */  { "  <storage>* <modifier>* [<C-type>]",
                 "  <storage>* <modifier>* [<C++-type>]" },
/* 15 | 15 */  { "  { enum | struct | union } <name>",
                 "  { enum [class|struct] | struct | union | class } <name>" },
/* 16 | 16 */  { "args: a comma separated list of <name>, <english>, or <name> as <english>",
                 "args: a comma separated list of <english> or <name> as <english>" },
/* 17 | 17 */  { "gibberish: a C declaration, like \"int x\"; or cast, like \"(int)x\"",
                 "gibberish: a C++ declaration, like \"int x\"; or cast, like \"(int)x\"" },
/* 18 | 18 */  { "C-type: bool char char16_t char32_t wchar_t int float double void",
                 "C++-type: bool char char16_t char32_t wchar_t int float double void" },
/* 19 | 19 */  { "cv-qualifier: _Atomic const restrict volatile",
                 "cv-qualifier: const volatile" },
/* -- | 20 */  { NOT_IN_LANG,
                 "fn-qualifier: const volatile [rvalue] reference" },
/* 20 | 21 */  { "modifier: short long signed unsigned atomic const restrict volatile",
                 "modifier: short long signed unsigned const volatile" },
/* 21 | 22 */  { "name: a C identifier",
                 "name: a C++ identifier" },
/* 22 | 23 */  { "storage: auto extern register static thread_local",
                 "storage: constexpr extern friend register static thread_local [pure] virtual" },
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints a line of help text (in color, if possible and requested).
 *
 * @param line The line to print.
 */
static void print_help_line( char const *line ) {
  bool escaped = false;

  for ( char const *c = line; *c != '\0'; ++c ) {
    switch ( *c ) {
      case '\\':
        if ( !escaped ) {
          escaped = true;
          continue;
        }
        // FALLTHROUGH
      case '<':
        if ( !escaped ) {
          SGR_START_COLOR( stdout, help_nonterm );
          break;
        }
        // FALLTHROUGH
      case '>':
        if ( !escaped ) {
          PUTC_OUT( *c );
          SGR_END_COLOR( stdout );
          continue;
        }
        // FALLTHROUGH
      case '*':
      case '[':
      case ']':
      case '{':
      case '|':
      case '}':
        if ( !escaped ) {
          SGR_START_COLOR( stdout, help_punct );
          PUTC_OUT( *c );
          SGR_END_COLOR( stdout );
          continue;
        }
        // FALLTHROUGH

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
        escaped = false;
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
  bool const is_cpp = opt_lang >= LANG_CPP_MIN;

  for ( size_t i = 0; i < ARRAY_SIZE( HELP_TEXT ); ++i ) {
    help_text_t const *const help = &HELP_TEXT[i];
    if ( is_cpp ) {
      if ( help->cpp_text[0] == NOT_IN_LANG[0] )
        continue;
      if ( help->cpp_text[0] != SAME_AS_C[0] ) {
        print_help_line( help->cpp_text );
        continue;
      }
    }

    if ( help->text[0] != NOT_IN_LANG[0] )
      print_help_line( help->text );
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
