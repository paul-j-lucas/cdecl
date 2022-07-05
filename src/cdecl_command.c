/*
**      cdecl -- C gibberish translator
**      src/cdecl_command.c
**
**      Copyright (C) 2017-2022  Paul J. Lucas, et al.
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
 * Declares data and functions for cdecl commands.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cdecl_command.h"
#include "c_lang.h"
#include "literals.h"
#include "util.h"

// standard
#include <assert.h>
#include <string.h>

// shorthands
#define FIRST_ARG                 CDECL_COMMAND_FIRST_ARG
#define LANG_ONLY                 CDECL_COMMAND_LANG_ONLY
#define PROG_NAME                 CDECL_COMMAND_PROG_NAME

///////////////////////////////////////////////////////////////////////////////

/**
 * Cdecl commands.
 */
static cdecl_command_t const CDECL_COMMANDS[] = {
  { L_CAST,                   PROG_NAME,  LANG_ANY                },
  { L_CLASS,                  FIRST_ARG,  LANG_CPP_ANY            },
  { L_CONST /* cast */,       FIRST_ARG,  LANG_CPP_ANY            },
  { L_DECLARE,                PROG_NAME,  LANG_ANY                },
  { L_DEFINE,                 FIRST_ARG,  LANG_ANY                },
  { L_DYNAMIC /* cast */,     FIRST_ARG,  LANG_CPP_ANY            },
  { L_ENUM,                   FIRST_ARG,  LANG_ENUM               },
  { L_EXIT,                   LANG_ONLY,  LANG_ANY                },
  { L_EXPLAIN,                PROG_NAME,  LANG_ANY                },
  { L_HELP,                   FIRST_ARG,  LANG_ANY                },
  { L_INCLUDE,                FIRST_ARG,  LANG_ANY                },
  { L_INLINE /* namespace */, FIRST_ARG,  LANG_INLINE_NAMESPACE   },
  { L_NAMESPACE,              FIRST_ARG,  LANG_CPP_ANY            },
  { L_QUIT,                   LANG_ONLY,  LANG_ANY                },
  { L_REINTERPRET /* cast */, FIRST_ARG,  LANG_CPP_ANY            },
  { L_SET_COMMAND,            FIRST_ARG,  LANG_ANY                },
  { L_SHOW,                   FIRST_ARG,  LANG_ANY                },
  { L_STATIC /* cast */,      FIRST_ARG,  LANG_CPP_ANY            },
  { L_STRUCT,                 FIRST_ARG,  LANG_ANY                },
  { L_TYPEDEF,                FIRST_ARG,  LANG_ANY                },
  { L_UNION,                  FIRST_ARG,  LANG_ANY                },
  { L_USING,                  FIRST_ARG,  LANG_USING_DECLARATION  },
  { NULL,                     0,          LANG_NONE               },
};

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether \a s starts with a token.  If so, the character following the
 * token in \a s also _must not_ be an identifier character, i.e., whitespace,
 * punctuation, or the null byte.
 *
 * @param s The null-terminated string to check.
 * @param token The token to check against.
 * @param token_len The length of \a token.
 * @return Returns `true` only if \a s starts with \a token.
 */
PJL_WARN_UNUSED_RESULT
static bool starts_with_token( char const *s, char const *token,
                               size_t token_len ) {
  assert( s != NULL );
  assert( token != NULL );
  return  strncmp( s, token, token_len ) == 0 &&
          !is_ident( token[ token_len ] );
}

////////// extern functions ///////////////////////////////////////////////////

cdecl_command_t const* cdecl_command_find( char const *s ) {
  assert( s != NULL );
  SKIP_WS( s );

  FOREACH_CDECL_COMMAND( command ) {
    size_t const literal_len = strlen( command->literal );
    if ( !starts_with_token( s, command->literal, literal_len ) )
      continue;
    if ( command->literal == L_CONST || command->literal == L_STATIC ) {
      //
      // When in explain-by-default mode, a special case has to be made for
      // const and static since explain is implied only when NOT followed by
      // "cast":
      //
      //      const int *p                      // Implies explain.
      //      const cast p into pointer to int  // Does NOT imply explain.
      //
      char const *p = s + literal_len;
      if ( !isspace( *p ) )
        break;
      SKIP_WS( p );
      if ( !starts_with_token( p, L_CAST, 4 ) )
        break;
      p += 4;
      if ( !isspace( *p ) )
        break;
    }
    return command;
  } // for
  return NULL;
}

cdecl_command_t const* cdecl_command_next( cdecl_command_t const *command ) {
  return command == NULL ?
    CDECL_COMMANDS :
    (++command)->literal == NULL ? NULL : command;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
