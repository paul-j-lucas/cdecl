/*
**      cdecl -- C gibberish translator
**      src/help.c
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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
#include "cdecl.h"                      /* must go first */
#include "color.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define NOT_IN_LANG   "~"               /* don't print text for current lang */
#define SAME_AS_C     ""                /* C++ text is the same as C */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Help text.
 */
struct help_text {
  char const *text;                     ///< Generic text.
  char const *cpp_text;                 ///< C++ specific text.
};
typedef struct help_text help_text_t;

///////////////////////////////////////////////////////////////////////////////

/**
 * Help text (limited to 80 columns and 23 lines so it fits on an 80x24
 * screen).
 */
static help_text_t const HELP_TEXT[] = {
/*  C | C++ */
/* ===|==== */
/*  1 |  1 */  { "[] = 0 or 1; * = 0 or more; {} = one of; | = alternate; <> = defined elsewhere", SAME_AS_C },
/*  2 |  2 */  { "command:", SAME_AS_C },
/*  3 |  3 */  { "  explain <gibberish>        | declare <name> as <english>",
                 "  explain <gibberish>        | declare { <name> | <operator> } as <english>" },
/*  4 |  4 */  { "  cast <name> into <english>",
                 "  [const | dynamic | reinterpret | static] cast <name> into <english>" },
/*  5 |  5 */  { "  define <name> as <english> | typedef <gibberish>",
                 "  define <name> as <english> | typedef <gibberish> | using <name> = <gibberish>" },
/*  6 |  6 */  { "  show { <name> | all | predefined | user } [typedef] | set [options]", SAME_AS_C },
/*  7 |  7 */  { "  {help|?} | {exit|quit|q}",
                 "  <scope-c> <name> \\{ {<scope-c>|<typedef>|<using>}; \\} | {help|?} | {exit|quit|q}" },
/*  8 |  8 */  { "english:", SAME_AS_C },
/*  9 |  9 */  { "  <store>* array [[static] <cv-qual>* {<number>|\\*}] of <english>",
                 "  <store>* array [<number>] of <english>" },
/* 10 | -- */  { "  <store>* variable length array <cv-qual>* of <english>", NOT_IN_LANG },
/* 11 | 10 */  { "  <store>* function [([<args>])] [returning <english>]",
                 "  <store>* <fn-qual>* [[non-]member] function [([<args>])] [returning <english>]" },
/* -- | 11 */  { NOT_IN_LANG,
                 "  <store>* <fn-qual>* [[non-]member] operator [([<args>])] [returning <english>]" },
/* 12 | 12 */  { "  <cv-qual>* pointer to <english>",
                 "  <cv-qual>* pointer to [member of {class|struct} <name>] <english>" },
/* 13 | 13 */  { "  <store>* <modifier>* [<C-type>]",
                 "  [rvalue] reference to <english> | <store>* <modifier>* [<C++-type>]" },
/* 14 | 14 */  { "  { enum | struct | union } <name>",
                 "  { enum [class|struct] | class | struct | union } <name>" },
/* 15 | 15 */  { "  block [([<args>])] [returning <english>]",
                 "  user-defined literal [([<args>])] [returning <english>]" },
/* 16 | 16 */  { "args: a comma separated list of <name>, <english>, or <name> as <english>",
                 "args: a comma separated list of <english> or <name> as <english>" },
/* 17 | 17 */  { "gibberish: a C declaration, like \"int x\"; or cast, like \"(int)x\"",
                 "gibberish: a C++ declaration, like \"int x\"; or cast, like \"(int)x\"" },
/* 18 | 18 */  { "C-type: bool char char16_t char32_t wchar_t int float double void",
                 "C++-type: bool char char16_t char32_t wchar_t int float double void" },
/* 19 | 19 */  { "cv-qual: _Atomic const restrict volatile",
                 "cv-qual: const volatile         | fn-qual: const volatile [rvalue] reference" },
/* 20 | 20 */  { "modifier: short long signed unsigned atomic const restrict volatile",
                 "modifier: short long signed unsigned const volatile" },
/* 21 | 21 */  { "name: a C identifier",
                 "name: a C++ identifier; or <name>[::<name>]* or <name> [of <scope-e> <name>]*" },
/* -- | 22 */  { NOT_IN_LANG,
                 "scope-c: class struct union [inline] namespace | scope-e: <scope-c> scope" },
/* 22 | 23 */  { "store: auto extern register static thread_local",
                 "store: const{eval|expr} extern friend mutable static thread_local [pure] virtual" },
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether \a c is a title character.
 *
 * @param c The character to check.
 * @return Returns `true` only if \a c is a title character.
 */
static bool is_title_char( char c ) {
  switch ( c ) {
    case '+':
    case '-':
    case '_':
      return true;
    default:
      return isalpha( c );
  } // switch
}

/**
 * Checks whether the string \a s is a title.
 *
 * @param s The string to check.
 * @return Returns `true` only if \a s is a title string.
 */
static bool is_title( char const *s ) {
  assert( s != NULL );
  if ( isalpha( *s ) ) {
    while ( *++s != '\0' ) {
      if ( *s == ':' )
        return true;
      if ( !is_title_char( *s ) )
        break;
    } // while
  }
  return false;
}

/**
 * Prints a line of help text (in color, if possible and requested).
 *
 * @param line The line to print.
 */
static void print_help_line( char const *line ) {
  bool is_escaped = false;              // was preceding char a '\'?
  bool in_title = false;                // is current char within a title?

  for ( char const *c = line; *c != '\0'; ++c ) {
    switch ( *c ) {
      case ':':
        if ( in_title ) {
          SGR_END_COLOR( stdout );
          in_title = false;
        }
        break;

      case '\\':
        if ( !is_escaped ) {
          is_escaped = true;
          continue;
        }
        // FALLTHROUGH
      case '<':
        if ( !is_escaped ) {
          SGR_START_COLOR( stdout, help_nonterm );
          break;
        }
        // FALLTHROUGH
      case '>':
        if ( !is_escaped ) {
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
        if ( !is_escaped ) {
          SGR_START_COLOR( stdout, help_punct );
          PUTC_OUT( *c );
          SGR_END_COLOR( stdout );
          continue;
        }
        // FALLTHROUGH

      default:
        if ( !in_title && is_title( c ) ) {
          SGR_START_COLOR( stdout, help_title );
          in_title = true;
        }
        is_escaped = false;
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
  bool const is_cpp = C_LANG_IS_CPP();

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
