/*
**      cdecl -- C gibberish translator
**      src/c_lang.c
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
 * Defines functions for C/C++ language versions.
 */

// local
#include "cdecl.h"                      /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_LANG_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_lang.h"
#include "options.h"
#include "prompt.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <string.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * A mapping between a language name and its corresponding `c_lang_id_t`.
 */
struct c_lang {
  char const   *name;                   ///< Language name.
  c_lang_id_t   lang_id;                ///< Language bit.
};
typedef struct c_lang c_lang_t;

/**
 * Array of `c_lang` for all supported languages. The last entry is
 * `{ NULL, LANG_NONE }`.
 */
static c_lang_t const C_LANG[] = {
  //
  // If this array is modified, also check SET_OPTIONS[] in autocomplete.c.
  //
  { "CK&R",    LANG_C_KNR   },          // synonym for "knr"
  { "CKNR",    LANG_C_KNR   },          // synonym for "knr"
  { "K&R",     LANG_C_KNR   },          // synonym for "knr"
  { "K&RC",    LANG_C_KNR   },          // synonym for "knr"
  { "KNR",     LANG_C_KNR   },
  { "KNRC",    LANG_C_KNR   },          // synonym for "knr"
  { "C",       LANG_C_NEW   },
  { "C89",     LANG_C_89,   },
  { "C95",     LANG_C_95    },
  { "C99",     LANG_C_99    },
  { "C11",     LANG_C_11    },
  { "C18",     LANG_C_18    },
  { "C++",     LANG_CPP_NEW },
  { "C++98",   LANG_CPP_98  },
  { "C++03",   LANG_CPP_03  },
  { "C++11",   LANG_CPP_11  },
  { "C++14",   LANG_CPP_14  },
  { "C++17",   LANG_CPP_17  },
  { "C++20",   LANG_CPP_20  },
  { NULL,      LANG_NONE    },
};

////////// extern functions ///////////////////////////////////////////////////

c_lang_id_t c_lang_find( char const *name ) {
  assert( name != NULL );

  // the list is small, so linear search is good enough
  for ( c_lang_t const *lang = C_LANG; lang->name != NULL; ++lang ) {
    if ( strcasecmp( name, lang->name ) == 0 )
      return lang->lang_id;
  } // for

  return LANG_NONE;
}

char const* c_lang_literal( c_lang_lit_t const lang_lit[] ) {
  for ( c_lang_lit_t const *ll = lang_lit; true; ++ll ) {
    if ( (ll->lang_ids & opt_lang) != LANG_NONE )
      return ll->literal;
  } // for
}

char const* c_lang_name( c_lang_id_t lang_id ) {
  assert( exactly_one_bit_set( lang_id ) );
  switch ( lang_id ) {
    case LANG_NONE  : return "";
    case LANG_C_KNR : return "K&R C";
    case LANG_C_89  : return "C89";
    case LANG_C_95  : return "C95";
    case LANG_C_99  : return "C99";
    case LANG_C_11  : return "C11";
    case LANG_C_18  : return "C18";
    case LANG_CPP_98: return "C++98";
    case LANG_CPP_03: return "C++03";
    case LANG_CPP_11: return "C++11";
    case LANG_CPP_14: return "C++14";
    case LANG_CPP_17: return "C++17";
    case LANG_CPP_20: return "C++20";
    default:
      UNEXPECTED_INT_VALUE( lang_id );
  } // switch
}

char const* c_lang_names( void ) {
  static char *names;

  if ( names == NULL ) {
    size_t names_len = 1;               // for trailing NULL
    for ( c_lang_t const *lang = C_LANG; lang->name != NULL; ++lang ) {
      if ( lang > C_LANG )
        names_len += 2;                 // ", "
      names_len += strlen( lang->name );
    } // for

    names = FREE_STR_LATER( MALLOC( char, names_len ) );
    names[0] = '\0';

    char *s = names;
    for ( c_lang_t const *lang = C_LANG; lang->name != NULL; ++lang ) {
      if ( s > names )
        s = strcpy_end( s, ", " );
      s = strcpy_end( s, lang->name );
    } // for
  }

  return names;
}

void c_lang_set( c_lang_id_t lang_id ) {
  assert( exactly_one_bit_set( lang_id ) );
  opt_lang = lang_id;

  bool const prompt_enabled =
    opt_prompt && (
    cdecl_prompt[0] == NULL || // first time: cdecl_prompt_init() not called yet
    cdecl_prompt[0][0] != '\0');

  cdecl_prompt_init();         // change prompt based on new language
  cdecl_prompt_enable( prompt_enabled );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
