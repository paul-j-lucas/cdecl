/*
**      cdecl -- C gibberish translator
**      src/cdecl_command.c
**
**      Copyright (C) 2017-2024  Paul J. Lucas, et al.
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
 * Declares data and functions for **cdecl** commands.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cdecl_command.h"
#include "c_lang.h"
#include "literals.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL */
#include <string.h>

/// @cond DOXYGEN_IGNORE

// shorthands
#define FIRST_ARG                 CDECL_COMMAND_FIRST_ARG
#define LANG_ONLY                 CDECL_COMMAND_LANG_ONLY
#define PROG_NAME                 CDECL_COMMAND_PROG_NAME

#define LANG_SAME(L)              LANG_##L, AC_LANG(L)
#define LANG_DIFF(L,ACL)          LANG_##L, AC_LANG(ACL)

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * **Cdecl** commands.
 */
static cdecl_command_t const CDECL_COMMANDS[] = {
  //
  // The `exit` command shares the same 2-character prefix of `ex` with
  // `explain` and it's far more likely that a user wants to autocomplete
  // `explain` than `exit`.  Therefore, make `exit` not autocompletable so
  // `explain` autocompletes after typing `ex` rather than `exp`.  Note that
  // the user can alternatively autocomplete `quit` (or just type `q`).
  //
  // This _must_ be in sorted order.
  //
  { L_PRE_P_define,         LANG_ONLY,  LANG_SAME(ANY)                },
  { L_PRE_P_include,        LANG_ONLY,  LANG_SAME(ANY)                },
  { L_PRE_P_undef,          LANG_ONLY,  LANG_SAME(ANY)                },
  { L_cast,                 FIRST_ARG,  LANG_SAME(ANY)                },
  { L_class,                FIRST_ARG,  LANG_SAME(class)              },
  { L_const /*cast*/,       FIRST_ARG,  LANG_SAME(NEW_STYLE_CASTS)    },
  { L_declare,              FIRST_ARG,  LANG_SAME(ANY)                },
  { L_define,               FIRST_ARG,  LANG_SAME(ANY)                },
  { L_dynamic /*cast*/,     FIRST_ARG,  LANG_SAME(NEW_STYLE_CASTS)    },
  { L_enum,                 FIRST_ARG,  LANG_SAME(enum)               },
  { L_exit,                 LANG_ONLY,  LANG_DIFF(ANY,NONE)           },
  { L_expand,               FIRST_ARG,  LANG_SAME(ANY)                },
  { L_explain,              PROG_NAME,  LANG_SAME(ANY)                },
  { L_help,                 FIRST_ARG,  LANG_SAME(ANY)                },
  { L_include,              FIRST_ARG,  LANG_SAME(ANY)                },
  { L_inline,               FIRST_ARG,  LANG_SAME(inline_namespace)   },
  { L_namespace,            FIRST_ARG,  LANG_SAME(namespace)          },
  { L_quit,                 LANG_ONLY,  LANG_SAME(ANY)                },
  { L_reinterpret /*cast*/, FIRST_ARG,  LANG_SAME(NEW_STYLE_CASTS)    },
  { L_set,                  FIRST_ARG,  LANG_SAME(ANY)                },
  { L_show,                 FIRST_ARG,  LANG_SAME(ANY)                },
  { L_static /*cast*/,      FIRST_ARG,  LANG_SAME(NEW_STYLE_CASTS)    },
  { L_struct,               FIRST_ARG,  LANG_SAME(ANY)                },
  { L_typedef,              FIRST_ARG,  LANG_SAME(ANY)                },
  { L_union,                FIRST_ARG,  LANG_SAME(ANY)                },
  { L_using,                FIRST_ARG,  LANG_SAME(using_DECLS)        },
  { NULL,                   0,          LANG_SAME(NONE)               },
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
NODISCARD
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
    size_t literal_len = strlen( command->literal );
    if ( !starts_with_token( s, command->literal, literal_len ) )
      continue;
    if ( !opt_infer_command )
      return command;
    if ( command->literal != L_const && command->literal != L_static )
      return command;

    //
    // When in infer-command mode, a special case has to be made for "const"
    // and "static" since "explain" is implied only when NOT followed by
    // "cast":
    //
    //      const int *p                            // Implies explain.
    //      const cast p into pointer to int        // Does NOT imply explain.
    //
    if ( strncmp( s, "constant", STRLITLEN( "constant" ) ) == 0 ) {
      //
      // An even more special case has to be made for "constant cast":
      //
      //      constant cast p into pointer to int   // Does NOT imply explain.
      //
      literal_len += STRLITLEN( "ant" );
    }

    char const *p = s + literal_len;
    if ( !isspace( *p ) )
      break;
    SKIP_WS( p );
    if ( !starts_with_token( p, L_cast, 4 ) )
      break;
    p += 4;
    if ( !isspace( *p ) )
      break;

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
