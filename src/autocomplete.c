/*
**      cdecl -- C gibberish translator
**      src/autocomplete.c
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
 * Defines functions for implementing command-line autocompletion.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "c_lang.h"
#include "literals.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <readline/readline.h>          /* must go last */

#if !HAVE_DECL_RL_COMPLETION_MATCHES
# define rl_completion_matches    completion_matches
#endif /* !HAVE_DECL_RL_COMPLETION_MATCHES */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * C/C++ autocompletion keyword.
 */
struct ac_keyword {
  char const *keyword;                  ///< The keyword literal.
  c_lang_id_t lang_ids;                 ///< Language(s) OK in.
};
typedef struct ac_keyword ac_keyword_t;

/**
 * Subset of cdecl keywords that are commands.
 */
static ac_keyword_t const CDECL_COMMANDS[] = {
  { L_CAST,         LANG_ALL          },
  { L_CONST,        LANG_CPP_ALL      },// const cast ...
  { L_DECLARE,      LANG_ALL          },
  { L_DEFINE,       LANG_ALL          },
  { L_DYNAMIC,      LANG_CPP_ALL      },// dynamic cast ...
  { L_EXIT,         LANG_ALL          },
  { L_EXPLAIN,      LANG_ALL          },
  { L_HELP,         LANG_ALL          },
  { L_NAMESPACE,    LANG_CPP_ALL      },
  { L_QUIT,         LANG_ALL          },
  { L_REINTERPRET,  LANG_CPP_ALL      },// reinterpret cast ...
  { L_SET_COMMAND,  LANG_ALL          },
  { L_SHOW,         LANG_ALL          },
  { L_STATIC,       LANG_CPP_ALL      },// static cast ...
  { L_TYPEDEF,      LANG_ALL          },
  { L_USING,        LANG_MIN(CPP_11)  },
  { NULL,           LANG_NONE         },
};

/**
 * Subset of cdecl keywords that are completable.
 */
static ac_keyword_t const CDECL_KEYWORDS[] = {
  { L_ALT_TOKENS,         LANG_ALL                          },
  { L_ARRAY,              LANG_ALL                          },
//  L_AS,                               // too short
  { L_ATOMIC,             LANG_MIN(C_11)                    },
  { L_AUTO,               LANG_ALL                          },
  { L_BLOCK,              LANG_ALL                          },
  { L___BLOCK,            LANG_ALL                          },
  { L_BOOL,               LANG_MIN(C_99)                    },
  { L_CARRIES_DEPENDENCY, LANG_MIN(CPP_11)                  },
  { L_CAST,               LANG_ALL                          },
  { L_CHAR,               LANG_ALL                          },
  { L_CHAR8_T,            LANG_MIN(CPP_20)                  },
  { L_CHAR16_T,           LANG_MIN(CPP_11)                  },
  { L_CHAR32_T,           LANG_MIN(CPP_11)                  },
  { L_CLASS,              LANG_CPP_ALL                      },
  { L_COMMANDS,           LANG_ALL                          },
  { L_COMPLEX,            LANG_MIN(C_99)                    },
  { L_CONST,              LANG_MIN(C_89)                    },
  { L_CONST_CAST,         LANG_CPP_ALL                      },
  { L_CONSTEVAL,          LANG_MIN(CPP_20)                  },
  { L_CONSTEXPR,          LANG_MIN(CPP_11)                  },
  { L_CONSTRUCTOR,        LANG_CPP_ALL                      },
  { L_CONVERSION,         LANG_CPP_ALL                      },
  { L_DEFAULT,            LANG_MIN(CPP_11)                  },
  { L_DELETE,             LANG_MIN(CPP_11)                  },
  { L_DEPRECATED,         LANG_MIN(CPP_14)                  },
  { L_DESTRUCTOR,         LANG_CPP_ALL                      },
  { L_DOUBLE,             LANG_ALL                          },
//  L_DYNAMIC,                          // handled in CDECL_COMMANDS
  { L_DYNAMIC_CAST,       LANG_MIN(CPP_11)                  },
  { L_ENGLISH,            LANG_ALL                          },
  { L_ENUM,               LANG_MIN(C_89)                    },
  { L_EXPLICIT,           LANG_CPP_ALL                      },
  { L_EXTERN,             LANG_ALL                          },
  { L_FALSE,              LANG_CPP_ALL                      },
  { L_FINAL,              LANG_MIN(CPP_11)                  },
  { L_FLOAT,              LANG_ALL                          },
  { L_FRIEND,             LANG_CPP_ALL                      },
  { L_FUNCTION,           LANG_ALL                          },
  { L_IMAGINARY,          LANG_MIN(C_99)                    },
  { L_INLINE,             LANG_MIN(C_99)                    },
  { L_INT,                LANG_ALL                          },
//{ L_INTO,                             // special case (see below)
  { L_LENGTH,             LANG_MIN(C_99) & ~LANG_CPP_ALL    },
  { L_LITERAL,            LANG_MIN(CPP_11)                  },
  { L_LONG,               LANG_ALL                          },
  { L_MAYBE_UNUSED,       LANG_MIN(CPP_11)                  },
  { L_MEMBER,             LANG_CPP_ALL                      },
  { L_MUTABLE,            LANG_CPP_ALL                      },
//{ L_NAMESPACE,                        // handled in CDECL_COMMANDS
  { L_NODISCARD,          LANG_MIN(CPP_17)                  },
  { L_NOEXCEPT,           LANG_MIN(CPP_11)                  },
  { L_NON_MEMBER,         LANG_CPP_ALL                      },
  { L_NORETURN,           LANG_MIN(C_11)                    },
//{ L_OF,                               // too short
  { L_OPERATOR,           LANG_CPP_ALL                      },
  { L_OVERRIDE,           LANG_MIN(CPP_11)                  },
  { L_POINTER,            LANG_ALL                          },
  { L_PREDEFINED,         LANG_ALL                          },
  { L_PURE,               LANG_CPP_ALL                      },
  { L_REFERENCE,          LANG_CPP_ALL                      },
  { L_REGISTER,           LANG_ALL                          },
//  L_REINTERPRET,                      // handled in CDECL_COMMANDS
  { L_REINTERPRET_CAST,   LANG_CPP_ALL                      },
  { L_RESTRICT,           LANG_MIN(C_89) & ~LANG_CPP_ALL    },
  { L_RETURNING,          LANG_ALL                          },
  { L_RVALUE,             LANG_MIN(CPP_11)                  },
  { L_SCOPE,              LANG_CPP_ALL                      },
  { L_SHORT,              LANG_ALL                          },
  { L_SIGNED,             LANG_MIN(C_89)                    },
  { L_STATIC,             LANG_ALL                          },
  { L_STATIC_CAST,        LANG_CPP_ALL                      },
  { L_STRUCT,             LANG_ALL                          },
//  L_TO,                               // too short
  { L_THREAD_LOCAL,       LANG_C_MIN(11) | LANG_MIN(CPP_11) },
  { L_THROW,              LANG_CPP_ALL                      },
  { L_TRUE,               LANG_CPP_ALL                      },
//{ L_TYPEDEF,                          // handled in CDECL_COMMANDS
  { L_UNION,              LANG_ALL                          },
  { L_UNSIGNED,           LANG_ALL                          },
  { L_USER_DEFINED,       LANG_CPP_ALL                      },
//{ L_USING,                            // handled in CDECL_COMMANDS
  { L_VARIABLE,           LANG_MIN(C_99) & ~LANG_CPP_ALL    },
  { L_VIRTUAL,            LANG_CPP_ALL                      },
  { L_VOID,               LANG_MIN(C_89)                    },
  { L_VOLATILE,           LANG_MIN(C_89)                    },
  { L_WCHAR_T,            LANG_MIN(C_95)                    },
  { NULL,                 LANG_NONE                         }
};

/**
 * cdecl options.
 */
static char const *const CDECL_OPTIONS[] = {
   "c89",
   "c95",
   "c99",
   "c11",
   "c18",
// "c++",                               // too short
   "c++98",
   "c++03",
   "c++11",
   "c++14",
   "c++17",
   "c++20",
   "create",
 "nocreate",
#ifdef ENABLE_CDECL_DEBUG
   "debug",
 "nodebug",
#endif /* ENABLE_CDECL_DEBUG */
 "nographs",
 "digraphs",
 "explain-by-default",
"trigraphs",
   "knr"
   "options",
   "prompt",
 "noprompt",
   "semicolon",
 "nosemicolon",
#ifdef YYDEBUG
   "yydebug",
 "noyydebug",
#endif /* YYDEBUG */
   NULL
};

// local functions
static char*  command_generator( char const*, int );

////////// local functions ////////////////////////////////////////////////////

/**
 * Attempts to find the `ac_keyword` (partially) matching the given text.
 *
 * @param keywords The array of keywords to search through.
 * @param text The text to search for.
 * @param text_len The length of text to (partially) match.
 * @param index A pointer to the current index into \a keywords to update.
 * @return Returns a copy of the keyword or null if not found.
 */
static char* ac_keyword_find( ac_keyword_t const keywords[], char const *text,
                              size_t text_len, size_t *index ) {
  for ( ac_keyword_t const *k; (k = keywords + *index)->keyword != NULL; ) {
    ++*index;
    if ( (k->lang_ids & opt_lang) == LANG_NONE )
      continue;
    if ( strncmp( text, k->keyword, text_len ) == 0 )
      return check_strdup( k->keyword );
  } // for
  return NULL;
}

/**
 * Attempts command completion for `readline()`.
 *
 * @param text The text read (so far) to match against.
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
 * @return Returns a copy of the command or null if not found.
 */
static char* command_generator( char const *text, int state ) {
  static size_t index;
  static size_t text_len;

  if ( state == 0 ) {                   // new word? reset
    index = 0;
    text_len = strlen( text );
  }

  return ac_keyword_find( CDECL_COMMANDS, text, text_len, &index );
}

/**
 * Checks whether the current line being read is a particular cdecl command.
 *
 * @param command The command to check for.
 * @return Returns `true` only if it is.
 */
static bool is_command( char const *command ) {
  size_t const command_len = strlen( command );
  if ( command_len > (size_t)rl_end )   // more chars than in rl_line_buffer?
    return false;
  return strncmp( rl_line_buffer, command, command_len ) == 0;
}

/**
 * Attempts to match a cdecl keyword (that is not a command).
 *
 * @param text The text read (so far) to match against.
 * @param state If 0, restart matching from the beginning; if non-zero,
 * continue to next match, if any.
 * @return Returns a copy of the keyword or null if none.
 */
static char* keyword_completion( char const *text, int state ) {
  static char const  *command;          // current command
  static size_t       index;
  static bool         no_more_matches;
  static size_t       text_len;

  if ( state == 0 ) {                   // new word? reset
    no_more_matches = false;
    index = 0;
    text_len = strlen( text );

    //
    // Special case: the "cast" command is begun by either "cast" or, if C++11
    // or later, any one of "const", "dynamic", "static", or "reinterpret" for
    // "const cast ...", etc.
    //
    command = is_command( L_CAST ) ||
      ( C_LANG_IS_CPP() && (
        is_command( L_CONST       ) ||
        is_command( L_DYNAMIC     ) ||
        is_command( L_STATIC      ) ||
        is_command( L_REINTERPRET ) ) ) ? L_CAST : NULL;

    //
    // If it's not the "cast" command, see if it's any other command.
    //
    if ( command == NULL ) {
      for ( ac_keyword_t const *k = CDECL_COMMANDS; k->keyword != NULL; ++k ) {
        if ( (k->lang_ids & opt_lang) != LANG_NONE &&
             is_command( k->keyword ) ) {
          command = k->keyword;
          break;
        }
      } // for
    }
  }

  if ( command == NULL || no_more_matches ) {
    //
    // We haven't at least matched a command yet, so don't match any other
    // keywords.
    //
    return NULL;
  }

  //
  // Special case: if it's the "cast" command, the text partially matches
  // "into", and the user hasn't typed "into" yet, complete as "into".
  //
  if ( strcmp( command, L_CAST ) == 0 &&
       strncmp( text, L_INTO, text_len ) == 0 &&
       strstr( rl_line_buffer, L_INTO ) == NULL ) {
    no_more_matches = true;             // unambiguously match "into"
    return check_strdup( L_INTO );
  }

  //
  // Special case: if it's the "set" command, complete options, not keywords.
  //
  if ( strcmp( command, L_SET_COMMAND ) == 0 ) {
    for ( char const *option; (option = CDECL_OPTIONS[ index ]) != NULL; ) {
      ++index;
      if ( strncmp( text, option, text_len ) == 0 )
        return check_strdup( option );
    } // for
    return NULL;
  }

  //
  // Otherwise, just attempt to match any keyword.
  //
  return ac_keyword_find( CDECL_KEYWORDS, text, text_len, &index );
}

////////// extern functions ///////////////////////////////////////////////////

#if !HAVE_DECL_RL_COMPLETION_FUNC_T
//
// CPPFunction was the original typedef in Readline prior to 4.2.  In 4.2, it
// was deprecated and replaced by rl_completion_func_t; in 6.3-5, CPPFunction
// was removed.
//
// To support Readlines older than 4.2 (such as that on macOS Mojave -- which
// is actually just a veneer on Editline), define rl_completion_func_t.
//
typedef CPPFunction rl_completion_func_t;
#endif /* HAVE_DECL_RL_COMPLETION_FUNC_T */

/**
 * Initializes readline.
 */
void readline_init( void ) {
  // allow conditional ~/.inputrc parsing
  rl_readline_name = CONST_CAST( char*, PACKAGE );

  rl_attempted_completion_function = (rl_completion_func_t*)attempt_completion;
  rl_completion_entry_function = (void*)keyword_completion;
  rl_instream = fin;
  rl_outstream = fout;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
