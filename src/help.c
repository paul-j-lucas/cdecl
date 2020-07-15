/*
**      cdecl -- C gibberish translator
**      src/help.c
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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
#include "c_lang.h"
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

#define NOT_IN_LANG   ((char const*)1)  /* don't print text for current lang */
#define SAME_AS_C     ((char const*)2)  /* C++ text is the same as C */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * A single line of help text.
 */
struct help_text {
  char const *text;                     ///< Generic text.
  char const *cpp_text;                 ///< C++ specific text.
};
typedef struct help_text help_text_t;

///////////////////////////////////////////////////////////////////////////////

/**
 * The help text for "where ...".
 */
static char const WHERE_TEXT[] =
  "  [] = 0 or 1; * = 0 or more; + = 1 or more; {} = one of; | = alternate";

// Ideally, help text should be 23 lines or less.

/**
 * The help text for commands.
 */
static help_text_t const HELP_TEXT_COMMANDS[] = {
  { "command:", SAME_AS_C },
  { "  cast <name> {as|into|to} <english>",
    "  [const | dynamic | reinterpret | static] cast <name> {as|into|to} <english>" },
  { "  declare <name> as <english> [aligned [as|to] {<number> [bytes] | <english>}]",
    SAME_AS_C },
  { NOT_IN_LANG,
    "  declare <operator> as <english>" },
  { "  define <name> as <english>", SAME_AS_C },
  { "  explain <gibberish>", SAME_AS_C },
  { "  { help | ? } [command[s] | english]", SAME_AS_C },
  { "  set [<option> [= <value>] | options | <lang>]", SAME_AS_C },
  { "  show [<name> | all | predefined | user] [[as] typedef]", SAME_AS_C },
  { "  typedef <gibberish>", SAME_AS_C },
  { NOT_IN_LANG,
    "  <scope-c> <name> \\{ { <scope-c> | <typedef> | <using> } ; \\}" },
  { NOT_IN_LANG,
    "  using <name> = <gibberish>" },
  { "  exit | quit | q", SAME_AS_C },
  { "gibberish: a C declaration, like \"int x\"; or cast, like \"(int)x\"",
    "gibberish: a C\\+\\+ declaration, like \"int x\"; or cast, like \"(int)x\"" },
  { "option: [no]alt-tokens [no]debug {di|tri|no}graphs [no]east-const",
    SAME_AS_C },
  { "        [no]explain-by-default [no]explicit-int[=<types>] lang=<lang>",
    SAME_AS_C },
  { "        [no]prompt [no]semicolon",
    SAME_AS_C },
  { "lang: C K&R C89 C95 C99 C11 C18 C\\+\\+ C\\+\\+98 C\\+\\+03 C\\+\\+11 C\\+\\+14 C\\+\\+17 C\\+\\+20",
    SAME_AS_C },
  { NOT_IN_LANG,
    "scope-c: class struct union [inline] namespace" },
  { NOT_IN_LANG,
    "scope-e: { <scope-c> | scope }" },
  { "", "" },
  { "where:", SAME_AS_C },
  { WHERE_TEXT, SAME_AS_C },
  { NULL, NULL }
};

/**
 * The help text for pseudo-English.
 */
static help_text_t const HELP_TEXT_ENGLISH[] = {
  { "english:", SAME_AS_C },
  { "  <store>* array [[static] <cv-qual>* {<number>|\\*}] of <english>",
    "  <store>* array [<number>] of <english>" },
  { "  <store>* variable [length] array <cv-qual>* of <english>", NOT_IN_LANG },
  { NOT_IN_LANG,
    "  <store>+ constructor [([<args>])]" },
  { NOT_IN_LANG,
    "  [virtual] destructor" },
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
  { NOT_IN_LANG,
    "  user-defined conversion [operator] [of <scope-e> <name>]* returning <english>" },
  { "  block [([<args>])] [returning <english>]",
    "  user-defined literal [([<args>])] [returning <english>]" },
  { "  <store>* <modifier>* [<C-type>]",
    "  <store>* <modifier>* [<C\\+\\+-type>]" },
  { "args: a comma separated list of <name>, <english>, or <name> as <english>",
    "args: a comma separated list of <english> or <name> as <english>" },
  { "C-type: bool char char16_t char32_t wchar_t int float double void",
    "C\\+\\+-type: bool char char8_t char16_t char32_t wchar_t int float double void" },
  { "cv-qual: _Atomic const restrict volatile",
    "cv-qual: const volatile" },
  { NOT_IN_LANG,
    "fn-qual: const volatile [rvalue] reference" },
  { "modifier: short long signed unsigned atomic const restrict volatile",
    "modifier: short long signed unsigned const volatile" },
  { "name: a C identifier",
    "name: a C\\+\\+ identifier; or <name>[::<name>]* or <name> [of <scope-e> <name>]*" },
  { "store: auto extern register static thread_local",
    "store: const{eval|expr} extern friend mutable static thread_local [pure] virtual" },
  { "", "" },
  { "where:", SAME_AS_C },
  { WHERE_TEXT, SAME_AS_C },
  { NULL, NULL }
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether \a c is a title character.
 *
 * @param c The character to check.
 * @return Returns `true` only if \a c is a title character.
 */
C_WARN_UNUSED_RESULT
static bool is_title_char( char c ) {
  switch ( c ) {
    case '+':
    case '-':
    case '\\':
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
C_WARN_UNUSED_RESULT
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
  assert( line != NULL );

  bool is_escaped = false;              // was preceding char a '\'?
  bool in_title = false;                // is current char within a title?

  for ( char const *c = line; *c != '\0'; ++c ) {
    switch ( *c ) {
      case '\\':                        // escapes next char
        if ( !is_escaped ) {
          is_escaped = true;
          continue;
        }
        break;
      case ':':                         // ends a title
        if ( in_title ) {
          SGR_END_COLOR( stdout );
          in_title = false;
        }
        break;
      case '<':                         // begins non-terminal
        if ( !is_escaped )
          SGR_START_COLOR( stdout, help_nonterm );
        break;
      case '>':                         // ends non-terminal
        if ( !is_escaped ) {
          PUTC_OUT( *c );
          SGR_END_COLOR( stdout );
          continue;
        }
        break;
      case '*':                         // other EBNF chars
      case '+':
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
        break;
    } // switch

    if ( c == line && is_title( c ) ) {
      SGR_START_COLOR( stdout, help_title );
      in_title = true;
    }

    PUTC_OUT( *c );
    is_escaped = false;
  } // for
  PUTC_OUT( '\n' );
}

static void print_help_text( help_text_t const *help ) {
  assert( help != NULL );
  bool const is_cpp = C_LANG_IS_CPP();

  for ( ; help->text != NULL; ++help ) {
    if ( is_cpp ) {
      if ( help->cpp_text == NOT_IN_LANG )
        continue;
      if ( help->cpp_text != SAME_AS_C ) {
        print_help_line( help->cpp_text );
        continue;
      }
    }
    if ( help->text != NOT_IN_LANG )
      print_help_line( help->text );
  } // for
}

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints the help message to standard output.
 *
 * @param what What to print help for, one of: L_COMMANDS, L_DEFAULT, or
 * L_ENGLISH.
 */
void print_help( char const *what ) {
  assert( what != NULL );

  // The == works because the parser gaurantees specific string literals.
  if ( what == L_DEFAULT || what == L_COMMANDS ) {
    print_help_text( HELP_TEXT_COMMANDS );
    return;
  }
  if ( what == L_ENGLISH ) {
    print_help_text( HELP_TEXT_ENGLISH );
    return;
  }
  UNEXPECTED_STR_VALUE( what );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
