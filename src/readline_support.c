/*
**    cdecl -- C gibberish translator
**    src/readline_support.c
*/

// local
#include "config.h"
#include "readline.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdbool.h>
#include <string.h>

// local functions
static char*  command_completion( char const*, int );

////////// local functions ////////////////////////////////////////////////////

/**
 * TODO
 */
static char** attempt_completion( char const *text, int start, int end ) {
  assert( text );
  (void)end;

  char **matches = NULL;
  if ( start == 0 )
    matches = completion_matches( text, command_completion );
  return matches;
}

/**
 * TODO
 */
static char* command_completion( char const *text, int flag ) {
  char const *const COMMANDS[] = {
    "cast",
    "declare",
    "exit",
    "explain",
    "help",
    "quit",
    "set",
    NULL
  };

  static unsigned index;
  static size_t len;

  if ( !flag ) {
    index = 0;
    len = strlen( text );
  }

  for ( char const *command; (command = COMMANDS[ index ]); ++index ) {
    if ( strncmp( command, text, len ) == 0 )
      return check_strdup( command );
  } // for
  return NULL;
}

/**
 * TODO
 */
static char* keyword_completion( char const *text, int flag ) {
  static char const *const KEYWORDS[] = {

    // english
    "array",
//  "as",                               // too short
//  "block",
    "function",
//  "into",                             // special case (see below)
    "member",
//  "of",                               // too short
    "pointer",
    "reference",
    "returning",
//  "to",                               // too short

    // K&R C
    "auto",
    "char",
    "double",
    "extern",
    "float",
//  "int",                              // special case (see below)
    "long",
    "noalias",
    "register",
    "short",
    "static",
    "struct",
    "union",
    "unsigned",

    // C89
    "const",
    "enum",
    "signed",
    "void",
    "volatile",

    // C99
    "bool",
    "complex",
    "restrict",
    "wchar_t",

    // C11
    "_Noreturn",
    "_Thread_local",

    // C11 & C++11
    "char16_t",
    "char32_t",

    // C++
    "class",

    NULL
  };

  static char const *const OPTIONS[] = {
    "ansi",
    "cplusplus",
    "create",
  "nocreate",
    "options",
    "preansi",
    "prompt",
  "noprompt",
    NULL
  };

  static unsigned index;
  static size_t len;
  static bool into, set;

  if ( !flag ) {
    index = 0;
    len = strlen( text );
    /* completion works differently if the line begins with "set" */
    set = strncmp( rl_line_buffer, "set", 3 ) == 0;
    into = false;
  }

  if ( set ) {
    for ( char const *option; (option = OPTIONS[ index ]); ++index )
      if ( strncmp( text, option, len ) == 0 )
        return check_strdup( option );
  } else {
    /* handle "int" and "into" as special cases */
    if ( !into ) {
      into = true;
      if ( strncmp( text, "into", len ) && strncmp( text, "int", len ) == 0 )
        return check_strdup( "into" );
      if ( strncmp( text, "int", len ) != 0 )
        return keyword_completion( text, into );

      /* normally "int" and "into" would conflict with one another when
       * completing; cdecl tries to guess which one you wanted, and it
       * always guesses correctly
       */
      if ( strncmp( rl_line_buffer, "cast", 4 ) == 0 &&
           !strstr( rl_line_buffer, "into" ) ) {
        return check_strdup( "into" );
      }
      return check_strdup( "int" );
    }

    for ( char const *keyword; (keyword = KEYWORDS[ index ]); ++index )
      if ( strncmp( text, keyword, len ) == 0 )
        return check_strdup( keyword );
  }

  return NULL;
}

////////// extern functions ///////////////////////////////////////////////////

void readline_init( void ) {
  rl_attempted_completion_function = (CPPFunction*)attempt_completion;
  rl_completion_entry_function = (Function*)keyword_completion;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
