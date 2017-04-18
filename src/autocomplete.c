/*
**      cdecl -- C gibberish translator
**      src/autocomplete.c
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
 * Defines functions for implementing command-line autocompletion.
 */

// local
#include "config.h"                     /* must go first */
#include "literals.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <readline/readline.h>          /* must go last */

///////////////////////////////////////////////////////////////////////////////

// subset of cdecl keywords that are commands
static char const *const CDECL_COMMANDS[] = {
  L_CAST,
  L_DECLARE,
  L_EXIT,
  L_EXPLAIN,
  L_HELP,
  L_QUIT,
  L_SET,
  NULL
};

// subset of cdecl keywords that are completable
static char const *const CDECL_KEYWORDS[] = {
  L_ARRAY,
//L_AS,                                 // too short
  L_AUTO,
  L_BLOCK,                              // Apple: English for '^'
  L___BLOCK,                            // Apple: storage class
  L_BOOL,
  L_CHAR,
  L_CHAR16_T,
  L_CHAR32_T,
  L_CLASS,
  L_COMPLEX,
  L_CONST,
  L_CONSTEXPR,
  L_DOUBLE,
  L_ENUM,
  L_EXTERN,
  L_FINAL,
  L_FLOAT,
  L_FRIEND,
  L_FUNCTION,
  L_INLINE,
//L_INT,                                // special case (see below)
//L_INTO,                               // special case (see below)
  L_LONG,
  L_MEMBER,
  L_MUTABLE,
  L_NORETURN,
//L_OF,                                 // too short
  L_OVERRIDE,
  L_POINTER,
  L_PURE,
  L_REFERENCE,
  L_REGISTER,
  L_RESTRICT,
  L_RETURNING,
  L_RVALUE,
  L_SHORT,
  L_SIGNED,
  L_SIZE_T,
  L_STATIC,
  L_STRUCT,
//L_TO,                                 // too short
  L_THREAD_LOCAL,
  L_TYPEDEF,
  L_UNION,
  L_UNSIGNED,
  L_VIRTUAL,
  L_VOID,
  L_VOLATILE,
  L_WCHAR_T,
  NULL
};

// cdecl options
static char const *const CDECL_OPTIONS[] = {
  "c89",
  "c95",
  "c99",
  "c11",
//"c++",                                // too short
  "c++98",
  "c++03",
  "c++11",
  "create",
"nocreate",
  "knr"
  "options",
  "prompt",
"noprompt",
  NULL
};

// local functions
static char*  command_generator( char const*, int );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks whether the current line being read is a particular cdecl command.
 *
 * @param command The command to check for.
 * @return Returns \c true only if it is.
 */
static inline bool is_command( char const *command ) {
  return strncmp( rl_line_buffer, command, strlen( command ) ) == 0;
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Attempts command completion for readline().
 *
 * @param text The text, so far, to match.
 * @param start The starting character position of \a text.
 * @param end The ending character position of \a text.
 * @return Returns an array of C strings of possible matches.
 */
static char** attempt_completion( char const *text, int start, int end ) {
  assert( text != NULL );
  (void)end;
  //
  // If the word is at the start of the line (start == 0), then attempt to
  // complete only cdecl commands and not all keywords.
  //
  return start == 0 ? rl_completion_matches( text, command_generator ) : NULL;
}

/**
 * Attempts to match a cdecl command.
 *
 * @param text The text read (so far) to match against.
 * @param state If 0, restart matching from the beginning; if non-zero,
 * continue to next match, if any.
 */
static char* command_generator( char const *text, int state ) {
  static unsigned index;
  static size_t text_len;

  if ( !state ) {                       // new word? reset
    index = 0;
    text_len = strlen( text );
  }

  for ( char const *command; (command = CDECL_COMMANDS[ index ]); ) {
    ++index;
    if ( strncmp( text, command, text_len ) == 0 )
      return check_strdup( command );
  } // for
  return NULL;
}

/**
 * Attempts to match a cdecl keyword (that is not a command).
 *
 * @param text The text read (so far) to match against.
 * @param state If 0, restart matching from the beginning; if non-zero,
 * continue to next match, if any.
 */
static char* keyword_completion( char const *text, int state ) {
  static unsigned index;
  static size_t text_len;

  static bool is_into;
  static bool is_set_command;

  if ( !state ) {                       // new word? reset
    index = 0;
    text_len = strlen( text );
    is_into = false;
    //
    // If we're doing the "set" command, complete options, not keywords.
    //
    is_set_command = is_command( L_SET );
  }

  if ( is_set_command ) {
    for ( char const *option; (option = CDECL_OPTIONS[ index ]); ) {
      ++index;
      if ( strncmp( text, option, text_len ) == 0 )
        return check_strdup( option );
    } // for
    return NULL;
  }

  // handle "int" and "into" as special cases
  if ( !is_into ) {
    is_into = true;
    if ( strncmp( text, L_INTO, text_len ) == 0 &&
         strncmp( text, L_INT, text_len ) != 0 ) {
      return check_strdup( L_INTO );
    }
    if ( strncmp( text, L_INT, text_len ) != 0 )
      return keyword_completion( text, is_into );

    /* normally "int" and "into" would conflict with one another when
      * completing; cdecl tries to guess which one you wanted, and it
      * always guesses correctly
      */
    return check_strdup(
      is_command( L_CAST ) && !strstr( rl_line_buffer, L_INTO ) ?
        L_INTO : L_INT
    );
  }

  for ( char const *keyword; (keyword = CDECL_KEYWORDS[ index ]); ) {
    ++index;
    if ( strncmp( text, keyword, text_len ) == 0 )
      return check_strdup( keyword );
  } // for
  return NULL;
}

////////// extern functions ///////////////////////////////////////////////////

void readline_init( void ) {
  // allow conditional ~/.inputrc parsing
  rl_readline_name = CONST_CAST( char*, PACKAGE );

  rl_attempted_completion_function = (CPPFunction*)attempt_completion;
  rl_completion_entry_function = (void*)keyword_completion;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
