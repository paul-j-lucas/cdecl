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
#include "literals.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define NOT_IN_LANG   "~"               /* don't print text for current lang */
#define SAME_AS_C     "$"               /* C++ text is the same as C */

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

static help_text_t const HELP_TEXT_COMMANDS[] = {
  { "command:", SAME_AS_C },
  { "  cast <name> into <english>",
    "  [const | dynamic | reinterpret | static] cast <name> into <english>" },
  { "  declare <name> as <english>",
    "  declare { <name> | <operator> } as <english>" },
  { "  define <name> as <english>", SAME_AS_C },
  { "  explain <gibberish>", SAME_AS_C },
  { "  { help | ? } [command[s] | english]", SAME_AS_C },
  { "  set [options]", SAME_AS_C },
  { "  show { <name> | all | predefined | user } [typedef]", SAME_AS_C },
  { "  typedef <gibberish>", SAME_AS_C },
  { NOT_IN_LANG,
    "  <scope-c> <name> \\{ { <scope-c> | <typedef> | <using> } ; \\}" },
  { NOT_IN_LANG,
    "  using <name> = <gibberish>" },
  { "  exit | quit | q", SAME_AS_C },
  { "gibberish: a C declaration, like \"int x\"; or cast, like \"(int)x\"",
    "gibberish: a C++ declaration, like \"int x\"; or cast, like \"(int)x\"" },
  { NOT_IN_LANG,
    "scope-c: class struct union [inline] namespace" },
  { NOT_IN_LANG,
    "scope-e: { <scope-c> | scope }" },
  { "", "" },
  { "where:", SAME_AS_C },
  { "  [] = 0 or 1; * = 0 or more; {} = one of; | = alternate; <> = defined elsewhere", SAME_AS_C },
  { NULL, NULL }
};

static help_text_t const HELP_TEXT_ENGLISH[] = {
  { "english:", SAME_AS_C },
  { "  <store>* array [[static] <cv-qual>* {<number>|\\*}] of <english>",
    "  <store>* array [<number>] of <english>" },
  { "  <store>* variable length array <cv-qual>* of <english>", NOT_IN_LANG },
  { "  <store>* function [([<args>])] [returning <english>]",
    "  <store>* <fn-qual>* [[non-]member] function [([<args>])] [returning <english>]" },
  { NOT_IN_LANG,
    "  <store>* <fn-qual>* [[non-]member] operator [([<args>])] [returning <english>]" },
  { "  <cv-qual>* pointer to <english>",
    "  <cv-qual>* pointer to [member of { class | struct } <name>] <english>" },

  { "  { enum | struct | union } <name>",
    "  { enum [class|struct] | class | struct | union } <name>" },
  { NOT_IN_LANG,
    "  [rvalue] reference to <english>" },
  { "  block [([<args>])] [returning <english>]",
    "  user-defined literal [([<args>])] [returning <english>]" },
  { "  <store>* <modifier>* [<C-type>]",
    "  <store>* <modifier>* [<C++-type>]" },
  { "args: a comma separated list of <name>, <english>, or <name> as <english>",
    "args: a comma separated list of <english> or <name> as <english>" },
  { "C-type: bool char char16_t char32_t wchar_t int float double void",
    "C++-type: bool char char8_t char16_t char32_t wchar_t int float double void" },
  { "cv-qual: _Atomic const restrict volatile",
    "cv-qual: const volatile" },
  { NOT_IN_LANG,
    "fn-qual: const volatile [rvalue] reference" },
  { "modifier: short long signed unsigned atomic const restrict volatile",
    "modifier: short long signed unsigned const volatile" },
  { "name: a C identifier",
    "name: a C++ identifier; or <name>[::<name>]* or <name> [of <scope-e> <name>]*" },
  { "store: auto extern register static thread_local",
    "store: const{eval|expr} extern friend mutable static thread_local [pure] virtual" },
  { "", "" },
  { "where:", SAME_AS_C },
  { "  [] = 0 or 1; * = 0 or more; {} = one of; | = alternate; <> = defined elsewhere", SAME_AS_C },
  { NULL, NULL }
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

static void print_help_text( help_text_t const *help ) {
  bool const is_cpp = C_LANG_IS_CPP();

  for ( ; help->text != NULL; ++help ) {
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

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints the help message to standard output.
 *
 * @param what What to print help for.
 */
void print_help( char const *what ) {
  // The == works because the parser gaurantees specific string literals.
  if ( what == L_DEFAULT || what == L_COMMANDS ) {
    print_help_text( HELP_TEXT_COMMANDS );
    return;
  }
  if ( what == L_ENGLISH ) {
    print_help_text( HELP_TEXT_ENGLISH );
    return;
  }
  INTERNAL_ERR( "unexpected value (\"%s\") for what\n", what );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
