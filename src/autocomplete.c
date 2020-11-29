/*
**      cdecl -- C gibberish translator
**      src/autocomplete.c
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

#if !HAVE_RL_COMPLETION_FUNC_T
//
// CPPFunction was the original typedef in Readline prior to 4.2.  In 4.2, it
// was deprecated and replaced by rl_completion_func_t; in 6.3-5, CPPFunction
// was removed.
//
// To support Readlines older than 4.2 (such as that on macOS Mojave -- which
// is actually just a veneer on Editline), define rl_completion_func_t.
//
typedef CPPFunction rl_completion_func_t;
#endif /* HAVE_RL_COMPLETION_FUNC_T */

#if !HAVE_DECL_RL_COMPLETION_MATCHES
# define rl_completion_matches    completion_matches
#endif /* !HAVE_DECL_RL_COMPLETION_MATCHES */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * Subset of cdecl keywords that are commands.
 */
static c_lang_lit_t const CDECL_COMMANDS[] = {
  //
  // If this array is modified, also check ARGV_COMMANDS[] in is_command().
  //
  { LANG_ALL,         L_CAST        },
  { LANG_CPP_ALL,     L_CLASS       },
  { LANG_CPP_ALL,     L_CONST       },  // const cast ...
  { LANG_ALL,         L_DECLARE     },
  { LANG_ALL,         L_DEFINE      },
  { LANG_CPP_ALL,     L_DYNAMIC     },  // dynamic cast ...
  { LANG_ALL,         L_EXIT        },
  { LANG_ALL,         L_EXPLAIN     },
  { LANG_ALL,         L_HELP        },
  { LANG_CPP_ALL,     L_NAMESPACE   },
  { LANG_ALL,         L_QUIT        },
  { LANG_CPP_ALL,     L_REINTERPRET },  // reinterpret cast ...
  { LANG_ALL,         L_SET_COMMAND },
  { LANG_ALL,         L_SHOW        },
  { LANG_CPP_ALL,     L_STATIC      },  // static cast ...
  { LANG_ALL,         L_STRUCT      },
  { LANG_ALL,         L_TYPEDEF     },
  { LANG_ALL,         L_UNION       },
  { LANG_CPP_MIN(11), L_USING       },
  { LANG_NONE,        NULL          }
};

/**
 * Subset of cdecl and C/C++ keywords that are completable.
 */
static c_lang_lit_t const CDECL_KEYWORDS[] = {
  { LANG_ALL,               L_ARRAY               },
  //                        L_AS          // too short
  { LANG_MIN(C_11),         L_ATOMIC              },
  { LANG_ALL,               L_AUTO                },
  { LANG_ALL,               L_APPLE_BLOCK         },
  { LANG_ALL,               L_APPLE___BLOCK       },
  { LANG_MIN(C_99),         L_BOOL                },
  { LANG_CPP_MIN(11),       L_CARRIES_DEPENDENCY  },
  { LANG_ALL,               L_CAST                },
  { LANG_ALL,               L_CHAR                },
  { LANG_CPP_MIN(20),       L_CHAR8_T             },
  { LANG_CPP_MIN(11),       L_CHAR16_T            },
  { LANG_CPP_MIN(11),       L_CHAR32_T            },
  { LANG_CPP_ALL,           L_CLASS               },
  { LANG_ALL,               L_COMMANDS            },
  { LANG_MIN(C_99),         L_COMPLEX             },
  { LANG_MIN(C_89),         L_CONST               },
  { LANG_CPP_ALL,           L_CONST_CAST          },
  { LANG_CPP_MIN(20),       L_CONSTEVAL           },
  { LANG_CPP_MIN(11),       L_CONSTEXPR           },
  { LANG_CPP_MIN(20),       L_CONSTINIT           },
  { LANG_CPP_ALL,           L_CONSTRUCTOR         },
  { LANG_CPP_ALL,           L_CONVERSION          },
  { LANG_CPP_MIN(11),       L_DEFAULT             },
  { LANG_CPP_MIN(11),       L_DELETE              },
  { LANG_C_CPP_MIN(2X,14),  L_DEPRECATED          },
  { LANG_CPP_ALL,           L_DESTRUCTOR          },
  { LANG_ALL,               L_DOUBLE              },
  //                        L_DYNAMIC     // handled in CDECL_COMMANDS
  { LANG_CPP_MIN(11),       L_DYNAMIC_CAST      },
  { LANG_ALL,               L_ENGLISH           },
  { LANG_MIN(C_89),         L_ENUM              },
  { LANG_CPP_ALL,           L_EXPLICIT          },
  { LANG_CPP_MIN(20),       L_EXPORT            },
  { LANG_ALL,               L_EXTERN            },
  { LANG_CPP_ALL,           L_FALSE             },
  { LANG_CPP_MIN(11),       L_FINAL             },
  { LANG_ALL,               L_FLOAT             },
  { LANG_CPP_ALL,           L_FRIEND            },
  { LANG_ALL,               L_FUNCTION          },
  { LANG_MIN(C_99),         L_IMAGINARY         },
  { LANG_MIN(C_99),         L_INLINE            },
  { LANG_ALL,               L_INT               },
  //                        L_INTO        // special case (see below)
  { LANG_C_MIN(99),         L_LENGTH            },
  { LANG_CPP_MIN(11),       L_LITERAL           },
  { LANG_ALL,               L_LONG              },
  { LANG_C_CPP_MIN(2X,17),  L_MAYBE_UNUSED      },
  { LANG_CPP_ALL,           L_MEMBER            },
  { LANG_CPP_ALL,           L_MUTABLE           },
  //                        L_NAMESPACE   // handled in CDECL_COMMANDS
  { LANG_C_CPP_MIN(2X,17),  L_NODISCARD         },
  { LANG_CPP_MIN(11),       L_NOEXCEPT          },
  { LANG_CPP_ALL,           L_NON_MEMBER        },
  { LANG_MIN(C_11),         L_NORETURN          },
  { LANG_CPP_MIN(20),       L_NO_UNIQUE_ADDRESS },
  //                        L_OF          // too short
  { LANG_CPP_ALL,           L_OPERATOR          },
  { LANG_CPP_MIN(11),       L_OVERRIDE          },
  { LANG_ALL,               L_POINTER           },
  { LANG_ALL,               L_PREDEFINED        },
  { LANG_CPP_ALL,           L_PURE              },
  { LANG_CPP_ALL,           L_REFERENCE         },
  { LANG_MAX(CPP_14),       L_REGISTER          },
  //                        L_REINTERPRET // handled in CDECL_COMMANDS
  { LANG_CPP_ALL,           L_REINTERPRET_CAST  },
  { LANG_C_MIN(99),         L_RESTRICT          },
  { LANG_ALL,               L_RETURNING         },
  { LANG_CPP_MIN(11),       L_RVALUE            },
  { LANG_CPP_ALL,           L_SCOPE             },
  { LANG_ALL,               L_SHORT             },
  { LANG_MIN(C_89),         L_SIGNED            },
  { LANG_ALL,               L_STATIC            },
  { LANG_CPP_ALL,           L_STATIC_CAST       },
  { LANG_ALL,               L_STRUCT            },
  //                        L_TO          // too short
  { LANG_C_CPP_MIN(11,11),  L_THREAD_LOCAL      },
  { LANG_CPP_MAX(17),       L_THROW             },
  { LANG_CPP_ALL,           L_TRUE              },
  //                        L_TYPEDEF     // handled in CDECL_COMMANDS
  { LANG_ALL,               L_UNION             },
  { LANG_ALL,               L_UNSIGNED          },
  { LANG_CPP_ALL,           L_USER_DEFINED      },
  //                        L_USING       // handled in CDECL_COMMANDS
  { LANG_C_MIN(99),         L_VARIABLE          },
  { LANG_CPP_ALL,           L_VIRTUAL           },
  { LANG_MIN(C_89),         L_VOID              },
  { LANG_MIN(C_89),         L_VOLATILE          },
  { LANG_MIN(C_95),         L_WCHAR_T           },

  // Embedded C extensions
  { LANG_C_99,              L_EMC_ACCUM         },
  { LANG_C_99,              L_EMC_FRACT         },
  { LANG_C_99,              L_EMC_SATURATED     },

  // Unified Parallel C extensions
  { LANG_C_99,              L_UPC_RELAXED       },
  { LANG_C_99,              L_UPC_SHARED        },
  { LANG_C_99,              L_UPC_STRICT        },

  { LANG_NONE,              NULL                }
};

/**
 * cdecl options.
 */
static char const *const SET_OPTIONS[] = {
  //
  // If this array is modified, also check C_LANG[] in c_lanc.c and
  // SET_OPTIONS[] in set.c.
  //
   "alt-tokens",
 "noalt-tokens",
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
#ifdef YYDEBUG
   "bison-debug",
 "nobison-debug",
#endif /* YYDEBUG */
#ifdef ENABLE_CDECL_DEBUG
   "debug",
 "nodebug",
#endif /* ENABLE_CDECL_DEBUG */
   "east-const",
 "noeast-const",
   "explain-by-default",
 "noexplain-by-default",
   "explicit-int",
 "noexplicit-int",
#ifdef ENABLE_FLEX_DEBUG
   "flex-debug",
 "noflex-debug",
#endif /* ENABLE_FLEX_DEBUG */
 "nographs",
 "digraphs",
"trigraphs",
   "knr"
   "options",
   "prompt",
 "noprompt",
   "semicolon",
 "nosemicolon",
   NULL
};

// local functions
PJL_WARN_UNUSED_RESULT
static char*  command_generator( char const*, int );

PJL_WARN_UNUSED_RESULT
static char* find_keyword( c_lang_lit_t const[], char const*, size_t, size_t* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Attempts command completion for `readline()`.
 *
 * @param text The text read (so far) to match against.
 * @param start The starting character position of \a text.
 * @param end The ending character position of \a text.
 * @return Returns an array of C strings of possible matches.
 */
PJL_WARN_UNUSED_RESULT
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
PJL_WARN_UNUSED_RESULT
static char* command_generator( char const *text, int state ) {
  assert( text != NULL );

  static size_t index;
  static size_t text_len;

  if ( state == 0 ) {                   // new word? reset
    index = 0;
    text_len = strlen( text );
  }

  return find_keyword( CDECL_COMMANDS, text, text_len, &index );
}

/**
 * Attempts to find the keyword (partially) matching \a text.
 *
 * @param keywords The c_lang_lit_t array to search through.  The last element
 * _must_ have a `literal` of NULL.
 * @param text The text to search for.
 * @param text_len The length of text to (partially) match.
 * @param index A pointer to the current index into \a keywords to update.
 * @return Returns a copy of the keyword or null if not found.
 */
PJL_WARN_UNUSED_RESULT
static char* find_keyword( c_lang_lit_t const keywords[const], char const *text,
                           size_t text_len, size_t *index ) {
  assert( text != NULL );
  assert( index != NULL );

  for ( c_lang_lit_t const *ll; (ll = keywords + *index)->literal != NULL; ) {
    ++*index;
    if ( (ll->lang_ids & opt_lang) == LANG_NONE )
      continue;
    if ( strncmp( text, ll->literal, text_len ) == 0 )
      return check_strdup( ll->literal );
  } // for
  return NULL;
}

/**
 * Checks whether the current line being read is a particular cdecl command.
 *
 * @param command The command to check for.
 * @return Returns `true` only if it is.
 */
PJL_WARN_UNUSED_RESULT
static bool is_command( char const *command ) {
  assert( command != NULL );
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
PJL_WARN_UNUSED_RESULT
static char* keyword_completion( char const *text, int state ) {
  assert( text != NULL );

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
      for ( c_lang_lit_t const *ll = CDECL_COMMANDS; ll->literal != NULL;
            ++ll ) {
        if ( (ll->lang_ids & opt_lang) != LANG_NONE &&
             is_command( ll->literal ) ) {
          command = ll->literal;
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
    for ( char const *option; (option = SET_OPTIONS[ index ]) != NULL; ) {
      ++index;
      if ( strncmp( text, option, text_len ) == 0 )
        return check_strdup( option );
    } // for
    return NULL;
  }

  //
  // Otherwise, just attempt to match any keyword.
  //
  return find_keyword( CDECL_KEYWORDS, text, text_len, &index );
}

////////// extern functions ///////////////////////////////////////////////////

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
