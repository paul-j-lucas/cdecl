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
#include "pjl_config.h"                 /* must go first */
#include "c_lang.h"
#include "cdecl.h"
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

/// @endcond

// local functions
static void print_help_where( void );

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether the string \a s is a title.
 *
 * @param s The string to check.
 * @return Returns `true` only if \a s is a title string.
 */
PJL_WARN_UNUSED_RESULT
static bool is_title( char const *s ) {
  assert( s != NULL );
  if ( isalpha( *s ) ) {
    while ( *++s != '\0' ) {
      switch ( *s ) {
        case ':':
          return true;
        case '+':
        case '-':
        case '\\':
          break;
        default:
          if ( !isalpha( *s ) )
            return false;
      } // switch
    } // while
  }
  return false;
}

/**
 * Prints a line of help text (in color, if possible and requested).
 *
 * @param line The line to print.
 */
static void print_h( char const *line ) {
  assert( line != NULL );

  bool is_escaped = false;              // was preceding char a '\'?
  bool in_title = is_title( line );     // is current char within a title?

  if ( in_title )
    SGR_START_COLOR( stdout, help_title );

  for ( char const *s = line; *s != '\0'; ++s ) {
    if ( !is_escaped ) {
      switch ( *s ) {
        case '\\':                      // escapes next char
          is_escaped = true;
          continue;
        case ':':                       // ends a title
          if ( in_title ) {
            SGR_END_COLOR( stdout );
            in_title = false;
          }
          break;
        case '<':                       // begins non-terminal
          SGR_START_COLOR( stdout, help_nonterm );
          break;
        case '>':                       // ends non-terminal
          PUTC( *s );
          SGR_END_COLOR( stdout );
          continue;
        case '*':                       // other EBNF chars
        case '+':
        case '[':
        case ']':
        case '{':
        case '|':
        case '}':
          SGR_START_COLOR( stdout, help_punct );
          PUTC( *s );
          SGR_END_COLOR( stdout );
          continue;
      } // switch
    }

    PUTC( *s );
    is_escaped = false;
  } // for
}

/**
 * Prints the help for commands.
 */
static void print_help_commands( void ) {
  print_h( "command:\n" );

  print_h( "  " );
  if ( C_LANG_IS_CPP() )
    print_h( "[const | dynamic | reinterpret | static] " );
  print_h( "cast <name> {as|into|to} <english>\n" );

  print_h( "  declare <name> as <english>" );
  if ( C_LANG_IS(C_CPP_MIN(11,11)) )
    print_h( " [aligned [as|to] {<number> [bytes] | <english>}]" );
  print_h( "\n" );

  if ( C_LANG_IS_CPP() ) {
    print_h( "  declare <operator> as <english>\n" );
    print_h( "  declare [<english>] user-defined <user-defined-english>\n" );
  }

  print_h( "  define <name> as <english>\n" );
  print_h( "  explain <gibberish>\n" );
  print_h( "  { help | ? } [command[s] | english]\n" );
  print_h( "  set [<option> [= <value>] | options | <lang>]*\n" );

  print_h( "  show [<name> | [all] {predefined | user}] [[as] {english | typedef" );
  if ( C_LANG_IS(MIN(CPP_11)) )
    print_h( " | using" );
  print_h( "}]\n" );

  print_h( "  typedef <gibberish>\n" );

  if ( C_LANG_IS_CPP() ) {
    print_h( "  <scope-c> <name> [\\{ [{ <scope-c> | <typedef>" );
    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( " | <using>" );
    print_h( " } ;]* \\}]\n" );
  }

  if ( C_LANG_IS(MIN(CPP_11)) )
    print_h( "  using <name> = <gibberish>\n" );

  print_h( "  exit | quit | q\n" );

  print_h( "gibberish: a C" );
  if ( C_LANG_IS_CPP() )
    print_h( "\\+\\+" );
  print_h( " declaration, like \"int x\"; or cast, like \"(int)x\"\n" );

  print_h( "option:\n" );
  print_h( "  [no]alt-tokens [no]debug {di|tri|no}graphs [no]east-const\n" );
  print_h( "  [no]explain-by-default [no]explicit-int[=<types>] lang=<lang>\n" );
  print_h( "  [no]prompt [no]semicolon\n" );

  print_h( "lang: C K&R C89 C95 C99 C11 C17 C2X C\\+\\+ C\\+\\+98 C\\+\\+03 C\\+\\+11 C\\+\\+14 C\\+\\+17 C\\+\\+20\n" );

  if ( C_LANG_IS_CPP() ) {
    print_h( "scope-c: class struct union" );
    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( " [inline]" );
    print_h( " namespace\n" );
  }

  print_help_where();
}

/**
 * Prints the help for pseudo English.
 */
static void print_help_english( void ) {
  print_h( "english:\n" );

  if ( C_LANG_IS_C() ) {
    if ( C_LANG_IS(MAX(C_89)) ) {
      print_h( "  <store>* array [<number>] of <english>\n" );
    } else {
      print_h( "  <store>* array [[static] <cv-qual>* {<number>|\\*}] of <english>\n" );
      print_h( "  <store>* variable [length] array <cv-qual>* of <english>\n" );
    }
    print_h( "  <store>* function [([<args>])] [returning <english>]\n" );
    print_h( "  " );
    if ( C_LANG_IS(MIN(C_89)) )
      print_h( "<cv-qual>* " );
    print_h( "pointer to <english>\n" );
  }
  else {
    print_h( "  <store>* array [<number>] of <english>\n" );
    print_h( "  <store>+ constructor [([<args>])]\n" );
    print_h( "  [virtual] destructor\n" );
    print_h( "  <store>* <fn-qual>* [[non-]member] function [([<args>])] [returning <english>]\n" );
    print_h( "  <store>* <fn-qual>* [[non-]member] operator [([<args>])] [returning <english>]\n" );
    print_h( "  <cv-qual>* pointer to [member of { class | struct } <name>] <english>\n" );
  }

  print_h( "  { " );
  if ( C_LANG_IS(MIN(C_89)) ) {
    print_h( "enum " );
    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( "[class|struct] " );
    print_h( "| " );
    if ( C_LANG_IS_CPP() )
      print_h( "class | " );
  }
  print_h( "struct | union } <name>\n" );

  if ( C_LANG_IS_C() ) {
    print_h( "  block [([<args>])] [returning <english>]\n" );
    print_h( "  <store>* <modifier>* [<C-type>]\n" );

    print_h( "args: a comma separated list of " );
    if ( C_LANG_IS(C_KNR) )
      print_h( "<name>\n" );
    else if ( C_LANG_IS(MAX(CPP_17)) )
      print_h( "<name>, <english>, or <name> as <english>\n" );
    else
      print_h( "<english> or <name> as <english>\n" );

    print_h( "C-type:" );
    if ( C_LANG_IS(MIN(C_99)) )
      print_h( " bool" );
    print_h( " char" );
    if ( C_LANG_IS(MIN(C_11)) )
      print_h( " char16_t char32_t" );
    if ( C_LANG_IS(MIN(C_95)) )
      print_h( " wchar_t" );
    print_h( " int float double" );
    if ( C_LANG_IS(MIN(C_89)) )
      print_h( " void" );
    print_h( "\n" );

    if ( C_LANG_IS(MIN(C_89)) ) {
      print_h( "cv-qual:" );
      if ( C_LANG_IS(MIN(C_11)) )
        print_h( " _Atomic" );
      print_h( " const" );
      if ( C_LANG_IS(MIN(C_99)) )
        print_h( " restrict" );
      print_h( " volatile\n" );
    }

    print_h( "modifier:" );
    if ( C_LANG_IS(MIN(C_11)) )
      print_h( " _Atomic" );
    print_h( " short" );
    if ( C_LANG_IS(MIN(C_89)) )
      print_h( " signed" );
    print_h( " long unsigned" );
    if ( C_LANG_IS(MIN(C_89)) )
      print_h( " const" );
    if ( C_LANG_IS(MIN(C_99)) )
      print_h( " restrict" );
    if ( C_LANG_IS(MIN(C_89)) )
      print_h( " volatile" );
    print_h( "\n" );

    print_h( "name: a C identifier\n" );
    print_h( "store: auto extern register static" );
    if ( C_LANG_IS(MIN(C_11)) )
      print_h( " thread_local" );
    print_h( "\n" );
  }
  else {
    print_h( "  " );
    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( "[rvalue] " );
    print_h( "reference to <english>\n" );

    print_h( "  <store>* <modifier>* [<C\\+\\+-type>]\n" );

    print_h( "user-defined-english:\n" );
    print_h( "  conversion [operator] [of <scope-e> <name>]* returning <english>\n" );

    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( "  literal [([<args>])] [returning <english>]\n" );

    print_h( "args: a comma separated list of <english> or <name> as <english>\n" );

    print_h( "C\\+\\+-type: bool char" );
    if ( C_LANG_IS(MIN(CPP_20)) )
      print_h( " char8_t" );
    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( " char16_t char32_t" );
    print_h( " wchar_t int float double void\n" );

    print_h( "cv-qual: const volatile\n" );

    print_h( "fn-qual: const volatile" );
    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( " [rvalue] reference" );
    print_h( "\n" );

    print_h( "modifier: short long signed unsigned const volatile\n" );
    print_h( "name: a C\\+\\+ identifier; or <name>[::<name>]* or <name> [of <scope-e> <name>]*\n" );

    print_h( "scope-e: scope class struct union" );
    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( " [inline]" );
    print_h( " namespace\n" );

    print_h( "store: const" );
    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( "[" );
    if ( C_LANG_IS(MIN(CPP_20)) )
      print_h( "eval|" );
    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( "expr" );
    if ( C_LANG_IS(MIN(CPP_20)) )
      print_h( "|init" );
    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( "]" );
    print_h( " extern friend mutable static" );
    if ( C_LANG_IS(MIN(CPP_11)) )
      print_h( " thread_local" );
    print_h( "\n" );
    print_h( "       [pure] virtual\n" );
  }

  print_help_where();
}

static void print_help_where( void ) {
  print_h( "where: [] = 0 or 1; * = 0 or more; + = 1 or more; {} = one of; | = alternate\n" );
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
    print_help_commands();
    return;
  }
  if ( what == L_ENGLISH ) {
    print_help_english();
    return;
  }
  UNEXPECTED_STR_VALUE( what );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
